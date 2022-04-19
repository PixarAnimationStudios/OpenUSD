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

#include "pxr/base/arch/hash.h"
#include "pxr/base/arch/library.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/sdr/registry.h"

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/materialNetwork2Interface.h"
#include "pxr/imaging/hdMtlx/hdMtlx.h"

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


// Use the given mxDocument to generate osl source code for the node from the 
// nodeGraph with the given names.
static std::string
_GenMaterialXShaderCode(
    mx::DocumentPtr const &mxDoc,
    mx::FileSearchPath const &searchPath,
    std::string const &shaderName,
    std::string const &mxNodeName,
    std::string const &mxNodeGraphName)
{
    // Initialize the Context for shaderGen
    mx::GenContext mxContext = mx::OslShaderGenerator::create();
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
    mxContext.getOptions().fileTextureVerticalFlip = false;

    // Get the Node from the Nodegraph/mxDoc 
    mx::NodeGraphPtr mxNodeGraph = mxDoc->getNodeGraph(mxNodeGraphName);
    if (!mxNodeGraph) {
        TF_WARN("NodeGraph '%s' not found in the mxDoc.",
                mxNodeGraphName.c_str());
        return mx::EMPTY_STRING;
    }
    mx::NodePtr mxNode = mxNodeGraph->getNode(mxNodeName);
    if (!mxNode) {
        TF_WARN("Node '%s' not found in '%s' nodeGraph.",
                mxNodeName.c_str(), mxNodeGraphName.c_str());
        return mx::EMPTY_STRING;
    }

    // Generate the OslShader for the Node
    TF_DEBUG(HDPRMAN_MATERIALS)
        .Msg("Generate a MaterialX Osl shader for '%s' node.\n", 
             mxNodeName.c_str());
    mx::ShaderPtr mxShader = mx::createShader(shaderName, mxContext, mxNode);
    if (mxShader) {
        return mxShader->getSourceCode();
    }
    TF_WARN("Unable to create Shader for node '%s'.", mxNodeName.c_str());
    return mx::EMPTY_STRING;
}

////////////////////////////////////////////////////////////////////////////////
// Helpers to update the material network for HdPrman 

// Convert the MaterialX SurfaceShader Token to the MaterialX Adapter Node Type
static TfToken
_GetAdapterNodeType(TfToken const &hdNodeType)
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
_GetUpdatedInputToken(TfToken const &currInputName)
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

static bool
_HasNode(
    HdMaterialNetworkInterface *netInterface,
    TfToken const &nodeName)
{
    return !netInterface->GetNodeType(nodeName).IsEmpty();
}

static void 
_GatherNodeGraphNodes(
    HdMaterialNetworkInterface *netInterface,
    TfToken const &hdNodeName,
    std::set<TfToken> *upstreamNodeNames,
    std::set<TfToken> *visitedNodeNames)
{
     TfTokenVector cNames =
        netInterface->GetNodeInputConnectionNames(hdNodeName);

    // Traverse the upsteam connections to gather the nodeGraph nodes
    for (TfToken const &cName : cNames) {
        auto inputConnections =
            netInterface->GetNodeInputConnection(hdNodeName, cName);

        for (auto const &currConnection : inputConnections) {
            TfToken const &upstreamNodeName = currConnection.upstreamNodeName;

            if (!_HasNode(netInterface, upstreamNodeName)) {
                TF_WARN("Unknown material node '%s'",
                         upstreamNodeName.GetText());
                continue;
            }
            if (visitedNodeNames->count(upstreamNodeName) > 0) {
                continue;
            }
            visitedNodeNames->insert(upstreamNodeName);

            // Gather the nodes uptream from the hdNode
            _GatherNodeGraphNodes(netInterface, upstreamNodeName, 
                                  upstreamNodeNames, visitedNodeNames);
            upstreamNodeNames->insert(upstreamNodeName);
        }
    }
}

// Compile the given oslSource returning the path to the compiled oso code 
static std::string 
_CompileOslSource(
    std::string const &name, 
    std::string const &oslSource,
    mx::FileSearchPath const &searchPaths)
{
#ifdef PXR_OSL_SUPPORT_ENABLED

    TF_DEBUG(HDPRMAN_DUMP_MATERIALX_OSL_SHADER)
        .Msg("--------- MaterialX Generated Shader '%s' ----------\n%s"
             "---------------------------\n\n", name.c_str(), oslSource.c_str());

    // Include the filepath to the MaterialX OSL directory (stdlib/osl)
    std::vector<std::string> oslArgs;
    oslArgs.reserve(searchPaths.size());
    const mx::FilePath stdlibOslPath = "stdlib/osl";
    for (mx::FilePath const &path : searchPaths) {
        const mx::FilePath fullPath = path/stdlibOslPath;
        oslArgs.push_back(fullPath.exists() ? "-I" + fullPath.asString()
                                            : "-I" + path.asString());
    }

    // Compile oslSource
    std::string oslCompiledSource;
    OSL::OSLCompiler oslCompiler;
    oslCompiler.compile_buffer(oslSource, oslCompiledSource, oslArgs);

    // Save compiled shader
    std::string compiledFilePath = ArchMakeTmpFileName("MX." + name, ".oso");
    FILE *compiledShader;
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

static void
_DeleteAllInputConnections(
    HdMaterialNetworkInterface *netInterface,
    TfToken const &nodeName)
{
    TfTokenVector cNames = netInterface->GetNodeInputConnectionNames(nodeName);
    for (const TfToken &cName : cNames) {
        netInterface->DeleteNodeInputConnection(nodeName, cName);
    }
}

static void
_DeleteAllParameters(
    HdMaterialNetworkInterface *netInterface,
    TfToken const &nodeName)
{
    TfTokenVector pNames =
        netInterface->GetAuthoredNodeParameterNames(nodeName);
    for (const TfToken &pName : pNames) {
        netInterface->DeleteNodeParameter(nodeName, pName);
    }
    
}

// For each of the outputs in the nodegraph create a sdrShaderNode with the
// compiled osl code generated by MaterialX and update the terminalNode's 
// input connections
// Removes the nodes that are not directly connected to the terminal node
static void
_UpdateNetwork(
    HdMaterialNetworkInterface *netInterface,
    TfToken const &terminalNodeName,
    mx::DocumentPtr const &mxDoc,
    mx::FileSearchPath const &searchPath)
{
    // Gather the nodeGraph nodes
    std::set<TfToken> nodesToKeep;   // nodes directly connected to the terminal
    std::set<TfToken> nodesToRemove; // nodes further removed from the terminal
    std::set<TfToken> visitedNodeNames;

    TfTokenVector terminalConnectionNames =
        netInterface->GetNodeInputConnectionNames(terminalNodeName);

    for (TfToken const &cName : terminalConnectionNames) {
        auto inputConnections =
            netInterface->GetNodeInputConnection(terminalNodeName, cName);

        for (auto const &currConnection : inputConnections) {
            TfToken const &upstreamNodeName = currConnection.upstreamNodeName;
            TfToken const &outputName = currConnection.upstreamOutputName;

            if (!_HasNode(netInterface, upstreamNodeName)) {
                TF_WARN("Unknown material node '%s'",
                         upstreamNodeName.GetText());
                continue;
            }
            bool newNode = visitedNodeNames.count(upstreamNodeName) == 0;
            if (!newNode) {
                // Re-using a node or node output, get the corresponding sdrNode
                SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
                SdrShaderNodeConstPtr sdrNode = 
                    sdrRegistry.GetShaderNodeByIdentifier(
                        netInterface->GetNodeType(upstreamNodeName));

                // Update the connection into the terminal node so that the
                // output makes it into the closure
                TfToken const &inputName = cName;
                if (sdrNode->GetOutput(outputName)) {
                    netInterface->SetNodeInputConnection(
                        terminalNodeName,
                        inputName,
                        { {upstreamNodeName, outputName} });
                }
                else {
                    TF_WARN("Output '%s' not found on node '%s'.",
                            outputName.GetText(), upstreamNodeName.GetText());
                }
                continue;
            }
            
            visitedNodeNames.insert(upstreamNodeName);
            // Collect nodes further removed from the terminal in nodesToRemove
            _GatherNodeGraphNodes(netInterface, upstreamNodeName, 
                                  &nodesToRemove, &visitedNodeNames);
            nodesToKeep.insert(upstreamNodeName);

            // Generate the oslSource code for the connected upstream node
            SdfPath const nodePath = SdfPath(upstreamNodeName);
            std::string const &mxNodeName = nodePath.GetName();
            std::string const &mxNodeGraphName =
                nodePath.GetParentPath().GetName();
            std::string shaderName = mxNodeName + "Shader";
            std::string oslSource = _GenMaterialXShaderCode(
                mxDoc, searchPath, shaderName, mxNodeName, mxNodeGraphName);
            
            if (oslSource.empty()) {
                continue;
            }

            // Compile the oslSource
            std::string compiledShaderPath = 
                _CompileOslSource(shaderName, oslSource, searchPath);
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

            // Update node type to that of the Sdr node.
            netInterface->SetNodeType(
                upstreamNodeName, sdrNode->GetIdentifier());

            // Update the connection into the terminal node so that the 
            // nodegraph outputs make their way into the closure
            if (sdrNode->GetOutput(outputName)) {
                TfToken inputName = cName;
                TfToken updatedInputName = _GetUpdatedInputToken(inputName);
                bool deletePreviousConnection = false;
                if (updatedInputName != TfToken()) {
                    inputName = updatedInputName;
                    deletePreviousConnection = true;
                }
                netInterface->SetNodeInputConnection(
                    terminalNodeName, inputName,
                    { {upstreamNodeName, outputName} });
                if (deletePreviousConnection) {
                    netInterface->DeleteNodeInputConnection(
                        terminalNodeName, cName);
                }
            }
            _DeleteAllInputConnections(netInterface, upstreamNodeName);
            _DeleteAllParameters(netInterface, upstreamNodeName);
        }
    }

    // Remove the nodes not directly connected to the terminal
    for (const TfToken& nodeName: nodesToRemove) {
        // As long as the node is not also directly connected to the terminal
        if (nodesToKeep.find(nodeName) == nodesToKeep.end()) {
            netInterface->DeleteNode(nodeName);
        }
    }
}

// Transform the original terminalNode with an Adapter Node which connects to a
// new PxrSurface Node that becomes the surfaceTerminal
// node in the hdNetwork.
static void 
_TransformTerminalNode(
    HdMaterialNetworkInterface *netInterface,
    TfToken const &terminalNodeName)
{
    // Create a SdrShaderNodes for the Adapter and PxrSurface Nodes.
    TfToken adapterType = _GetAdapterNodeType(
                            netInterface->GetNodeType(terminalNodeName));

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

    // Transform the terminalNode with the appropriate Adapter Node, which
    // translates the MaterialX parameters into PxrSurface Node inputs.
    netInterface->SetNodeType(terminalNodeName, adapterType);
    if (adapterType != _tokens->USD_Adapter) {
        // Update the TfTokens associated with the input parameters to the
        // Standard Surface Adapter Node that conflict with OSL reserved words. 
        // The corresponding input connection is updated in _UpdateNetwork()
        TfTokenVector pNames =
            netInterface->GetAuthoredNodeParameterNames(terminalNodeName);
        for (TfToken const &pName : pNames) {
            TfToken updatedName = _GetUpdatedInputToken(pName);
            if (!updatedName.IsEmpty()) {
                VtValue val = netInterface->GetNodeParameterValue(
                                                terminalNodeName, pName);
                netInterface->SetNodeParameterValue(
                    terminalNodeName, updatedName, val);
                netInterface->DeleteNodeParameter(terminalNodeName, pName);
            }
        }
    }
    
    // Create a PxrSurface material node
    TfToken pxrSurfaceNodeName =
        TfToken(terminalNodeName.GetString() + "_PxrSurface");
    netInterface->SetNodeType(pxrSurfaceNodeName, _tokens->PxrSurface);

    // Connect the PxrSurface inputs to the Adapter's outputs
    for (const auto& inParamName: sdrPxrSurface->GetInputNames()) {

        if (sdrPxrSurface->GetShaderInput(inParamName)) {

            // Convert the parameter name to the "xxxOut" format
            TfToken adapterOutParam = TfToken(inParamName.GetString() + "Out");
            
            // If the PxrSurface Input is an Adapter node output add the
            // inputConnection to the PxrSurface Node
            // Note: not every input has a corresponding output
            if (sdrAdapter->GetShaderOutput(adapterOutParam)) {
                netInterface->SetNodeInputConnection(
                    pxrSurfaceNodeName, inParamName, 
                    { {terminalNodeName, adapterOutParam} });
            }
        }
    }

    // Update the network terminals so that the terminal Node is the PxrSurface
    // Node instead of the Adapter Node (previously the mtlx terminal node)
    netInterface->SetTerminalConnection(
        HdMaterialTerminalTokens->surface, { pxrSurfaceNodeName, TfToken() });
}

// Get the Hydra equivalent for the given MaterialX input value
static TfToken
_GetHdWrapString(
    TfToken const &hdTextureNodeName,
    std::string const &mxInputValue)
{
    if (mxInputValue == "constant") {
        TF_WARN("RtxHioImagePlugin: Texture '%s' has unsupported wrap mode "
            "'constant' using 'black' instead.", hdTextureNodeName.GetText());
        return _tokens->black;
    }
    if (mxInputValue == "clamp") {
        return _tokens->clamp;
    }
    if (mxInputValue == "mirror") {
        TF_WARN("RtxHioImagePlugin: Texture '%s' has unsupported wrap mode "
            "'mirror' using 'repeat' instead.", hdTextureNodeName.GetText());
        return _tokens->repeat;
    }
    return _tokens->repeat;
}
      
static void
_GetWrapModes(
    HdMaterialNetworkInterface *netInterface,
    TfToken const &hdTextureNodeName,
    TfToken *uWrap,
    TfToken *vWrap)
{
    // For <tiledimage> nodes want to always use "repeat"
    *uWrap = _tokens->repeat;
    *vWrap = _tokens->repeat;

    // For <image> nodes:
    VtValue vUAddrMode = netInterface->GetNodeParameterValue(
                                    hdTextureNodeName, _tokens->uaddressmode);
    if (!vUAddrMode.IsEmpty()) {
        *uWrap = _GetHdWrapString(hdTextureNodeName, 
                                  vUAddrMode.UncheckedGet<std::string>());
    }
    VtValue vVAddrMode = netInterface->GetNodeParameterValue(
                                    hdTextureNodeName, _tokens->vaddressmode);
    if (!vVAddrMode.IsEmpty()) {
        *vWrap = _GetHdWrapString(hdTextureNodeName, 
                                  vVAddrMode.UncheckedGet<std::string>());
    }
}

static void 
_UpdateTextureNodes(
    HdMaterialNetworkInterface *netInterface,
    std::set<SdfPath> const &hdTextureNodePaths,
    mx::DocumentPtr const &mxDoc)
{
    for (SdfPath const &texturePath : hdTextureNodePaths) {
        TfToken const &textureNodeName = texturePath.GetToken();
        const TfToken nodeType = netInterface->GetNodeType(textureNodeName);
        if (nodeType.IsEmpty()) {
            TF_WARN("Connot find texture node '%s' in material network.",
                    textureNodeName.GetText());
            continue;
        }
        
        VtValue vFile =
            netInterface->GetNodeParameterValue(textureNodeName, _tokens->file);
        if (vFile.IsEmpty()) {
            TF_WARN("File path missing for texture node '%s'.",
                    textureNodeName.GetText());
            continue;
        }

        if (vFile.IsHolding<SdfAssetPath>()) {
            std::string path = vFile.Get<SdfAssetPath>().GetResolvedPath();
            std::string ext = ArGetResolver().GetExtension(path);

            // Update texture nodes that use non-native texture formats
            // to read them via a Renderman texture plugin.
            if (!ext.empty() && ext != "tex") {

                // Update the input value to use the Renderman texture plugin
                const std::string pluginName = 
                    std::string("RtxHioImage") + ARCH_LIBRARY_SUFFIX;

                TfToken uWrap, vWrap;
                _GetWrapModes(netInterface, textureNodeName, &uWrap, &vWrap);
                
                std::string const &mxInputValue = 
                    TfStringPrintf("rtxplugin:%s?filename=%s&wrapS=%s&wrapT=%s", 
                                    pluginName.c_str(), path.c_str(), 
                                    uWrap.GetText(), vWrap.GetText());
                TF_DEBUG(HDPRMAN_IMAGE_ASSET_RESOLVE)
                    .Msg("Resolved MaterialX asset path: %s\n",
                         mxInputValue.c_str());
                
                // Update the MaterialX Texture Node with the new mxInputValue
                const mx::NodeGraphPtr mxNodeGraph = 
                    mxDoc->getNodeGraph(texturePath.GetParentPath().GetName());
                const mx::NodePtr mxTextureNode = 
                    mxNodeGraph->getNode(texturePath.GetName());
                mxTextureNode->setInputValue(_tokens->file.GetText(), // name
                                             mxInputValue,            // value
                                             _tokens->filename.GetText());//type
            }
            else {
                TF_DEBUG(HDPRMAN_IMAGE_ASSET_RESOLVE)
                    .Msg("Resolved MaterialX asset path: %s\n",
                         path.c_str());
            }
        }
    }
}

void
MatfiltMaterialX(
    HdMaterialNetworkInterface *netInterface,
    std::vector<std::string> *outputErrorMessages)
{
    if (!netInterface) {
        return;
    }

    // Check presence of surface terminal
    const HdMaterialNetworkInterface::InputConnectionResult res =
        netInterface->GetTerminalConnection(HdMaterialTerminalTokens->surface);
    if (!res.first) { // "surface" terminal absent
        return;
    }
    const TfToken &terminalNodeName = res.second.upstreamNodeName;
    const TfToken terminalNodeType =
        netInterface->GetNodeType(terminalNodeName);
    
    // Check if the node connected to the terminal is a MaterialX node
    SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
    const SdrShaderNodeConstPtr mtlxSdrNode = 
        sdrRegistry.GetShaderNodeByIdentifierAndType(terminalNodeType, 
                                                     _tokens->mtlx);
    if (!mtlxSdrNode) {
        return;
    }

    TfTokenVector cNames =
        netInterface->GetNodeInputConnectionNames(terminalNodeName);
    // If we have a nodegraph (i.e., input into the terminal node)...
    if (!cNames.empty()) {
        // Load Standard Libraries/setup SearchPaths (for mxDoc and
        // mxShaderGen)
        mx::FilePathVec libraryFolders;
        mx::FileSearchPath searchPath = HdMtlxSearchPaths();
        mx::DocumentPtr stdLibraries = mx::createDocument();
        mx::loadLibraries(libraryFolders, searchPath, stdLibraries);

        // Create the MaterialX Document from the material network
        std::set<SdfPath> hdTextureNodePaths;
        mx::StringMap mxHdTextureMap; // Store Mx-Hd texture counterparts 
        mx::DocumentPtr mxDoc =
            HdMtlxCreateMtlxDocumentFromHdMaterialNetworkInterface(
                netInterface, terminalNodeName, cNames,
                stdLibraries, &hdTextureNodePaths, &mxHdTextureMap);
        
        _UpdateTextureNodes(netInterface, hdTextureNodePaths, mxDoc);

        // Remove the material and shader nodes from the MaterialX Document
        // (since we need to use PxrSurface as the closure instead of the 
        // MaterialX surfaceshader node)
        SdfPath materialPath = netInterface->GetMaterialPrimPath();
        mxDoc->removeNode("SR_" + materialPath.GetName());  // Shader Node
        mxDoc->removeNode(materialPath.GetName());          // Material Node

        // Update nodes directly connected to the terminal node with 
        // MX generated shaders that capture the rest of the nodegraph
        _UpdateNetwork(netInterface, terminalNodeName, mxDoc, searchPath);
    }

    // Convert the terminal node to an AdapterNode + PxrSurfaceNode
    _TransformTerminalNode(netInterface, terminalNodeName);
}

void
MatfiltMaterialX(
    const SdfPath &materialPath,
    HdMaterialNetwork2 &hdNetwork,
    const std::map<TfToken, VtValue> &contextValues,
    const NdrTokenVec &shaderTypePriority,
    std::vector<std::string> *outputErrorMessages)
{
    TF_UNUSED(contextValues);
    TF_UNUSED(shaderTypePriority);

    HdMaterialNetwork2Interface netInterface(materialPath, &hdNetwork);
    MatfiltMaterialX(&netInterface, outputErrorMessages);
}

PXR_NAMESPACE_CLOSE_SCOPE                        