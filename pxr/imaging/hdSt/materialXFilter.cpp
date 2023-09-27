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
#include "pxr/imaging/hdSt/package.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdMtlx/hdMtlx.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/usd/sdr/registry.h"
#include "pxr/imaging/hio/glslfx.h"
#include "pxr/imaging/hgi/capabilities.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/diagnostic.h"

#include <MaterialXGenShader/Util.h>
#include <MaterialXGenShader/DefaultColorManagementSystem.h>
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
    ((defaultInput, "default"))

    // Opacity Parameters
    (opacity)
    (opacityThreshold)
    (transmission)

    // Fallback Dome Light Tokens
    (domeLightFallback)
    (ND_image_color3)
    (file)

    // Colorspace Tokens
    (colorSpace)
    (sourceColorSpace)
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

    // MaterialX v1.38.5 added Transmission Refraction method as the default
    // method, this maintains the previous Transmission Opacity behavior.
#if MATERIALX_MAJOR_VERSION >= 1 && MATERIALX_MINOR_VERSION >= 38 && \
    MATERIALX_BUILD_VERSION >= 5
    materialContext.getOptions().hwTransmissionRenderMethod =
        mx::HwTransmissionRenderMethod::TRANSMISSION_OPACITY;
#endif
    
    // Use the domeLightPrefilter texture instead of sampling the Environment Map
    materialContext.getOptions().hwSpecularEnvironmentMethod =
        mx::HwSpecularEnvironmentMethod::SPECULAR_ENVIRONMENT_PREFILTER;

    return mx::createShader("Shader", materialContext, mxElem);
}

// Results in lightData.type = 1 for point lights in the Mx Shader
static const std::string mxDirectLightString = 
R"(
<?xml version="1.0"?>
<materialx version="1.38">
  <point_light name="pt_light" type="lightshader">
  </point_light>
  <directional_light name="dir_light" type="lightshader">
  </directional_light>
</materialx>
)";

static mx::GenContext
_CreateHdStMaterialXContext(
    HdSt_MxShaderGenInfo const& mxHdInfo,
    TfToken const& apiName)
{
#if MATERIALX_MAJOR_VERSION >= 1 && MATERIALX_MINOR_VERSION >= 38 && \
    MATERIALX_BUILD_VERSION >= 7
    if (apiName == HgiTokens->Metal) {
        return HdStMaterialXShaderGenMsl::create(mxHdInfo);
    }
#endif
    if (apiName == HgiTokens->OpenGL) {
        return HdStMaterialXShaderGenGlsl::create(mxHdInfo);
    }
    else {
        TF_CODING_ERROR(
            "MaterialX Shader Generator doesn't support %s API.",
            apiName.GetText());
        return mx::ShaderGeneratorPtr();
    }
}

// Use the given mxDocument to generate the corresponding glsl shader
// Based on MaterialXViewer Viewer::loadDocument()
mx::ShaderPtr
HdSt_GenMaterialXShader(
    mx::DocumentPtr const& mxDoc,
    mx::DocumentPtr const& stdLibraries,
    mx::FileSearchPath const& searchPath,
    HdSt_MxShaderGenInfo const& mxHdInfo,
    TfToken const& apiName)
{
    // Initialize the Context for shaderGen. 
    mx::GenContext mxContext = _CreateHdStMaterialXContext(mxHdInfo, apiName);

#if MATERIALX_MAJOR_VERSION == 1 && MATERIALX_MINOR_VERSION == 38 && \
    MATERIALX_BUILD_VERSION == 3
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

    // Initialize the color management system
    mx::DefaultColorManagementSystemPtr cms =
        mx::DefaultColorManagementSystem::create(
            mxContext.getShaderGenerator().getTarget());
    cms->loadLibrary(stdLibraries);
    mxContext.getShaderGenerator().setColorManagementSystem(cms);

    // Set the colorspace
    // XXX: This is the equivalent of the default source colorSpace, which does
    // not yet have a schema and is therefore not yet accessable here 
    mxDoc->setColorSpace("lin_rec709");

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
    if (mxInputValue == "closest") {
        return VtValue(HdStTextureTokens->nearestMipmapNearest);
    }
    // linear/cubic
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
    // periodic
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
}

static void
_AddDefaultMtlxTextureValues(std::map<TfToken, VtValue>* hdTextureParams)
{
    // MaterialX uses repeat/periodic for the default wrap values, without
    // this the texture will use the Hydra default useMetadata. 
    // Note that these will get overwritten by any authored values
    (*hdTextureParams)[HdStTextureTokens->wrapS] = 
        VtValue(HdStTextureTokens->repeat);
    (*hdTextureParams)[HdStTextureTokens->wrapT] = 
        VtValue(HdStTextureTokens->repeat);

    // Set the default colorSpace to be 'raw'. This allows MaterialX to handle
    // colorspace transforms.
    (*hdTextureParams) [_tokens->sourceColorSpace] = VtValue(HdStTokens->raw);
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

// Return the specified or default Texture coordinate name as a TfToken, 
// and initialize the primvar type or default name for MaterialX ShaderGen.
static TfToken
_GetTextureCoordinateName(
    mx::DocumentPtr const& mxDoc,
    HdMaterialNetwork2* hdNetwork,
    HdMaterialNode2 const& hdTextureNode,
    SdfPath const& hdTextureNodePath,
    mx::StringMap* mxHdPrimvarMap,
    std::string* defaultTexcoordName)
{
    // Get the Texture Coordinate name through the connected node
    bool textureCoordSet = false;
    std::string textureCoordName;
    for (auto const& inputConnections : hdTextureNode.inputConnections) {
        // Texture Coordinates are connected through the 'texcoord' input
        if ( inputConnections.first != _tokens->texcoord) {
            continue;
        }

        for (auto const& currConnection : inputConnections.second) {
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

                textureCoordName = HdMtlxConvertToString(coordNameIt->second);

                // Save texture coordinate primvar name for the glslfx header;
                // figure out the mx typename
                const mx::NodeDefPtr mxNodeDef = mxDoc->getNodeDef(
                        hdCoordNode.nodeTypeId.GetString());
                if (mxNodeDef) {
                    (*mxHdPrimvarMap)[textureCoordName] = mxNodeDef->getType();
                    textureCoordSet = true;
                    break;
                }
            }
        }
    }
    
    // If we did not have a connected node, and the 'st' parameter is not set
    // get the default texture cordinate name from the textureNodes sdr metadata 
    if ( !textureCoordSet && hdTextureNode.parameters.find(_tokens->st) == 
                             hdTextureNode.parameters.end()) {
        // Get the sdr node for the mxTexture node
        SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
        const SdrShaderNodeConstPtr sdrTextureNode = 
            sdrRegistry.GetShaderNodeByIdentifierAndType(
                hdTextureNode.nodeTypeId, _tokens->mtlx);

        if (sdrTextureNode) {
            // Get the texture coordinate name from the sdrTextureNode metadata
            auto metadata = sdrTextureNode->GetMetadata();
            textureCoordName = metadata[SdrNodeMetadata->Primvars];

            // Save the default texture coordinate name for the glslfx header
            *defaultTexcoordName = textureCoordName;
        }
    }
    return TfToken(textureCoordName);
}

static void
_AddFallbackDomeLightTextureNode(
    HdMaterialNetwork2* hdNetwork,
    SdfPath const& hdTerminalNodePath,
    mx::StringMap* mxHdTextureMap=nullptr)
{
    // Create and add a Fallback Dome Light Texture Node to the hdNetwork
    HdMaterialNode2 hdDomeTextureNode;
    hdDomeTextureNode.nodeTypeId = _tokens->ND_image_color3;
    hdDomeTextureNode.parameters[_tokens->file] =
        VtValue(SdfAssetPath(
            HdStPackageFallbackDomeLightTexture(), 
            HdStPackageFallbackDomeLightTexture()));
    const SdfPath domeTexturePath = 
        hdTerminalNodePath.ReplaceName(_tokens->domeLightFallback);
    hdNetwork->nodes.insert({domeTexturePath, hdDomeTextureNode});

    // Connect the new Texture Node to the Terminal Node
    HdMaterialConnection2 domeTextureConn;
    domeTextureConn.upstreamNode = domeTexturePath;
    domeTextureConn.upstreamOutputName = domeTexturePath.GetNameToken();
    hdNetwork->nodes[hdTerminalNodePath].
        inputConnections[domeTextureConn.upstreamOutputName] = {domeTextureConn};

    // Add the Dome Texture name to the TextureMap for MaterialXShaderGen
    if (mxHdTextureMap) {
        (*mxHdTextureMap)[domeTexturePath.GetName()] = domeTexturePath.GetName();
    }
}

static TfToken
_GetHdNodeTypeId(mx::NodePtr const& mxNode)
{
    std::string nodeDefName = mxNode->getNodeDefString();
    if (nodeDefName.empty()) {
        if (mx::NodeDefPtr const& imageNodeDef = mxNode->getNodeDef()) {
            nodeDefName = imageNodeDef->getName();
        }
    }
    return TfToken(nodeDefName);
}

// If the imageNode is connected to the given filenameInput, return 
// the hdNodeTypeId and any texture parameters on the node
static bool
_GatherTextureNodeInformation(
    mx::NodePtr const& imageNode,
    std::string const& filenameInputName,
    TfToken* textureNodeTypeId,
    std::map<TfToken, VtValue>* hdParameters)
{
    const mx::InputPtr fileInput = imageNode->getActiveInput("file");
    if (!fileInput || fileInput->getInterfaceName() != filenameInputName) {
        return false;
    }
    *textureNodeTypeId = _GetHdNodeTypeId(imageNode);
    for (mx::InputPtr const& input : imageNode->getActiveInputs()) {
        _GetHdTextureParameters(
            input->getName(), input->getValueString(), hdParameters);
    }
    return true;
}

// Replace the filenameInput parameter with a connection to a new texture node
static void
_ReplaceFilenameInput(
    mx::DocumentPtr const& mxDoc,
    HdMaterialNetwork2* hdNetwork,
    HdMaterialNode2 const& hdTerminalNode,
    SdfPath const& hdTerminalNodePath,
    mx::NodeDefPtr const& mxNodeDef,
    std::string const& filenameInputName,
    mx::StringMap* mxHdTextureMap,
    mx::StringMap* mxHdPrimvarMap,
    std::string* defaultTexcoordName)
{
    // Get the Stdlib Image node from the Implementation Nodegraph and gather 
    // nodeTypeId and parameter information. 
    TfToken terminalTextureTypeId;
    std::map<TfToken, VtValue> terminalTextureParams;
    _AddDefaultMtlxTextureValues(&terminalTextureParams);
    const mx::InterfaceElementPtr impl = mxNodeDef->getImplementation();
    if (impl && impl->isA<mx::NodeGraph>()) {
        const mx::NodeGraphPtr ng = impl->asA<mx::NodeGraph>();

        // Check the custom Material for <image> nodes
        for (mx::NodePtr const& imageNode : ng->getNodes("image")) {
            const bool foundConnectedImageNode =
                _GatherTextureNodeInformation(imageNode, filenameInputName, 
                    &terminalTextureTypeId, &terminalTextureParams);
            if (foundConnectedImageNode) {
                break;
            }
        }
        // Check the custom Material for <tiledimage> nodes
        for (mx::NodePtr const& imageNode : ng->getNodes("tiledimage")) {
            const bool foundConnectedImageNode = 
                _GatherTextureNodeInformation(imageNode, filenameInputName, 
                    &terminalTextureTypeId, &terminalTextureParams);
            if (foundConnectedImageNode) {
                break;
            }
        }
    }
    if (terminalTextureTypeId.IsEmpty()) {
        return;
    }

    // Gather the Hydra Texture Parameters on the terminal node.
    for (auto const& param : hdTerminalNode.parameters) {
        // Get the Hydra equivalents for the Mx Texture node parameters
        std::string const& mxInputName = param.first.GetString();
        std::string const mxInputValue = HdMtlxConvertToString(param.second);
        _GetHdTextureParameters(
            mxInputName, mxInputValue, &terminalTextureParams);
    }

    // Get the filename parameter value from the terminal node
    const TfToken filenameToken(filenameInputName);
    auto filenameParamIt = hdTerminalNode.parameters.find(filenameToken);
    if (filenameParamIt == hdTerminalNode.parameters.end()) {
        return;
    }

    // Create a new Texture Node
    HdMaterialNode2 terminalTextureNode;
    terminalTextureNode.nodeTypeId = terminalTextureTypeId;
    terminalTextureNode.parameters[_tokens->file] = filenameParamIt->second;
    terminalTextureNode.parameters[_tokens->st] = _GetTextureCoordinateName(
        mxDoc, hdNetwork, hdTerminalNode, hdTerminalNodePath, 
        mxHdPrimvarMap, defaultTexcoordName);
    for (auto const& param : terminalTextureParams) {
        terminalTextureNode.parameters[param.first] = param.second;
    }

    // Add the Texture Node to the hdNetwork 
    const SdfPath terminalTexturePath = 
        hdTerminalNodePath.AppendChild(filenameToken);
    hdNetwork->nodes.insert({terminalTexturePath, terminalTextureNode});

    // Make a new connection to the terminal node
    HdMaterialConnection2 terminalTextureConn;
    terminalTextureConn.upstreamNode = terminalTexturePath;
    terminalTextureConn.upstreamOutputName = terminalTexturePath.GetNameToken();

    // Replace the filename parameter with the TerminalTextureConnection
    hdNetwork->nodes[hdTerminalNodePath].parameters.erase(filenameParamIt);
    hdNetwork->nodes[hdTerminalNodePath].
        inputConnections[terminalTextureConn.upstreamOutputName] =
            {terminalTextureConn};

    // Insert the new texture into the mxHdTextureMap for MxShaderGen
    if (mxHdTextureMap) {
        (*mxHdTextureMap)[filenameInputName] = filenameInputName;
    }
}

// Add the Hydra texture node parameters to the texture nodes and connect the 
// texture nodes to the terminal node
static void 
_UpdateTextureNodes(
    mx::DocumentPtr const &mxDoc,
    HdMaterialNetwork2* hdNetwork,
    HdMaterialNode2 const& hdTerminalNode,
    SdfPath const& hdTerminalNodePath,
    std::set<SdfPath> const& hdTextureNodes,
    mx::StringMap const& hdMtlxTextureInfo,
    mx::StringMap* mxHdTextureMap,
    mx::StringMap* mxHdPrimvarMap,
    std::string* defaultTexcoordName)
{
    // Storm does not expect textures to be direct inputs on materials, replace 
    // each 'filename' input on the material with a connection to an image node
    const mx::NodeDefPtr mxMaterialNodeDef = 
        mxDoc->getNodeDef(hdTerminalNode.nodeTypeId.GetString());
    if (mxMaterialNodeDef) {
        for (auto const& mxInput : mxMaterialNodeDef->getActiveInputs()) {
            if (mxInput->getType() == "filename") {
                _ReplaceFilenameInput(
                    mxDoc, hdNetwork, hdTerminalNode, hdTerminalNodePath, 
                    mxMaterialNodeDef, mxInput->getName(), 
                    mxHdTextureMap, mxHdPrimvarMap, defaultTexcoordName);
            }
        }
    }

    for (SdfPath const& texturePath : hdTextureNodes) {
        HdMaterialNode2 hdTextureNode = hdNetwork->nodes[texturePath];

        // Set the texture coordinate name as the 'st' parameter.
        hdNetwork->nodes[texturePath].parameters[_tokens->st] =
            _GetTextureCoordinateName(
                mxDoc, hdNetwork, hdTextureNode, texturePath, 
                mxHdPrimvarMap, defaultTexcoordName);

        // Gather the Hydra Texture Parameters
        std::map<TfToken, VtValue> hdParameters;
        _AddDefaultMtlxTextureValues(&hdParameters);
        for (auto const& param : hdTextureNode.parameters) {
            // Get the Hydra equivalents for the Mx Texture node parameters
            std::string const& mxInputName = param.first.GetString();
            std::string const mxInputValue = HdMtlxConvertToString(param.second);
            _GetHdTextureParameters(mxInputName, mxInputValue, &hdParameters);
        }

        // Add the Hydra Texture Parameters to the Texture Node
        for (auto const& param : hdParameters) {
            hdNetwork->nodes[texturePath].parameters[param.first] = param.second;
        }

        // Connect the texture node to the terminal node for HdStMaterialNetwork
        // Create a unique name for the new connection, and store in the 
        // mxHdTextureMap so Hydra's codegen and HdStMaterialXShaderGen 
        // match up correctly.
        std::string newConnName = texturePath.GetName() + "_" + 
            hdMtlxTextureInfo.find(texturePath.GetName())->second;

        // Replace the texturePath.GetName() in the textureMap to the variable
        // name used in the shader: textureName_fileInputName 
        std::string fileInputName = "file";
        const mx::NodeDefPtr mxNodeDef =
            mxDoc->getNodeDef(hdTextureNode.nodeTypeId.GetString());
        if (mxNodeDef) {
            for (auto const& mxInput : mxNodeDef->getActiveInputs()) {
                if (mxInput->getType() == "filename") {
                    fileInputName = mxInput->getName();
                }
            }
        }
        mxHdTextureMap->insert(std::pair<std::string, std::string>(
                texturePath.GetName() + "_" + fileInputName, newConnName));

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
    mx::DocumentPtr const& mxDoc,
    HdMaterialNetwork2* hdNetwork,
    SdfPath const& hdTerminalNodePath,
    std::set<SdfPath> const& hdPrimvarNodes,
    mx::StringMap* mxHdPrimvarMap,
    mx::StringMap* mxHdPrimvarDefaultValueMap)
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

            // Get the Default value if authored
            std::string defaultPrimvarValue;
            const auto defaultPrimvarValueIt =
                hdPrimvarNode.parameters.find(_tokens->defaultInput);
            if (hdPrimvarNode.parameters.end() != defaultPrimvarValueIt) {
                defaultPrimvarValue = 
                    HdMtlxConvertToString(defaultPrimvarValueIt->second);
            }
            (*mxHdPrimvarDefaultValueMap)[primvarName] = defaultPrimvarValue;
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

// Gather the Material Params from the glslfx ShaderPtr
void
_AddMaterialXParams(
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
        if (varType->getBaseType() == mx::TypeDesc::BASETYPE_BOOLEAN) {
            const bool val = valueStream.str() == "true";
            param.fallbackValue = VtValue(val);
        }
        else if (varType->getBaseType() == mx::TypeDesc::BASETYPE_FLOAT) {
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

// Gather the Material Params from the terminal HdMaterialNode/SdrShaderNode
void 
_AddMaterialXParams(
    mx::ShaderPtr const& glslfxShader,
    HdMaterialNode2 const& terminalNode,
    SdrShaderNodeConstPtr const& mtlxSdrNode,
    HdSt_MaterialParamVector* materialParams)
{
    if (!glslfxShader) {
        return;
    }

    const mx::ShaderStage& pxlStage = glslfxShader->getStage(mx::Stage::PIXEL);
    const auto& paramsBlock = pxlStage.getUniformBlock(mx::HW::PUBLIC_UNIFORMS);
    for (size_t i = 0; i < paramsBlock.size(); ++i) {

        // MaterialX parameter Information
        const mx::ShaderPort* variable = paramsBlock[i];

        // Create a corresponding HdSt_MaterialParam
        HdSt_MaterialParam param;
        param.paramType = HdSt_MaterialParam::ParamTypeFallback;
        param.name = TfToken(variable->getVariable());

        const auto varType = variable->getType()->getBaseType();
        if (varType == mx::TypeDesc::BASETYPE_FLOAT ||
            varType == mx::TypeDesc::BASETYPE_INTEGER ||
            varType == mx::TypeDesc::BASETYPE_BOOLEAN) {
            
            // Get the paramter value from the terminal node
            const auto paramIt = terminalNode.parameters.find(param.name);
            if (paramIt != terminalNode.parameters.end()) {
                param.fallbackValue = paramIt->second;
            }
            // Only the authored values are in the terminalNode.parameters
            // get the default value from the mtlxSdrNode.
            else if (const auto input = mtlxSdrNode->GetInput(param.name)) {
                param.fallbackValue = input->GetDefaultValue();
            }
        }

        if (!param.fallbackValue.IsEmpty()) {
            materialParams->push_back(std::move(param));
        }
    }
}

static mx::ShaderPtr
_GenerateMaterialXShader(
    HdMaterialNetwork2* hdNetwork,
    SdfPath const& materialPath,
    HdMaterialNode2 const& terminalNode,
    SdfPath const& terminalNodePath,
    TfToken const& materialTagToken,
    TfToken const& apiName,
    bool const bindlessTexturesEnabled)
{
    // Load Standard Libraries/setup SearchPaths (for mxDoc and mxShaderGen)
    mx::FilePathVec libraryFolders;
    mx::FileSearchPath searchPath = HdMtlxSearchPaths();
    mx::DocumentPtr stdLibraries = mx::createDocument();
    mx::loadLibraries(libraryFolders, searchPath, stdLibraries);

    // Create the MaterialX Document from the HdMaterialNetwork
    HdSt_MxShaderGenInfo mxHdInfo;
    HdMtlxTexturePrimvarData hdMtlxData;
    mx::DocumentPtr mtlxDoc = HdMtlxCreateMtlxDocumentFromHdNetwork(
                                    *hdNetwork,
                                    terminalNode,   // MaterialX HdNode
                                    terminalNodePath,
                                    materialPath,
                                    stdLibraries,
                                    &hdMtlxData);

    // Add a Fallback DomeLight texture node to make sure the indirect
    // light computations always has a texture to sample from
    _AddFallbackDomeLightTextureNode(
        hdNetwork, terminalNodePath, &mxHdInfo.textureMap);

    // Add Hydra parameters for each of the Texture nodes
    _UpdateTextureNodes(mtlxDoc, hdNetwork, terminalNode, terminalNodePath, 
                        hdMtlxData.hdTextureNodes, hdMtlxData.mxHdTextureMap,
                        &mxHdInfo.textureMap, &mxHdInfo.primvarMap, 
                        &mxHdInfo.defaultTexcoordName);

    _UpdatePrimvarNodes(mtlxDoc, hdNetwork, terminalNodePath,
                        hdMtlxData.hdPrimvarNodes, &mxHdInfo.primvarMap,
                        &mxHdInfo.primvarDefaultValueMap);

    mxHdInfo.materialTag = materialTagToken.GetString();
    mxHdInfo.bindlessTexturesEnabled = bindlessTexturesEnabled;
    
    // Generate the glslfx source code from the mtlxDoc
    return HdSt_GenMaterialXShader(
        mtlxDoc, stdLibraries, searchPath, mxHdInfo, apiName);
}

void
HdSt_ApplyMaterialXFilter(
    HdMaterialNetwork2* hdNetwork,
    SdfPath const& materialPath,
    HdMaterialNode2 const& terminalNode,
    SdfPath const& terminalNodePath,
    HdSt_MaterialParamVector* materialParams,
    HdStResourceRegistry *resourceRegistry)
{
    // Check if the Terminal is a MaterialX Node
    SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
    const SdrShaderNodeConstPtr mtlxSdrNode = 
        sdrRegistry.GetShaderNodeByIdentifierAndType(terminalNode.nodeTypeId, 
                                                     _tokens->mtlx);

    if (mtlxSdrNode) {

        mx::ShaderPtr glslfxShader;
        const TfToken materialTagToken(_GetMaterialTag(terminalNode));
        const bool bindlessTexturesEnabled = 
            resourceRegistry->GetHgi()->GetCapabilities()->IsSet(
                HgiDeviceCapabilitiesBitsBindlessTextures);
        const TfToken apiName = resourceRegistry->GetHgi()->GetAPIName();

        // If the MaterialNetwork has just a terminal node, utilize the
        // Resource Registry to cache the generated MaterialX glslfx Shader
        if (hdNetwork->nodes.size() == 1) {
            // Get the glslfx ShaderPtr for the terminal node
            Tf_HashState terminalNodeHash;
            TfHashAppend(terminalNodeHash, terminalNode.nodeTypeId);
            TfHashAppend(terminalNodeHash, materialTagToken);
            for (auto param : terminalNode.parameters) {
                auto result = SdfPath::StripPrefixNamespace(
                        param.first.GetString(), _tokens->colorSpace);
                if (result.second) {
                    TfHashAppend(terminalNodeHash, param);
                }
            }
            HdInstance<mx::ShaderPtr> glslfxInstance = 
                resourceRegistry->RegisterMaterialXShader(
                    terminalNodeHash.GetCode());

            if (glslfxInstance.IsFirstInstance()) {
                // Generate the MaterialX glslfx ShaderPtr
                try {
                    glslfxShader = _GenerateMaterialXShader(
                        hdNetwork, materialPath, terminalNode, terminalNodePath,
                        materialTagToken, apiName, bindlessTexturesEnabled);
                } catch (mx::Exception& exception) {
                    TF_CODING_ERROR("Unable to create the Glslfx Shader.\n"
                        "MxException: %s", exception.what());
                }

                // Store the mx::ShaderPtr 
                glslfxInstance.SetValue(glslfxShader);

                // Add material parameters from the glslfxShader
                _AddMaterialXParams(glslfxShader, materialParams);

            } else {
                // Get the mx::ShaderPtr from the resource registry
                glslfxShader = glslfxInstance.GetValue();

                // Add a Fallback DomeLight texture node to the network
                _AddFallbackDomeLightTextureNode(hdNetwork, terminalNodePath);

                // Add material parameters from the terminal node
                _AddMaterialXParams(glslfxShader, terminalNode, mtlxSdrNode, 
                                    materialParams);
            }
        }
        else {
            // Process the network and generate the MaterialX glslfx ShaderPtr
            try {
                glslfxShader = _GenerateMaterialXShader(
                    hdNetwork, materialPath, terminalNode, terminalNodePath, 
                    materialTagToken, apiName, bindlessTexturesEnabled);
            } catch (mx::Exception& exception) {
                TF_CODING_ERROR("Unable to create the Glslfx Shader.\n"
                    "MxException: %s", exception.what());
            }

            // Add material parameters from the glslfxShader
            _AddMaterialXParams(glslfxShader, materialParams);
        }

        // Create a new terminal node with the glslfxShader
        if (glslfxShader) {
            const std::string glslfxSourceCode =
                glslfxShader->getSourceCode(mx::Stage::PIXEL);
            SdrShaderNodeConstPtr sdrNode = 
                sdrRegistry.GetShaderNodeFromSourceCode(glslfxSourceCode, 
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
}

PXR_NAMESPACE_CLOSE_SCOPE
