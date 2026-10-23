//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/implicitSurfaceShaderKey.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticTokens.h"

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((baseGLSLFX,               "implicitSurface.glslfx"))

    ((normalsDoubleSidedFS,     "ImplicitSurfaceNormal.Fragment.DoubleSided"))
    ((normalsSingleSidedFS,     "ImplicitSurfaceNormal.Fragment.SingleSided"))

    ((faceCullNoneFS,           "ImplicitSurfaceFaceCull.Fragment.None"))
    ((faceCullFrontFacingFS,    "ImplicitSurfaceFaceCull.Fragment.FrontFacing"))
    ((faceCullBackFacingFS,     "ImplicitSurfaceFaceCull.Fragment.BackFacing"))

    // point id mixins (for point picking & selection)
    ((pointIdNoneVS,            "PointId.Vertex.None"))
    ((pointIdFallbackFS,        "PointId.Fragment.Fallback"))

    // edge id mixins (for edge picking & selection)
    ((edgeIdNoneFS,             "EdgeId.Fragment.None"))

    // shape-agnostic type defs and utils shared by every shape module
    ((commonConstants,          "ImplicitSurface.Common.Constants"))
    ((vertexTypeDef,            "ImplicitSurface.Vertex.TypeDef"))
    ((fragmentTypeDef,          "ImplicitSurface.Fragment.TypeDef"))
    ((vertUtils,                "ImplicitSurface.Vertex.Utils"))
    ((fragUtils,                "ImplicitSurface.Fragment.Utils"))

    // shape-specific mixins (must precede the main mixins)
    ((sphereTypeDef,            "ImplicitSurface.TypeDef.Sphere"))
    ((sphereCommon,             "ImplicitSurface.Common.Sphere"))
    ((sphereBounds,             "ImplicitSurface.Bounds.Sphere"))
    ((sphereIntersection,       "ImplicitSurface.Intersection.Sphere"))
    ((sphereVertex,             "ImplicitSurface.Vertex.Sphere"))
    ((sphereFragment,           "ImplicitSurface.Fragment.Sphere"))

    ((conicalTypeDef,           "ImplicitSurface.TypeDef.ConicalFrustum"))
    ((conicalCommon,            "ImplicitSurface.Common.ConicalFrustum"))
    ((conicalBounds,            "ImplicitSurface.Bounds.ConicalFrustum"))
    ((conicalIntersection,      "ImplicitSurface.Intersection.ConicalFrustum"))
    ((conicalVertex,            "ImplicitSurface.Vertex.ConicalFrustum"))
    ((conicalFragment,          "ImplicitSurface.Fragment.ConicalFrustum"))

    ((capsuleCommon,            "ImplicitSurface.Common.Capsule"))
    ((capsuleVertex,            "ImplicitSurface.Vertex.Capsule"))
    ((capsuleFragment,          "ImplicitSurface.Fragment.Capsule"))

    // main for all the shader stages
    ((mainVS,                   "ImplicitSurface.Vertex"))
    ((mainFS,                   "ImplicitSurface.Fragment"))

    // terminals
    ((commonFS,                 "Fragment.CommonTerminals"))
    ((surfaceFS,                "Fragment.Surface"))
    ((noScalarOverrideFS,       "Fragment.NoScalarOverride"))

    // instancing
    ((instancing,               "Instancing.Transform"))

    // utils
    ((transformUtils,           "CoordUtils.Transform"))
    ((projectionUtils,          "CoordUtils.Projection"))
    ((depthUtils,               "CoordUtils.Depth"))

    // shape-agnostic wireframe sections
    ((edgeCommonFS,             "ImplicitSurfaceWire.Fragment.EdgeCommon"))
    ((edgeOpacityNoForceFS,     "ImplicitSurfaceWire.Fragment.FinalEdgeOpacityNoForce"))
    ((edgeOpacityForceFS,       "ImplicitSurfaceWire.Fragment.FinalEdgeOpacityForce"))
    ((edgeOnlyCommonFS,         "ImplicitSurfaceWire.Fragment.EdgeOnlyCommon"))
    ((noEdgeFS,                 "ImplicitSurfaceWire.Fragment.NoEdge"))
    ((edgeOnSurfaceFS,          "ImplicitSurfaceWire.Fragment.EdgeOnSurface"))
    ((edgeOnlyBlendFS,          "ImplicitSurfaceWire.Fragment.EdgeOnlyBlendColor"))
    ((edgeOnlyNoBlendFS,        "ImplicitSurfaceWire.Fragment.EdgeOnlyNoBlend"))

    // shape-specific wireframe sections
    ((sphereEdge,               "ImplicitSurface.Edge.Sphere"))
    ((sphereWire,               "ImplicitSurface.Wire.Sphere"))
    ((conicalEdge,              "ImplicitSurface.Edge.ConicalFrustum"))
    ((conicalWire,              "ImplicitSurface.Wire.ConicalFrustum"))
    ((capsuleWire,              "ImplicitSurface.Wire.Capsule"))
);

namespace {
const TfToken& _CullStyleToken(const HdCullStyle cullStyle,
                               const bool doubleSided)
{
    if (cullStyle == HdCullStyleFront ||
        (cullStyle == HdCullStyleFrontUnlessDoubleSided && !doubleSided)) {
        return _tokens->faceCullFrontFacingFS;
    }
    else if (cullStyle == HdCullStyleBack ||
            (cullStyle == HdCullStyleBackUnlessDoubleSided && !doubleSided)) {
        return _tokens->faceCullBackFacingFS;
    }
    else {
        return _tokens->faceCullNoneFS;
    }
}

const TfToken& _NormalsToken(const bool doubleSided)
{
    if (doubleSided) {
        return _tokens->normalsDoubleSidedFS;
    }
    else {
        return _tokens->normalsSingleSidedFS;
    }
}

bool _DrawsEdges(const HdImplicitSurfaceGeomStyle geomStyle)
{
    return geomStyle == HdImplicitSurfaceGeomStyleEdgeOnSurf ||
           geomStyle == HdImplicitSurfaceGeomStyleEdgeOnly;
}

const TfToken& _EdgeOpacityToken(const bool forceOpaqueEdges)
{
    return forceOpaqueEdges ? _tokens->edgeOpacityForceFS
                            : _tokens->edgeOpacityNoForceFS;
}

const TfToken& _WireframeModeToken(const HdImplicitSurfaceGeomStyle geomStyle,
                                   const bool blendWireframeColor)
{
    switch (geomStyle) {
        case HdImplicitSurfaceGeomStyleEdgeOnSurf:
            return _tokens->edgeOnSurfaceFS;
        case HdImplicitSurfaceGeomStyleEdgeOnly:
            return blendWireframeColor
                   ? _tokens->edgeOnlyBlendFS
                   : _tokens->edgeOnlyNoBlendFS;
        default:
            return _tokens->noEdgeFS;
    }
}

// The set of glslfx sections for a shape module.
struct _ShapeSections
{
    // optional: added to both stages, before common
    TfSmallVector<TfToken, 2> commonExtras;

    // required: added to both stages before ImplicitSurface.<Stage>.TypeDef
    TfToken common;

    // optional: added to VS only, before vertexGlue
    TfSmallVector<TfToken, 2> vsExtras;

    // required: added to VS only
    TfToken vertexGlue;

    // optional: added to FS only, before fragmentGlue
    TfSmallVector<TfToken, 2> fsExtras;

    // required: added to FS only
    TfToken fragmentGlue;

    // optional: added to FS only when edges are drawn, after fragmentGlue
    TfSmallVector<TfToken, 2> wireSections;
};

const _ShapeSections &_GetShapeSections(TfToken const &primType)
{
    static const std::unordered_map<TfToken, _ShapeSections,
                                    TfToken::HashFunctor> shapeMap = {
        { HdStNativeImplicitsTokens->sphere,
          { { _tokens->sphereTypeDef },
            _tokens->sphereCommon,
            { _tokens->sphereBounds },
            _tokens->sphereVertex,
            { _tokens->sphereIntersection },
            _tokens->sphereFragment,
            { _tokens->sphereEdge,
              _tokens->sphereWire } } },
        { HdStNativeImplicitsTokens->conicalFrustum,
          { { _tokens->conicalTypeDef },
            _tokens->conicalCommon,
            { _tokens->conicalBounds },
            _tokens->conicalVertex,
            { _tokens->conicalIntersection },
            _tokens->conicalFragment,
            { _tokens->conicalEdge,
              _tokens->conicalWire } } },
        { HdStNativeImplicitsTokens->capsule,
          { { _tokens->conicalTypeDef,
              _tokens->sphereTypeDef   },
            _tokens->capsuleCommon,
            { _tokens->sphereBounds },
            _tokens->capsuleVertex,
            { _tokens->conicalIntersection,
              _tokens->sphereIntersection },
            _tokens->capsuleFragment,
            { _tokens->conicalEdge,
              _tokens->capsuleWire } } },
    };

    auto it = shapeMap.find(primType);
    if (it == shapeMap.end()) {
        TF_CODING_ERROR("Unsupported implicit surface prim type for Storm '%s'",
                        primType.GetText());
        static const _ShapeSections emptySections{};
        return emptySections;
    }

    return it->second;
}
}

HdSt_ImplicitSurfaceShaderKey::HdSt_ImplicitSurfaceShaderKey(
    const HdCullStyle cullStyle,
    const bool doubleSided,
    const uint32_t vertexCount,
    TfToken const &primType,
    const HdImplicitSurfaceGeomStyle geomStyle,
    const bool blendWireframeColor,
    const bool forceOpaqueEdges)
    : cullStyle(cullStyle)
    , doubleSided(doubleSided)
    , vertexCount(vertexCount)
    , glslfx(_tokens->baseGLSLFX)
{
    _ShapeSections const &shapeSections = _GetShapeSections(primType);

    VS = { _tokens->instancing,
           _tokens->transformUtils,
           _tokens->depthUtils,
           _tokens->commonConstants };

    for (TfToken const &section : shapeSections.commonExtras) {
        VS.push_back(section);
    }

    VS.push_back(shapeSections.common);

    VS.push_back(_tokens->vertexTypeDef);
    VS.push_back(_tokens->vertUtils);

    for (TfToken const &section : shapeSections.vsExtras) {
        VS.push_back(section);
    }

    VS.push_back(shapeSections.vertexGlue);
    VS.push_back(_tokens->mainVS);
    VS.push_back(_tokens->pointIdNoneVS);
    VS.push_back(TfToken());

    FS = { _tokens->commonFS,
           _tokens->edgeIdNoneFS,
           _tokens->surfaceFS,
           _tokens->noScalarOverrideFS,
           _tokens->transformUtils,
           _tokens->projectionUtils,
           _tokens->depthUtils,
           _tokens->instancing,
           _NormalsToken(doubleSided),
           _CullStyleToken(cullStyle, doubleSided),
           _tokens->commonConstants };

    for (TfToken const &section : shapeSections.commonExtras) {
        FS.push_back(section);
    }

    FS.push_back(shapeSections.common);
    FS.push_back(_tokens->fragmentTypeDef);
    FS.push_back(_tokens->fragUtils);

    for (TfToken const &section : shapeSections.fsExtras) {
        FS.push_back(section);
    }

    FS.push_back(shapeSections.fragmentGlue);

    // Wireframe sections must follow the shape fragment section and precede
    // main.
    if (_DrawsEdges(geomStyle)) {
        for (const TfToken &section : shapeSections.wireSections) {
            FS.push_back(section);
        }

        FS.push_back(_tokens->edgeCommonFS);
        FS.push_back(_EdgeOpacityToken(forceOpaqueEdges));
    }

    if (geomStyle == HdImplicitSurfaceGeomStyleEdgeOnly) {
        FS.push_back(_tokens->edgeOnlyCommonFS);
    }

    FS.push_back(_WireframeModeToken(geomStyle, blendWireframeColor));

    FS.push_back(_tokens->mainFS);
    FS.push_back(_tokens->pointIdFallbackFS);
    FS.push_back(TfToken());
}

PXR_NAMESPACE_CLOSE_SCOPE
