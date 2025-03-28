//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/matfiltMaterialX.h"
#include "hdPrman/debugCodes.h"

#include "pxr/base/arch/hash.h"
#include "pxr/base/arch/library.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/envSetting.h"
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
#ifdef PXR_DCC_LOCATION_ENV_VAR
#include "hdPrman/hdMtlx.h" // copied from pxr/imaging/hdMtlx, this has fixes
#else
#include "pxr/imaging/hdMtlx/hdMtlx.h"
#endif

#include <MaterialXCore/Node.h>
#include <MaterialXCore/Document.h>
#include <MaterialXFormat/Environ.h>
#include <MaterialXFormat/Util.h>
#include <MaterialXFormat/XmlIo.h>
#include <MaterialXGenShader/DefaultColorManagementSystem.h>
#include <MaterialXGenShader/Shader.h>
#include <MaterialXGenShader/Util.h>
#include <MaterialXGenOsl/OslShaderGenerator.h>
#include <MaterialXRender/Util.h>

namespace mx = MaterialX;

PXR_NAMESPACE_OPEN_SCOPE

#if PXR_VERSION < 2505
using SdrTokenMap = NdrTokenMap;
#endif

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (mtlx)

    // Hydra MaterialX Node Types
    (ND_standard_surface_surfaceshader)
    (ND_UsdPreviewSurface_surfaceshader)
    (ND_UsdPreviewSurface)
    (ND_displacement_float)
    (ND_displacement_vector3)
    (ND_image_vector2)
    (ND_image_vector3)
    (ND_image_vector4)

    (ND_surface)
    (ND_oren_nayar_diffuse_bsdf)
    (MaterialXOrenNayarDiffuse)
    (ND_generalized_schlick_bsdf)
    (MaterialXGeneralizedSchlick)
    (ND_burley_diffuse_bsdf)
    (MaterialXBurleyDiffuse)
    (ND_dielectric_bsdf)
    (MaterialXDielectric)
    (ND_sheen_bsdf)
    (MaterialXSheen)

    // MaterialX - OSL Adapter Node names
    ((SS_Adapter, "StandardSurfaceParameters"))
    ((USD_Adapter, "UsdPreviewSurfaceParameters"))
    ((Displacement_Adapter, "DisplacementParameters"))

    // HdPrman Terminal Nodes
    (PxrSurface)
    (PxrDisplace)

    // Texture Coordinate Tokens
    (ND_geompropvalue_vector2)
    (ND_separate2_vector2)
    (ND_floor_float)
    (ND_multiply_float)
    (ND_add_float)
    (ND_subtract_float)
    (ND_combine2_vector2)
    (separate2)
    (floor)
    (multiply)
    (add)
    (subtract)
    (combine2)
    (texcoord)
    (geomprop)
    (geompropvalue)
    (in)
    (in1)
    (in2)
    (out)
    (outx)
    (outy)
    (st)
    (vector2)
    ((string_type, "string"))

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

    // Color Space
    ((cs_raw, "raw"))

    // For supporting Usd texturing nodes
    (ND_UsdUVTexture)
    (ND_dot_vector2)
    (ND_UsdPrimvarReader_vector2)
    (UsdPrimvarReader_float2)
    (UsdUVTexture)
    (UsdVerticalFlip)

    // Additional terminal tokens needed for LookDevX materials
    ((mtlx_surface, "mtlx:surface"))
    ((mtlx_displacement, "mtlx:displacement"))
);

static bool
_FindGraphAndNodeByName(
    mx::DocumentPtr const &mxDoc,
    std::string const &mxNodeGraphName,
    std::string const &mxNodeName,
    mx::NodeGraphPtr *mxNodeGraph,
    mx::NodePtr *mxNode)
{
    // Graph names are uniquified with mxDoc->createValidChildName in hdMtlx,
    // so attempting to get the graph by the expected name may fail.
    // Go to some extra effort to find the graph that contains the named node.

    *mxNodeGraph = mxDoc->getNodeGraph(mxNodeGraphName);

    if (*mxNodeGraph) {
        *mxNode = (*mxNodeGraph)->getNode(mxNodeName);
    }
    if (!*mxNode) {
        std::vector<mx::NodeGraphPtr> graphs = mxDoc->getNodeGraphs();
        // first try last graph
        if (graphs.size()) {
            *mxNode =
                (*(graphs.rbegin()))->getNode(mxNodeName);
            if (*mxNode) {
                *mxNodeGraph = *graphs.rbegin();
            }
        }
        // Sometimes the above approach fails, so go looking
        // through all the graph nodes for the texture
        if (!*mxNode) {
            for(auto graph : graphs) {
                *mxNode = graph->getNode(mxNodeName);
                if (*mxNode) {
                    *mxNodeGraph = graph;
                    break;
                }
            }
        }
    }
    return (*mxNode != nullptr);
}

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
    mxContext.getOptions().fileTextureVerticalFlip = false;
    
    // Initialize the color management system
    mx::DefaultColorManagementSystemPtr cms =
        mx::DefaultColorManagementSystem::create(
            mxContext.getShaderGenerator().getTarget());
    cms->loadLibrary(HdMtlxStdLibraries());
    mxContext.getShaderGenerator().setColorManagementSystem(cms);

    // Set the target colorspace
    // XXX: This is equivalent to the default source colorspace, which does
    // not yet have a schema and is therefore not yet accessible here
    mxContext.getOptions().targetColorSpaceOverride = "lin_rec709";

    // Get the Node from the Nodegraph/mxDoc 
    mx::NodeGraphPtr mxNodeGraph;
    mx::NodePtr mxNode;

    _FindGraphAndNodeByName(mxDoc,
                            mxNodeGraphName,
                            mxNodeName,
                            &mxNodeGraph,
                            &mxNode);

    if (!mxNodeGraph) {
        TF_WARN("NodeGraph '%s' not found in the mxDoc.",
                mxNodeGraphName.c_str());
        return mx::EMPTY_STRING;
    }

    if (!mxNode) {
        TF_WARN("Node '%s' not found in '%s' nodeGraph.",
                mxNodeName.c_str(), mxNodeGraphName.c_str());
        return mx::EMPTY_STRING;
    }

    // Generate the OslShader for the Node
    TF_DEBUG(HDPRMAN_MATERIALS)
        .Msg("Generate a MaterialX Osl shader for '%s' node.\n", 
             mxNodeName.c_str());
    mx::ShaderPtr mxShader;
    try {
        mxShader = mx::createShader(shaderName, mxContext, mxNode);
    } catch (mx::Exception& exception) {
        TF_WARN("Unable to create Osl Shader for node '%s'.\nMxException: %s", 
                mxNodeName.c_str(), exception.what());
        return mx::EMPTY_STRING;
    }
    if (!mxShader) {
        TF_WARN("Unable to create Osl Shader for node '%s'.", 
                mxNodeName.c_str());
        return mx::EMPTY_STRING;
    }
    return mxShader->getSourceCode();
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
    else if (hdNodeType == _tokens->ND_UsdPreviewSurface_surfaceshader ||
        hdNodeType == _tokens->ND_UsdPreviewSurface) {
        return _tokens->USD_Adapter;
    }
    else if (hdNodeType == _tokens->ND_displacement_float ||
             hdNodeType == _tokens->ND_displacement_vector3) {
        return _tokens->Displacement_Adapter;
    }
    else {
        TF_WARN("Unsupported Node Type '%s'", hdNodeType.GetText());
        return TfToken();
    }
}

// Some material nodes in RenderMan require a mapping from the ND node type
// to the one used by RenderMan
static TfToken
_GetMaterialBsdfNodeType(TfToken const &hdNodeType)
{
    if (hdNodeType == _tokens->ND_oren_nayar_diffuse_bsdf) {
        return _tokens->MaterialXOrenNayarDiffuse;
    } else if (hdNodeType == _tokens->ND_generalized_schlick_bsdf) {
        return _tokens->MaterialXGeneralizedSchlick;
    } else if (hdNodeType == _tokens->ND_burley_diffuse_bsdf) {
        return _tokens->MaterialXBurleyDiffuse;
    } else if (hdNodeType == _tokens->ND_dielectric_bsdf) {
        return _tokens->MaterialXDielectric;
    } else if (hdNodeType == _tokens->ND_sheen_bsdf) {
        return _tokens->MaterialXSheen;
    }
    return hdNodeType;
}

// Convert terminal MaterialX shader type to corresponding rman material type.
static TfToken
_GetTerminalShaderType(TfToken const &hdNodeType)
{
    return (hdNodeType == _tokens->ND_displacement_float ||
            hdNodeType == _tokens->ND_displacement_vector3) ?
            _tokens->PxrDisplace : _tokens->PxrSurface;    
}

// Convert terminal MaterialX shader type to corresponding connection name
static TfToken
_GetTerminalConnectionName(TfToken const &hdNodeType)
{
    return (hdNodeType == _tokens->ND_displacement_float ||
            hdNodeType == _tokens->ND_displacement_vector3) ?
            HdMaterialTerminalTokens->displacement :
            HdMaterialTerminalTokens->surface;
}

// Convert the TfToken associated with the input parameters to Adapter Nodes
// that conflict with OSL reserved words. 
static TfToken
_GetUpdatedInputToken(TfToken const &currInputName)
{
    static const mx::StringMap conflicts = {
    // { currInputNname , updatedInputName }
        {"emission",    "emission_value"},
        {"subsurface",  "subsurface_value"},
        {"normal",      "normalIn"}};
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
    TF_DEBUG(HDPRMAN_DUMP_MATERIALX_OSL_SHADER)
        .Msg("--------- MaterialX Generated Shader '%s' ----------\n%s"
             "---------------------------\n\n", name.c_str(), oslSource.c_str());

    // Include the filepath to the MaterialX OSL directory containing mx_funcs.h
    std::vector<std::string> oslArgs;
    oslArgs.reserve(searchPaths.size());
#if MATERIALX_MAJOR_VERSION == 1 && \
    MATERIALX_MINOR_VERSION == 38 && \
    MATERIALX_BUILD_VERSION == 3
    static const mx::FilePath stdlibOslPath = "stdlib/osl";
#else 
    // MaterialX v1.38.4 restructured the OSL files and moved mx_funcs.h
    static const mx::FilePath stdlibOslPath = "stdlib/genosl/include"; 
#endif
    for (mx::FilePath const &path : searchPaths) {
        const mx::FilePath fullPath = path/stdlibOslPath;
        oslArgs.push_back(fullPath.exists() ? "-I" + fullPath.asString()
                                            : "-I" + path.asString());
    }

#if MATERIALX_MAJOR_VERSION == 1 && \
    MATERIALX_MINOR_VERSION == 38 && \
    MATERIALX_BUILD_VERSION == 3
    // Nothing
#else
    // MaterialX 1.38.4 removed its copy of stdosl.h and other OSL headers
    // and requires it to be included from the OSL installation itself.
    oslArgs.push_back(std::string("-I") + TfGetenv("RMANTREE") + "lib/osl");
#endif

    // Save compiled shader
    std::string sourceFilePath = ArchMakeTmpFileName("MX." + name, ".osl");
    FILE *sourceFile;
    sourceFile = fopen((sourceFilePath).c_str(), "w+");
    if (!sourceFile) {
        TF_WARN("Unable to save MaterialX OSL shader at '%s'\n",
                sourceFilePath.c_str());
        return mx::EMPTY_STRING;
    }
    else {
        fputs(oslSource.c_str(), sourceFile);
        fclose(sourceFile);
    }

    // Generate compiled shader
    const std::string compiledFilePath = ArchMakeTmpFileName("MX." + name, ".oso");
    std::string oslcLaunch = TfGetenv("RMANTREE");
    oslcLaunch += "/bin/oslc ";
    for (const auto& arg : oslArgs) {
        oslcLaunch += " " + arg;
    }
    oslcLaunch += " -q ";
    oslcLaunch += " -o " + compiledFilePath;
    oslcLaunch += " " + sourceFilePath;
#ifdef ARCH_OS_WINDOWS
    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));
    STARTUPINFO si;
    memset(&si, 0, sizeof(si));
    bool success = CreateProcess(NULL,
                                 (LPSTR)TEXT(oslcLaunch.c_str()),
                                 NULL,
                                 NULL,
                                 FALSE,
                                 CREATE_NO_WINDOW,
                                 NULL,
                                 NULL,
                                 &si,
                                 &pi);
    if(success) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    const int oslcResult = success ? 0 : HRESULT_FROM_WIN32(GetLastError());

#else
    const int oslcResult = std::system(oslcLaunch.c_str());
#endif
    // Check compiler was successful
    if (oslcResult != 0) {
        TF_WARN("Unable to compile MaterialX OSL shader at '%s'\n",
                compiledFilePath.c_str());
        return mx::EMPTY_STRING;
    }

    return compiledFilePath;
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
    mx::FileSearchPath const &searchPath,
    std::set<TfToken> nodesToKeep,
    std::set<TfToken> nodesToRemove)
{
    // Gather the nodeGraph nodes
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

                if(!sdrNode) {
                    continue;
                }

                // Update the connection into the terminal node so that the
                // output makes it into the closure
                TfToken const &inputName = cName;
#if PXR_VERSION >= 2505
                if (sdrNode->GetShaderOutput(outputName)) {
#else
                if (sdrNode->GetOutput(outputName)) {
#endif
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

            // Recursively look upstream for the first mtlx pattern.
            // In other words, skip over non-mtlx nodes and mtlx bsdf nodes.
            SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
            SdfPath const nodePath = SdfPath(upstreamNodeName);
            std::string const &mxNodeName = HdMtlxCreateNameFromPath(nodePath);
            std::string const &mxNodeGraphName =
                nodePath.GetParentPath().GetName();
            
            mx::NodePtr mxNode;
            mx::NodeGraphPtr mxNodeGraph;
            _FindGraphAndNodeByName(
                mxDoc, mxNodeGraphName, mxNodeName, &mxNodeGraph, &mxNode);

            // If this node was written in an older version of MaterialX, we 
            // want to look to the mxDoc for the nodeType because that will 
            // have the updated information to match the version of MaterialX 
            // that is being used. 
            const TfToken nodeType =
                mxNode
                    ? TfToken(mxNode->getNodeDefString())
                    : netInterface->GetNodeType(upstreamNodeName);

            const SdrShaderNodeConstPtr sdrMtlxNode =
                sdrRegistry.GetShaderNodeByIdentifierAndType(
                    nodeType, _tokens->mtlx);
            if (!sdrMtlxNode ||
               TfStringEndsWith(nodeType.GetText(), "_bsdf")) {
                _UpdateNetwork(netInterface, upstreamNodeName, mxDoc,
                               searchPath, nodesToKeep, nodesToRemove);
                netInterface->SetNodeType(
                    upstreamNodeName,
                    _GetMaterialBsdfNodeType(
                        netInterface->GetNodeType(upstreamNodeName)));
                continue;
            }
            
            // Collect nodes further removed from the terminal in nodesToRemove
            std::set<TfToken> tmpVisitedNodeNames;
            _GatherNodeGraphNodes(netInterface, upstreamNodeName, 
                                  &nodesToRemove, &tmpVisitedNodeNames);
            nodesToKeep.insert(upstreamNodeName);

            // Generate the oslSource code for the connected upstream node
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
            SdrShaderNodeConstPtr sdrNode = 
                sdrRegistry.GetShaderNodeFromAsset(
                                SdfAssetPath(compiledShaderPath),
                                SdrTokenMap(),  // metadata
                                _tokens->mtlx,  // subId
                                _tokens->OSL);  // sourceType

            if (!sdrNode) {
                continue;
            }

            // Update node type to that of the Sdr node.
            netInterface->SetNodeType(
                upstreamNodeName, sdrNode->GetIdentifier());

            // Update the connection into the terminal node so that the 
            // nodegraph outputs make their way into the closure
#if PXR_VERSION >= 2505
            if (sdrNode->GetShaderOutput(outputName)) {
#else
            if (sdrNode->GetOutput(outputName)) {
#endif
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
}

// Transform the original terminalNode with an Adapter Node which connects to a
// new PxrSurface or PxrDisplace Node that becomes the surfaceTerminal
// node in the hdNetwork.
static void 
_TransformTerminalNode(
    HdMaterialNetworkInterface *netInterface,
    TfToken const &terminalNodeName)
{
    // Create a SdrShaderNode for the Adapter and PxrSurface/PxrDisplace Nodes.
    TfToken const nodeType = netInterface->GetNodeType(terminalNodeName);
    TfToken const adapterType = _GetAdapterNodeType( nodeType );
    TfToken const shaderType = _GetTerminalShaderType( nodeType );
    TfToken const terminalToken = _GetTerminalConnectionName( nodeType );

    SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
    SdrShaderNodeConstPtr const sdrAdapter = 
        sdrRegistry.GetShaderNodeByIdentifier(adapterType, {_tokens->OSL});
    SdrShaderNodeConstPtr const sdrShader = 
        sdrRegistry.GetShaderNodeByIdentifier(shaderType,
                                              {_tokens->RmanCpp});
    if (!sdrAdapter) {
        TF_WARN("No sdrAdater node of type '%s'", adapterType.GetText());

        // Prman does not have an adapter node for MtlxSurface terminal nodes.
        // Instead use the the upstream BSDF node as the terminal.
        if (nodeType == _tokens->ND_surface) {
            for (const auto& inParamName:
                     netInterface->GetNodeInputConnectionNames(terminalNodeName)) {
                HdMaterialNetwork2Interface::InputConnectionVector inputs =
                    netInterface->GetNodeInputConnection(terminalNodeName,
                                                         inParamName);
                for (const auto& input: inputs) {
                    TfToken upstreamNode = input.upstreamNodeName;
                    netInterface->SetTerminalConnection(
                        terminalToken, { upstreamNode, TfToken() });
                    break;
                }
                break;
            }
        }
        return;
    }

    // Transform the terminalNode with the appropriate Adapter Node, which
    // translates the MaterialX parameters into PxrSurface/PxrDisplace inputs.
    netInterface->SetNodeType(terminalNodeName, adapterType);

    // Update the TfTokens associated with the Adapter Node's input parameters
    // that conflict with OSL reserved words. 
    // The corresponding input connection is updated in _UpdateNetwork()
    TfTokenVector pNames =
        netInterface->GetAuthoredNodeParameterNames(terminalNodeName);
    for (TfToken const &pName : pNames) {
        const TfToken updatedName = _GetUpdatedInputToken(pName);
        if (!updatedName.IsEmpty()) {
            const VtValue val = netInterface->GetNodeParameterValue(
                terminalNodeName, pName);
            netInterface->SetNodeParameterValue(
                terminalNodeName, updatedName, val);
            netInterface->DeleteNodeParameter(terminalNodeName, pName);
        }
    }
    
    // Create a RenderMan material node (ie. PxrSurface or PxrDisplace)
    TfToken rmanShaderNodeName =
        TfToken(terminalNodeName.GetString() + "_" + shaderType.GetString());
    netInterface->SetNodeType(rmanShaderNodeName, shaderType);

    // Connect the RenderMan material inputs to the Adapter's outputs
#if PXR_VERSION >= 2505
    for (const auto& inParamName: sdrShader->GetShaderInputNames()) {
#else
    for (const auto& inParamName: sdrShader->GetInputNames()) {
#endif

        if (sdrShader->GetShaderInput(inParamName)) {

            // Convert the parameter name to the "xxxOut" format
            TfToken adapterOutParam = TfToken(inParamName.GetString() + "Out");
            
            // If the shader Input is an Adapter node output add the
            // inputConnection to the shader Node
            // Note: not every input has a corresponding output
            if (sdrAdapter->GetShaderOutput(adapterOutParam)) {
                netInterface->SetNodeInputConnection(
                    rmanShaderNodeName, inParamName, 
                    { {terminalNodeName, adapterOutParam} });
            }
        }
    }

    // Update the network terminals so that the terminal Node is the RenderMan
    // Node instead of the Adapter Node (previously the mtlx terminal node)
    netInterface->SetTerminalConnection(
        terminalToken, { rmanShaderNodeName, TfToken() });
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

// Returns true is the given mtlxSdrNode requires primvar support for texture 
// coordinates
static bool
_NodeHasTextureCoordPrimvar(
    mx::DocumentPtr const &mxDoc,
    const SdrShaderNodeConstPtr mtlxSdrNode)
{
    // Custom nodes may have a <texcoord> or <geompropvalue> node as
    // a part of the defining nodegraph
    const mx::NodeDefPtr mxNodeDef =
        mxDoc->getNodeDef(mtlxSdrNode->GetIdentifier().GetString());
    mx::InterfaceElementPtr impl = mxNodeDef->getImplementation();
    if (impl && impl->isA<mx::NodeGraph>()) {
        const mx::NodeGraphPtr nodegraph = impl->asA<mx::NodeGraph>();
        // Return True if the defining nodegraph uses a texcoord node
        if (!nodegraph->getNodes(_tokens->texcoord).empty()) {
            return true;
        } 
        // Or a geompropvalue node of type vector2, which we assume to be 
        // for texture coordinates. 
        auto geompropvalueNodes = nodegraph->getNodes(_tokens->geompropvalue);
        for (const mx::NodePtr& mxGeomPropNode : geompropvalueNodes) {
#if MATERIALX_MAJOR_VERSION == 1 && MATERIALX_MINOR_VERSION <= 38
            if (mxGeomPropNode->getType() == mx::Type::VECTOR2->getName()) {
#else
            if (mxGeomPropNode->getType() == mx::Type::VECTOR2.getName()) {
#endif
                return true;
            }
        }
    }
    return false;
}


// Return true if the network contains any mtlx nodes
static bool
_NetworkHasMtlxNodes(HdMaterialNetworkInterface *netInterface)
{
    SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
    const TfTokenVector nodeNames = netInterface->GetNodeNames();
    for (TfToken const &nodeName : nodeNames) {
        const TfToken nodeType = netInterface->GetNodeType(nodeName);
        const SdrShaderNodeConstPtr sdrNode =
            sdrRegistry.GetShaderNodeByIdentifierAndType(
                nodeType, _tokens->mtlx);
        if (sdrNode) {
            return true;
        }
    }
    return false;
}


// Look for UsdUvTexture, UsdPrimvarReader_float2, etc
// and replace with the corresponding mtlx definition type,
// available in Solaris with "ND_" prefix.
// The network has already gone through MatfiltUsdPreviewSurface, which
// may have inserted UsdVerticalFlip.
// Convert nonstandard UsdVerticalFlip to a pass through ND_dot_vector2,
// and the mtlx nodes for flipping will be inserted by _UpdateTextureNodes.
#ifdef PXR_DCC_LOCATION_ENV_VAR
static void
_FixNodeNames(
    HdMaterialNetworkInterface *netInterface)
{
    const TfTokenVector nodeNames = netInterface->GetNodeNames();
    for (TfToken const &nodeName : nodeNames) {
        TfToken nodeType = netInterface->GetNodeType(nodeName);
        if(TfStringStartsWith(nodeType.GetText(), "Usd")) {
            if(nodeType == _tokens->UsdPrimvarReader_float2) {
                nodeType = _tokens->ND_UsdPrimvarReader_vector2;
            } else if(nodeType == _tokens->UsdVerticalFlip) {
                nodeType = _tokens->ND_dot_vector2; // pass through node
            } else {
                nodeType = TfToken("ND_"+nodeType.GetString());
            }
            netInterface->SetNodeType(nodeName, nodeType);
        }
    }
}
#endif

static void 
_UpdateTextureNodes(
    HdMaterialNetworkInterface *netInterface,
    std::set<SdfPath> const &hdTextureNodePaths,
    mx::DocumentPtr const &mxDoc)
{
    for (SdfPath const &texturePath : hdTextureNodePaths) {
        TfToken const &textureNodeName = texturePath.GetToken();
        std::string mxTextureNodeName =
                HdMtlxCreateNameFromPath(texturePath);
        const TfToken nodeType = netInterface->GetNodeType(textureNodeName);
        if (nodeType.IsEmpty()) {
            TF_WARN("Connot find texture node '%s' in material network.",
                    textureNodeName.GetText());
            continue;
        }
        // Get the filename parameter name, 
        // MaterialX stdlib nodes use 'file' however, this could be different
        // for custom nodes that use textures.
        TfToken fileParamName = _tokens->file;
        const mx::NodeDefPtr nodeDef = mxDoc->getNodeDef(nodeType);
        if (nodeDef) {
            for (auto const& mxInput : nodeDef->getActiveInputs()) {
                if (mxInput->getType() == _tokens->filename) {
                    fileParamName = TfToken(mxInput->getName());
                }
            }

        }
#if PXR_VERSION >= 2402
        HdMaterialNetworkInterface::NodeParamData fileParamData =
            netInterface->GetNodeParameterData(textureNodeName, fileParamName);
        const VtValue vFile = fileParamData.value;
#else
        VtValue vFile =
            netInterface->GetNodeParameterValue(textureNodeName, fileParamName);
#endif
        if (vFile.IsEmpty()) {
            TF_WARN("File path missing for texture node '%s'.",
                    textureNodeName.GetText());
            continue;
        }

        std::string path;

        // Typically expect SdfAssetPath, but UsdUVTexture nodes may
        // have changed value to string due to MatfiltConvertPreviewMaterial
        // inserting rtxplugin call.
        if (vFile.IsHolding<SdfAssetPath>()) {
            path = vFile.Get<SdfAssetPath>().GetResolvedPath();
            if(path.empty()) {
                path = vFile.Get<SdfAssetPath>().GetAssetPath();
            }
        } else if(vFile.IsHolding<std::string>()) {
            path = vFile.Get<std::string>();
        }
        // Convert to posix path beause windows backslashes will get lost
        // before reaching the rtx plugin
        path = mx::FilePath(path).asString(mx::FilePath::FormatPosix);

        if(!path.empty()) {
            const std::string ext = ArGetResolver().GetExtension(path);

            mx::NodeGraphPtr mxNodeGraph;
            mx::NodePtr mxTextureNode;
            _FindGraphAndNodeByName(mxDoc,
                                    texturePath.GetParentPath().GetName(),
                                    mxTextureNodeName,
                                    &mxNodeGraph,
                                    &mxTextureNode);

            if(!mxTextureNode) {
                continue;
            }

            // Update texture nodes that use non-native texture formats
            // to read them via a Renderman texture plugin.
            bool needInvertT = false;
            if(TfStringStartsWith(path, "rtxplugin:")) {
                mxTextureNode->setInputValue(_tokens->file.GetText(), // name
                                             path,                    // value
                                             _tokens->filename.GetText());//type
            }
            else if (!ext.empty() && ext != "tex") {

                // Update the input value to use the Renderman texture plugin
                const std::string pluginName = 
                    std::string("RtxHioImage") + ARCH_LIBRARY_SUFFIX;

                TfToken uWrap, vWrap;
                _GetWrapModes(netInterface, textureNodeName, &uWrap, &vWrap);

                // Use 'raw' for the colorspace, this allows MaterialX to 
                // handle any colorspace transforms.
                const TfToken colorSpace = _tokens->cs_raw;

                std::string const &mxInputValue = TfStringPrintf(
                    "rtxplugin:%s?filename=%s&wrapS=%s&wrapT=%s&sourceColorSpace=%s",
                    pluginName.c_str(), path.c_str(), uWrap.GetText(),
                    vWrap.GetText(), colorSpace.GetText());
                TF_DEBUG(HDPRMAN_IMAGE_ASSET_RESOLVE)
                    .Msg("Resolved MaterialX asset path: %s\n",
                         mxInputValue.c_str());

                // Update the MaterialX Texture Node with the new mxInputValue
                mxTextureNode->setInputValue(fileParamName.GetText(), // name
                                             mxInputValue,            // value
                                             _tokens->filename.GetText());//type
            }
            else {
                needInvertT = true;
                // For tex files, update value with resolved path, because prman
                // may not be able to find a usd relative path.
                mxTextureNode->
                    setInputValue(_tokens->file.GetText(), // name
                                  path,                    // value
                                  _tokens->filename.GetText());//type
                TF_DEBUG(HDPRMAN_IMAGE_ASSET_RESOLVE)
                    .Msg("Resolved MaterialX asset path: %s\n",
                         path.c_str());
            }

            // UsdUvTexture nodes and MtlxImage nodes have different
            // names for their texture coordinate connection.
            const TfToken texCoordToken =
                (nodeType == _tokens->ND_UsdUVTexture) ?
                _tokens->st : _tokens->texcoord;

            // If texcoord param isn't connected, make a default connection
            // to a mtlx geompropvalue node.
            mx::InputPtr texcoordInput =
                mxTextureNode->getInput(texCoordToken);
            if(!texcoordInput) {

                // Get the sdr node for the mxTexture node
                SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
                const SdrShaderNodeConstPtr sdrTextureNode =
                    sdrRegistry.GetShaderNodeByIdentifierAndType(
                        nodeType, _tokens->mtlx);

                // If the node does not already contain a texcoord primvar node
                // add one and connect it to the mxTextureNode
                // XXX If a custom node uses a texture but does not explicitly
                // use a texcoords or geomprop node for the texture coordinates
                // this will force a connection onto the custom node and the 
                // material will likely not render.
                if (!_NodeHasTextureCoordPrimvar(mxDoc, sdrTextureNode)) {
                    // Get the primvarname from the sdrTextureNode metadata
                    auto metadata = sdrTextureNode->GetMetadata();
                    auto primvarName = metadata[SdrNodeMetadata->Primvars];

                    // Create a geompropvalue node for the texture coordinates
                    const std::string stNodeName =
                        textureNodeName.GetString() + "__texcoord";
                    mx::NodePtr geompropNode =
                        mxNodeGraph->addNode(_tokens->geompropvalue,
                                            stNodeName,
                                            _tokens->vector2);
                    geompropNode->setInputValue(_tokens->geomprop,
                                                primvarName,
                                                _tokens->string_type);
                    geompropNode->setNodeDefString(
                        _tokens->ND_geompropvalue_vector2);
                    
                    // Add the texcoord input and connect to the new node
                    texcoordInput =
                        mxTextureNode->addInput(_tokens->texcoord,
                                                _tokens->vector2);
                    texcoordInput->setConnectedNode(geompropNode);
                }
            }
            if(needInvertT) {
                // This inserts standard mtlx nodes to carry out the math
                // for udim aware invert of t; only want to flip
                // the fractional portion of the t value, like this:
                // 2*floor(t) + 1.0 - t
                texcoordInput = mxTextureNode->getInput(texCoordToken);
                if(texcoordInput) {
                    mx::NodePtr primvarNode = texcoordInput->getConnectedNode();
                    const std::string separateNodeName =
                        mxTextureNodeName + "__separate";
                    const std::string floorNodeName =
                        mxTextureNodeName + "__floor";
                    const std::string multiplyNodeName =
                        mxTextureNodeName + "__multiply";
                    const std::string addNodeName =
                        mxTextureNodeName + "__add";
                    const std::string subtractNodeName =
                        mxTextureNodeName + "__subtract";
                    const std::string combineNodeName =
                        mxTextureNodeName + "__combine";

                    mx::NodePtr separateNode =
                        mxNodeGraph->addNode(_tokens->separate2,
                                             separateNodeName,
                                             _tokens->vector2);
                    separateNode->
                        setNodeDefString(_tokens->ND_separate2_vector2);

                    mx::NodePtr floorNode =
                        mxNodeGraph->addNode(_tokens->floor,
                                             floorNodeName);
                    floorNode->
                        setNodeDefString(_tokens->ND_floor_float);

                    mx::NodePtr multiplyNode =
                        mxNodeGraph->addNode(_tokens->multiply,
                                             multiplyNodeName);
                    multiplyNode->
                        setNodeDefString(_tokens->ND_multiply_float);

                    mx::NodePtr addNode =
                        mxNodeGraph->addNode(_tokens->add,
                                             addNodeName);
                    addNode->
                        setNodeDefString(_tokens->ND_add_float);

                    mx::NodePtr subtractNode =
                        mxNodeGraph->addNode(_tokens->subtract,
                                             subtractNodeName);
                    subtractNode->
                        setNodeDefString(_tokens->ND_subtract_float);

                    mx::NodePtr combineNode =
                        mxNodeGraph->addNode(_tokens->combine2,
                                             combineNodeName);
                    combineNode->
                        setNodeDefString(_tokens->ND_combine2_vector2);

                    mx::InputPtr separateNode_inInput =
                            separateNode->addInput(_tokens->in,
                                                   _tokens->vector2);
                    mx::OutputPtr separateNode_outxOutput =
                        separateNode->addOutput(_tokens->outx);
                    mx::OutputPtr separateNode_outyOutput =
                        separateNode->addOutput(_tokens->outy);
                    separateNode_inInput->setConnectedNode(primvarNode);

                    mx::InputPtr floorNode_inInput =
                        floorNode->addInput(_tokens->in);
                    mx::OutputPtr floorNode_outOutput =
                        floorNode->addOutput(_tokens->out);
                    floorNode_inInput->setConnectedNode(separateNode);
                    floorNode_inInput->
                        setConnectedOutput(separateNode_outyOutput);

                    mx::InputPtr multiplyNode_in1Input =
                        multiplyNode->addInput(_tokens->in1);
                    mx::OutputPtr multiplyNode_outOutput =
                        multiplyNode->addOutput(_tokens->out);
                    multiplyNode_in1Input->setConnectedNode(floorNode);
                    multiplyNode->setInputValue(_tokens->in2, 2);

                    mx::InputPtr addNode_in1Input =
                        addNode->addInput(_tokens->in1);
                    mx::OutputPtr addNode_outOutput =
                        addNode->addOutput(_tokens->out);
                    addNode_in1Input->setConnectedNode(multiplyNode);
                    addNode->setInputValue(_tokens->in2, 1);

                    mx::InputPtr subtractNode_in1Input =
                        subtractNode->addInput(_tokens->in1);
                    mx::InputPtr subtractNode_in2Input =
                        subtractNode->addInput(_tokens->in2);
                    mx::OutputPtr subtractNode_outOutput =
                        subtractNode->addOutput(_tokens->out);
                    subtractNode_in1Input->setConnectedNode(addNode);
                    subtractNode_in2Input->setConnectedNode(separateNode);
                    subtractNode_in2Input->
                        setConnectedOutput(separateNode_outyOutput);

                    mx::InputPtr combineNode_in1Input =
                        combineNode->addInput(_tokens->in1);
                    mx::InputPtr combineNode_in2Input =
                        combineNode->addInput(_tokens->in2);
                    mx::OutputPtr combineNode_outOutput =
                        combineNode->addOutput(_tokens->out,
                                               _tokens->vector2);
                    combineNode_in1Input->setConnectedNode(separateNode);
                    combineNode_in2Input->setConnectedNode(subtractNode);
                    texcoordInput->setConnectedNode(combineNode);
                }
            }
        }
    }
}

// Texcoord nodes don't work for RenderMan, so convert them
// to geompropvalue nodes that look up the texture coordinate primvar name.
static void
_UpdatePrimvarNodes(
    HdMaterialNetworkInterface *netInterface,
    std::set<SdfPath> const &hdPrimvarNodePaths,
    mx::DocumentPtr const &mxDoc)
{
    for (SdfPath const &nodePath : hdPrimvarNodePaths) {
        TfToken const &nodeName = nodePath.GetToken();
        std::string mxNodeName = HdMtlxCreateNameFromPath(nodePath);
        const TfToken nodeType = netInterface->GetNodeType(nodeName);
        if (nodeType.IsEmpty()) {
            TF_WARN("Can't find node '%s' in material network.",
                    nodeName.GetText());
            continue;
        }

        mx::NodeGraphPtr mxNodeGraph;
        mx::NodePtr mxNode;

        _FindGraphAndNodeByName(mxDoc, nodePath.GetParentPath().GetName(),
                                mxNodeName, &mxNodeGraph, &mxNode);

        // Ignore nodes that aren't "texcoord" nodes
        if (!mxNode || mxNode->getCategory() != _tokens->texcoord) {
            continue;
        }
        mx::NodeDefPtr mxNodeDef = mxDoc->getNodeDef(
            _tokens->ND_geompropvalue_vector2.GetText());
        if (!mxNodeDef) {
            continue;
        }

        // Get the sdr node for the texcoord node
        SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
        const SdrShaderNodeConstPtr sdrTexcoordNode =
            sdrRegistry.GetShaderNodeByIdentifierAndType(
                nodeType, _tokens->mtlx);
        // Get the primvarname from the sdrTexcoordNode metadata
        auto metadata = sdrTexcoordNode->GetMetadata();
        auto primvarName = metadata[SdrNodeMetadata->Primvars];

        // Set the category and type of this texcoord node
        // so that it will become a geompropvalue node
        // that looks up the texture coordinate primvar name.
        mxNode->setType(mxNodeDef->getType());
        mxNode->setCategory(mxNodeDef->getNodeString());
        mxNode->setNodeDefString(_tokens->ND_geompropvalue_vector2);
        mxNode->setInputValue(_tokens->geomprop.GetText(),
                              primvarName,
                              _tokens->string_type.GetText());
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

    static const std::vector<TfToken> supportedTerminalTokens = {
        HdMaterialTerminalTokens->surface,
        _tokens->mtlx_surface,
        HdMaterialTerminalTokens->displacement,
        _tokens->mtlx_displacement
    };

    std::set<TfToken> nodesToKeep;   // nodes directly connected to the terminal
    std::set<TfToken> nodesToRemove; // nodes further removed from the terminal

    for (auto terminalName : supportedTerminalTokens ) {

        // Check presence of terminal
        const HdMaterialNetworkInterface::InputConnectionResult res =
            netInterface->GetTerminalConnection(terminalName);
        if (!res.first) { // terminal absent, skip
            continue;
        }
        const TfToken &terminalNodeName = res.second.upstreamNodeName;
        const TfToken terminalNodeType =
            netInterface->GetNodeType(terminalNodeName);

        // Check if the network uses any Mtlx nodes, and return early if not.
        // The terminal node may be Mtlx, but we also want to support
        // using mtlx patterns with Usd, Pxr or Lama materials.
        if(!_NetworkHasMtlxNodes(netInterface)) {
            return;
        }

        TfTokenVector cNames =
            netInterface->GetNodeInputConnectionNames(terminalNodeName);
        // If we have a nodegraph (i.e., input into the terminal node)...
        if (!cNames.empty()) {
            // Serialize MaterialX usage to avoid crashes.
            //
            // XXX It may be the case that a finer-grained locking
            //     pattern can be used here.  Starting with a coarse
            //     lock to establish a basic level of safety.
            //
            static std::mutex materialXMutex;
            std::lock_guard<std::mutex> lock(materialXMutex);

            // Get Standard Libraries and SearchPaths (for mxDoc and 
            // mxShaderGen)
            mx::DocumentPtr stdLibraries = HdMtlxStdLibraries();
            mx::FileSearchPath searchPath = HdMtlxSearchPaths();

#ifdef PXR_DCC_LOCATION_ENV_VAR
            // Preprocess node network, converting UsdUvTexture, and
            // related nodes to their mtlx definition nodes.
            _FixNodeNames(netInterface);
#endif

            // Create the MaterialX Document from the material network
            HdMtlxTexturePrimvarData hdMtlxData;
            mx::DocumentPtr mxDoc =
                HdMtlxCreateMtlxDocumentFromHdMaterialNetworkInterface(
                    netInterface, terminalNodeName, cNames,
                    stdLibraries, &hdMtlxData);

            _UpdateTextureNodes(netInterface, hdMtlxData.hdTextureNodes, mxDoc);
            _UpdatePrimvarNodes(netInterface, hdMtlxData.hdPrimvarNodes, mxDoc);

            // Remove the material and shader nodes from the MaterialX Document
            // (since we need to use PxrSurface as the closure instead of the 
            // MaterialX surfaceshader node)
            SdfPath materialPath = netInterface->GetMaterialPrimPath();
            mxDoc->removeNode("SR_" + materialPath.GetName());  // Shader Node
            mxDoc->removeNode(materialPath.GetName());          // Material Node

            // Update nodes directly connected to the terminal node with 
            // MX generated shaders that capture the rest of the nodegraph
            _UpdateNetwork(netInterface, terminalNodeName, mxDoc, searchPath,
                           nodesToKeep, nodesToRemove);
        }

        // Convert the terminal node to an AdapterNode + PxrSurfaceNode
        _TransformTerminalNode(netInterface, terminalNodeName);
    }

    // Remove the nodes not directly connected to the terminal
    for (const TfToken& nodeName: nodesToRemove) {
        // As long as the node is not also directly connected to the terminal
        if (nodesToKeep.find(nodeName) == nodesToKeep.end()) {
            netInterface->DeleteNode(nodeName);
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE                        
