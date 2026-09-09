//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/backPlateAPIAdapter.h"
#include "pxr/usdImaging/usdImaging/dataSourcePrim.h"

#include "pxr/imaging/hd/backPlateSchema.h"
#include "pxr/imaging/hd/cameraSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"

#include "pxr/usd/usdGeom/backPlateAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdImagingBackPlateAPIAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingAPISchemaAdapterFactory<Adapter> >();
}
UsdImagingBackPlateAPIAdapter::
UsdImagingBackPlateAPIAdapter() = default;

UsdImagingBackPlateAPIAdapter::
~UsdImagingBackPlateAPIAdapter() = default;

HdContainerDataSourceHandle
UsdImagingBackPlateAPIAdapter::GetImagingSubprimData(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (appliedInstanceName.IsEmpty()) {
        return nullptr;
    }
    UsdGeomBackPlateAPI backPlate(prim, appliedInstanceName);

    TfToken plateVisibility = HdBackPlateSchemaTokens->solo;
    backPlate.GetPlateVisibilityAttr().Get(&plateVisibility);

    HdDataSourceLocator plateLocator = 
        HdCameraSchema::GetBackPlateLocator().Append(appliedInstanceName);

    HdContainerDataSourceHandle backPlateDs =
        HdBackPlateSchema::Builder()
            .SetScaleTweak(
                UsdImagingDataSourceAttribute<GfVec2f>::New(
                    backPlate.GetScaleTweakAttr(), 
                    stageGlobals, 
                    prim.GetPath(),
                    plateLocator.Append(HdBackPlateSchemaTokens->scaleTweak))) 
            .SetRotateXYZTweak(
                UsdImagingDataSourceAttribute<GfVec3f>::New(
                    backPlate.GetRotateXYZTweakAttr(), 
                    stageGlobals,
                    prim.GetPath(),
                    plateLocator.Append(
                        HdBackPlateSchemaTokens->rotateXYZTweak))) 
            .SetTranslateTweak(
                UsdImagingDataSourceAttribute<GfVec3f>::New(
                    backPlate.GetTranslateTweakAttr(),
                    stageGlobals,
                    prim.GetPath(),
                    plateLocator.Append(
                        HdBackPlateSchemaTokens->translateTweak))) 
            .SetImage(
                UsdImagingDataSourceAttribute<SdfAssetPath>::New(
                    backPlate.GetImageAttr(), 
                    stageGlobals,
                    prim.GetPath(),
                    plateLocator.Append(
                        HdBackPlateSchemaTokens->image)))
            .SetAlphaImage(
                UsdImagingDataSourceAttribute<SdfAssetPath>::New(
                    backPlate.GetAlphaImageAttr(), stageGlobals))
            .SetDepthImage(
                UsdImagingDataSourceAttribute<SdfAssetPath>::New(
                    backPlate.GetDepthImageAttr(), stageGlobals))
            .SetDepthMinOffset(
                UsdImagingDataSourceAttribute<float>::New(
                    backPlate.GetDepthMinOffsetAttr(), stageGlobals))
            .SetDepthNormalizingFactor(
                UsdImagingDataSourceAttribute<float>::New(
                    backPlate.GetDepthNormalizingFactorAttr(), stageGlobals))
            .SetDepthCameraSpaceOffset(
                UsdImagingDataSourceAttribute<float>::New(
                    backPlate.GetDepthCameraSpaceOffsetAttr(), stageGlobals))
            .SetLumaSlope(
               UsdImagingDataSourceAttribute<GfVec3f>::New(
                    backPlate.GetLumaSlopeAttr(), 
                    stageGlobals,
                    prim.GetPath(),
                    plateLocator.Append(
                        HdBackPlateSchemaTokens->lumaSlope)))
            .SetLumaPower(
                UsdImagingDataSourceAttribute<GfVec3f>::New(
                    backPlate.GetLumaPowerAttr(), 
                    stageGlobals,
                    prim.GetPath(),
                    plateLocator.Append(
                        HdBackPlateSchemaTokens->lumaPower))) 
            .SetLumaOffset(
                UsdImagingDataSourceAttribute<GfVec3f>::New(
                    backPlate.GetLumaOffsetAttr(), 
                    stageGlobals,
                    prim.GetPath(),
                    plateLocator.Append(
                        HdBackPlateSchemaTokens->lumaOffset))) 
            .SetPlateVisibility(
                HdBackPlateSchema::BuildPlateVisibilityDataSource(plateVisibility))
            .Build();

    return HdRetainedContainerDataSource::New(
            HdCameraSchema::GetSchemaToken(),
            HdRetainedContainerDataSource::New(
            HdBackPlateSchema::GetSchemaToken(),
            HdRetainedContainerDataSource::New(
                appliedInstanceName, backPlateDs)));
}

HdDataSourceLocatorSet
UsdImagingBackPlateAPIAdapter::InvalidateImagingSubprim(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    TfTokenVector const& properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    if (!subprim.IsEmpty() || appliedInstanceName.IsEmpty()) {
        return HdDataSourceLocatorSet();
    }
    const std::string prefix = TfStringPrintf(
        "backPlate:%s:", appliedInstanceName.data());

    for (const TfToken &propertyName : properties) {
        if (TfStringStartsWith(propertyName.GetString(), prefix)) {
            return HdCameraSchema::GetBackPlateLocator()
                .Append(appliedInstanceName);
        }
    }
    return HdDataSourceLocatorSet();
}

PXR_NAMESPACE_CLOSE_SCOPE
