//
// Copyright 2021 Pixar
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
#ifndef EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATERIAL_FILTERING_SCENE_INDEX_H
#define EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATERIAL_FILTERING_SCENE_INDEX_H


#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/materialNetworkInterface.h"

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(HdPrmanMaterialFilteringSceneIndexBase);

/// \class HdPrmanMaterialFilteringSceneIndexBase
///
/// Base class for implementing scene indices which read from and write to
/// only material network data sources. Subclasses implement only 
/// _GetFilteringFunction to provide a callback to run when a material network
/// is first queried.
class HdPrmanMaterialFilteringSceneIndexBase :
    public HdSingleInputFilteringSceneIndexBase
{
public:
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override final;

    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override final;

    using FilteringFnc =
        std::function<void(HdMaterialNetworkInterface *)>;

protected:
    virtual FilteringFnc _GetFilteringFunction() const = 0;

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override final;

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override final;

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override final;

    HdPrmanMaterialFilteringSceneIndexBase(
        const HdSceneIndexBaseRefPtr &inputSceneIndex);

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATERIAL_FILTERING_SCENE_INDEX_H
