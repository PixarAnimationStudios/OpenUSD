//
// Copyright 2023 Pixar
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

#ifndef PXR_USD_IMAGING_USD_RI_IMAGING_DATA_SOURCE_RENDER_TERMINAL_PRIMS_H
#define PXR_USD_IMAGING_USD_RI_IMAGING_DATA_SOURCE_RENDER_TERMINAL_PRIMS_H

#include "pxr/usdImaging/usdImaging/dataSourcePrim.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/materialNodeSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/usdImaging/usdRiImaging/api.h"


PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdRiImaging_DataSourceRenderTerminalPrim
///
/// A prim data source representing Render Terminal prims inheriting from 
///     PxrDisplayFilterPluginBase, 
///     PxrIntegratorPluginBase, 
///     PxrSampleFilterPluginBase
///
template <typename TerminalSchema>
class UsdRiImaging_DataSourceRenderTerminalPrim : public UsdImagingDataSourcePrim
{
public:
    HD_DECLARE_DATASOURCE(UsdRiImaging_DataSourceRenderTerminalPrim);

    USDRIIMAGING_API
    TfTokenVector GetNames() override;

    USDRIIMAGING_API
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    USDRIIMAGING_API
    static
    HdDataSourceLocatorSet
    Invalidate(
        UsdPrim const& prim,
        const TfToken &subprim,
        const TfTokenVector &properties,
        UsdImagingPropertyInvalidationType invalidationType);

private:
    // Private constructor, use static New() instead.
    UsdRiImaging_DataSourceRenderTerminalPrim(
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
                HdRetainedTypedSampledDataSource<VtValue>::New(value)
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
UsdRiImaging_DataSourceRenderTerminalPrim<TerminalSchema>::
UsdRiImaging_DataSourceRenderTerminalPrim(
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
UsdRiImaging_DataSourceRenderTerminalPrim<TerminalSchema>::GetNames()
{
    // Note: Skip properties on UsdImagingDataSourcePrim.
    return { TerminalSchema::GetSchemaToken() };
}

template <typename TerminalSchema>
HdDataSourceBaseHandle 
UsdRiImaging_DataSourceRenderTerminalPrim<TerminalSchema>::Get(
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
UsdRiImaging_DataSourceRenderTerminalPrim<TerminalSchema>::Invalidate(
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

#endif // PXR_USD_IMAGING_USD_RI_IMAGING_DATA_SOURCE_RENDER_TERMINAL_PRIMS_H
