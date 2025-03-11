//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((baseGLSLFX,                      "basisCurves.glslfx"))

    // curve data
    ((curvesCommonData,                "Curves.CommonData"))
    ((curvesPostTessCurveData,         "Curves.PostTess.CurveData"))
    ((curvesTessCurveDataPatch,        "Curves.Tess.CurveData.Patch"))
    ((curvesTessCurveDataWire,         "Curves.Tess.CurveData.Wire"))

    // tess factors
    ((curvesTessFactorsGLSL,           "Curves.TessFactorsGLSL"))
    ((curvesTessFactorsMSL,            "Curves.TessFactorsMSL"))

    // normal related mixins
    ((curvesVertexNormalOriented,      "Curves.Vertex.Normal.Oriented"))
    ((curvesVertexNormalImplicit,      "Curves.Vertex.Normal.Implicit"))
    ((curvesPostTessNormalOriented,    "Curves.PostTess.Normal.Oriented"))
    ((curvesPostTessNormalImplicit,    "Curves.PostTess.Normal.Implicit"))

    // basis mixins
    ((curvesCoeffs,                    "Curves.Coeffs"))
    ((curvesBezier,                    "Curves.BezierBasis"))
    ((curvesBspline,                   "Curves.BsplineBasis"))
    ((curvesCatmullRom,                "Curves.CatmullRomBasis"))
    ((curvesCentripetalCatmullRom,     "Curves.CentripetalCatmullRomBasis"))
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

    ((curvesCommonControl,             "Curves.CommonControl"))
    ((curvesCommonControlLinearRibbon, "Curves.CommonControl.Linear.Ribbon"))
    ((curvesCommonControlLinearHalfTube,"Curves.CommonControl.Linear.HalfTube"))
    ((curvesCommonControlCubicRibbon,  "Curves.CommonControl.Cubic.Ribbon"))
    ((curvesCommonControlCubicHalfTube,"Curves.CommonControl.Cubic.HalfTube"))

    ((curvesCommonEvalLinearPatch,     "Curves.CommonEval.Linear.Patch"))
    ((curvesCommonEvalCubicPatch,      "Curves.CommonEval.Cubic.Patch"))
    ((curvesCommonEvalRibbonImplicit,  "Curves.CommonEval.Ribbon.Implicit"))
    ((curvesCommonEvalRibbonOriented,  "Curves.CommonEval.Ribbon.Oriented"))
    ((curvesCommonEvalHalfTube,        "Curves.CommonEval.HalfTube"))

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
    ((curvesTessEvalCubicWire,         "Curves.TessEval.Cubic.Wire"))

    ((curvesPostTessControlLinearPatch,"Curves.PostTessControl.Linear.Patch"))
    ((curvesPostTessControlCubicWire,  "Curves.PostTessControl.Cubic.Wire"))
    ((curvesPostTessControlCubicPatch, "Curves.PostTessControl.Cubic.Patch"))

    ((curvesPostTessVertexPatch,       "Curves.PostTessVertex.Patch"))
    ((curvesPostTessVertexWire,        "Curves.PostTessVertex.Wire"))
    ((curvesPostTessVertexCubicWire,   "Curves.PostTessVertex.Cubic.Wire"))

    ((curvesCommonEvalPatch,           "Curves.CommonEval.Patch"))

    ((curvesFragmentWire,              "Curves.Fragment.Wire"))
    ((curvesFragmentPatch,             "Curves.Fragment.Patch"))

    // instancing related mixins
    ((instancing,                      "Instancing.Transform"))

    // terminals
    ((commonFS,                        "Fragment.CommonTerminals"))
    ((hullColorFS,                     "Fragment.HullColor"))
    ((pointColorFS,                    "Fragment.PointColor"))
    ((pointShadedFS,                   "Fragment.PointShaded"))
    ((surfaceFS,                       "Fragment.Surface"))
    ((surfaceUnlitFS,                  "Fragment.SurfaceUnlit"))
    ((scalarOverrideFS,                "Fragment.ScalarOverride"))

    // rounded points
    ((pointSizeBiasVS,                 "PointDisk.Vertex.PointSizeBias"))
    ((noPointSizeBiasVS,               "PointDisk.Vertex.None"))
    ((diskSampleMaskFS,                "PointDisk.Fragment.SampleMask"))
    ((noDiskSampleMaskFS,              "PointDisk.Fragment.None"))
);

static TfToken HdSt_BasisToShaderKey(const TfToken& basis){
    if (basis == HdTokens->bezier)
        return _tokens->curvesBezier;
    else if (basis == HdTokens->catmullRom)
        return _tokens->curvesCatmullRom;
    else if (basis == HdTokens->bspline)
        return _tokens->curvesBspline;
    else if (basis == HdTokens->centripetalCatmullRom)
        return _tokens->curvesCentripetalCatmullRom;
    TF_WARN("Unknown basis");
    return _tokens->curvesFallback;
}

HdSt_BasisCurvesShaderKey::HdSt_BasisCurvesShaderKey(
    TfToken const &type, TfToken const &basis, 
    DrawStyle drawStyle, NormalStyle normalStyle,
    bool basisWidthInterpolation,
    bool basisNormalInterpolation,
    TfToken shadingTerminal,
    bool hasAuthoredTopologicalVisibility,
    bool pointsShadingEnabled,
    bool hasMetalTessellation,
    bool nativeRoundPoints)
    : useMetalTessellation(false)
    , glslfx(_tokens->baseGLSLFX)
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

    // skip Metal tessellation for linear points and wire curves.
    bool const skipTessLinear =
            linear && (drawStyle == HdSt_BasisCurvesShaderKey::POINTS ||
                       drawStyle == HdSt_BasisCurvesShaderKey::WIRE);

    useMetalTessellation = hasMetalTessellation && !(skipTessLinear);

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
        VS[vsIndex++] = nativeRoundPoints ? _tokens->noPointSizeBiasVS :
            _tokens->pointSizeBiasVS;
    } else {
        VS[vsIndex++] = _tokens->pointIdNoneVS;
        VS[vsIndex++] =  _tokens->noPointSizeBiasVS;
    }
    VS[vsIndex]  = TfToken();

    // Setup Tessellation
    uint8_t tcsIndex = 0;
    uint8_t tesIndex = 0;
    uint8_t ptcsIndex = 0;
    uint8_t ptvsIndex = 0;

    TfToken const ribbonToken =
        oriented
            ? _tokens->curvesCommonEvalRibbonOriented
            : _tokens->curvesCommonEvalRibbonImplicit;

    TfToken const basisWidthsInterpolationToken =
        basisWidthInterpolation
            ? _tokens->curveCubicWidthsBasis
            : _tokens->curveCubicWidthsLinear;

    TfToken const basisNormalInterpolationToken =
        basisNormalInterpolation
            ? _tokens->curveCubicNormalsBasis
            : _tokens->curveCubicNormalsLinear;

    TfToken const postTessNormal =
        oriented
            ? _tokens->curvesPostTessNormalOriented
            : _tokens->curvesPostTessNormalImplicit;

    TCS[tcsIndex++] = _tokens->curvesCommonData;

    TES[tesIndex++] = _tokens->curvesCommonData;

    PTCS[ptcsIndex++] = _tokens->curvesCommonData;
    PTCS[ptcsIndex++] = _tokens->curvesPostTessCurveData;
    PTCS[ptcsIndex++] = _tokens->curvesTessFactorsMSL;
    PTCS[ptcsIndex++] = postTessNormal;

    PTVS[ptvsIndex++] = _tokens->curvesCommonData;
    PTVS[ptvsIndex++] = _tokens->curvesPostTessCurveData;
    PTVS[ptvsIndex++] = postTessNormal;
    PTVS[ptvsIndex++] = _tokens->pointIdNoneVS;

    if (linear) {
        switch(drawStyle) {
        case HdSt_BasisCurvesShaderKey::POINTS:
        case HdSt_BasisCurvesShaderKey::WIRE:
        {
            TCS[0] = TfToken();
            TES[0] = TfToken();

            PTCS[0] = TfToken();
            PTVS[0] = TfToken();
            break;
        }
        case HdSt_BasisCurvesShaderKey::RIBBON:
        {
            TCS[tcsIndex++] = _tokens->curvesTessFactorsGLSL;
            TCS[tcsIndex++] = _tokens->curvesCommonControl;
            TCS[tcsIndex++] = _tokens->curvesTessCurveDataPatch;
            TCS[tcsIndex++] = _tokens->curvesTessControlLinearPatch;
            TCS[tcsIndex++] = _tokens->curvesCommonControlLinearRibbon;
            TCS[tcsIndex++] = TfToken();

            TES[tesIndex++] = _tokens->instancing;
            TES[tesIndex++] = _tokens->curvesTessCurveDataPatch;
            TES[tesIndex++] = _tokens->curvesTessEvalPatch;
            TES[tesIndex++] = _tokens->curvesCommonEvalPatch;
            TES[tesIndex++] = _tokens->curvesFallback;
            TES[tesIndex++] = _tokens->curvesCommonEvalLinearPatch;
            TES[tesIndex++] = ribbonToken;
            TES[tesIndex++] = _tokens->curvesLinearVaryingInterp;
            TES[tesIndex++] = TfToken();

            PTCS[ptcsIndex++] = _tokens->instancing;
            PTCS[ptcsIndex++] = _tokens->curvesCommonControl;
            PTCS[ptcsIndex++] = _tokens->curvesPostTessControlLinearPatch;
            PTCS[ptcsIndex++] = _tokens->curvesCommonControlLinearRibbon;
            PTCS[ptcsIndex++] = TfToken();

            PTVS[ptvsIndex++] = _tokens->instancing;
            PTVS[ptvsIndex++] = _tokens->curvesPostTessVertexPatch;
            PTVS[ptvsIndex++] = _tokens->curvesCommonEvalPatch;
            PTVS[ptvsIndex++] = _tokens->curvesFallback;
            PTVS[ptvsIndex++] = _tokens->curvesCommonEvalLinearPatch;
            PTVS[ptvsIndex++] = ribbonToken;
            PTVS[ptvsIndex++] = _tokens->curvesLinearVaryingInterp;
            PTVS[ptvsIndex++] = TfToken();
            break;
        }
        case HdSt_BasisCurvesShaderKey::HALFTUBE:
        {
            TCS[tcsIndex++] = _tokens->curvesTessFactorsGLSL;
            TCS[tcsIndex++] = _tokens->curvesCommonControl;
            TCS[tcsIndex++] = _tokens->curvesTessCurveDataPatch;
            TCS[tcsIndex++] = _tokens->curvesTessControlLinearPatch;
            TCS[tcsIndex++] = _tokens->curvesCommonControlLinearHalfTube;
            TCS[tcsIndex++] = TfToken();

            TES[tesIndex++] = _tokens->instancing;
            TES[tesIndex++] = _tokens->curvesTessCurveDataPatch;
            TES[tesIndex++] = _tokens->curvesTessEvalPatch;
            TES[tesIndex++] = _tokens->curvesCommonEvalPatch;
            TES[tesIndex++] = _tokens->curvesFallback;
            TES[tesIndex++] = _tokens->curvesCommonEvalLinearPatch;
            TES[tesIndex++] = _tokens->curvesCommonEvalHalfTube;
            TES[tesIndex++] = _tokens->curvesLinearVaryingInterp;
            TES[tesIndex++] = TfToken();

            PTCS[ptcsIndex++] = _tokens->instancing;
            PTCS[ptcsIndex++] = _tokens->curvesCommonControl;
            PTCS[ptcsIndex++] = _tokens->curvesPostTessControlLinearPatch;
            PTCS[ptcsIndex++] = _tokens->curvesCommonControlLinearHalfTube;
            PTCS[ptcsIndex++] = TfToken();

            PTVS[ptvsIndex++] = _tokens->instancing;
            PTVS[ptvsIndex++] = _tokens->curvesPostTessVertexPatch;
            PTVS[ptvsIndex++] = _tokens->curvesCommonEvalPatch;
            PTVS[ptvsIndex++] = _tokens->curvesFallback;
            PTVS[ptvsIndex++] = _tokens->curvesCommonEvalLinearPatch;
            PTVS[ptvsIndex++] = _tokens->curvesCommonEvalHalfTube;
            PTVS[ptvsIndex++] = _tokens->curvesLinearVaryingInterp;
            PTVS[ptvsIndex++] = TfToken();
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

            PTCS[0] = TfToken();
            PTVS[0] = TfToken();
            break;
        }
        case HdSt_BasisCurvesShaderKey::WIRE:
        {
            TCS[tcsIndex++] = _tokens->curvesTessFactorsGLSL;
            TCS[tcsIndex++] = _tokens->curvesCommonControl;
            TCS[tcsIndex++] = _tokens->curvesTessCurveDataWire;
            TCS[tcsIndex++] = _tokens->curvesTessControlCubicWire;
            TCS[tcsIndex++] = TfToken();

            TES[tesIndex++] = _tokens->instancing;
            TES[tesIndex++] = _tokens->curvesTessCurveDataWire;
            TES[tesIndex++] = _tokens->curvesTessEvalCubicWire;
            TES[tesIndex++] = HdSt_BasisToShaderKey(basis);
            TES[tesIndex++] = _tokens->curvesCubicVaryingInterp;
            TES[tesIndex++] = TfToken();

            PTCS[ptcsIndex++] = _tokens->instancing;
            PTCS[ptcsIndex++] = _tokens->curvesCommonControl;
            PTCS[ptcsIndex++] = _tokens->curvesPostTessControlCubicWire;
            PTCS[ptcsIndex++] = TfToken();

            PTVS[ptvsIndex++] = _tokens->instancing;
            PTVS[ptvsIndex++] = _tokens->curvesPostTessVertexCubicWire;
            PTVS[ptvsIndex++] = HdSt_BasisToShaderKey(basis);
            PTVS[ptvsIndex++] = _tokens->curvesCubicVaryingInterp;
            PTVS[ptvsIndex++] = TfToken();
            break;
        }
        case HdSt_BasisCurvesShaderKey::RIBBON:
        {
            TCS[tcsIndex++] = _tokens->curvesTessFactorsGLSL;
            TCS[tcsIndex++] = _tokens->curvesCommonControl;
            TCS[tcsIndex++] = _tokens->curvesTessCurveDataPatch;
            TCS[tcsIndex++] = _tokens->curvesTessControlCubicPatch;
            TCS[tcsIndex++] = _tokens->curvesCommonControlCubicRibbon;
            TCS[tcsIndex++] = TfToken();

            TES[tesIndex++] = _tokens->instancing;
            TES[tesIndex++] = _tokens->curvesTessCurveDataPatch;
            TES[tesIndex++] = _tokens->curvesTessEvalPatch;
            TES[tesIndex++] = _tokens->curvesCommonEvalPatch;
            TES[tesIndex++] = _tokens->curvesCommonEvalCubicPatch;
            TES[tesIndex++] = HdSt_BasisToShaderKey(basis);
            TES[tesIndex++] = ribbonToken;
            TES[tesIndex++] = basisWidthsInterpolationToken;
            TES[tesIndex++] = basisNormalInterpolationToken;
            TES[tesIndex++] = _tokens->curvesCubicVaryingInterp;
            TES[tesIndex++] = TfToken();

            PTCS[ptcsIndex++] = _tokens->instancing;
            PTCS[ptcsIndex++] = _tokens->curvesCommonControl;
            PTCS[ptcsIndex++] = _tokens->curvesPostTessControlCubicPatch;
            PTCS[ptcsIndex++] = _tokens->curvesCommonControlCubicRibbon;
            PTCS[ptcsIndex++] = TfToken();

            PTVS[ptvsIndex++] = _tokens->instancing;
            PTVS[ptvsIndex++] = _tokens->curvesPostTessVertexPatch;
            PTVS[ptvsIndex++] = _tokens->curvesCommonEvalPatch;
            PTVS[ptvsIndex++] = _tokens->curvesCommonEvalCubicPatch;
            PTVS[ptvsIndex++] = HdSt_BasisToShaderKey(basis);
            PTVS[ptvsIndex++] = ribbonToken;
            PTVS[ptvsIndex++] = basisWidthsInterpolationToken;
            PTVS[ptvsIndex++] = basisNormalInterpolationToken;
            PTVS[ptvsIndex++] = _tokens->curvesCubicVaryingInterp;
            PTVS[ptvsIndex++] = TfToken();
            break;
        }
        case HdSt_BasisCurvesShaderKey::HALFTUBE:
        {
            TCS[tcsIndex++] = _tokens->curvesTessFactorsGLSL;
            TCS[tcsIndex++] = _tokens->curvesCommonControl;
            TCS[tcsIndex++] = _tokens->curvesTessCurveDataPatch;
            TCS[tcsIndex++] = _tokens->curvesTessControlCubicPatch;
            TCS[tcsIndex++] = _tokens->curvesCommonControlCubicHalfTube;
            TCS[tcsIndex++] = TfToken();

            TES[tesIndex++] = _tokens->instancing;
            TES[tesIndex++] = _tokens->curvesTessCurveDataPatch;
            TES[tesIndex++] = _tokens->curvesTessEvalPatch;
            TES[tesIndex++] = _tokens->curvesCommonEvalPatch;
            TES[tesIndex++] = _tokens->curvesCommonEvalCubicPatch;
            TES[tesIndex++] = HdSt_BasisToShaderKey(basis);
            TES[tesIndex++] = _tokens->curvesCommonEvalHalfTube;
            TES[tesIndex++] = basisWidthsInterpolationToken;
            TES[tesIndex++] = basisNormalInterpolationToken;
            TES[tesIndex++] = _tokens->curvesCubicVaryingInterp;
            TES[tesIndex++] = TfToken();

            PTCS[ptcsIndex++] = _tokens->instancing;
            PTCS[ptcsIndex++] = _tokens->curvesCommonControl;
            PTCS[ptcsIndex++] = _tokens->curvesPostTessControlCubicPatch;
            PTCS[ptcsIndex++] = _tokens->curvesCommonControlCubicHalfTube;
            PTCS[ptcsIndex++] = TfToken();

            PTVS[ptvsIndex++] = _tokens->instancing;
            PTVS[ptvsIndex++] = _tokens->curvesPostTessVertexPatch;
            PTVS[ptvsIndex++] = _tokens->curvesCommonEvalPatch;
            PTVS[ptvsIndex++] = _tokens->curvesCommonEvalCubicPatch;
            PTVS[ptvsIndex++] = HdSt_BasisToShaderKey(basis);
            PTVS[ptvsIndex++] = _tokens->curvesCommonEvalHalfTube;
            PTVS[ptvsIndex++] = basisWidthsInterpolationToken;
            PTVS[ptvsIndex++] = basisNormalInterpolationToken;
            PTVS[ptvsIndex++] = _tokens->curvesCubicVaryingInterp;
            PTVS[ptvsIndex++] = TfToken();
            break;
        }
        default:
            TF_CODING_ERROR("Unhandled drawstyle for basis curves");
        }
    }

    // Disable VS/TCS/TES if we're using Metal tessellation and vice versa.
    if (useMetalTessellation) {
        VS[0]  = TfToken();
        TCS[0] = TfToken();
        TES[0] = TfToken();
    } else {
        PTCS[0] = TfToken();
        PTVS[0] = TfToken();
    }

    // setup fragment shaders
    // Common must be first as it defines terminal interfaces
    uint8_t fsIndex = 0;
    FS[fsIndex++] = _tokens->commonFS;
    if (shadingTerminal == HdBasisCurvesReprDescTokens->hullColor) {
        FS[fsIndex++] = _tokens->hullColorFS;
    } else if (shadingTerminal == HdBasisCurvesReprDescTokens->pointColor) {
        if (pointsShadingEnabled) {
            // Let points for these curves be affected by the associated
            // material so as to appear coherent with the other shaded surfaces
            // that may be part of this rprim.
            FS[fsIndex++] = _tokens->pointShadedFS;
        } else {
            FS[fsIndex++] = _tokens->pointColorFS;
        }
    } else if (shadingTerminal ==
               HdBasisCurvesReprDescTokens->surfaceShaderUnlit) {
        FS[fsIndex++] = _tokens->surfaceUnlitFS;
    } else {
        FS[fsIndex++] = _tokens->surfaceFS;
    }
    FS[fsIndex++] = _tokens->scalarOverrideFS;

    if (isPrimTypePoints) {
        FS[fsIndex++] = _tokens->pointIdFS;
        FS[fsIndex++] = nativeRoundPoints ? _tokens->noDiskSampleMaskFS :
            _tokens->diskSampleMaskFS;
    } else {
        FS[fsIndex++] = _tokens->pointIdFallbackFS;
        FS[fsIndex++] = _tokens->noDiskSampleMaskFS;
    }

    FS[fsIndex++] = hasAuthoredTopologicalVisibility? _tokens->topVisFS :
                        _tokens->topVisFallbackFS;


    if (drawStyle == HdSt_BasisCurvesShaderKey::WIRE || 
        drawStyle == HdSt_BasisCurvesShaderKey::POINTS) {
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

