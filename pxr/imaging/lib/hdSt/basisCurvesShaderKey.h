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


struct HdSt_BasisCurvesShaderKey
{
    HdSt_BasisCurvesShaderKey(TfToken const &basis, bool authoredNormals,
                            bool refine);
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
    HdSt_GeometricShader::PrimitiveType GetPrimitiveType() const { 
        return primType; 
    }
    bool IsFaceVarying() const { return false; }

    HdSt_GeometricShader::PrimitiveType primType;
    TfToken glslfx;
    TfToken VS[4];
    TfToken TCS[4];
    TfToken TES[5];
    TfToken FS[6];
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDST_BASIS_CURVES_SHADER_KEY
