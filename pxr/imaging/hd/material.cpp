//
// Copyright 2016 Pixar
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
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

HdMaterial::HdMaterial(SdfPath const& id)
 : HdSprim(id)
{
    // NOTHING
}

HdMaterial::~HdMaterial()
{
    // NOTHING
}


void
HdMaterialNetwork2ConvertFromHdMaterialNetworkMap(
    const HdMaterialNetworkMap & hdNetworkMap,
    HdMaterialNetwork2 *result,
    bool *isVolume)
{
    HD_TRACE_FUNCTION();

    for (auto const& iter: hdNetworkMap.map) {
        const TfToken & terminalName = iter.first;
        const HdMaterialNetwork & hdNetwork = iter.second;

        // Check if there are nodes associated with the volume terminal
        // This value is used in Storm to get the proper glslfx fragment shader
        if (terminalName == HdMaterialTerminalTokens->volume && isVolume) {
            *isVolume = !hdNetwork.nodes.empty();
        }

        // Transfer over individual nodes.
        // Note that the same nodes may be shared by multiple terminals.
        // We simply overwrite them here.
        if (hdNetwork.nodes.empty()) {
            continue;
        }
        for (const HdMaterialNode & node : hdNetwork.nodes) {
            HdMaterialNode2 & materialNode2 = result->nodes[node.path];
            materialNode2.nodeTypeId = node.identifier;
            materialNode2.parameters = node.parameters;
        }
        // Assume that the last entry is the terminal 
        result->terminals[terminalName].upstreamNode = 
                hdNetwork.nodes.back().path;

        // Transfer relationships to inputConnections on receiving/downstream nodes.
        for (const HdMaterialRelationship & rel : hdNetwork.relationships) {
            
            // outputId (in hdMaterial terms) is the input of the receiving node
            auto iter = result->nodes.find(rel.outputId);

            // skip connection if the destination node doesn't exist
            if (iter == result->nodes.end()) {
                continue;
            }

            std::vector<HdMaterialConnection2> &conns =
                iter->second.inputConnections[rel.outputName];
            HdMaterialConnection2 conn {rel.inputId, rel.inputName};
            
            // skip connection if it already exists (it may be shared
            // between surface and displacement)
            if (std::find(conns.begin(), conns.end(), conn) == conns.end()) {
                conns.push_back(std::move(conn));
            }
        }

        // Transfer primvars:
        result->primvars = hdNetwork.primvars;
    }
}


// -------------------------------------------------------------------------- //
// VtValue Requirements
// -------------------------------------------------------------------------- //

std::ostream& operator<<(std::ostream& out, const HdMaterialNetwork& pv)
{
    out << "HdMaterialNetwork Params: (...) " ;
    return out;
}

bool operator==(const HdMaterialRelationship& lhs, 
                const HdMaterialRelationship& rhs)
{
    return lhs.outputId   == rhs.outputId && 
           lhs.outputName == rhs.outputName &&
           lhs.inputId    == rhs.inputId &&
           lhs.inputName  == rhs.inputName;
}

bool operator==(const HdMaterialNode& lhs, const HdMaterialNode& rhs)
{
    return lhs.path == rhs.path &&
           lhs.identifier == rhs.identifier &&
           lhs.parameters == rhs.parameters;
}

bool operator==(const HdMaterialNetwork& lhs, const HdMaterialNetwork& rhs) 
{
    return lhs.relationships           == rhs.relationships && 
           lhs.nodes                   == rhs.nodes &&
           lhs.primvars                == rhs.primvars;
}

bool operator!=(const HdMaterialNetwork& lhs, const HdMaterialNetwork& rhs) 
{
    return !(lhs == rhs);
}


std::ostream& operator<<(std::ostream& out, const HdMaterialNetworkMap& pv)
{
    out << "HdMaterialNetworkMap Params: (...) " ;
    return out;
}

bool operator==(const HdMaterialNetworkMap& lhs,
                const HdMaterialNetworkMap& rhs) 
{
    return lhs.map == rhs.map &&
           lhs.terminals == rhs.terminals;
}

bool operator!=(const HdMaterialNetworkMap& lhs,
                const HdMaterialNetworkMap& rhs) 
{
    return !(lhs == rhs);
}


PXR_NAMESPACE_CLOSE_SCOPE