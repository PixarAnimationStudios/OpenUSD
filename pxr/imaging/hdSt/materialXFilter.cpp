//
// Copyright 2020 Pixar
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
#include "pxr/imaging/hdSt/materialParam.h"
#include "pxr/imaging/hdSt/materialXFilter.h"
#include "pxr/imaging/hdSt/materialXShaderGen.h"
#include "pxr/imaging/hdMtlx/hdMtlx.h"

#include "pxr/usd/sdr/registry.h"
#include "pxr/imaging/hio/glslfx.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/diagnostic.h"

#include <MaterialXGenShader/Util.h>
#include <MaterialXRender/Util.h>
#include <MaterialXRender/LightHandler.h> 

namespace mx = MaterialX;

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (mtlx)

    // Default Texture Coordinate Token
    (st)
    (texcoord)
    (geomprop)
    (index)

    // Opacity Parameters
    (opacity)
    (opacityThreshold)
    (transmission)
);


////////////////////////////////////////////////////////////////////////////////
// Shader Gen Functions

// Generate the Glsl Pixel Shader based on the given mxContext and mxElement
// Based on MaterialXViewer Material::generateShader()
static mx::ShaderPtr
_GenMaterialXShader(mx::GenContext & mxContext, mx::ElementPtr const& mxElem)
{
    bool hasTransparency = mx::isTransparentSurface(mxElem,
                               mxContext.getShaderGenerator().getTarget());

    mx::GenContext materialContext = mxContext;
    materialContext.getOptions().hwTransparency = hasTransparency;
    materialContext.getOptions().hwShadowMap = 
        materialContext.getOptions().hwShadowMap && !hasTransparency;
    
    // Use the domeLightPrefilter texture instead of sampling the Environment Map
    materialContext.getOptions().hwSpecularEnvironmentMethod =
        mx::HwSpecularEnvironmentMethod::SPECULAR_ENVIRONMENT_PREFILTER;

    return mx::createShader("Shader", materialContext, mxElem);
}

// Results in lightData.type = 1 for point lights in the Mx Shader
static const std::string mxDirectLightString = 
R"(
<?xml version="1.0"?>
<materialx version="1.37">
  <point_light name="pt_light" type="lightshader">
  </point_light>
</materialx>
)";

// Use the given mxDocument to generate the corresponding glsl shader
// Based on MaterialXViewer Viewer::loadDocument()
mx::ShaderPtr
HdSt_GenMaterialXShader(
    mx::DocumentPtr const& mxDoc,
    mx::FileSearchPath const& searchPath,
    MxHdInfo const& mxHdInfo)
{
    // Initialize the Context for shaderGen. 
    mx::GenContext mxContext = HdStMaterialXShaderGen::create(mxHdInfo);

#if MATERIALX_MAJOR_VERSION == 1 && MATERIALX_MINOR_VERSION == 38 && MATERIALX_BUILD_VERSION == 3
    mxContext.registerSourceCodeSearchPath(searchPath);
#else
    // Starting from MaterialX 1.38.4 at PR 877, we must remove the "libraries" part:
    mx::FileSearchPath libSearchPaths;
    for (const mx::FilePath &path : searchPath) {
        if (path.getBaseName() == "libraries") {
            libSearchPaths.append(path.getParentPath());
        }
        else {
            libSearchPaths.append(path);
        }
    }
    mxContext.registerSourceCodeSearchPath(libSearchPaths);
#endif

    // Add the Direct Light mtlx file to the mxDoc 
    mx::DocumentPtr lightDoc = mx::createDocument();
    mx::readFromXmlString(lightDoc, mxDirectLightString);
    mxDoc->importLibrary(lightDoc);

    // Make sure the Light data properties are added to the mxLightData struct
    mx::LightHandler lightHandler;
    std::vector<mx::NodePtr> lights;
    lightHandler.findLights(mxDoc, lights);
    lightHandler.registerLights(mxDoc, lights, mxContext);

    // Find renderable elements in the Mtlx Document.
    std::vector<mx::TypedElementPtr> renderableElements;
    mx::findRenderableElements(mxDoc, renderableElements);

    // Should have exactly one renderable element (material).
    if (renderableElements.size() != 1) {
        TF_CODING_ERROR("Generated MaterialX Document does not "
                        "have 1 material");
        return nullptr;
    }

    // Extract out the Surface Shader Node for the Material Node 
    mx::TypedElementPtr renderableElem = renderableElements.at(0);
    mx::NodePtr node = renderableElem->asA<mx::Node>();
    if (node && node->getType() == mx::MATERIAL_TYPE_STRING) {
        // Use auto so can compile against MaterialX 1.38.0 or 1.38.1
        auto mxShaderNodes = mx::getShaderNodes(node, mx::SURFACE_SHADER_TYPE_STRING);
        if (!mxShaderNodes.empty()) {
            renderableElem = *mxShaderNodes.begin();
        }
    }
    // Generate the PixelShader for the renderable element (surfaceshader).
    const mx::ElementPtr & mxElem = mxDoc->getDescendant(
                                            renderableElem->getNamePath());
    mx::TypedElementPtr typedElem = mxElem ? mxElem->asA<mx::TypedElement>()
                                         : nullptr;
    if (typedElem) {
        return _GenMaterialXShader(mxContext, typedElem);
    }
    TF_CODING_ERROR("Unable to generate a shader from the MaterialX Document");
    return nullptr;
}


////////////////////////////////////////////////////////////////////////////////
// Helper Functions to convert MX texture node parameters to Hd parameters

// Get the Hydra VtValue for the given MaterialX input value
static VtValue
_GetHdFilterValue(std::string const& mxInputValue)
{
    if(mxInputValue == "closest") {
        return VtValue(HdStTextureTokens->nearestMipmapNearest);
    }
    return VtValue(HdStTextureTokens->linearMipmapLinear);
}

// Get the Hydra VtValue for the given MaterialX input value
static VtValue
_GetHdSamplerValue(std::string const& mxInputValue)
{
    if (mxInputValue == "constant") {
        return VtValue(HdStTextureTokens->black);
    }
    if (mxInputValue == "clamp") {
        return VtValue(HdStTextureTokens->clamp);
    }
    if (mxInputValue == "mirror") {
        return VtValue(HdStTextureTokens->mirror);
    }
    return VtValue(HdStTextureTokens->repeat);
}

// Translate the MaterialX texture node input into the Hydra equivalents
static void
_GetHdTextureParameters(
    std::string const& mxInputName,
    std::string const& mxInputValue,
    std::map<TfToken, VtValue>* hdTextureParams)
{
    // MaterialX has two texture2d node types <image> and <tiledimage>

    // Properties common to both <image> and <tiledimage> texture nodes:
    if (mxInputName == "filtertype") {
        (*hdTextureParams)[HdStTextureTokens->minFilter] = 
            _GetHdFilterValue(mxInputValue);
        (*hdTextureParams)[HdStTextureTokens->magFilter] = 
            VtValue(HdStTextureTokens->linear);
    }

    // Properties specific to <image> nodes:
    else if (mxInputName == "uaddressmode") {
        (*hdTextureParams)[HdStTextureTokens->wrapS] = 
            _GetHdSamplerValue(mxInputValue);
    }
    else if (mxInputName == "vaddressmode") {
        (*hdTextureParams)[HdStTextureTokens->wrapT] = 
            _GetHdSamplerValue(mxInputValue);
    }

    // Properties specific to <tiledimage> nodes:
    else if (mxInputName == "uvtiling" || mxInputName == "uvoffset" ||
        mxInputName == "realworldimagesize" || 
        mxInputName == "realworldtilesize") {
        (*hdTextureParams)[HdStTextureTokens->wrapS] = 
            VtValue(HdStTextureTokens->repeat);
        (*hdTextureParams)[HdStTextureTokens->wrapT] = 
            VtValue(HdStTextureTokens->repeat);
    }
}

// Find the HdNode and its corresponding NodePath in the given HdNetwork 
// based on the given HdConnection
static bool 
_FindConnectedNode(
    HdMaterialNetwork2 const& hdNetwork,
    HdMaterialConnection2 const& hdConnection,
    HdMaterialNode2* hdNode,
    SdfPath* hdNodePath)
{
    // Get the path to the connected node
    const SdfPath & connectionPath = hdConnection.upstreamNode;

    // If this path is not in the network raise a warning
    auto hdNodeIt = hdNetwork.nodes.find(connectionPath);
    if (hdNodeIt == hdNetwork.nodes.end()) {
        TF_WARN("Unknown material node '%s'", connectionPath.GetText());
        return false;
    }

    // Otherwise return the HdNode and corresponding NodePath
    *hdNode = hdNodeIt->second;
    *hdNodePath = connectionPath;
    return true;
}

// Get the Texture coordinate name if specified, otherwise get the default name 
static void
_GetTextureCoordinateName(
    mx::DocumentPtr const &mxDoc,
    HdMaterialNetwork2* hdNetwork,
    HdMaterialNode2* hdTextureNode,
    SdfPath const& hdTextureNodePath,
    mx::StringMap* mxHdPrimvarMap,
    std::string* defaultTexcoordName)
{
    // Get the Texture Coordinate name through the connected node
    bool textureCoordSet = false;
    for (auto& inputConnections : hdTextureNode->inputConnections) {

        // Texture Coordinates are connected through the 'texcoord' input
        if ( inputConnections.first != _tokens->texcoord) {
            continue;
        }

        for (auto& currConnection : inputConnections.second) {

            // Get the connected Texture Coordinate node
            SdfPath hdCoordNodePath;
            HdMaterialNode2 hdCoordNode;
            const bool found = _FindConnectedNode(*hdNetwork, currConnection,
                                            &hdCoordNode, &hdCoordNodePath);
            if (!found) {
                continue;
            }
            
            // Get the texture coordinate name from the 'geomprop' parameter
            auto coordNameIt = hdCoordNode.parameters.find(_tokens->geomprop);
            if (coordNameIt != hdCoordNode.parameters.end()) {

                std::string const& texcoordName = 
                    HdMtlxConvertToString(coordNameIt->second);
                
                // Set the 'st' parameter as a TfToken
                hdNetwork->nodes[hdTextureNodePath].parameters[_tokens->st] = 
                                TfToken(texcoordName.c_str());

                // Save texture coordinate primvar name for the glslfx header;
                // figure out the mx typename
                mx::NodeDefPtr mxNodeDef = mxDoc->getNodeDef(
                        hdCoordNode.nodeTypeId.GetString());
                if (mxNodeDef) {
                    (*mxHdPrimvarMap)[texcoordName] = mxNodeDef->getType();
                    textureCoordSet = true;
                    break;
                }
            }
        }
    }
    
    // If we did not have a connected node, and the 'st' parameter is not set
    // get the default texture cordinate name from the textureNodes sdr metadata 
    if ( !textureCoordSet && hdTextureNode->parameters.find(_tokens->st) == 
                             hdTextureNode->parameters.end()) {

        // Get the sdr node for the mxTexture node
        SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
        const SdrShaderNodeConstPtr sdrTextureNode = 
            sdrRegistry.GetShaderNodeByIdentifierAndType(
                hdTextureNode->nodeTypeId, _tokens->mtlx);

        if (sdrTextureNode) {

            // Get the primvarname from the sdrTextureNode metadata
            auto metadata = sdrTextureNode->GetMetadata();
            auto primvarName = metadata[SdrNodeMetadata->Primvars];

            // Set the 'st' parameter as a TfToken
            hdNetwork->nodes[hdTextureNodePath].parameters[_tokens->st] = 
                            TfToken(primvarName.c_str());

            // Save the default texture coordinate name for the glslfx header
            *defaultTexcoordName = primvarName;
        }
        
    }
}

// Add the Hydra texture node parameters to the texture nodes and connect the 
// texture nodes to the terminal node
static void 
_UpdateTextureNodes(
    mx::DocumentPtr const &mxDoc,
    HdMaterialNetwork2* hdNetwork,
    SdfPath const& hdTerminalNodePath,
    std::set<SdfPath> const& hdTextureNodes,
    mx::StringMap* mxHdTextureMap,
    mx::StringMap* mxHdPrimvarMap,
    std::string* defaultTexcoordName)
{
    for (auto const& texturePath : hdTextureNodes) {
        HdMaterialNode2 hdTextureNode = hdNetwork->nodes[texturePath];

        _GetTextureCoordinateName(mxDoc, hdNetwork, &hdTextureNode, texturePath, 
                                  mxHdPrimvarMap, defaultTexcoordName);

        // Gather the Hydra Texture Parameters
        std::map<TfToken, VtValue> hdParameters;
        for (auto const& currParam : hdTextureNode.parameters) {
        
            // Get the MaterialX Input Value string
            std::string mxInputValue = HdMtlxConvertToString(currParam.second);

            // Get the Hydra equivalents for the MX Texture node parameters
            std::string mxInputName = currParam.first.GetText();
            _GetHdTextureParameters(mxInputName, mxInputValue, &hdParameters);
        }

        // Add the Hydra Texture Parameters to the Texture Node
        for (auto param : hdParameters) {
            hdNetwork->nodes[texturePath].parameters[param.first] = param.second;
        }

        // Connect the texture node to the terminal node for HdStMaterialNetwork
        // Create a unique name for the new connection, and update the
        // mxHdTextureMap with this connection name so Hydra's codegen and 
        // HdStMaterialXShaderGen match up correctly
        std::string newConnName = texturePath.GetName() + "_" + 
                                (*mxHdTextureMap)[texturePath.GetName()];;
        (*mxHdTextureMap)[texturePath.GetName()] = newConnName;

        // Make and add a new connection to the terminal node
        HdMaterialConnection2 textureConn;
        textureConn.upstreamNode = texturePath;
        textureConn.upstreamOutputName = TfToken(newConnName);
        hdNetwork->nodes[hdTerminalNodePath].
            inputConnections[textureConn.upstreamOutputName] = {textureConn};
    }
}

// Connect the primvar nodes to the terminal node
static void 
_UpdatePrimvarNodes(
    mx::DocumentPtr const &mxDoc,
    HdMaterialNetwork2* hdNetwork,
    SdfPath const& hdTerminalNodePath,
    std::set<SdfPath> const& hdPrimvarNodes,
    mx::StringMap* mxHdPrimvarMap)
{
    for (auto const& primvarPath : hdPrimvarNodes) {
        HdMaterialNode2 hdPrimvarNode = hdNetwork->nodes[primvarPath];

        // Save primvar name for the glslfx header
        auto primvarNameIt = hdPrimvarNode.parameters.find(_tokens->geomprop);
        if (primvarNameIt != hdPrimvarNode.parameters.end()) {
            std::string const& primvarName =
                HdMtlxConvertToString(primvarNameIt->second);

            // Figure out the mx typename
            mx::NodeDefPtr mxNodeDef = mxDoc->getNodeDef(
                    hdPrimvarNode.nodeTypeId.GetString());
            if (mxNodeDef) {
                (*mxHdPrimvarMap)[primvarName] = mxNodeDef->getType();
            }
        }

        // Texcoord nodes will have an index parameter set
        primvarNameIt = hdPrimvarNode.parameters.find(_tokens->index);
        if (primvarNameIt != hdPrimvarNode.parameters.end()) {
            // Get the sdr node for the texcoord node
            SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
            const SdrShaderNodeConstPtr sdrTexCoordNode = 
                sdrRegistry.GetShaderNodeByIdentifierAndType(
                    hdPrimvarNode.nodeTypeId, _tokens->mtlx);

            // Get the default texture coordinate name from the sdr metadata
            std::string texCoordName;
            if (sdrTexCoordNode) {
                auto metadata = sdrTexCoordNode->GetMetadata();
                texCoordName = metadata[SdrNodeMetadata->Primvars];
            }

            // Figure out the mx typename
            mx::NodeDefPtr mxNodeDef = mxDoc->getNodeDef(
                    hdPrimvarNode.nodeTypeId.GetString());
            if (mxNodeDef) {
                (*mxHdPrimvarMap)[texCoordName] = mxNodeDef->getType();
            }
        }

        // Connect the primvar node to the terminal node for HdStMaterialNetwork
        // Create a unique name for the new connection.
        std::string newConnName = primvarPath.GetName() + "_primvarconn";
        HdMaterialConnection2 primvarConn;
        primvarConn.upstreamNode = primvarPath;
        primvarConn.upstreamOutputName = TfToken(newConnName);
        hdNetwork->nodes[hdTerminalNodePath]
            .inputConnections[primvarConn.upstreamOutputName] = {primvarConn};
    }
}

static std::string const&
_GetMaterialTag(HdMaterialNode2 const& terminal)
{
    // Masked MaterialTag:
    // UsdPreviewSurface: terminal.opacityThreshold value > 0
    // StandardSurface materials do not have an opacityThreshold parameter
    // so we StandardSurface will not use the Masked materialTag.
    for (auto const& currParam : terminal.parameters) {
        if (currParam.first != _tokens->opacityThreshold) continue;

        if (currParam.second.Get<float>() > 0.0f) {
            return HdStMaterialTagTokens->masked.GetString();
        }
    }

    // Translucent MaterialTag
    bool isTranslucent = false;

    // UsdPreviewSurface uses the opacity parameter to indicate the transparency
    // when 1.0 the material is fully opaque, the smaller the value the more 
    // translucent the material, with a default value of 1.0 
    // StandardSurface indicates material transparency through two different 
    // parameters; the transmission parameter (float) where the greater the 
    // value the more transparent the material and a default value of 0.0,
    // the opacity parameter (color3) indicating the opacity of the entire 
    // material, where the default value of (1,1,1) is fully opaque.

    // First check the opacity and transmission connections
    auto const& opacityConnIt = terminal.inputConnections.find(_tokens->opacity);
    if (opacityConnIt != terminal.inputConnections.end()) {
        return HdStMaterialTagTokens->translucent.GetString();
    }

    auto const& transmissionConnIt = terminal.inputConnections.find(
                                                _tokens->transmission);
    isTranslucent = (transmissionConnIt != terminal.inputConnections.end());

    // Then check the opacity and transmission parameter value
    if (!isTranslucent) {
        for (auto const& currParam : terminal.parameters) {

            // UsdPreviewSurface
            if (currParam.first == _tokens->opacity && 
                currParam.second.IsHolding<float>()) {
                isTranslucent = currParam.second.Get<float>() < 1.0f;
                break;
            }
            // StandardSurface
            if (currParam.first == _tokens->opacity && 
                currParam.second.IsHolding<GfVec3f>()) {
                GfVec3f opacityColor = currParam.second.Get<GfVec3f>();
                isTranslucent |= ( opacityColor[0] < 1.0f 
                                || opacityColor[1] < 1.0f
                                || opacityColor[2] < 1.0f );
            }
            if (currParam.first == _tokens->transmission && 
                currParam.second.IsHolding<float>()) {
                isTranslucent |= currParam.second.Get<float>() > 0.0f;
            }
        }
    }

    if (isTranslucent) {
        return HdStMaterialTagTokens->translucent.GetString();
    }
    return HdStMaterialTagTokens->defaultMaterialTag.GetString();
}

void _AddMaterialXParams(
    mx::ShaderPtr const& glslfxShader,
    HdSt_MaterialParamVector* materialParams)
{
    if (!glslfxShader) {
        return;
    }

    const mx::ShaderStage& pxlStage = glslfxShader->getStage(mx::Stage::PIXEL);
    const auto& paramsBlock = pxlStage.getUniformBlock(mx::HW::PUBLIC_UNIFORMS);
    for (size_t i = 0; i < paramsBlock.size(); ++i) {

        // MaterialX parameter Information
        const auto variable = paramsBlock[i];
        const auto varValue = variable->getValue();
        std::istringstream valueStream(varValue 
            ? varValue->getValueString() : std::string());

        // Create a corresponding HdSt_MaterialParam
        HdSt_MaterialParam param;
        param.paramType = HdSt_MaterialParam::ParamTypeFallback;
        param.name = TfToken(variable->getVariable());

        std::string separator;
        const auto varType = variable->getType();
        if (varType->getBaseType() == mx::TypeDesc::BASETYPE_FLOAT) {
            if (varType->getSize() == 1) {
                float val;
                valueStream >> val;
                param.fallbackValue = VtValue(val);
            }
            else if (varType->getSize() == 2) {
                GfVec2f val;
                valueStream >> val[0] >> separator >> val[1];
                param.fallbackValue = VtValue(val);
            }
            else if (varType->getSize() == 3) {
                GfVec3f val;
                valueStream >> val[0] >> separator >> val[1] >> separator 
                            >> val[2];
                param.fallbackValue = VtValue(val);
            }
            else if (varType->getSize() == 4) {
                GfVec4f val;
                valueStream >> val[0] >> separator >> val[1] >> separator
                            >> val[2] >> separator >> val[3];
                param.fallbackValue = VtValue(val);
            }
        }
        else if (varType->getBaseType() == mx::TypeDesc::BASETYPE_INTEGER) {
            if (varType->getSize() == 1) {
                int val;
                valueStream >> val;
                param.fallbackValue = VtValue(val);
            }
            else if (varType->getSize() == 2) {
                GfVec2i val;
                valueStream >> val[0] >> separator >> val[1];
                param.fallbackValue = VtValue(val);
            }
            else if (varType->getSize() == 3) {
                GfVec3i val;
                valueStream >> val[0] >> separator >> val[1] >> separator 
                            >> val[2];
                param.fallbackValue = VtValue(val);
            }
            else if (varType->getSize() == 4) {
                GfVec4i val;
                valueStream >> val[0] >> separator >> val[1] >> separator
                    >> val[2] >> separator >> val[3];
                param.fallbackValue = VtValue(val);
            }
        }

        if (!param.fallbackValue.IsEmpty()) {
            materialParams->push_back(std::move(param));
        }
    }
}

void
HdSt_ApplyMaterialXFilter(
    HdMaterialNetwork2* hdNetwork,
    SdfPath const& materialPath,
    HdMaterialNode2 const& terminalNode,
    SdfPath const& terminalNodePath,
    HdSt_MaterialParamVector* materialParams,
    bool const bindlessTexturesEnabled)
{
    // Check if the Terminal is a MaterialX Node
    SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
    const SdrShaderNodeConstPtr mtlxSdrNode = 
        sdrRegistry.GetShaderNodeByIdentifierAndType(terminalNode.nodeTypeId, 
                                                     _tokens->mtlx);

    if (mtlxSdrNode) {

        // Load Standard Libraries/setup SearchPaths (for mxDoc and mxShaderGen)
        mx::FilePathVec libraryFolders;
        mx::FileSearchPath searchPath = HdMtlxSearchPaths();
        mx::DocumentPtr stdLibraries = mx::createDocument();
        mx::loadLibraries(libraryFolders, searchPath, stdLibraries);

        // Create the MaterialX Document from the HdMaterialNetwork
        MxHdInfo mxHdInfo; // Hydra information for MaterialX glslfx shaderGen 
        std::set<SdfPath> hdTextureNodes;
        std::set<SdfPath> hdPrimvarNodes;
        mx::DocumentPtr mtlxDoc = HdMtlxCreateMtlxDocumentFromHdNetwork(
                                        *hdNetwork,
                                        terminalNode,   // MaterialX HdNode
                                        terminalNodePath,
                                        materialPath,
                                        stdLibraries,
                                        &hdTextureNodes,
                                        &mxHdInfo.textureMap,
                                        &hdPrimvarNodes);

        // Add Hydra parameters for each of the Texture nodes
        _UpdateTextureNodes(mtlxDoc, hdNetwork, terminalNodePath, hdTextureNodes, 
                            &mxHdInfo.textureMap, &mxHdInfo.primvarMap, 
                            &mxHdInfo.defaultTexcoordName);

        _UpdatePrimvarNodes(mtlxDoc, hdNetwork, terminalNodePath,
                            hdPrimvarNodes, &mxHdInfo.primvarMap);

        mxHdInfo.materialTag = _GetMaterialTag(terminalNode);
        mxHdInfo.bindlessTexturesEnabled = bindlessTexturesEnabled;

        // Load MaterialX Document and generate the glslfxShader
        mx::ShaderPtr glslfxShader = HdSt_GenMaterialXShader(mtlxDoc,
                                    searchPath, mxHdInfo);
        if (!glslfxShader) {
            return;
        }
        // Add material parameters from the glslfxShader to the materialParams
        _AddMaterialXParams(glslfxShader, materialParams);

        // Create a new terminal node with the new glslfxSource
        const std::string glslfxSource =
            glslfxShader->getSourceCode(mx::Stage::PIXEL);
        SdrShaderNodeConstPtr sdrNode = 
            sdrRegistry.GetShaderNodeFromSourceCode(glslfxSource, 
                                                    HioGlslfxTokens->glslfx,
                                                    NdrTokenMap()); // metadata
        HdMaterialNode2 newTerminalNode;
        newTerminalNode.nodeTypeId = sdrNode->GetIdentifier();
        newTerminalNode.inputConnections = terminalNode.inputConnections;
        newTerminalNode.parameters = terminalNode.parameters;

        // Replace the original terminalNode with this newTerminalNode
        hdNetwork->nodes[terminalNodePath] = newTerminalNode;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
