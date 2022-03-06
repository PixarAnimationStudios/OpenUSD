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

#include "pxr/imaging/hdSt/basisCurvesShaderKey.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/basisCurves.h"
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

namespace pxrImagingHdStBasisCurvesShaderKey {

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

    // visibility mixin (for curve and point visibility)
    ((topVisFallbackFS,                "Visibility.Fragment.Fallback"))
    ((topVisFS,                        "Visibility.Fragment.Topology"))

    // helper mixins
    ((curveCubicWidthsBasis,           "Curves.Cubic.Widths.Basis"))
    ((curveCubicWidthsLinear,          "Curves.Cubic.Widths.Linear"))
    ((curveCubicNormalsBasis,          "Curves.Cubic.Normals.Basis"))
    ((curveCubicNormalsLinear,         "Curves.Cubic.Normals.Linear"))
    ((curvesLinearVaryingInterp,       "Curves.Linear.VaryingInterpolation"))
    ((curvesCubicVaryingInterp,        "Curves.Cubic.VaryingInterpolation"))

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
    ((commonFS,                        "Fragment.CommonTerminals"))
    ((hullColorFS,                     "Fragment.HullColor"))
    ((pointColorFS,                    "Fragment.PointColor"))
    ((surfaceFS,                       "Fragment.Surface"))
    ((surfaceUnlitFS,                  "Fragment.SurfaceUnlit"))
    ((scalarOverrideFS,                "Fragment.ScalarOverride"))
);

} // pxrImagingHdStBasisCurvesShaderKey

static TfToken HdSt_BasisToShaderKey(const TfToken& basis){
    if (basis == HdTokens->bezier)
        return pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesBezier;
    else if (basis == HdTokens->catmullRom)
        return pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesCatmullRom;
    else if (basis == HdTokens->bSpline)
        return pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesBspline;
    TF_WARN("Unknown basis");
    return pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesFallback;
}

HdSt_BasisCurvesShaderKey::HdSt_BasisCurvesShaderKey(
    TfToken const &type, TfToken const &basis, 
    DrawStyle drawStyle, NormalStyle normalStyle,
    bool basisWidthInterpolation,
    bool basisNormalInterpolation,
    TfToken shadingTerminal,
    bool hasAuthoredTopologicalVisibility)
    : glslfx(pxrImagingHdStBasisCurvesShaderKey::_tokens->baseGLSLFX)
{
    bool drawThick = (drawStyle == HdSt_BasisCurvesShaderKey::HALFTUBE) || 
                     (drawStyle == HdSt_BasisCurvesShaderKey::RIBBON);
    bool cubic  = (type == HdTokens->cubic);
    bool linear = (type == HdTokens->linear);
    TF_VERIFY(cubic || linear);

    // The order of the clauses below matters!
    if (drawStyle == HdSt_BasisCurvesShaderKey::POINTS) {
        primType = HdSt_GeometricShader::PrimitiveType::PRIM_POINTS;
    } else if (cubic) {
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
    VS[vsIndex++]  = pxrImagingHdStBasisCurvesShaderKey::_tokens->instancing;
    VS[vsIndex++]  = drawThick ? pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesVertexPatch 
                       : pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesVertexWire;
    VS[vsIndex++]  = oriented ? pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesVertexNormalOriented 
                      : pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesVertexNormalImplicit;
    if (isPrimTypePoints) {
        // Add mixins that allow for picking and sel highlighting of points.
        // Even though these are more "render pass-ish", we do this here to
        // reduce the shader code generated when the points repr isn't used.
        VS[vsIndex++] = pxrImagingHdStBasisCurvesShaderKey::_tokens->pointIdVS;
        VS[vsIndex++] = pxrImagingHdStBasisCurvesShaderKey::_tokens->pointIdSelDecodeUtilsVS;
        VS[vsIndex++] = pxrImagingHdStBasisCurvesShaderKey::_tokens->pointIdSelPointSelVS;
    } else {
        VS[vsIndex++] = pxrImagingHdStBasisCurvesShaderKey::_tokens->pointIdNoneVS;
    }
    VS[vsIndex]  = TfToken();

    // Setup Tessellation
    if (linear) {
        switch(drawStyle) {
        case HdSt_BasisCurvesShaderKey::POINTS:
        case HdSt_BasisCurvesShaderKey::WIRE:
        {
            TCS[0] = TfToken();
            TES[0] = TfToken();
            break;
        }
        case HdSt_BasisCurvesShaderKey::RIBBON:
        {
            TCS[0] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessControlShared;
            TCS[1] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessControlLinearPatch; 
            TCS[2] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessControlLinearRibbon;
            TCS[3] = TfToken();  

            TES[0] = pxrImagingHdStBasisCurvesShaderKey::_tokens->instancing;
            TES[1] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessEvalPatch;
            TES[2] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesFallback;
            TES[3] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessEvalLinearPatch;
            TES[4] = oriented ? pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessEvalRibbonOriented
                            : pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessEvalRibbonImplicit;
            TES[5] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesLinearVaryingInterp;
            TES[6] = TfToken();
            break;
        }
        case HdSt_BasisCurvesShaderKey::HALFTUBE:
        {
            TCS[0] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessControlShared;
            TCS[1] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessControlLinearPatch; 
            TCS[2] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessControlLinearHalfTube;
            TCS[3] = TfToken();  

            TES[0] = pxrImagingHdStBasisCurvesShaderKey::_tokens->instancing;
            TES[1] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessEvalPatch;
            TES[2] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesFallback;
            TES[3] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessEvalLinearPatch;
            TES[4] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessEvalHalfTube;
            TES[5] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesLinearVaryingInterp;
            TES[6] = TfToken();
            break;
        }
        default:
            TF_CODING_ERROR("Unhandled drawstyle for basis curves");
        }
    } else { // cubic
        switch(drawStyle) {
        case HdSt_BasisCurvesShaderKey::POINTS:
        {
            TCS[0] = TfToken();
            TES[0] = TfToken();
            break;
        }
        case HdSt_BasisCurvesShaderKey::WIRE:
        {
            TCS[0] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessControlShared;
            TCS[1] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessControlCubicWire;
            TCS[2] = TfToken();

            TES[0] = pxrImagingHdStBasisCurvesShaderKey::_tokens->instancing;
            TES[1] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessEvalCubicWire;
            TES[2] = HdSt_BasisToShaderKey(basis);
            TES[3] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesCubicVaryingInterp;
            TES[4] = TfToken();
            break;
        }
        case HdSt_BasisCurvesShaderKey::RIBBON:
        {
            TCS[0] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessControlShared;
            TCS[1] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessControlCubicPatch;
            TCS[2] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessControlCubicRibbon;
            TCS[3] = TfToken();


            TES[0] = pxrImagingHdStBasisCurvesShaderKey::_tokens->instancing;
            TES[1] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessEvalPatch;
            TES[2] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessEvalCubicPatch;
            TES[3] = HdSt_BasisToShaderKey(basis);
            TES[4] = oriented ? pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessEvalRibbonOriented
                            : pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessEvalRibbonImplicit;
            TES[5] = basisWidthInterpolation ? 
                        pxrImagingHdStBasisCurvesShaderKey::_tokens->curveCubicWidthsBasis 
                    : pxrImagingHdStBasisCurvesShaderKey::_tokens->curveCubicWidthsLinear;
            TES[6] = basisNormalInterpolation ?
                        pxrImagingHdStBasisCurvesShaderKey::_tokens->curveCubicNormalsBasis :
                        pxrImagingHdStBasisCurvesShaderKey::_tokens->curveCubicNormalsLinear;
            TES[7] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesCubicVaryingInterp;
            TES[8] = TfToken();
            break;
        }
        case HdSt_BasisCurvesShaderKey::HALFTUBE:
        {
            TCS[0] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessControlShared;
            TCS[1] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessControlCubicPatch;
            TCS[2] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessControlCubicHalfTube;
            TCS[3] = TfToken();

            TES[0] = pxrImagingHdStBasisCurvesShaderKey::_tokens->instancing;
            TES[1] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessEvalPatch;
            TES[2] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessEvalCubicPatch;
            TES[3] = HdSt_BasisToShaderKey(basis);
            TES[4] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesTessEvalHalfTube;
            TES[5] = basisWidthInterpolation ? 
                        pxrImagingHdStBasisCurvesShaderKey::_tokens->curveCubicWidthsBasis 
                    : pxrImagingHdStBasisCurvesShaderKey::_tokens->curveCubicWidthsLinear;
            TES[6] = basisNormalInterpolation ?
                        pxrImagingHdStBasisCurvesShaderKey::_tokens->curveCubicNormalsBasis :
                        pxrImagingHdStBasisCurvesShaderKey::_tokens->curveCubicNormalsLinear;
            TES[7] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesCubicVaryingInterp;
            TES[8] = TfToken();
            break;
        }
        default:
            TF_CODING_ERROR("Unhandled drawstyle for basis curves");
        }
    }

    // setup fragment shaders
    // Common must be first as it defines terminal interfaces
    uint8_t fsIndex = 0;
    FS[fsIndex++] = pxrImagingHdStBasisCurvesShaderKey::_tokens->commonFS;
    if (shadingTerminal == HdBasisCurvesReprDescTokens->hullColor) {
        FS[fsIndex++] = pxrImagingHdStBasisCurvesShaderKey::_tokens->hullColorFS;
    } else if (shadingTerminal == HdBasisCurvesReprDescTokens->pointColor) {
        FS[fsIndex++] = pxrImagingHdStBasisCurvesShaderKey::_tokens->pointColorFS;
    } else if (shadingTerminal ==
               HdBasisCurvesReprDescTokens->surfaceShaderUnlit) {
        FS[fsIndex++] = pxrImagingHdStBasisCurvesShaderKey::_tokens->surfaceUnlitFS;
    } else {
        FS[fsIndex++] = pxrImagingHdStBasisCurvesShaderKey::_tokens->surfaceFS;
    }
    FS[fsIndex++] = pxrImagingHdStBasisCurvesShaderKey::_tokens->scalarOverrideFS;

    FS[fsIndex++] = isPrimTypePoints?
                        pxrImagingHdStBasisCurvesShaderKey::_tokens->pointIdFS : pxrImagingHdStBasisCurvesShaderKey::_tokens->pointIdFallbackFS;
    
    FS[fsIndex++] = hasAuthoredTopologicalVisibility? pxrImagingHdStBasisCurvesShaderKey::_tokens->topVisFS :
                                                      pxrImagingHdStBasisCurvesShaderKey::_tokens->topVisFallbackFS;


    if (drawStyle == HdSt_BasisCurvesShaderKey::WIRE || 
        drawStyle == HdSt_BasisCurvesShaderKey::POINTS) {
        FS[fsIndex++] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesFragmentWire;
        FS[fsIndex++] = TfToken();
    }
    else if (drawStyle == HdSt_BasisCurvesShaderKey::RIBBON &&
             normalStyle == HdSt_BasisCurvesShaderKey::ORIENTED){
        FS[fsIndex++] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesFragmentPatch;
        FS[fsIndex++] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesFragmentRibbonOriented;
        FS[fsIndex++] = TfToken();
    }
    else if (drawStyle == HdSt_BasisCurvesShaderKey::RIBBON &&
             normalStyle == HdSt_BasisCurvesShaderKey::ROUND){
        FS[fsIndex++] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesFragmentPatch;
        FS[fsIndex++] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesFragmentRibbonRound;
        FS[fsIndex++] = TfToken();
    }
    else if (drawStyle == HdSt_BasisCurvesShaderKey::RIBBON &&
             normalStyle == HdSt_BasisCurvesShaderKey::HAIR){
        FS[fsIndex++] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesFragmentPatch;
        FS[fsIndex++] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesFragmentHair;
        FS[fsIndex++] = TfToken();
    }
    else if (drawStyle == HdSt_BasisCurvesShaderKey::HALFTUBE &&
             normalStyle == HdSt_BasisCurvesShaderKey::ROUND){
        FS[fsIndex++] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesFragmentPatch;
        FS[fsIndex++] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesFragmentHalfTube;
        FS[fsIndex++] = TfToken();
    }
    else if (drawStyle == HdSt_BasisCurvesShaderKey::HALFTUBE &&
             normalStyle == HdSt_BasisCurvesShaderKey::HAIR){
        FS[fsIndex++] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesFragmentPatch;
        FS[fsIndex++] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesFragmentHair;
        FS[fsIndex++] = TfToken();
    }
    else{
        TF_WARN("Cannot setup fragment shaders for invalid combination of \
                 basis curves shader key settings.");
        FS[fsIndex++] = pxrImagingHdStBasisCurvesShaderKey::_tokens->curvesFragmentHair;
        FS[fsIndex++] = TfToken();
    }
}

HdSt_BasisCurvesShaderKey::~HdSt_BasisCurvesShaderKey()
{
}

PXR_NAMESPACE_CLOSE_SCOPE

