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

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

class HdGpGenerativeProceduralFilteringSceneIndex;
TF_DECLARE_REF_PTRS(HdGpGenerativeProceduralFilteringSceneIndex);

/// \class HdGpGenerativeProceduralFilteringSceneIndex
///
/// HdGpGenerativeProceduralFilteringSceneIndex is a scene index which
/// filters prims representing generative procedurals within its incoming
/// scene against a requested pattern.
///
/// Typically, this scene index re-types (to its observers) any procedural prim
/// it filters to the type "skippedGenerativeProcedural" and ones that are
/// allowed will have their types remain the same.  This scene index can also
/// be configured to have specific primTypes for procedurals that are skipped
/// or allowed.
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

    /// This constructs a filtering scene index that will try to filter prims of
    /// type \p targetPrimType.  For each prim of this type, 
    /// \p allowedProceduralTypes will be used to determined if the procedural
    /// is "allowed" or "skipped".
    /// 
    /// Prims that are not of type \p targetPrimType are left alone.
    ///
    /// If \p allowedPrimTypeName is specified, then "allowed" prims will have
    /// their type set to that.  Otherwise, it will be set to \p targetPrimType.
    /// 
    /// If \p skippedPrimTypeName is specified, then "skipped" prims will have
    /// their type set to that.  Otherwise, it will be set to
    /// "skippedGenerativeProcedural".
    static HdGpGenerativeProceduralFilteringSceneIndexRefPtr New(
            const HdSceneIndexBaseRefPtr &inputScene,
            const TfTokenVector &allowedProceduralTypes,
            const std::optional<TfToken> &targetPrimTypeName,
            const std::optional<TfToken> &allowedPrimTypeName,
            const std::optional<TfToken> &skippedPrimTypeName) {
        return TfCreateRefPtr(
            new HdGpGenerativeProceduralFilteringSceneIndex(
                inputScene, allowedProceduralTypes, targetPrimTypeName,
                allowedPrimTypeName, skippedPrimTypeName));
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
        const std::optional<TfToken> &targetPrimTypeName,
        const std::optional<TfToken> &allowedPrimTypeName,
        const std::optional<TfToken> &skippedPrimTypeName);

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

    enum class _ShouldSkipResult {
        Ignore = 0,
        Skip,
        Allow,
    };
    _ShouldSkipResult _ShouldSkipPrim(HdSceneIndexPrim const& prim) const;

    const TfTokenVector _allowedProceduralTypes;
    const TfToken _targetPrimTypeName;

    TfToken _allowedPrimTypeName;
    TfToken _skippedPrimTypeName;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
