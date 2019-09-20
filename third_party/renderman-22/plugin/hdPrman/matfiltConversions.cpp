//
// Copyright 2019 Pixar
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
#include "hdPrman/matfiltConversions.h"
#include "pxr/base/tf/stl.h"
#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

bool
MatfiltConvertFromHdMaterialNetworkMapTerminal(
    const HdMaterialNetworkMap & hdNetworkMap,
    const TfToken & terminalName,
    MatfiltNetwork *result)
{
    auto iter = hdNetworkMap.map.find(terminalName);
    if (iter == hdNetworkMap.map.end()) {
        return false;
    }
    const HdMaterialNetwork & hdNetwork = iter->second;

    // Transfer over individual nodes
    for (const HdMaterialNode & node : hdNetwork.nodes) {
        MatfiltNode & matfiltNode = result->nodes[node.path];
        matfiltNode.nodeTypeId = node.identifier;
        matfiltNode.parameters = node.parameters;
    }

    // Assume that the last entry is the terminal (as that's not specified in
    // HdMaterialNetworkMap/HdMaterialNetwork).
    if (!hdNetwork.nodes.empty()) {
        result->terminals[terminalName].upstreamNode =
            hdNetwork.nodes.back().path;
    }

    // Transfer over relationships to inputConnections on receiving/downstream
    // nodes.
    for (const HdMaterialRelationship & rel : hdNetwork.relationships) {
        // outputId (in hdMaterial terms) is the input of the receiving node
        auto iter = result->nodes.find(rel.outputId);
        // skip connection if the destination node doesn't exist
        if (iter == result->nodes.end()) {
            continue;
        }
        iter->second.inputConnections[rel.outputName]
            .emplace_back( MatfiltConnection{rel.inputId, rel.inputName});
    }

    return true;
}

static void
_MatfiltConvertToHdMaterialNetworkMapVisitNode(
    const MatfiltNetwork & matfiltNetwork,
    const SdfPath & nodeId,
    std::unordered_set<SdfPath, SdfPath::Hash> & visitedNodes,
    HdMaterialNetwork *result)
{
    if (!visitedNodes.insert(nodeId).second) {
        // Already visited
        return;
    }
    MatfiltNode const* matfiltNode =
        TfMapLookupPtr(matfiltNetwork.nodes, nodeId);
    if (!matfiltNode) {
        // can't find the node?, skip it
        return;
    }

    // walk the inputParameters first so that dependencies are declared first
    for (const auto & I : matfiltNode->inputConnections) {
        const TfToken & inputName = I.first;
        const std::vector<MatfiltConnection> & connectionVector = I.second;

        for (const MatfiltConnection & connection : connectionVector) {
            _MatfiltConvertToHdMaterialNetworkMapVisitNode(matfiltNetwork,
                    connection.upstreamNode, visitedNodes, result);

            result->relationships.emplace_back();
            HdMaterialRelationship & relationship =
                    result->relationships.back();

            relationship.inputId = connection.upstreamNode;
            relationship.inputName = connection.upstreamOutputName;
            relationship.outputId = nodeId;
            relationship.outputName = inputName;
        }
    }

    result->nodes.emplace_back();
    HdMaterialNode & hdNode = result->nodes.back();
    hdNode.path = nodeId;
    hdNode.identifier = matfiltNode->nodeTypeId;
    hdNode.parameters = matfiltNode->parameters;
}

bool
MatfiltConvertToHdMaterialNetworkMap(
    const MatfiltNetwork & matfiltNetwork,
    HdMaterialNetworkMap *result)
{
    std::unordered_set<SdfPath, SdfPath::Hash> visitedNodes;
    for (const auto & I : matfiltNetwork.terminals) {
        visitedNodes.clear();
        const TfToken & terminalName = I.first;
        const MatfiltConnection & terminalConnection = I.second;        
        _MatfiltConvertToHdMaterialNetworkMapVisitNode(
            matfiltNetwork,
            terminalConnection.upstreamNode,
            visitedNodes,
            &result->map[terminalName]);
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
