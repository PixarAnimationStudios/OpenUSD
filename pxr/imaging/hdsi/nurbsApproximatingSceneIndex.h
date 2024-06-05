//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef PXR_IMAGING_HDSI_NURBS_APPROXIMATING_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_NURBS_APPROXIMATING_SCENE_INDEX_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdsi/api.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdsiNurbsApproximatingSceneIndex);

///
/// \class HdsiNurbsApproximatingSceneIndex
///
/// Converts prims of type nurbsCurves and nurbsPatches to basisCurves and mesh,
/// respectively. The result is only an approximation for clients that do not
/// natively supports nurbs.
///
class HdsiNurbsApproximatingSceneIndex :
    public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdsiNurbsApproximatingSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr &inputSceneIndex);
    
    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;
    
protected:
    HdsiNurbsApproximatingSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex);

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HDSI_NURBS_APPROXIMATING_SCENE_INDEX_H
