//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/sphereShaderKey.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((baseGLSLFX,               "sphere.glslfx"))

    ((normalsDoubleSidedFS,     "SphereNormal.Fragment.DoubleSided"))
    ((normalsSingleSidedFS,     "SphereNormal.Fragment.SingleSided"))

    ((faceCullNoneFS,           "SphereFaceCull.Fragment.None"))
    ((faceCullFrontFacingFS,    "SphereFaceCull.Fragment.FrontFacing"))
    ((faceCullBackFacingFS,     "SphereFaceCull.Fragment.BackFacing"))

    // point id mixins (for point picking & selection)
    ((pointIdNoneVS,            "PointId.Vertex.None"))
    ((pointIdFallbackFS,        "PointId.Fragment.Fallback"))

    // edge id mixins (for edge picking & selection)
    ((edgeIdNoneFS,             "EdgeId.Fragment.None"))

    // main for all the shader stages
    ((mainVS,                   "Sphere.Vertex"))
    ((mainFS,                   "Sphere.Fragment"))

    // terminals        
    ((commonFS,                 "Fragment.CommonTerminals"))
    ((surfaceFS,                "Fragment.Surface"))
    ((noScalarOverrideFS,       "Fragment.NoScalarOverride"))

    // instancing       
    ((instancing,               "Instancing.Transform"))
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
}

HdSt_SphereShaderKey::HdSt_SphereShaderKey(
    const HdCullStyle cullStyle,
    const bool doubleSided,
    const uint32_t vertexCount)
    : cullStyle(cullStyle)
    , doubleSided(doubleSided)
    , vertexCount(vertexCount)
    , glslfx(_tokens->baseGLSLFX)
    , VS{ _tokens->instancing,
          _tokens->mainVS,
          _tokens->pointIdNoneVS,
          TfToken() }
    , FS{ _tokens->commonFS,
          _tokens->edgeIdNoneFS,
          _tokens->surfaceFS,
          _tokens->noScalarOverrideFS,
          _tokens->instancing,
          _NormalsToken(doubleSided),
          _CullStyleToken(cullStyle, doubleSided),
          _tokens->mainFS,
          _tokens->pointIdFallbackFS,
          TfToken() }
{
}

HdSt_SphereShaderKey::~HdSt_SphereShaderKey() = default;

PXR_NAMESPACE_CLOSE_SCOPE

