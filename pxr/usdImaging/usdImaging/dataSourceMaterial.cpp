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

#include "pxr/usd/usdLux/lightAPI.h"
#include "pxr/usd/usdLux/lightFilter.h"

#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"
#include "pxr/usd/usdShade/nodeDefAPI.h"

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hd/materialConnectionSchema.h"
#include "pxr/imaging/hd/materialNetworkSchema.h"
#include "pxr/imaging/hd/materialNodeSchema.h"
#include "pxr/imaging/hd/materialSchema.h"

#include "pxr/base/work/utils.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (info)
    (sdrMetadata)
    (sourceAsset)
    (subIdentifier)
    ((infoSdrMetadata, "info:sdrMetadata"))
);

namespace
{

class _UsdImagingDataSourceShadingNodeParameters : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_UsdImagingDataSourceShadingNodeParameters);

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
            return UsdImagingDataSourceAttributeNew(attr, _stageGlobals,
                _sceneIndexPath,
                    _locatorPrefix.IsEmpty()
                        ? _locatorPrefix
                        : _locatorPrefix.Append(name));
        }

        // fallback case for requested but unauthored inputs on lights or
        // light filters -- which will not return a value for
        // GetValueProducingAttribute but can still provide an attr
        if (_shaderNode.GetPrim().HasAPI<UsdLuxLightAPI>()
                || _shaderNode.GetPrim().IsA<UsdLuxLightFilter>()) {
            attr = input.GetAttr();
            if (attr) {
                return UsdImagingDataSourceAttributeNew(attr, _stageGlobals,
                    _sceneIndexPath,
                        _locatorPrefix.IsEmpty()
                            ? _locatorPrefix
                            : _locatorPrefix.Append(name));
            }
        }

        return nullptr;
    }

private:
    _UsdImagingDataSourceShadingNodeParameters(
        UsdShadeShader shaderNode,
        const UsdImagingDataSourceStageGlobals &stageGlobals,
        const SdfPath &sceneIndexPath,
        const HdDataSourceLocator &locatorPrefix)
    : _shaderNode(shaderNode)
    , _stageGlobals(stageGlobals)
    , _sceneIndexPath(sceneIndexPath)
    , _locatorPrefix(locatorPrefix)
    {}

    UsdShadeShader _shaderNode;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
    SdfPath _sceneIndexPath;
    HdDataSourceLocator _locatorPrefix;
};

class _UsdImagingDataSourceShadingNodeInputs : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_UsdImagingDataSourceShadingNodeInputs);

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

template <typename T>
class _UsdImagingDataSourceRenderContextIdentifiers
        : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_UsdImagingDataSourceRenderContextIdentifiers);

    TfTokenVector GetNames() override
    {
        TfTokenVector result;
        for (const TfToken &propName :  _t.GetPrim().GetPropertyNames()) {
            const std::string &tokenStr = propName.GetString();
            std::size_t firstDelimIndex = tokenStr.find(':');
            if (firstDelimIndex == std::string::npos) {
                continue;
            }

            if (tokenStr.substr(firstDelimIndex + 1) !=
                    UsdLuxTokens->lightShaderId.GetString()) {
                continue;
            }

            result.push_back(TfToken(tokenStr.substr(0, firstDelimIndex)));
        }

        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (UsdAttribute attr = _t.GetShaderIdAttrForRenderContext(name)) {
            TfToken shaderId;
            if (attr.Get(&shaderId)) {
                return HdRetainedTypedSampledDataSource<TfToken>::New(shaderId);
            }
        }

        return nullptr;
    }

private:

    _UsdImagingDataSourceRenderContextIdentifiers(
        const T &t)
    : _t(t){}

    UsdLuxLightAPI _t;
};

// returns true if the name is of the form "info:<sourceType>:<leafName>".
static bool
_ParseInfoName(const std::string& name, TfToken* sourceType, TfToken* leafName)
{
    auto nameTokens = SdfPath::TokenizeIdentifierAsTokens(name);
    if (nameTokens.size() != 3 || nameTokens[0] != _tokens->info) {
        return false;
    }

    if (sourceType) {
        *sourceType = nameTokens[1];
    }

    if (leafName) {
        *leafName = nameTokens[2];
    }
    return true;
}

// For things that are "sourceAsset", we populate the
// renderContextNodeIdentifier with the known "info:" attributes as well as some
// other relevant data.
class _UsdImagingRenderContextNodeIdentifiersForSourceAsset
    : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(
        _UsdImagingRenderContextNodeIdentifiersForSourceAsset);

    TfTokenVector GetNames() override
    {
        UsdShadeNodeDefAPI nodeDef(_shaderNode);

        TfTokenVector names;
        names.push_back(_tokens->infoSdrMetadata);
        for (const UsdAttribute& attr : _GetSourceAssetAttributes()) {
            names.push_back(attr.GetName());

            TfToken sourceType;
            if (!_ParseInfoName(attr.GetName(), &sourceType, nullptr)) {
                continue;
            }

            // the per-sourceType subidentifier gets saved as
            // "info:<sourceType>:subIdentifier".
            if (nodeDef
                && nodeDef.GetSourceAssetSubIdentifier(nullptr, sourceType)) {
                // info:<sourceType>:subIdentifier
                const TfToken subIdentifier(SdfPath::JoinIdentifier(
                    TfTokenVector({ _tokens->info, sourceType,
                                    _tokens->subIdentifier })));
                names.push_back(subIdentifier);
            }
        }
        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken& name) override
    {
        if (name == _tokens->infoSdrMetadata) {
            VtDictionary metadata;
            _shaderNode.GetPrim().GetMetadata(
                UsdShadeTokens->sdrMetadata, &metadata);
            return HdRetainedTypedSampledDataSource<VtDictionary>::New(
                metadata);
        }

        TfToken sourceType, leafName;
        if (!_ParseInfoName(name, &sourceType, &leafName)) {
            return nullptr;
        }

        if (leafName == _tokens->subIdentifier) {
            TfToken subIdentifier;
            if (UsdShadeNodeDefAPI nodeDef = UsdShadeNodeDefAPI(_shaderNode)) {
                if (nodeDef.GetSourceAssetSubIdentifier(
                        &subIdentifier, sourceType)) {
                    return HdRetainedTypedSampledDataSource<TfToken>::New(
                        subIdentifier);
                }
            }
            return nullptr;
        }

        if (leafName == _tokens->sourceAsset) {
            SdfAssetPath sourceAssetPath;
            UsdAttribute attr = _shaderNode.GetPrim().GetAttribute(name);
            if (!attr || !attr.Get(&sourceAssetPath)
                || sourceAssetPath.GetAssetPath().empty()) {
                return nullptr;
            }

            // XXX we want to have usd do the resolution.. do we noed to do
            // something special to do that?
            const std::string& resolvedUri = sourceAssetPath.GetResolvedPath();

            // Create a discoveryResult only if the referenced
            // sourceAsset can be resolved.
            // XXX: Should we do this regardless and expect the parser
            // to be able to resolve the unresolved asset path?
            if (resolvedUri.empty()) {
                TF_WARN(
                    "Unable to resolve info:sourceAsset <%s> with value "
                    "@%s@.",
                    attr.GetPath().GetText(),
                    sourceAssetPath.GetAssetPath().c_str());
                return nullptr;
            }

            return HdRetainedTypedSampledDataSource<SdfAssetPath>::New(
                sourceAssetPath);
        }

        return nullptr;
    }

private:
    _UsdImagingRenderContextNodeIdentifiersForSourceAsset(
        const UsdShadeShader& shaderNode)
        : _shaderNode(shaderNode)
    {
    }

    // returns all of the info:<sourceType>:sourceAsset attributes.
    std::vector<UsdAttribute> _GetSourceAssetAttributes() const
    {
        std::vector<UsdAttribute> attrs
            = _shaderNode.GetPrim().GetAuthoredAttributes();
        attrs.erase(
            std::remove_if(
                attrs.begin(), attrs.end(),
                [](const UsdAttribute& attr) {
                    auto nameTokens
                        = SdfPath::TokenizeIdentifierAsTokens(attr.GetName());
                    TfToken leafName;
                    return _ParseInfoName(attr.GetName(), nullptr, &leafName)
                        && leafName != _tokens->sourceAsset;
                }),
            attrs.end());
        return attrs;
    }

    UsdShadeShader _shaderNode;
};

class _UsdImagingDataSourceShadingNode : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_UsdImagingDataSourceShadingNode);

    TfTokenVector GetNames() override
    {
        return HdMaterialNodeSchemaTokens->allTokens;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (name == HdMaterialNodeSchemaTokens->nodeIdentifier) {
            TfToken nodeId;

            // the default identifier
            UsdShadeNodeDefAPI nodeDef(_shaderNode.GetPrim());
            if (nodeDef) {
                nodeDef.GetShaderId(&nodeId);
            } else if (UsdLuxLightFilter lightFilter =
                    UsdLuxLightFilter(_shaderNode.GetPrim())) {
                nodeId = lightFilter.GetShaderId({});

            } else if (UsdLuxLightAPI light =
                    UsdLuxLightAPI(_shaderNode.GetPrim())) {
                nodeId = light.GetShaderId({});
            }

            _shaderNode.GetShaderId(&nodeId);
            return HdRetainedTypedSampledDataSource<TfToken>::New(nodeId);
        }

        if (name == HdMaterialNodeSchemaTokens->renderContextNodeIdentifiers) {
            if (UsdLuxLightAPI light =
                    UsdLuxLightAPI(_shaderNode.GetPrim())) {
                return _UsdImagingDataSourceRenderContextIdentifiers<
                    UsdLuxLightAPI>::New(light);
            } else if (UsdLuxLightFilter lightFilter =
                    UsdLuxLightFilter(_shaderNode.GetPrim())) {
                return _UsdImagingDataSourceRenderContextIdentifiers<
                    UsdLuxLightFilter>::New(lightFilter);
            }
            else {
                if (_shaderNode.GetImplementationSource()
                    == UsdShadeTokens->sourceAsset) {
                    return _UsdImagingRenderContextNodeIdentifiersForSourceAsset::
                        New(_shaderNode);
                }
            }

            return nullptr;
        }

        if (name == HdMaterialNodeSchemaTokens->parameters) {
            return _UsdImagingDataSourceShadingNodeParameters::New(
                _shaderNode, _stageGlobals, _sceneIndexPath,
                    _locatorPrefix.IsEmpty()
                        ? _locatorPrefix
                        : _locatorPrefix
                            .Append(
                                _shaderNode.GetPrim().GetPath().GetToken())
                            .Append(
                                HdMaterialNodeSchemaTokens->parameters)
                            );

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
        const UsdImagingDataSourceStageGlobals &stageGlobals,
        const SdfPath &sceneIndexPath,
        const HdDataSourceLocator &locatorPrefix)
    : _shaderNode(shaderNode)
    , _stageGlobals(stageGlobals)
    , _sceneIndexPath(sceneIndexPath)
    , _locatorPrefix(locatorPrefix)
    {}

    UsdShadeShader _shaderNode;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
    SdfPath _sceneIndexPath;
    HdDataSourceLocator _locatorPrefix;
};


} // namespace anonymous

// ----------------------------------------------------------------------------

UsdImagingDataSourceMaterial::UsdImagingDataSourceMaterial(
    UsdPrim usdPrim,
    const UsdImagingDataSourceStageGlobals & stageGlobals,
    const TfToken &fixedTerminalName)
: _usdPrim(usdPrim)
, _stageGlobals(stageGlobals)
, _fixedTerminalName(fixedTerminalName)
{
}

UsdImagingDataSourceMaterial::~UsdImagingDataSourceMaterial()
{
    WorkMoveDestroyAsync(_networks);
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
    UsdShadeConnectableAPI const &shadeNode,
    _TokenDataSourceMap * const outputNodes,
    const UsdImagingDataSourceStageGlobals &stageGlobals,
    const SdfPath &sceneIndexPath,
    const HdDataSourceLocator &locatorPrefix)
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

    HdDataSourceBaseHandle nodeValue =
        _UsdImagingDataSourceShadingNode::New(
            shadeNode, stageGlobals, sceneIndexPath, locatorPrefix);

    outputNodes->insert({nodeName, nodeValue});

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
                UsdShadeConnectableAPI(attr.GetPrim()),
                outputNodes,
                stageGlobals,
                sceneIndexPath,
                locatorPrefix);
        }
    }

}

static
HdDataSourceBaseHandle
_BuildNetwork(
    UsdShadeConnectableAPI const &terminalNode,
    const TfToken &terminalName,
    UsdImagingDataSourceStageGlobals const &stageGlobals,
    TfToken const& context,
    const SdfPath &sceneIndexPath,
    const HdDataSourceLocator &locatorPrefix)
{

    _TokenDataSourceMap nodeDataSources;
    _WalkGraph(terminalNode,
                &nodeDataSources,
                stageGlobals,
                sceneIndexPath,
                locatorPrefix.IsEmpty()
                    ? locatorPrefix
                    : locatorPrefix.Append(
                        HdMaterialNetworkSchemaTokens->nodes));

    TfTokenVector nodeNames;
    std::vector<HdDataSourceBaseHandle> nodeValues;
    nodeNames.reserve(nodeDataSources.size());
    nodeValues.reserve(nodeDataSources.size());
    for (const auto &tokenDsPair : nodeDataSources) {
        nodeNames.push_back(tokenDsPair.first);
        nodeValues.push_back(tokenDsPair.second);
    }


    HdContainerDataSourceHandle terminalsDs = 
        HdRetainedContainerDataSource::New(
            terminalName,
            HdMaterialConnectionSchema::BuildRetained(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    TfToken(terminalNode.GetPrim().GetPath().GetToken())),
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    terminalName)));

    HdContainerDataSourceHandle nodesDs = 
        HdRetainedContainerDataSource::New(
            nodeNames.size(),
            nodeNames.data(),
            nodeValues.data());

    return HdMaterialNetworkSchema::BuildRetained(
        nodesDs,
        terminalsDs);
}

static 
HdDataSourceBaseHandle
_BuildMaterial(
    UsdShadeNodeGraph const &usdMat, 
    UsdImagingDataSourceStageGlobals const &stageGlobals,
    TfToken const& context,
    const SdfPath &sceneIndexPath,
    const HdDataSourceLocator &locatorPrefix)
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

            UsdShadeConnectableAPI upstreamShader(sourceInfo.source.GetPrim());

            _WalkGraph(upstreamShader,
                &nodeDataSources,
                stageGlobals,
                sceneIndexPath,
                locatorPrefix.IsEmpty()
                    ? locatorPrefix
                    : locatorPrefix.Append(
                        HdMaterialNetworkSchemaTokens->nodes));

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

    HdDataSourceBaseHandle networkDs;

    // sceneIndexPath and dataSourceLocator are sent along so that discovery
    // of time-varying shader parameters are managed for the hydra material
    // prim and not individual USD shader prims.

    if (_fixedTerminalName.IsEmpty()) {
        networkDs = _BuildMaterial(
            UsdShadeNodeGraph(_usdPrim), _stageGlobals, name,
                _usdPrim.GetPath(), HdDataSourceLocator(
                    HdMaterialSchemaTokens->material, name));
    } else {
        networkDs = _BuildNetwork(UsdShadeConnectableAPI(_usdPrim),
            _fixedTerminalName, _stageGlobals, name,
                _usdPrim.GetPath(), HdDataSourceLocator(
                    HdMaterialSchemaTokens->material, name));
    }


    _networks[name] = networkDs;
    return networkDs;
}


PXR_NAMESPACE_CLOSE_SCOPE
