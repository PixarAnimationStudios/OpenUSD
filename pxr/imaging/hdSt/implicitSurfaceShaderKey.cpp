//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/implicitSurfaceShaderKey.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticTokens.h"

#include <utility>

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
    ((typeDef,                  "ImplicitSurface.TypeDef"))
    ((vertUtils,                "ImplicitSurface.Vertex.Utils"))
    ((fragUtils,                "ImplicitSurface.Fragment.Utils"))

    // shape-specific mixins (must precede the main mixins)
    ((sphereTypeDef,            "ImplicitSurface.TypeDef.Sphere"))
    ((sphereBounds,             "ImplicitSurface.Bounds.Sphere"))
    ((sphereIntersection,       "ImplicitSurface.Intersection.Sphere"))
    ((sphereVertex,             "ImplicitSurface.Vertex.Sphere"))
    ((sphereFragment,           "ImplicitSurface.Fragment.Sphere"))

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
);

namespace {
const TfToken _CullStyleToken(const HdCullStyle cullStyle,
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

const TfToken _NormalsToken(const bool doubleSided)
{
    if (doubleSided) {
        return _tokens->normalsDoubleSidedFS;
    }
    else {
        return _tokens->normalsSingleSidedFS;
    }
}

// The set of glslfx sections a shape module must supply.
struct _ShapeSections
{
    TfToken typeDef;
    TfToken bounds;
    TfToken intersection;
    TfToken vertexGlue;
    TfToken fragmentGlue;
};

const _ShapeSections &_GetShapeSections(TfToken const &primType)
{
    static const std::pair<TfToken, _ShapeSections> shapeTable[] = {
        { HdPrimTypeTokens->sphere,
          { _tokens->sphereTypeDef,
            _tokens->sphereBounds,
            _tokens->sphereIntersection,
            _tokens->sphereVertex,
            _tokens->sphereFragment } }
    };

    for (auto const &entry : shapeTable) {
        if (entry.first == primType) {
            return entry.second;
        }
    }

    TF_CODING_ERROR("Unsupported implicit surface prim type for Storm '%s'",
                    primType.GetText());
    static const _ShapeSections emptySections{};
    return emptySections;
}
}

HdSt_ImplicitSurfaceShaderKey::HdSt_ImplicitSurfaceShaderKey(
    const HdCullStyle cullStyle,
    const bool doubleSided,
    const uint32_t vertexCount,
    TfToken const &primType)
    : cullStyle(cullStyle)
    , doubleSided(doubleSided)
    , vertexCount(vertexCount)
    , glslfx(_tokens->baseGLSLFX)
{
    _ShapeSections const &shapeSections = _GetShapeSections(primType);

    VS = { _tokens->instancing,
           _tokens->transformUtils,
           _tokens->depthUtils,
           _tokens->typeDef,
           _tokens->vertUtils,
           shapeSections.typeDef,
           shapeSections.bounds,
           shapeSections.vertexGlue,
           _tokens->mainVS,
           _tokens->pointIdNoneVS,
           TfToken() };

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
           _tokens->typeDef,
           _tokens->fragUtils,
           shapeSections.typeDef,
           shapeSections.intersection,
           shapeSections.fragmentGlue,
           _tokens->mainFS,
           _tokens->pointIdFallbackFS,
           TfToken() };
}

PXR_NAMESPACE_CLOSE_SCOPE
