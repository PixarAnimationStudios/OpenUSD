//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "hdPrman/legacyMeshLightAdapter.h"
#include "pxr/usd/usdLux/lightAPI.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usd/ar/resolverContextBinder.h"
#include "pxr/usd/ar/resolverScopedCache.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/light.h"
#include "pxr/usdImaging/usdImaging/materialParamUtils.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/materialNetworkSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (isLight)
);

TF_REGISTRY_FUNCTION(TfType)
{
    typedef HdPrman_LegacyMeshLightAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

void
HdPrman_LegacyMeshLightAdapter::TrackVariability(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    HdDirtyBits* timeVaryingBits,
    UsdImagingInstancerContext const* instancerContext) const
{
    BaseAdapter::TrackVariability(
        prim, cachePath, timeVaryingBits, instancerContext);

    UsdLuxLightAPI light(prim);
    if (TF_VERIFY(light)) {
        UsdImaging_CollectionCache& collectionCache = _GetCollectionCache();
        collectionCache.UpdateCollection(light.GetLightLinkCollectionAPI());
        collectionCache.UpdateCollection(light.GetShadowLinkCollectionAPI());
        // TODO: When collections change we need to invalidate affected
        // prims with the DirtyCollections flag.
    }
}

HdDirtyBits
HdPrman_LegacyMeshLightAdapter::ProcessPropertyChange(
    UsdPrim const& prim, SdfPath const& cachePath, TfToken const& propertyName)
{
    HdDirtyBits dirtyBits = BaseAdapter::ProcessPropertyChange(
        prim, cachePath, propertyName);
    if (propertyName == _tokens->isLight)
        dirtyBits |= HdChangeTracker::AllDirty;
    else if (TfStringStartsWith(propertyName, "inputs:")
        || TfStringStartsWith(propertyName, "light:")
        || TfStringStartsWith(propertyName, "collection:"))
        dirtyBits |= HdChangeTracker::DirtyMaterialId;
    return dirtyBits;
}


VtValue
HdPrman_LegacyMeshLightAdapter::GetMaterialResource(
    UsdPrim const& prim, SdfPath const& cachePath, UsdTimeCode time) const
{
    // std::cout << "HdPrman_LegacyMeshLightAdapter::GetMaterialResource" << std::endl;
    if (!_GetSceneLightsEnabled()) {
        return VtValue();
    }

    if (!prim.HasAPI<UsdLuxLightAPI>()) {
        TF_RUNTIME_ERROR(
            "Expected light prim at <%s> to have an applied API "
            "of type 'UsdLuxLightAPI'; ignoring",
            prim.GetPath().GetText());
        return VtValue();
    }

    // Bind the usd stage's resolver context for correct asset resolution.
    ArResolverContextBinder binder(prim.GetStage()->GetPathResolverContext());
    ArResolverScopedCache resolverCache;

    HdMaterialNetworkMap networkMap;

    UsdImagingBuildHdMaterialNetworkFromTerminal(
        prim, HdMaterialTerminalTokens->light, _GetShaderSourceTypes(),
        _GetMaterialRenderContexts(), &networkMap, time);

    return VtValue(networkMap);
}

PXR_NAMESPACE_CLOSE_SCOPE