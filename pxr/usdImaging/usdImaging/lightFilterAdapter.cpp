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
#include "pxr/usdImaging/usdImaging/lightFilterAdapter.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hd/light.h"
#include "pxr/usd/usdLux/light.h"
#include "pxr/usd/usdLux/lightFilter.h"

#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingLightFilterAdapter Adapter;
    TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    // No factory here, UsdImagingLightFilterAdapter is abstract.
}

UsdImagingLightFilterAdapter::~UsdImagingLightFilterAdapter() 
{
}

void 
UsdImagingLightFilterAdapter::TrackVariability(UsdPrim const& prim,
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

    UsdImagingValueCache* valueCache = _GetValueCache();

    // XXX: The usage of _GetTimeWithOffset here is super-sketch, but avoids
    // blowing up the inherited visibility cache. This belongs in
    // UpdateForTime, except that we don't currently call UpdateForTime on
    // lights...
    valueCache->GetVisible(cachePath) = GetVisible(prim,
        _GetTimeWithOffset(0.0));

    UsdLuxLightFilter lightFilter(prim);
    if (TF_VERIFY(lightFilter)) {
        UsdImaging_CollectionCache &collectionCache = _GetCollectionCache();
        collectionCache.UpdateCollection(
                                lightFilter.GetFilterLinkCollectionAPI());
        // TODO: When collections change we need to invalidate affected
        // prims with the DirtyCollections flag.
    }
}

// Thread safe.
//  * Populate dirty bits for the given \p time.
void 
UsdImagingLightFilterAdapter::UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext) const
{
}

HdDirtyBits
UsdImagingLightFilterAdapter::ProcessPropertyChange(UsdPrim const& prim,
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
UsdImagingLightFilterAdapter::MarkDirty(UsdPrim const& prim,
                                  SdfPath const& cachePath,
                                  HdDirtyBits dirty,
                                  UsdImagingIndexProxy* index)
{
    index->MarkSprimDirty(cachePath, dirty);
}

void
UsdImagingLightFilterAdapter::MarkTransformDirty(UsdPrim const& prim,
                                           SdfPath const& cachePath,
                                           UsdImagingIndexProxy* index)
{
    static const HdDirtyBits transformDirty = HdLight::DirtyTransform;
    index->MarkSprimDirty(cachePath, transformDirty);
}

void
UsdImagingLightFilterAdapter::MarkVisibilityDirty(UsdPrim const& prim,
                                            SdfPath const& cachePath,
                                            UsdImagingIndexProxy* index)
{
    // TBD
}

PXR_NAMESPACE_CLOSE_SCOPE
