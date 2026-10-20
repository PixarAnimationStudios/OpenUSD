//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef PXR_IMAGING_HDSI_PURPOSE_FILTERING_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_PURPOSE_FILTERING_SCENE_INDEX_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hdsi/api.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

#define HDSI_PURPOSE_FILTERING_SCENE_INDEX_TOKENS \
    (purposes)

TF_DECLARE_PUBLIC_TOKENS(HdsiPurposeFilteringSceneIndexTokens, HDSI_API,
                         HDSI_PURPOSE_FILTERING_SCENE_INDEX_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(HdsiPurposeFilteringSceneIndex);

/// Scene index that prunes prims whose resolved (flattened) purpose is not
/// in the configured set of accepted purposes.
///
/// Pruned prims are not removed from the scene index; instead, they are given
/// an empty primType. This preserves hierarchy and allows children of pruned
/// prims to still exist, which is important because they may have a different
/// resolved purpose.
///
/// \section Operating modes
///
/// This scene index operates in one of two modes:
///
/// **Hint mode** (the initial mode): The \p inputArgs container passed to
/// New() is queried via HdSceneIndexCreateArgsSchema for the
/// \c excludedPurposesHint field. If the field is present, prims whose
/// resolved purpose appears in the hint are pruned. If the field is absent,
/// no prims are pruned. This allows a renderer to declare which purposes it
/// does not consume (e.g. proxy, guide) and have the scene index honor that
/// automatically, without requiring an explicit call to SetPurposes() which
/// can be expensive because it may require a full traversal of the input scene.
/// This is because this scene index is currently stateless.
///
/// **Explicit mode**: Calling SetPurposes() exits hint mode and switches to
/// an accept-list: only prims whose resolved purpose is in the provided array
/// are kept; an empty array disables filtering entirely. The \p inputArgs
/// container may also seed this mode directly by supplying the
/// \c purposes key (HdsiPurposeFilteringSceneIndexTokens->purposes), in which
/// case the scene index starts in explicit mode at construction time. This may
/// be useful for non-interactive (aka batch) rendering scenarios where the
/// renderable set of purposes is known and fixed.
///
/// \note
/// This scene index should be placed after an HdFlatteningSceneIndex instance
/// with a HdFlattenedPurposeDataSourceProvider so that purpose has already
/// been resolved through ancestor inheritance before this index queries it.
///
/// \section Historial (Hydra 1) notes
/// Hydra 1 refers to the USD purpose as "render tag".
/// Only HdRprim types have a render tag ( \c HdRprim::GetRenderTag).
/// Purpose filtering happens during the sync phase ( \c HdRprim::Sync ) based
/// on the render tags aggregated from the tasks ( \c HdTask::GetRenderTags ).
///
/// The USD "default" purpose maps to HdRenderTagTokens->geometry.
///
/// \section USD purpose inheritance
/// UsdGeomImageable introduces the \c purpose attribute. The inheritance rules
/// are as follows (from the "Rendering with USD" guide on OpenUSD):
/// 
/// "If a prim does not have an authored purpose but has an ancestor that may 
/// have an authored purpose, the rules about how the ancestor purpose is
/// inherited at render-time are as follows:
///  If a prim is not imageable or does not have an authored opinion about its
///  own purpose, then it will inherit the purpose of the closest imageable
///  ancestor with an authored purpose opinion.
///  If there are no imageable ancestors with an authored purpose opinion then
///  the prim uses its fallback purpose."
///
/// This behavior is provided by the \c HdFlattenedPurposeDataSourceProvider.
///
/// \sa HdRenderTagTokens
/// \sa HdSceneIndexCreateArgsSchema
///
class HdsiPurposeFilteringSceneIndex final
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    /// \p The fields in \p inputArgs that this scene index consumes are:
    /// 1. \c excludedPurposesHint, a \c HdTokenArrayDataSource under
    ///    \c HdSceneIndexCreateArgsSchema::GetSchemaToken()) and
    /// 2. \c purposes, also a \c HdTokenArrayDataSource
    ///
    /// They dictate whether the initial operating mode of the scene index is
    /// in hint mode or explicit mode.
    HDSI_API
    static HdsiPurposeFilteringSceneIndexRefPtr
    New(HdSceneIndexBaseRefPtr const &inputSceneIndex,
        HdContainerDataSourceHandle const &inputArgs);

    /// Switch to explicit (accept-list) mode and set the accepted purposes.
    /// Prims whose resolved purpose is not in this array will be pruned
    /// (primType set to empty). An empty array disables purpose filtering
    /// (no prims are pruned). Once called, hint mode is no longer active.
    HDSI_API
    void SetPurposes(VtArray<TfToken> const &purposes);

    HDSI_API
    VtArray<TfToken> const &GetPurposes() const;

public: // HdSceneIndex overrides
    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected: // HdSingleInputFilteringSceneIndexBase overrides
    HDSI_API
    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;
    HDSI_API
    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;
    HDSI_API
    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

protected:
    HDSI_API
    HdsiPurposeFilteringSceneIndex(
        HdSceneIndexBaseRefPtr const &inputSceneIndex,
        HdContainerDataSourceHandle const &inputArgs);
    HDSI_API
    ~HdsiPurposeFilteringSceneIndex() override;

private:
    bool _IsFilteringActive() const;

    // Returns true if the prim represented by dataSource should be pruned
    // based on the current state (_purposes or _excludedPurposesHint).
    bool _ShouldPrune(const HdContainerDataSourceHandle &dataSource) const;

    // nullopt = hint mode (purposes not provided at c'tor time and SetPurposes
    // has not been called).
    // [] = explicit empty = no filtering.
    // [...] = explicit accepted-purpose filter.
    std::optional<VtArray<TfToken>> _purposes;

    // Purposes that should be pruned while in hint mode (nullopt _purposes).
    VtArray<TfToken> _excludedPurposesHint;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
