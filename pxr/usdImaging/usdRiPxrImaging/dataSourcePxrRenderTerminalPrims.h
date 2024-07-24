//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_IMAGING_USD_RI_PXR_IMAGING_DATA_SOURCE_RENDER_TERMINAL_PRIMS_H
#define PXR_USD_IMAGING_USD_RI_PXR_IMAGING_DATA_SOURCE_RENDER_TERMINAL_PRIMS_H

#include "pxr/usdImaging/usdImaging/dataSourcePrim.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/materialNodeSchema.h"
#include "pxr/imaging/hd/materialNodeParameterSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/usdImaging/usdRiPxrImaging/api.h"


PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdRiPxrImaging_DataSourceRenderTerminalPrim
///
/// A prim data source representing Render Terminal prims inheriting from 
///     PxrDisplayFilterPluginBase, 
///     PxrIntegratorPluginBase, 
///     PxrSampleFilterPluginBase
///
template <typename TerminalSchema>
class UsdRiPxrImaging_DataSourceRenderTerminalPrim : public UsdImagingDataSourcePrim
{
public:
    HD_DECLARE_DATASOURCE(UsdRiPxrImaging_DataSourceRenderTerminalPrim);

    USDRIPXRIMAGING_API
    TfTokenVector GetNames() override;

    USDRIPXRIMAGING_API
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    USDRIPXRIMAGING_API
    static
    HdDataSourceLocatorSet
    Invalidate(
        UsdPrim const& prim,
        const TfToken &subprim,
        const TfTokenVector &properties,
        UsdImagingPropertyInvalidationType invalidationType);

private:
    // Private constructor, use static New() instead.
    UsdRiPxrImaging_DataSourceRenderTerminalPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const TfToken &shaderId,
        const UsdImagingDataSourceStageGlobals &stageGlobals);

    TfToken _shaderId;
};


namespace {

static bool
_HasInputPrefix(const TfToken &attrName, std::string *inputName=nullptr)
{
    const std::pair<std::string, bool> strippedInputName =
        SdfPath::StripPrefixNamespace(attrName, "inputs");

    if (inputName) {
        *inputName = strippedInputName.first;
    }
    return strippedInputName.second;
}

static TfToken
_GetNodeTypeId(
    UsdPrim const& prim,
    TfToken const& shaderId,
    TfToken const& primType)
{
    UsdAttribute attr = prim.GetAttribute(shaderId);
    if (attr) {
        VtValue value;
        if (attr.Get(&value)) {
            if (value.IsHolding<TfToken>()) {
                return value.UncheckedGet<TfToken>();
            }
        }
    }
    return primType;
}

static HdContainerDataSourceHandle
_ComputeResourceDS(
    UsdPrim const& prim,
    TfToken const& shaderId,
    TfToken const& primType)
{
    std::vector<TfToken> paramsNames;
    std::vector<HdDataSourceBaseHandle> paramsValues;

    UsdAttributeVector attrs = prim.GetAuthoredAttributes();
    for (const auto& attr : attrs) {
        VtValue value;
        std::string inputName;
        if (_HasInputPrefix(attr.GetName(), &inputName) && attr.Get(&value)) {
            paramsNames.push_back(TfToken(inputName));
            paramsValues.push_back(
                HdMaterialNodeParameterSchema::Builder()
                    .SetValue(
                        HdRetainedTypedSampledDataSource<VtValue>::New(value))
                    .Build()
            );
        }
    }

    HdContainerDataSourceHandle nodeDS =
        HdMaterialNodeSchema::Builder()
            .SetParameters(
                HdRetainedContainerDataSource::New(
                    paramsNames.size(), 
                    paramsNames.data(),
                    paramsValues.data()))
            .SetNodeIdentifier(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    _GetNodeTypeId(prim, shaderId, primType)))
        .Build();
    
    return nodeDS;
}

}

template <typename TerminalSchema>
UsdRiPxrImaging_DataSourceRenderTerminalPrim<TerminalSchema>::
UsdRiPxrImaging_DataSourceRenderTerminalPrim(
    const SdfPath &sceneIndexPath,
    UsdPrim usdPrim,
    const TfToken &shaderId,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
    : UsdImagingDataSourcePrim(sceneIndexPath, usdPrim, stageGlobals)
    , _shaderId(shaderId)
{
}

template <typename TerminalSchema>
TfTokenVector 
UsdRiPxrImaging_DataSourceRenderTerminalPrim<TerminalSchema>::GetNames()
{
    // Note: Skip properties on UsdImagingDataSourcePrim.
    return { TerminalSchema::GetSchemaToken() };
}

template <typename TerminalSchema>
HdDataSourceBaseHandle 
UsdRiPxrImaging_DataSourceRenderTerminalPrim<TerminalSchema>::Get(
    const TfToken & name)
{
    if (name == TerminalSchema::GetSchemaToken()) {
        return 
            HdRetainedContainerDataSource::New(
                TfToken("resource"),
                _ComputeResourceDS(_GetUsdPrim(), _shaderId, name));
    }

    // Note: Skip properties on UsdImagingDataSourcePrim.
    return nullptr;
}

template <typename TerminalSchema>
HdDataSourceLocatorSet
UsdRiPxrImaging_DataSourceRenderTerminalPrim<TerminalSchema>::Invalidate(
    UsdPrim const& prim,
    const TfToken &subprim,
    const TfTokenVector &properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    TRACE_FUNCTION();

    HdDataSourceLocatorSet locators;
    for (const TfToken &propertyName : properties) {
        // Properties with the "inputs" prefix are aggregated under the Resource
        if (_HasInputPrefix(propertyName)) {
            locators.insert(TerminalSchema::GetResourceLocator());
        }
        // Note: Skip UsdImagingDataSourcePrim::Invalidate(...)
        // since none of the "base" set of properties are relevant here.
    }

    return locators;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_RI_PXR_IMAGING_DATA_SOURCE_RENDER_TERMINAL_PRIMS_H
