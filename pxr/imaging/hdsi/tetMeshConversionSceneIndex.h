//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDSI_TET_MESH_CONVERSION_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_TET_MESH_CONVERSION_SCENE_INDEX_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdsi/api.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

#include "pxr/base/gf/vec3i.h"
#include "pxr/base/gf/vec4i.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DECLARE_REF_PTRS(HdsiTetMeshConversionSceneIndex);

///
/// \class HdsiTetMeshConversionSceneIndex
///
/// A scene index converting TetMeshes into standard triangle based Meshes.
///
class HdsiTetMeshConversionSceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdsiTetMeshConversionSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr &inputSceneIndex);

    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:
    HdsiTetMeshConversionSceneIndex(
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

public:

    static void _ComputeSurfaceFaces(
        const VtVec4iArray& tetVertexIndices,
        VtVec3iArray *surfaceFaceIndices);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HDSI_TET_MESH_CONVERSION_SCENE_INDEX_H
