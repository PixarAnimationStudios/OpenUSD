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

    ((conicalFrustumTypeDef,        "ImplicitSurface.TypeDef.ConicalFrustum"))
    ((conicalFrustumCommon,         "ImplicitSurface.Common.ConicalFrustum"))
    ((conicalFrustumBounds,         "ImplicitSurface.Bounds.ConicalFrustum"))
    ((conicalFrustumIntersection,   "ImplicitSurface.Intersection.ConicalFrustum"))
    ((conicalFrustumVertex,         "ImplicitSurface.Vertex.ConicalFrustum"))
    ((conicalFrustumFragment,       "ImplicitSurface.Fragment.ConicalFrustum"))

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
            _tokens->sphereFragment } },
        { HdStNativeImplicitsTokens->conicalFrustum,
          { { _tokens->conicalFrustumTypeDef },
            _tokens->conicalFrustumCommon,
            { _tokens->conicalFrustumBounds },
            _tokens->conicalFrustumVertex,
            { _tokens->conicalFrustumIntersection },
            _tokens->conicalFrustumFragment } },
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
    FS.push_back(_tokens->mainFS);
    FS.push_back(_tokens->pointIdFallbackFS);
    FS.push_back(TfToken());
}

PXR_NAMESPACE_CLOSE_SCOPE
