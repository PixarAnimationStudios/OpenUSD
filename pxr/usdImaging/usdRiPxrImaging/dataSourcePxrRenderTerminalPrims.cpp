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
// You may obtain a copy of the Apache License atNAMESPACE
// language governing permissions and limitations under the Apache License.
//
#include "pxr/usdImaging/usdRiImaging/dataSourcePxrRenderTerminalPrims.h"
#include "pxr/usdImaging/usdImaging/dataSourceAttribute.h"

#include "pxr/imaging/hd/displayFilterSchema.h"
#include "pxr/imaging/hd/integratorSchema.h"
#include "pxr/imaging/hd/sampleFilterSchema.h"

#include "pxr/imaging/hd/retainedDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (inputs)
    ((riDisplayFilterShaderId,  "ri:displayFilter:shaderId"))
    ((riIntegratorShaderId,     "ri:integrator:shaderId"))
    ((riSampleFilterShaderId,   "ri:sampleFilter:shaderId"))
);

namespace {

static TfToken
_GetNodeTypeId(
    UsdPrim const& prim,
    TfToken const& shaderIdToken,
    TfToken const& primTypeToken)
{
    UsdAttribute attr = prim.GetAttribute(shaderIdToken);
    if (attr) {
        VtValue value;
        if (attr.Get(&value)) {
            if (value.IsHolding<TfToken>()) {
                return value.UncheckedGet<TfToken>();
            }
        }
    }
    return primTypeToken;
}

static HdContainerDataSourceHandle
_ComputeResourceDS(
    UsdPrim const& prim,
    TfToken const& shaderIdToken,
    TfToken const& primTypeToken)
{
    std::vector<TfToken> paramsNames;
    std::vector<HdDataSourceBaseHandle> paramsValues;

    UsdAttributeVector attrs = prim.GetAuthoredAttributes();
    for (const auto& attr : attrs) {
        VtValue value;
        const std::pair<std::string, bool> inputName =
            SdfPath::StripPrefixNamespace(attr.GetName(), _tokens->inputs);
        if (inputName.second && attr.Get(&value)) {
            paramsNames.push_back(TfToken(inputName.first));
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
            .SetInputConnections(HdRetainedContainerDataSource::New())
            .SetNodeIdentifier(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    _GetNodeTypeId(prim, shaderIdToken, primTypeToken)))
        .Build();
    
    return nodeDS;
}

}


// ----------------------------------------------------------------------------
//                                Integrator 
// ----------------------------------------------------------------------------

UsdRiImagingDataSourceIntegratorPrim::UsdRiImagingDataSourceIntegratorPrim(
    const SdfPath &sceneIndexPath,
    UsdPrim usdPrim,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
    : UsdImagingDataSourcePrim(sceneIndexPath, usdPrim, stageGlobals)
{
}

TfTokenVector 
UsdRiImagingDataSourceIntegratorPrim::GetNames()
{
    // Note: Skip properties on UsdImagingDataSourcePrim.
    return { HdIntegratorSchemaTokens->integrator };
}

HdDataSourceBaseHandle 
UsdRiImagingDataSourceIntegratorPrim::Get(const TfToken & name)
{
    if (name == HdIntegratorSchemaTokens->integrator) {
        return 
            HdRetainedContainerDataSource::New(
                HdIntegratorSchemaTokens->integratorResource,
                _ComputeResourceDS(
                    _GetUsdPrim(),
                    _tokens->riIntegratorShaderId,
                    HdIntegratorSchemaTokens->integrator));
    } 

    // Note: Skip properties on UsdImagingDataSourcePrim.
    return nullptr;
}

HdDataSourceLocatorSet
UsdRiImagingDataSourceIntegratorPrim::Invalidate(
    UsdPrim const& prim,
    const TfToken &subprim,
    const TfTokenVector &properties)
{
    TRACE_FUNCTION();

    HdDataSourceLocatorSet locators;
    for (const TfToken &propertyName : properties) {
        if (propertyName != HdIntegratorSchemaTokens->integratorResource) {
            locators.insert(
                HdIntegratorSchema::GetDefaultLocator().Append(propertyName));
        } else {
            // It is likely that the property is an attribute that's
            // aggregated under the resource. For performance, skip
            // validating whether that is the case.
            locators.insert(HdIntegratorSchema::GetResourceLocator());
        }
        // Note: Skip UsdImagingDataSourcePrim::Invalidate(...)
        // since none of the "base" set of properties are relevant here.
    }

    return locators;
}

// ----------------------------------------------------------------------------
//                              Sample Filter
// ----------------------------------------------------------------------------

UsdRiImagingDataSourceSampleFilterPrim::UsdRiImagingDataSourceSampleFilterPrim(
    const SdfPath &sceneIndexPath,
    UsdPrim usdPrim,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
    : UsdImagingDataSourcePrim(sceneIndexPath, usdPrim, stageGlobals)
{
}

TfTokenVector 
UsdRiImagingDataSourceSampleFilterPrim::GetNames()
{
    // Note: Skip properties on UsdImagingDataSourcePrim.
    return { HdSampleFilterSchemaTokens->sampleFilter };
}

HdDataSourceBaseHandle 
UsdRiImagingDataSourceSampleFilterPrim::Get(const TfToken & name)
{
    if (name == HdSampleFilterSchemaTokens->sampleFilter) {
        return 
            HdRetainedContainerDataSource::New(
                HdSampleFilterSchemaTokens->sampleFilterResource,
                _ComputeResourceDS(
                    _GetUsdPrim(),
                    _tokens->riSampleFilterShaderId,
                    HdSampleFilterSchemaTokens->sampleFilter));
    } 

    // Note: Skip properties on UsdImagingDataSourcePrim.
    return nullptr;
}

HdDataSourceLocatorSet
UsdRiImagingDataSourceSampleFilterPrim::Invalidate(
    UsdPrim const& prim,
    const TfToken &subprim,
    const TfTokenVector &properties)
{
    TRACE_FUNCTION();

    HdDataSourceLocatorSet locators;
    for (const TfToken &propertyName : properties) {
        if (propertyName != HdSampleFilterSchemaTokens->sampleFilterResource) {
            locators.insert(
                HdSampleFilterSchema::GetDefaultLocator().Append(propertyName));
        } else {
            // It is likely that the property is an attribute that's
            // aggregated under the resource. For performance, skip
            // validating whether that is the case.
            locators.insert(HdSampleFilterSchema::GetResourceLocator());
        }
        // Note: Skip UsdImagingDataSourcePrim::Invalidate(...)
        // since none of the "base" set of properties are relevant here.
    }

    return locators;
}

// ----------------------------------------------------------------------------
//                               Display Filter
// ----------------------------------------------------------------------------

UsdRiImagingDataSourceDisplayFilterPrim::UsdRiImagingDataSourceDisplayFilterPrim(
    const SdfPath &sceneIndexPath,
    UsdPrim usdPrim,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
    : UsdImagingDataSourcePrim(sceneIndexPath, usdPrim, stageGlobals)
{
}

TfTokenVector 
UsdRiImagingDataSourceDisplayFilterPrim::GetNames()
{
    // Note: Skip properties on UsdImagingDataSourcePrim.
    return { HdDisplayFilterSchemaTokens->displayFilter };
}

HdDataSourceBaseHandle 
UsdRiImagingDataSourceDisplayFilterPrim::Get(const TfToken & name)
{
    if (name == HdDisplayFilterSchemaTokens->displayFilter) {
        return 
            HdRetainedContainerDataSource::New(
                HdDisplayFilterSchemaTokens->displayFilterResource,
                _ComputeResourceDS(
                    _GetUsdPrim(),
                    _tokens->riDisplayFilterShaderId,
                    HdDisplayFilterSchemaTokens->displayFilter));
            
    } 

    // Note: Skip properties on UsdImagingDataSourcePrim.
    return nullptr;
}

HdDataSourceLocatorSet
UsdRiImagingDataSourceDisplayFilterPrim::Invalidate(
    UsdPrim const& prim,
    const TfToken &subprim,
    const TfTokenVector &properties)
{
    TRACE_FUNCTION();

    HdDataSourceLocatorSet locators;
    for (const TfToken &propertyName : properties) {
        if (propertyName != HdDisplayFilterSchemaTokens->displayFilterResource) {
            locators.insert(
                HdDisplayFilterSchema::GetDefaultLocator().Append(propertyName));
        } else {
            // It is likely that the property is an attribute that's
            // aggregated under the resource. For performance, skip
            // validating whether that is the case.
            locators.insert(
                HdDisplayFilterSchema::GetResourceLocator());
        }
        // Note: Skip UsdImagingDataSourcePrim::Invalidate(...)
        // since none of the "base" set of properties are relevant here.
    }

    return locators;
}

PXR_NAMESPACE_CLOSE_SCOPE