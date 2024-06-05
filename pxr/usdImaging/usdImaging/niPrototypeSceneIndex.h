//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
/// That is, the isolating scene index in the prototype propagating scene index
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
    // forNativePrototype = false indicates that this scene index is
    // instantiated for the USD stage with all USD prototypes filtered out.
    // forNativePrototype = true indicates that it is instantiated for a USD
    // prototype and it needs to populate the instancedBy data source.
    //
    // The given data source is overlayed over the prototype root prim's
    // data source.
    //
    // If instances with a particular opinion about, say, purpose, are
    // aggregated together, this opinion needs to be applied to the respective
    // prototype. This can be done by passing it as prototypeRootOverlayDs
    // here. A later flattening scene index can then apply the opinion to the
    // descendants of the prototype root that do not have a stronger opinion.
    //
    // Note that the flattening scene index is not flattening
    // model:applyDrawMode - but it still has an effect on the prototype root.
    //
    static
    UsdImaging_NiPrototypeSceneIndexRefPtr
    New(HdSceneIndexBaseRefPtr const &inputSceneIndex,
        bool forNativePrototype,
        HdContainerDataSourceHandle const &prototypeRootOverlayDs);

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
        bool forNativePrototype,
        HdContainerDataSourceHandle const &prototypeRootOverlayDso);

    const bool _forNativePrototype;
    HdContainerDataSourceHandle const _prototypeRootOverlaySource;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
