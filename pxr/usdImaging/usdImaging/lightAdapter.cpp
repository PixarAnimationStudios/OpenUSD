//
// Copyright 2017 Pixar
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
#include "pxr/usdImaging/usdImaging/lightAdapter.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/materialParamUtils.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/light.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/usd/ar/resolverScopedCache.h"
#include "pxr/usd/ar/resolverContextBinder.h"
#include "pxr/usd/usdLux/lightAPI.h"

#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingLightAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

TF_DEFINE_ENV_SETTING(USDIMAGING_ENABLE_SCENE_LIGHTS, 1, 
                      "Enable loading scene lights.");
/*static*/
bool UsdImagingLightAdapter::IsEnabledSceneLights() {
    static bool _v = TfGetEnvSetting(USDIMAGING_ENABLE_SCENE_LIGHTS) == 1;
    return _v;
}

UsdImagingLightAdapter::~UsdImagingLightAdapter() 
{
}

bool
UsdImagingLightAdapter::IsSupported(UsdImagingIndexProxy const* index) const
{
    return IsEnabledSceneLights() &&
           index->IsSprimTypeSupported(HdPrimTypeTokens->light);
}

SdfPath
UsdImagingLightAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    index->InsertSprim(HdPrimTypeTokens->light, prim.GetPath(), prim);
    HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);
    _RegisterLightCollections(prim);

    return prim.GetPath();
}

void
UsdImagingLightAdapter::_RemovePrim(SdfPath const& cachePath,
                                         UsdImagingIndexProxy* index)
{
    _UnregisterLightCollections(cachePath);
    index->RemoveSprim(HdPrimTypeTokens->domeLight, cachePath);
}

void
UsdImagingLightAdapter::MarkCollectionsDirty(UsdPrim const& prim,
                                             SdfPath const& cachePath,
                                             UsdImagingIndexProxy* index)
{
    index->MarkSprimDirty(cachePath, HdLight::DirtyCollection);
}

bool
UsdImagingLightAdapter::_UpdateCollectionsChanged(UsdPrim const& prim) const {
    UsdImaging_CollectionCache &collectionCache = _GetCollectionCache();
    UsdLuxLightAPI light(prim);
    bool lightColChanged = collectionCache.UpdateCollection(light.GetLightLinkCollectionAPI());
    bool shadowColChanged = collectionCache.UpdateCollection(light.GetShadowLinkCollectionAPI());
    return lightColChanged || shadowColChanged;
}

void
UsdImagingLightAdapter::_UnregisterLightCollections(SdfPath const& cachePath) {
    UsdImaging_CollectionCache &collectionCache = _GetCollectionCache();
    SdfPath lightLinkPath = cachePath.AppendProperty(UsdImagingTokens->collectionLightLink);
    collectionCache.RemoveCollection(_GetStage(), lightLinkPath);
    SdfPath shadowLinkPath = cachePath.AppendProperty(UsdImagingTokens->collectionShadowLink);
    collectionCache.RemoveCollection(_GetStage(), shadowLinkPath);
}

void
UsdImagingLightAdapter::_RegisterLightCollections(UsdPrim const& prim) {
    _UpdateCollectionsChanged(prim);
}

void 
UsdImagingLightAdapter::TrackVariability(UsdPrim const& prim,
                                        SdfPath const& cachePath,
                                        HdDirtyBits* timeVaryingBits,
                                        UsdImagingInstancerContext const* 
                                        instancerContext) const
{
    // Discover time-varying transforms.
    _IsTransformVarying(prim,
        HdLight::DirtyBits::DirtyTransform,
        UsdImagingTokens->usdVaryingXform,
        timeVaryingBits);

    // Discover time-varying visibility.
    _IsVarying(prim,
        UsdGeomTokens->visibility,
        HdLight::DirtyBits::DirtyParams,
        UsdImagingTokens->usdVaryingVisibility,
        timeVaryingBits,
        true);
    
    // Determine if the light material network is time varying.
    if (UsdImagingIsHdMaterialNetworkTimeVarying(prim)) {
        *timeVaryingBits |= HdLight::DirtyBits::DirtyResource;
    }

    // If any of the light attributes is time varying 
    // we will assume all light params are time-varying.
    const std::vector<UsdAttribute> &attrs = prim.GetAttributes();
    for (UsdAttribute const& attr : attrs) {
        // Don't double-count transform attrs.
        if (UsdGeomXformable::IsTransformationAffectedByAttrNamed(
                attr.GetBaseName())) {
            continue;
        }
        if (attr.GetNumTimeSamples()>1){
            *timeVaryingBits |= HdLight::DirtyBits::DirtyParams;
            break;
        }
    }

    UsdImagingPrimvarDescCache* primvarDescCache = _GetPrimvarDescCache();

    // XXX Cache primvars for lights.
    {
        // Establish a primvar desc cache entry.
        HdPrimvarDescriptorVector& vPrimvars = 
            primvarDescCache->GetPrimvars(cachePath);

        // Compile a list of primvars to check.
        std::vector<UsdGeomPrimvar> primvars;
        UsdImaging_InheritedPrimvarStrategy::value_type inheritedPrimvarRecord =
            _GetInheritedPrimvars(prim.GetParent());
        if (inheritedPrimvarRecord) {
            primvars = inheritedPrimvarRecord->primvars;
        }

        UsdGeomPrimvarsAPI primvarsAPI(prim);
        std::vector<UsdGeomPrimvar> local = primvarsAPI.GetPrimvarsWithValues();
        primvars.insert(primvars.end(), local.begin(), local.end());
        for (auto const &pv : primvars) {
            _ComputeAndMergePrimvar(prim, pv, UsdTimeCode(), &vPrimvars);
        }
    }
}

// Thread safe.
//  * Populate dirty bits for the given \p time.
void 
UsdImagingLightAdapter::UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext) const
{
}

HdDirtyBits
UsdImagingLightAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                      SdfPath const& cachePath, 
                                      TfToken const& propertyName)
{
    if (UsdGeomXformable::IsTransformationAffectedByAttrNamed(propertyName)) {
        return HdLight::DirtyBits::DirtyTransform;
    }

    if (TfStringStartsWith(propertyName.GetString(), UsdImagingTokens->collectionShadowLink.GetString()) || 
        TfStringStartsWith(propertyName.GetString(), UsdImagingTokens->collectionLightLink.GetString())) {
        if (_UpdateCollectionsChanged(prim)) {
            return HdLight::DirtyBits::DirtyCollection;
        }
    }

    // "DirtyParam" is the catch-all bit for light params.
    return HdLight::DirtyBits::DirtyParams;
}

void
UsdImagingLightAdapter::MarkDirty(UsdPrim const& prim,
                                  SdfPath const& cachePath,
                                  HdDirtyBits dirty,
                                  UsdImagingIndexProxy* index)
{
    index->MarkSprimDirty(cachePath, dirty);
}

void
UsdImagingLightAdapter::MarkTransformDirty(UsdPrim const& prim,
                                           SdfPath const& cachePath,
                                           UsdImagingIndexProxy* index)
{
    static const HdDirtyBits transformDirty = HdLight::DirtyTransform;
    index->MarkSprimDirty(cachePath, transformDirty);
}

void
UsdImagingLightAdapter::MarkVisibilityDirty(UsdPrim const& prim,
                                            SdfPath const& cachePath,
                                            UsdImagingIndexProxy* index)
{
    static const HdDirtyBits paramsDirty = HdLight::DirtyParams;
    index->MarkSprimDirty(cachePath, paramsDirty);
}

void
UsdImagingLightAdapter::MarkLightParamsDirty(UsdPrim const& prim,
                                             SdfPath const& cachePath,
                                             UsdImagingIndexProxy* index)
{
    static const HdDirtyBits paramsDirty = HdLight::DirtyParams;
    index->MarkSprimDirty(cachePath, paramsDirty);
}


VtValue 
UsdImagingLightAdapter::GetMaterialResource(UsdPrim const &prim,
                                            SdfPath const& cachePath, 
                                            UsdTimeCode time) const
{
    if (!_GetSceneLightsEnabled()) {
        return VtValue();
    }

    if (!prim.HasAPI<UsdLuxLightAPI>()) {
        TF_RUNTIME_ERROR("Expected light prim at <%s> to have an applied API "
                         "of type 'UsdLuxLightAPI'; ignoring",
                         prim.GetPath().GetText());
        return VtValue();
    }

    // Bind the usd stage's resolver context for correct asset resolution.
    ArResolverContextBinder binder(prim.GetStage()->GetPathResolverContext());
    ArResolverScopedCache resolverCache;

    HdMaterialNetworkMap networkMap;

    UsdImagingBuildHdMaterialNetworkFromTerminal(
        prim, 
        HdMaterialTerminalTokens->light,
        _GetShaderSourceTypes(),
        _GetMaterialRenderContexts(),
        &networkMap,
        time);

    return VtValue(networkMap);
}

PXR_NAMESPACE_CLOSE_SCOPE
