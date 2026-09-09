//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/denseHashSet.h"
#include "pxr/base/trace/trace.h"

#include <typeinfo>

PXR_NAMESPACE_OPEN_SCOPE

struct HdSceneIndexBase::_NotifyScope
{
    HdSceneIndexBase *_sceneIndex;

    _NotifyScope(HdSceneIndexBase *si)
        : _sceneIndex(si)
    {
        ++_sceneIndex->_notifyDepth;
    }

    ~_NotifyScope()
    {
        --_sceneIndex->_notifyDepth;
        if (_sceneIndex->_notifyDepth == 0
            && _sceneIndex->_shouldRemoveExpiredObservers) {
            _sceneIndex->_RemoveExpiredObservers();
        }
    }
};

HdSceneIndexBase::HdSceneIndexBase()
    : _notifyDepth(0)
    , _shouldRemoveExpiredObservers(false)
{
}

HdSceneIndexBase::~HdSceneIndexBase() = default;

void
HdSceneIndexBase::AddObserver(const HdSceneIndexObserverPtr &observer)
{
    if (std::find(_observers.begin(), _observers.end(), observer)
        != _observers.end()) {
        TF_CODING_ERROR("Observer is already registered");
        return;
    }

    _observers.push_back(observer);
}

void
HdSceneIndexBase::RemoveObserver(const HdSceneIndexObserverPtr &observer)
{
    _Observers::iterator i =
        std::find(_observers.begin(), _observers.end(), observer);
    if (i != _observers.end()) {
        if (_notifyDepth == 0) {
            _observers.erase(i);
        } else {
            // Observer notification is underway, so to avoid disrupting
            // traversal, zero out the entry and flag it for removal.
            *i = nullptr;
            _shouldRemoveExpiredObservers = true;
        }
    }
}

void
HdSceneIndexBase::_RemoveExpiredObservers()
{
    if (_notifyDepth == 0) {
        _observers.erase(
            std::remove_if(
                _observers.begin(), _observers.end(),
                [](const HdSceneIndexObserverPtr &observer) {
                    return !observer;
                }),
            _observers.end());
        _shouldRemoveExpiredObservers = false;
    }
}

void
HdSceneIndexBase::_SendPrimsAdded(
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    if (entries.empty()) {
        return;
    }

    _NotifyScope scope(this);

    // Observers may be added during notification, so capture the
    // initial count upfront, and use indexing rather than iterators
    // (which might become invalidated).
    const size_t num = _observers.size();
    for (size_t i=0; i < num; ++i) {
        if (const HdSceneIndexObserverPtr &observer = _observers[i]) {
            observer->PrimsAdded(*this, entries);
        } else {
            _shouldRemoveExpiredObservers = true;
        }
    }
}

void
HdSceneIndexBase::_SendPrimsRemoved(
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    if (entries.empty()) {
        return;
    }

    _NotifyScope scope(this);

    // See note in _SendPrimsAdded.
    const size_t num = _observers.size();
    for (size_t i=0; i < num; ++i) {
        if (const HdSceneIndexObserverPtr &observer = _observers[i]) {
            observer->PrimsRemoved(*this, entries);
        } else {
            _shouldRemoveExpiredObservers = true;
        }
    }
}

void
HdSceneIndexBase::_SendPrimsDirtied(
    const HdSceneIndexObserver::DirtiedPrimEntries & entries)
{
    if (entries.empty()) {
        return;
    }

    _NotifyScope scope(this);

    // See note in _SendPrimsAdded.
    const size_t num = _observers.size();
    for (size_t i=0; i < num; ++i) {
        if (const HdSceneIndexObserverPtr &observer = _observers[i]) {
            observer->PrimsDirtied(*this, entries);
        } else {
            _shouldRemoveExpiredObservers = true;
        }
    }
}

void
HdSceneIndexBase::_SendPrimsRenamed(
    const HdSceneIndexObserver::RenamedPrimEntries & entries)
{
    if (entries.empty()) {
        return;
    }

    _NotifyScope scope(this);

    // See note in _SendPrimsAdded.
    const size_t num = _observers.size();
    for (size_t i=0; i < num; ++i) {
        if (const HdSceneIndexObserverPtr &observer = _observers[i]) {
            observer->PrimsRenamed(*this, entries);
        } else {
            _shouldRemoveExpiredObservers = true;
        }
    }
}

bool
HdSceneIndexBase::_IsObserved() const
{
    return !_observers.empty();
}

void
HdSceneIndexBase::SystemMessage(
    const TfToken &messageType,
    const HdDataSourceBaseHandle &args)
{
    TRACE_FUNCTION();

    // Build a depth-first traversal order of the scene index graph.
    // The graph may branch widely and re-converge, so avoid visiting
    // nodes multiple times through different paths.
    //
    // XXX Graph structure changes infrequently, so consider caching
    // this traversal order.  We would need a way to invalidate it
    // when the graph structure changes.
    //
    TfSmallVector<HdSceneIndexBase*, 128> order;
    TfDenseHashSet<HdSceneIndexBase*, TfHash> visitedSet;
    order.push_back(this);
    for (size_t i=0; i < order.size(); ++i) {
        HdSceneIndexBase *si = order[i];

        // Visit inputs of filtering scene indexes.
        if (const HdFilteringSceneIndexBase * const fsi =
                dynamic_cast<const HdFilteringSceneIndexBase*>(si)) {
            for (const auto &si : fsi->GetInputScenes()) {
                HdSceneIndexBase* siPtr = si.operator->();
                if (visitedSet.insert(siPtr).second) {
                    order.push_back(siPtr);
                }
            }
        }
        
        // Visit scene indexes inside encapsulating scenes.
        if (const HdEncapsulatingSceneIndexBase * const esi =
                dynamic_cast<const HdEncapsulatingSceneIndexBase*>(si)) {
            for (const auto &si: esi->GetEncapsulatedScenes()) {
                HdSceneIndexBase* siPtr = si.operator->();
                if (visitedSet.insert(siPtr).second) {
                    order.push_back(siPtr);
                }
            }
        }
    }

    // Dispatch system message to each scene index.
    //
    // XXX Unsure whether order matters here; maybe system messages were
    // intended to be idempotent?  Preserving existing behavior for now,
    // i.e. upstream scene indexes before downstream observers.
    for (size_t i=0; i < order.size(); ++i) {
        order[order.size()-1-i]->_SystemMessage(messageType, args);
    }
}

void HdSceneIndexBase::_SystemMessage(
    const TfToken &messageType,
    const HdDataSourceBaseHandle &args)
{
    // no behavior for base
}

std::string
HdSceneIndexBase::GetDisplayName() const
{
    if (!_displayName.empty()) {
        return _displayName;
    }

    return ArchGetDemangled(typeid(*this).name());
}

void
HdSceneIndexBase::SetDisplayName(const std::string &n)
{
    _displayName = n;
}

void
HdSceneIndexBase::AddTag(const TfToken &tag)
{
    _tags.insert(tag);
}

void
HdSceneIndexBase::RemoveTag(const TfToken &tag)
{
    _tags.erase(tag);
}

bool
HdSceneIndexBase::HasTag(const TfToken &tag) const
{
    return _tags.find(tag) != _tags.end();
}

TfTokenVector
HdSceneIndexBase::GetTags() const
{
    return TfTokenVector(_tags.begin(), _tags.end());
}


// ----------------------------------------------------------------------------

TF_INSTANTIATE_SINGLETON(HdSceneIndexNameRegistry);

void
HdSceneIndexNameRegistry::RegisterNamedSceneIndex(
    const std::string &name, HdSceneIndexBasePtr instance)
{
    _namedInstances[name] = instance;
}

std::vector<std::string>
HdSceneIndexNameRegistry::GetRegisteredNames()
{
    std::vector<std::string> result;
    result.reserve(_namedInstances.size());

    _NamedInstanceMap::const_iterator it = _namedInstances.begin();
    while (it != _namedInstances.end()) {
        if (it->second) {
            result.push_back(it->first);
            ++it;
        } else {
            it = _namedInstances.erase(it);
        }
    }

    return result;
}

HdSceneIndexBaseRefPtr
HdSceneIndexNameRegistry::GetNamedSceneIndex(const std::string &name)
{
    _NamedInstanceMap::const_iterator it = _namedInstances.find(name);
    if (it == _namedInstances.end()) {
        return nullptr;
    }

    if (it->second) {
        return it->second;
    } else {
        _namedInstances.erase(it);
    }

    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE

