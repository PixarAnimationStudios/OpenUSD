//
// Copyright 2022 Pixar
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
#include "pxr/usdImaging/usdImaging/dataSourceMaterial.h"

#include "pxr/usdImaging/usdImaging/dataSourceAttribute.h"

#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hd/materialConnectionSchema.h"
#include "pxr/imaging/hd/materialNetworkSchema.h"
#include "pxr/imaging/hd/materialNodeSchema.h"
#include "pxr/imaging/hd/materialSchema.h"

#include "pxr/base/work/utils.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

class _UsdImagingDataSourceShadingNodeParameters : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_UsdImagingDataSourceShadingNodeParameters);

    bool Has(const TfToken &name) override
    {
        return Get(name) != nullptr;
    }

    TfTokenVector GetNames() override
    {
        TfTokenVector result;
        const std::vector<UsdShadeInput> shadeNodeInputs =
            _shaderNode.GetInputs();
        result.reserve(shadeNodeInputs.size());
        for (UsdShadeInput const & input: shadeNodeInputs) {
            UsdShadeAttributeType attrType;
            UsdAttribute attr(input.GetValueProducingAttribute(&attrType));
            if (attrType == UsdShadeAttributeType::Input) {
                result.push_back(input.GetBaseName());
            }
        }
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        UsdShadeInput input = _shaderNode.GetInput(name);
        if (!input.IsDefined()) {
            return nullptr;
        }

        UsdShadeAttributeType attrType;
        UsdAttribute attr = input.GetValueProducingAttribute(&attrType);
        if (attrType == UsdShadeAttributeType::Input) {
            return UsdImagingDataSourceAttributeNew(attr, _stageGlobals);
        }

        return nullptr;
    }

private:
    _UsdImagingDataSourceShadingNodeParameters(
        UsdShadeShader shaderNode,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : _shaderNode(shaderNode), _stageGlobals(stageGlobals){}

    UsdShadeShader _shaderNode;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
};

class _UsdImagingDataSourceShadingNodeInputs : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_UsdImagingDataSourceShadingNodeInputs);

    bool Has(const TfToken &name) override
    {
        return Get(name) != nullptr;
    }

    TfTokenVector GetNames() override
    {
        TfTokenVector result;
        const std::vector<UsdShadeInput> shadeNodeInputs =
            _shaderNode.GetInputs();
        result.reserve(shadeNodeInputs.size());
        for (UsdShadeInput const & input: shadeNodeInputs) {
            UsdShadeAttributeType attrType;
            UsdAttribute attr = input.GetValueProducingAttribute(&attrType);
            if (attrType == UsdShadeAttributeType::Output) {
                result.push_back(input.GetBaseName());
            }
        }
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        UsdShadeInput input = _shaderNode.GetInput(name);
        if (!input.IsDefined()) {
            return nullptr;
        }

        UsdShadeAttributeVector attrs =
            input.GetValueProducingAttributes(/*shaderOutputsOnly =*/ true);

        if (attrs.empty()) {
            return nullptr;
        }

        TfSmallVector<HdDataSourceBaseHandle, 8> elements;
        elements.reserve(attrs.size());

        for (const UsdAttribute &attr : attrs) {
            const TfToken outputPath(attr.GetPrim().GetPath().GetString());
            const TfToken outputName(
                UsdShadeOutput(attr).GetBaseName().GetString());

            elements.push_back(HdMaterialConnectionSchema::BuildRetained(
                HdRetainedTypedSampledDataSource<TfToken>::New(outputPath),
                HdRetainedTypedSampledDataSource<TfToken>::New(outputName)));
        }

        return HdRetainedSmallVectorDataSource::New(
            elements.size(), elements.data());
    }

private:
    _UsdImagingDataSourceShadingNodeInputs(
        UsdShadeShader shaderNode,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : _shaderNode(shaderNode), _stageGlobals(stageGlobals){}

    UsdShadeShader _shaderNode;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
};


class _UsdImagingDataSourceShadingNode : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_UsdImagingDataSourceShadingNode);

    bool Has(const TfToken &name) override
    {
        return std::find(
            HdMaterialNodeSchemaTokens->allTokens.begin(),
            HdMaterialNodeSchemaTokens->allTokens.end(),
            name)
                != HdMaterialNodeSchemaTokens->allTokens.end();
    }

    TfTokenVector GetNames() override
    {
        return HdMaterialNodeSchemaTokens->allTokens;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (name == HdMaterialNodeSchemaTokens->nodeIdentifier) {
            TfToken nodeId;
            _shaderNode.GetShaderId(&nodeId);
            return HdRetainedTypedSampledDataSource<TfToken>::New(nodeId);
        }

        if (name == HdMaterialNodeSchemaTokens->parameters) {
            return _UsdImagingDataSourceShadingNodeParameters::New(
                _shaderNode, _stageGlobals);
        }

        if (name == HdMaterialNodeSchemaTokens->inputConnections) {
            return _UsdImagingDataSourceShadingNodeInputs::New(
                _shaderNode, _stageGlobals);
        }

        return nullptr;
    }

private:
    _UsdImagingDataSourceShadingNode(
        UsdShadeShader shaderNode,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : _shaderNode(shaderNode), _stageGlobals(stageGlobals){}

    UsdShadeShader _shaderNode;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
};


} // namespace anonymous

// ----------------------------------------------------------------------------

UsdImagingDataSourceMaterial::UsdImagingDataSourceMaterial(
    UsdPrim usdPrim,
    const UsdImagingDataSourceStageGlobals & stageGlobals)
: _usdPrim(usdPrim)
, _stageGlobals(stageGlobals)
{
}

UsdImagingDataSourceMaterial::~UsdImagingDataSourceMaterial()
{
    WorkMoveDestroyAsync(_networks);
}

bool 
UsdImagingDataSourceMaterial::Has(const TfToken & name)
{
    // The only way to tell if a protocol is present is to compute it. We
    // cache the networks so that we won't do the work again when we call Get().
    if (Get(name) != nullptr) {
        return true;
    }
    return false;
}

TfTokenVector 
UsdImagingDataSourceMaterial::GetNames()
{
    // Just return the universal render context here, though we may return
    // networks for other protocols.

    // NOTE: We do want to be able to return all render contexts but don't
    //       want to rely on the render delegate answering this question as
    //       scene indices may be observed by multiple render delegates at
    //       once. We will likely need to base this on available connections
    //       of the material prim itself.

    TfTokenVector result;
    result.push_back(HdMaterialSchemaTokens->universalRenderContext);
    return result;
}

using _TokenDataSourceMap =
    TfDenseHashMap<TfToken, HdDataSourceBaseHandle, TfHash>;

static void
_WalkGraph(
    UsdShadeShader const &shadeNode,
    _TokenDataSourceMap *outputNodes,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (!shadeNode) {
        return;
    }

    // Check if path is incorrect
    SdfPath const nodePath = shadeNode.GetPath();
    if (nodePath == SdfPath::EmptyPath()) {
        return;
    }

    TfToken const nodeName(nodePath.GetString());
    if (outputNodes->find(nodeName) != outputNodes->end()) {
        return;
    }

    // Visit inputs of this node to ensure they are emitted first.
    const std::vector<UsdShadeInput> shadeNodeInputs = shadeNode.GetInputs();

    for (UsdShadeInput const & input: shadeNodeInputs) {
        TfToken const inputName = input.GetBaseName();

        UsdShadeAttributeVector attrs =
            input.GetValueProducingAttributes(/*shaderOutputsOnly =*/ true);

        if (attrs.empty()) {
            continue;
        }

        for (const UsdAttribute &attr : attrs) {
            _WalkGraph(
                UsdShadeShader(attr.GetPrim()),
                outputNodes,
                stageGlobals);
        }
    }

    HdDataSourceBaseHandle nodeValue =
        _UsdImagingDataSourceShadingNode::New(shadeNode, stageGlobals);

    outputNodes->insert({nodeName, nodeValue});
}

static 
HdDataSourceBaseHandle
_BuildMaterial(
    UsdShadeMaterial const &usdMat, 
    UsdImagingDataSourceStageGlobals const &stageGlobals,
    TfToken const& context)
{
    TfTokenVector terminalsNames;
    std::vector<HdDataSourceBaseHandle> terminalsValues;
    TfTokenVector nodeNames;
    std::vector<HdDataSourceBaseHandle> nodeValues;

    _TokenDataSourceMap nodeDataSources;

    for (UsdShadeOutput &output : usdMat.GetOutputs()) {
        TfToken outputName = output.GetBaseName();

        for (const UsdShadeConnectionSourceInfo &sourceInfo :
                output.GetConnectedSources()) {
            if (!sourceInfo.IsValid()) {
                continue;
            }

            UsdShadeShader upstreamShader =
                UsdShadeShader(sourceInfo.source.GetPrim());

            _WalkGraph(upstreamShader,
                &nodeDataSources,
                stageGlobals);

            terminalsNames.push_back(outputName);
            terminalsValues.push_back(HdMaterialConnectionSchema::BuildRetained(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    TfToken(upstreamShader.GetPath().GetString())),
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    sourceInfo.sourceName)));
        }
    }

    if (terminalsNames.empty()) {
        return nullptr;
    }

    HdContainerDataSourceHandle terminalsDs = 
        HdRetainedContainerDataSource::New(
            terminalsNames.size(),
            terminalsNames.data(),
            terminalsValues.data());

    nodeNames.reserve(nodeDataSources.size());
    nodeValues.reserve(nodeDataSources.size());
    for (const auto &tokenDsPair : nodeDataSources) {
        nodeNames.push_back(tokenDsPair.first);
        nodeValues.push_back(tokenDsPair.second);
    }


    HdContainerDataSourceHandle nodesDs = 
        HdRetainedContainerDataSource::New(
            nodeNames.size(),
            nodeNames.data(),
            nodeValues.data());

    // Create the material network, potentially one per network selector
    HdDataSourceBaseHandle network = HdMaterialNetworkSchema::BuildRetained(
        nodesDs,
        terminalsDs);

    return network;
}

HdDataSourceBaseHandle 
UsdImagingDataSourceMaterial::Get(const TfToken &name)
{
    TRACE_FUNCTION();

    const auto it = _networks.find(name);

    if (it != _networks.end()) {
        return it->second;
    }

    HdDataSourceBaseHandle networkDs = _BuildMaterial(
        UsdShadeMaterial(_usdPrim), _stageGlobals, name);

    _networks[name] = networkDs;
    return networkDs;
}


PXR_NAMESPACE_CLOSE_SCOPE
