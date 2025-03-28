
//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// XXX change directive
#ifndef PXR_IMAGING_HDSI_EXT_COMPUTATION_PRIMVAR_PRUNING_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_EXT_COMPUTATION_PRIMVAR_PRUNING_SCENE_INDEX_H

#include "pxr/imaging/hdsi/api.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(HdSiExtComputationPrimvarPruningSceneIndex);

/// \class HdSiExtComputationPrimvarPruningSceneIndex
///
/// Hydra ExtComputations provide a simple computation framework allowing
/// primvars to be computed using CPU or GPU kernels.
/// Computed primvars backed by CPU kernels are evaluated during the Hydra
/// sync phase. This disallows transformations on the computed values via
/// scene indices.
/// This scene index alleviates this by pruning computed primvars and presenting
/// them as authored primvars. The computation is executed when pulling on the 
/// primvar's value.
/// Thus, scene indices downstream that take this as an input can transform
/// the (computed) primvar data just like any authored primvar.
///
/// \note This scene index is in service of emulated ExtComputations (i.e., when
///       HD_ENABLE_SCENE_INDEX_EMULATION is true).
/// 
class HdSiExtComputationPrimvarPruningSceneIndex :
    public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdSiExtComputationPrimvarPruningSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr &inputSceneIndex);

    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override final;

    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override final;

protected:
    HDSI_API
    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override final;

    HDSI_API
    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override final;

    HDSI_API
    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override final;

    HDSI_API
    HdSiExtComputationPrimvarPruningSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex);

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HDSI_EXT_COMPUTATION_PRIMVAR_PRUNING_SCENE_INDEX_H
