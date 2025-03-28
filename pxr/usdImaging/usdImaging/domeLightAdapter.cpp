//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/domeLightAdapter.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/light.h"
#include "pxr/imaging/hd/lightSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/usdLux/tokens.h"
#include "pxr/usd/usdLux/domeLight.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingDomeLightAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

namespace {

// An HdTypedSampledDataSource that determines the list of portals
class _PortalsDataSource : public HdTypedSampledDataSource<SdfPathVector>
{
public:
    HD_DECLARE_DATASOURCE(_PortalsDataSource);

    _PortalsDataSource(const UsdPrim& prim)
    : _prim(prim)
    {
    }

    VtValue GetValue(HdSampledDataSource::Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

    SdfPathVector GetTypedValue(HdSampledDataSource::Time shutterOffset) override
    {
        UsdRelationship portalsRel = UsdLuxDomeLight(_prim).GetPortalsRel();
        SdfPathVector portalPaths;
        if (portalsRel) {
            portalsRel.GetForwardedTargets(&portalPaths);
        }
        return portalPaths;
    }

    bool GetContributingSampleTimesForInterval(
        HdSampledDataSource::Time startTime,
        HdSampledDataSource::Time endTime,
        std::vector<HdSampledDataSource::Time>* outSampleTimes) override
    {
        return false;
    }

private:
    UsdPrim _prim;
};

} // namespace

UsdImagingDomeLightAdapter::~UsdImagingDomeLightAdapter() 
{
}

TfTokenVector
UsdImagingDomeLightAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdImagingDomeLightAdapter::GetImagingSubprimType(
    UsdPrim const& prim, TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return HdPrimTypeTokens->domeLight;
    }

    return TfToken();
}

HdContainerDataSourceHandle
UsdImagingDomeLightAdapter::GetImagingSubprimData(
    UsdPrim const& prim,
    TfToken const& subprim,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    return HdOverlayContainerDataSource::New(
        HdRetainedContainerDataSource::New(
            HdLightSchema::GetSchemaToken(),
            HdRetainedContainerDataSource::New(
                HdTokens->portals,
                _PortalsDataSource::New(prim))),
        UsdImagingLightAdapter::GetImagingSubprimData(
            prim, subprim, stageGlobals));
}

HdDataSourceLocatorSet
UsdImagingDomeLightAdapter::InvalidateImagingSubprim(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfTokenVector const& properties,
    UsdImagingPropertyInvalidationType invalidationType)
{
    HdDataSourceLocatorSet result =
        UsdImagingLightAdapter::InvalidateImagingSubprim(
            prim, subprim, properties, invalidationType);

    for (const TfToken &propertyName : properties) {
        if (propertyName == UsdLuxTokens->portals) {
            result.insert(HdLightSchema::GetDefaultLocator());
        }
    }

    return result;
}

bool
UsdImagingDomeLightAdapter::IsSupported(UsdImagingIndexProxy const* index) const
{
    return UsdImagingLightAdapter::IsEnabledSceneLights() &&
           index->IsSprimTypeSupported(HdPrimTypeTokens->domeLight);
}

SdfPath
UsdImagingDomeLightAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    return _AddSprim(HdPrimTypeTokens->domeLight, prim, index, instancerContext);
}

void
UsdImagingDomeLightAdapter::_RemovePrim(SdfPath const& cachePath,
                                          UsdImagingIndexProxy* index)
{
    _RemoveSprim(HdPrimTypeTokens->domeLight, cachePath, index);
}


PXR_NAMESPACE_CLOSE_SCOPE
