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
#ifndef PXR_IMAGING_HD_MATERIAL_H
#define PXR_IMAGING_HD_MATERIAL_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hd/sceneDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// Hydra Schema for a material object.
///
class HdMaterial : public HdSprim {
public:
    // change tracking for HdMaterial prim
    enum DirtyBits : HdDirtyBits {
        Clean                 = 0,
        // XXX: Got to skip varying and force sync bits for now
        DirtyParams           = 1 << 2,
        DirtyResource         = 1 << 3,
        AllDirty              = (DirtyParams | DirtyResource)
    };

    HD_API
    virtual ~HdMaterial();

protected:
    HD_API
    HdMaterial(SdfPath const& id);

private:
    // Class can not be default constructed or copied.
    HdMaterial()                             = delete;
    HdMaterial(const HdMaterial &)             = delete;
    HdMaterial &operator =(const HdMaterial &) = delete;
};


/// \struct HdMaterialRelationship
///
/// Describes a connection between two nodes in a material.
///
/// A brief discussion of terminology follows:
///
/// * Shading nodes have inputs and outputs.
/// * Shading nodes consume input values and produce output values.
/// * Connections also have inputs and outputs.
/// * Connections consume a value from the (\em inputId, \em inputName)
///   and pass that value to the (\em outputId, \em outputName).
///
/// Note that a connection's input is considered an output on the
/// upstream shading node, and the connection's output is an input
/// on the downstream shading node.
///
/// A guideline to remember this terminology is that inputs
/// are always upstream of outputs in the dataflow.
/// 
struct HdMaterialRelationship {
    SdfPath inputId;
    TfToken inputName;
    SdfPath outputId;
    TfToken outputName;
};

// VtValue requirements
HD_API
bool operator==(const HdMaterialRelationship& lhs, 
                const HdMaterialRelationship& rhs);


/// \struct HdMaterialNode
///
/// Describes a material node which is made of a path, an identifier and
/// a list of parameters.
struct HdMaterialNode {
    SdfPath path;
    TfToken identifier;
    std::map<TfToken, VtValue> parameters;
};

// VtValue requirements
HD_API
bool operator==(const HdMaterialNode& lhs, const HdMaterialNode& rhs);


/// \struct HdMaterialNetwork
///
/// Describes a material network composed of nodes, primvars, and relationships
/// between the nodes and terminals of those nodes.
struct HdMaterialNetwork {
    std::vector<HdMaterialRelationship> relationships;
    std::vector<HdMaterialNode> nodes;
    TfTokenVector primvars;
};

/// \struct HdMaterialNetworkMap
///
/// Describes a map from network type to network.
struct HdMaterialNetworkMap {
    std::map<TfToken, HdMaterialNetwork> map;
    std::vector<SdfPath> terminals;
};


///
/// HdMaterialNetwork2
///
/// This struct replaces the previously used MatfiltNetwork and
/// HdSt_MaterialNetwork. 
/// In the furuture this HdMaterialNetwork2 will replace the current 
/// HdMaterialNetwork defined above.
///

/// \struct HdMaterialConnection2
///
/// Describes a single connection to an upsream node and output port 
/// Replacement for HdRelationship.
struct HdMaterialConnection2 {
    SdfPath upstreamNode;
    TfToken upstreamOutputName;

    bool operator==(const HdMaterialConnection2 & rhs) const {
        return upstreamNode == rhs.upstreamNode
            && upstreamOutputName == rhs.upstreamOutputName;
    }
};

/// \struct HdMaterialNode2
///
/// Describes an instance of a node within a network
/// A node contains a (shader) type identifier, parameter values, and 
/// connections to upstream nodes. A single input (mapped by TfToken) may have
/// multiple upstream connections to describe connected array elements.
struct HdMaterialNode2 {
    TfToken nodeTypeId;
    std::map<TfToken, VtValue> parameters;
    std::map<TfToken, std::vector<HdMaterialConnection2>> inputConnections;

    bool operator==(const HdMaterialNode2 & rhs) const {
        return nodeTypeId == rhs.nodeTypeId
            && parameters == rhs.parameters
            && inputConnections == rhs.inputConnections;
    }
};

/// \struct HdMaterialNetwork2
/// 
/// Container of nodes and top-level terminal connections. This is the mutable
/// representation of a shading network sent to filtering functions by a
/// MatfiltFilterChain.
struct HdMaterialNetwork2 {
    std::map<SdfPath, HdMaterialNode2> nodes;
    std::map<TfToken, HdMaterialConnection2> terminals;
    TfTokenVector primvars;

    bool operator==(const HdMaterialNetwork2 & rhs) const {
        return nodes == rhs.nodes 
            && terminals == rhs.terminals
            && primvars == rhs.primvars;
    }
};

/// Converts a HdMaterialNetworkMap to a HdMaterialNetwork2
HD_API
void HdMaterialNetwork2ConvertFromHdMaterialNetworkMap(
    const HdMaterialNetworkMap & hdNetworkMap,
    HdMaterialNetwork2 *result, 
    bool *isVolume = nullptr);


// VtValue requirements
HD_API
std::ostream& operator<<(std::ostream& out, const HdMaterialNetwork& pv);
HD_API
bool operator==(const HdMaterialNetwork& lhs, const HdMaterialNetwork& rhs);
HD_API
bool operator!=(const HdMaterialNetwork& lhs, const HdMaterialNetwork& rhs);

HD_API
std::ostream& operator<<(std::ostream& out,
                         const HdMaterialNetworkMap& pv);
HD_API
bool operator==(const HdMaterialNetworkMap& lhs,
                const HdMaterialNetworkMap& rhs);
HD_API
bool operator!=(const HdMaterialNetworkMap& lhs,
                const HdMaterialNetworkMap& rhs);


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_MATERIAL_H
