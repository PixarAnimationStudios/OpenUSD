
//
// Copyright 2022 Pixar
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
//     http://www.apache.org/licenses/LICEN SE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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
