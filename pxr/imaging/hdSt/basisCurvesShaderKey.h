//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_BASIS_CURVES_SHADER_KEY_H
#define PXR_IMAGING_HD_ST_BASIS_CURVES_SHADER_KEY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hdSt/shaderKey.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdSt_BasisCurvesShaderKey
///
/// The draw styles are designed to strike a balance between matching offline
/// renderers like RenderMan and providing high interactive performance. At
/// the time of this writing, RenderMan (as of R22) only provides two curve
/// drawing modes: a round ray oriented half tube (HALFTUBE, ROUND) and a 
/// flat primvar oriented ribbon (RIBBON, ORIENTED).
///
/// We allow all curves to be drawn as wires: for interactive guides which 
/// may not have authored width and as a performance optimization.
/// 
/// We allow for the combination of (RIBBON, ROUND) as a cheaper code path
/// which fakes a round normal on a flat camera oriented ribbon as an
/// optimization for half tubes. To alleviate aliasing, for very thin curves, 
/// we provide a HAIR mode.
/// 
/// Not all combinations of DrawStyle and NormalStyle are meaningful. For
/// example ORIENTED only makes sense with RIBBON. In the future, we hope to 
/// eliminate NormalStyle, perhaps by merging the (RIBBON, ROUND) mode into a 
/// more automatic HALFTUBE and by relying more on materials for HAIR.
struct HdSt_BasisCurvesShaderKey : public HdSt_ShaderKey
{
    enum DrawStyle{
        POINTS,       // Draws only the control vertices.
        WIRE,         // Draws as lines or isolines, tessellated along length
        RIBBON,       // Draws as patch, tessellated along length only
        HALFTUBE      // Draws as patch, displaced into a half tube shape
    };

    enum NormalStyle{
        ORIENTED,     // Orient to user supplied normals
        HAIR,         // Generated camera oriented normal
        ROUND         // Generated camera oriented normal as a tube
    };

    HdSt_BasisCurvesShaderKey(TfToken const &type, TfToken const &basis,                              
                              DrawStyle drawStyle, NormalStyle normalStyle,
                              bool basisWidthInterpolation,
                              bool basisNormalInterpolation,
                              TfToken shadingTerminal,
                              bool hasAuthoredTopologicalVisibility,
                              bool pointsShadingEnabled,
                              bool hasMetalTessellation);

    ~HdSt_BasisCurvesShaderKey();

    TfToken const &GetGlslfxFilename() const override { return glslfx; }
    TfToken const *GetVS() const override  { return VS; }
    TfToken const *GetTCS() const override { return TCS; }
    TfToken const *GetTES() const override { return TES; }
    TfToken const *GetPTCS() const override { return PTCS; }
    TfToken const *GetPTVS() const override { return PTVS; }
    TfToken const *GetFS() const override { return FS; }

    HdSt_GeometricShader::PrimitiveType GetPrimitiveType() const override { 
        return primType; 
    }

    bool UseMetalTessellation() const override {
        return useMetalTessellation;
    }

    HdSt_GeometricShader::PrimitiveType primType;
    bool useMetalTessellation;
    TfToken glslfx;
    TfToken VS[7];
    TfToken TCS[7];
    TfToken TES[12];
    TfToken PTCS[9];
    TfToken PTVS[14];
    TfToken FS[8];
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDST_BASIS_CURVES_SHADER_KEY
