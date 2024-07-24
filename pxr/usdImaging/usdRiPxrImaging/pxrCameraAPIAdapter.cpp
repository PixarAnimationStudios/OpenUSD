//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdRiPxrImaging/pxrCameraAPIAdapter.h"

#include "pxr/usdImaging/usdImaging/dataSourceMapped.h"

#include "pxr/imaging/hd/cameraSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"

#include "pxr/usd/usd/primDefinition.h"
#include "pxr/usd/usd/schemaRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

static const char * const _appliedSchemaName = "PxrCameraAPI";

std::pair<TfToken, TfToken>
_SplitNamespace(const TfToken &name)
{
    static const char namespaceDelimiter =
        SdfPathTokens->namespaceDelimiter.GetText()[0];
    
    const std::string &str = name.GetString();
    const size_t i = str.find(namespaceDelimiter);
    if (i == std::string::npos) {
        return { TfToken(), TfToken() };
    }

    return { TfToken(str.substr(0, i)),
             TfToken(str.substr(i + 1)) };
}

HdSampledDataSourceHandle
_DataSourceAuthoredAttributeNew(
    const UsdAttribute &usdAttr,
    const UsdImagingDataSourceStageGlobals &stageGlobals,
    const SdfPath &sceneIndexPath,
    const HdDataSourceLocator &timeVaryingFlagLocator)
{
    UsdAttributeQuery query(usdAttr);
    if (!query.HasAuthoredValue()) {
        return nullptr;
    }
    return UsdImagingDataSourceAttributeNew(
        query, stageGlobals, sceneIndexPath, timeVaryingFlagLocator);
}

//
// This method or a generalization of this method might be useful for other
// adapters. Consider moving it to a more central place such as UsdImaging.
//
std::vector<UsdImagingDataSourceMapped::AttributeMapping>
_GetNamespacedAttributeMappingsForAppliedSchema(
    const TfToken &appliedSchemaName)
{
    std::vector<UsdImagingDataSourceMapped::AttributeMapping> result;

    const UsdPrimDefinition * const primDef =
        UsdSchemaRegistry::GetInstance().FindAppliedAPIPrimDefinition(
            appliedSchemaName);
    if (!primDef) {
        TF_CODING_ERROR(
            "Could not find definition for applied schema '%s'.",
            appliedSchemaName.GetText());
        return result;
    }

    for (const TfToken &usdName : primDef->GetPropertyNames()) {
        const std::pair<TfToken, TfToken> namespaceAndName =
            _SplitNamespace(usdName);
        if (namespaceAndName.second.IsEmpty()) {
            TF_CODING_ERROR(
                "Expected all attributes on applied schema "
                "'%s' to be namespaced but attribute '%s' "
                "schema has no namespace.",
                appliedSchemaName.GetText(),
                usdName.GetText());
            continue;
        }

        result.push_back(
            { usdName,
              HdDataSourceLocator(namespaceAndName.first,
                                  namespaceAndName.second),
              _DataSourceAuthoredAttributeNew});
    }

    return result;
}

const UsdImagingDataSourceMapped::AttributeMappings &
_GetMappings() {
    static const UsdImagingDataSourceMapped::AttributeMappings result(
        _GetNamespacedAttributeMappingsForAppliedSchema(
            TfToken(_appliedSchemaName)),
        HdCameraSchema::GetNamespacedPropertiesLocator());
    return result;
}

}

TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdRiPxrImagingCameraAPIAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter>>();
    t.SetFactory<UsdImagingAPISchemaAdapterFactory<Adapter>>();
}

HdContainerDataSourceHandle
UsdRiPxrImagingCameraAPIAdapter::GetImagingSubprimData(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const &appliedInstanceName,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (!subprim.IsEmpty() || !appliedInstanceName.IsEmpty()) {
        return nullptr;
    }

    return
        HdRetainedContainerDataSource::New(
            HdCameraSchema::GetSchemaToken(),
            HdCameraSchema::Builder()
                .SetNamespacedProperties(
                    UsdImagingDataSourceMapped::New(
                        prim, prim.GetPath(), _GetMappings(), stageGlobals))
                .Build());
}

HdDataSourceLocatorSet
UsdRiPxrImagingCameraAPIAdapter::InvalidateImagingSubprim(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const &appliedInstanceName,
    TfTokenVector const& properties,
    UsdImagingPropertyInvalidationType invalidationType)
{
    if (!subprim.IsEmpty() || !appliedInstanceName.IsEmpty()) {
        return HdDataSourceLocatorSet();
    }

    return UsdImagingDataSourceMapped::Invalidate(
        properties, _GetMappings());
}

PXR_NAMESPACE_CLOSE_SCOPE
