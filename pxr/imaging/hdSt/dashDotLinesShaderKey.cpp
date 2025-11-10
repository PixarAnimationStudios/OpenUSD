//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/imaging/hdSt/dashDotLinesShaderKey.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/dashDotLines.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((dashDotGLSLFX,                   "dashDotLines.glslfx"))

    // point id mixins (for point picking & selection)
    ((pointIdNoneVS,                   "PointId.Vertex.None"))
    ((pointIdFallbackFS,               "PointId.Fragment.Fallback"))

    // visibility mixin (for curve and point visibility)
    ((topVisFallbackFS,                "Visibility.Fragment.Fallback"))
    ((topVisFS,                        "Visibility.Fragment.Topology"))

    // factor
    ((dashDotFactorSS,           "DashDotFactor.ScreenSpace"))
    ((dashDotFactorNoSS,         "DashDotFactor.NoScreenSpace"))

    ((dashDotCalculatedPixelInfo, "DashDot.Calculated.PixelInfo"))
    ((dashDotSampledPixelInfo,    "DashDot.Sampled.PixelInfo"))

    // main for all the shader stages
    ((dashDotDefaultVertex,      "DashDotDefault.Vertex"))
    ((noCapJointVertex,          "NoCapJoint.Vertex"))

    ((dashDotDefaultFragment,    "DashDotDefault.Fragment"))
    ((noCapJointFragment,        "NoCapJoint.Fragment"))

    // instancing related mixins
    ((instancing,                      "Instancing.Transform"))

    // terminals
    ((commonFS,                        "Fragment.CommonTerminals"))
    ((surfaceFS,                       "Fragment.Surface"))
    ((scalarOverrideFS,                "Fragment.ScalarOverride"))

    // rounded points
    ((pointSizeBiasVS,                 "PointDisk.Vertex.PointSizeBias"))
    ((noPointSizeBiasVS,               "PointDisk.Vertex.None"))
    ((diskSampleMaskFS,                "PointDisk.Fragment.SampleMask"))
    ((noDiskSampleMaskFS,              "PointDisk.Fragment.None"))
);

HdSt_DashDotLinesShaderKey::HdSt_DashDotLinesShaderKey(
    const TfToken& shapeDetail,
    bool screenSpacePattern,
    float lineWidth,
    bool hasTexture,
    bool hasAuthoredTopologicalVisibility)
{
    bool simpleImpl = (shapeDetail == HdTokens->noCapJoint);
    _lineWidth = lineWidth;

    glslfx = _tokens->dashDotGLSLFX;

    // We need the adjacent information if the line has style.
    // For noCapJoint, we don't need the adjacent information, so we can still
    // use PRIM_BASIS_CURVES_LINES primitive type.
    if (simpleImpl)
    {
        primType =
            HdSt_GeometricShader::PrimitiveType::PRIM_BASIS_CURVES_LINES;
    } else
    {
        primType = HdSt_GeometricShader::PrimitiveType::PRIM_DASH_DOT_LINES;
    }

    uint8_t vsIndex = 0;

    VS[vsIndex++]  = _tokens->instancing;
    if (simpleImpl)
    {
        VS[vsIndex++] = _tokens->noCapJointVertex;
    }
    else
    {
        VS[vsIndex++] = screenSpacePattern ? _tokens->dashDotFactorSS : _tokens->dashDotFactorNoSS;
        VS[vsIndex++] = _tokens->dashDotDefaultVertex;
    }
    VS[vsIndex++] = _tokens->pointIdNoneVS;
    VS[vsIndex++] = _tokens->noPointSizeBiasVS;
    VS[vsIndex]  = TfToken();

    // setup fragment shaders
    // Common must be first as it defines terminal interfaces
    uint8_t fsIndex = 0;
    FS[fsIndex++] = _tokens->commonFS;
    FS[fsIndex++] = _tokens->surfaceFS;
    FS[fsIndex++] = _tokens->scalarOverrideFS;

    FS[fsIndex++] = _tokens->pointIdFallbackFS;
    FS[fsIndex++] = _tokens->noDiskSampleMaskFS;

    FS[fsIndex++] = hasAuthoredTopologicalVisibility? _tokens->topVisFS :
                        _tokens->topVisFallbackFS;

    if (simpleImpl)
    {
        // If there is the texture, we will use texture sampling to get the pixel information.
        // Otherwise, we will use the calculated pixel information.
        FS[fsIndex++] = hasTexture ? _tokens->dashDotSampledPixelInfo : _tokens->dashDotCalculatedPixelInfo;
        FS[fsIndex++] = _tokens->noCapJointFragment;
    }
    else
    {
        // If there is the texture, we will use texture sampling to get the pixel information.
        // Otherwise, we will use the calculated pixel information.
        FS[fsIndex++] = hasTexture ? _tokens->dashDotSampledPixelInfo : _tokens->dashDotCalculatedPixelInfo;
        FS[fsIndex++] = _tokens->dashDotDefaultFragment;
    }
    FS[fsIndex] = TfToken();
}

HdSt_DashDotLinesShaderKey::~HdSt_DashDotLinesShaderKey()
{
}

PXR_NAMESPACE_CLOSE_SCOPE

