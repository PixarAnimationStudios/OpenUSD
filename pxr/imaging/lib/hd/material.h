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
#ifndef HD_MATERIAL_H
#define HD_MATERIAL_H

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
        DirtySurfaceShader    = 1 << 2,
        DirtyParams           = 1 << 3,
        DirtyResource         = 1 << 4,
        AllDirty              = (DirtySurfaceShader
                                 |DirtyParams
                                 |DirtyResource)
    };

    HD_API
    virtual ~HdMaterial();

    /// Causes the shader to be reloaded.
    virtual void Reload() = 0;

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
/// Describes a connection between two nodes/terminals.
struct HdMaterialRelationship {
    SdfPath sourceId;
    TfToken sourceTerminal;
    SdfPath remoteId;
    TfToken remoteTerminal;
};

// VtValue requirements
bool operator==(const HdMaterialRelationship& lhs, 
                const HdMaterialRelationship& rhs);


/// \struct HdMaterialNode
///
/// Describes a material node which is made of a path, a type and
/// a list of parameters.
struct HdMaterialNode {
    SdfPath path;
    TfToken type;
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
};

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

#endif // HD_MATERIAL_H
