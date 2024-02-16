//
// Copyright 2023 Pixar
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