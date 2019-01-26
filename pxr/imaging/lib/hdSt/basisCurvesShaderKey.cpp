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
#include "pxr/pxr.h"
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hdSt/basisCurvesShaderKey.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfEnum) {
     // Register the names for the values:
     TF_ADD_ENUM_NAME(HdSt_BasisCurvesShaderKey::WIRE);
     TF_ADD_ENUM_NAME(HdSt_BasisCurvesShaderKey::RIBBON);
     TF_ADD_ENUM_NAME(HdSt_BasisCurvesShaderKey::HALFTUBE);
};

TF_REGISTRY_FUNCTION(TfEnum) {
     // Register the names for the values:
     TF_ADD_ENUM_NAME(HdSt_BasisCurvesShaderKey::ORIENTED);
     TF_ADD_ENUM_NAME(HdSt_BasisCurvesShaderKey::HAIR);
     TF_ADD_ENUM_NAME(HdSt_BasisCurvesShaderKey::ROUND);
};

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((baseGLSLFX,                      "basisCurves.glslfx"))

    // normal related mixins
    ((curvesVertexNormalOriented,      "Curves.Vertex.Normal.Oriented"))
    ((curvesVertexNormalImplicit,      "Curves.Vertex.Normal.Implicit"))

    // basis mixins
    ((curvesBezier,                    "Curves.BezierBasis"))
    ((curvesBspline,                   "Curves.BsplineBasis"))
    ((curvesCatmullRom,                "Curves.CatmullRomBasis"))
    ((curvesFallback,                  "Curves.LinearBasis"))

    // point id mixins (for point picking & selection)
    ((pointIdNoneVS,                   "PointId.Vertex.None"))
    ((pointIdVS,                       "PointId.Vertex.PointParam"))
    ((pointIdSelDecodeUtilsVS,         "Selection.DecodeUtils"))
    ((pointIdSelPointSelVS,            "Selection.Vertex.PointSel"))
    ((pointIdFallbackFS,               "PointId.Fragment.Fallback"))
    ((pointIdFS,                       "PointId.Fragment.PointParam"))

    // helper mixins
    ((curveCubicWidthsBasis,           "Curves.Cubic.Widths.Basis"))
    ((curveCubicWidthsLinear,          "Curves.Cubic.Widths.Linear"))
    ((curveCubicNormalsBasis,          "Curves.Cubic.Normals.Basis"))
    ((curveCubicNormalsLinear,         "Curves.Cubic.Normals.Linear"))

    ((curvesTessControlShared,         "Curves.TessControl.Shared"))
    ((curvesTessControlLinearRibbon,   "Curves.TessControl.Linear.Ribbon"))
    ((curvesTessControlLinearHalfTube, "Curves.TessControl.Linear.HalfTube"))
    ((curvesTessControlCubicRibbon,    "Curves.TessControl.Cubic.Ribbon"))
    ((curvesTessControlCubicHalfTube,  "Curves.TessControl.Cubic.HalfTube"))
    ((curvesTessEvalLinearPatch,       "Curves.TessEval.Linear.Patch"))
    ((curvesTessEvalCubicWire,         "Curves.TessEval.Cubic.Wire"))
    ((curvesTessEvalCubicPatch,        "Curves.TessEval.Cubic.Patch"))
    ((curvesTessEvalRibbonImplicit,    "Curves.TessEval.Ribbon.Implicit"))
    ((curvesTessEvalRibbonOriented,    "Curves.TessEval.Ribbon.Oriented"))
    ((curvesTessEvalHalfTube,          "Curves.TessEval.HalfTube"))

    ((curvesFragmentHalfTube,          "Curves.Fragment.HalfTube"))
    ((curvesFragmentRibbonRound,       "Curves.Fragment.Ribbon.Round"))
    ((curvesFragmentRibbonOriented,    "Curves.Fragment.Ribbon.Oriented"))
    ((curvesFragmentHair,              "Curves.Fragment.Hair"))

    // main for all the shader stages
    ((curvesVertexPatch,               "Curves.Vertex.Patch"))
    ((curvesVertexWire,                "Curves.Vertex.Wire"))
    ((curvesTessControlLinearPatch,    "Curves.TessControl.Linear.Patch"))
    ((curvesTessControlCubicWire,      "Curves.TessControl.Cubic.Wire"))
    ((curvesTessControlCubicPatch,     "Curves.TessControl.Cubic.Patch"))
    ((curvesTessEvalPatch,             "Curves.TessEval.Patch"))
    ((curvesFragmentWire,              "Curves.Fragment.Wire"))
    ((curvesFragmentPatch,             "Curves.Fragment.Patch"))

    // instancing related mixins
    ((instancing,                      "Instancing.Transform"))

    // terminals
    ((surfaceFS,                       "Fragment.Surface"))
    ((commonFS,                        "Fragment.CommonTerminals"))
    ((scalarOverrideFS,                "Fragment.ScalarOverride"))
);

static TfToken HdSt_BasisToShaderKey(const TfToken& basis){
    if (basis == HdTokens->bezier)
        return _tokens->curvesBezier;
    else if (basis == HdTokens->catmullRom)
        return _tokens->curvesCatmullRom;
    else if (basis == HdTokens->bSpline)
        return _tokens->curvesBspline;
    TF_WARN("Unknown basis");
    return _tokens->curvesFallback;
}

HdSt_BasisCurvesShaderKey::HdSt_BasisCurvesShaderKey(
    TfToken const &type, TfToken const &basis, 
    DrawStyle drawStyle, NormalStyle normalStyle,
    bool basisWidthInterpolation,
    bool basisNormalInterpolation)
    : glslfx(_tokens->baseGLSLFX)
{
    bool drawThick = (drawStyle == HdSt_BasisCurvesShaderKey::HALFTUBE) || 
                     (drawStyle == HdSt_BasisCurvesShaderKey::RIBBON);
    bool cubic  = (type == HdTokens->cubic);
    bool linear = (type == HdTokens->linear);
    TF_VERIFY(cubic || linear);

    if (cubic) {
        // cubic curves get drawn via isolines in a tessellation shader
        // even in wire mode.
        primType =
        HdSt_GeometricShader::PrimitiveType::PRIM_BASIS_CURVES_CUBIC_PATCHES;
    } else if (drawThick){
        primType =
        HdSt_GeometricShader::PrimitiveType::PRIM_BASIS_CURVES_LINEAR_PATCHES;
    } else {
        primType = HdSt_GeometricShader::PrimitiveType::PRIM_BASIS_CURVES_LINES;
    }

    bool isPrimTypePoints = HdSt_GeometricShader::IsPrimTypePoints(primType);

    bool oriented = normalStyle == HdSt_BasisCurvesShaderKey::ORIENTED;

    uint8_t vsIndex = 0;
    VS[vsIndex++]  = _tokens->instancing;
    VS[vsIndex++]  = drawThick ? _tokens->curvesVertexPatch 
                       : _tokens->curvesVertexWire;
    VS[vsIndex++]  = oriented ? _tokens->curvesVertexNormalOriented 
                      : _tokens->curvesVertexNormalImplicit;
    if (isPrimTypePoints) {
        // Add mixins that allow for picking and sel highlighting of points.
        // Even though these are more "render pass-ish", we do this here to
        // reduce the shader code generated when the points repr isn't used.
        VS[vsIndex++] = _tokens->pointIdVS;
        VS[vsIndex++] = _tokens->pointIdSelDecodeUtilsVS;
        VS[vsIndex++] = _tokens->pointIdSelPointSelVS;
    } else {
        VS[vsIndex++] = _tokens->pointIdNoneVS;
    }
    VS[vsIndex]  = TfToken();

    // Setup Tessellation
    if (linear) {
        switch(drawStyle) {
        case HdSt_BasisCurvesShaderKey::WIRE:
        {
            TCS[0] = TfToken();
            TES[0] = TfToken();
            break;
        }
        case HdSt_BasisCurvesShaderKey::RIBBON:
        {
            TCS[0] = _tokens->curvesTessControlShared;
            TCS[1] = _tokens->curvesTessControlLinearPatch; 
            TCS[2] = _tokens->curvesTessControlLinearRibbon;
            TCS[3] = TfToken();  

            TES[0] = _tokens->instancing;
            TES[1] = _tokens->curvesTessEvalPatch;
            TES[2] = _tokens->curvesTessEvalLinearPatch;
            TES[3] = oriented ? _tokens->curvesTessEvalRibbonOriented
                            : _tokens->curvesTessEvalRibbonImplicit;
            TES[4] = TfToken();
            break;
        }
        case HdSt_BasisCurvesShaderKey::HALFTUBE:
        {
            TCS[0] = _tokens->curvesTessControlShared;
            TCS[1] = _tokens->curvesTessControlLinearPatch; 
            TCS[2] = _tokens->curvesTessControlLinearHalfTube;
            TCS[3] = TfToken();  

            TES[0] = _tokens->instancing;
            TES[1] = _tokens->curvesTessEvalPatch;
            TES[2] = _tokens->curvesTessEvalLinearPatch;
            TES[3] = _tokens->curvesTessEvalHalfTube;
            TES[4] = TfToken();
            break;
        }
        default:
            TF_CODING_ERROR("Unhandled drawstyle for basis curves");
        }
    } else { // cubic
        switch(drawStyle) {
        case HdSt_BasisCurvesShaderKey::WIRE:
        {
            TCS[0] = _tokens->curvesTessControlShared;
            TCS[1] = _tokens->curvesTessControlCubicWire;
            TCS[2] = TfToken();

            TES[0] = _tokens->instancing;
            TES[1] = _tokens->curvesTessEvalCubicWire;
            TES[2] = HdSt_BasisToShaderKey(basis);
            TES[3] = TfToken();
            break;
        }
        case HdSt_BasisCurvesShaderKey::RIBBON:
        {
            TCS[0] = _tokens->curvesTessControlShared;
            TCS[1] = _tokens->curvesTessControlCubicPatch;
            TCS[2] = _tokens->curvesTessControlCubicRibbon;
            TCS[3] = TfToken();


            TES[0] = _tokens->instancing;
            TES[1] = _tokens->curvesTessEvalPatch;
            TES[2] = _tokens->curvesTessEvalCubicPatch;
            TES[3] = HdSt_BasisToShaderKey(basis);
            TES[4] = oriented ? _tokens->curvesTessEvalRibbonOriented
                            : _tokens->curvesTessEvalRibbonImplicit;
            TES[5] = basisWidthInterpolation ? 
                        _tokens->curveCubicWidthsBasis 
                    : _tokens->curveCubicWidthsLinear;
            TES[6] = basisNormalInterpolation ?
                        _tokens->curveCubicNormalsBasis :
                        _tokens->curveCubicNormalsLinear;
            TES[7] = TfToken();
            break;
        }
        case HdSt_BasisCurvesShaderKey::HALFTUBE:
        {
            TCS[0] = _tokens->curvesTessControlShared;
            TCS[1] = _tokens->curvesTessControlCubicPatch;
            TCS[2] = _tokens->curvesTessControlCubicHalfTube;
            TCS[3] = TfToken();

            TES[0] = _tokens->instancing;
            TES[1] = _tokens->curvesTessEvalPatch;
            TES[2] = _tokens->curvesTessEvalCubicPatch;
            TES[3] = HdSt_BasisToShaderKey(basis);
            TES[4] = _tokens->curvesTessEvalHalfTube;
            TES[5] = basisWidthInterpolation ? 
                        _tokens->curveCubicWidthsBasis 
                    : _tokens->curveCubicWidthsLinear;
            TES[6] = basisNormalInterpolation ?
                        _tokens->curveCubicNormalsBasis :
                        _tokens->curveCubicNormalsLinear;
            TES[7] = TfToken();
            break;
        }
        default:
            TF_CODING_ERROR("Unhandled drawstyle for basis curves");
        }
    }

    // setup fragment shaders
    // Common must be first as it defines terminal interfaces
    uint8_t fsIndex = 0;
    FS[fsIndex++] = _tokens->commonFS;
    FS[fsIndex++] = _tokens->surfaceFS;
    FS[fsIndex++] = _tokens->scalarOverrideFS;

    // we don't currently ever set primType to PRIM_POINTS for curves, but
    // if we ever want to view them as just points, this allows point picking to
    // work.
    FS[fsIndex++] = isPrimTypePoints?
                        _tokens->pointIdFS : _tokens->pointIdFallbackFS;

    if (drawStyle == HdSt_BasisCurvesShaderKey::WIRE){
        FS[fsIndex++] = _tokens->curvesFragmentWire;
        FS[fsIndex++] = TfToken();
    }
    else if (drawStyle == HdSt_BasisCurvesShaderKey::RIBBON &&
             normalStyle == HdSt_BasisCurvesShaderKey::ORIENTED){
        FS[fsIndex++] = _tokens->curvesFragmentPatch;
        FS[fsIndex++] = _tokens->curvesFragmentRibbonOriented;
        FS[fsIndex++] = TfToken();
    }
    else if (drawStyle == HdSt_BasisCurvesShaderKey::RIBBON &&
             normalStyle == HdSt_BasisCurvesShaderKey::ROUND){
        FS[fsIndex++] = _tokens->curvesFragmentPatch;
        FS[fsIndex++] = _tokens->curvesFragmentRibbonRound;
        FS[fsIndex++] = TfToken();
    }
    else if (drawStyle == HdSt_BasisCurvesShaderKey::RIBBON &&
             normalStyle == HdSt_BasisCurvesShaderKey::HAIR){
        FS[fsIndex++] = _tokens->curvesFragmentPatch;
        FS[fsIndex++] = _tokens->curvesFragmentHair;
        FS[fsIndex++] = TfToken();
    }
    else if (drawStyle == HdSt_BasisCurvesShaderKey::HALFTUBE &&
             normalStyle == HdSt_BasisCurvesShaderKey::ROUND){
        FS[fsIndex++] = _tokens->curvesFragmentPatch;
        FS[fsIndex++] = _tokens->curvesFragmentHalfTube;
        FS[fsIndex++] = TfToken();
    }
    else if (drawStyle == HdSt_BasisCurvesShaderKey::HALFTUBE &&
             normalStyle == HdSt_BasisCurvesShaderKey::HAIR){
        FS[fsIndex++] = _tokens->curvesFragmentPatch;
        FS[fsIndex++] = _tokens->curvesFragmentHair;
        FS[fsIndex++] = TfToken();
    }
    else{
        TF_WARN("Cannot setup fragment shaders for invalid combination of \
                 basis curves shader key settings.");
        FS[fsIndex++] = _tokens->curvesFragmentHair;
        FS[fsIndex++] = TfToken();
    }
}

HdSt_BasisCurvesShaderKey::~HdSt_BasisCurvesShaderKey()
{
}

PXR_NAMESPACE_CLOSE_SCOPE

