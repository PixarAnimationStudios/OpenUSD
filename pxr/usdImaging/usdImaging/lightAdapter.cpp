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
#include "pxr/usd/usdLux/light.h"

#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingLightAdapter Adapter;
    TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    // No factory here, UsdImagingLightAdapter is abstract.
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
    if (UsdImaging_IsHdMaterialNetworkTimeVarying(prim)) {
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

    UsdLuxLight light(prim);
    if (TF_VERIFY(light)) {
        UsdImaging_CollectionCache &collectionCache = _GetCollectionCache();
        collectionCache.UpdateCollection(light.GetLightLinkCollectionAPI());
        collectionCache.UpdateCollection(light.GetShadowLinkCollectionAPI());
        // TODO: When collections change we need to invalidate affected
        // prims with the DirtyCollections flag.
    }

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
    UsdLuxLight light(prim);
    if (!light) {
        TF_RUNTIME_ERROR("Expected light prim at <%s> to be a subclass of type "
                         "'UsdLuxLight', not type '%s'; ignoring",
                         prim.GetPath().GetText(),
                         prim.GetTypeName().GetText());
        return VtValue();
    }

    // Bind the usd stage's resolver context for correct asset resolution.
    ArResolverContextBinder binder(prim.GetStage()->GetPathResolverContext());
    ArResolverScopedCache resolverCache;

    HdMaterialNetworkMap networkMap;

    UsdImaging_BuildHdMaterialNetworkFromTerminal(
        prim, 
        HdMaterialTerminalTokens->light,
        _GetShaderSourceTypes(),
        &networkMap,
        time);

    return VtValue(networkMap);
}

PXR_NAMESPACE_CLOSE_SCOPE
