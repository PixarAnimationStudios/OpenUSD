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
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_NI_PROTOTYPE_SCENE_INDEX_H
#define PXR_USD_IMAGING_USD_IMAGING_NI_PROTOTYPE_SCENE_INDEX_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USDIMAGING_NI_PROTOTYPE_SCENE_INDEX_TOKENS \
    ((instancer, "UsdNiInstancer"))                \
    ((prototype, "UsdNiPrototype"))

TF_DECLARE_PUBLIC_TOKENS(
    UsdImaging_NiPrototypeSceneIndexTokens,
    USDIMAGING_NI_PROTOTYPE_SCENE_INDEX_TOKENS);

TF_DECLARE_REF_PTRS(UsdImaging_NiPrototypeSceneIndex);

/// \class UsdImaging_NiPrototypeSceneIndex
///
/// A scene index that prepares the prims under /UsdNiInstancer/UsdPrototype
/// to be instanced by the instancer /UsdNiInstancer created
/// by the UsdImaging_InstanceAggregationSceneIndex.
/// Note that /UsdNiInstancer/UsdPrototype corresponds to a USD prototype.
/// That is, te isolating scene index in the prototype propagating scene index
/// is taking a USD prototype at, e.g., /__Prototype_1 and moves it underneath
/// /UsdNiInstancer/UsdPrototype.
///
/// It forces an empty type on all prims that are instances
/// (that is prims with non-trivial usdPrototypePath).
/// The reason is: an instance in usd can have a type such as
/// sphere, yet we do not want to see this sphere in the render.
///
/// It also adds an instanced by data source with
/// instancedBy:paths being /UsdNiInstancer and instancedBy:prototypeRoot being
/// /UsdNiInstancer/UsdNiPrototype. These are only added if they are not
/// already present. That way, point instancers and prototypes within
/// native prototypes are handled correctly.
///
class UsdImaging_NiPrototypeSceneIndex
            : public HdSingleInputFilteringSceneIndexBase
{
public:
    // forPrototype = false indicates that this scene index is instantiated
    // for the USD stage with all USD prototypes filtered out.
    // forPrototype = true indicates that it is instantiated for a USD
    // prototype and it needs to populate the instancedBy data source.
    static
    UsdImaging_NiPrototypeSceneIndexRefPtr
    New(HdSceneIndexBaseRefPtr const &inputSceneIndex,
        bool forPrototype);

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

    /// Get path of the instancer that the instance aggregation scene index
    /// will add. This path is used by this scene index as well.
    static
    const SdfPath &GetInstancerPath();

    /// Get path of the copy of the USD prototype that is a child of the
    /// instancer.
    static
    const SdfPath &GetPrototypePath();

    /// Get's data source for instancedBy schema for prims within this
    /// prototype.
    static
    const HdDataSourceBaseHandle &GetInstancedByDataSource();

protected:
    void _PrimsAdded(
        const HdSceneIndexBase&,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;
    void _PrimsDirtied(
        const HdSceneIndexBase&,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;
    void _PrimsRemoved(
        const HdSceneIndexBase&,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;

private:
    UsdImaging_NiPrototypeSceneIndex(
        HdSceneIndexBaseRefPtr const &inputSceneIndex,
        bool forPrototype);

    const bool _forPrototype;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
