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
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/pxOsd/subdivTags.h"

PXR_NAMESPACE_OPEN_SCOPE

#define HD_MESH_REPR_DESC_TOKENS \
    (surfaceShader)              \
    (surfaceShaderUnlit)         \
    (surfaceShaderSheer)         \
    (surfaceShaderOutline)       \
    (constantColor)              \
    (hullColor)                  \
    (pointColor)

TF_DECLARE_PUBLIC_TOKENS(HdMeshReprDescTokens, HD_API,
        HD_MESH_REPR_DESC_TOKENS);

/// \class HdMeshReprDesc
///
/// descriptor to configure a drawItem for a repr
///
struct HdMeshReprDesc {
    HdMeshReprDesc(HdMeshGeomStyle geomStyle = HdMeshGeomStyleInvalid,
                   HdCullStyle cullStyle = HdCullStyleDontCare,
                   TfToken shadingTerminal = HdMeshReprDescTokens->surfaceShader,
                   bool smoothNormals = false,
                   bool blendWireframeColor = true,
                   bool doubleSided = false,
                   float lineWidth = 0,
                   bool useCustomDisplacement = true)
        : geomStyle(geomStyle)
        , cullStyle(cullStyle)
        , shadingTerminal(shadingTerminal)
        , smoothNormals(smoothNormals)
        , blendWireframeColor(blendWireframeColor)
        , doubleSided(doubleSided)
        , lineWidth(lineWidth)
        , useCustomDisplacement(useCustomDisplacement)
        {}

    /// The rendering style: draw refined/unrefined, edge, points, etc.
    HdMeshGeomStyle geomStyle;
    /// The culling style: draw front faces, back faces, etc.
    HdCullStyle     cullStyle;
    /// Specifies how the fragment color should be computed from surfaceShader;
    /// this can be used to render a mesh lit, unlit, unshaded, etc.
    TfToken         shadingTerminal;
    /// Does this mesh need to generate smooth normals?
    bool            smoothNormals;
    /// Should the wireframe color be blended into the color primvar?
    bool            blendWireframeColor;
    /// Should this mesh be treated as double-sided? The resolved value is
    /// (prim.doubleSided || repr.doubleSided).
    bool            doubleSided;
    /// How big (in pixels) should line drawing be?
    float           lineWidth;
    /// Should this mesh use displacementShader() to displace points?
    bool            useCustomDisplacement;
};

/// Hydra Schema for a subdivision surface or poly-mesh object.
///
class HdMesh : public HdRprim {
public:
    HD_API
    virtual ~HdMesh();

    ///
    /// Render State
    ///
    inline bool        IsDoubleSided(HdSceneDelegate* delegate) const;
    inline HdCullStyle GetCullStyle(HdSceneDelegate* delegate)  const;
    inline VtValue     GetShadingStyle(HdSceneDelegate* delegate)  const;

    ///
    /// Topological accessors via the scene delegate
    ///
    inline HdMeshTopology  GetMeshTopology(HdSceneDelegate* delegate) const;
    inline HdDisplayStyle  GetDisplayStyle(HdSceneDelegate* delegate)  const;
    inline PxOsdSubdivTags GetSubdivTags(HdSceneDelegate* delegate)   const;

    /// Topology getter
    virtual HdMeshTopologySharedPtr  GetTopology() const;

    ///
    /// Primvars Accessors
    ///
    inline VtValue GetPoints(HdSceneDelegate* delegate)  const;
    inline VtValue GetNormals(HdSceneDelegate* delegate) const;

    /// Configure geometric style of drawItems for \p reprName
    /// HdMesh can have up to 2 descriptors for some complex styling
    /// (FeyRay, Outline)
    HD_API
    static void ConfigureRepr(TfToken const &reprName,
                              HdMeshReprDesc desc1,
                              HdMeshReprDesc desc2=HdMeshReprDesc());

protected:
    /// Constructor. instancerId, if specified, is the instancer which uses
    /// this mesh as a prototype.
    HD_API
    HdMesh(SdfPath const& id,
           SdfPath const& instancerId = SdfPath());

    typedef _ReprDescConfigs<HdMeshReprDesc, /*max drawitems=*/2>
        _MeshReprConfig;

    HD_API
    static _MeshReprConfig::DescArray _GetReprDesc(TfToken const &reprName);

private:

    // Class can not be default constructed or copied.
    HdMesh()                           = delete;
    HdMesh(const HdMesh &)             = delete;
    HdMesh &operator =(const HdMesh &) = delete;

    static _MeshReprConfig _reprDescConfig;
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

inline VtValue
HdMesh::GetShadingStyle(HdSceneDelegate* delegate) const
{
    return delegate->GetShadingStyle(GetId());
}

inline HdMeshTopology
HdMesh::GetMeshTopology(HdSceneDelegate* delegate) const
{
    return delegate->GetMeshTopology(GetId());
}

inline HdDisplayStyle
HdMesh::GetDisplayStyle(HdSceneDelegate* delegate) const
{
    return delegate->GetDisplayStyle(GetId());
}

inline PxOsdSubdivTags
HdMesh::GetSubdivTags(HdSceneDelegate* delegate) const
{
    return delegate->GetSubdivTags(GetId());
}

inline HdMeshTopologySharedPtr
HdMesh::GetTopology() const
{
    return HdMeshTopologySharedPtr();
}

inline VtValue
HdMesh::GetPoints(HdSceneDelegate* delegate) const
{
    return GetPrimvar(delegate, HdTokens->points);
}

inline VtValue
HdMesh::GetNormals(HdSceneDelegate* delegate) const
{
    return GetPrimvar(delegate, HdTokens->normals);
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_MESH_H
