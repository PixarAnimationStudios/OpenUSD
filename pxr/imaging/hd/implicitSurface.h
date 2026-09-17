//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_IMPLICITSURFACE_H
#define PXR_IMAGING_HD_IMPLICITSURFACE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/rprim.h"

PXR_NAMESPACE_OPEN_SCOPE

#define HD_IMPLICITSURFACE_REPR_DESC_TOKENS \
    (surfaceShader)                         \
    (surfaceShaderUnlit)                    \
    (surfaceShaderSheer)                    \
    (surfaceShaderOutline)                  \
    (constantColor)                         \
    (hullColor)                             \
    (pointColor)

TF_DECLARE_PUBLIC_TOKENS(HdImplicitSurfaceReprDescTokens, HD_API,
        HD_IMPLICITSURFACE_REPR_DESC_TOKENS);

/// \class HdImplicitSurfaceReprDesc
///
/// Descriptor to configure the drawItem for a repr
///
struct HdImplicitSurfaceReprDesc
{
    HdImplicitSurfaceReprDesc(
                HdImplicitSurfaceGeomStyle geomStyle = HdImplicitSurfaceGeomStyleInvalid,
                HdCullStyle cullStyle = HdCullStyleDontCare,
                TfToken shadingTerminal = HdImplicitSurfaceReprDescTokens->surfaceShader,
                bool blendWireframeColor = true,
                bool forceOpaqueRings = true,
                bool doubleSided = false)
        : geomStyle(geomStyle)
        , cullStyle(cullStyle)
        , shadingTerminal(shadingTerminal)
        , blendWireframeColor(blendWireframeColor)
        , forceOpaqueRings(forceOpaqueRings)
        , doubleSided(doubleSided)
        {}

    bool IsEmpty() const {
        return geomStyle == HdImplicitSurfaceGeomStyleInvalid;
    }

    /// The rendering style: surface, wireframe-only, wireframe-on-surface etc.
    HdImplicitSurfaceGeomStyle geomStyle;
    /// The culling style: draw front faces, back faces, etc.
    HdCullStyle                cullStyle;
    /// Specifies how the fragment color should be computed from surfaceShader;
    /// this can be used to render the surface lit, unlit, unshaded, etc.
    TfToken                    shadingTerminal;
    /// Should the wireframe color be blended into the color primvar?
    bool                       blendWireframeColor;
    /// If the geom style includes wireframe lines, should those lines be
    /// forced to be fully opaque, ignoring any applicable opacity inputs.
    bool                       forceOpaqueRings;
    /// Should this surface be treated as double-sided? The resolved value is
    /// (prim.doubleSided || repr.doubleSided).
    bool                       doubleSided;
};

/// \class HdImplicitSurface
///
/// Hydra Schema for a natively supported implicit surface (e.g., spheres).
///
class HdImplicitSurface : public HdRprim
{
public:
    HD_API
    ~HdImplicitSurface() override;

    ///
    /// Render State
    ///
    bool IsDoubleSided(HdSceneDelegate* delegate) const {
        return delegate->GetDoubleSided(GetId());
    }

    HdCullStyle GetCullStyle(HdSceneDelegate* delegate) const {
        return delegate->GetCullStyle(GetId());
    }

    HdDisplayStyle GetDisplayStyle(HdSceneDelegate* delegate) const {
        return delegate->GetDisplayStyle(GetId());
    }

    HD_API
    TfTokenVector const & GetBuiltinPrimvarNames() const override;

    /// Configure geometric style of drawItems for \p reprName
    HD_API
    static void ConfigureRepr(TfToken const &reprName,
                              HdImplicitSurfaceReprDesc desc);

protected:
    HD_API
    explicit HdImplicitSurface(SdfPath const& id);

    using _ImplicitSurfaceReprConfig = _ReprDescConfigs<HdImplicitSurfaceReprDesc>;

    HD_API
    static _ImplicitSurfaceReprConfig::DescArray
        _GetReprDesc(TfToken const &reprName);

private:

    // Class can not be default constructed, copied, or moved.
    HdImplicitSurface()                                       = delete;
    HdImplicitSurface(const HdImplicitSurface &)              = delete;
    HdImplicitSurface &operator=(const HdImplicitSurface &)   = delete;
    HdImplicitSurface(HdImplicitSurface &&)                   = delete;
    HdImplicitSurface &operator=(HdImplicitSurface &&)        = delete;

    static _ImplicitSurfaceReprConfig _reprDescConfig;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_IMPLICITSURFACE_H
