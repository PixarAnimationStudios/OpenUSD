//
// Copyright 2019 Pixar
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

#include "pxr/imaging/hdSt/materialNetwork.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/glf/udimTexture.h"

#include "pxr/imaging/hio/glslfx.h"

#include "pxr/usd/sdr/declare.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/sdr/registry.h"

#include "pxr/usd/sdf/types.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

typedef std::unique_ptr<HioGlslfx> HioGlslfxUniquePtr;

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (opacity)
    (isPtex)
    (st)
    (uv)
    (inputs)
    (fieldname)
);


/// \struct HdStMaterialConnection
///
/// Describes a single connection to an upsream node and output port 
///
/// XXX Replacement for HdRelationship. Unify with HdPrman: MatfiltConnection.
struct HdSt_MaterialConnection {
    SdfPath upstreamNode;
    TfToken upstreamOutputName;
};

/// \struct HdStMaterialNode
///
/// Describes an instance of a node within a network
/// A node contains a (shader) type identifier, parameter values, and 
/// connections to upstream nodes. A single input (mapped by TfToken) may have
/// multiple upstream connections to describe connected array elements.
///
/// XXX Replacement for HdMaterialNode. Unify with HdPrman: MatfiltNode.
struct HdSt_MaterialNode {
    TfToken nodeTypeId;
    std::map<TfToken, VtValue> parameters;
    std::map<TfToken, std::vector<HdSt_MaterialConnection>> inputConnections;
};

/// \struct HdStMaterialNetwork
/// 
/// Container of nodes and top-level terminal connections. This is the mutable
/// representation of a shading network sent to filtering functions by a
/// MatfiltFilterChain.
///
/// XXX Replacement for HdMaterialNetwork. Unify with HdPrman: MatfiltNetwork.
struct HdSt_MaterialNetwork {
    std::map<SdfPath, HdSt_MaterialNode> nodes;
    std::map<TfToken, HdSt_MaterialConnection> terminals;
    TfTokenVector primvars;
};

// XXX We wish to transition to a new material network description and unify
// with HdPrman's MatfiltNetwork. For now we internally convert the deprecated
// HdMaterialNetwork over to the new description so we do not have to redo all
// the Storm code when we swap the classes in Hd to the new description.
//
// Equivilant of: HdPrman's MatfiltConvertFromHdMaterialNetworkMapTerminal.
// With some modifications since HdMaterialNetworkMap now has 'terminals'.
bool
_ConvertLegacyHdMaterialNetwork(
    HdMaterialNetworkMap const& hdNetworkMap,
    TfToken const & terminalName,
    HdSt_MaterialNetwork *result)
{
    auto const& iter = hdNetworkMap.map.find(terminalName);
    if (iter == hdNetworkMap.map.end()) {
        return false;
    }
    const HdMaterialNetwork & hdNetwork = iter->second;

    // Transfer over individual nodes
    for (const HdMaterialNode & node : hdNetwork.nodes) {
        HdSt_MaterialNode & newNode = result->nodes[node.path];
        newNode.nodeTypeId = node.identifier;
        newNode.parameters = node.parameters;

        // Check if this node is a terminal
        auto const& termIt = std::find(
            hdNetworkMap.terminals.begin(), 
            hdNetworkMap.terminals.end(), 
            node.path);

        if (termIt != hdNetworkMap.terminals.end()) {
            result->terminals[terminalName].upstreamNode = node.path;
        }
    }

    // Transfer relationships to inputConnections on receiving/downstream nodes.
    for (HdMaterialRelationship const& rel : hdNetwork.relationships) {
        // outputId (in hdMaterial terms) is the input of the receiving node
        auto const& iter = result->nodes.find(rel.outputId);
        // skip connection if the destination node doesn't exist
        if (iter == result->nodes.end()) {
            continue;
        }
        iter->second.inputConnections[rel.outputName]
            .emplace_back( HdSt_MaterialConnection{rel.inputId, rel.inputName});
    }

    // Transfer primvars:
    result->primvars = hdNetwork.primvars;

    return true;
}

static TfToken
_GetMaterialTag(
    VtDictionary const& metadata,
    HdSt_MaterialNode const& terminal)
{
    // Strongest materialTag opinion is a hardcoded tag in glslfx meta data.
    // This can be used for additive, translucent or volume materials.
    // See HdMaterialTagTokens.
    VtValue vtMetaTag = TfMapLookupByValue(
        metadata,
        HdShaderTokens->materialTag,
        VtValue());

    if (vtMetaTag.IsHolding<std::string>()) {
        return TfToken(vtMetaTag.UncheckedGet<std::string>());
    }

    bool isTranslucent = false;

    // Next strongest opinion is a connection to 'terminal.opacity'
    auto const& opacityConIt = terminal.inputConnections.find(_tokens->opacity);
    isTranslucent = (opacityConIt != terminal.inputConnections.end());

    // Weakest opinion is an authored terminal.opacity value.
    if (!isTranslucent) {
        for (auto const& paramIt : terminal.parameters) {
            if (paramIt.first != _tokens->opacity) continue;

            VtValue const& vtOpacity = paramIt.second;
            isTranslucent = vtOpacity.Get<float>() < 1.0f;
            break;
        }
    }

    if (isTranslucent) {
        // Default to our cheapest blending: unsorted additive
        return HdStMaterialTagTokens->additive;
    }

    // An empty materialTag on the HdRprimCollection level means: 'ignore all
    // materialTags and add everything to the collection'. Instead we return a
    // default token because we want materialTags to drive HdSt collections.
    return HdStMaterialTagTokens->defaultMaterialTag;
}

static void
_GetGlslfxForTerminal(
    HioGlslfxUniquePtr& glslfxOut,
    TfToken const& nodeTypeId)
{
    // 1. info::id was set in usda (token info:id = "UsdPreviewSurface").
    //
    // We have an info:id so we can use Sdr to get to the source code path
    // for glslfx. GetShaderNodeByIdentifierAndType() will insert a SdrNode
    // and we can use GetResolvedSourceURI to query the source code path.
    SdrRegistry &shaderReg = SdrRegistry::GetInstance();
    SdrShaderNodeConstPtr sdrNode = shaderReg.GetShaderNodeByIdentifierAndType(
        nodeTypeId, HioGlslfxTokens->glslfx);

    if (sdrNode) {
        std::string const& glslfxPath = sdrNode->GetResolvedSourceURI();
        glslfxOut.reset(new HioGlslfx(glslfxPath));
        return;
    }

    // 2. info::sourceAsset (asset info:glslfx:sourceAsset = @custm.glslfx@)
    // We did not have info::id so we expect the terminal type id token to
    // have been resolved into the path or source code for the glslfx.
    // E.g. UsdImagingMaterialAdapter handles this for us.
    if (!nodeTypeId.IsEmpty()) {
        // Most likely: the identifier is a path to a glslfx file
        glslfxOut.reset(new HioGlslfx(nodeTypeId));
        if (!glslfxOut->IsValid()) {
            // Less likely: the identifier is a glslfx code snippet
            std::istringstream sourceCodeStream(nodeTypeId);
            glslfxOut.reset(new HioGlslfx(sourceCodeStream));
        }
    }
}

static HdSt_MaterialNode const*
_GetTerminalNode(
    SdfPath const& id,
    HdSt_MaterialNetwork const& network)
{
    if (network.terminals.size() != 1) {
        if (network.terminals.size() > 1) {
            TF_WARN("Unsupported number of terminals [%d] in material [%s]", 
                    (int)network.terminals.size(), id.GetText());
        }
        return nullptr;
    }

    auto const& terminalConnIt = network.terminals.begin();
    HdSt_MaterialConnection const& connection = terminalConnIt->second;
    SdfPath const& terminalPath = connection.upstreamNode;
    auto const& terminalIt = network.nodes.find(terminalPath);
    return &terminalIt->second;
}

static VtValue
_GetParamFallbackValue(
    HdSt_MaterialNetwork const& network,
    HdSt_MaterialNode const& node,
    TfToken const& paramName)
{
    // The 'fallback value' will be the value of the material param if nothing 
    // is connected or what is connected is mis-configured. For example a 
    // missing texture file.

    SdrRegistry& shaderReg = SdrRegistry::GetInstance();

    // Check if there are any connections to the terminal input.
    auto const& connIt = node.inputConnections.find(paramName);

    if (connIt != node.inputConnections.end()) {
        if (!connIt->second.empty()) {
            HdSt_MaterialConnection const& con = connIt->second.front();
            auto const& pnIt = network.nodes.find(con.upstreamNode);
            HdSt_MaterialNode const& upstreamNode = pnIt->second;
        
            // Find the Sdr node that is connected to the terminal input

            SdrShaderNodeConstPtr conSdr = 
                shaderReg.GetShaderNodeByIdentifierAndType(
                    upstreamNode.nodeTypeId,
                    HioGlslfxTokens->glslfx);

            if (conSdr) {
                // XXX Storm hack: Incorrect usage of GetDefaultInput to
                // determine what the fallback value is.
                // GetDefaultInput is meant to be used for 'disabled'
                // node where the 'default input' becomes the value
                // pass-through in the network. But Storm has no other
                // mechanism currently to deal with fallback values.
                if (SdrShaderPropertyConstPtr const& defaultInput = 
                        conSdr->GetDefaultInput()) {
                    TfToken const& def = defaultInput->GetName();
                    auto const& defParamIt = upstreamNode.parameters.find(def);
                    if (defParamIt != upstreamNode.parameters.end()) {
                        return defParamIt->second;
                    }
                }

                // Sdr supports specifying default values for outputs so if we
                // did not use the GetDefaultInput hack above, we fallback to
                // using this DefaultOutput value.
                if (SdrShaderPropertyConstPtr const& output = 
                        conSdr->GetShaderOutput(con.upstreamOutputName)) {
                    VtValue out =  output->GetDefaultValue();
                    if (out.IsEmpty()) {
                        // If no default value was registered with Sdr for
                        // the output, fallback to the type's default.
                        if (out.IsEmpty()) {
                            out = output->GetTypeAsSdfType()
                                .first.GetDefaultValue();
                        }
                    }

                    return out;
                }
            }
        }
    }

    // If there are no connections there may be an authored value.

    auto const& it = node.parameters.find(paramName);
    if (it != node.parameters.end()) {
        return it->second;
    }

    // If we had nothing connected, but we do have an Sdr node we can use the
    // DefaultValue for the input as specified in the Sdr schema.
    // E.g. PreviewSurface is a terminal with an Sdr schema.

    SdrShaderNodeConstPtr terminalSdr = 
        shaderReg.GetShaderNodeByIdentifierAndType(
            node.nodeTypeId,
            HioGlslfxTokens->glslfx);

    if (terminalSdr) {
        if (SdrShaderPropertyConstPtr const& input = 
                terminalSdr->GetShaderInput(paramName)) {
            VtValue out = input->GetDefaultValue();
            // If not default value was registered with Sdr for
            // the output, fallback to the type's default.
            if (out.IsEmpty()) {
                out = input->GetTypeAsSdfType().first.GetDefaultValue();
            }

            if (!out.IsEmpty()) return out;
        }
    }

    // Returning an empty value will likely result in a shader compile error,
    // because the buffer source will not be able to determine the HdTupleType.
    // Hope for the best and return a vec3.

    TF_WARN("Couldn't determine default value for: %s on nodeType: %s", 
            paramName.GetText(), node.nodeTypeId.GetText());

    return VtValue(GfVec3f(0));
}

static TfToken
_GetPrimvarNameAttributeValue(
    SdrShaderNodeConstPtr const& sdrNode,
    HdSt_MaterialNode const& node,
    TfToken const& propName)
{
    VtValue vtName;

    // If the name of the primvar was authored, the material adapter would have
    // put that that authored value in the node's parameter list. 
    // The authored value is the strongest opinion/

    auto const& paramIt = node.parameters.find(propName);
    if (paramIt != node.parameters.end()) {
        vtName = paramIt->second;
    }

    // If we didn't find an authored value consult Sdr for the default value.
    if (vtName.IsEmpty() && sdrNode) {
        if (SdrShaderPropertyConstPtr sdrPrimvarInput = 
                sdrNode->GetShaderInput(propName)) {
            vtName = sdrPrimvarInput->GetDefaultValue();
        }
    }

    if (vtName.IsHolding<TfToken>()) {
        return vtName.UncheckedGet<TfToken>();
    } else if (vtName.IsHolding<std::string>()) {
        return TfToken(vtName.UncheckedGet<std::string>());
    }

    return TfToken();
}

static HdMaterialParamVector
_MakeMaterialParamsForUnconnectedParam(
    HdSt_MaterialNode const& node,
    TfToken const& paramName)
{
    HdMaterialParamVector params;
    HdMaterialParam param;
    param.paramType = HdMaterialParam::ParamTypeFallback;
    param.name = paramName;
    param.connection = SdfPath();          /*No connection*/
    param.samplerCoords = TfTokenVector(); /*No UV*/
    param.textureType = HdTextureType::Uv  /*No Texture*/;

    params.emplace_back(std::move(param));
    return params;
}

static HdMaterialParamVector
_MakeMaterialParamsForPrimvarInput(
    HdSt_MaterialNetwork const& network,
    HdSt_MaterialNode const& node,
    SdfPath const& nodePath,
    TfToken const& paramName,
    SdfPathSet* visitedNodes)
{
    HdMaterialParamVector params;
    if (visitedNodes->find(nodePath) != visitedNodes->end()) return params;

    SdrRegistry& shaderReg = SdrRegistry::GetInstance();
    SdrShaderNodeConstPtr sdrNode = shaderReg.GetShaderNodeByIdentifierAndType(
        node.nodeTypeId, HioGlslfxTokens->glslfx);

    HdMaterialParam param;
    param.paramType = HdMaterialParam::ParamTypePrimvar;
    param.name = paramName;
    param.connection = SdfPath("primvar." + nodePath.GetName());
    param.textureType = HdTextureType::Uv  /*No Texture*/;

    // A node may require 'additional primvars' to function correctly.

    TfTokenVector varNames;
    for (auto const& propName: sdrNode->GetAdditionalPrimvarProperties()) {
        TfToken primvarName = 
            _GetPrimvarNameAttributeValue(sdrNode, node, propName);

        if (!primvarName.IsEmpty()) {
            param.samplerCoords.push_back(primvarName);
        }
    }

    params.emplace_back(std::move(param));
    return params;
}

static std::string
_ResolveAssetPath(VtValue const& value)
{
    // Note that the SdfAssetPath should really be resolved into an ArAsset via
    // ArGetResolver (Eg. USDZ). Using GetResolvePath directly isn't sufficient.
    // Texture loading in Storm goes via Glf, which will handle the ArAsset
    // resolution already, so we skip doing it here and simply use the string.
    if (value.IsHolding<SdfAssetPath>()) {
        SdfAssetPath p = value.Get<SdfAssetPath>();
        std::string v = p.GetResolvedPath();
        if (v.empty()) {
            v = p.GetAssetPath();
        }
        return v;
    } else if (value.IsHolding<std::string>()) {
        return value.UncheckedGet<std::string>();
    }

    return std::string();
}

static HdMaterialParamVector
_MakeMaterialParamsForTextureInput(
    HdSt_MaterialNetwork const& network,
    HdSt_MaterialNode const& node,
    SdfPath const& nodePath,
    TfToken const& paramName,
    SdfPathSet* visitedNodes)
{
    HdMaterialParamVector params;
    if (visitedNodes->find(nodePath) != visitedNodes->end()) return params;

    SdrRegistry& shaderReg = SdrRegistry::GetInstance();
    SdrShaderNodeConstPtr sdrNode = shaderReg.GetShaderNodeByIdentifierAndType(
        node.nodeTypeId, HioGlslfxTokens->glslfx);

    HdMaterialParam texParam;
    texParam.paramType = HdMaterialParam::ParamTypeTexture;
    texParam.name = paramName;

    // Extract texture file path
    std::string filePath;
    SdfPath textureId;

    NdrTokenVec const& assetIdentifierPropertyNames = 
        sdrNode->GetAssetIdentifierInputNames();

    if (assetIdentifierPropertyNames.size() == 1) {
        TfToken const& fileProp = assetIdentifierPropertyNames[0];
        auto const& it = node.parameters.find(fileProp);
        if (it != node.parameters.end()){
            filePath = _ResolveAssetPath(it->second);
            // Our texture loading goes via sceneDelegate->GetTextureResource() 
            // which will try to find a texture prim by by the textureId path.
            // There is a difference in path between UsdShader attribute paths 
            // and the names/paths given to us by HdMaterialNode.parameters.
            // UsdShade inputs have 'inputs:' identifier where node.parameters
            // do not.
            //
            //  Hd:
            //    /Materials/HwUvTexture_1/Shader/Clamp/Tex.file
            //  UdShade:
            //    /Materials/HwUvTexture_1/Shader/Clamp/Tex.inputs:file
            //
            SdfPath UsdShadeInputPath = nodePath.AppendProperty(
                TfToken(SdfPath::JoinIdentifier(
                    _tokens->inputs, fileProp)));

            textureId = SdfPath(UsdShadeInputPath);
        }
    } else {
        TF_WARN("Invalid number of asset identifier input names: %s", 
                nodePath.GetText());
    }

    texParam.connection = textureId;

    // Determine the texture type
    HdTextureType textureType = HdTextureType::Uv;
    if (sdrNode && sdrNode->GetMetadata().count(_tokens->isPtex)) {
        textureType = HdTextureType::Ptex;
    } else if (GlfIsSupportedUdimTexture(filePath)) {
        textureType = HdTextureType::Udim;
    }
    texParam.textureType = textureType;

    // Check to see if a primvar node is connected to 'st' or 'uv'.
    // Instead of looking for a st inputs by name we could traverse all
    // connections to inputs and pick one that has a 'primvar' node attached.
    // That could also be problematic if you connect a primvar to one of the
    // other inputs of the texture node.
    auto stIt = node.inputConnections.find(_tokens->st);
    if (stIt == node.inputConnections.end()) {
        stIt = node.inputConnections.find(_tokens->uv);
    }

    if (stIt != node.inputConnections.end()) {
        if (!stIt->second.empty()) {
            HdSt_MaterialConnection const& con = stIt->second.front();
            SdfPath const& primvarNodePath = con.upstreamNode;
            
            auto const& pnIt = network.nodes.find(primvarNodePath);
            HdSt_MaterialNode const& primvarNode = pnIt->second;

            HdMaterialParamVector primvarParams = 
                _MakeMaterialParamsForPrimvarInput(
                    network,
                    primvarNode,
                    primvarNodePath,
                    stIt->first,
                    visitedNodes);

            if (!primvarParams.empty()) {
                HdMaterialParam const& primvarParam = primvarParams.front();
                // We do not put the primvar connected to the texture into the
                // material params. We only wanted to extract the primvar name
                // and put it into the texture's samplerCoords.
                //    params.emplace_back(std::move(primvarParam));
                texParam.samplerCoords = primvarParam.samplerCoords;
            }
        }
    }

    params.emplace_back(std::move(texParam));
    return params;
}

static HdMaterialParamVector
_MakeMaterialParamsForFieldInput(
    HdSt_MaterialNetwork const& network,
    HdSt_MaterialNode const& node,
    SdfPath const& nodePath,
    TfToken const& paramName,
    SdfPathSet* visitedNodes)
{
    HdMaterialParamVector params;
    if (visitedNodes->find(nodePath) != visitedNodes->end()) return params;

    // Volume Fields act more like a primvar then a texture.
    // There is a `Volume` prim with 'fields' that may point to a
    // OpenVDB file. We have to find the 'inputs:fieldname' on the
    // HWFieldReader in the material network to know what 'field' to use.
    // See also HdStVolume and HdStField for how volume textures are
    // inserted into Storm.

    HdMaterialParam param;
    param.paramType = HdMaterialParam::ParamTypeField;
    param.name = paramName;
    param.connection = nodePath;
    param.textureType = HdTextureType::Uvw;

    // XXX Why _tokens->fieldname:
    // Hard-coding the name of the attribute of HwFieldReader identifying
    // the field name for now.
    // The equivalent of the generic mechanism Sdr provides for primvars
    // is missing for fields: UsdPrimvarReader.inputs:varname is tagged with
    // sdrMetadata as primvarProperty="1" so that we can use
    // sdrNode->GetAdditionalPrimvarProperties to know what attribute to use.
    TfToken const& varName = _tokens->fieldname;

    auto const& it = node.parameters.find(varName);
    if (it != node.parameters.end()){
        VtValue fieldName = it->second;
        if (fieldName.IsHolding<TfToken>()) {
            // Stashing name of field in _samplerCoords.
            param.samplerCoords.push_back(
                fieldName.UncheckedGet<TfToken>());
        }
    }

    params.emplace_back(std::move(param));
    return params;
}

static HdMaterialParamVector
_MakeParamsForInputParameter(
    HdSt_MaterialNetwork const& network,
    HdSt_MaterialNode const& node,
    TfToken const& paramName,
    SdfPathSet* visitedNodes)
{
    SdrRegistry& shaderReg = SdrRegistry::GetInstance();

    // Resolve what is connected to this param (eg. primvar, texture, nothing)
    // and then make the correct HdMaterialParam for it.
    auto const& conIt = node.inputConnections.find(paramName);

    if (conIt != node.inputConnections.end()) {

        std::vector<HdSt_MaterialConnection> const& cons = conIt->second;
        if (!cons.empty()) {

            // Find the node that is connected to this input
            HdSt_MaterialConnection const& con = cons.front();
            auto const& upIt = network.nodes.find(con.upstreamNode);

            if (upIt != network.nodes.end()) {

                SdfPath const& upstreamPath = upIt->first;
                HdSt_MaterialNode const& upstreamNode = upIt->second;

                SdrShaderNodeConstPtr upstreamSdr = 
                    shaderReg.GetShaderNodeByIdentifierAndType(
                        upstreamNode.nodeTypeId,
                        HioGlslfxTokens->glslfx);

                if (upstreamSdr) {
                    TfToken sdrRole(upstreamSdr->GetRole());
                    if (sdrRole == SdrNodeRole->Texture) {

                        return _MakeMaterialParamsForTextureInput(
                            network,
                            upstreamNode,
                            upstreamPath,
                            paramName,
                            visitedNodes);

                    } else if (sdrRole == SdrNodeRole->Primvar) {

                        return _MakeMaterialParamsForPrimvarInput(
                            network,
                            upstreamNode,
                            upstreamPath,
                            paramName,
                            visitedNodes);

                    } else if (sdrRole == SdrNodeRole->Field) {

                        return _MakeMaterialParamsForFieldInput(
                            network,
                            upstreamNode,
                            upstreamPath,
                            paramName,
                            visitedNodes);

                    }
                }
            }
        }
    } 

    // Nothing (supported) was connected, output a fallback material param    
    return _MakeMaterialParamsForUnconnectedParam(node, paramName);
}

static HdMaterialParamVector 
_GatherMaterialParams(
    HdSt_MaterialNetwork const& network,
    HdSt_MaterialNode const& node,
    HioGlslfxUniquePtr const& glslfx) 
{
    // Hydra Storm currently supports two material configurations.
    // A custom glslfx file or a PreviewSurface material network.
    // Either configuration consists of a terminal (Shader or PreviewSurface)
    // with its input values authored or connected to a primvar, texture or
    // volume node. The texture may have a primvar connected to provide UVs.
    //
    // The following code is made to process one of these two material configs
    // exclusively. It cannot convert arbitrary material networks to Storm by
    // generating the appropriate glsl code.

    HdMaterialParamVector params;

    SdrRegistry &shaderReg = SdrRegistry::GetInstance();
    SdrShaderNodeConstPtr sdrNode = shaderReg.GetShaderNodeByIdentifierAndType(
        node.nodeTypeId, HioGlslfxTokens->glslfx);

    SdfPathSet visitedNodes;

    TfTokenVector parameters;
    if (sdrNode) {
        parameters = sdrNode->GetInputNames();
    } else {
        // For custom glslfx, that have no schema / sdr node so we cannot
        // ask for GetInputNames. As alternative we build the list of params
        // XXX If a shader parameter is not authored in the usda it will not
        // appear in the node.parameters list and we won't be able to make
        // HdMaterialParam out of it. And the shader will probably fail to 
        // compile.
        parameters.reserve(node.parameters.size());
        for (auto const& it : node.parameters) {
            parameters.push_back(it.first);
        }
        for (auto const& it : node.inputConnections) {
            parameters.push_back(it.first);
        }

        // Ensure each parameters is only once in the parameters vector.
        // Some parameters may have been authored AND have a connection.
        std::sort(parameters.begin(), parameters.end());
        parameters.erase(std::unique(parameters.begin(), parameters.end()),
                         parameters.end());
    }

    for (TfToken const& inputName : parameters) {
        HdMaterialParamVector inputParams = _MakeParamsForInputParameter(
            network, node, inputName, &visitedNodes);
        params.insert(params.end(), inputParams.begin(), inputParams.end());
    }

    // Set fallback values for the inputs on the terminal
    for (HdMaterialParam& p : params) {
        p.fallbackValue= _GetParamFallbackValue(network, node, p.name);
    }

    return params;
}

HdStMaterialNetwork::HdStMaterialNetwork()
{
}

HdStMaterialNetwork::~HdStMaterialNetwork()
{

}

void
HdStMaterialNetwork::ProcessMaterialNetwork(
    SdfPath const& materialId,
    HdMaterialNetworkMap const& hdNetworkMap)
{
    HdSt_MaterialNetwork surfaceNetwork;
    HdSt_MaterialNetwork displacementNetwork;

    // The fragment source comes from the 'surface' network or the
    // 'volume' network.
    _ConvertLegacyHdMaterialNetwork(
        hdNetworkMap,
        HdMaterialTerminalTokens->surface,
        &surfaceNetwork);

    bool isVolume = surfaceNetwork.terminals.empty();
    if (isVolume) {
        _ConvertLegacyHdMaterialNetwork(
            hdNetworkMap,
            HdMaterialTerminalTokens->volume,
            &surfaceNetwork);
    }

    // Geometry source can be provided via a 'displacement' network.
    _ConvertLegacyHdMaterialNetwork(
        hdNetworkMap,
        HdMaterialTerminalTokens->displacement,
        &displacementNetwork);

    if (HdSt_MaterialNode const* surfTerminal = 
            _GetTerminalNode(materialId, surfaceNetwork)) 
    {
        // Extract the glslfx and metadata for surface/volume.
        HioGlslfxUniquePtr surfaceGfx;
        _GetGlslfxForTerminal(surfaceGfx, surfTerminal->nodeTypeId);
        if (surfaceGfx) {

            // If the glslfx file is not valid we skip parsing the network.
            // This produces no fragmentSource which means Storm's material
            // will use the fallback shader.

            if (surfaceGfx->IsValid()) {
                _fragmentSource = isVolume ? surfaceGfx->GetVolumeSource() : 
                    surfaceGfx->GetSurfaceSource();
                _materialMetadata = surfaceGfx->GetMetadata();
                _materialTag= _GetMaterialTag(_materialMetadata, *surfTerminal);
                _materialParams = _GatherMaterialParams(
                    surfaceNetwork, *surfTerminal, surfaceGfx);
            }
        }
    }

    if (HdSt_MaterialNode const* dispTerminal = 
            _GetTerminalNode(materialId, displacementNetwork)) 
    {
        // Extract the glslfx for displacement.
        HioGlslfxUniquePtr displacementGfx;
        _GetGlslfxForTerminal(displacementGfx, dispTerminal->nodeTypeId);
        if (displacementGfx && displacementGfx->IsValid()) {
            _geometrySource = displacementGfx->GetDisplacementSource();
        }
    }
}

TfToken const&
HdStMaterialNetwork::GetMaterialTag() const
{
    return _materialTag;
}

std::string const& 
HdStMaterialNetwork::GetFragmentCode() const
{
    return _fragmentSource;
}

std::string const&
HdStMaterialNetwork::GetGeometryCode() const
{
    return _geometrySource;
}

VtDictionary const&
HdStMaterialNetwork::GetMetadata() const
{
    return _materialMetadata;
}

HdMaterialParamVector const&
HdStMaterialNetwork::GetMaterialParams() const
{
    return _materialParams;
}

PXR_NAMESPACE_CLOSE_SCOPE

