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
#ifndef HD_SHADER_H
#define HD_SHADER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hd/sceneDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE

// XXX: Temporary until Rprim moves to HdSt.
typedef boost::shared_ptr<class HdShaderCode> HdShaderCodeSharedPtr;

///
/// Hydra Schema for a shader object.
///
class HdShader : public HdSprim {
public:
    // change tracking for HdShader prim
    enum DirtyBits : HdDirtyBits {
        Clean                 = 0,
        // XXX: Got to skip varying and force sync bits for now
        DirtySurfaceShader    = 1 << 2,
        DirtyParams           = 1 << 3,
        DirtyComputeShader    = 1 << 4,
        DirtyResource         = 1 << 5,
        AllDirty              = (DirtySurfaceShader
                                 |DirtyParams
                                 |DirtyComputeShader
                                 |DirtyResource)
    };

    HD_API
    virtual ~HdShader();

    /// Causes the shader to be reloaded.
    virtual void Reload() = 0;

    // XXX: Temporary until Rprim moves to HdSt.
    // Obtains the render delegate specific representation of the shader.
    virtual HdShaderCodeSharedPtr GetShaderCode() const = 0;

protected:
    HD_API
    HdShader(SdfPath const& id);

private:
    // Class can not be default constructed or copied.
    HdShader()                             = delete;
    HdShader(const HdShader &)             = delete;
    HdShader &operator =(const HdShader &) = delete;
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


/// \struct HdValueAndRole
///
/// A pair of (value, role).  The role value comes from SdfValueRoleNames
/// and indicates the intended interpretation.  For example, the role
/// indicates whether a GfVec3f value should be interpreted as a color,
/// point, vector, or normal.
struct HdValueAndRole {
    VtValue value;
    TfToken role;
};

// VtValue requirements
bool operator==(const HdValueAndRole& lhs,
                const HdValueAndRole& rhs);

/// \struct HdMaterialNode
///
/// Describes a material node which is made of a path, a type and
/// a list of parameters.
struct HdMaterialNode {
    SdfPath path;
    TfToken type;
    std::map<TfToken, HdValueAndRole> parameters;
};

// VtValue requirements
HD_API
bool operator==(const HdMaterialNode& lhs, const HdMaterialNode& rhs);


/// \struct HdMaterialNodes
///
/// Describes a material network composed of nodes and relationships
/// between the nodes and terminals of those nodes.
struct HdMaterialNodes {
    std::vector<HdMaterialRelationship> relationships;
    std::vector<HdMaterialNode> nodes;
};

// VtValue requirements
HD_API
std::ostream& operator<<(std::ostream& out, const HdMaterialNodes& pv);
HD_API
bool operator==(const HdMaterialNodes& lhs, const HdMaterialNodes& rhs);
HD_API
bool operator!=(const HdMaterialNodes& lhs, const HdMaterialNodes& rhs);


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_SHADER_H
