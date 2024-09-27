//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "hdPrman/legacyVolumeLightAdapter.h"

#if !defined(ARCH_OS_WINDOWS)

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
#include "pxr/usdImaging/usdImaging/fieldAdapter.h"
#include "pxr/usd/usdVol/fieldBase.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (isLight)
);

TF_REGISTRY_FUNCTION(TfType)
{
    typedef HdPrman_LegacyVolumeLightAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

void
HdPrman_LegacyVolumeLightAdapter::TrackVariability(
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
#if PXR_VERSION > 2308 || !defined(ARCH_OS_WINDOWS)
        collectionCache.UpdateCollection(light.GetLightLinkCollectionAPI());
        collectionCache.UpdateCollection(light.GetShadowLinkCollectionAPI());
#endif
        // TODO: When collections change we need to invalidate affected
        // prims with the DirtyCollections flag.
    }
}

HdDirtyBits
HdPrman_LegacyVolumeLightAdapter::ProcessPropertyChange(
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
HdPrman_LegacyVolumeLightAdapter::GetMaterialResource(
    UsdPrim const& prim, SdfPath const& cachePath, UsdTimeCode time) const
{
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

HdVolumeFieldDescriptorVector
HdPrman_LegacyVolumeLightAdapter::GetVolumeFieldDescriptors(UsdPrim const& usdPrim,
                                                    SdfPath const &id,
                                                    UsdTimeCode time) const
{
    HdVolumeFieldDescriptorVector descriptors;
    UsdVolVolume::FieldMap fieldMap;

    UsdVolVolume volume(usdPrim);
    fieldMap = volume.GetFieldPaths();

    if (!fieldMap.empty()) {
        for (auto it = fieldMap.begin(); it != fieldMap.end(); ++it) {
            UsdPrim fieldUsdPrim(_GetPrim(it->second));
            UsdVolFieldBase fieldPrim(fieldUsdPrim);

            if (fieldPrim) {
                TfToken fieldPrimType;
                UsdImagingPrimAdapterSharedPtr adapter
                    = _GetPrimAdapter(fieldUsdPrim);
                UsdImagingFieldAdapter *fieldAdapter;

                fieldAdapter = dynamic_cast<UsdImagingFieldAdapter *>(
                    adapter.get());
                if (TF_VERIFY(fieldAdapter)) {
                    fieldPrimType = fieldAdapter->GetPrimTypeToken();
                    // XXX(UsdImagingPaths): Using usdPath directly
                    // as cachePath here -- we should do the correct
                    // mapping in order for instancing to work.
                    SdfPath const& cachePath = fieldUsdPrim.GetPath();
                    descriptors.push_back(
                        HdVolumeFieldDescriptor(it->first, fieldPrimType,
                            _ConvertCachePathToIndexPath(cachePath)));
                }
            }
        }
    }

    return descriptors;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // !defined(ARCH_OS_WINDOWS)
