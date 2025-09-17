//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef PXR_IMAGING_HDSI_IMPLICIT_SURFACE_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_IMPLICIT_SURFACE_SCENE_INDEX_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdsi/api.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

#define HDSI_IMPLICIT_SURFACE_SCENE_INDEX_TOKENS \
    (toMesh) \
    (axisToTransform) \

TF_DECLARE_PUBLIC_TOKENS(HdsiImplicitSurfaceSceneIndexTokens, HDSI_API,
     HDSI_IMPLICIT_SURFACE_SCENE_INDEX_TOKENS);

TF_DECLARE_REF_PTRS(HdsiImplicitSurfaceSceneIndex);

///
/// \class HdsiImplicitSurfaceSceneIndex
///
/// The implicit surface scene index can be "configured" to either generate
/// the mesh for a given implicit primitive (for renderers that don't
/// natively support it) or overload the transform to account for a different
/// "spine" axis (relevant for cones, capsules and cylinders) for those that
/// do.
class HdsiImplicitSurfaceSceneIndex :
    public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdsiImplicitSurfaceSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr &inputSceneIndex,
        const HdContainerDataSourceHandle &inputArgs);
    
    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;
    
protected:
    HdsiImplicitSurfaceSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex,
        const HdContainerDataSourceHandle &inputArgs);

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

private:
    const TfToken _capsuleMode;
    const TfToken _coneMode;
    const TfToken _cubeMode;
    const TfToken _cylinderMode;
    const TfToken _planeMode;
    const TfToken _sphereMode;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HDSI_IMPLICIT_SURFACE_SCENE_INDEX_H
