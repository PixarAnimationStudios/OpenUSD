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

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/pxOsd/subdivTags.h"

PXR_NAMESPACE_OPEN_SCOPE


/// Hydra Schema for a subdivision surface or poly-mesh object.
///
class HdMesh : public HdRprim {
public:
    virtual ~HdMesh();

    ///
    /// Render State
    ///
    inline bool        IsDoubleSided(HdSceneDelegate* delegate) const;
    inline HdCullStyle GetCullStyle(HdSceneDelegate* delegate)  const;

    ///
    /// Topology
    ///
    inline HdMeshTopology  GetMeshTopology(HdSceneDelegate* delegate) const;
    inline int             GetRefineLevel(HdSceneDelegate* delegate)  const;
    inline PxOsdSubdivTags GetSubdivTags(HdSceneDelegate* delegate)   const;


    ///
    /// Primvars Accessors
    ///
    inline VtValue GetPoints(HdSceneDelegate* delegate)  const;
    inline VtValue GetNormals(HdSceneDelegate* delegate) const;

protected:
    /// Constructor. instancerId, if specified, is the instancer which uses
    /// this mesh as a prototype.
    HdMesh(SdfPath const& id,
           SdfPath const& instancerId = SdfPath());

private:

    // Class can not be default constructed or copied.
    HdMesh()                           = delete;
    HdMesh(const HdMesh &)             = delete;
    HdMesh &operator =(const HdMesh &) = delete;
};

inline bool
HdMesh::IsDoubleSided(HdSceneDelegate* delegate) const
{
    return delegate->GetDoubleSided(GetId());
}

inline HdCullStyle
HdMesh::GetCullStyle(HdSceneDelegate* delegate) const
{
    return delegate->GetCullStyle(GetId());
}

inline HdMeshTopology
HdMesh::GetMeshTopology(HdSceneDelegate* delegate) const
{
    return delegate->GetMeshTopology(GetId());
}

inline int
HdMesh::GetRefineLevel(HdSceneDelegate* delegate) const
{
    return delegate->GetRefineLevel(GetId());
}

inline PxOsdSubdivTags
HdMesh::GetSubdivTags(HdSceneDelegate* delegate) const
{
    return delegate->GetSubdivTags(GetId());
}

inline VtValue
HdMesh::GetPoints(HdSceneDelegate* delegate) const
{
    return GetPrimVar(delegate, HdTokens->points);
}

inline VtValue
HdMesh::GetNormals(HdSceneDelegate* delegate) const
{
    return GetPrimVar(delegate, HdTokens->normals);
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_MESH_H
