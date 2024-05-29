//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/lightFilterAdapter.h"

#include "pxr/usdImaging/usdImaging/dataSourceMaterial.h"
#include "pxr/usdImaging/usdImaging/dataSourcePrim.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/lightAdapter.h"
#include "pxr/usdImaging/usdImaging/materialParamUtils.h"
#include "pxr/usdImaging/usdImaging/tokens.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/light.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/usd/ar/resolverScopedCache.h"
#include "pxr/usd/ar/resolverContextBinder.h"
#include "pxr/usd/usdLux/lightFilter.h"
#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingLightFilterAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingLightFilterAdapter::~UsdImagingLightFilterAdapter() 
{
}

bool
UsdImagingLightFilterAdapter::IsSupported(UsdImagingIndexProxy const* index) const
{
    return UsdImagingLightAdapter::IsEnabledSceneLights() &&
           index->IsSprimTypeSupported(HdPrimTypeTokens->lightFilter);
}

SdfPath
UsdImagingLightFilterAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    index->InsertSprim(HdPrimTypeTokens->lightFilter, prim.GetPath(), prim);
    HD_PERF_COUNTER_INCR(HdPrimTypeTokens->lightFilter);

    return prim.GetPath();
}

void
UsdImagingLightFilterAdapter::_RemovePrim(SdfPath const& cachePath,
                                          UsdImagingIndexProxy* index)
{
    index->RemoveSprim(HdPrimTypeTokens->lightFilter, cachePath);
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

    // Determine if the light filter material network is time varying.
    if (UsdImagingIsHdMaterialNetworkTimeVarying(prim)) {
        *timeVaryingBits |= HdLight::DirtyBits::DirtyResource;
    }

    // If any of the light attributes is time varying 
    // we will assume all light params are time-varying.
    const std::vector<UsdAttribute> &attrs = prim.GetAttributes();
    for (UsdAttribute const& attr : attrs) {
        // Don't double-count transform attrs.
        if (UsdGeomXformable::IsTransformationAffectedByAttrNamed(
                attr.GetName())) {
            continue;
        }
        if (attr.GetNumTimeSamples()>1){
            *timeVaryingBits |= HdLight::DirtyBits::DirtyParams;
            break;
        }
    }

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

VtValue 
UsdImagingLightFilterAdapter::GetMaterialResource(UsdPrim const &prim,
                                                  SdfPath const& cachePath, 
                                                  UsdTimeCode time) const
{
    if (!_GetSceneLightsEnabled()) {
        return VtValue();
    }

    UsdLuxLightFilter lightFilter(prim);
    if (!lightFilter) {
        TF_RUNTIME_ERROR("Expected light filter prim at <%s> to be a subclass of type "
                         "'UsdLuxLightFilter', not type '%s'; ignoring",
                         prim.GetPath().GetText(),
                         prim.GetTypeName().GetText());
        return VtValue();
    }

    // Bind the usd stage's resolver context for correct asset resolution.
    ArResolverContextBinder binder(prim.GetStage()->GetPathResolverContext());
    ArResolverScopedCache resolverCache;

    HdMaterialNetworkMap networkMap;

    UsdImagingBuildHdMaterialNetworkFromTerminal(
        prim, 
        HdMaterialTerminalTokens->lightFilter,
        _GetShaderSourceTypes(),
        _GetMaterialRenderContexts(),
        &networkMap,
        time);

    return VtValue(networkMap);
}

TfTokenVector
UsdImagingLightFilterAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdImagingLightFilterAdapter::GetImagingSubprimType(UsdPrim const& prim,
    TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return HdPrimTypeTokens->lightFilter;
    }

    return TfToken();
}

HdContainerDataSourceHandle
UsdImagingLightFilterAdapter::GetImagingSubprimData(
        UsdPrim const& prim,
        TfToken const& subprim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (!subprim.IsEmpty()) {
        return nullptr;
    }

    // Overlay the material data source, which computes the node
    // network, over the base prim data source, which provides
    // other needed data like xform and visibility.
    return HdOverlayContainerDataSource::New(
        HdRetainedContainerDataSource::New(
            HdPrimTypeTokens->material,
            UsdImagingDataSourceMaterial::New(
                prim,
                stageGlobals,
                HdMaterialTerminalTokens->lightFilter)
            ),
        UsdImagingDataSourcePrim::New(
            prim.GetPath(), prim, stageGlobals));
}

HdDataSourceLocatorSet
UsdImagingLightFilterAdapter::InvalidateImagingSubprim(
        UsdPrim const& prim,
        TfToken const& subprim,
        TfTokenVector const& properties,
        const UsdImagingPropertyInvalidationType invalidationType)
{
    if (subprim.IsEmpty()) {
        return UsdImagingDataSourcePrim::Invalidate(
            prim, subprim, properties, invalidationType);
    }

    HdDataSourceLocatorSet result;
    for (const TfToken &propertyName : properties) {
        if (TfStringStartsWith(propertyName.GetString(), "inputs:")) {
            // NOTE: since we don't have access to the prim itself and our
            //       lightFilter terminal is currently named for the USD path,
            //       we cannot be specific to the individual parameter.
            // TODO: Consider whether we want to make the terminal node
            //       in the material network have a fixed name for the
            //       lightFilter case so that we could.
            result.insert(HdMaterialSchema::GetDefaultLocator());
            break;
        }
    }

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
