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
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((baseGLSLFX,            "basisCurves.glslfx"))
    ((basisCurvesBezier,     "BasisCurves.BezierBasis"))
    ((basisCurvesBspline,    "BasisCurves.BsplineBasis"))
    ((basisCurvesCatmullRom, "BasisCurves.CatmullRomBasis"))
    ((basisCurvesVS,         "BasisCurves.Vertex"))
    ((basisCurvesTCS,        "BasisCurves.TessellationControl"))
    ((basisCurvesTES,        "BasisCurves.TessellationEvaluation"))
    ((basisCurvesVertNormalAuth, "BasisCurves.Vertex.Normal.Authored"))
    ((basisCurvesTESNormalAuth, "BasisCurves.TES.Normal.Authored"))
    ((basisCurvesVertNormalCam,  "BasisCurves.Vertex.Normal.CameraFacing"))
    ((basisCurvesTESNormalCam,  "BasisCurves.TES.Normal.CameraFacing"))
    ((basisCurvesFS,         "BasisCurves.Fragment"))
    ((lineVS,                "Line.Vertex"))
    ((surfaceFS,             "Fragment.Surface"))
    ((lineFS,                "Line.Fragment"))
    ((instancing,            "Instancing.Transform"))
);

HdSt_BasisCurvesShaderKey::HdSt_BasisCurvesShaderKey(TfToken const &basis,
                                                     bool authoredNormals,
                                                     bool refine)
    : glslfx(_tokens->baseGLSLFX)
{
    if (refine) {
        primType = Hd_GeometricShader::PrimitiveType::PRIM_BASIS_CURVES_PATCHES;
    } else {
        primType = Hd_GeometricShader::PrimitiveType::PRIM_BASIS_CURVES_LINES;
    }

    VS[0]  = _tokens->instancing;
    VS[1]  = refine ? _tokens->basisCurvesVS
                    : _tokens->lineVS;
    VS[2] = authoredNormals ? _tokens->basisCurvesVertNormalAuth 
                             : _tokens->basisCurvesVertNormalCam;
    VS[3]  = TfToken();

    TCS[0] = refine ? _tokens->basisCurvesTCS   : TfToken();

    TES[0] = refine ? _tokens->instancing       : TfToken();
    TES[1] = refine ? _tokens->basisCurvesTES   : TfToken();
    TES[2] = refine ? (basis == HdTokens->bezier     ? _tokens->basisCurvesBezier
                    : (basis == HdTokens->catmullRom ? _tokens->basisCurvesCatmullRom
                                                     : _tokens->basisCurvesBspline))
                    : TfToken();
    TES[3] = authoredNormals ? _tokens->basisCurvesTESNormalAuth 
                             : _tokens->basisCurvesTESNormalCam;
    TES[4] = TfToken();

    FS[0] = _tokens->surfaceFS;
    FS[1]  = refine ? _tokens->basisCurvesFS
                    : _tokens->lineFS;
    FS[2]  = TfToken();
}

HdSt_BasisCurvesShaderKey::~HdSt_BasisCurvesShaderKey()
{
}

PXR_NAMESPACE_CLOSE_SCOPE

