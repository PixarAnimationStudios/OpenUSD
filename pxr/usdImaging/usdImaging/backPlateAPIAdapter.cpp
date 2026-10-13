//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/backPlateAPIAdapter.h"
#include "pxr/usdImaging/usdImaging/dataSourcePrim.h"
#include "pxr/usdImaging/usdImaging/dataSourceMapped.h"

#include "pxr/imaging/hd/backPlateSchema.h"
#include "pxr/imaging/hd/cameraSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"

#include "pxr/usd/usdGeom/backPlateAPI.h"

#include <oneapi/tbb/concurrent_unordered_map.h>

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

using _PropertyMappingsUniquePtr = 
    std::unique_ptr<UsdImagingDataSourceMapped::PropertyMappings>;
// Pairs of USD multi-apply template and Hydra back plate schema property 
// names
const std::vector<std::pair<TfToken, TfToken>> &
_GetUsdGeomToHdPropertyNames() {
    static const  std::vector<std::pair<TfToken, TfToken>> namePairs {
        {UsdGeomTokens->backPlate_MultipleApplyTemplate_AlphaImage,
            HdBackPlateSchemaTokens->alphaImage},
        {UsdGeomTokens->backPlate_MultipleApplyTemplate_DepthCameraSpaceOffset,
            HdBackPlateSchemaTokens->depthCameraSpaceOffset},
        {UsdGeomTokens->backPlate_MultipleApplyTemplate_DepthImage,
            HdBackPlateSchemaTokens->depthImage},
        {UsdGeomTokens->backPlate_MultipleApplyTemplate_DepthMinOffset,
            HdBackPlateSchemaTokens->depthMinOffset},
        {UsdGeomTokens->backPlate_MultipleApplyTemplate_DepthNormalizingFactor,
            HdBackPlateSchemaTokens->depthNormalizingFactor},
        {UsdGeomTokens->backPlate_MultipleApplyTemplate_Image,
            HdBackPlateSchemaTokens->image},
        {UsdGeomTokens->backPlate_MultipleApplyTemplate_LumaOffset,
            HdBackPlateSchemaTokens->lumaOffset},
        {UsdGeomTokens->backPlate_MultipleApplyTemplate_LumaPower,
            HdBackPlateSchemaTokens->lumaPower},
        {UsdGeomTokens->backPlate_MultipleApplyTemplate_LumaSlope, 
            HdBackPlateSchemaTokens->lumaSlope},
        {UsdGeomTokens->backPlate_MultipleApplyTemplate_PlateVisibility,
            HdBackPlateSchemaTokens->plateVisibility},
        {UsdGeomTokens->backPlate_MultipleApplyTemplate_RotateXYZTweak,
            HdBackPlateSchemaTokens->rotateXYZTweak},
        {UsdGeomTokens->backPlate_MultipleApplyTemplate_ScaleTweak,
             HdBackPlateSchemaTokens->scaleTweak},
        {UsdGeomTokens->backPlate_MultipleApplyTemplate_TranslateTweak,
             HdBackPlateSchemaTokens->translateTweak},
    };
    return namePairs;
}

_PropertyMappingsUniquePtr _GetMappingsUncached(const TfToken &instanceName) {
    std::vector<UsdImagingDataSourceMapped::PropertyMapping> result;
    for (const auto &[usdGeomName, hdName] : _GetUsdGeomToHdPropertyNames())
    {
        result.push_back(
            UsdImagingDataSourceMapped::AttributeMapping{{
                UsdSchemaRegistry::MakeMultipleApplyNameInstance(
                    usdGeomName, instanceName),
                HdDataSourceLocator(hdName) }
            });
    }
    return std::make_unique<UsdImagingDataSourceMapped::PropertyMappings>(
        result,
        HdCameraSchema::GetBackPlateLocator().Append(instanceName));
}

const UsdImagingDataSourceMapped::PropertyMappings &
_GetMappings(const TfToken &instanceName)
{
    // Cache each instance's property mappings
    static tbb::concurrent_unordered_map<
        TfToken,
        _PropertyMappingsUniquePtr,
        TfToken::HashFunctor> instanceToMappingsMap;

    const auto it = instanceToMappingsMap.find(instanceName);
    if (it != instanceToMappingsMap.end()) {
        return *(it->second);
    }

    return *(instanceToMappingsMap.emplace(
        instanceName, _GetMappingsUncached(instanceName)).first->second);
}

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

    return HdRetainedContainerDataSource::New(
        HdCameraSchema::GetSchemaToken(),
        HdRetainedContainerDataSource::New(
            HdBackPlateSchema::GetSchemaToken(),
            HdRetainedContainerDataSource::New(
                appliedInstanceName,
                UsdImagingDataSourceMapped::New(
                    prim, 
                    prim.GetPath(),
                    _GetMappings(appliedInstanceName),
                    stageGlobals)
            )
        )
    );
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

    return UsdImagingDataSourceMapped::Invalidate(
        properties, _GetMappings(appliedInstanceName));
}

PXR_NAMESPACE_CLOSE_SCOPE
