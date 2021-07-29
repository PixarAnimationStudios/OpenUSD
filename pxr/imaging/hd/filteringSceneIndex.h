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
#ifndef PXR_IMAGING_HD_FILTERING_SCENE_INDEX_H
#define PXR_IMAGING_HD_FILTERING_SCENE_INDEX_H

#include "pxr/pxr.h"

#include <set>
#include <unordered_map>

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/singleton.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/sceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(HdFilteringSceneIndexBase);

///
/// \class HdFilteringSceneIndexBase
///
/// An abstract base class for scene indexes that have one or more input scene
/// indexes which serve as a basis for their own scene.
///
class HdFilteringSceneIndexBase : public HdSceneIndexBase
{
public:
    virtual std::vector<HdSceneIndexBaseRefPtr> GetInputScenes() const = 0;
};



TF_DECLARE_WEAK_AND_REF_PTRS(HdSingleInputFilteringSceneIndexBase);

///
/// \class HdSingleInputFilteringSceneIndexBase
///
/// An abstract base class for a filtering scene index that observes a single 
/// input scene index.
/// 
class HdSingleInputFilteringSceneIndexBase : public HdFilteringSceneIndexBase
{
public:

    std::vector<HdSceneIndexBaseRefPtr> GetInputScenes() const final;

protected:

    HdSingleInputFilteringSceneIndexBase(
            const HdSceneIndexBaseRefPtr &inputSceneIndex);

    virtual void _PrimsAdded(
            const HdSceneIndexBase &sender,
            const HdSceneIndexObserver::AddedPrimEntries &entries) = 0;

    virtual void _PrimsRemoved(
            const HdSceneIndexBase &sender,
            const HdSceneIndexObserver::RemovedPrimEntries &entries) = 0;

    virtual void _PrimsDirtied(
            const HdSceneIndexBase &sender,
            const HdSceneIndexObserver::DirtiedPrimEntries &entries) = 0;

    const HdSceneIndexBaseRefPtr &_GetInputSceneIndex() const {
        return _inputSceneIndex;
    }

private:

    HdSceneIndexBaseRefPtr _inputSceneIndex;

    friend class _Observer;

    class _Observer : public HdSceneIndexObserver
    {
    public:
        _Observer(HdSingleInputFilteringSceneIndexBase *owner)
        : _owner(owner) {}

        void PrimsAdded(
                const HdSceneIndexBase &sender,
                const AddedPrimEntries &entries) override;

        void PrimsRemoved(
                const HdSceneIndexBase &sender,
                const RemovedPrimEntries &entries) override;

        void PrimsDirtied(
                const HdSceneIndexBase &sender,
                const DirtiedPrimEntries &entries) override;
    private:
        HdSingleInputFilteringSceneIndexBase *_owner;
    };

    _Observer _observer;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_FILTERING_SCENE_INDEX_H
