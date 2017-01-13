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
#ifndef HD_MESH_H
#define HD_MESH_H

#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/pxOsd/subdivTags.h"

/// Hydra Schema for a subdivision surface or poly-mesh object.
///
class HdMesh : public HdRprim {
public:
    virtual ~HdMesh();

    ///
    /// Render State
    ///
    bool        IsDoubleSided() const { return GetDelegate()->GetDoubleSided(GetId()); }
    HdCullStyle GetCullStyle()  const { return GetDelegate()->GetCullStyle(GetId());   }

    ///
    /// Mesh Topology
    ///
    HdMeshTopology  GetMeshTopology() const { return GetDelegate()->GetMeshTopology(GetId()); }
    int             GetRefineLevel()  const { return GetDelegate()->GetRefineLevel(GetId());  }
    PxOsdSubdivTags GetSubdivTags()   const { return GetDelegate()->GetSubdivTags(GetId());   }

    ///
    /// Primvar Query
    ///
    // XXX: Should these be in rprim?
    TfTokenVector GetPrimVarVertexNames()      const { return GetDelegate()->GetPrimVarVertexNames(GetId());      }
    TfTokenVector GetPrimVarVaryingNames()     const { return GetDelegate()->GetPrimVarVaryingNames(GetId());     }
    TfTokenVector GetPrimVarFacevaryingNames() const { return GetDelegate()->GetPrimVarFacevaryingNames(GetId()); }
    TfTokenVector GetPrimVarUniformNames()     const { return GetDelegate()->GetPrimVarUniformNames(GetId());     }

    ///
    /// Primvars Accessors
    ///
    // XXX: Should GetPrimVar be in rprim?
    VtValue GetPrimVar(const TfToken &name) const {  return GetDelegate()->Get(GetId(), name); }
    VtValue GetPoints()  const { return GetPrimVar(HdTokens->points);  }
    VtValue GetNormals() const { return GetPrimVar(HdTokens->normals); }

protected:
    /// Constructor. instancerId, if specified, is the instancer which uses
    /// this mesh as a prototype.
    HdMesh(HdSceneDelegate* delegate, SdfPath const& id,
           SdfPath const& instancerId = SdfPath());

private:

    // Class can not be default constructed or copied.
    HdMesh()                           = delete;
    HdMesh(const HdMesh &)             = delete;
    HdMesh &operator =(const HdMesh &) = delete;
};

#endif //HD_MESH_H
