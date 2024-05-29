//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_GP_GENERATIVE_PROCEDURAL_FILTERING_SCENE_INDEX_H
#define PXR_IMAGING_HD_GP_GENERATIVE_PROCEDURAL_FILTERING_SCENE_INDEX_H

#include "pxr/imaging/hdGp/generativeProcedural.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/usd/sdf/pathTable.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdGpGenerativeProceduralFilteringSceneIndex;
TF_DECLARE_REF_PTRS(HdGpGenerativeProceduralFilteringSceneIndex);

/// \class HdGpGenerativeProceduralFilteringSceneIndex
/// 
/// HdGpGenerativeProceduralFilteringSceneIndex is a scene index which
/// filters prims representing generative procedurals within its incoming
/// scene against a requested pattern.
///
/// This scene index re-types (to its observers) any procedural prim it
/// filters to the type "skippedGenerativeProcedural".
///
/// The hydra prim type used to identify generative procedurals can be
/// configured per instance of this scene index to allow for a pipeline to
/// stage when certain procedural prims are resolved within the chain of scene
/// indicies. By default that type is "generativeProcedural".
///
class HdGpGenerativeProceduralFilteringSceneIndex final
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    static HdGpGenerativeProceduralFilteringSceneIndexRefPtr New(
            const HdSceneIndexBaseRefPtr &inputScene,
            const TfTokenVector &allowedProceduralTypes) {
        return TfCreateRefPtr(
            new HdGpGenerativeProceduralFilteringSceneIndex(
                inputScene, allowedProceduralTypes));
    }
    static HdGpGenerativeProceduralFilteringSceneIndexRefPtr New(
            const HdSceneIndexBaseRefPtr &inputScene,
            const TfTokenVector &allowedProceduralTypes,
            const TfToken &targetPrimTypeName) {
        return TfCreateRefPtr(
            new HdGpGenerativeProceduralFilteringSceneIndex(
                inputScene, allowedProceduralTypes, targetPrimTypeName));
    }

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

private:
    HdGpGenerativeProceduralFilteringSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const TfTokenVector &allowedProceduralTypes);
    HdGpGenerativeProceduralFilteringSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const TfTokenVector &allowedProceduralTypes,
        const TfToken &targetPrimTypeName);

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;
    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;
    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

    TfToken _GetProceduralType(HdSceneIndexPrim const& prim) const;
    bool _ShouldSkipPrim(HdSceneIndexPrim const& prim) const;

    const TfTokenVector _allowedProceduralTypes;
    const TfToken _targetPrimTypeName;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
