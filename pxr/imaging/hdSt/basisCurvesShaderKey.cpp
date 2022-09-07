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

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((baseGLSLFX,                      "basisCurves.glslfx"))

    //Set tess factors
    ((setTessFactorsGLSL,    "Curves.JointTessControl.SetTessFactorGLSL"))
    ((setTessFactorsMSL,      "Curves.JointTessControl.SetTessFactorMSL"))
    ((invertNormal,           "Curves.InvertNormal"))
    ((noInvertNormal,         "Curves.NoInvertNormal"))
    ((postTessellationShared,  "Curves.PostTessellation.Shared"))

    // normal related mixins
    ((curvesVertexNormalOriented,      "Curves.Vertex.Normal.Oriented"))
    ((curvesVertexNormalImplicit,      "Curves.Vertex.Normal.Implicit"))
    ((curvesPostTessVertexNormalOriented,
        "Curves.PostTess.Normal.Oriented"))
    ((curvesPostTessVertexNormalImplicit,
        "Curves.PostTess.Normal.Implicit"))

    // basis mixins
    ((curvesCoeffs,                    "Curves.Coeffs"))
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

    ((curvesTessControlShared,         "Curves.Shared"))
    ((curvesJointTessControlLinearRibbon, "Curves.JointTessControl.Linear.Ribbon"))
    ((curvesJointTessControlLinearHalfTube, "Curves.JointTessControl.Linear.HalfTube"))
    ((curvesJointTessControlCubicRibbon,    "Curves.JointTessControl.Cubic.Ribbon"))
    ((curvesJointTessControlCubicHalfTube,  "Curves.JointTessControl.Cubic.HalfTube"))
    ((curvesJointTessEvalLinearPatch,       "Curves.JointTessEval.Linear.Patch"))
    ((curvesJointTessEvalCubicPatch,        "Curves.JointTessEval.Cubic.Patch"))
    ((curvesTessEvalRibbonImplicit,    "Curves.Ribbon.Implicit"))
    ((curvesTessEvalRibbonOriented,    "Curves.Ribbon.Oriented"))
    ((curvesTessEvalHalfTube,          "Curves.TessEval.HalfTube"))

    ((curvesFragmentHalfTube,          "Curves.Fragment.HalfTube"))
    ((curvesFragmentRibbonRound,       "Curves.Fragment.Ribbon.Round"))
    ((curvesFragmentRibbonOriented,    "Curves.Fragment.Ribbon.Oriented"))
    ((curvesFragmentHair,              "Curves.Fragment.Hair"))

    // main for all the shader stages
    ((curvesVertexPatch,               "Curves.Vertex.Patch"))
    ((curvesVertexWire,                "Curves.Vertex.Wire"))
    ((curvesPostTessVertexPatch,       "Curves.PostTessVertex.Patch"))
    ((curvesPostTessVertexWire,        "Curves.PostTessVertex.Wire"))
    ((curvesPostTessVertexCubicWire,   "Curves.PostTessVertex.Cubic.Wire"))
    ((curvesTessControlLinearPatch,    "Curves.TessControl.Linear.Patch"))
    ((curvesTessControlCubicWire,      "Curves.TessControl.Cubic.Wire"))
    ((curvesTessControlCubicPatch,     "Curves.TessControl.Cubic.Patch"))
    ((curvesPostTessControlLinearPatch,"Curves.PostTessControl.Linear.Patch"))
    ((curvesPostTessControlCubicWire,  "Curves.PostTessControl.Cubic.Wire"))
    ((curvesPostTessControlCubicPatch, "Curves.PostTessControl.Cubic.Patch"))
    ((curvesTessEvalPatch,             "Curves.TessEval.Patch"))
    ((curvesTessEvalCubicWire,         "Curves.TessEval.Cubic.Wire"))
    ((curvesFragmentWire,              "Curves.Fragment.Wire"))
    ((curvesFragmentPatch,             "Curves.Fragment.Patch"))

    // instancing related mixins
    ((instancing,                      "Instancing.Transform"))

    // terminals
    ((commonFS,                        "Fragment.CommonTerminals"))
    ((hullColorFS,                     "Fragment.HullColor"))
    ((pointColorFS,                    "Fragment.PointColor"))
    ((pointShadedFS,                    "Fragment.PointShaded"))
    ((surfaceFS,                       "Fragment.Surface"))
    ((surfaceUnlitFS,                  "Fragment.SurfaceUnlit"))
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
    bool basisNormalInterpolation,
    TfToken shadingTerminal,
    bool hasAuthoredTopologicalVisibility,
    bool pointsShadingEnabled,
    bool hasPostTessVertexSupport)
    : glslfx(_tokens->baseGLSLFX),
      useMetalTessellation(false)
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
        if (drawStyle == DrawStyle::WIRE) {
            primType =
            HdSt_GeometricShader::PrimitiveType::PRIM_BASIS_CURVES_CUBIC_WIRE_PATCHES;
        } else {
            primType =
            HdSt_GeometricShader::PrimitiveType::PRIM_BASIS_CURVES_CUBIC_PATCHES;
        }
    } else if (drawThick){
        primType =
        HdSt_GeometricShader::PrimitiveType::PRIM_BASIS_CURVES_LINEAR_PATCHES;
    } else {
        primType =
        HdSt_GeometricShader::PrimitiveType::PRIM_BASIS_CURVES_LINES;
    }

    bool isPrimTypePoints = HdSt_GeometricShader::IsPrimTypePoints(primType);

    bool oriented = normalStyle == HdSt_BasisCurvesShaderKey::ORIENTED;

    bool skipTessLinear = (linear && (drawStyle == HdSt_BasisCurvesShaderKey::POINTS ||
                    drawStyle == HdSt_BasisCurvesShaderKey::WIRE));
    bool skipTessCubic = false;
        //(cubic && (drawStyle == HdSt_BasisCurvesShaderKey::POINTS));

    //Cubic wire on metal needs improvements, not enabled
    bool demandPostTessVertexShader = hasPostTessVertexSupport &&
        !(skipTessLinear || skipTessCubic);
    useMetalTessellation = demandPostTessVertexShader;

    uint8_t vsIndex = 0;
    if (!demandPostTessVertexShader) {
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
      }

      uint8_t tesIndex = 0;
      uint8_t tcsIndex = 0;
      uint8_t ptcsIndex = 0;
      uint8_t ptvsIndex = 0;

      TfToken ribbonToken = oriented ? _tokens->curvesTessEvalRibbonOriented
                            : _tokens->curvesTessEvalRibbonImplicit;
      TfToken basisWidthsInterpolationToken = basisWidthInterpolation ?
                                              _tokens->curveCubicWidthsBasis :
                                              _tokens->curveCubicWidthsLinear;
      if (!demandPostTessVertexShader) {
          TfToken basisNormalInterpolationToken =
              basisNormalInterpolation ?
              _tokens->curveCubicNormalsBasis :
              _tokens->curveCubicNormalsLinear;
              TES[tesIndex++] = _tokens->curvesCoeffs;
              TES[tesIndex++] = _tokens->invertNormal;
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
                   TCS[tcsIndex++] = _tokens->setTessFactorsGLSL;
                   TCS[tcsIndex++] = _tokens->curvesTessControlShared;
                   TCS[tcsIndex++] = _tokens->curvesTessControlLinearPatch;
                   TCS[tcsIndex++] = _tokens->curvesJointTessControlLinearRibbon;
                   TCS[tcsIndex++] = TfToken();

                   TES[tesIndex++] = _tokens->instancing;
                   TES[tesIndex++] = _tokens->curvesTessEvalPatch;
                   TES[tesIndex++] = _tokens->curvesFallback;
                   TES[tesIndex++] = _tokens->curvesJointTessEvalLinearPatch;
                   TES[tesIndex++] = ribbonToken;
                   TES[tesIndex++] = _tokens->curvesLinearVaryingInterp;
                   TES[tesIndex++] = TfToken();
                   break;
               }
               case HdSt_BasisCurvesShaderKey::HALFTUBE:
               {
                   TCS[tcsIndex++] = _tokens->setTessFactorsGLSL;
                   TCS[tcsIndex++] = _tokens->curvesTessControlShared;
                   TCS[tcsIndex++] = _tokens->curvesTessControlLinearPatch;
                   TCS[tcsIndex++] = _tokens->curvesJointTessControlLinearHalfTube;
                   TCS[tcsIndex++] = TfToken();

                   TES[tesIndex++] = _tokens->instancing;
                   TES[tesIndex++] = _tokens->curvesTessEvalPatch;
                   TES[tesIndex++] = _tokens->curvesFallback;
                   TES[tesIndex++] = _tokens->curvesJointTessEvalLinearPatch;
                   TES[tesIndex++] = _tokens->curvesTessEvalHalfTube;
                   TES[tesIndex++] = _tokens->curvesLinearVaryingInterp;
                   TES[tesIndex++] = TfToken();
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
                   TCS[tcsIndex++] = _tokens->setTessFactorsGLSL;
                   TCS[tcsIndex++] = _tokens->curvesTessControlShared;
                   TCS[tcsIndex++] = _tokens->curvesTessControlCubicWire;
                   TCS[tcsIndex++] = TfToken();

                   TES[tesIndex++] = _tokens->instancing;
                   TES[tesIndex++] = _tokens->curvesTessEvalCubicWire;
                   TES[tesIndex++] = HdSt_BasisToShaderKey(basis);
                   TES[tesIndex++] = _tokens->curvesCubicVaryingInterp;
                   TES[tesIndex++] = TfToken();
                   break;
               }
               case HdSt_BasisCurvesShaderKey::RIBBON:
               {
                   TCS[tcsIndex++] = _tokens->setTessFactorsGLSL;
                   TCS[tcsIndex++] = _tokens->curvesTessControlShared;
                   TCS[tcsIndex++] = _tokens->curvesTessControlCubicPatch;
                   TCS[tcsIndex++] = _tokens->curvesJointTessControlCubicRibbon;
                   TCS[tcsIndex++] = TfToken();


                   TES[tesIndex++] = _tokens->instancing;
                   TES[tesIndex++] = _tokens->curvesTessEvalPatch;
                   TES[tesIndex++] = _tokens->curvesJointTessEvalCubicPatch;
                   TES[tesIndex++] = HdSt_BasisToShaderKey(basis);
                   TES[tesIndex++] = ribbonToken;
                   TES[tesIndex++] = basisWidthsInterpolationToken;
                   TES[tesIndex++] = basisNormalInterpolationToken;
                   TES[tesIndex++] = _tokens->curvesCubicVaryingInterp;
                   TES[tesIndex++] = TfToken();
                   break;
               }
               case HdSt_BasisCurvesShaderKey::HALFTUBE:
               {
                   TCS[tcsIndex++] = _tokens->setTessFactorsGLSL;
                   TCS[tcsIndex++] = _tokens->curvesTessControlShared;
                   TCS[tcsIndex++] = _tokens->curvesTessControlCubicPatch;
                   TCS[tcsIndex++] = _tokens->curvesJointTessControlCubicHalfTube;
                   TCS[tcsIndex++] = TfToken();

                   TES[tesIndex++] = _tokens->instancing;
                   TES[tesIndex++] = _tokens->curvesTessEvalPatch;
                   TES[tesIndex++] = _tokens->curvesJointTessEvalCubicPatch;
                   TES[tesIndex++] = HdSt_BasisToShaderKey(basis);
                   TES[tesIndex++] = _tokens->curvesTessEvalHalfTube;
                   TES[tesIndex++] = basisWidthsInterpolationToken;
                   TES[tesIndex++] = basisNormalInterpolationToken;
                   TES[tesIndex++] = _tokens->curvesCubicVaryingInterp;
                   TES[tesIndex++] = TfToken();
                   break;
               }
               default:
                   TF_CODING_ERROR("Unhandled drawstyle for basis curves");
           }
       }
   } else { //Is post tess vertex shader
       TCS[0] = TfToken();
       TES[0] = TfToken();
       TfToken basisNormalInterpolationToken =
           basisNormalInterpolation ?
           _tokens->curveCubicNormalsBasis :
           _tokens->curveCubicNormalsLinear;
       TfToken postTessNormal =
           oriented ? _tokens->curvesPostTessVertexNormalOriented :
               _tokens->curvesPostTessVertexNormalImplicit;
       PTVS[ptvsIndex++] = _tokens->postTessellationShared;
       PTVS[ptvsIndex++] = _tokens->curvesCoeffs;
       PTVS[ptvsIndex++] = postTessNormal;
       PTVS[ptvsIndex++] = _tokens->pointIdNoneVS;
       //ON PTVS, the orientation of linear ribbons is inverted, correct that
       PTVS[ptvsIndex++] = (drawStyle == HdSt_BasisCurvesShaderKey::RIBBON && linear)
               ? _tokens->invertNormal : _tokens->noInvertNormal;

       PTCS[ptcsIndex++] = _tokens->postTessellationShared;
       PTCS[ptcsIndex++] = _tokens->setTessFactorsMSL;
       PTCS[ptcsIndex++] = _tokens->curvesCoeffs;
       PTCS[ptcsIndex++] = _tokens->instancing;
       PTCS[ptcsIndex++] = postTessNormal;
       if (linear) {
           switch(drawStyle) {
               case HdSt_BasisCurvesShaderKey::POINTS:
               case HdSt_BasisCurvesShaderKey::WIRE:
               {
                   //This path is currently not taken. Kept in as
                   //a hard to spot bug might otherwise appear
                   PTCS[ptcsIndex++] = TfToken();
                   PTVS[ptvsIndex++] = TfToken();
                   break;
               }
               case HdSt_BasisCurvesShaderKey::RIBBON:
               {
                   PTCS[ptcsIndex++] = _tokens->curvesTessControlShared;

                   PTCS[ptcsIndex++] = _tokens->curvesPostTessControlLinearPatch;
                   PTCS[ptcsIndex++] = _tokens->curvesJointTessControlLinearRibbon;
                   PTCS[ptcsIndex++] = TfToken();

                   PTVS[ptvsIndex++] = _tokens->instancing;
                   PTVS[ptvsIndex++] = _tokens->curvesPostTessVertexPatch;
                   PTVS[ptvsIndex++] = _tokens->curvesFallback;
                   PTVS[ptvsIndex++] = _tokens->curvesJointTessEvalLinearPatch;
                   PTVS[ptvsIndex++] = ribbonToken;
                   PTVS[ptvsIndex++] = _tokens->curvesLinearVaryingInterp;
                   PTVS[ptvsIndex++] = TfToken();
                   break;
               }
               case HdSt_BasisCurvesShaderKey::HALFTUBE:
               {
                   PTCS[ptcsIndex++] = _tokens->curvesTessControlShared;
                   PTCS[ptcsIndex++] = _tokens->curvesPostTessControlLinearPatch;
                   PTCS[ptcsIndex++] = _tokens->curvesJointTessControlLinearHalfTube;
                   PTCS[ptcsIndex++] = TfToken();

                   PTVS[ptvsIndex++] = _tokens->instancing;
                   PTVS[ptvsIndex++] = _tokens->curvesPostTessVertexPatch;
                   PTVS[ptvsIndex++] = _tokens->curvesFallback;
                   PTVS[ptvsIndex++] = _tokens->curvesJointTessEvalLinearPatch;
                   PTVS[ptvsIndex++] = _tokens->curvesTessEvalHalfTube;
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
                   PTCS[ptcsIndex++] = TfToken();
                   PTVS[ptvsIndex++] = TfToken();
                   break;
               }
               case HdSt_BasisCurvesShaderKey::WIRE:
               {
                   //METAL Cubic wire support needs improvements
                   PTCS[ptcsIndex++] = _tokens->curvesTessControlShared;
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
                   PTCS[ptcsIndex++] = _tokens->curvesTessControlShared;
                   PTCS[ptcsIndex++] = _tokens->curvesPostTessControlCubicPatch;
                   PTCS[ptcsIndex++] = _tokens->curvesJointTessControlCubicRibbon;
                   PTCS[ptcsIndex++] = TfToken();

                   PTVS[ptvsIndex++] = _tokens->instancing;
                   PTVS[ptvsIndex++] = _tokens->curvesPostTessVertexPatch;
                   PTVS[ptvsIndex++] = _tokens->curvesJointTessEvalCubicPatch;
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
                   PTCS[ptcsIndex++] = _tokens->curvesTessControlShared;
                   PTCS[ptcsIndex++] = _tokens->curvesPostTessControlCubicPatch;
                   PTCS[ptcsIndex++] = _tokens->curvesJointTessControlCubicHalfTube;
                   PTCS[ptcsIndex++] = TfToken();

                   PTVS[ptvsIndex++] = _tokens->instancing;
                   PTVS[ptvsIndex++] = _tokens->curvesPostTessVertexPatch;
                   PTVS[ptvsIndex++] = _tokens->curvesJointTessEvalCubicPatch;
                   PTVS[ptvsIndex++] = HdSt_BasisToShaderKey(basis);
                   PTVS[ptvsIndex++] = _tokens->curvesTessEvalHalfTube;
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

    FS[fsIndex++] = isPrimTypePoints?
                        _tokens->pointIdFS : _tokens->pointIdFallbackFS;
    
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

