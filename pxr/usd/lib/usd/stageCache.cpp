//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/usd/usd/stageCache.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/debugCodes.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/ar/resolverContext.h"

#include "pxr/base/arch/nap.h"

#include <boost/bind.hpp>
#include <boost/functional/hash.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/global_fun.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

#include <atomic>
#include <vector>
#include <utility>

using std::string;
using std::vector;
using std::pair;

using LockGuard = std::lock_guard<std::mutex>;

#define DBG TF_DEBUG(USD_STAGE_CACHE).Msg
#define FMT(...) (TfStringPrintf(__VA_ARGS__).c_str())

namespace {

typedef UsdStageCache::Id Id;

std::atomic_long idCounter(9223000);

Id GetNextId() { return Id::FromLongInt(++idCounter); }

struct Entry {
    Entry() {}
    Entry(const UsdStageRefPtr &stage, Id id) : stage(stage), id(id) {}
    UsdStageRefPtr stage;
    Id id;
};

struct ById {};

struct ByStage {};

struct ByRootLayer {
    static SdfLayerHandle Get(const Entry &entry) {
        return entry.stage->GetRootLayer();
    }
};

typedef boost::multi_index::multi_index_container<

    Entry,

    boost::multi_index::indexed_by<

        boost::multi_index::hashed_unique<
            boost::multi_index::tag<ById>,
            boost::multi_index::member<Entry, Id, &Entry::id>
            >,

        boost::multi_index::hashed_unique<
            boost::multi_index::tag<ByStage>,
            boost::multi_index::member<Entry, UsdStageRefPtr, &Entry::stage>
            >,

        boost::multi_index::hashed_non_unique<
            boost::multi_index::tag<ByRootLayer>,
            boost::multi_index::global_fun<
                const Entry &, SdfLayerHandle, &ByRootLayer::Get>
            >
        >
    > StageContainer;

typedef StageContainer::index<ById>::type StagesById;
typedef StageContainer::index<ByStage>::type StagesByStage;
typedef StageContainer::index<ByRootLayer>::type StagesByRootLayer;

// Walk range, which must be from index, applying pred() to every element.  For
// those elements where pred(element) is true, erase the element from the index
// and invoke out->push_back(element) if out is not null.  Return the number of
// elements erased.
template <class Index, class Range, class Pred, class BackIns>
size_t EraseIf(Index &index, Range range, Pred pred, BackIns *out) {
    size_t numErased = 0;
    while (range.first != range.second) {
        if (pred(*range.first)) {
            if (out)
                out->push_back(*range.first);
            index.erase(range.first++);
            ++numErased;
        } else {
            ++range.first;
        }
    }
    return numErased;
}

struct DebugHelper
{
    explicit DebugHelper(const UsdStageCache &cache, const char *prefix = "")
        : _cache(cache)
        , _prefix(prefix)
        , _enabled(TfDebug::IsEnabled(USD_STAGE_CACHE)) {}

    ~DebugHelper() {
        if (IsEnabled())
            IssueMessage();
    }

    bool IsEnabled() const { return _enabled; }

    template <class Range>
    void AddEntries(const Range &rng) {
        if (IsEnabled())
            _entries.insert(_entries.end(), boost::begin(rng), boost::end(rng));
    }

    void AddEntry(const Entry &entry) {
        if (IsEnabled())
            _entries.push_back(entry);
    }

    void IssueMessage() const {
        if (_entries.size() == 1) {
            DBG("%s %s %s (id=%s)\n",
                UsdDescribe(_cache).c_str(),
                _prefix,
                UsdDescribe(_entries.front().stage).c_str(),
                _entries.front().id.ToString().c_str());
        } else if (_entries.size() > 1) {
            DBG("%s %s %zu entries:\n",
                UsdDescribe(_cache).c_str(), _prefix, _entries.size());
            for (auto const &entry: _entries) {
                DBG("      %s (id=%s)\n", UsdDescribe(entry.stage).c_str(),
                    entry.id.ToString().c_str());
            }
        }
    }

    vector<Entry> *GetEntryVec() {
        return IsEnabled() ? &_entries : NULL;
    }

private:
    vector<Entry> _entries;
    const UsdStageCache &_cache;
    const char *_prefix;
    bool _enabled;
};

} // anonymous ns

struct UsdStageCacheRequest::_Mailbox {
    _Mailbox() : state(0) {}

    UsdStageRefPtr Wait() {
        while (state == 1)
            ArchThreadYield();
        return stage;
    }

    bool Subscribed() const { return state > 0; }            

    std::atomic_int state; // 0: unsubscribed, 1: subscribed, 2: delivered.
    UsdStageRefPtr stage;
};

struct UsdStageCacheRequest::_Data
{
    std::vector<_Mailbox *> subscribed;
};

void
UsdStageCacheRequest::_DataDeleter::operator()(UsdStageCacheRequest::_Data *d)
{
    delete d;
}

struct Usd_StageCacheImpl
{
    StageContainer stages;
    std::vector<UsdStageCacheRequest *> pendingRequests;
    string debugName;
};

UsdStageCache::UsdStageCache() : _impl(new _Impl)
{
}

UsdStageCache::UsdStageCache(const UsdStageCache &other)
{
    LockGuard lock(other._mutex);
    _impl.reset(new _Impl(*other._impl));
}

UsdStageCache::~UsdStageCache()
{
}

void
UsdStageCache::swap(UsdStageCache &other)
{
    if (this != &other) {
        {
            LockGuard lockThis(_mutex), lockOther(other._mutex);
            _impl.swap(other._impl);
        }
        DBG("swapped %s with %s\n",
            UsdDescribe(*this).c_str(), UsdDescribe(other).c_str());
    }
}

UsdStageCache &
UsdStageCache::operator=(const UsdStageCache &other)
{
    if (this != &other) {
        DBG("assigning %s from %s\n",
            UsdDescribe(*this).c_str(), UsdDescribe(other).c_str());
        UsdStageCache tmp(other);
        LockGuard lock(_mutex);
        _impl.swap(tmp._impl);
    }
    return *this;
}

vector<UsdStageRefPtr>
UsdStageCache::GetAllStages() const
{
    LockGuard lock(_mutex);
    StagesByStage &byStage = _impl->stages.get<ByStage>();
    vector<UsdStageRefPtr> result;
    result.reserve(_impl->stages.size());
    for (auto const &entry: byStage)
        result.push_back(entry.stage);
    return result;
}

size_t
UsdStageCache::Size() const
{
    LockGuard lock(_mutex);
    return _impl->stages.size();
}

std::pair<UsdStageRefPtr, bool>
UsdStageCache::RequestStage(UsdStageCacheRequest &&request)
{
    UsdStageCacheRequest::_Mailbox mailbox;

    { LockGuard lock(_mutex);
        // Does the cache currently have a match?  If so, done.
        StagesByStage &byStage = _impl->stages.get<ByStage>();
        for (auto const &entry: byStage)
            if (request.IsSatisfiedBy(entry.stage))
                return std::make_pair(entry.stage, false);

        // Check to see if any pending requests can satisfy.
        for (auto *pending: _impl->pendingRequests) {
            if (request.IsSatisfiedBy(*pending)) {
                // Subscribe to the request so it delivers us a stage.
                pending->_Subscribe(&mailbox);
                break;
            }
        }

        // If we didn't subscribe to a pending request, then we will become a
        // pending request and load the stage.
        if (!mailbox.Subscribed())
            _impl->pendingRequests.push_back(&request);

        // Drop the lock to wait for the pending request or to load the stage.
    }

    // If we subscribed to another pending request, just wait for it.
    if (mailbox.Subscribed())
        return std::make_pair(mailbox.Wait(), false);

    // We are a pending request -- manufacture a stage.  If manufacturing fails,
    // issue an error only if the manufacturing process didn't issue its own.
    TfErrorMark m;
    UsdStageRefPtr stage = request.Manufacture();
    if (!stage && m.IsClean()) {
        TF_RUNTIME_ERROR("UsdStageCacheRequest failed to manufacture a valid "
                         "stage.");
    }

    // If we successfully instantiated a stage, insert it into the cache.
    if (stage)
        Insert(stage);

    // We have to deliver our stage to all the subscribed mailboxes, even if our
    // stage is null.
    { LockGuard lock(_mutex);
        if (request._data) {
            for (auto *mbox: request._data->subscribed) {
                mbox->stage = stage;
                mbox->state = 2; // delivered - this unblocks Wait()ing mboxes.
            }
        }
        // Remove this request as a pending request.
        _impl->pendingRequests.erase(
            std::remove(_impl->pendingRequests.begin(),
                        _impl->pendingRequests.end(), &request),
            _impl->pendingRequests.end());
    }

    return std::make_pair(stage, true);
}

UsdStageRefPtr
UsdStageCache::Find(Id id) const
{
    UsdStageRefPtr result;
    { LockGuard lock(_mutex);
        StagesById &byId = _impl->stages.get<ById>();
        StagesById::const_iterator iter = byId.find(id);
        result = iter != byId.end() ? iter->stage : TfNullPtr;
    }

    DBG("%s for id=%s in %s\n",
        (result ? FMT("found %s", UsdDescribe(result).c_str())
                : "failed to find stage"),
        id.ToString().c_str(), UsdDescribe(*this).c_str());

    return result;
}

UsdStageRefPtr
UsdStageCache::FindOneMatching(const SdfLayerHandle &rootLayer) const
{
    UsdStageRefPtr result;
    { LockGuard lock(_mutex);
        StagesByRootLayer &byRootLayer = _impl->stages.get<ByRootLayer>();
        auto iter = byRootLayer.find(rootLayer);
        result = iter != byRootLayer.end() ? iter->stage : TfNullPtr;
    }

    DBG("%s by rootLayer%s in %s\n",
        (result ? FMT("found %s", UsdDescribe(result).c_str())
                : "failed to find stage"),
        (result ? "" : FMT(" @%s@", rootLayer->GetIdentifier().c_str())),
        UsdDescribe(*this).c_str());

    return result;
}

UsdStageRefPtr
UsdStageCache::FindOneMatching(const SdfLayerHandle &rootLayer,
                                const SdfLayerHandle &sessionLayer) const
{
    UsdStageRefPtr result;
    { LockGuard lock(_mutex);
        StagesByRootLayer &byRootLayer = _impl->stages.get<ByRootLayer>();
        auto range = byRootLayer.equal_range(rootLayer);
        for (auto entryIt = range.first; entryIt != range.second; ++entryIt) { 
            const auto& entry = *entryIt;
            if (entry.stage->GetSessionLayer() == sessionLayer) {
                result = entry.stage;
                break;
            }
        }
    }

    DBG("%s by rootLayer%s, sessionLayer%s in %s\n",
        (result ? FMT("found %s", UsdDescribe(result).c_str())
                : "failed to find stage"),
        (result ? "" : FMT(" @%s@", rootLayer->GetIdentifier().c_str())),
        (result ? "" : (not sessionLayer ? " <null>" :
                        FMT(" @%s@", sessionLayer->GetIdentifier().c_str()))),
        UsdDescribe(*this).c_str());

    return result;
}

UsdStageRefPtr
UsdStageCache::FindOneMatching(
    const SdfLayerHandle &rootLayer,
    const ArResolverContext &pathResolverContext) const
{
    UsdStageRefPtr result;
    { LockGuard lock(_mutex);
        StagesByRootLayer &byRootLayer = _impl->stages.get<ByRootLayer>();
        auto range = byRootLayer.equal_range(rootLayer);
        for (auto entryIt = range.first; entryIt != range.second; ++entryIt) { 
            const auto& entry = *entryIt;
            if (entry.stage->GetPathResolverContext() == pathResolverContext) {
                result = entry.stage;
                break;
            }
        }
    }

    DBG("%s by rootLayer%s, pathResolverContext in %s\n",
        (result ? FMT("found %s", UsdDescribe(result).c_str())
                : "failed to find stage"),
        (result ? "" : FMT(" @%s@", rootLayer->GetIdentifier().c_str())),
        UsdDescribe(*this).c_str());

    return result;
}

UsdStageRefPtr
UsdStageCache::FindOneMatching(
    const SdfLayerHandle &rootLayer,
    const SdfLayerHandle &sessionLayer,
    const ArResolverContext &pathResolverContext) const
{
    UsdStageRefPtr result;
    { LockGuard lock(_mutex);
        StagesByRootLayer &byRootLayer = _impl->stages.get<ByRootLayer>();
        auto range = byRootLayer.equal_range(rootLayer);
        for (auto entryIt = range.first; entryIt != range.second; ++entryIt) { 
            const auto& entry = *entryIt;
            if (entry.stage->GetSessionLayer() == sessionLayer and
                entry.stage->GetPathResolverContext() == pathResolverContext) {
                result = entry.stage;
                break;
            }
        }
    }

    DBG("%s by rootLayer%s, sessionLayer%s, pathResolverContext in %s\n",
        (result ? FMT("found %s", UsdDescribe(result).c_str())
                : "failed to find stage"),
        (result ? "" : FMT(" @%s@", rootLayer->GetIdentifier().c_str())),
        (result ? "" : (not sessionLayer ? " <null>" :
                        FMT(" @%s@", sessionLayer->GetIdentifier().c_str()))),
        UsdDescribe(*this).c_str());

    return result;
}

std::vector<UsdStageRefPtr>
UsdStageCache::FindAllMatching(const SdfLayerHandle &rootLayer) const
{
    LockGuard lock(_mutex);
    StagesByRootLayer &byRootLayer = _impl->stages.get<ByRootLayer>();
    vector<UsdStageRefPtr> result;
    auto range = byRootLayer.equal_range(rootLayer);
    for (auto entryIt = range.first; entryIt != range.second; ++entryIt) { 
         const auto& entry = *entryIt;
        result.push_back(entry.stage);
    }
    return result;
}

std::vector<UsdStageRefPtr>
UsdStageCache::FindAllMatching(const SdfLayerHandle &rootLayer,
                               const SdfLayerHandle &sessionLayer) const
{
    LockGuard lock(_mutex);
    StagesByRootLayer &byRootLayer = _impl->stages.get<ByRootLayer>();
    vector<UsdStageRefPtr> result;
    auto range = byRootLayer.equal_range(rootLayer);
    for (auto entryIt = range.first; entryIt != range.second; ++entryIt) { 
        const auto& entry = *entryIt;
        if (entry.stage->GetSessionLayer() == sessionLayer)
            result.push_back(entry.stage);
    }
    return result;
}

std::vector<UsdStageRefPtr>
UsdStageCache::FindAllMatching(
    const SdfLayerHandle &rootLayer,
    const ArResolverContext &pathResolverContext) const
{
    LockGuard lock(_mutex);
    StagesByRootLayer &byRootLayer = _impl->stages.get<ByRootLayer>();
    vector<UsdStageRefPtr> result;
    auto range = byRootLayer.equal_range(rootLayer);
    for (auto entryIt = range.first; entryIt != range.second; ++entryIt) { 
        const auto& entry = *entryIt;
        if (entry.stage->GetPathResolverContext() == pathResolverContext)
            result.push_back(entry.stage);
    }
    return result;
}

std::vector<UsdStageRefPtr>
UsdStageCache::FindAllMatching(
    const SdfLayerHandle &rootLayer,
    const SdfLayerHandle &sessionLayer,
    const ArResolverContext &pathResolverContext) const
{
    LockGuard lock(_mutex);
    StagesByRootLayer &byRootLayer = _impl->stages.get<ByRootLayer>();
    vector<UsdStageRefPtr> result;
    auto range = byRootLayer.equal_range(rootLayer);
    for (auto entryIt = range.first; entryIt != range.second; ++entryIt) { 
        const auto& entry = *entryIt;
        if (entry.stage->GetSessionLayer() == sessionLayer and
            entry.stage->GetPathResolverContext() == pathResolverContext) {
            result.push_back(entry.stage);
        }
    }
    return result;
}

UsdStageCache::Id
UsdStageCache::GetId(const UsdStageRefPtr &stage) const
{
    LockGuard lock(_mutex);
    StagesByStage &byStage = _impl->stages.get<ByStage>();
    auto iter = byStage.find(stage);
    return iter != byStage.end() ? iter->id : Id();
}

UsdStageCache::Id
UsdStageCache::Insert(const UsdStageRefPtr &stage)
{
    if (not stage) {
        TF_CODING_ERROR("Inserted null stage in cache");
        return Id();
    }

    DebugHelper debug(*this, "inserted");
    Id ret;

    { LockGuard lock(_mutex);
        StagesByStage &byStage = _impl->stages.get<ByStage>();
        auto iresult = byStage.insert(Entry(stage, GetNextId()));
        if (iresult.second and debug.IsEnabled())
            debug.AddEntry(*iresult.first);
        ret = iresult.first->id;
    }
    return ret;
}

bool
UsdStageCache::Erase(Id id)
{
    DebugHelper debug(*this, "erased");
    bool result;
    { LockGuard lock(_mutex);
        if (debug.IsEnabled())
            debug.AddEntries(_impl->stages.get<ById>().equal_range(id));
        result = _impl->stages.get<ById>().erase(id);
    }
    return result;
}

bool
UsdStageCache::Erase(const UsdStageRefPtr &stage)
{
    DebugHelper debug(*this, "erased");
    bool result;
    { LockGuard lock(_mutex);
        if (debug.IsEnabled())
            debug.AddEntries(_impl->stages.get<ByStage>().equal_range(stage));
        result = _impl->stages.get<ByStage>().erase(stage);
    }
    return result;
}

size_t
UsdStageCache::EraseAll(const SdfLayerHandle &rootLayer)
{
    DebugHelper debug(*this, "erased");
    size_t result;
    { LockGuard lock(_mutex);
        if (debug.IsEnabled()) {
            debug.AddEntries(
                _impl->stages.get<ByRootLayer>().equal_range(rootLayer));
        }
        result = _impl->stages.get<ByRootLayer>().erase(rootLayer);
    }
    return result;
}

size_t
UsdStageCache::EraseAll(const SdfLayerHandle &rootLayer,
                         const SdfLayerHandle &sessionLayer)
{
    DebugHelper debug(*this, "erased");
    size_t numErased;
    { LockGuard lock(_mutex);
        StagesByRootLayer &byRootLayer = _impl->stages.get<ByRootLayer>();
        numErased =
            EraseIf(byRootLayer, byRootLayer.equal_range(rootLayer),
                    bind(&UsdStage::GetSessionLayer,
                         bind(&Entry::stage, _1)) == sessionLayer,
                    debug.GetEntryVec());
    }
    return numErased;
}

size_t
UsdStageCache::EraseAll(const SdfLayerHandle &rootLayer,
                         const SdfLayerHandle &sessionLayer,
                         const ArResolverContext &pathResolverContext)
{
    DebugHelper debug(*this, "erased");
    size_t numErased;
    { LockGuard lock(_mutex);
        StagesByRootLayer &byRootLayer = _impl->stages.get<ByRootLayer>();
        numErased =
            EraseIf(byRootLayer, byRootLayer.equal_range(rootLayer),
                    (bind(&UsdStage::GetSessionLayer,
                          bind(&Entry::stage, _1)) == sessionLayer) and
                    (bind(&UsdStage::GetPathResolverContext,
                          bind(&Entry::stage, _1)) == pathResolverContext),
                    debug.GetEntryVec());
    }
    return numErased;
}


void
UsdStageCache::Clear()
{
    DebugHelper debug(*this, "cleared");

    UsdStageCache tmp;
    { LockGuard lock(_mutex);
        if (debug.IsEnabled())
            debug.AddEntries(_impl->stages.get<ByStage>());
        _impl.swap(tmp._impl);
    }
}

void
UsdStageCache::SetDebugName(const string &debugName)
{
    LockGuard lock(_mutex);
    _impl->debugName = debugName;
}

string
UsdStageCache::GetDebugName() const
{
    LockGuard lock(_mutex);
    return _impl->debugName;
}

string
UsdDescribe(const UsdStageCache &cache)
{
    return TfStringPrintf("stage cache %s (size=%zu)",
                          (cache.GetDebugName().empty()
                           ? FMT("%p", &cache)
                           : FMT("\"%s\"", cache.GetDebugName().c_str())),
                          cache.Size());
}

UsdStageCacheRequest::~UsdStageCacheRequest()
{
}

void
UsdStageCacheRequest::_Subscribe(_Mailbox *mailbox)
{
    if (!_data)
        _data.reset(new _Data);
    _data->subscribed.push_back(mailbox);
    mailbox->state = 1; // subscribed.
}
