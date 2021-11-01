//
// Copyright 2021 Pixar
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
#include "pxr/imaging/hd/sceneIndex.h"

#include "pxr/base/tf/instantiateSingleton.h"

PXR_NAMESPACE_OPEN_SCOPE


HdSceneIndexBase::~HdSceneIndexBase() = default;

void
HdSceneIndexBase::AddObserver(const HdSceneIndexObserverPtr &observer)
{
    if (_observers.insert(observer).second) {
        // XXX: newly inserted...
        // should we call PrimAdded for the entire scene?
        // or should there be a mask or subscription notion?
        // Our current assumption is that the entity adding the observer
        // should know what they need to know when adding a new observer.
    }
}

void
HdSceneIndexBase::RemoveObserver(const HdSceneIndexObserverPtr &observer)
{
    _observers.erase(observer);
}

void
HdSceneIndexBase::_SendPrimsAdded(
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    if (entries.empty()) {
        return;
    }
    _ObserverSet::const_iterator it = _observers.begin();
    while (it != _observers.end()) {
        if (*it) {
            (*it)->PrimsAdded(*this, entries);
            ++it;
        } else {
            _ObserverSet::const_iterator it2 = it;
            ++it;
            _observers.erase(it2);
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
    _ObserverSet::const_iterator it = _observers.begin();
    while (it != _observers.end()) {
        if (*it) {
            (*it)->PrimsRemoved(*this, entries);
            ++it;
        } else {
            _ObserverSet::const_iterator it2 = it;
            ++it;
            _observers.erase(it2);
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
    _ObserverSet::const_iterator it = _observers.begin();
    while (it != _observers.end()) {
        if (*it) {
            (*it)->PrimsDirtied(*this, entries);
            ++it;
        } else {
            _ObserverSet::const_iterator it2 = it;
            ++it;
            _observers.erase(it2);
        }
    }
}

bool
HdSceneIndexBase::_IsObserved() const
{
    return !_observers.empty();
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
            _NamedInstanceMap::const_iterator it2 = it;
            ++it;
            _namedInstances.erase(it2);
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

