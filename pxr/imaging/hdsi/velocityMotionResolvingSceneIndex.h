//
// Copyright 2024 Pixar
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
#ifndef PXR_IMAGING_HDSI_VELOCITY_MOTION_RESOLVING_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_VELOCITY_MOTION_RESOLVING_SCENE_INDEX_H

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"
#include "pxr/imaging/hdsi/api.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/declarePtrs.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdsiVelocityMotionResolvingSceneIndex);

class HdsiVelocityMotionResolvingSceneIndex
  : public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdsiVelocityMotionResolvingSceneIndexRefPtr
    New(
        const HdSceneIndexBaseRefPtr& inputSceneIndex,
        const HdContainerDataSourceHandle& inputArgs);
    
    HDSI_API
    HdSceneIndexPrim
    GetPrim(const SdfPath& primPath) const override;
    
    HDSI_API
    SdfPathVector
    GetChildPrimPaths(const SdfPath& primPath) const override;

protected:
    HdsiVelocityMotionResolvingSceneIndex(
        const HdSceneIndexBaseRefPtr& inputSceneIndex,
        const HdContainerDataSourceHandle& inputArgs);
    
    void
    _PrimsAdded(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::AddedPrimEntries& entries) override;
    
    void
    _PrimsRemoved(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::RemovedPrimEntries& entries) override;
    
    void
    _PrimsDirtied(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::DirtiedPrimEntries& entries) override;

private:
    HdContainerDataSourceHandle _inputArgs;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDSI_VELOCITY_MOTION_RESOLVING_SCENE_INDEX_H
