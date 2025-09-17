//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdMtlx/hdMtlx.h"
#include "pxr/imaging/hdMtlx/debugCodes.h"
#include "pxr/imaging/hdMtlx/tokens.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/materialNetwork2Interface.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/debug.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdr/registry.h"

#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/trace/trace.h"

#include "pxr/usd/usdMtlx/utils.h"

#include <MaterialXCore/Document.h>
#include <MaterialXCore/Node.h>
#include <MaterialXFormat/Environ.h>
#include <MaterialXFormat/Util.h>
#include <MaterialXFormat/XmlIo.h>

#include <regex>

namespace mx = MaterialX;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (texcoord)
    (geompropvalue)
    (filename)
    (ND_surface)
    (typeName)
    (mtlx)
    ((mtlxVersion, "mtlx:version"))
);

TF_DEFINE_PRIVATE_TOKENS(
    _usdTypeTokens,
    ((boolType, "bool"))
    ((intType, "int"))
    (intarray)
    ((floatType, "float"))
    (floatarray)
    (color3f)
    (color3fArray)
    (color4f)
    (color4fArray)
    (float2)
    (float2Array)
    (float3)
    (float3Array)
    (float4)
    (float4Array)
    (matrix3d)
    (matrix4d)
    (asset)
    (string)
    (stringArray)
);

static mx::FileSearchPath
_ComputeSearchPaths()
{
    mx::FileSearchPath searchPaths;
    static const SdrStringVec searchPathStrings = UsdMtlxSearchPaths();
    for (auto path : searchPathStrings) {
        searchPaths.append(mx::FilePath(path));
    }
#ifdef PXR_DCC_LOCATION_ENV_VAR
    const std::string dccLocationEnvVar(PXR_DCC_LOCATION_ENV_VAR);
    const std::string dccLocation = mx::getEnviron(dccLocationEnvVar);
    searchPaths.append(mx::FilePath(dccLocation + PXR_MATERIALX_STDLIB_DIR));
    searchPaths.append(mx::FilePath(dccLocation + PXR_MATERIALX_BASE_DIR));
#endif
    return searchPaths;
}

const mx::FileSearchPath&
HdMtlxSearchPaths()
{
    static const mx::FileSearchPath searchPaths = _ComputeSearchPaths();
    return searchPaths;
}

static mx::DocumentPtr
_ComputeStdLibraries()
{
    mx::FilePathVec libraryFolders;
    mx::DocumentPtr stdLibraries = mx::createDocument();
    mx::loadLibraries(libraryFolders, HdMtlxSearchPaths(), stdLibraries);
    return stdLibraries;
}

const mx::DocumentPtr&
HdMtlxStdLibraries()
{
    static const mx::DocumentPtr stdLibraries = _ComputeStdLibraries();
    return stdLibraries;
}

// Return the MaterialX Node string with the namespace prepended when present
static std::string
_GetMxNodeString(mx::NodeDefPtr const& mxNodeDef)
{    
    // If the nodedef is in a namespace, add it to the node string 
    return mxNodeDef->hasNamespace()
        ? mxNodeDef->getNamespace() + ":" + mxNodeDef->getNodeString()
        : mxNodeDef->getNodeString();
}

std::string
HdMtlxCreateNameFromPath(SdfPath const& path)
{
#ifdef PXR_DCC_LOCATION_ENV_VAR
    std::string pathnm = path.GetText();
    if(pathnm.size() > 3 &&
       pathnm[0] == '/' && pathnm[1] == '_' && pathnm[2] == '_') {
        pathnm[0] = 's'; // triple leading underscores aren't allowed in osl
    }
    return TfStringReplace( pathnm, "/", "_");
#else
    return path.GetName();
#endif
}

// Convert the HdParameterValue to a string MaterialX can understand
std::string 
HdMtlxConvertToString(VtValue const& hdParameterValue)
{
    std::ostringstream valStream;
    if (hdParameterValue.IsHolding<bool>()) {
        return hdParameterValue.UncheckedGet<bool>() ? "true" : "false";
    }
    else if (hdParameterValue.IsHolding<int>() || 
             hdParameterValue.IsHolding<float>()) {
        valStream << hdParameterValue;
        return  valStream.str();
    }
    else if (hdParameterValue.IsHolding<GfVec2f>()) {
        const GfVec2f & value = hdParameterValue.UncheckedGet<GfVec2f>();
        valStream << value.data()[0] << ", " << value.data()[1];
        return valStream.str();
    }
    else if (hdParameterValue.IsHolding<GfVec3f>()) {
        const GfVec3f & value = hdParameterValue.UncheckedGet<GfVec3f>();
        valStream << value.data()[0] << ", " << value.data()[1] << ", "
                  << value.data()[2];
        return valStream.str();
    }
    else if (hdParameterValue.IsHolding<GfVec4f>()) {
        const GfVec4f & value = hdParameterValue.UncheckedGet<GfVec4f>();
        valStream << value.data()[0] << ", " << value.data()[1] << ", "
                  << value.data()[2] << ", " << value.data()[3];
        return valStream.str();
    }
    else if (hdParameterValue.IsHolding<GfMatrix3d>()) {
        const GfMatrix3d & value = hdParameterValue.UncheckedGet<GfMatrix3d>();
        valStream << value[0][0] << ", " << value[0][1] << ", "
                  << value[0][2] << ",  "
                  << value[1][0] << ", " << value[1][1] << ", "
                  << value[1][2] << ",  "
                  << value[2][0] << ", " << value[2][1] << ", "
                  << value[2][2] << ",  ";
        return valStream.str();
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
        return valStream.str();
    }
    else if (hdParameterValue.IsHolding<SdfAssetPath>()) {
        return hdParameterValue.UncheckedGet<SdfAssetPath>().GetAssetPath();
    }
    else if (hdParameterValue.IsHolding<std::string>()) {
        return hdParameterValue.UncheckedGet<std::string>();
    }
    else if (hdParameterValue.IsHolding<TfToken>()) {
        return hdParameterValue.UncheckedGet<TfToken>();
    }
    else {
        TF_WARN("Unsupported Parameter Type '%s'", 
                hdParameterValue.GetTypeName().c_str());
        return mx::EMPTY_STRING;
    }
}

static bool
_UsesTexcoordNode(mx::NodeDefPtr const& mxNodeDef)
{
    mx::InterfaceElementPtr impl = mxNodeDef->getImplementation();
    if (impl && impl->isA<mx::NodeGraph>()) {
        mx::NodeGraphPtr nodegraph = impl->asA<mx::NodeGraph>();
        if (!nodegraph->getNodes(_tokens->texcoord).empty()) {
            return true;
        }
    }
    return false;
}

static std::string
_ConvertToMtlxType(const TfToken& usdTypeName)
{
    static const auto _typeTable =
      std::unordered_map<TfToken, std::string, TfToken::HashFunctor>{
           {_usdTypeTokens->boolType,    "boolean"},
           {_usdTypeTokens->intType,     "integer"},
           {_usdTypeTokens->intarray,    "integerarray"},
           {_usdTypeTokens->floatType,   "float"},
           {_usdTypeTokens->floatarray,  "floatarray"},
           {_usdTypeTokens->color3f,     "color3"},
           {_usdTypeTokens->color3fArray,"color3array"},
           {_usdTypeTokens->color4f,     "color4"},
           {_usdTypeTokens->color4fArray,"color4array"},
           {_usdTypeTokens->float2,      "vector2"},
           {_usdTypeTokens->float2Array, "vector2array"},
           {_usdTypeTokens->float3,      "vector3"},
           {_usdTypeTokens->float3Array, "vector3array"},
           {_usdTypeTokens->float4,      "vector4"},
           {_usdTypeTokens->float4Array, "vector4array"},
           {_usdTypeTokens->matrix3d,    "matrix33"},
           {_usdTypeTokens->matrix4d,    "matrix44"},
           {_usdTypeTokens->asset,       "filename"},
           {_usdTypeTokens->string,      "string"},
           {_usdTypeTokens->stringArray, "stringarray"}
      };
    auto typeIt = _typeTable.find(usdTypeName);
    return typeIt == _typeTable.end() ? "" : typeIt->second;
}

static std::string
_GetMxInputType(
    mx::NodeDefPtr const& mxNodeDef, 
    std::string const& mxInputName,
    TfToken const& usdTypeName=TfToken())
{
    // If given, use the usdTypeName to get the materialX input type
    if (!usdTypeName.IsEmpty()) {
        return _ConvertToMtlxType(usdTypeName);
    }

    // Otherwise look to the nodedef to get the input type
    std::string mxInputType;
    mx::InputPtr mxInput = mxNodeDef->getActiveInput(mxInputName);
    if (mxInput) {
        mxInputType = mxInput->getType();
    }
    return mxInputType;
}

std::string 
HdMtlxGetNodeDefName(std::string const& prevMxNodeDefName)
{
    std::string mxNodeDefName = prevMxNodeDefName;
    // For nodeDef name changes between MaterialX v1.38 and the current version
#if MATERIALX_MAJOR_VERSION == 1 && MATERIALX_MINOR_VERSION >= 39
    // The normalmap nodeDef name changed in v1.39
    if (prevMxNodeDefName == "ND_normalmap") {
        mxNodeDefName = "ND_normalmap_float";
    }
#endif
    return mxNodeDefName;
}


static mx::NodeDefPtr
_GetNodeDef(TfToken const& hdNodeType, mx::DocumentPtr const& mxDoc)
{
    const mx::DocumentPtr& stdLibraries =
        (mxDoc) ? mxDoc : HdMtlxStdLibraries();
    const mx::NodeDefPtr mxNodeDef =
        stdLibraries->getNodeDef(hdNodeType.GetString());
    if (mxNodeDef) {
        return mxNodeDef;
    }

    // If we were not able to find the nodeDef in the stdLibraries it
    // may have been implemented within an asset, stored on the sdrNode.
    const SdrShaderNodeConstPtr sdrNode =
        SdrRegistry::GetInstance().GetShaderNodeByIdentifierAndType(
            hdNodeType, _tokens->mtlx);
    if (!sdrNode) {
        return nullptr;
    }

    const std::string assetPath = sdrNode->GetResolvedImplementationURI();
    if (assetPath.empty()) {
        return nullptr;
    }

    // If we found an asset path load it to the stdLibraries and try and
    // get the nodeDef again. 
    mx::loadLibrary(assetPath, stdLibraries);
    const std::string nodeDefName = sdrNode->GetImplementationName();
    return stdLibraries->getNodeDef(nodeDefName);
}

// Between MaterialX versions nodeDef names may change or nodes may be removed.
// This function calls the above HdMtlxGetNodeDefName() to get the correct 
// nodeDef name and returns a temporary nodeDef for nodes that have been removed
mx::NodeDefPtr
HdMtlxGetNodeDef(TfToken const& hdNodeType, mx::DocumentPtr const& mxDoc)
{
    const std::string nodeTypeStr = hdNodeType.GetString();

    // For node removals between MaterialX v1.38 and the current version
#if MATERIALX_MAJOR_VERSION == 1 && MATERIALX_MINOR_VERSION >= 39
    // Swizzle nodes were deleted in v1.39, return a temporary NodeDef
    std::smatch match;
    static const auto swizzleRegex = std::regex("ND_swizzle_([^_]+)_([^_]+)");
    if (std::regex_match(nodeTypeStr, match, swizzleRegex)) {
        static mx::DocumentPtr swizzleDoc = mx::createDocument();
        if (auto swizzleNodeDef = swizzleDoc->getNodeDef(nodeTypeStr)) {
            return swizzleNodeDef;
        }
        mx::NodeDefPtr swizzleNodeDef = swizzleDoc->addNodeDef(
            nodeTypeStr, match[2].str(), "swizzle");
        swizzleNodeDef->addInput("in", match[1].str());
        swizzleNodeDef->addInput("channels", "string");
        return swizzleNodeDef;
    }
#endif
    const std::string mxNodeDefName = HdMtlxGetNodeDefName(nodeTypeStr);
    return _GetNodeDef(TfToken(mxNodeDefName), mxDoc);
}

static void
_AddParameterInputs(
    HdMaterialNetworkInterface *netInterface,
    TfToken const& hdNodeName,
    mx::NodeDefPtr const& mxNodeDef,
    mx::NodePtr const& mxNode)
{
    // For each hdNode parameter add the corresponding input to the mxNode
    const TfTokenVector hdNodeParamNames =
        netInterface->GetAuthoredNodeParameterNames(hdNodeName);

    for (TfToken const &paramName : hdNodeParamNames) {
        const std::string &mxInputName = paramName.GetString();

        // Skip Colorspace and typeName parameters, these are already 
        // captured in the paramData. Note: these inputs are of the form:
        //  'colorSpace:inputName' and 'typeName:inputName'
        const std::pair<std::string, bool> csResult = 
            SdfPath::StripPrefixNamespace(mxInputName, SdfFieldKeys->ColorSpace);
        if (csResult.second) {
            continue;
        }
        const std::pair<std::string, bool> tnResult = 
            SdfPath::StripPrefixNamespace(mxInputName, _tokens->typeName);
        if (tnResult.second) {
            continue;
        }

        // Get the MaterialX Parameter info
        const HdMaterialNetworkInterface::NodeParamData paramData = 
            netInterface->GetNodeParameterData(hdNodeName, paramName);
        const std::string mxInputValue = HdMtlxConvertToString(paramData.value);

        // Set the input value, and colorspace on the mxNode
        const std::string mxInputType = 
            _GetMxInputType(mxNodeDef, mxInputName, paramData.typeName);
        mx::InputPtr mxInput = 
            mxNode->setInputValue(mxInputName, mxInputValue, mxInputType);
        if (!paramData.colorSpace.IsEmpty()) {
            mxInput->setColorSpace(paramData.colorSpace);
        }
    }

}

// Add a MaterialX version of the hdNode to the mxDoc/mxNodeGraph
static mx::NodePtr 
_AddMaterialXNode(
    HdMaterialNetworkInterface *netInterface,
    TfToken const& hdNodeName,
    mx::DocumentPtr const& mxDoc,
    mx::NodeGraphPtr const& mxNodeGraph,
    std::string const& connectionName,
    HdMtlxTexturePrimvarData *mxHdData)
{
    // Get the mxNode information
    const TfToken hdNodeType = netInterface->GetNodeType(hdNodeName);
    mx::NodeDefPtr mxNodeDef = HdMtlxGetNodeDef(hdNodeType, mxDoc);
    if (!mxNodeDef) {
        TF_WARN("NodeDef not found for Node '%s'", hdNodeType.GetText());
        // Instead of returning here, use a ND_surface definition so that the
        // rest of the network can be processed without errors.
        // This allows networks that might have non mtlx nodes next to
        // the terminal node to come through, and those nodes will be kept
        // out of the shader compile in hdPrman.
        mxNodeDef = mxDoc->getNodeDef(_tokens->ND_surface);
    }

    const SdfPath hdNodePath(hdNodeName.GetString());
    const std::string &mxNodeName = HdMtlxCreateNameFromPath(hdNodePath);
    const std::string &mxNodeCategory = _GetMxNodeString(mxNodeDef);
    const std::string &mxNodeType = mxNodeDef->getType();
    const std::string &mxNodeDefString = 
        (mxNodeDef->getName() == _tokens->ND_surface) 
            ? hdNodeType.GetString()
            : mxNodeDef->getName();

    // Add the mxNode to the mxNodeGraph
    mx::NodePtr mxNode = mxNodeGraph->getNode(mxNodeName);
    if (!mxNode) {
        mxNode = mxNodeGraph->addNode(mxNodeCategory, mxNodeName, mxNodeType);
        if (mxNode->getNodeDef()) {
            mxNode->setNodeDefString(mxNodeDefString);
        }
    }

    // Add the hdNode parameters as inputs to the mxNode
    _AddParameterInputs(netInterface, hdNodeName, mxNodeDef, mxNode);

    // The rest of this function is populating mxHdData
    if (!mxHdData) {
        return mxNode;
    }

    // Texture based nodes: found by having filename input(s)
    for (mx::InputPtr const& mxInput : mxNodeDef->getActiveInputs()) {
        if (mxInput->getType() == _tokens->filename) {
            // Save the corresponding Mx and Hydra names for ShaderGen
            mxHdData->mxHdTextureMap[mxNodeName].insert(mxInput->getName());
            // Save the node path to adjust parameters after for ShaderGen
            mxHdData->hdTextureNodes.insert(hdNodePath);
        }
    }

    // Primvar nodes:
    // Save the node path so the primvarName can be declared in ShaderGen
    if (mxNodeCategory == _tokens->geompropvalue) {
        mxHdData->hdPrimvarNodes.insert(hdNodePath);
    }

    // Nodes using texcoords: either a stdlib texture coordinate node or a 
    // custom node using a texture coordinate node
    // Save the node path so the textureCoord name can be declared in ShaderGen
    if (mxNodeCategory == _tokens->texcoord || _UsesTexcoordNode(mxNodeDef)) {
        mxHdData->hdPrimvarNodes.insert(hdNodePath);
    }
    return mxNode;
}

static mx::InputPtr
_AddNodeInput(
    HdMaterialNetworkInterface *netInterface,
    HdMaterialNetworkInterface::InputConnection const &conn,
    TfToken const &inputName,
    mx::DocumentPtr const &mxDoc,
    mx::NodePtr const &mxCurrNode,
    mx::NodePtr const &mxNextNode)
{
    // Get the input if it exits, otherwise add it below
    mx::InputPtr mxInput = mxCurrNode->getInput(inputName);
    if (mxInput) {
        return mxInput;
    }

    // If the currNode is connected to a multi-output node, the input on the 
    // currNode needs to set the output type and the output name. 
    if (mxNextNode->isMultiOutputType()) {
        TfToken hdNextType = netInterface->GetNodeType(conn.upstreamNodeName);
        mx::NodeDefPtr mxNextNodeDef = mxDoc->getNodeDef(hdNextType.GetString());
        if (!mxNextNodeDef) {
            return mxInput;
        }
        // Add input with the connected ouptut type and set the output name 
        const mx::OutputPtr mxConnOutput = mxNextNodeDef->getOutput(
                conn.upstreamOutputName.GetString());
        mxInput = mxCurrNode->addInput(inputName, mxConnOutput->getType());
        mxInput->setConnectedOutput(mxConnOutput);
    }
    else {
        mxInput = mxCurrNode->addInput(inputName, mxNextNode->getType());
    }

    return mxInput;
}

static mx::OutputPtr
_AddNodeGraphOutput(
    HdMaterialNetworkInterface *netInterface,
    HdMaterialNetworkInterface::InputConnection const &conn,
    std::string const &outputPrefix,
    mx::DocumentPtr const &mxDoc,
    mx::NodeGraphPtr const &mxNodeGraph,
    mx::NodePtr const &mxNextNode)
{
    mx::OutputPtr mxOutput;
    const std::string outputName =
        outputPrefix + "_" + conn.upstreamOutputName.GetString();

    // If the mxNodeGraph output is connected to a multi-output node, the 
    // output on the mxNodegraph needs to get the output type from that 
    // connected node and indicate the output name.
    if (mxNextNode->isMultiOutputType()) {
        TfToken hdNextType = netInterface->GetNodeType(conn.upstreamNodeName);
        mx::NodeDefPtr mxNextNodeDef = mxDoc->getNodeDef(hdNextType.GetString());
        if (!mxNextNodeDef) {
            return mxOutput;
        }
        // Add output with the connected Ouptut type and set the output name 
        const mx::OutputPtr mxConnOutput = mxNextNodeDef->getOutput(
                conn.upstreamOutputName.GetString());
        mxOutput = mxNodeGraph->addOutput(outputName, mxConnOutput->getType());
        mxOutput->setOutputString(mxConnOutput->getName());
    }
    else {
        mxOutput = mxNodeGraph->addOutput(outputName, mxNextNode->getType());
    }

    mxOutput->setConnectedNode(mxNextNode);
    return mxOutput;
}

// Recursively traverse the material n/w and gather the nodes in the MaterialX
// NodeGraph and Document
static void
_GatherUpstreamNodes(
    HdMaterialNetworkInterface *netInterface,
    HdMaterialNetworkInterface::InputConnection const& hdConnection,
    mx::DocumentPtr const& mxDoc,
    mx::NodeGraphPtr *mxNodeGraph,
    mx::NodePtr *mxUpstreamNode,
    std::string const& connectionName,
    HdMtlxTexturePrimvarData *mxHdData)
{
    const TfToken &hdNodeName = hdConnection.upstreamNodeName;
    if (netInterface->GetNodeType(hdNodeName).IsEmpty()) {
        TF_WARN("Could not find the connected Node '%s'", hdNodeName.GetText());
        return;
    }
    
    // Initilize the mxNodeGraph if needed
    if (!(*mxNodeGraph)) {
        const std::string nodeGraphName = mxDoc->createValidChildName(
            SdfPath(hdNodeName).GetParentPath().GetName());
        *mxNodeGraph = mxDoc->addNodeGraph(nodeGraphName);
    }
    
    // Add the node to the mxNodeGraph/mxDoc.
    const mx::NodePtr mxCurrNode =
        _AddMaterialXNode(netInterface, hdNodeName, mxDoc, 
            *mxNodeGraph, connectionName, mxHdData);
    if (!mxCurrNode) {
        return;
    }

    // Continue traversing the upsteam connections to create the mxNodeGraph
    TfTokenVector hdConnectionNames =
        netInterface->GetNodeInputConnectionNames(hdNodeName);
    for (TfToken connName : hdConnectionNames) {
        const auto inputConnections =
            netInterface->GetNodeInputConnection(hdNodeName, connName);
        for (const auto& currConn : inputConnections) {
            // Gather the nodes uptream from the mxCurrNode
            _GatherUpstreamNodes(
                netInterface, currConn, mxDoc, mxNodeGraph,
                mxUpstreamNode, connName.GetString(), mxHdData);

            if (!mxUpstreamNode) {
                continue;
            }

            // Connect mxCurrNode to the mxUpstreamNode    
            mx::NodePtr mxNextNode = *mxUpstreamNode;
            mx::InputPtr mxInput =
                _AddNodeInput(netInterface, currConn, connName, mxDoc, 
                    mxCurrNode, mxNextNode);
            mxInput->setConnectedNode(mxNextNode);
        }
    }

    *mxUpstreamNode = mxCurrNode;
}

static void
_CreateNodeGraphFromTerminalNodeConnections(
    HdMaterialNetworkInterface *netInterface,
    TfToken const& terminalNodeName,
    TfTokenVector const& terminalNodeConnectionNames,
    mx::DocumentPtr const& mxDoc,
    mx::NodePtr const& mxShaderNode,
    HdMtlxTexturePrimvarData * mxHdData)
{
    mx::NodeGraphPtr mxNodeGraph;
    for (TfToken const &connName : terminalNodeConnectionNames) {
        
        const std::string & nodeGraphOutput = connName.GetString();
        const auto inputConnections =
            netInterface->GetNodeInputConnection(terminalNodeName, connName);
        for (const auto &currConn : inputConnections) {

            // Gather the nodes uptream from the 
            mx::NodePtr mxUpstreamNode;
            _GatherUpstreamNodes(
                netInterface, currConn, mxDoc, &mxNodeGraph,
                &mxUpstreamNode, nodeGraphOutput, mxHdData);

            if (!mxUpstreamNode) {
                continue;
            }

            // Connect NodeGraph Output to the ShaderNode input
            mx::OutputPtr mxOutput =
                _AddNodeGraphOutput(netInterface, currConn, nodeGraphOutput,
                    mxDoc, mxNodeGraph, mxUpstreamNode);

            mx::InputPtr mxInput =
                _AddNodeInput(netInterface, currConn, connName, mxDoc, 
                    mxShaderNode, mxUpstreamNode);
            mxInput->setConnectedOutput(mxOutput);
        }
    }
}

// Create a MaterialX Document from the given HdMaterialNetwork2
mx::DocumentPtr 
HdMtlxCreateMtlxDocumentFromHdNetwork(
    HdMaterialNetwork2 const& hdNetwork,
    HdMaterialNode2 const& hdMaterialXNode,
    SdfPath const& hdMaterialXNodePath,
    SdfPath const& materialPath,
    mx::DocumentPtr const& libraries,
    HdMtlxTexturePrimvarData* mxHdData)
{
    // XXX Unfortunate but necessary to cast away constness even though
    // hdNetwork isn't modified.
    HdMaterialNetwork2Interface netInterface(
        materialPath, const_cast<HdMaterialNetwork2*>(&hdNetwork));

    TfToken terminalNodeName = hdMaterialXNodePath.GetAsToken();
    
    return HdMtlxCreateMtlxDocumentFromHdMaterialNetworkInterface(
        &netInterface,
        terminalNodeName,
        netInterface.GetNodeInputConnectionNames(terminalNodeName),
        libraries,
        mxHdData);
}

MaterialX::DocumentPtr
HdMtlxCreateMtlxDocumentFromHdMaterialNetworkInterface(
    HdMaterialNetworkInterface *netInterface,
    TfToken const& terminalNodeName,
    TfTokenVector const& terminalNodeConnectionNames,
    MaterialX::DocumentPtr const& libraries,
    HdMtlxTexturePrimvarData *mxHdData)
{
    TRACE_FUNCTION_SCOPE("Create Mtlx Document from HdMaterialNetwork")
    if (!netInterface) {
        return nullptr;
    }

    // Initialize a MaterialX Document
    mx::DocumentPtr mxDoc = mx::createDocument();
    mxDoc->importLibrary(libraries);

    // Get the version of the MaterialX document if specified, otherwise
    // default to v1.38. Note that we should always default to 1.38 to handle 
    // the case where older USD files have not made use of this config schema. 
    std::string materialXVersionString = "1.38";
    const VtValue materialXVersionValue =
        netInterface->GetMaterialConfigValue(_tokens->mtlxVersion);
    if (materialXVersionValue.IsHolding<std::string>()) {
        materialXVersionString = materialXVersionValue.Get<std::string>();
        TF_DEBUG(HDMTLX_VERSION_UPGRADE).Msg(
            "[%s] : MaterialX document version : '%s'\n",
            TF_FUNC_NAME().c_str(), materialXVersionString.c_str());
    } else {
        TF_DEBUG(HDMTLX_VERSION_UPGRADE).Msg(
            "[%s] : MaterialX document version : '%s' (Using default)\n",
            TF_FUNC_NAME().c_str(), materialXVersionString.c_str());
    }
    mxDoc->setVersionString(materialXVersionString);
    
    // Create the terminal shader and material nodes 
    const TfToken hdTerminalType = netInterface->GetNodeType(terminalNodeName);
    const mx::NodeDefPtr terminalNodeDef =
        HdMtlxGetNodeDef(hdTerminalType, mxDoc);
    if (!terminalNodeDef) {
        TF_WARN("Unsupported terminal node type '%s' cannot find the "
                "associated NodeDef.", hdTerminalType.GetText());
    }
    const std::string mxTerminalType = 
        (terminalNodeDef) ? _GetMxNodeString(terminalNodeDef) : "";
    mx::NodePtr mxShaderNode = mxDoc->addNode(
        mxTerminalType,
        HdMtlxTokens->surfaceshaderName, mx::SURFACE_SHADER_TYPE_STRING);

    const std::string &materialName =
        netInterface->GetMaterialPrimPath().GetName();
    mx::NodePtr mxMaterial = mxDoc->addMaterialNode(
        mxDoc->createValidChildName(materialName), mxShaderNode);

    // Create the NodeGraph
    _CreateNodeGraphFromTerminalNodeConnections(
        netInterface, terminalNodeName, terminalNodeConnectionNames,
        mxDoc, mxShaderNode, mxHdData);

    // Add terminal shader node inputs
    _AddParameterInputs(
        netInterface, terminalNodeName, terminalNodeDef, mxShaderNode);

    if (TfDebug::IsEnabled(HDMTLX_VERSION_UPGRADE)) {
        const std::string filename = mxMaterial->getName() + "_before.mtlx";
        TF_DEBUG(HDMTLX_VERSION_UPGRADE).Msg(
            "[%s] : MaterialX document before upgrade: '%s'\n",
            TF_FUNC_NAME().c_str(), filename.c_str());
        mx::writeToXmlFile(mxDoc, mx::FilePath(filename));
    }

    // Potentially upgrade the MaterialX document to the "current" version,
    // using the MaterialX upgrade mechanism.
    mxDoc->upgradeVersion();

    if (TfDebug::IsEnabled(HDMTLX_VERSION_UPGRADE)) {
        const std::string filename = mxMaterial->getName() + "_after.mtlx";
        TF_DEBUG(HDMTLX_VERSION_UPGRADE).Msg(
            "[%s] : MaterialX document after upgrade: '%s'\n",
            TF_FUNC_NAME().c_str(), filename.c_str());
        mx::writeToXmlFile(mxDoc, mx::FilePath(filename));
    } 
    else if (TfDebug::IsEnabled(HDMTLX_WRITE_DOCUMENT)) {
        const std::string filename = mxMaterial->getName() + ".mtlx";
        TF_DEBUG(HDMTLX_WRITE_DOCUMENT).Msg(
            "[%s] : MaterialX document: '%s'\n",
            TF_FUNC_NAME().c_str(), filename.c_str());
        mx::writeToXmlFile(mxDoc, mx::FilePath(filename));
    }

    // Validate the MaterialX Document.
    {
        TRACE_FUNCTION_SCOPE("Validate created Mtlx Document")
        std::string message;
        if (!mxDoc->validate(&message)) {
            TF_WARN("Validation warnings for generated MaterialX file.\n%s\n", 
                message.c_str());
        }
    }
    return mxDoc;
}

PXR_NAMESPACE_CLOSE_SCOPE
