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
#include "pxr/imaging/hdMtlx/hdMtlx.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/materialNetwork2Interface.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdr/registry.h"

#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/token.h"

#include "pxr/usd/usdMtlx/utils.h"

#include <MaterialXCore/Document.h>
#include <MaterialXCore/Node.h>
#include <MaterialXFormat/Util.h>
#include <MaterialXFormat/XmlIo.h>

namespace mx = MaterialX;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (index)
);

static mx::FileSearchPath
_ComputeSearchPaths()
{
    mx::FileSearchPath searchPaths;
    static const NdrStringVec searchPathStrings = UsdMtlxSearchPaths();
    for (auto path : searchPathStrings) {
        searchPaths.append(mx::FilePath(path));
    }
    return searchPaths;
}

const mx::FileSearchPath&
HdMtlxSearchPaths()
{
    static const mx::FileSearchPath searchPaths = _ComputeSearchPaths();
    return searchPaths;
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

// Return the MaterialX Node Type based on the corresponding NodeDef name, 
// which is stored as the hdNodeType. 
static TfToken
_GetMxNodeType(mx::DocumentPtr const& mxDoc, TfToken const& hdNodeType)
{
    mx::NodeDefPtr mxNodeDef = mxDoc->getNodeDef(hdNodeType.GetString());
    if (!mxNodeDef){
        TF_WARN("Unsupported node type '%s' cannot find the associated NodeDef.",
                hdNodeType.GetText());
        return TfToken();
    }

    return TfToken(_GetMxNodeString(mxNodeDef));
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
    else {
        TF_WARN("Unsupported Parameter Type '%s'", 
                hdParameterValue.GetTypeName().c_str());
        return mx::EMPTY_STRING;
    }
}

static bool
_ContainsTexcoordNode(mx::NodeDefPtr const& mxNodeDef)
{
    mx::InterfaceElementPtr impl = mxNodeDef->getImplementation();
    if (impl && impl->isA<mx::NodeGraph>()) {
        mx::NodeGraphPtr nodegraph = impl->asA<mx::NodeGraph>();
        if (nodegraph->getNodes("texcoord").size() != 0) {
            return true;
        }
    }
    return false;
}

// Add a MaterialX version of the hdNode to the mxDoc/mxNodeGraph
static mx::NodePtr 
_AddMaterialXNode(
    HdMaterialNetworkInterface *netInterface,
    TfToken const& hdNodeName,
    mx::DocumentPtr const& mxDoc,
    mx::NodeGraphPtr const& mxNodeGraph,
    mx::StringSet *addedNodeNames,
    std::string const& connectionName,
    HdMtlxTexturePrimvarData *mxHdData)
{
    // Get the mxNode information
    TfToken hdNodeType = netInterface->GetNodeType(hdNodeName);
    mx::NodeDefPtr mxNodeDef = mxDoc->getNodeDef(hdNodeType.GetString());
    if (!mxNodeDef) {
        TF_WARN("NodeDef not found for Node '%s'", hdNodeType.GetText());
        return mx::NodePtr();
    }
    const SdfPath hdNodePath(hdNodeName.GetString());
    const std::string mxNodeCategory = _GetMxNodeString(mxNodeDef);
    const std::string &mxNodeType = mxNodeDef->getType();
    const std::string &mxNodeName = hdNodePath.GetName();

    // Add the mxNode to the mxNodeGraph
    mx::NodePtr mxNode =
        _AddNodeToNodeGraph(mxNodeName, mxNodeCategory, 
                            mxNodeType, mxNodeGraph, addedNodeNames);

    if (mxNode->getNodeDef()) {
        // Sometimes mxNode->getNodeDef() starts failing.
        // It seems to happen when there are connections with mismatched types.
        // Explicitly setting the node def string appparently fixes the problem.
        // If we don't do this code gen may fail.
        if (mxNode->getNodeDefString().empty()) {
            mxNode->setNodeDefString(hdNodeType.GetText());
        }
    }

    // For each of the HdNode parameters add the corresponding parameter/input 
    // to the mxNode
    TfTokenVector hdNodeParamNames =
        netInterface->GetAuthoredNodeParameterNames(hdNodeName);
    for (TfToken const &paramName : hdNodeParamNames) {
        // Get the MaterialX Parameter info
        const std::string &mxInputName = paramName.GetString();
        std::string mxInputType;
        mx::InputPtr mxInput = mxNodeDef->getActiveInput(mxInputName);
        if (mxInput) {
            mxInputType = mxInput->getType();
        }
        std::string mxInputValue = HdMtlxConvertToString(
            netInterface->GetNodeParameterValue(hdNodeName, paramName));

        mxNode->setInputValue(mxInputName, mxInputValue, mxInputType);
    }

    // MaterialX nodes that use textures are assumed to have a filename input
    if (mxNodeDef->getNodeGroup() == "texture2d") {
        if (mxHdData) {
            // Save the corresponding MaterialX and Hydra names for ShaderGen
            mxHdData->mxHdTextureMap[mxNodeName] = connectionName;
            // Save the path to adjust parameters after traversing the network
            mxHdData->hdTextureNodes.insert(hdNodePath);
        }
    }

    // MaterialX primvar node
    if (mxNodeCategory == "geompropvalue") {
        if (mxHdData) {
            // Save the path to have the primvarName declared in ShaderGen
            mxHdData->hdPrimvarNodes.insert(hdNodePath);
        }
    }

    // Stdlib MaterialX texture coordinate node or a custom node that 
    // uses a texture coordinate node
    if (mxNodeCategory == "texcoord" || _ContainsTexcoordNode(mxNodeDef)) {
        if (mxHdData) {
            // Make sure it has the index parameter set.
            if (std::find(hdNodeParamNames.begin(), hdNodeParamNames.end(), 
                _tokens->index) == hdNodeParamNames.end()) {
                netInterface->SetNodeParameterValue(
                    hdNodeName, _tokens->index, VtValue(0));
            }
            // Save the path to have the textureCoord name declared in ShaderGen
            mxHdData->hdPrimvarNodes.insert(hdNodePath);
        }
    }
    return mxNode;
}

static void
_AddInput(
    HdMaterialNetworkInterface *netInterface,
    HdMaterialNetworkInterface::InputConnection const &conn,
    TfToken const &inputName,
    mx::DocumentPtr const &mxDoc,
    mx::NodePtr const &mxCurrNode,
    mx::NodePtr const &mxNextNode,
    mx::InputPtr *mxInput)
{
    // If the currNode is connected to a multi-output node, the input on the 
    // currNode needs to get the output type and indicate the output name. 
    if (mxNextNode->isMultiOutputType()) {
        TfToken hdNextType = netInterface->GetNodeType(conn.upstreamNodeName);
        mx::NodeDefPtr mxNextNodeDef = mxDoc->getNodeDef(hdNextType.GetString());
        if (mxNextNodeDef) {
            mx::OutputPtr mxConnOutput = mxNextNodeDef->getOutput(
                    conn.upstreamOutputName.GetString());
            // Add input with the connected Ouptut type and set the output name 
            *mxInput = mxCurrNode->addInput(inputName, mxConnOutput->getType());
            (*mxInput)->setConnectedOutput(mxConnOutput);
        }
    }
    else {
        *mxInput = mxCurrNode->addInput(inputName, mxNextNode->getType());
    }
}

static void
_AddNodeGraphOutput(
    HdMaterialNetworkInterface *netInterface,
    HdMaterialNetworkInterface::InputConnection const &conn,
    std::string const &outputName,
    mx::DocumentPtr const &mxDoc,
    mx::NodeGraphPtr const &mxNodeGraph,
    mx::NodePtr const &mxNextNode,
    mx::OutputPtr *mxOutput)
{
    // If the mxNodeGraph output is connected to a multi-output node, the 
    // output on the mxNodegraph needs to get the output type from that 
    // connected node and indicate the output name.
    if (mxNextNode->isMultiOutputType()) {
        TfToken hdNextType = netInterface->GetNodeType(conn.upstreamNodeName);
        mx::NodeDefPtr mxNextNodeDef = mxDoc->getNodeDef(hdNextType.GetString());
        if (mxNextNodeDef) {
            mx::OutputPtr mxConnOutput = mxNextNodeDef->getOutput(
                    conn.upstreamOutputName.GetString());
            // Add output with the connected Ouptut type and set the output name 
            *mxOutput = mxNodeGraph->addOutput(
                outputName, mxConnOutput->getType());
            (*mxOutput)->setOutputString(mxConnOutput->getName());
        }
    }
    else {
        *mxOutput = mxNodeGraph->addOutput(outputName, mxNextNode->getType());
    }
}

// Recursively traverse the material n/w and gather the nodes in the MaterialX
// NodeGraph and Document
static void
_GatherUpstreamNodes(
    HdMaterialNetworkInterface *netInterface,
    HdMaterialNetworkInterface::InputConnection const& hdConnection,
    mx::DocumentPtr const& mxDoc,
    mx::NodeGraphPtr *mxNodeGraph,
    mx::StringSet *addedNodeNames,
    mx::NodePtr *mxUpstreamNode,
    std::string const& connectionName,
    HdMtlxTexturePrimvarData *mxHdData)
{
    TfToken const &hdNodeName = hdConnection.upstreamNodeName;
    if (netInterface->GetNodeType(hdNodeName).IsEmpty()) {
         TF_WARN("Could not find the connected Node '%s'", 
                    hdConnection.upstreamNodeName.GetText());
        return;
    }
    
    // Initilize the mxNodeGraph if needed
    if (!(*mxNodeGraph)) {
        const std::string nodeGraphName = mxDoc->createValidChildName(
            SdfPath(hdNodeName).GetParentPath().GetName());
        *mxNodeGraph = mxDoc->addNodeGraph(nodeGraphName);
    }
    
    // Add the node to the mxNodeGraph/mxDoc.
    mx::NodePtr mxCurrNode =
        _AddMaterialXNode(netInterface, hdNodeName, mxDoc, *mxNodeGraph, 
                          addedNodeNames, connectionName, mxHdData);

    if (!mxCurrNode) {
        return;
    }

    TfTokenVector hdConnectionNames =
        netInterface->GetNodeInputConnectionNames(hdNodeName);

    // Continue traversing the upsteam connections to create the mxNodeGraph
    for (TfToken connName : hdConnectionNames) {
        const auto inputConnections =
            netInterface->GetNodeInputConnection(hdNodeName, connName);
        for (const auto& currConnection : inputConnections) {
            // Gather the nodes uptream from the mxCurrNode
            _GatherUpstreamNodes(
                netInterface, currConnection, mxDoc, mxNodeGraph,
                addedNodeNames, mxUpstreamNode, connName.GetString(), mxHdData);

            // Connect mxCurrNode to the mxUpstreamNode
            mx::NodePtr mxNextNode = *mxUpstreamNode;
            if (!mxNextNode) {
                continue;
            }

            // Make sure to not add the same input twice 
            mx::InputPtr mxInput = mxCurrNode->getInput(connName);
            if (!mxInput) {
                _AddInput(netInterface, currConnection, connName,
                          mxDoc, mxCurrNode, mxNextNode, &mxInput);
            }
            mxInput->setConnectedNode(mxNextNode);
        }
    }

    *mxUpstreamNode = mxCurrNode;
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

// Add parameter inputs for the terminal node (which is a StandardSurface or
// USDPreviewSurface node)
static void
_AddParameterInputsToTerminalNode(
    HdMaterialNetworkInterface *netInterface,
    TfToken const& terminalNodeName,
    TfToken const& mxType,
    mx::NodePtr const& mxShaderNode)
{
    TfTokenVector paramNames =
        netInterface->GetAuthoredNodeParameterNames(terminalNodeName);

    mx::NodeDefPtr mxNodeDef = mxShaderNode->getNodeDef();
    if (!mxNodeDef){
        TF_WARN("NodeDef not found for Node '%s'", mxType.GetText());
        return;
    }

    for (TfToken const &paramName : paramNames) {
        // Get the MaterialX Parameter info
        const std::string &mxInputName = paramName.GetString();
        std::string mxInputType;
        mx::InputPtr mxInput = mxNodeDef->getActiveInput(mxInputName);
        if (mxInput) {
            mxInputType = mxInput->getType();
        }
        std::string mxInputValue = HdMtlxConvertToString(
            netInterface->GetNodeParameterValue(terminalNodeName, paramName));

        mxShaderNode->setInputValue(mxInputName, mxInputValue, mxInputType);
    }
}

// Updates mxDoc from traversing the node graph leading into the terminal node.
static void
_CreateMtlxNodeGraphFromTerminalNodeConnections(
    HdMaterialNetworkInterface *netInterface,
    TfToken const& terminalNodeName,
    TfTokenVector const& terminalNodeConnectionNames,
    mx::DocumentPtr const& mxDoc,
    mx::NodePtr const& mxShaderNode,
    HdMtlxTexturePrimvarData * mxHdData)
{
    mx::NodeGraphPtr mxNodeGraph;
    mx::StringSet addedNodeNames; // Set of NodeNames in the mxNodeGraph
    for (TfToken const &cName : terminalNodeConnectionNames) {
        const std::string & mxNodeGraphOutput = cName.GetString();
        const auto inputConnections =
            netInterface->GetNodeInputConnection(terminalNodeName, cName);
        for (const auto &currConnection : inputConnections) {
            // Gather the nodes uptream from the hdMaterialXNode
            mx::NodePtr mxUpstreamNode;

            _GatherUpstreamNodes(
                netInterface, currConnection, mxDoc, &mxNodeGraph,
                &addedNodeNames, &mxUpstreamNode, mxNodeGraphOutput, mxHdData);
            
            if (!mxUpstreamNode) {
                continue;
            }

            // Connect currNode to the upstream Node
            std::string fullOutputName = mxNodeGraphOutput + "_" +
                            currConnection.upstreamOutputName.GetString();
            mx::OutputPtr mxOutput;
            _AddNodeGraphOutput(netInterface, currConnection, fullOutputName,
                       mxDoc, mxNodeGraph, mxUpstreamNode, &mxOutput);
            mxOutput->setConnectedNode(mxUpstreamNode);

            // Connect NodeGraph Output to the ShaderNode
            mx::InputPtr mxInput;
            _AddInput(netInterface, currConnection, cName,
                      mxDoc, mxShaderNode, mxUpstreamNode, &mxInput);
            mxInput->setConnectedOutput(mxOutput);
        }
    }
}

MaterialX::DocumentPtr
HdMtlxCreateMtlxDocumentFromHdMaterialNetworkInterface(
    HdMaterialNetworkInterface *netInterface,
    TfToken const& terminalNodeName,
    TfTokenVector const& terminalNodeConnectionNames,
    MaterialX::DocumentPtr const& libraries,
    HdMtlxTexturePrimvarData *mxHdData)
{
    if (!netInterface) {
        return nullptr;
    }

    // Initialize a MaterialX Document
    mx::DocumentPtr mxDoc = mx::createDocument();
    mxDoc->importLibrary(libraries);
    
    // Create a material that instantiates the shader
    SdfPath materialPath = netInterface->GetMaterialPrimPath();
    const std::string & materialName = materialPath.GetName();
    TfToken mxType =
        _GetMxNodeType(mxDoc, netInterface->GetNodeType(terminalNodeName));
    mx::NodePtr mxShaderNode = mxDoc->addNode(mxType.GetString(),
                                              "Surface",
                                              "surfaceshader");
    mx::NodePtr mxMaterial = mxDoc->addMaterialNode(
        mxDoc->createValidChildName(materialName), mxShaderNode);

    _CreateMtlxNodeGraphFromTerminalNodeConnections(
        netInterface, terminalNodeName, terminalNodeConnectionNames,
        mxDoc, mxShaderNode, mxHdData);

    _AddParameterInputsToTerminalNode(
        netInterface,
        terminalNodeName,
        mxType,
        mxShaderNode);

    // Validate the MaterialX Document.
    std::string message;
    if (!mxDoc->validate(&message)) {
        TF_WARN("Validation warnings for generated MaterialX file.\n%s\n", 
                message.c_str());
    }

    return mxDoc;
}

PXR_NAMESPACE_CLOSE_SCOPE
