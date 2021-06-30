//
// Copyright 2021 Pixar
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
#include "hdPrman/matfiltMaterialX.h"
#include "hdPrman/debugCodes.h"
#include "pxr/imaging/hdMtlx/hdMtlx.h"

#include "pxr/base/arch/hash.h"
#include "pxr/base/arch/library.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix4d.h"

#include <MaterialXCore/Node.h>
#include <MaterialXCore/Document.h>
#include <MaterialXFormat/Util.h>
#include <MaterialXFormat/XmlIo.h>
#include <MaterialXGenShader/Shader.h>
#include <MaterialXGenShader/Util.h>
#include <MaterialXGenOsl/OslShaderGenerator.h>
#include <MaterialXRender/Util.h>

#ifdef PXR_OSL_SUPPORT_ENABLED
#include <OSL/oslcomp.h>
#include <fstream>
#endif

namespace mx = MaterialX;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (mtlx)

    // Hydra MaterialX Node Types
    (ND_standard_surface_surfaceshader)
    (ND_UsdPreviewSurface_surfaceshader)

    // MaterialX - OSL Adapter Node names
    ((SS_Adapter, "StandardSurfaceParameters"))
    ((USD_Adapter, "UsdPreviewSurfaceParameters"))

    // HdPrman Surface Terminal Node
    (PxrSurface)

    // Hydra SourceTypes
    (OSL)       // Adapter Node
    (RmanCpp)   // PxrSurface Node

    // MaterialX Texture Node input and type
    (file)
    (filename)

    // Wrap Modes
    (black)
    (clamp)
    (repeat)
    (uaddressmode)
    (vaddressmode)
);


// Use the given mxDocument to generate osl source code for the nodegraph output 
// with the given hdOutputName
static std::string
_GenMaterialXShaderCode(
    mx::DocumentPtr const& mxDoc,
    mx::FileSearchPath const& searchPath,
    std::string const& hdOutputName,
    std::string const& shaderName)
{
    // Initialize the Context for shaderGen
    mx::GenContext mxContext = mx::OslShaderGenerator::create();
    mxContext.registerSourceCodeSearchPath(searchPath);
    mxContext.getOptions().fileTextureVerticalFlip = false;

    // Find renderable elements in the Mtlx Document (NodeGraph outputs)
    std::vector<mx::TypedElementPtr> renderableElements;
    mx::findRenderableElements(mxDoc, renderableElements);

    // Generate the OslShader for the matching hdOutput
    for (auto& nodeGraphOutputElement : renderableElements) {
        if (hdOutputName == nodeGraphOutputElement->getName()) {
            TF_DEBUG(HDPRMAN_MATERIALS)
                .Msg("Generate a MaterialX Osl shader for output '%s'\n", 
                     hdOutputName.c_str());
            mx::ShaderPtr mxShader = mx::createShader(shaderName, mxContext, 
                                                      nodeGraphOutputElement);
            if (mxShader) {
                return mxShader->getSourceCode();
            }
            return mx::EMPTY_STRING;
        }
    }
    TF_WARN("No matching output for '%s'\n", hdOutputName.c_str());
    return mx::EMPTY_STRING;
}

////////////////////////////////////////////////////////////////////////////////
// Helpers to update the HdMaterialNetwork for HdPrman 

static HdMaterialNode2 const*
_GetSurfaceTerminalNode(
    HdMaterialNetwork2 const& network,
    SdfPath * terminalNodePath)
{
    // Get the Surface Terminal
    auto const& terminalConnIt = network.terminals.find(
                                            HdMaterialTerminalTokens->surface);
    if (terminalConnIt == network.terminals.end()) {
        return nullptr;
    }
    HdMaterialConnection2 const& connection = terminalConnIt->second;
    SdfPath const& terminalPath = connection.upstreamNode;
    auto const& terminalIt = network.nodes.find(terminalPath);
    *terminalNodePath = terminalPath;
    return &terminalIt->second;
}

// Convert the MaterialX SurfaceShader Token to the MaterialX Adapter Node Type
static TfToken
_GetAdapterNodeType(TfToken const& hdNodeType)
{
    if (hdNodeType == _tokens->ND_standard_surface_surfaceshader) {
        return _tokens->SS_Adapter;
    } 
    else if (hdNodeType == _tokens->ND_UsdPreviewSurface_surfaceshader) {
        return _tokens->USD_Adapter;
    } 
    else {
        TF_WARN("Unsupported Node Type '%s'", hdNodeType.GetText());
        return TfToken();
    }
}

// Convert the TfToken associated with the input parameters to the Standard
// Surface Adapter Node that conflict with OSL reserved words. 
static TfToken
_GetUpdatedInputToken(TfToken const& currInputName)
{
    // { currentInputNname , updatedInputName }
    static const mx::StringMap conflicts = {{"emission",   "emission_value"},
                                            {"subsurface", "subsurface_value"},
                                            {"normal", "input_normal"}};
    auto it = conflicts.find(currInputName.GetString());
    if (it != conflicts.end()) {
        return TfToken(it->second);
    }
    return TfToken();
}

// Find the HdNode and its corresponding NodePath in the given HdNetwork 
// based on the given HdConnection
static bool 
_FindConnectedNode(
    HdMaterialNetwork2 const& hdNetwork,
    HdMaterialConnection2 const& hdConnection,
    HdMaterialNode2 * hdNode,
    SdfPath * hdNodePath,
    std::set<SdfPath> * visitedNodes)
{
    // Get the path to the connected node
    const SdfPath & connectionPath = hdConnection.upstreamNode;

    // Make sure we don't process the node again if we already visited it
    if (visitedNodes->count(connectionPath) > 0) {
        *hdNode = hdNetwork.nodes.find(connectionPath)->second;
        *hdNodePath = connectionPath;
        return false;
    }
    visitedNodes->insert(connectionPath);

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

static void 
_GatherNodeGraphNodes(
    HdMaterialNetwork2 const& hdNetwork,
    HdMaterialNode2 const& hdNode,
    std::set<SdfPath> * nodeGraphNodes,
    std::set<SdfPath> * visitedNodes)
{
    // Traverse the upsteam connections to gather the nodeGraph nodes
    for (auto & inputConnections: hdNode.inputConnections) {
        for (auto const& currConnection : inputConnections.second) {

            SdfPath nextNodePath;
            HdMaterialNode2 nextNode;
            const bool found = _FindConnectedNode(hdNetwork, currConnection, 
                                    &nextNode, &nextNodePath, visitedNodes);
            if (!found) {
                return;
            }
            // Gather the nodes uptream from the hdNode
            _GatherNodeGraphNodes(hdNetwork, nextNode, 
                                  nodeGraphNodes, visitedNodes);
            nodeGraphNodes->insert(nextNodePath);
        }
    }
}

// Compile the given oslSource returning the path to the compiled oso code 
static std::string 
_CompileOslSource(std::string const& name, std::string const& oslSource)
{
#ifdef PXR_OSL_SUPPORT_ENABLED

    TF_DEBUG(HDPRMAN_DUMP_MATERIALX_OSL_SHADER)
        .Msg("--------- MaterialX Generated Shader '%s' ----------\n%s"
             "---------------------------\n\n", name.c_str(), oslSource.c_str());

    // Compile oslSource
    std::string oslCompiledSource;
    OSL::OSLCompiler oslCompiler;
    oslCompiler.compile_buffer(oslSource, oslCompiledSource, 
                {"-I" + std::string(PXR_MATERIALX_STDLIB_DIR) + "/stdlib/osl"});

    // Save compiled shader
    std::string compiledFilePath = ArchMakeTmpFileName("MX." + name, ".oso");
    FILE * compiledShader;
    compiledShader = fopen((compiledFilePath).c_str(), "w+");
    if (!compiledShader) {
        TF_WARN("Unable to save compiled MaterialX Osl shader at '%s'\n",
                compiledFilePath.c_str());
        return mx::EMPTY_STRING;
    }
    else {
        fputs(oslCompiledSource.c_str(), compiledShader);
        fclose(compiledShader);
        return compiledFilePath;
    }
#else        
    TF_WARN("Unable to compile MaterialX generated Osl shader, enable OSL "
            "support for full MaterialX support in HdPrman.\n");
    return mx::EMPTY_STRING;
#endif
}

// For each of the outputs in the nodegraph create a sdrShaderNode with the
// compiled osl code generated by MaterialX and update the terminalNode's 
// input connections
// Removes the nodes that are not directly connected to the terminal node
static void
_UpdateNetwork(
    HdMaterialNetwork2 * hdNetwork,
    HdMaterialNode2 const* terminalNode,
    SdfPath const& terminalNodePath,
    mx::DocumentPtr const& mxDoc,
    mx::FileSearchPath const& searchPath)
{
    // Gather the nodeGraph nodes
    std::set<SdfPath> nodesToKeep;   // nodes directly connected to the terminal
    std::set<SdfPath> nodesToRemove; // nodes further removed from the terminal
    std::set<SdfPath> visitedNodes;

    // Save the terminalNode inputConnections and clear so that inputNames can 
    // be renamed if necessary
    auto terminalConnections = terminalNode->inputConnections;
    hdNetwork->nodes[terminalNodePath].inputConnections.clear();
    for (auto inputConns : terminalConnections) {

        const std::string & mxNodeGraphOutput = inputConns.first.GetString();
        for (HdMaterialConnection2 & currConnection : inputConns.second) {

            SdfPath level1NodePath;
            HdMaterialNode2 level1Node;
            bool newNode = _FindConnectedNode(*hdNetwork, currConnection, 
                                &level1Node, &level1NodePath, &visitedNodes);
            // If we did not find the node
            if (level1Node == HdMaterialNode2()) {
                continue;
            }
            // if we are connected to a node we have already processed (ie. we
            // are re-using a nodegraph output)
            if (!newNode) {
                // Get the sdrNode for this nodegraph output. 
                SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
                SdrShaderNodeConstPtr sdrNode = 
                    sdrRegistry.GetShaderNodeByIdentifier(
                        hdNetwork->nodes[level1NodePath].nodeTypeId);

                // Update the connection into the terminal node so that the
                // nodegraph output makes it into the closure
                // Note: MaterialX nodes only have one output
                TfToken inputName = TfToken(mxNodeGraphOutput);
                TfToken outputName = sdrNode->GetOutputNames().at(0);
                hdNetwork->nodes[terminalNodePath].inputConnections[inputName] 
                    = { {level1NodePath, outputName} };
                continue;
            }
            // collect nodes further removed from the terminal in nodesToRemove
            _GatherNodeGraphNodes(*hdNetwork, level1Node, 
                                  &nodesToRemove, &visitedNodes);
            nodesToKeep.insert(level1NodePath);

            // Generate the oslSource code for the output            
            std::string fullOutputName = mxNodeGraphOutput + "_" +
                                currConnection.upstreamOutputName.GetString();
            std::string shaderName = mxNodeGraphOutput + "Shader";
            std::string oslSource = _GenMaterialXShaderCode(mxDoc, 
                                        searchPath, fullOutputName, shaderName);
            if (oslSource.empty()) {
                continue;
            }

            // Compile the oslSource
            std::string compiledShaderPath = _CompileOslSource(shaderName, 
                                                               oslSource);
            if (compiledShaderPath.empty()) {
                continue;
            }

            // Create a new SdrShaderNode with the compiled oslSource
            SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
            SdrShaderNodeConstPtr sdrNode = 
                sdrRegistry.GetShaderNodeFromAsset(
                                SdfAssetPath(compiledShaderPath),
                                NdrTokenMap(),  // metadata
                                _tokens->mtlx,  // subId
                                _tokens->OSL);  // sourceType

            // Update NodeTypeId to point to this new sdrNode
            hdNetwork->nodes[level1NodePath].nodeTypeId = sdrNode->GetIdentifier();

            // Update the connection into the terminal node so that the 
            // nodegraph outputs make their way into the closure
            TfToken outputName(fullOutputName);
            if (sdrNode->GetOutput(outputName)) {
                TfToken inputName = TfToken(mxNodeGraphOutput);
                TfToken updatedInputName = _GetUpdatedInputToken(inputName);
                if (updatedInputName != TfToken()) {
                    inputName = updatedInputName;
                }
                hdNetwork->nodes[terminalNodePath].inputConnections[inputName] 
                    = { {level1NodePath, outputName} };
            }

            // Clear inputConnections since they will be removed (nodesToRemove)
            hdNetwork->nodes[level1NodePath].inputConnections.clear();
            // Clear parameters since they are already used in the generated shader
            hdNetwork->nodes[level1NodePath].parameters.clear();
        }
    }

    // Remove the nodes not directly connected to the terminal
    for (auto const& node: nodesToRemove) {
        // As long as the node is not also directly connected to the terminal
        if (nodesToKeep.find(node) == nodesToKeep.end()) {
            hdNetwork->nodes.erase(node);
        }
    }
}

// Update the TfTokens associated with the input parameters to the Standard
// Surface Adapter Node that conflict with OSL reserved words. 
// The corresponding input connection is updated in _UpdateNetwork()
static void
_UpdateTerminalConnections(HdMaterialNode2 * adapterNode)
{
    if (adapterNode->nodeTypeId == _tokens->USD_Adapter) {
        return;
    }

    for (auto param : adapterNode->parameters) {
        
        TfToken updatedName = _GetUpdatedInputToken(param.first);
        if (updatedName != TfToken()) {
            // Replace the conflicting parameter name with the updatedName
            adapterNode->parameters[updatedName] = param.second;
            adapterNode->parameters.erase(param.first);
        }
    }
}

// Replace the original HdMaterialNode2 MaterialX terminalNode to an Adapter 
// Node which connects to a new PxrSurface Node that becomes the surfaceTerminal
// node in the hdNetwork.
static void 
_UpdateTerminal(
    HdMaterialNetwork2 * hdNetwork,
    HdMaterialNode2 const& terminalNode,
    SdfPath const& terminalNodePath)
{
    // Create a SdrShaderNodes for the Adapter and PxrSurface Nodes.
    TfToken adapterType = _GetAdapterNodeType(terminalNode.nodeTypeId);

    SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
    SdrShaderNodeConstPtr const sdrAdapter = 
        sdrRegistry.GetShaderNodeByIdentifier(adapterType, {_tokens->OSL});
    SdrShaderNodeConstPtr const sdrPxrSurface = 
        sdrRegistry.GetShaderNodeByIdentifier(_tokens->PxrSurface, 
                                              {_tokens->RmanCpp});
    if (!sdrAdapter) {
        TF_WARN("No sdrAdater node of type '%s'", adapterType.GetText());
        return;
    }

    // Replace the terminalNode with the appropriate Adapter Node, which
    // translates the MaterialX parameters into PxrSurface Node inputs.
    HdMaterialNode2 adapterNode;
    adapterNode.nodeTypeId = adapterType;
    adapterNode.inputConnections = terminalNode.inputConnections;
    adapterNode.parameters = terminalNode.parameters;
    _UpdateTerminalConnections(&adapterNode);
    hdNetwork->nodes[terminalNodePath] = adapterNode;
    
    // Create a PxrSurface HdMaterialNode2
    HdMaterialNode2 pxrSurfaceNode;
    pxrSurfaceNode.nodeTypeId = _tokens->PxrSurface;

    // Connect the PxrSurface inputs to the Adapter's outputs
    for (const auto& inParamName: sdrPxrSurface->GetInputNames()) {

        if (sdrPxrSurface->GetShaderInput(inParamName)) {

            // Convert the parameter name to the "xxxOut" format
            TfToken adapterOutParam = TfToken(inParamName.GetString() + "Out");
            
            // If the PxrSurface Input is an Adapter node output add the
            // inputConnection to the PxrSurface Node
            // Note: not every input has a corresponding output
            if (sdrAdapter->GetShaderOutput(adapterOutParam)) {

                pxrSurfaceNode.inputConnections[inParamName] = 
                        { {terminalNodePath, adapterOutParam} };
            }
        }
    }

    // Add the PxrSurface Node to the network
    SdfPath pxrSurfacePath = terminalNodePath.GetParentPath().AppendChild(
                        TfToken(terminalNodePath.GetName() + "_PxrSurface"));
    hdNetwork->nodes[pxrSurfacePath] = pxrSurfaceNode;

    // Update the network terminals so that the terminal Node is the PxrSurface
    // Node instead of the Adapter Node (previously the mtlx terminal node)
    hdNetwork->terminals[HdMaterialTerminalTokens->surface] = 
                        { pxrSurfacePath, TfToken() };
}

// Get the Hydra equivalent for the given MaterialX input value
static TfToken
_GetHdWrapString(
    SdfPath const& hdTextureNodePath,
    std::string const& mxInputValue)
{
    if (mxInputValue == "constant") {
        TF_WARN("RtxHioImagePlugin: Texture '%s' has unsupported wrap mode "
            "'constant' using 'black' instead.",
            hdTextureNodePath.GetName().c_str());
        return _tokens->black;
    }
    if (mxInputValue == "clamp") {
        return _tokens->clamp;
    }
    if (mxInputValue == "mirror") {
        TF_WARN("RtxHioImagePlugin: Texture '%s' has unsupported wrap mode "
            "'mirror' using 'repeat' instead.",
            hdTextureNodePath.GetName().c_str());
        return _tokens->repeat;
    }
    return _tokens->repeat;
}

static void
_GetWrapModes(
    HdMaterialNode2 const& hdTextureNode,
    SdfPath const& hdTextureNodePath,
    TfToken * uWrap,
    TfToken * vWrap)
{
    // For <tiledimage> nodes want to always use "repeat"
    *uWrap = _tokens->repeat;
    *vWrap = _tokens->repeat;

    // For <image> nodes:
    const auto uWapIt = hdTextureNode.parameters.find(_tokens->uaddressmode);
    if (uWapIt != hdTextureNode.parameters.end()) {
        *uWrap = _GetHdWrapString(hdTextureNodePath, 
                                  uWapIt->second.UncheckedGet<std::string>());
    }
    const auto vWrapIt = hdTextureNode.parameters.find(_tokens->vaddressmode);
    if (vWrapIt != hdTextureNode.parameters.end()) {
        *vWrap = _GetHdWrapString(hdTextureNodePath, 
                                  vWrapIt->second.UncheckedGet<std::string>());
    }
}

static void 
_UpdateTextureNodes(
    HdMaterialNetwork2 const& hdNetwork,
    std::set<SdfPath> const& hdTextureNodes,
    mx::DocumentPtr const& mxDoc)
{
    for (SdfPath const& texturePath : hdTextureNodes) {

        // Get the hdTextureNode from the hdNetwork
        const auto textureNodeIt = hdNetwork.nodes.find(texturePath);
        if (textureNodeIt == hdNetwork.nodes.end()) {
            TF_WARN("Connot find texture node '%s' in HdMaterialNetwork.",
                    texturePath.GetText());
            continue;
        }
        const HdMaterialNode2& hdTextureNode = textureNodeIt->second;

        // Get the filepath 
        const auto fileIt = hdTextureNode.parameters.find(_tokens->file);
        if (fileIt == hdTextureNode.parameters.end()) {
            TF_WARN("File path missing for texture node '%s'.",
                    texturePath.GetText());
            continue;
        }
        VtValue const& pathValue = fileIt->second;

        if (pathValue.IsHolding<SdfAssetPath>()) {
            std::string path = pathValue.Get<SdfAssetPath>().GetResolvedPath();
            std::string ext = ArGetResolver().GetExtension(path);

            // Update texture nodes that use non-native texture formats
            // to read them via a Renderman texture plugin.
            if (!ext.empty() && ext != "tex") {

                // Update the input value to use the Renderman texture plugin
                const std::string pluginName = 
                    std::string("RtxHioImage") + ARCH_LIBRARY_SUFFIX;

                TfToken uWrap, vWrap;
                _GetWrapModes(hdTextureNode, texturePath, &uWrap, &vWrap);
                
                std::string const& mxInputValue = 
                    TfStringPrintf("rtxplugin:%s?filename=%s&wrapS=%s&wrapT=%s", 
                                    pluginName.c_str(), path.c_str(), 
                                    uWrap.GetText(), vWrap.GetText());
                
                // Update the MaterialX Texture Node with the new mxInputValue
                const mx::NodeGraphPtr mxNodeGraph = 
                    mxDoc->getNodeGraph(texturePath.GetParentPath().GetName());
                const mx::NodePtr mxTextureNode = 
                    mxNodeGraph->getNode(texturePath.GetName());
                mxTextureNode->setInputValue(_tokens->file.GetText(), // name
                                             mxInputValue,            // value
                                             _tokens->filename.GetText());//type
            }
        }
    }
}

void
MatfiltMaterialX(
    const SdfPath & materialPath,
    HdMaterialNetwork2 & hdNetwork,
    const std::map<TfToken, VtValue> & contextValues,
    const NdrTokenVec & shaderTypePriority,
    std::vector<std::string> * outputErrorMessages)
{
    // Get the surface terminal node 
    SdfPath terminalNodePath;
    HdMaterialNode2 const* terminalNode = _GetSurfaceTerminalNode(hdNetwork, 
                                                &terminalNodePath);
    // Return if there is no surface terminal
    if (!terminalNode) {
        return;
    }

    // Check if the surface terminal is a MaterialX Node
    SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
    const SdrShaderNodeConstPtr mtlxSdrNode = 
        sdrRegistry.GetShaderNodeByIdentifierAndType(terminalNode->nodeTypeId, 
                                                     _tokens->mtlx);

    if (mtlxSdrNode) {

        // if we have a nodegraph (ie. input into the terminal node)
        if (!terminalNode->inputConnections.empty()) {

            // Load Standard Libraries/setup SearchPaths (for mxDoc and mxShaderGen)
            mx::FilePathVec libraryFolders = { "libraries", };
            mx::FileSearchPath searchPath;
            searchPath.append(mx::FilePath(PXR_MATERIALX_STDLIB_DIR));
            searchPath.append(mx::FilePath(PXR_MATERIALX_BASE_DIR));
            mx::DocumentPtr stdLibraries = mx::createDocument();
            mx::loadLibraries(libraryFolders, searchPath, stdLibraries);

            // Create the MaterialX Document from the HdMaterialNetwork 
            std::set<SdfPath> hdTextureNodes;
            mx::StringMap mxHdTextureMap; // Store Mx-Hd texture counterparts 
            mx::DocumentPtr mxDoc = HdMtlxCreateMtlxDocumentFromHdNetwork(
                                        hdNetwork, *terminalNode, 
                                        materialPath, stdLibraries, 
                                        &hdTextureNodes, &mxHdTextureMap);

            _UpdateTextureNodes(hdNetwork, hdTextureNodes, mxDoc);

            // Remove the material and shader nodes from the MaterialX Document
            // (since we need to use PxrSurface as the closure instead of the 
            // MaterialX surfaceshader node)
            mxDoc->removeNode("SR_" + materialPath.GetName());  // Shader Node
            mxDoc->removeNode(materialPath.GetName());          // Material Node

            // Update nodes directly connected to the terminal node with 
            // MX generated shaders that capture the rest of the nodegraph
            _UpdateNetwork(&hdNetwork, terminalNode, terminalNodePath, 
                           mxDoc, searchPath);
        }
        // Convert the terminal node to an AdapterNode + PxrSurfaceNode
        _UpdateTerminal(&hdNetwork, *terminalNode, terminalNodePath);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE