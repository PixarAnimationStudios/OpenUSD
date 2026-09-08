//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "hdPrman/matfiltMaterialX.h"
#include "hdPrman/debugCodes.h"

#include "pxr/base/arch/env.h"
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
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#if PXR_VERSION >= 2608
#include "pxr/imaging/hdMtlx/hdMtlx.h"
#else
#include "hdPrman/pxr/imaging/hdMtlx/hdMtlx.h"
#endif

#if PXR_VERSION >= 2511
#include "pxr/imaging/hdMtlx/combinedMtlxVersion.h"
#else
#include <MaterialXCore/Generated.h>
#define MTLX_COMBINED_VERSION                                                \
    ((MATERIALX_MAJOR_VERSION * 100 * 100) + (MATERIALX_MINOR_VERSION * 100) \
     + MATERIALX_BUILD_VERSION)
#endif

#include <MaterialXCore/Node.h>
#include <MaterialXCore/Document.h>
#include <MaterialXFormat/Environ.h>
#include <MaterialXFormat/Util.h>
#include <MaterialXFormat/XmlIo.h>
#include <MaterialXGenShader/DefaultColorManagementSystem.h>
#include <MaterialXGenShader/Shader.h>
#include <MaterialXGenShader/ShaderNode.h>
#include <MaterialXGenShader/Util.h>
#include <MaterialXGenOsl/OslShaderGenerator.h>
#include <MaterialXRender/Util.h>

#if defined(ARCH_OS_WINDOWS)
#include <Windows.h>
#endif

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
    (ND_burley_diffuse_bsdf)
    (MaterialXBurleyDiffuse)
    (ND_conductor_bsdf)
    (MaterialXConductor)
    (ND_dielectric_bsdf)
    (MaterialXDielectric)
    (ND_generalized_schlick_bsdf)
    (MaterialXGeneralizedSchlick)
    (ND_oren_nayar_diffuse_bsdf)
    (MaterialXOrenNayarDiffuse)
    (ND_sheen_bsdf)
    (MaterialXSheen)
    (ND_subsurface_bsdf)
    (MaterialXSubsurface)
    (ND_translucent_bsdf)
    (MaterialXTranslucent)
    (ND_mix_bsdf)
    (ND_mix_surfaceshader)
    (MaterialXMix)
    (ND_multiply)
    (ND_multiply_bsdf)
    (ND_multiply_bsdfC)
    (MaterialXMultiply)
    (ND_add_bsdf)
    (MaterialXAdd)
    (ND_uniform_edf)
    (MaterialXEmissionUniform)
    (ND_generalized_schlick_edf)
    (MaterialXEmissionGeneralizedSchlick)
    (ND_mix_edf)
    (ND_multiply_edf)
    (ND_multiply_edfC)
    (color)
    (ND_layer_bsdf)
    (ND_layer)
    (MaterialXLayer)
    (MaterialXSeparate2)
    (top)
    (base)

    // Houdini's visualize VOP for mtlx
    (ND_surface_unlit)
    (PxrConstant)
    (emission_color)
    (emitColor)

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

    // Hydra shading systems
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

    // expanding implementation graphs
    (bsdf)
    (edf)
    (vdf)
    (BSDF)
    (EDF)
    (VDF)
    ((_float, "float"))
    (ND_convert)
    (ND_convert_float_color3)
    (ND_extract)
    (opacity)
    (Surface)
    (surfaceshader)

    // converting geomcolor to geompropvalue nodes
    (displayColor)
    (geomcolor)
    (ND_geompropvalue_color3)

    // Additional terminal tokens needed for LookDevX materials
    ((mtlx_surface, "mtlx:surface"))
    ((mtlx_displacement, "mtlx:displacement"))
);

TF_DEFINE_ENV_SETTING(HD_PRMAN_ENABLE_IMPLEMENTATION_GRAPH, true,
                      "Enable translating MaterialX surfaces via their implementation graph.");

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
    } else if (hdNodeType == _tokens->ND_conductor_bsdf) {
        return _tokens->MaterialXConductor;
    } else if (hdNodeType == _tokens->ND_subsurface_bsdf) {
        return _tokens->MaterialXSubsurface;
    } else if (hdNodeType == _tokens->ND_translucent_bsdf) {
        return _tokens->MaterialXTranslucent;
    } else if (hdNodeType == _tokens->ND_mix_bsdf ||
               hdNodeType == _tokens->ND_mix_surfaceshader) {
        return _tokens->MaterialXMix;
    } else if (hdNodeType == _tokens->ND_multiply_bsdf ||
               hdNodeType == _tokens->ND_multiply_bsdfC ||
               hdNodeType == _tokens->ND_multiply_edf ||
               hdNodeType == _tokens->ND_multiply_edfC) {
        return _tokens->MaterialXMultiply;  // temporary, until multiply is supported
    } else if (hdNodeType == _tokens->ND_add_bsdf) {
        return _tokens->MaterialXAdd;
    } else if (hdNodeType == _tokens->ND_uniform_edf) {
        return _tokens->MaterialXEmissionUniform;
    } else if (hdNodeType == _tokens->ND_mix_edf) {
        return _tokens->MaterialXMix;
    } else if (hdNodeType == _tokens->ND_generalized_schlick_edf) {
        return _tokens->MaterialXEmissionGeneralizedSchlick;
    } else if (hdNodeType == _tokens->ND_layer || hdNodeType == _tokens->ND_layer_bsdf) {
        return _tokens->MaterialXLayer;
    }
    return hdNodeType;
}

// Decompose an ND_surface node into its bsdf/edf components.
// If both bsdf and edf are connected, creates a MaterialXAdd combiner
// and returns its name. If only one is connected, returns that node.
// Returns an empty token if neither is connected.
static TfToken
_DecomposeMtlxSurface(
    HdMaterialNetworkInterface *netInterface,
    TfToken const &ndSurfaceNodeName)
{
    auto bsdfConns = netInterface->GetNodeInputConnection(
        ndSurfaceNodeName, _tokens->bsdf);
    auto edfConns = netInterface->GetNodeInputConnection(
        ndSurfaceNodeName, _tokens->edf);

    TfToken bsdfNode = bsdfConns.empty() ?
        TfToken() : bsdfConns[0].upstreamNodeName;
    TfToken edfNode = edfConns.empty() ?
        TfToken() : edfConns[0].upstreamNodeName;

    if (!bsdfNode.IsEmpty() && !edfNode.IsEmpty()) {
        // Combine bsdf and edf via MaterialXAdd
        TfToken combinerName(
            ndSurfaceNodeName.GetString() + "_edf_add");
        netInterface->SetNodeType(combinerName, _tokens->MaterialXAdd);
        netInterface->SetNodeInputConnection(
            combinerName, _tokens->in1, {{bsdfNode, TfToken()}});
        netInterface->SetNodeInputConnection(
            combinerName, _tokens->in2, {{edfNode, TfToken()}});
        return combinerName;
    } else if (!bsdfNode.IsEmpty()) {
        return bsdfNode;
    } else if (!edfNode.IsEmpty()) {
        return edfNode;
    }
    return TfToken();
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
#if MTLX_COMBINED_VERSION == 13803
    static const mx::FilePath stdlibOslPath = "stdlib/osl";
#else 
    // MaterialX v1.38.4 restructured the OSL files and moved mx_funcs.h
    static const mx::FilePath stdlibOslPath = "stdlib/genosl/include"; 
#endif
    for (mx::FilePath const &path : searchPaths) {
        const mx::FilePath fullPath = path/stdlibOslPath;
        oslArgs.push_back(fullPath.exists() ? "-I\"" + fullPath.asString() + "\""
                                            : "-I\"" + path.asString() + "\"");
    }

#if MTLX_COMBINED_VERSION >= 13804
    // MaterialX 1.38.4 removed its copy of stdosl.h and other OSL headers
    // and requires it to be included from the OSL installation itself.
    oslArgs.push_back(std::string("-I\"") + TfGetenv("RMANTREE") + "lib/osl\"");
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

// Detect if a parameter needs vector2 splitting based on SDR definition.
static bool
_NeedsVector2Split(
    SdrShaderNodeConstPtr const &sdrNode,
    TfToken const &baseName)
{
    if (!sdrNode) {
        return false;
    }

    // Check if both [0] and [1] array indices exist in the SDR definition
    TfToken arrayParam0(baseName.GetString() + "[0]");
    TfToken arrayParam1(baseName.GetString() + "[1]");

    SdrShaderPropertyConstPtr prop0 = sdrNode->GetShaderInput(arrayParam0);
    SdrShaderPropertyConstPtr prop1 = sdrNode->GetShaderInput(arrayParam1);

    return (prop0 && prop1);
}

// Split vector2 parameters into array-indexed float parameters,
// for rman implementations of bsdfs which don't support vector2 types.
// Uses the sdrDefinitionArrayIndex tag in shader args to detect parameters
// that need splitting (e.g., roughness[0], roughness[1]).
//
static void
_SplitVector2ConnectionsForNode(
    HdMaterialNetworkInterface *netInterface,
    const TfToken &nodeName,
    SdrShaderNodeConstPtr const &sdrNode,
    std::map<std::pair<TfToken, TfToken>, TfToken> *separate2NodeCache)
{
    if (!sdrNode) {
        return;
    }

    // Process connections
    const TfTokenVector connectionNames =
        netInterface->GetNodeInputConnectionNames(nodeName);

    for (const TfToken &connectionName : connectionNames) {
        // Check if the shader expects array-indexed versions of this parameter
        if (!_NeedsVector2Split(sdrNode, connectionName)) {
            continue;
        }

        auto inputConnections = netInterface->GetNodeInputConnection(
            nodeName, connectionName);

        if (inputConnections.empty()) {
            continue;
        }

        // Get the upstream node information
        const TfToken &upstreamNodeName = inputConnections[0].upstreamNodeName;
        const TfToken &upstreamOutputName = inputConnections[0].upstreamOutputName;

        // Check if we already created a separate2 node for this source
        std::pair<TfToken, TfToken> sourceKey(upstreamNodeName, upstreamOutputName);
        TfToken sep2NodeToken;

        auto it = separate2NodeCache->find(sourceKey);
        if (it != separate2NodeCache->end()) {
            // Reuse existing separate2 node
            sep2NodeToken = it->second;
        } else {
            // Create a new separate2 node with name based only on source
            // (multiple targets may share it)
            sep2NodeToken = TfToken(
                upstreamNodeName.GetString() + "_" +
                upstreamOutputName.GetString() + "_separate2");

            // Create the MaterialXSeparate2 node
            netInterface->SetNodeType(sep2NodeToken, _tokens->MaterialXSeparate2);

            // Connect the vector2 output to the separate2 input
            netInterface->SetNodeInputConnection(
                sep2NodeToken, _tokens->in,
                {{upstreamNodeName, upstreamOutputName}});

            // Cache the separate2 node for reuse
            (*separate2NodeCache)[sourceKey] = sep2NodeToken;
        }

        // Delete the original connection and create array-indexed connections
        netInterface->DeleteNodeInputConnection(nodeName, connectionName);

        const TfToken arrayParam0(connectionName.GetString() + "[0]");
        const TfToken arrayParam1(connectionName.GetString() + "[1]");

        netInterface->SetNodeInputConnection(
            nodeName, arrayParam0,
            {{sep2NodeToken, _tokens->outx}});

        netInterface->SetNodeInputConnection(
            nodeName, arrayParam1,
            {{sep2NodeToken, _tokens->outy}});
    }

    // Process parameter values (not connections)
    const TfTokenVector parameterNames =
        netInterface->GetAuthoredNodeParameterNames(nodeName);

    for (const TfToken &parameterName : parameterNames) {
        // Check if the shader expects array-indexed versions of this parameter
        if (!_NeedsVector2Split(sdrNode, parameterName)) {
            continue;
        }

        const VtValue paramValue = netInterface->GetNodeParameterValue(
            nodeName, parameterName);

        if (!paramValue.IsHolding<GfVec2f>()) {
            continue;
        }

        const GfVec2f vec2Value = paramValue.UncheckedGet<GfVec2f>();

        // Set the individual array-indexed parameter values
        const TfToken arrayParam0(parameterName.GetString() + "[0]");
        const TfToken arrayParam1(parameterName.GetString() + "[1]");

        netInterface->SetNodeParameterValue(
            nodeName, arrayParam0, VtValue(vec2Value[0]));
        netInterface->SetNodeParameterValue(
            nodeName, arrayParam1, VtValue(vec2Value[1]));

        // Delete the original vector2 parameter
        netInterface->DeleteNodeParameter(nodeName, parameterName);
    }
}

// Convert geomcolor nodes to geompropvalue nodes that look up "displayColor".
// geomcolor nodes look up "color" primvar by default in OSL, but USD's standard
// primvar naming uses "displayColor", so geomcolor nodes are likely to fail.
static void
_ConvertGeomColorNodes(
    mx::DocumentPtr const &mxDoc,
    mx::NodeGraphPtr const &mxNodeGraph)
{
    if (!mxNodeGraph) {
        return;
    }

    // Find geomcolor nodes in the node graph
    std::vector<mx::NodePtr> geomcolorNodes =
        mxNodeGraph->getNodes(_tokens->geomcolor);

    if (geomcolorNodes.empty()) {
        return;
    }

    // Get the nodedef for color3 geompropvalue
    const mx::NodeDefPtr mxNodeDef =
        HdMtlxGetNodeDef(_tokens->ND_geompropvalue_color3, mxDoc);

    if (!mxNodeDef) {
        return;
    }

    for (mx::NodePtr mxGeomColorNode : geomcolorNodes) {
        mxGeomColorNode->setType(mxNodeDef->getType());
        mxGeomColorNode->setCategory(mxNodeDef->getNodeString());
        mxGeomColorNode->setNodeDefString(_tokens->ND_geompropvalue_color3);

        // Set the geomprop input to "displayColor" (USD's standard primvar)
        mxGeomColorNode->setInputValue(_tokens->geomprop.GetText(),
                                       _tokens->displayColor.GetText(),
                                       _tokens->string_type.GetText());
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
    std::set<TfToken> nodesToRemove,
    // Held on entry and on return; released only around the oslc compile.
    std::unique_lock<std::mutex> &mtlxLock)
{
    // Cache for deduplicating MaterialXSeparate2 nodes
    // Maps (upstreamNodeName, upstreamOutputName) to separate2 node name
    std::map<std::pair<TfToken, TfToken>, TfToken> separate2NodeCache;

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

                if (!sdrNode) {
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
                (mxNode && !mxNode->getNodeDefString().empty())
                    ? TfToken(mxNode->getNodeDefString())
                    : netInterface->GetNodeType(upstreamNodeName);

            SdrShaderNodeConstPtr sdrMtlxNode =
                sdrRegistry.GetShaderNodeByIdentifierAndSystem(
                    nodeType, _tokens->mtlx);

            // Custom nodes do not use the nodeDefString as the identifier
            // make sure to look to the type indicated in the HdNetwork
            if (!sdrMtlxNode) {
                sdrMtlxNode = sdrRegistry.GetShaderNodeByIdentifierAndSystem(
                    netInterface->GetNodeType(upstreamNodeName), _tokens->mtlx);
            }

            if (!sdrMtlxNode ||
                (_GetMaterialBsdfNodeType(nodeType) != nodeType) ||
                TfStringEndsWith(nodeType.GetText(), _tokens->bsdf) ||
                TfStringEndsWith(nodeType.GetText(), _tokens->edf) ||
                TfStringEndsWith(nodeType.GetText(), _tokens->vdf) ||
                nodeType == _tokens->ND_surface) {
                _UpdateNetwork(netInterface, upstreamNodeName, mxDoc,
                               searchPath, nodesToKeep, nodesToRemove,
                               mtlxLock);

                // Convert node type from ND_ to MaterialX* type
                TfToken bsdfNodeType = _GetMaterialBsdfNodeType(
                    netInterface->GetNodeType(upstreamNodeName));
                netInterface->SetNodeType(upstreamNodeName, bsdfNodeType);

                // Split vector2 connections for bsdf
                SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
                SdrShaderNodeConstPtr sdrNode =
                    sdrRegistry.GetShaderNodeByIdentifier(bsdfNodeType);
                _SplitVector2ConnectionsForNode(netInterface, upstreamNodeName, sdrNode,
                                                &separate2NodeCache);
                continue;
            }

            // Collect nodes further removed from the terminal in nodesToRemove
            std::set<TfToken> tmpVisitedNodeNames;
            _GatherNodeGraphNodes(netInterface, upstreamNodeName,
                                  &nodesToRemove, &tmpVisitedNodeNames);
            nodesToKeep.insert(upstreamNodeName);

            // Convert geomcolor nodes to geompropvalue nodes
            _ConvertGeomColorNodes(mxDoc, mxNodeGraph);

            // Generate the oslSource code for the connected upstream node
            std::string shaderName = mxNodeName + "_Shader";
            // Triple leading underscores are not allowed in OSL
            if (TfStringStartsWith(shaderName, "___")) {
                shaderName = "mtlx" + shaderName.substr(2);
            }
            std::string oslSource = _GenMaterialXShaderCode(
                mxDoc, searchPath, shaderName, mxNodeName, mxNodeGraphName);

            if (oslSource.empty()) {
                continue;
            }

            // Compile the oslSource.  oslc runs out of process and doesn't
            // touch MaterialX, so don't hold the lock while it runs.
            mtlxLock.unlock();
            std::string compiledShaderPath =
                _CompileOslSource(shaderName, oslSource, searchPath);
            mtlxLock.lock();

            if (compiledShaderPath.empty()) {
                continue;
            }

            // Create a new SdrShaderNode with the compiled oslSource
            SdrShaderNodeConstPtr sdrNode = 
                sdrRegistry.GetShaderNodeFromAsset(
                                SdfAssetPath(compiledShaderPath),
                                SdrTokenMap(),  // metadata
                                _tokens->mtlx,  // subId
                                _tokens->OSL);  // shading system

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
                // Rename inputs that conflict with OSL reserved words
                // (e.g. "normal" -> "normalIn"), but only for nodes
                // that will become OSL adapter nodes. MaterialX BSDF
                // nodes are C++ plugins, not OSL, so the reserved-word
                // conflict doesn't apply; they use sdrDefinitionName
                // to map "normal" -> "shadingNormal" instead.
                bool deletePreviousConnection = false;
                TfToken const terminalType =
                    netInterface->GetNodeType(terminalNodeName);
                if (_GetMaterialBsdfNodeType(terminalType) == terminalType) {
                    TfToken updatedInputName =
                        _GetUpdatedInputToken(inputName);
                    if (!updatedInputName.IsEmpty()) {
                        inputName = updatedInputName;
                        deletePreviousConnection = true;
                    }
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
        // Decompose the ND_surface into its bsdf/edf components.
        if (nodeType == _tokens->ND_surface) {
            TfToken resultNode = _DecomposeMtlxSurface(
                netInterface, terminalNodeName);
            if (!resultNode.IsEmpty()) {
                netInterface->SetTerminalConnection(
                    terminalToken, {resultNode, TfToken()});
                netInterface->DeleteNode(terminalNodeName);
            } else {
                // Nothing connected — retype to a black diffuse bsdf
                // to avoid warning about ND_surface, while keeping
                // the node for reconnection on subsequent syncs.
                netInterface->SetNodeType(
                    terminalNodeName, _tokens->MaterialXBurleyDiffuse);
                netInterface->SetNodeParameterValue(
                    terminalNodeName, _tokens->color,
                    VtValue(GfVec3f(0.0f, 0.0f, 0.0f)));
            }
        }
        // Houdini's visualize VOP for mtlx:
        // ND_surface_unlit -> PxrConstant
        // emission_color -> emitColor
        else if (nodeType == _tokens->ND_surface_unlit) {
            netInterface->SetNodeType(terminalNodeName, _tokens->PxrConstant);
            auto conns = netInterface->GetNodeInputConnection(terminalNodeName, _tokens->emission_color);
            if (!conns.empty()) {
                netInterface->DeleteNodeInputConnection(terminalNodeName, _tokens->emission_color);
                netInterface->SetNodeInputConnection(terminalNodeName, _tokens->emitColor, conns);
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
        HdMtlxGetNodeDef(TfToken(mtlxSdrNode->GetIdentifier()), mxDoc);
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
#if MTLX_COMBINED_VERSION < 13900
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
            sdrRegistry.GetShaderNodeByIdentifierAndSystem(
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
static void
_FixNodeNames(
    HdMaterialNetworkInterface *netInterface)
{
    SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
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
            // Only rename if the target node definition exists
            if (sdrRegistry.GetShaderNodeByIdentifier(nodeType)) {
                netInterface->SetNodeType(nodeName, nodeType);
            }
        }
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
        std::string mxTextureNodeName =
                HdMtlxCreateNameFromPath(texturePath);
        const TfToken nodeType = netInterface->GetNodeType(textureNodeName);
        if (nodeType.IsEmpty()) {
            TF_WARN("Connot find texture node '%s' in material network.",
                    textureNodeName.GetText());
            continue;
        }
        // Get the filename parameter names, 
        // MaterialX stdlib nodes use 'file' however, this could be different
        // for custom nodes that use textures.
        std::vector<TfToken> fileParamNames;
        const mx::NodeDefPtr nodeDef = HdMtlxGetNodeDef(nodeType, mxDoc);
        if (nodeDef) {
            for (auto const& mxInput : nodeDef->getActiveInputs()) {
                if (mxInput->getType() == _tokens->filename) {
                    fileParamNames.push_back(TfToken(mxInput->getName()));
                }
            }

        }

        for(auto fileParamName : fileParamNames) {
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

                // If texcoord param is missing from the nodedef it isn't valid.
                // Don't add it to the node, or shader compile will fail.
                if(!nodeDef->getInput(texCoordToken)) {
                    return;
                }

                // If texcoord param isn't connected, make a default connection
                // to a mtlx geompropvalue node.
                mx::InputPtr texcoordInput =
                    mxTextureNode->getInput(texCoordToken);
                if(!texcoordInput) {

                    // Get the sdr node for the mxTexture node
                    SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
                    const SdrShaderNodeConstPtr sdrTextureNode =
                        sdrRegistry.GetShaderNodeByIdentifierAndSystem(
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
        const mx::NodeDefPtr mxNodeDef =
            HdMtlxGetNodeDef(_tokens->ND_geompropvalue_vector2, mxDoc);
        if (!mxNodeDef) {
            continue;
        }

        // Get the sdr node for the texcoord node
        SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
        const SdrShaderNodeConstPtr sdrTexcoordNode =
            sdrRegistry.GetShaderNodeByIdentifierAndSystem(
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

static mx::FileSearchPath
_HdPrmanMtlxSearchPaths()
{
    mx::FileSearchPath searchPaths = HdMtlxSearchPaths();
#ifdef HDPRMAN_MTLX_SEARCH_PATHS
    for (const std::string& path :
            TfStringSplit(HDPRMAN_MTLX_SEARCH_PATHS, " ")) {
        std::string expanded = ArchExpandEnvironmentVariables(path);
        if (!expanded.empty()) {
            searchPaths.append(mx::FilePath(expanded));
        }
    }
#endif
    return searchPaths;
}

static mx::DocumentPtr
_HdPrmanMtlxStdLibraries()
{
    mx::FilePathVec libraryFolders;
    mx::FileSearchPath searchPaths = _HdPrmanMtlxSearchPaths();
    mx::DocumentPtr stdLibraries = mx::createDocument();
    mx::loadLibraries(libraryFolders, searchPaths, stdLibraries);
    return stdLibraries;
}

// The mtlx impementation of a surface node is a component graph
// that is traversed and added to the the hydra shading network here.
// Returns set of node names created during expansion.
static std::set<TfToken>
_ExpandNodeImplementationGraph(
    HdMaterialNetworkInterface *netInterface,
    mx::DocumentPtr const &mxDoc,
    const mx::NodePtr &surfaceNode,
    const SdfPath &materialPath,
    const TfToken &terminalNodeName,
    TfToken &newTerminalNodeName)
{
    mx::InterfaceElementPtr impl = surfaceNode->getImplementation();

    std::set<TfToken> createdNodes;

    if (impl && impl->isA<mx::NodeGraph>()) {
        mx::NodeGraphPtr nodegraph =
            impl->asA<mx::NodeGraph>();
        for (auto n : nodegraph->getNodes()) {
            // Add each node from the implementation graph to the hd network,
            // and rewire inputs from original surface node to the new nodes.
            // Prefix the node name with the original node name to ensure uniqueness
            // when multiple nodes are expanded from the same implementation graph.
            // For example, LamaDiffuse2 and LamaDiffuse3 both create "oren_nayar", so we
            // need to create "LamaDiffuse2_oren_nayar" and "LamaDiffuse3_oren_nayar".
            std::string uniqueNodeName = terminalNodeName.GetString() + "_" + n->getName();
            TfToken hdShaderNodeName = TfToken(uniqueNodeName);
            // The nodedef resolves against the doc holding the MaterialX
            // libraries; skip the node rather than crash if it is missing.
            if (!n->getNodeDef()) {
                TF_WARN("No MaterialX nodedef for node '%s' (category '%s') in "
                        "the implementation graph of '%s'.",
                        n->getName().c_str(), n->getCategory().c_str(),
                        surfaceNode->getName().c_str());
                continue;
            }            
            // Build the nodedef type string (e.g., "ND_add_float", "ND_extract_color3")
            // Start with base: ND_<nodeString>
            std::string nodedefType = "ND_" + n->getNodeDef()->getNodeString();

            // Try to find it as-is first
            mx::NodeDefPtr nodeDefFromDoc = HdMtlxGetNodeDef(TfToken(nodedefType), mxDoc);

            // Track if we need to convert float to color for bsdf multiply weight
            bool convertFloatToColor = false;

            if (!nodeDefFromDoc) {
                // Nodedef not found with base name, need to add type suffixes
                // Different node types have different suffix patterns:
                // - ND_extract: input type only (e.g., ND_extract_color3)
                // - ND_convert: input + output type (e.g., ND_convert_float_color3)
                // - Others: output type only (e.g., ND_add_float)

                std::string outputType = n->getNodeDef()->getType();
                std::transform(outputType.begin(), outputType.end(), outputType.begin(),
                               [](unsigned char c){ return std::tolower(c); });

                // Add suffixes if nodedefType is still the base form,
                // ie. no underscores found after "ND_"
                bool isBaseForm = (nodedefType.find('_', 3) == std::string::npos);

                if (isBaseForm) {
                    if (nodedefType == _tokens->ND_extract) {
                        // ND_extract needs input type only
                        auto inputs = n->getInputs();
                        if (inputs.size()) {
                            nodedefType += "_" + inputs[0]->getType();
                        }
                    } else if (nodedefType == _tokens->ND_convert) {
                        // ND_convert needs both input and output type
                        auto inputs = n->getInputs();
                        if (inputs.size()) {
                            nodedefType += "_" + inputs[0]->getType() + "_" + outputType;
                        }
                    } else if (nodedefType.find(outputType) == std::string::npos) {
                        // other nodes need output type only
                        nodedefType += "_" + outputType;
                    }
                }

                // Special case for ND_multiply nodes where in2 is float
                // MaterialXMultiply only supports color inputs, so flag for conversion
                if (nodedefType.find(_tokens->ND_multiply) == 0) {
                    mx::InputPtr in2Input = n->getInput(_tokens->in2);
                    if (in2Input && in2Input->getType() == _tokens->_float) {
                        convertFloatToColor = true;
                    }
                }

                nodeDefFromDoc = mxDoc->getNodeDef(nodedefType);
            }

            TfToken nodedefTypeTok = TfToken(nodedefType.c_str());
            TfToken hdShaderType = nodedefTypeTok;
            netInterface->SetNodeType(hdShaderNodeName, hdShaderType);
            createdNodes.insert(hdShaderNodeName);
            newTerminalNodeName = hdShaderNodeName;
            auto inputs = n->getActiveInputs();

            // Deal with authored params. These may be connections or values.
            // Create the equivalent connection or set value in the hd network.
            for (auto input : inputs) {
                auto cn = input->getConnectedNode();
                // Interface name is the param name on the surface node
                // (eg. MtlxStandardSurface)
                bool needDefaultVal = true;
                if (input->hasInterfaceName()) {
                    // Copy the value or connection from the hd node
                    // for the original surface to the matching input
                    // on the new hd node from the component graph.
                    TfToken interfaceParamNm(
                        input->getInterfaceName().c_str());
                    TfToken paramNm(input->getName().c_str());
                    auto inputConnections =
                        netInterface->GetNodeInputConnection(
                        terminalNodeName, interfaceParamNm);
                    if (inputConnections.size()) {
                        // For bsdf multiply with float weight connection,
                        // insert a convert node to transform float to color
                        if (convertFloatToColor &&
                            paramNm == _tokens->in2) {
                            std::string convertNodeName =
                                hdShaderNodeName.GetString() + "_in2_convert";
                            TfToken convertNodeTok(convertNodeName);
                            netInterface->SetNodeType(
                                convertNodeTok,
                                _tokens->ND_convert_float_color3);
                            createdNodes.insert(convertNodeTok);
                            netInterface->SetNodeInputConnection(
                                convertNodeTok,
                                _tokens->in,
                                inputConnections);
                            netInterface->SetNodeInputConnection(
                                hdShaderNodeName,
                                paramNm,
                                {{convertNodeTok, _tokens->out}});
                        } else {
                            netInterface->
                                SetNodeInputConnection(hdShaderNodeName,
                                                       paramNm,
                                                       inputConnections);
                        }
                        needDefaultVal = false;
                    } else {
                        VtValue val = netInterface->GetNodeParameterValue(
                            terminalNodeName, interfaceParamNm);
                        if (!val.IsEmpty()) {
                            // Convert float to color for bsdf multiply weight
                            if (convertFloatToColor &&
                                paramNm == _tokens->in2 &&
                                val.IsHolding<float>()) {
                                float f = val.UncheckedGet<float>();
                                val = VtValue(GfVec3f(f, f, f));
                            }
                            netInterface->
                                SetNodeParameterValue(hdShaderNodeName,
                                                      paramNm, val);
                            needDefaultVal = false;
                        }
                    }
                }

                if(needDefaultVal) {
                    // Get default value from the surface definition if present.
                    // In other words, interface parameters for a surface
                    // have defaults that can be queried from the node def,
                    // but other non-interface params on component nodes
                    // will just use their usual default values.
                    auto mxval =
                        (surfaceNode->getNodeDef()->
                         getActiveInput(input->getInterfaceName())) ?
                        surfaceNode->getNodeDef()->
                        getActiveInput(input->getInterfaceName())->getValue() :
                        input->getValue();

                    // Translate mtlx value to a VtValue for the hd network
                    if(mxval) {
                        TfToken paramNm(input->getName().c_str());
                        VtValue val;
                        if(mxval->isA<mx::Color3>()) {
                            const mx::Color3 &cval = mxval->asA<mx::Color3>();
                            val = VtValue(GfVec3f(cval[0], cval[1], cval[2]));
                        } else if(mxval->isA<float>()) {
                            const float &fval = mxval->asA<float>();
                            // Convert float to color for bsdf multiply weight
                            if (convertFloatToColor &&
                                paramNm == _tokens->in2) {
                                val = VtValue(GfVec3f(fval, fval, fval));
                            } else {
                                val = VtValue(fval);
                            }
                        } else if(mxval->isA<std::string>()) {
                            const std::string &sval = mxval->asA<std::string>();
                            val = VtValue(sval.c_str());
                        } else if(mxval->isA<bool>()) {
                            const bool &bval = mxval->asA<bool>();
                            val = VtValue(bval);
                        } else if(mxval->isA<int>()) {
                            const int &ival = mxval->asA<int>();
                            val = VtValue(ival);
                        } else if(mxval->isA<mx::Vector2>()) {
                            const mx::Vector2 &v = mxval->asA<mx::Vector2>();
                            val = VtValue(GfVec2f(v[0], v[1]));
                        }
                        netInterface->SetNodeParameterValue(hdShaderNodeName,
                                                            paramNm, val);
                    }
                }
                if (cn) {
                    TfToken hdUpstreamNodeOutputName =
                        cn->getNodeDefOutput(input)
                        ? TfToken(
                            (cn->getNodeDefOutput(input)->getName().c_str()))
                        : _tokens->out;
                    // Prefix the upstream node name to match how we created it
                    std::string upstreamUniqueName = terminalNodeName.GetString() + "_" + cn->getName();
                    TfToken hdUpstreamNodeName(upstreamUniqueName);
                    TfToken hdInputName(input->getName().c_str());
                    netInterface->SetNodeInputConnection(
                        hdShaderNodeName, hdInputName,
                        {{hdUpstreamNodeName, hdUpstreamNodeOutputName}});
                }
            }
        }
    }

    return createdNodes;
}

// Update node connections after expansion, replacing references to old
// nodes with their expanded replacements.
static void
_UpdateNodeConnectionsAfterExpansion(
    HdMaterialNetworkInterface *netInterface,
    const TfTokenVector &nodeNames,
    const std::map<TfToken, TfToken> &nodeReplacements)
{
    for (const TfToken &nodeName : nodeNames) {
        TfTokenVector inputNames = netInterface->GetNodeInputConnectionNames(nodeName);
        for (const TfToken &inputName : inputNames) {
            auto connections = netInterface->GetNodeInputConnection(nodeName, inputName);

            bool needsUpdate = false;
            HdMaterialNetworkInterface::InputConnectionVector updatedConnections;

            for (const auto &conn : connections) {
                auto it = nodeReplacements.find(conn.upstreamNodeName);
                if (it != nodeReplacements.end()) {
                    HdMaterialNetworkInterface::InputConnection newConn = conn;
                    newConn.upstreamNodeName = it->second;
                    updatedConnections.push_back(newConn);
                    needsUpdate = true;
                } else {
                    updatedConnections.push_back(conn);
                }
            }

            if (needsUpdate) {
                netInterface->SetNodeInputConnection(nodeName, inputName, updatedConnections);
            }
        }
    }
}

// Expand implementation graphs for all bsdf nodes in the network that have them.
static void
_ExpandAllImplementationGraphs(
    HdMaterialNetworkInterface *netInterface,
    mx::DocumentPtr const &mxDoc,
    const mx::NodePtr &surfaceNode ,
    const SdfPath &materialPath,
    const TfToken &terminalNodeName,
    TfToken &newTerminalNodeName)
{
    if(!surfaceNode) {
        return;
    }
    // Get all node names in the network
    TfTokenVector allNodeNames = netInterface->GetNodeNames();

    // Track node replacements: old node name -> new terminal node name
    std::map<TfToken, TfToken> nodeReplacements;

    // Track nodes created during expansion (since GetNodeNames() doesn't
    // include nodes created via SetNodeType during this processing)
    std::set<TfToken> allCreatedNodes;

    // Expand any bsdf nodes that may need expansion,
    // such as Lama nodes implemented via mtlx components.
    for (const TfToken &hdNodeName : allNodeNames) {
       // Find corresponding MaterialX node
        SdfPath nodePath(hdNodeName);
        std::string mxNodeName = HdMtlxCreateNameFromPath(nodePath);

        // Try to find the node in the MaterialX document
        mx::NodePtr mxNode = mxDoc->getNode(mxNodeName);
        if (!mxNode) {
            // Node might be in a node graph
            for (auto g : mxDoc->getNodeGraphs()) {
                mxNode = g->getNode(mxNodeName);
                if (mxNode) break;
            }
        }
        if (!mxNode) {
            continue;
        }

        // Only expand bsdf nodes, not pattern nodes
        // Pattern nodes (image, noise, etc.) will be converted to OSL shaders by _UpdateNetwork
        // bsdf nodes need to be expanded here to get proper MaterialX* types
        std::string outputType = mxNode->getType();
        bool isBsdfNode = (outputType == _tokens->BSDF ||
                           outputType == _tokens->EDF ||
                           outputType == _tokens->VDF ||
                           outputType == _tokens->surfaceshader);

        if (!isBsdfNode) {
            continue;
        }

        TfToken newNodeName;
        std::set<TfToken> createdNodes = _ExpandNodeImplementationGraph(
            netInterface, mxDoc, mxNode,
            materialPath, hdNodeName, newNodeName);

        if (!newNodeName.IsEmpty()) {
            // If the new terminal node is an ND_surface, decompose it
            // into its bsdf/edf components.
            TfToken newNodeType = netInterface->GetNodeType(newNodeName);
            if (newNodeType == _tokens->ND_surface) {
                // Decompose the ND_surface into its bsdf/edf components.
                TfToken resultNode = _DecomposeMtlxSurface(
                    netInterface, newNodeName);
                if (!resultNode.IsEmpty()) {
                    nodeReplacements[hdNodeName] = resultNode;
                    allCreatedNodes.insert(resultNode);
                    netInterface->DeleteNode(newNodeName);
                    allCreatedNodes.erase(newNodeName);
                } else {
                    // Nothing connected — retype to a black diffuse bsdf
                    // so Riley receives a valid shader while keeping the
                    // node for reconnection on subsequent syncs.
                    netInterface->SetNodeType(
                        newNodeName, _tokens->MaterialXBurleyDiffuse);
                    netInterface->SetNodeParameterValue(
                        newNodeName, _tokens->color,
                        VtValue(GfVec3f(0.0f, 0.0f, 0.0f)));
                    nodeReplacements[hdNodeName] = newNodeName;
                }
            } else {
                nodeReplacements[hdNodeName] = newNodeName;
            }
        }

        // Accumulate all created nodes
        allCreatedNodes.insert(createdNodes.begin(), createdNodes.end());
    }

    // The terminal node exists in graph with name "Surface",
    // rather than with the hd node name.
    std::set<TfToken> terminalCreatedNodes = _ExpandNodeImplementationGraph(
        netInterface, mxDoc, surfaceNode,
        materialPath, terminalNodeName, newTerminalNodeName);

    if (!newTerminalNodeName.IsEmpty() && newTerminalNodeName != terminalNodeName) {
        nodeReplacements[terminalNodeName] = newTerminalNodeName;
    }

    // Accumulate terminal expansion created nodes
    allCreatedNodes.insert(terminalCreatedNodes.begin(), terminalCreatedNodes.end());

    // Update connections to point to the new nodes after expansion.
    // We need to iterate through all nodes and update any connections that
    // reference an old node to point to its replacement.
    if (!nodeReplacements.empty()) {
        TfTokenVector nodesToUpdate = netInterface->GetNodeNames();

        // Add all created nodes to the update list
        for (const TfToken &createdNode : allCreatedNodes) {
            nodesToUpdate.push_back(createdNode);
        }

        _UpdateNodeConnectionsAfterExpansion(netInterface, nodesToUpdate, nodeReplacements);

        // After updating all connections, delete the old nodes that were expanded.
        for (const auto &replacement : nodeReplacements) {
            netInterface->DeleteNode(replacement.first);
        }
    }
}

// This should be called before
// HdMtlxCreateMtlxDocumentFromHdMaterialNetworkInterface and _UpdateNetwork,
// which convert the pattern portions of the graph to osl.
void
_TransformTerminalNodeToImplementationGraph(
    HdMaterialNetworkInterface *netInterface,
    mx::DocumentPtr const &mxDoc,
    const mx::NodePtr &surfaceNode ,
    const SdfPath &materialPath,
    const TfToken &terminalNodeName,
    TfToken &newTerminalNodeName
)
{
    _ExpandAllImplementationGraphs(
        netInterface, mxDoc, surfaceNode,
        materialPath, terminalNodeName, newTerminalNodeName
        );

    // Update hd network with the new terminal node from the implementation
    // graph that has just been added in place of the original surface node.
    // Note, the new terminal node will typically be an ND_surface
    // which ends up getting skipped over in _TransformTerminalNode,
    // because XPU wants the connected bsdf node.
    TfToken const terminalToken =
        _GetTerminalConnectionName(newTerminalNodeName);
    netInterface->SetTerminalConnection(terminalToken,
                                        {newTerminalNodeName, TfToken()});
}


// Bypass MaterialXLayer nodes whose 'base' input connects to a VDF node.
// XPU cannot resolve VDF closures, which causes the entire layer to output
// black. Replace references to the layer with its 'top' input (the BSDF).
// If terminalNodeName points to a bypassed layer, it is updated in place.
static void
_BypassVdfLayers(
    HdMaterialNetworkInterface *netInterface,
    std::set<TfToken> *nodesToRemove,
    TfToken *terminalNodeName)
{
    // Gather all nodes reachable upstream from the terminal
    std::set<TfToken> allNodes;
    if (terminalNodeName && !terminalNodeName->IsEmpty()) {
        std::set<TfToken> visited;
        _GatherNodeGraphNodes(netInterface, *terminalNodeName,
                              &allNodes, &visited);
        allNodes.insert(*terminalNodeName);
    }

    // Collect replacements: layer node -> top BSDF node
    std::map<TfToken, TfToken> nodeReplacements;
    std::set<TfToken> vdfNodesToRemove;

    for (const TfToken &nodeName : allNodes) {
        TfToken nodeType = netInterface->GetNodeType(nodeName);
        if (nodeType != _tokens->MaterialXLayer) {
            continue;
        }

        // Check if 'base' input connects to a VDF node
        auto baseConns = netInterface->GetNodeInputConnection(
            nodeName, _tokens->base);
        if (baseConns.empty()) {
            continue;
        }

        const TfToken &baseNodeName = baseConns[0].upstreamNodeName;
        TfToken baseNodeType = netInterface->GetNodeType(baseNodeName);

        if (!TfStringEndsWith(baseNodeType.GetText(),
                              _tokens->vdf.GetText())) {
            continue;
        }

        // Found a layer(top=BSDF, base=VDF) -- bypass it.
        auto topConns = netInterface->GetNodeInputConnection(
            nodeName, _tokens->top);

        if (topConns.empty()) {
            TF_WARN("MaterialXLayer node '%s' has VDF base but no 'top' "
                    "connection; cannot bypass.",
                    nodeName.GetText());
            continue;
        }

        nodeReplacements[nodeName] = topConns[0].upstreamNodeName;
        vdfNodesToRemove.insert(baseNodeName);
    }

    if (nodeReplacements.empty()) {
        return;
    }

    // Update terminal node name if it was bypassed
    auto it = nodeReplacements.find(*terminalNodeName);
    if (it != nodeReplacements.end()) {
        *terminalNodeName = it->second;
    }

    // Rewire all downstream connections to point past bypassed layers
    TfTokenVector nodesToUpdate(allNodes.begin(), allNodes.end());
    _UpdateNodeConnectionsAfterExpansion(
        netInterface, nodesToUpdate, nodeReplacements);

    // Delete bypassed layer nodes and VDF nodes
    for (const auto &replacement : nodeReplacements) {
        netInterface->DeleteNode(replacement.first);
    }
    for (const TfToken &vdfNode : vdfNodesToRemove) {
        netInterface->DeleteNode(vdfNode);
        if (nodesToRemove) {
            nodesToRemove->erase(vdfNode);
        }
    }
}


// Bypass generalized_schlick_edf, which modulates the emission on its 'base'
// input.  RenderMan has no EDF form of schlick yet, so replace references
// to it with the emission it was modulating; the Fresnel falloff is lost,
// but that only matters when the emission is non-zero.
static void
_BypassSchlickEdf(
    HdMaterialNetworkInterface *netInterface,
    std::set<TfToken> *nodesToRemove,
    TfToken *terminalNodeName)
{
    if (!terminalNodeName || terminalNodeName->IsEmpty()) {
        return;
    }

    // Walk the network; GetNodeNames() omits the nodes just created by
    // expansion.
    std::set<TfToken> allNodes, visited;
    _GatherNodeGraphNodes(netInterface, *terminalNodeName, &allNodes, &visited);
    allNodes.insert(*terminalNodeName);

    // schlick edf node -> the emission on its base input
    std::map<TfToken, TfToken> nodeReplacements;
    for (const TfToken &nodeName : allNodes) {
        if (netInterface->GetNodeType(nodeName) !=
                _tokens->MaterialXEmissionGeneralizedSchlick) {
            continue;
        }
        auto baseConns = netInterface->GetNodeInputConnection(
            nodeName, _tokens->base);
        if (baseConns.empty()) {
            // An unconnected 'base' is legal mtlx (no emission) but leaves us
            // nothing to bypass to.  Retype to a black emission so Riley
            // receives a resolvable shader that emits nothing; the marker type
            // would fail Sdr lookup and drop the whole material.
            TF_WARN("generalized_schlick_edf node '%s' has no 'base' "
                    "connection; cannot bypass.", nodeName.GetText());
            netInterface->SetNodeType(
                nodeName, _tokens->MaterialXEmissionUniform);
            netInterface->SetNodeParameterValue(
                nodeName, _tokens->color,
                VtValue(GfVec3f(0.0f, 0.0f, 0.0f)));
            // Keep it; the caller's sweep would otherwise delete it and
            // leave the downstream connection dangling.
            if (nodesToRemove) {
                nodesToRemove->erase(nodeName);
            }
            continue;
        }
        nodeReplacements[nodeName] = baseConns[0].upstreamNodeName;
    }

    if (nodeReplacements.empty()) {
        return;
    }

    auto it = nodeReplacements.find(*terminalNodeName);
    if (it != nodeReplacements.end()) {
        *terminalNodeName = it->second;
    }

    _UpdateNodeConnectionsAfterExpansion(
        netInterface, TfTokenVector(allNodes.begin(), allNodes.end()),
        nodeReplacements);

    for (const auto &replacement : nodeReplacements) {
        netInterface->DeleteNode(replacement.first);
        if (nodesToRemove) {
            nodesToRemove->erase(replacement.first);
        }
    }
}

void
MatfiltMaterialX(
    HdMaterialNetworkInterface *netInterface,
    bool enableImplementationGraph,
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

    bool expandImplementationGraph = enableImplementationGraph &&
        TfGetEnvSetting(HD_PRMAN_ENABLE_IMPLEMENTATION_GRAPH);

    for (auto terminalName : supportedTerminalTokens ) {

        // Check presence of terminal
        const HdMaterialNetworkInterface::InputConnectionResult res =
            netInterface->GetTerminalConnection(terminalName);
        if (!res.first) { // terminal absent, skip
            continue;
        }
        const TfToken &terminalNodeName = res.second.upstreamNodeName;

        // Check if the network uses any Mtlx nodes, and return early if not.
        // The terminal node may be Mtlx, but we also want to support
        // using mtlx patterns with Usd, Pxr or Lama materials.
        if (!_NetworkHasMtlxNodes(netInterface)) {
            return;
        }
        TfToken newTerminalNodeName = terminalNodeName;
        TfTokenVector cNames =
            netInterface->GetNodeInputConnectionNames(terminalNodeName);
        // If we have a nodegraph (i.e., input into the terminal node)...
        // Or, for XPU, we may need to expand terminal node
        // even if it doesn't have connections.
        if (!cNames.empty() || expandImplementationGraph) {
            // Serialize MaterialX usage. stdLibraries below and
            // HdMtlxStdLibraries() are shared across SyncAll worker threads,
            // and MaterialX does not lock its element tree:
            // Element keeps  children and attributes in plain
            // unordered_maps, so a write on one thread can rehash a map that
            // another is reading.
            //
            // _UpdateNetwork() releases this lock around the out-of-process
            // oslc compile, so use unique_lock rather than lock_guard.
            static std::mutex materialXMutex;
            std::unique_lock<std::mutex> mtlxLock(materialXMutex);

            // Get Standard Libraries and SearchPaths (for mxDoc and 
            // mxShaderGen)
            static const mx::DocumentPtr stdLibraries = _HdPrmanMtlxStdLibraries();
            static const mx::FileSearchPath searchPaths = _HdPrmanMtlxSearchPaths();

            // Preprocess node network, converting UsdUvTexture, and
            // related nodes to their mtlx definition nodes.
            _FixNodeNames(netInterface);

            // Create the MaterialX Document from the material network
            HdMtlxTexturePrimvarData hdMtlxData;
            mx::DocumentPtr mxDoc =
                HdMtlxCreateMtlxDocumentFromHdMaterialNetworkInterface(
                    netInterface, terminalNodeName, cNames,
                    stdLibraries, &hdMtlxData);

            // Remove the material and shader nodes from the MaterialX Document
            // (since we need to use PxrSurface as the closure instead of the 
            // MaterialX surfaceshader node)
            SdfPath materialPath = netInterface->GetMaterialPrimPath();
            if(!expandImplementationGraph) {
                mxDoc->removeNode(                                  // Shader Node
                    HdMtlxGetMxTerminalName(netInterface, terminalNodeName));
            }
            mxDoc->removeNode(materialPath.GetName());          // Material Node

            if (expandImplementationGraph && mxDoc) {
                mx::NodeGraphPtr sg =
                    mxDoc->getNodeGraph(materialPath.GetName());
                if (!sg) {
                    for (auto g : mxDoc->getNodeGraphs()) {
                        if (TfStringStartsWith(g->getName(),
                                               materialPath.GetName())) {
                            sg = g;
                            break;
                        }
                    }
                }
                // hdMtlx adds a shader node called "Surface";
                mx::NodePtr surfaceNode = mxDoc->getNode(_tokens->Surface);

                if (surfaceNode) {
                    // Translate the mtlx implementation network for
                    // the terminal shader node into the hd network, and then
                    // proceed as we usually do for any other hd network.
                    _TransformTerminalNodeToImplementationGraph(
                        netInterface, mxDoc, surfaceNode, materialPath,
                        terminalNodeName, newTerminalNodeName);

                    // Recreate the MaterialX document from the updated network
                    // now that implementation graphs have been expanded.
                    // Note: We reuse stdLibraries from outer scope, and we're
                    // already holding materialXMutex from the outer lock.
                    cNames = netInterface->GetNodeInputConnectionNames(
                        newTerminalNodeName);
                    mxDoc =
                        HdMtlxCreateMtlxDocumentFromHdMaterialNetworkInterface(
                            netInterface, newTerminalNodeName, cNames,
                            stdLibraries, &hdMtlxData);
                }
            }
            _UpdateTextureNodes(netInterface, hdMtlxData.hdTextureNodes, mxDoc);
            _UpdatePrimvarNodes(netInterface, hdMtlxData.hdPrimvarNodes, mxDoc);

            // Update nodes directly connected to the terminal node with
            // MX generated shaders that capture the rest of the nodegraph
            _UpdateNetwork(netInterface, newTerminalNodeName, mxDoc, searchPaths,
                           nodesToKeep, nodesToRemove, mtlxLock);

            // Ensure the terminal node gets converted if it's a bsdf
            TfToken terminalNodeType = netInterface->GetNodeType(newTerminalNodeName);
            TfToken terminalBsdfType = _GetMaterialBsdfNodeType(terminalNodeType);
            if (terminalBsdfType != terminalNodeType) {
                netInterface->SetNodeType(newTerminalNodeName, terminalBsdfType);
            }

            // Bypass layer nodes with VDF base (XPU doesn't support VDF
            // closures yet, causing the layer to output black)
            _BypassVdfLayers(netInterface, &nodesToRemove,
                             &newTerminalNodeName);

            // Bypass Schlick EDF, which has no RenderMan implementation
            _BypassSchlickEdf(netInterface, &nodesToRemove,
                              &newTerminalNodeName);
        }

        // Convert the terminal node to an AdapterNode + PxrSurfaceNode
        _TransformTerminalNode(netInterface, newTerminalNodeName);
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
