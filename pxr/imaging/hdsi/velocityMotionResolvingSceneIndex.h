//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
