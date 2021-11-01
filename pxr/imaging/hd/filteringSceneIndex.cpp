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
#include "pxr/imaging/hd/filteringSceneIndex.h"

#include "pxr/base/tf/instantiateSingleton.h"

PXR_NAMESPACE_OPEN_SCOPE

HdSingleInputFilteringSceneIndexBase::HdSingleInputFilteringSceneIndexBase(
        const HdSceneIndexBaseRefPtr &inputSceneIndex)
: _inputSceneIndex(inputSceneIndex)
, _observer(this)
{
    if (inputSceneIndex) {
        inputSceneIndex->AddObserver(HdSceneIndexObserverPtr(&_observer));
    }
}

std::vector<HdSceneIndexBaseRefPtr>
HdSingleInputFilteringSceneIndexBase::GetInputScenes() const
{
    return {_inputSceneIndex};
}

void
HdSingleInputFilteringSceneIndexBase::_Observer::PrimsAdded(
        const HdSceneIndexBase &sender,
        const AddedPrimEntries &entries)
{
    _owner->_PrimsAdded(sender, entries);
}

void
HdSingleInputFilteringSceneIndexBase::_Observer::PrimsRemoved(
        const HdSceneIndexBase &sender,
        const RemovedPrimEntries &entries)
{
    _owner->_PrimsRemoved(sender, entries);
}

void
HdSingleInputFilteringSceneIndexBase::_Observer::PrimsDirtied(
        const HdSceneIndexBase &sender,
        const DirtiedPrimEntries &entries)
{
    _owner->_PrimsDirtied(sender, entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
