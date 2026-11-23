//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "rmanUsdImaging/renderTerminalTimeSamplingAdapter.h"

#if PXR_VERSION >= 2505 && PXR_VERSION < 2603

#include "pxr/imaging/hd/displayFilterSchema.h"
#include "pxr/imaging/hd/integratorSchema.h"
#include "pxr/imaging/hd/materialNodeParameterSchema.h"
#include "pxr/imaging/hd/materialNodeSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sampleFilterSchema.h"
#include "pxr/usdImaging/usdImaging/dataSourceAttribute.h"

#endif

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = HdPrman_RenderTerminalTimeSamplingAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter>>();
    t.SetFactory<UsdImagingAPISchemaAdapterFactory<Adapter>>();
}

#if PXR_VERSION >= 2505 && PXR_VERSION < 2603

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((riIntegratorShaderId, "ri:integrator:shaderId"))
    ((riSampleFilterShaderId, "ri:sampleFilter:shaderId"))
    ((riDisplayFilterShaderId, "ri:displayFilter:shaderId"))
);

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
    TfToken const& primType,
    UsdImagingDataSourceStageGlobals const& stageGlobals,
    const SdfPath &sceneIndexPath,
    const HdDataSourceLocator &locatorPrefix)
{
    std::vector<TfToken> paramsNames;
    std::vector<HdDataSourceBaseHandle> paramsValues;

    UsdAttributeVector attrs = prim.GetAuthoredAttributes();
    for (const auto& attr : attrs) {
        std::string inputName;
        if (_HasInputPrefix(attr.GetName(), &inputName)) {
            const HdDataSourceLocator paramValueLocator(
                TfToken(inputName), HdMaterialNodeParameterSchemaTokens->value);
            paramsNames.push_back(TfToken(inputName));
            paramsValues.push_back(
                HdMaterialNodeParameterSchema::Builder()
                    .SetValue(
                        UsdImagingDataSourceAttributeNew(
                            attr, stageGlobals,
                            sceneIndexPath,
                            locatorPrefix.Append(paramValueLocator)))
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

HdContainerDataSourceHandle
HdPrman_RenderTerminalTimeSamplingAdapter::GetImagingSubprimData(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    TfToken shaderIdToken;
    TfToken schemaToken;
    HdDataSourceLocator resourceLocator;
    if (prim.HasAttribute(_tokens->riIntegratorShaderId)) {
        shaderIdToken = _tokens->riIntegratorShaderId;
        schemaToken = HdIntegratorSchema::GetSchemaToken();
        resourceLocator = HdIntegratorSchema::GetResourceLocator();
    }
    else if (prim.HasAttribute(_tokens->riSampleFilterShaderId)) {
        shaderIdToken = _tokens->riSampleFilterShaderId;
        schemaToken = HdSampleFilterSchema::GetSchemaToken();
        resourceLocator = HdSampleFilterSchema::GetResourceLocator();
    }
    else if (prim.HasAttribute(_tokens->riDisplayFilterShaderId)) {
        shaderIdToken = _tokens->riDisplayFilterShaderId;
        schemaToken = HdDisplayFilterSchema::GetSchemaToken();
        resourceLocator = HdDisplayFilterSchema::GetResourceLocator();
    }
    else {
        return nullptr;
    }

    return HdRetainedContainerDataSource::New(
        TfToken("__renderTerminalTimeSampling"),
        HdRetainedContainerDataSource::New(
            TfToken("resource"),
            _ComputeResourceDS(prim, shaderIdToken, schemaToken, stageGlobals, prim.GetPath(),
                               resourceLocator.Append(HdMaterialNodeSchemaTokens->parameters))));
}

HdDataSourceLocatorSet
HdPrman_RenderTerminalTimeSamplingAdapter::InvalidateImagingSubprim(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    TfTokenVector const& properties,
    UsdImagingPropertyInvalidationType invalidationType)
{
    HdDataSourceLocatorSet locators;
    for (const TfToken &propertyName : properties) {
        if (_HasInputPrefix(propertyName)) {
            if (prim.HasAttribute(_tokens->riIntegratorShaderId)) {
                locators.insert(HdIntegratorSchema::GetResourceLocator());
            }
            else if (prim.HasAttribute(_tokens->riSampleFilterShaderId)) {
                locators.insert(HdSampleFilterSchema::GetResourceLocator());
            }
            else if (prim.HasAttribute(_tokens->riDisplayFilterShaderId)) {
                locators.insert(HdDisplayFilterSchema::GetResourceLocator());
            }
            break;
        }
    }
    return locators;
}

#endif // PXR_VERSION

PXR_NAMESPACE_CLOSE_SCOPE
