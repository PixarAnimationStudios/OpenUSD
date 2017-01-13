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
    inline bool        IsDoubleSided() const;
    inline HdCullStyle GetCullStyle()  const;

    ///
    /// Topology
    ///
    inline HdMeshTopology  GetMeshTopology() const;
    inline int             GetRefineLevel()  const;
    inline PxOsdSubdivTags GetSubdivTags()   const;


    ///
    /// Primvars Accessors
    ///
    inline VtValue GetPoints()  const;
    inline VtValue GetNormals() const;

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

inline bool
HdMesh::IsDoubleSided() const
{
    return GetDelegate()->GetDoubleSided(GetId());
}

inline HdCullStyle
HdMesh::GetCullStyle() const
{
    return GetDelegate()->GetCullStyle(GetId());
}

inline HdMeshTopology
HdMesh::GetMeshTopology() const
{
    return GetDelegate()->GetMeshTopology(GetId());
}

inline int
HdMesh::GetRefineLevel() const
{
    return GetDelegate()->GetRefineLevel(GetId());
}

inline PxOsdSubdivTags
HdMesh::GetSubdivTags() const
{
    return GetDelegate()->GetSubdivTags(GetId());
}

inline VtValue
HdMesh::GetPoints() const
{
    return GetPrimVar(HdTokens->points);
}

inline VtValue
HdMesh::GetNormals() const
{
    return GetPrimVar(HdTokens->normals);
}

#endif //HD_MESH_H
