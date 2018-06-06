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
#ifndef HDST_BASIS_CURVES_SHADER_KEY_H
#define HDST_BASIS_CURVES_SHADER_KEY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hdSt/geometricShader.h"
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
struct HdSt_BasisCurvesShaderKey
{
    enum DrawStyle{
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
                              bool basisNormalInterpolation);
    ~HdSt_BasisCurvesShaderKey();

    TfToken const &GetGlslfxFile() const { return glslfx; }
    TfToken const *GetVS() const  { return VS; }
    TfToken const *GetTCS() const { return TCS; }
    TfToken const *GetTES() const { return TES; }
    TfToken const *GetGS() const  { return NULL; }
    TfToken const *GetFS() const  { return FS; }
    bool IsCullingPass() const { return false; }
    HdCullStyle GetCullStyle() const { return HdCullStyleDontCare; }
    HdPolygonMode GetPolygonMode() const { return HdPolygonModeFill; }
    float GetLineWidth() const { return 0; }
    HdSt_GeometricShader::PrimitiveType GetPrimitiveType() const { 
        return primType; 
    }
    bool IsFaceVarying() const { return false; }

    HdSt_GeometricShader::PrimitiveType primType;
    TfToken glslfx;
    TfToken VS[7];
    TfToken TCS[4];
    TfToken TES[8];
    TfToken FS[6];
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDST_BASIS_CURVES_SHADER_KEY
