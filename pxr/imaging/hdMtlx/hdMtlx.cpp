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

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/base/tf/diagnostic.h"

#include <MaterialXCore/Document.h>
#include <MaterialXCore/Node.h>
#include <MaterialXFormat/Util.h>
#include <MaterialXFormat/XmlIo.h>

namespace mx = MaterialX;

PXR_NAMESPACE_OPEN_SCOPE

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
    return TfToken(mxNodeDef->getNodeString());
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
        return (hdParameterValue.UncheckedGet<bool>()) ? "false" 
                                                                : "true";
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

    // Get the mxInputValue from the Value
    *mxInputValue = HdMtlxConvertToString(hdParameter.second);

    // Get the mxInputType for the mxNodeDef
    mx::InputPtr mxInput = mxNodeDef->getInput(*mxInputName);
    if (mxInput) {
        *mxInputType = mxInput->getType();
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

            // Make sure to not add the same input twice 
            mx::InputPtr mxInput = mxCurrNode->getInput(connName);
            if (!mxInput){
                mxInput = mxCurrNode->addInput(connName, mxNextNode->getType());
            }
            mxInput->setConnectedNode(mxNextNode);
        }
    }

    *mxUpstreamNode = mxCurrNode;
}


// Create a MaterialX Document from the given HdMaterialNetwork
mx::DocumentPtr 
HdMtlxCreateMtlxDocumentFromHdNetwork(
    HdMaterialNetwork2 const& hdNetwork,
    HdMaterialNode2 const& hdMaterialXNode,
    SdfPath const& materialPath,
    mx::DocumentPtr const& libraries,
    std::set<SdfPath> * hdTextureNodes, // Paths to the Hd Texture Nodes
    mx::StringMap * mxHdTextureMap)     // Mx-Hd texture name counterparts
{
    // Initialize a MaterialX Document
    mx::DocumentPtr mxDoc = mx::createDocument();
    mxDoc->importLibrary(libraries);
    
    // Create a material that instantiates the shader
    const std::string & materialName = materialPath.GetName();
    TfToken mxType = _GetMxNodeType(mxDoc, hdMaterialXNode.nodeTypeId);
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

PXR_NAMESPACE_CLOSE_SCOPE