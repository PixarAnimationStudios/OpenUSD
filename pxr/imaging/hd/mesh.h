//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_MESH_H
#define PXR_IMAGING_HD_MESH_H

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
/// Descriptor to configure the drawItem(s) for a repr
///
struct HdMeshReprDesc
{
    HdMeshReprDesc(HdMeshGeomStyle geomStyle = HdMeshGeomStyleInvalid,
                   HdCullStyle cullStyle = HdCullStyleDontCare,
                   TfToken shadingTerminal = HdMeshReprDescTokens->surfaceShader,
                   bool flatShadingEnabled = false,
                   bool blendWireframeColor = true,
                   bool forceOpaqueEdges = true,
                   bool surfaceEdgeIds = false,
                   bool doubleSided = false,
                   float lineWidth = 0,
                   bool useCustomDisplacement = true,
                   bool enableScalarOverride = true)
        : geomStyle(geomStyle)
        , cullStyle(cullStyle)
        , shadingTerminal(shadingTerminal)
        , flatShadingEnabled(flatShadingEnabled)
        , blendWireframeColor(blendWireframeColor)
        , forceOpaqueEdges(forceOpaqueEdges)
        , surfaceEdgeIds(surfaceEdgeIds)
        , doubleSided(doubleSided)
        , lineWidth(lineWidth)
        , useCustomDisplacement(useCustomDisplacement)
        , enableScalarOverride(enableScalarOverride)
        {}
    
    bool IsEmpty() const {
        return geomStyle == HdMeshGeomStyleInvalid;
    }

    /// The rendering style: draw refined/unrefined, edge, points, etc.
    HdMeshGeomStyle geomStyle;
    /// The culling style: draw front faces, back faces, etc.
    HdCullStyle     cullStyle;
    /// Specifies how the fragment color should be computed from surfaceShader;
    /// this can be used to render a mesh lit, unlit, unshaded, etc.
    TfToken         shadingTerminal;
    /// Does this mesh want flat shading?
    bool            flatShadingEnabled;
    /// Should the wireframe color be blended into the color primvar?
    bool            blendWireframeColor;
    /// If the geom style includes edges, should those edges be forced
    /// to be fully opaque, ignoring any applicable opacity inputs.
    /// Does not apply to patch edges.
    bool            forceOpaqueEdges;
    /// Generate edge ids for surface and hull geom styles that do not
    /// otherwise render edges, e.g. to support picking and highlighting
    /// of edges with these mesh geom styles.
    bool            surfaceEdgeIds;
    /// Should this mesh be treated as double-sided? The resolved value is
    /// (prim.doubleSided || repr.doubleSided).
    bool            doubleSided;
    /// How big (in pixels) should line drawing be?
    float           lineWidth;
    /// Should this mesh use displacementShader() to displace points?
    bool            useCustomDisplacement;
    /// Should scalar override be allowed on this drawItem.
    /// scalar override allows for visualization of a single float value
    /// across a prim.
    bool            enableScalarOverride;
};

/// Hydra Schema for a subdivision surface or poly-mesh object.
///
class HdMesh : public HdRprim
{
public:
    HD_API
    ~HdMesh() override;

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

    HD_API
    TfTokenVector const & GetBuiltinPrimvarNames() const override;

    /// Configure the geometric style of the mesh for a given representation.
    /// We currently allow up to 2 descriptors for a representation.
    /// Example of when this may be useful:
    ///     Drawing the outline in addition to the surface for a mesh.
    HD_API
    static void ConfigureRepr(TfToken const &reprName,
                              HdMeshReprDesc desc1,
                              HdMeshReprDesc desc2=HdMeshReprDesc());

protected:
    /// Constructor. instancerId, if specified, is the instancer which uses
    /// this mesh as a prototype.
    HD_API
    HdMesh(SdfPath const& id);

    // We allow up to 2 repr descs per repr for meshes (see ConfigureRepr above)
    using _MeshReprConfig = _ReprDescConfigs<HdMeshReprDesc, 2>;

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

#endif //PXR_IMAGING_HD_MESH_H
