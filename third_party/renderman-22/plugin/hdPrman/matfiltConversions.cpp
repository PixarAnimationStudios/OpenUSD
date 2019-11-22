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
MatfiltConvertFromHdMaterialNetworkMap(
    const HdMaterialNetworkMap & hdNetworkMap,
    MatfiltNetwork *result)
{
    for (auto const& iter: hdNetworkMap.map) {
        const TfToken & terminalName = iter.first;
        const HdMaterialNetwork & hdNetwork = iter.second;

        if (hdNetwork.nodes.empty()) {
            continue;
    }

        // Transfer over individual nodes.
        // Note that the same nodes may be shared by multiple terminals.
        // We simply overwrite them here.
    for (const HdMaterialNode & node : hdNetwork.nodes) {
        MatfiltNode & matfiltNode = result->nodes[node.path];
        matfiltNode.nodeTypeId = node.identifier;
        matfiltNode.parameters = node.parameters;
    }

        // Assume that the last entry is the terminal (as that's not
        // specified in HdMaterialNetworkMap/HdMaterialNetwork).
        result->terminals[terminalName].upstreamNode =
            hdNetwork.nodes.back().path;

        // Transfer over relationships to inputConnections on
        // receiving/downstream nodes.
    for (const HdMaterialRelationship & rel : hdNetwork.relationships) {
        // outputId (in hdMaterial terms) is the input of the receiving node
        auto iter = result->nodes.find(rel.outputId);
        // skip connection if the destination node doesn't exist
        if (iter == result->nodes.end()) {
            continue;
        }
            std::vector<MatfiltConnection> &conns =
                iter->second.inputConnections[rel.outputName];
            MatfiltConnection conn = {rel.inputId, rel.inputName};
            // skip connection if it already exists (it may be shared
            // between surface and displacement)
            if (std::find(conns.begin(), conns.end(), conn) == conns.end()) {
                conns.emplace_back(conn);
        }
    }
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
