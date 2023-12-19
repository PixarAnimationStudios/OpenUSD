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
#include "pxr/usdImaging/usdImaging/dataSourceAttributeColorSpace.h"

#include "pxr/usd/usdLux/lightAPI.h"
#include "pxr/usd/usdLux/lightFilter.h"

#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"
#include "pxr/usd/usdShade/nodeDefAPI.h"

#include "pxr/imaging/hd/lazyContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hd/materialConnectionSchema.h"
#include "pxr/imaging/hd/materialNetworkSchema.h"
#include "pxr/imaging/hd/materialNodeSchema.h"
#include "pxr/imaging/hd/materialNodeParameterSchema.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/materialInterfaceMappingSchema.h"

#include "pxr/base/work/utils.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

// Strip </prefix> from </prefix/path> to yield <path>
SdfPath
_RelativePath(const SdfPath &prefix, const SdfPath &path)
{
    return prefix.IsEmpty() ? path
        : path.ReplacePrefix(prefix, SdfPath::ReflexiveRelativePath());
}

// Extract the render context from an output name, ex:
// "outputs:surface" -> ""
// "outputs:ri:surface" -> "ri"
TfToken
_GetRenderContextForShaderOutput(UsdShadeOutput const& output)
{
    TfToken ns = output.GetAttr().GetNamespace();
    if (TfStringStartsWith(ns, UsdShadeTokens->outputs)) {
        return TfToken(ns.GetString().substr(UsdShadeTokens->outputs.size()));
    }
    // Empty namespace, e.g. "outputs:foo" -> ""
    return TfToken();
}

bool
_Contains(const TfTokenVector &v, const TfToken &t)
{
    return std::find(v.begin(), v.end(), t) != v.end();
}

class _UsdImagingDataSourceInterfaceMappings : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_UsdImagingDataSourceInterfaceMappings);

    TfTokenVector GetNames() override
    {
        TfTokenVector result;
        result.reserve(_consumerMap.size());

        for (const auto &nameConsumersPair : _consumerMap) {

            result.push_back(nameConsumersPair.first.GetBaseName());
        }
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        const auto it = _consumerMap.find(_material.GetInput(name));
        if (it == _consumerMap.end()) {
            return nullptr;
        }

        const std::vector<UsdShadeInput> &consumers = it->second;
        if (consumers.empty()) {
            return nullptr;
        }

        TfSmallVector<HdDataSourceBaseHandle, 2> consumerContainers;
        consumerContainers.reserve(consumers.size());

        for (const UsdShadeInput &input : consumers) {
            consumerContainers.push_back(
                HdMaterialInterfaceMappingSchema::Builder()
                    .SetNodePath(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            _RelativePath(_material.GetPrim().GetPath(),
                                input.GetPrim().GetPath()).GetToken()))
                    .SetInputName(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            input.GetBaseName()))
                    .Build()
            );
        }

        return HdRetainedSmallVectorDataSource::New(
            consumerContainers.size(), consumerContainers.data());
    }

private:

    _UsdImagingDataSourceInterfaceMappings(const UsdShadeMaterial &material)
    : _material(material)
    {
        _consumerMap = _material.ComputeInterfaceInputConsumersMap(true);
    }

    UsdShadeMaterial _material;
    UsdShadeNodeGraph::InterfaceInputConsumersMap _consumerMap;
};

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
            for (UsdAttribute const& attr:
                 input.GetValueProducingAttributes()) {
                UsdShadeAttributeType attrType =
                    UsdShadeUtils::GetType(attr.GetName());
                if (attrType == UsdShadeAttributeType::Input) {
                    // Found at least one
                    result.push_back(input.GetBaseName());
                    // Proceed to next input
                    break;
                }
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

        for (UsdAttribute const& attr:
             input.GetValueProducingAttributes()) {
            UsdShadeAttributeType attrType =
                UsdShadeUtils::GetType(attr.GetName());
            if (attrType == UsdShadeAttributeType::Input) {
                const HdDataSourceLocator paramValueLocator(
                    name, HdMaterialNodeParameterSchemaTokens->value);
                return HdMaterialNodeParameterSchema::Builder()
                    .SetValue(
                        UsdImagingDataSourceAttributeNew(attr, _stageGlobals,
                            _sceneIndexPath,
                            _locatorPrefix.Append(paramValueLocator)))
                    .SetColorSpace(
                        UsdImagingDataSourceAttributeColorSpace::New(attr))
                    .Build();
            }
        }

        // fallback case for requested but unauthored inputs on lights or
        // light filters -- which will not return a value for
        // GetValueProducingAttributes() but can still provide an attr
        if (_shaderNode.GetPrim().HasAPI<UsdLuxLightAPI>() ||
            _shaderNode.GetPrim().IsA<UsdLuxLightFilter>()) {
            const HdDataSourceLocator paramValueLocator(
                name, HdMaterialNodeParameterSchemaTokens->value);
            return HdMaterialNodeParameterSchema::Builder()
                .SetValue(
                    UsdImagingDataSourceAttributeNew(input, _stageGlobals,
                        _sceneIndexPath,
                        _locatorPrefix.Append(paramValueLocator)))
                .Build();
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
            for (UsdAttribute const& attr:
                 input.GetValueProducingAttributes()) {
                UsdShadeAttributeType attrType =
                    UsdShadeUtils::GetType(attr.GetName());
                if (attrType == UsdShadeAttributeType::Output) {
                    // Found at least one connection on this input
                    result.push_back(input.GetBaseName());
                    // Proceed to next input
                    break;
                }
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
            const TfToken outputPath(
                _RelativePath(_materialPrefix, attr.GetPrim().GetPath())
                .GetToken());
            const TfToken outputName = UsdShadeOutput(attr).GetBaseName();

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
        const UsdImagingDataSourceStageGlobals &stageGlobals,
        const SdfPath &materialPrefix)
    : _shaderNode(shaderNode)
    , _stageGlobals(stageGlobals)
    , _materialPrefix(materialPrefix)
    {}

    UsdShadeShader _shaderNode;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
    const SdfPath _materialPrefix;
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
        for (const TfToken &propNameToken : _t.GetPrim().GetPropertyNames()) {
            const std::string &propName = propNameToken.GetString();
            static const std::string suffix =
                ":" + UsdLuxTokens->lightShaderId.GetString();
            if (TfStringEndsWith(propName, suffix)) {
                result.push_back(
                    TfToken(propName.substr(0, propName.size() - suffix.size())));
            }
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

    T _t;
};

// Populate the "nodeTypeInfo" of a node using the "info:" attributes and
// the meta data (skipping info:id).
class _UsdImagingNodeTypeInfoSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_UsdImagingNodeTypeInfoSource);

    TfTokenVector GetNames() override
    {
        TfTokenVector names;
        names.push_back(UsdShadeTokens->sdrMetadata);
        // Missing metadata:
        // subIdentifier
        for (const TfToken &propNameToken :
                 _shaderNode.GetPrim().GetPropertyNames()) {
            const std::string &propName = propNameToken.GetString();
            if (propNameToken != UsdShadeTokens->infoId &&
                TfStringStartsWith(propName, _GetPrefix())) {
                names.push_back(TfToken(propName.substr(_GetPrefix().size())));
            }
        }
        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken& name) override
    {
        if (name == UsdShadeTokens->sdrMetadata) {
            VtDictionary metadata;
            _shaderNode.GetPrim().GetMetadata(name, &metadata);
            return HdRetainedTypedSampledDataSource<VtDictionary>::New(
                metadata);
        }

        const TfToken attrName(_GetPrefix() + name.GetString());

        if (UsdAttribute attr = _shaderNode.GetPrim().GetAttribute(attrName)) {
            return UsdImagingDataSourceAttributeNew(attr, _stageGlobals);
        }

        return nullptr;
    }

private:
    _UsdImagingNodeTypeInfoSource(
        const UsdShadeShader& shaderNode,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
      : _shaderNode(shaderNode)
      , _stageGlobals(stageGlobals)
    {
    }

    static const std::string &_GetPrefix() {
        static const std::string prefix = "info:";
        return prefix;
    }

    UsdShadeShader _shaderNode;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
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
            }
            if (UsdLuxLightFilter lightFilter =
                    UsdLuxLightFilter(_shaderNode.GetPrim())) {
                return _UsdImagingDataSourceRenderContextIdentifiers<
                    UsdLuxLightFilter>::New(lightFilter);
            }
            return nullptr;
            
        }
        if (name == HdMaterialNodeSchemaTokens->nodeTypeInfo) {
            if (_shaderNode.GetImplementationSource() != UsdShadeTokens->id) {
                return _UsdImagingNodeTypeInfoSource::New(
                    _shaderNode, _stageGlobals);
            }
            return nullptr;
        }

        if (name == HdMaterialNodeSchemaTokens->parameters) {
            return _UsdImagingDataSourceShadingNodeParameters::New(
                _shaderNode, _stageGlobals, _sceneIndexPath,
                    _locatorPrefix.IsEmpty()
                        ? _locatorPrefix
                        : _locatorPrefix
                            .Append(_shaderNode.GetPrim().GetPath().GetToken())
                            .Append(HdMaterialNodeSchemaTokens->parameters)
                            );

        }

        if (name == HdMaterialNodeSchemaTokens->inputConnections) {
            return _UsdImagingDataSourceShadingNodeInputs::New(
                _shaderNode, _stageGlobals, _materialPrefix);
        }

        return nullptr;
    }

private:
    _UsdImagingDataSourceShadingNode(
        UsdShadeShader shaderNode,
        const UsdImagingDataSourceStageGlobals &stageGlobals,
        const SdfPath &sceneIndexPath,
        const HdDataSourceLocator &locatorPrefix,
        const SdfPath &materialPrefix)
    : _shaderNode(shaderNode)
    , _stageGlobals(stageGlobals)
    , _sceneIndexPath(sceneIndexPath)
    , _locatorPrefix(locatorPrefix)
    , _materialPrefix(materialPrefix)
    {}

    UsdShadeShader _shaderNode;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
    SdfPath _sceneIndexPath;
    HdDataSourceLocator _locatorPrefix;
    const SdfPath _materialPrefix;
};


} // namespace anonymous

// ----------------------------------------------------------------------------

UsdImagingDataSourceMaterial::UsdImagingDataSourceMaterial(
    const UsdPrim &usdPrim,
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
    if (!_fixedTerminalName.IsEmpty()) {
        return { HdMaterialSchemaTokens->universalRenderContext };
    }

    TfTokenVector renderContexts;
    for (const UsdShadeOutput &output :
             UsdShadeNodeGraph(_usdPrim).GetOutputs()) {
        const TfToken renderContext = _GetRenderContextForShaderOutput(output);
        // Only add a renderContext if it has not been added before so
        // we do not have duplicates (there may be multiple outputs for
        // the same context).
        if (!_Contains(renderContexts, renderContext)) {
            renderContexts.push_back(renderContext);
        }
    }
    return renderContexts;
}

using _TokenDataSourceMap =
    TfDenseHashMap<TfToken, HdDataSourceBaseHandle, TfHash>;

static void
_WalkGraph(
    UsdShadeConnectableAPI const &shadeNode,
    _TokenDataSourceMap * const outputNodes,
    const UsdImagingDataSourceStageGlobals &stageGlobals,
    const SdfPath &sceneIndexPath,
    const HdDataSourceLocator &locatorPrefix,
    const SdfPath &materialPrefix = SdfPath())
{
    if (!shadeNode) {
        return;
    }

    // Check if path is incorrect
    SdfPath const nodePath = shadeNode.GetPath();
    if (nodePath == SdfPath::EmptyPath()) {
        return;
    }

    TfToken const nodeName = _RelativePath(materialPrefix, nodePath).GetToken();
    if (outputNodes->find(nodeName) != outputNodes->end()) {
        return;
    }

    HdDataSourceBaseHandle nodeValue =
        _UsdImagingDataSourceShadingNode::New(
            shadeNode, stageGlobals, sceneIndexPath,
            locatorPrefix, materialPrefix);

    outputNodes->insert({nodeName, nodeValue});

    // Visit inputs of this node to ensure they are emitted first.
    for (UsdShadeInput const & input: shadeNode.GetInputs()) {
        for (const UsdAttribute &attr :
             input.GetValueProducingAttributes(/*shaderOutputsOnly =*/ true)) {
            _WalkGraph(
                UsdShadeConnectableAPI(attr.GetPrim()),
                outputNodes,
                stageGlobals,
                sceneIndexPath,
                locatorPrefix,
                materialPrefix);
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
                    terminalNode.GetPrim().GetPath().GetToken()),
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    terminalName)));

    HdContainerDataSourceHandle nodesDs = 
        HdRetainedContainerDataSource::New(
            nodeNames.size(),
            nodeNames.data(),
            nodeValues.data());

    // for capture in the HdLazyContainerDataSource
    UsdShadeMaterial material(terminalNode.GetPrim());

    return HdMaterialNetworkSchema::Builder()
            .SetNodes(nodesDs)
            .SetTerminals(terminalsDs)
            .SetInterfaceMappings(
                HdLazyContainerDataSource::New([material](){
                    return _UsdImagingDataSourceInterfaceMappings
                        ::New(material);
                }))
            .Build();

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
    TRACE_FUNCTION();

    TfTokenVector terminalsNames;
    std::vector<HdDataSourceBaseHandle> terminalsValues;
    TfTokenVector nodeNames;
    std::vector<HdDataSourceBaseHandle> nodeValues;

    // Strip the material path prefix from all node names.
    // This makes the network more concise to read, as well
    // as enables the potential to detect duplication as
    // the same network appears under different scene models.
    const SdfPath materialPrefix = usdMat.GetPrim().GetPath();

    _TokenDataSourceMap nodeDataSources;

    for (UsdShadeOutput &output : usdMat.GetOutputs()) {
        // Skip terminals from other contexts.
        if (_GetRenderContextForShaderOutput(output) != context) {
            continue;
        }

        // E.g. "ri:surface"
        TfToken outputName = output.GetBaseName();

        // Strip the context, if there is one.
        if (!context.IsEmpty()) {
            // Skip the context and subsequent ':'
            outputName = TfToken(
                outputName.GetString().substr(context.size()+1));
        }

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
                        HdMaterialNetworkSchemaTokens->nodes),
                materialPrefix);

            terminalsNames.push_back(outputName);

            // Strip materialPrefix.
            SdfPath upstreamPath =
                _RelativePath(materialPrefix, upstreamShader.GetPath());

            terminalsValues.push_back(HdMaterialConnectionSchema::BuildRetained(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    upstreamPath.GetToken()),
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


    return HdMaterialNetworkSchema::Builder()
        .SetNodes(nodesDs)
        .SetTerminals(terminalsDs)
        .SetInterfaceMappings(_UsdImagingDataSourceInterfaceMappings::New(
            UsdShadeMaterial(usdMat.GetPrim())))
        .Build();
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
            _usdPrim.GetPath(),
            HdMaterialSchema::GetDefaultLocator().Append(name));
    } else {
        networkDs = _BuildNetwork(
            UsdShadeConnectableAPI(_usdPrim),
            _fixedTerminalName, _stageGlobals, name,
            _usdPrim.GetPath(),
            HdMaterialSchema::GetDefaultLocator().Append(name));
    }


    _networks[name] = networkDs;
    return networkDs;
}

UsdImagingDataSourceMaterialPrim::UsdImagingDataSourceMaterialPrim(
    const SdfPath &sceneIndexPath,
    const UsdPrim &usdPrim,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
 : UsdImagingDataSourcePrim(sceneIndexPath, usdPrim, stageGlobals)
{
}

UsdImagingDataSourceMaterialPrim::~UsdImagingDataSourceMaterialPrim() = default;

TfTokenVector
UsdImagingDataSourceMaterialPrim::GetNames()
{
    TfTokenVector result = UsdImagingDataSourcePrim::GetNames();
    result.push_back(HdMaterialSchema::GetSchemaToken());
    return result;
}

HdDataSourceBaseHandle
UsdImagingDataSourceMaterialPrim::Get(const TfToken &name)
{
    if (name == HdMaterialSchema::GetSchemaToken()) {
        return UsdImagingDataSourceMaterial::New(
            _GetUsdPrim(),
            _GetStageGlobals());
    }
    return UsdImagingDataSourcePrim::Get(name);
}

HdDataSourceLocatorSet
UsdImagingDataSourceMaterialPrim::Invalidate(
    UsdPrim const& prim,
    const TfToken &subprim,
    const TfTokenVector &properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    HdDataSourceLocatorSet result =
        UsdImagingDataSourcePrim::Invalidate(
            prim, subprim, properties, invalidationType);

    if (subprim.IsEmpty()) {
        UsdShadeMaterial material(prim);
        if (material) {
            // Public interface values changes
            for (const TfToken &propertyName : properties) {
                if (UsdShadeInput::IsInterfaceInputName(
                        propertyName.GetString())) {
                    // TODO, invalidate specifically connected node parameters.
                    // FOR NOW: just dirty the whole material.

                    result.insert(HdMaterialSchema::GetDefaultLocator());
                    break;
                }
            }
        }
    }
    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
