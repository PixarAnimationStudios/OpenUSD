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
#include "pxr/pxr.h"
#include "pxr/usd/usd/stageCache.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/debugCodes.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/ar/resolverContext.h"
#include "pxr/base/tf/hash.h"

#include <atomic>
#include <vector>
#include <thread>
#include <unordered_map>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE


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
    Entry() = default;
    Entry(const UsdStageRefPtr &stage, Id id) : stage(stage), id(id) {}
    UsdStageRefPtr stage;
    Id id;
};

using StagesById = std::unordered_map<Id, UsdStageRefPtr, TfHash>;
using IdsByStage = std::unordered_map<UsdStageRefPtr, Id, TfHash>;
using StagesByRootLayer = std::unordered_multimap<SdfLayerHandle,
                                                  UsdStageRefPtr,
                                                  TfHash>;

/// The stage container provides a set of synchronized maps to enable
/// lookup by Id, LayerHandle, or Stage. It provides a single insertion
/// function to populate these maps and erasure by multiple key types.
///
/// This data structure does not add any thread safety guarantees. Users should
/// lock appropriately before read and write.
class StageContainer {
public:
    StageContainer() = default;

    /// Return an unordered_map of the stages indexed by id
    const StagesById& ById() const { return _byId; }
    /// Return an unordered_map of the ids indexed by stage
    const IdsByStage& ByStage() const { return _byStage; }
    /// Return an unordered_multimap of the stages indexed by root layer
    /// NOTE: This is a multimap where multiple stages may mapped by a single
    /// root layer.
    const StagesByRootLayer& ByRootLayer() const { return _byRootLayer; }

    // If the stage is already in the cache, return the pair {Id, false}
    // If the stage is not in the cache, generate a new id, populate
    // the synchronized maps, and return {NewId, true}
    std::pair<Id, bool> Insert(const UsdStageRefPtr& stage) {
        const auto it = _byStage.find(stage);
        if (  it != _byStage.cend()) {
            return std::make_pair(it->second, false);
        }
        const Id id = GetNextId();
        TF_VERIFY(_byStage.emplace(stage, id).second);
        TF_VERIFY(_byId.emplace(id, stage).second);
        _byRootLayer.emplace(stage->GetRootLayer(), stage);
        return std::make_pair(id, true);
    }

    // Return true if the stage was successfully erased
    bool Erase(const UsdStageRefPtr& stage) {
        const auto it = _byStage.find(stage);
        if (it == _byStage.cend()) {
            return false;
        }
        _EraseRootLayerEntry(stage);
        TF_VERIFY(_byId.erase(it->second) == 1);
        _byStage.erase(it);
        return true;
    }

    // Return true if the stage was successfully erased
    bool Erase(const Id id) {
        const auto it = _byId.find(id);
        if (it == _byId.cend()) {
            return false;
        }
        _EraseRootLayerEntry(it->second);
        TF_VERIFY(_byStage.erase(it->second) == 1);
        _byId.erase(it);
        return true;
    }

    size_t EraseAll(const SdfLayerHandle& rootLayer,
                    vector<Entry>* erased) {
        return _EraseAllIf(rootLayer,
                           [](const auto& stage){ return true; },
                           erased);
    }

    size_t EraseAll(const SdfLayerHandle& rootLayer,
                    const SdfLayerHandle& sessionLayer,
                    vector<Entry>* erased) {
        return _EraseAllIf(rootLayer,
                          [&sessionLayer](const auto& stage){
                              return stage->GetSessionLayer() == sessionLayer;
                          },
                          erased);
    }

    size_t EraseAll(const SdfLayerHandle& rootLayer,
                    const SdfLayerHandle& sessionLayer,
                    const ArResolverContext& resolverContext,
                    vector<Entry>* erased) {
        return _EraseAllIf(rootLayer,
                          [&sessionLayer, &resolverContext](const auto& stage){
                              return stage->GetSessionLayer() == sessionLayer &&
                                     stage->GetPathResolverContext() == resolverContext;
                          },
                          erased);
    }

    size_t size() const { return _byId.size(); }

private:
    // Helper function to erase the ByRootLayer() entry associated with the
    // stage (and no other entries).
    //
    // This must be used in tandem with erase methods on ById() and ByStage()
    // to uphold the synchronization invariant.
    void _EraseRootLayerEntry(const UsdStageRefPtr& stage) {
        const auto range = _byRootLayer.equal_range(
            stage->GetRootLayer()
        );
        const auto it = std::find_if(
            range.first, range.second,
            [&stage](const auto& element){
                return element.second == stage;
            });
        if (it != range.second) {
            _byRootLayer.erase(it);
        } else {
            TF_CODING_ERROR(
                "Internal StageCache is out of sync."
                "Cannot find root layer entry for stage '%s'."
                "Skipping erase of incomplete element.",
                UsdDescribe(stage).c_str()
            );
        }
    }

    template <typename ConditionFn>
    size_t _EraseAllIf(const SdfLayerHandle& rootLayer,
                       ConditionFn&& conditionFn,
                       vector<Entry>* erased) {
        const auto range = _byRootLayer.equal_range(rootLayer);
        size_t erasedCount = 0;
        // In C++20, consider using a filtered range with the condition
        // function
        for (auto it = range.first; it != range.second;) {
            if (conditionFn(it->second)) {
                const auto byStageIt = _byStage.find(it->second);
                if (byStageIt == _byStage.cend()) {
                    TF_CODING_ERROR(
                        "Internal StageCache is out of sync. "
                        "Cannot locate ID for stage '%s'."
                        "Skipping erase of incomplete element.",
                        UsdDescribe(it->second).c_str());
                    ++it;
                } else {
                    if (erased) {
                        erased->emplace_back(byStageIt->first,
                                             byStageIt->second);
                    }
                    // Erase the ID => Stage map by value first and
                    // verify it succeeded.
                    TF_VERIFY(_byId.erase(byStageIt->second) == 1);
                    // Erase the Stage => ID map using the found iterator
                    _byStage.erase(byStageIt);
                    // Erase the multimap last and increment it to the
                    // next element in the equal_range
                    it = _byRootLayer.erase(it);
                    ++erasedCount;
                }
            } else {
                ++it;
            }
        }
        return erasedCount;
    }

    StagesById _byId;
    IdsByStage _byStage;
    StagesByRootLayer _byRootLayer;
};

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

    void AddEntry(const UsdStageRefPtr& stagePtr, const Id id) {
        if (IsEnabled()) {
            _entries.emplace_back(stagePtr, id);
        }
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
        return IsEnabled() ? &_entries : nullptr;
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
        while (state == 1) {
            std::this_thread::yield();
        }
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
    const IdsByStage &byStage = _impl->stages.ByStage();
    vector<UsdStageRefPtr> result(_impl->stages.size());
    std::transform(byStage.cbegin(), byStage.cend(), result.begin(),
                   [](const auto& entry) { return entry.first; });
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
        {
            const IdsByStage &byStage = _impl->stages.ByStage();
            for (auto const &element: byStage) {
                if (request.IsSatisfiedBy(element.first)) {
                    return std::make_pair(element.first, false);
                }
            }
        }

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
        const StagesById& byId = _impl->stages.ById();
        StagesById::const_iterator iter = byId.find(id);
        result = iter != byId.end() ? iter->second : TfNullPtr;
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
        const StagesByRootLayer& byRootLayer = _impl->stages.ByRootLayer();
        auto iter = byRootLayer.find(rootLayer);
        result = iter != byRootLayer.end() ? iter->second : TfNullPtr;
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
        const StagesByRootLayer &byRootLayer = _impl->stages.ByRootLayer();
        auto range = byRootLayer.equal_range(rootLayer);
        for (auto entryIt = range.first; entryIt != range.second; ++entryIt) { 
            const auto& entry = *entryIt;
            if (entry.second->GetSessionLayer() == sessionLayer) {
                result = entry.second;
                break;
            }
        }
    }

    DBG("%s by rootLayer%s, sessionLayer%s in %s\n",
        (result ? FMT("found %s", UsdDescribe(result).c_str())
                : "failed to find stage"),
        (result ? "" : FMT(" @%s@", rootLayer->GetIdentifier().c_str())),
        (result ? "" : (!sessionLayer ? " <null>" :
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
        const StagesByRootLayer &byRootLayer = _impl->stages.ByRootLayer();
        auto range = byRootLayer.equal_range(rootLayer);
        for (auto entryIt = range.first; entryIt != range.second; ++entryIt) { 
            const auto& entry = *entryIt;
            if (entry.second->GetPathResolverContext() == pathResolverContext) {
                result = entry.second;
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
        const StagesByRootLayer &byRootLayer = _impl->stages.ByRootLayer();
        auto range = byRootLayer.equal_range(rootLayer);
        for (auto entryIt = range.first; entryIt != range.second; ++entryIt) { 
            const auto& entry = *entryIt;
            if (entry.second->GetSessionLayer() == sessionLayer &&
                entry.second->GetPathResolverContext() == pathResolverContext) {
                result = entry.second;
                break;
            }
        }
    }

    DBG("%s by rootLayer%s, sessionLayer%s, pathResolverContext in %s\n",
        (result ? FMT("found %s", UsdDescribe(result).c_str())
                : "failed to find stage"),
        (result ? "" : FMT(" @%s@", rootLayer->GetIdentifier().c_str())),
        (result ? "" : (!sessionLayer ? " <null>" :
                        FMT(" @%s@", sessionLayer->GetIdentifier().c_str()))),
        UsdDescribe(*this).c_str());

    return result;
}

std::vector<UsdStageRefPtr>
UsdStageCache::FindAllMatching(const SdfLayerHandle &rootLayer) const
{
    LockGuard lock(_mutex);
    const StagesByRootLayer &byRootLayer = _impl->stages.ByRootLayer();
    vector<UsdStageRefPtr> result;
    auto range = byRootLayer.equal_range(rootLayer);
    for (auto entryIt = range.first; entryIt != range.second; ++entryIt) { 
         const auto& entry = *entryIt;
        result.push_back(entry.second);
    }
    return result;
}

std::vector<UsdStageRefPtr>
UsdStageCache::FindAllMatching(const SdfLayerHandle &rootLayer,
                               const SdfLayerHandle &sessionLayer) const
{
    LockGuard lock(_mutex);
    const StagesByRootLayer &byRootLayer = _impl->stages.ByRootLayer();
    vector<UsdStageRefPtr> result;
    auto range = byRootLayer.equal_range(rootLayer);
    for (auto entryIt = range.first; entryIt != range.second; ++entryIt) { 
        const auto& entry = *entryIt;
        if (entry.second->GetSessionLayer() == sessionLayer)
            result.push_back(entry.second);
    }
    return result;
}

std::vector<UsdStageRefPtr>
UsdStageCache::FindAllMatching(
    const SdfLayerHandle &rootLayer,
    const ArResolverContext &pathResolverContext) const
{
    LockGuard lock(_mutex);
    const StagesByRootLayer &byRootLayer = _impl->stages.ByRootLayer();
    vector<UsdStageRefPtr> result;
    auto range = byRootLayer.equal_range(rootLayer);
    for (auto entryIt = range.first; entryIt != range.second; ++entryIt) { 
        const auto& entry = *entryIt;
        if (entry.second->GetPathResolverContext() == pathResolverContext)
            result.push_back(entry.second);
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
    const StagesByRootLayer &byRootLayer = _impl->stages.ByRootLayer();
    vector<UsdStageRefPtr> result;
    auto range = byRootLayer.equal_range(rootLayer);
    for (auto entryIt = range.first; entryIt != range.second; ++entryIt) { 
        const auto& entry = *entryIt;
        if (entry.second->GetSessionLayer() == sessionLayer &&
            entry.second->GetPathResolverContext() == pathResolverContext) {
            result.push_back(entry.second);
        }
    }
    return result;
}

UsdStageCache::Id
UsdStageCache::GetId(const UsdStageRefPtr &stage) const
{
    LockGuard lock(_mutex);
    const IdsByStage& byStage = _impl->stages.ByStage();
    auto iter = byStage.find(stage);
    return iter != byStage.end() ? iter->second : Id();
}

UsdStageCache::Id
UsdStageCache::Insert(const UsdStageRefPtr &stage)
{
    if (!stage) {
        TF_CODING_ERROR("Inserted null stage in cache");
        return Id();
    }

    DebugHelper debug(*this, "inserted");
    Id ret;

    { LockGuard lock(_mutex);
        auto iresult = _impl->stages.Insert(stage);
        if (iresult.second && debug.IsEnabled()) {
            debug.AddEntry(stage, iresult.first);
        }
        ret = iresult.first;
    }
    return ret;
}

bool
UsdStageCache::Erase(Id id)
{
    DebugHelper debug(*this, "erased");
    bool result;
    { LockGuard lock(_mutex);
        if (debug.IsEnabled()) {
            const StagesById& stagesById = _impl->stages.ById();
            const auto it = stagesById.find(id);
            if (it != stagesById.end()){
                debug.AddEntry(it->second, it->first);
            }
        }
        result = _impl->stages.Erase(id);
    }
    return result;
}

bool
UsdStageCache::Erase(const UsdStageRefPtr &stage)
{
    DebugHelper debug(*this, "erased");
    bool result;
    { LockGuard lock(_mutex);
        if (debug.IsEnabled()) {
            const IdsByStage& idsByStage = _impl->stages.ByStage();
            const auto it = idsByStage.find(stage);
            if (it != idsByStage.end()) {
                debug.AddEntry(it->first, it->second);
            }
        }
        result = _impl->stages.Erase(stage);
    }
    return result;
}

size_t
UsdStageCache::EraseAll(const SdfLayerHandle &rootLayer)
{
    DebugHelper debug(*this, "erased");
    size_t result;
    { LockGuard lock(_mutex);
        result = _impl->stages.EraseAll(rootLayer, debug.GetEntryVec());
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
        numErased = _impl->stages.EraseAll(
            rootLayer, sessionLayer, debug.GetEntryVec()
        );
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
        numErased = _impl->stages.EraseAll(
            rootLayer, sessionLayer, pathResolverContext, debug.GetEntryVec()
        );
    }
    return numErased;
}


void
UsdStageCache::Clear()
{
    DebugHelper debug(*this, "cleared");

    UsdStageCache tmp;
    { LockGuard lock(_mutex);
        if (debug.IsEnabled()) {
            const IdsByStage& idsByStage = _impl->stages.ByStage();
            for (const auto& element : idsByStage) {
                debug.AddEntry(element.first, element.second);
            }
        }
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

PXR_NAMESPACE_CLOSE_SCOPE

