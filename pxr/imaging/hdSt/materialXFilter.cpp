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
#include "pxr/imaging/hdSt/materialXFilter.h"
#include "pxr/imaging/hdSt/materialXShaderGen.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/usd/sdr/registry.h"
#include "pxr/imaging/hio/glslfx.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/diagnostic.h"

#include <MaterialXCore/Node.h>
#include <MaterialXFormat/XmlIo.h>
#include <MaterialXGenShader/Util.h>
#include <MaterialXGenShader/Shader.h>
#include <MaterialXRender/Util.h>
#include <MaterialXRender/LightHandler.h> 

namespace mx = MaterialX;

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (mtlx)

    // Texture Coordinate Token
    (st)

    // Hd MaterialX Node Types
    (ND_standard_surface_surfaceshader)
    (ND_UsdPreviewSurface_surfaceshader)

    // MaterialX Node Types
    (standard_surface)
    (UsdPreviewSurface)
);


////////////////////////////////////////////////////////////////////////////////
// Shader Gen Functions

// Generate the Glsl Pixel Shader based on the given mxContext and mxElement
// Based on MaterialXViewer Material::generateShader()
static std::string
_GenPixelShader(mx::GenContext & mxContext, 
                mx::ElementPtr const& mxElem)
{
    bool hasTransparency = mx::isTransparentSurface(mxElem, 
                                mxContext.getShaderGenerator());

    mx::GenContext materialContext = mxContext;
    materialContext.getOptions().hwTransparency = hasTransparency;
    materialContext.getOptions().hwShadowMap = 
        materialContext.getOptions().hwShadowMap && !hasTransparency;
    
    // Use the domeLightPrefilter texture instead of sampling the Environment Map
    materialContext.getOptions().hwSpecularEnvironmentMethod =
        mx::HwSpecularEnvironmentMethod::SPECULAR_ENVIRONMENT_PREFILTER;

    mx::ShaderPtr mxShader = mx::createShader("Shader", materialContext, mxElem);
    if (mxShader) {
        return mxShader->getSourceCode(mx::Stage::PIXEL);
    }
    return mx::EMPTY_STRING;
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

// Use the given mxDocument to generate the corresponding glsl source code
// Based on MaterialXViewer Viewer::loadDocument()
std::string
HdSt_GenMaterialXShaderCode(
    mx::DocumentPtr const& mxDoc,
    mx::FileSearchPath const& searchPath,
    mx::StringMap const& mxHdTextureMap)
{
    // Initialize the Context for shaderGen. 
    mx::GenContext mxContext = HdStMaterialXShaderGen::create(mxHdTextureMap);
    mxContext.registerSourceCodeSearchPath(searchPath);
    
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
        return mx::EMPTY_STRING;
    }

    // Extract out the Surface Shader Node for the Material Node 
    mx::TypedElementPtr renderableElem = renderableElements.at(0);
    mx::NodePtr node = renderableElem->asA<mx::Node>();
    if (node && node->getType() == mx::MATERIAL_TYPE_STRING) {
        std::unordered_set<mx::NodePtr> mxShaderNodes;
        mxShaderNodes = mx::getShaderNodes(node, mx::SURFACE_SHADER_TYPE_STRING);
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
        return _GenPixelShader(mxContext, typedElem);
    }
    return mx::EMPTY_STRING;
}


////////////////////////////////////////////////////////////////////////////////
// Convert HdMaterialNetwork to MaterialX Document Helper Functions

// Convert the Token to the MaterialX Node Type
static TfToken
_GetMxNodeType(TfToken const& hdNodeType)
{
    if (hdNodeType == _tokens->ND_standard_surface_surfaceshader) {
        return _tokens->standard_surface;
    } 
    else if (hdNodeType == _tokens->ND_UsdPreviewSurface_surfaceshader) {
        return _tokens->UsdPreviewSurface;
    } 
    else {
        TF_WARN("Unsupported Node Type '%s'", hdNodeType.GetText());
        return TfToken();
    }
}

// Determine if the given mxInputName is of type mx::Vector3 
// Hd stores both mx::Vector3 and mx::Color3 as a GlfVec3f
static bool
_IsInputVector3(std::string const& mxInputName)
{    
    // mxInputs from UsdPreviewSurface and standard_surface nodes that are 
    // Vector3 types
    static const mx::StringSet Vector3Inputs = {"normal", 
                                                "coat_normal",
                                                "tangent"};
    return Vector3Inputs.count(mxInputName) > 0;
}

// Find the HdNode and its corresponding NodePath in the given HdNetwork 
// based on the given HdConnection
static bool 
_FindConnectedNode(
    HdMaterialNetwork2 const& hdNetwork,
    HdMaterialConnection2 const& hdConnection,
    HdMaterialNode2 * hdNode,
    SdfPath * hdNodePath)
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
    if (mxInputValue == "constant" || mxInputValue == "clamp") {
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
    std::map<TfToken, VtValue> * hdTextureParams)
{
    // MaterialX has two texture2d node types <image> and <tiledimage>

    // Properties common to both <image> and <tiledimage> texture nodes:
    if (mxInputName == "file") {
        // Add 'st' as a parameter for texture nodes, the mesh itself will
        // still need texture coordinate primvars to work correctly
        (*hdTextureParams)[_tokens->st] = VtValue(_tokens->st);
    }
    else if (mxInputName == "filtertype") {
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

// Add the mxNode to the mxNodeGraph, or get the mxNode from the NodeGraph 
static mx::NodePtr
_AddNodeToNodeGraph(
    std::string const& mxNodeName, 
    std::string const& mxNodeCategory, 
    std::string const& mxNodeType, 
    mx::NodeGraphPtr const& mxNodeGraph,
    mx::StringSet * addedNodeNames)
{
    // Add the node to the  mxNodeGraph if needed 
    if (addedNodeNames->find(mxNodeName) == addedNodeNames->end()) {
        addedNodeNames->insert(mxNodeName);
        return mxNodeGraph->addNode(mxNodeCategory, mxNodeName, mxNodeType);
    }
    // Otherwise get the existing node from the mxNodeGraph
    return mxNodeGraph->getNode(mxNodeName);
}

// Extract the mxInputValue from the HdParameter
static void 
_GetMxInputValue(
    std::pair<TfToken, VtValue> const& hdParameter,
    std::string * mxInputValue)
{
    std::ostringstream valStream;
    VtValue hdParameterValue = hdParameter.second;
    if (hdParameterValue.IsHolding<bool>()) {
        *mxInputValue = (hdParameterValue.UncheckedGet<bool>()) ? "false" 
                                                                : "true";
    }
    else if (hdParameterValue.IsHolding<int>() || 
             hdParameterValue.IsHolding<float>()) {
        valStream << hdParameterValue;
        *mxInputValue = valStream.str();
    }
    else if (hdParameterValue.IsHolding<GfVec2f>()) {
        const GfVec2f & value = hdParameterValue.UncheckedGet<GfVec2f>();
        valStream << value.data()[0] << ", " << value.data()[1];
        *mxInputValue = valStream.str();
    }
    else if (hdParameterValue.IsHolding<GfVec3f>()) {
        const GfVec3f & value = hdParameterValue.UncheckedGet<GfVec3f>();
        valStream << value.data()[0] << ", " << value.data()[1] << ", "
                  << value.data()[2];
        *mxInputValue = valStream.str();
    }
    else if (hdParameterValue.IsHolding<GfVec4f>()) {
        const GfVec4f & value = hdParameterValue.UncheckedGet<GfVec4f>();
        valStream << value.data()[0] << ", " << value.data()[1] << ", "
                  << value.data()[2] << ", " << value.data()[3];
        *mxInputValue = valStream.str();
    }
    else if (hdParameterValue.IsHolding<GfMatrix3d>()) {
        const GfMatrix3d & value = hdParameterValue.UncheckedGet<GfMatrix3d>();
        valStream << value[0][0] << ", " << value[0][1] << ", "
                  << value[0][2] << ",  "
                  << value[1][0] << ", " << value[1][1] << ", "
                  << value[1][2] << ",  "
                  << value[2][0] << ", " << value[2][1] << ", "
                  << value[2][2] << ",  ";
        *mxInputValue = valStream.str();
    }
    else if (hdParameterValue.IsHolding<GfMatrix4d>()) {
        const GfMatrix4d & value = hdParameterValue.UncheckedGet<GfMatrix4d>();
        valStream << value[0][0] << ", " << value[0][1] << ", "
                  << value[0][2] << ", " << value[0][3] << ",  "
                  << value[1][0] << ", " << value[1][1] << ", "
                  << value[1][2] << ", " << value[1][3] << ",  "
                  << value[2][0] << ", " << value[2][1] << ", "
                  << value[2][2] << ", " << value[2][3] << ",  "
                  << value[3][0] << ", " << value[3][1] << ", "
                  << value[3][2] << ", " << value[3][3] << ",  ";
        *mxInputValue = valStream.str();
    }
    else if (hdParameterValue.IsHolding<SdfAssetPath>()) {
        *mxInputValue = hdParameterValue.UncheckedGet<SdfAssetPath>().GetAssetPath();
    }
    else if (hdParameterValue.IsHolding<std::string>()) {
        *mxInputValue = hdParameterValue.UncheckedGet<std::string>();
    }
    else {
        TF_WARN("Unsupported Parameter Type '%s'", 
                hdParameterValue.GetTypeName().c_str());
    }
}

// Get the MaterialX Input information from the mxNodeDef and hdParameter
static void
_GetMxInputInfo(
    std::pair<TfToken, VtValue> const& hdParameter,
    mx::NodeDefPtr const& mxNodeDef,
    std::string * mxInputName,
    std::string * mxInputValue,
    std::string * mxInputType)
{
    // Get the mxInputName from the HdParameter
    *mxInputName = hdParameter.first.GetText();

    // Get the mxInputValue from the HdParameter
    _GetMxInputValue(hdParameter, mxInputValue);

    // Get the mxInputType for the mxNodeDef
    mx::InputPtr mxInput = mxNodeDef->getInput(*mxInputName);
    if (mxInput) {
        *mxInputType = mxInput->getType();
    }
}

// Add the Hydra texture node parameters to the texture nodes
static void 
_AddHdTextureNodeParameters(
    HdMaterialNetwork2 * hdNetwork,
    std::set<SdfPath> const& hdTextureNodes)
{
    for (auto texturePath : hdTextureNodes) {
        auto & textureNode = hdNetwork->nodes[texturePath];

        // Gather the Hydra Texture Parameters
        std::map<TfToken, VtValue> hdParameters;
        for (auto const& currParam : textureNode.parameters) {
        
            // Get the MaterialX Parameter info
            std::string mxInputValue;
            _GetMxInputValue(currParam, &mxInputValue);

            // Get the Hydra equivalents for the MX Texture node parameters
            std::string mxInputName = currParam.first.GetText();
            _GetHdTextureParameters(mxInputName, mxInputValue, &hdParameters);
        }

        // Add the Hydra Texture Parameters to the Texture Node
        for (auto param : hdParameters) {
            textureNode.parameters[param.first] = param.second;
        }    
    }
}

// Add a MaterialX version of the HdNode to the mxDoc/mxNodeGraph
static mx::NodePtr 
_AddMaterialXNode(
    HdMaterialNetwork2 const& hdNetwork,
    HdMaterialNode2 const& hdNode,
    SdfPath const& hdNodePath,
    mx::DocumentPtr const& mxDoc,
    mx::NodeGraphPtr const& mxNodeGraph,
    mx::StringSet * addedNodeNames,
    std::set<SdfPath> * hdTextureNodes,
    std::string const& connectionName,
    mx::StringMap * mxHdTextureMap)
{
    // Get the mxNode information
    mx::NodeDefPtr mxNodeDef = mxDoc->getNodeDef(hdNode.nodeTypeId.GetString());
    if (!mxNodeDef){
        TF_WARN("NodeDef not found for Node '%s'", hdNode.nodeTypeId.GetText());
        return mx::NodePtr();
    }
    const std::string & mxNodeCategory = mxNodeDef->getNodeString();
    const std::string & mxNodeType = mxNodeDef->getType();
    const std::string & mxNodeName = hdNodePath.GetName();

    // Add the mxNode to the mxNodeGraph
    mx::NodePtr mxNode = _AddNodeToNodeGraph(mxNodeName, mxNodeCategory, 
                                    mxNodeType, mxNodeGraph, addedNodeNames);

    // For each of the HdNode parameters add the corresponding parameter/input 
    // to the mxNode
    for (auto const& currParam : hdNode.parameters) {
        
        // Get the MaterialX Parameter info
        std::string mxInputName, mxInputValue, mxInputType;
        _GetMxInputInfo(currParam, mxNodeDef, &mxInputName,
                        &mxInputValue, &mxInputType);
        mxNode->setInputValue(mxInputName, mxInputValue, mxInputType);

        // If this is a MaterialX Texture node
        if (mxNodeCategory == "image" || mxNodeCategory == "tiledimage") {

            // Save the corresponding MaterialX and Hydra names for ShaderGen
            (*mxHdTextureMap)[hdNodePath.GetName()] = connectionName;

            // Save the path to adjust the parameters after traversing the network
            hdTextureNodes->insert(hdNodePath);
        }
    }
    return mxNode;
}

// Recurrsively traverse the HdNetwork and gather the nodes in the MaterialX
// NodeGraph and Document
static void
_GatherUpstreamNodes(
    HdMaterialNetwork2 const& hdNetwork,
    HdMaterialConnection2 const& hdConnection,  // connection from previous node
    mx::DocumentPtr const& mxDoc,
    mx::NodeGraphPtr * mxNodeGraph,
    mx::StringSet * addedNodeNames,
    mx::NodePtr * mxUpstreamNode,
    std::set<SdfPath> * hdTextureNodes,
    std::string const& connectionName,
    mx::StringMap * mxHdTextureMap)
{
    // Get the connected node (hdNode) from the hdConnection
    SdfPath hdNodePath;
    HdMaterialNode2 hdNode;  // HdNode -> mxCurrNode
    bool found = _FindConnectedNode(hdNetwork, hdConnection,
                                    &hdNode, &hdNodePath);
    
    if (!found) {
        TF_WARN("Could not find the connected Node with path '%s'", 
                    hdConnection.upstreamNode.GetText());
        return;
    }

    // Initilize the mxNodeGraph if needed
    if (!(*mxNodeGraph)) {
        const std::string & nodeGraphName  = hdNodePath.GetParentPath().GetName();
        *mxNodeGraph = mxDoc->addNodeGraph(nodeGraphName);
    }
    
    // Add the node to the mxNodeGraph/mxDoc.
    mx::NodePtr mxCurrNode = _AddMaterialXNode(hdNetwork, hdNode, hdNodePath, 
                                mxDoc, *mxNodeGraph, addedNodeNames, 
                                hdTextureNodes, connectionName, mxHdTextureMap);

    if (!mxCurrNode) {
        return;
    }

    // Continue traversing the upsteam connections to create the mxNodeGraph
    for (auto & inputConnections: hdNode.inputConnections) {

        TfToken connName = inputConnections.first;
        for (auto & currConnection : inputConnections.second) {

            // Gather the nodes uptream from the mxCurrNode
            _GatherUpstreamNodes(hdNetwork, currConnection, mxDoc, mxNodeGraph,
                                 addedNodeNames, mxUpstreamNode, hdTextureNodes, 
                                 connName.GetString(), mxHdTextureMap);

            // Connect mxCurrNode to the mxUpstreamNode
            mx::NodePtr mxNextNode = *mxUpstreamNode;
            mx::InputPtr mxInput = mxCurrNode->addInput(connName,
                                                        mxNextNode->getType());
            mxInput->setConnectedNode(mxNextNode);
        }
    }

    *mxUpstreamNode = mxCurrNode;
}

// Convert the HdMaterialNetwork to a MaterialX Document that is used for
// the MaterialX ShaderGen
static mx::DocumentPtr 
_CreateMtlxDocumentFromHdNetwork(
    HdMaterialNetwork2 const& hdNetwork,
    HdMaterialNode2 const& hdMaterialXNode,
    SdfPath const& materialPath,
    mx::DocumentPtr libraries,
    std::set<SdfPath> * hdTextureNodes,
    mx::StringMap * mxHdTextureMap)
{
    // Initialize a MaterialX Document
    mx::DocumentPtr mxDoc = mx::createDocument();
    mxDoc->importLibrary(libraries);
    
    // Create a material that instantiates the shader
    const std::string & materialName = materialPath.GetName();
    TfToken mxType = _GetMxNodeType(hdMaterialXNode.nodeTypeId);
    mx::NodePtr mxShaderNode = mxDoc->addNode(mxType.GetString(),
                                              "SR_" + materialName,
                                              "surfaceshader");
    mx::NodePtr mxMaterial = mxDoc->addMaterialNode(materialName, mxShaderNode);

    // Create mxNodeGraph from the inputConnections in the HdMaterialNetwork
    mx::NodeGraphPtr mxNodeGraph;
    mx::StringSet addedNodeNames;   // Set of NodeNames in the mxNodeGraph
    for (auto inputConns : hdMaterialXNode.inputConnections) {

        const std::string & mxNodeGraphOutput = inputConns.first.GetString();
        for (HdMaterialConnection2 & currConnection : inputConns.second) {

            // Gather the nodes uptream from the hdMaterialXNode
            mx::NodePtr mxUpstreamNode;
            _GatherUpstreamNodes(hdNetwork, currConnection, mxDoc, &mxNodeGraph,
                        &addedNodeNames, &mxUpstreamNode, hdTextureNodes, 
                        mxNodeGraphOutput, mxHdTextureMap);

            if (!mxUpstreamNode) {
                continue;
            }

            // Connect currNode to the upstream Node
            std::string fullOutputName = mxNodeGraphOutput + "_" +
                                currConnection.upstreamOutputName.GetString();
            mx::OutputPtr mxOutput = mxNodeGraph->addOutput(fullOutputName, 
                                                    mxUpstreamNode->getType());
            mxOutput->setConnectedNode(mxUpstreamNode);

            // Connect NodeGraph Output to the ShaderNode
            mx::InputPtr mxInput = mxShaderNode->addInput(mxNodeGraphOutput,
                                                          mxOutput->getType());
            mxInput->setConnectedOutput(mxOutput);
        }
    }

    // Add Inputs - The StandardSurface or USDPreviewSurface inputs
    for (auto currParameter : hdMaterialXNode.parameters) {

        const std::string & mxInputName = currParameter.first.GetString();
        mx::InputPtr mxInput = mxShaderNode->addInput(mxInputName);
        
        // Convert the parameter to the appropriate MaterialX input format
        VtValue hdParamValue = currParameter.second;
        if (hdParamValue.IsHolding<bool>()) {
            bool value = hdParamValue.UncheckedGet<bool>();
            mxInput->setValue(value);
        }
        else if (hdParamValue.IsHolding<int>()) {
            int value = hdParamValue.UncheckedGet<int>();
            mxInput->setValue(value);
        }
        else if (hdParamValue.IsHolding<float>()) {
            float value = hdParamValue.UncheckedGet<float>();
            mxInput->setValue(value);
        }
        else if (hdParamValue.IsHolding<GfVec3f>()) {

            const GfVec3f & value = hdParamValue.UncheckedGet<GfVec3f>();
            // Check if the parameter is a mx::vector3 or mx::color3
            if (_IsInputVector3(mxInputName)) {
                mxInput->setValue(mx::Vector3(value.data()[0], 
                                              value.data()[1], 
                                              value.data()[2]));
            }
            else {
                mxInput->setValue(mx::Color3(value.data()[0], 
                                             value.data()[1], 
                                             value.data()[2]));
            }
        }
        else {
            mxShaderNode->removeInput(mxInputName);
            TF_WARN("Unsupported Input Type '%s' for mxNode '%s' of type '%s'",
                    hdParamValue.GetTypeName().c_str(), mxInputName.c_str(), 
                    mxType.GetText());
        }
    }

    // Validate the MaterialX Document.
    std::string message;
    if (!mxDoc->validate(&message)) {
        TF_WARN("Validation warnings for generated MaterialX file.\n%s\n", 
                message.c_str());
    }

    return mxDoc;
}



void
HdSt_ApplyMaterialXFilter(
    HdMaterialNetwork2 * hdNetwork,
    SdfPath const& materialPath,
    HdMaterialNode2 const& terminalNode,
    SdfPath const& terminalNodePath)
{
    // Check if the Terminal is a MaterialX Node
    SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
    const SdrShaderNodeConstPtr mtlxSdrNode = 
        sdrRegistry.GetShaderNodeByIdentifierAndType(terminalNode.nodeTypeId, 
                                                     _tokens->mtlx);

    if (mtlxSdrNode) {

        // Load Standard Libraries/setup SearchPaths (for mxDoc and mxShaderGen)
        mx::FilePathVec libraryFolders = { "libraries", };
        mx::FileSearchPath searchPath;
        searchPath.append(mx::FilePath(PXR_MATERIALX_STDLIB_DIR));
        searchPath.append(mx::FilePath(PXR_MATERIALX_RESOURCE_DIR));
        mx::DocumentPtr stdLibraries = mx::createDocument();
        mx::loadLibraries(libraryFolders, searchPath, stdLibraries);

        // Create the MaterialX Document from the HdMaterialNetwork
        std::set<SdfPath> hdTextureNodes;
        mx::StringMap mxHdTextureMap; // Store Mx-Hd texture counterparts 
        mx::DocumentPtr mtlxDoc = _CreateMtlxDocumentFromHdNetwork(*hdNetwork,
                                        terminalNode,   // MaterialX HdNode
                                        materialPath,
                                        stdLibraries,
                                        &hdTextureNodes,
                                        &mxHdTextureMap);

        // Add Hydra parameters for each of the Texture nodes
        _AddHdTextureNodeParameters(hdNetwork, hdTextureNodes);

        // Load MaterialX Document and generate the glslfxSource
        std::string glslfxSource = HdSt_GenMaterialXShaderCode(mtlxDoc, 
                                        searchPath, mxHdTextureMap);

        // Create a new terminal node with the new glslfxSource
        SdrShaderNodeConstPtr sdrNode = 
            sdrRegistry.GetShaderNodeFromSourceCode(glslfxSource, 
                                                    HioGlslfxTokens->glslfx,
                                                    NdrTokenMap()); // metadata
        HdMaterialNode2 newTerminalNode;
        newTerminalNode.nodeTypeId = sdrNode->GetIdentifier();
        newTerminalNode.inputConnections = terminalNode.inputConnections;

        // Replace the original terminalNode with this newTerminalNode
        hdNetwork->nodes[terminalNodePath] = newTerminalNode;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE