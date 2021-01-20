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
#ifndef PXR_IMAGING_HD_ST_MESH_SHADER_KEY_H
#define PXR_IMAGING_HD_ST_MESH_SHADER_KEY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hdSt/shaderKey.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE


struct HdSt_MeshShaderKey : public HdSt_ShaderKey
{
    enum NormalSource
    {
        NormalSourceScene,
        NormalSourceSmooth,
        NormalSourceLimit,
        NormalSourceFlat,
        NormalSourceGeometryShader
    };

    HdSt_MeshShaderKey(HdSt_GeometricShader::PrimitiveType primType,
                       TfToken shadingTerminal,
                       bool useCustomDisplacement,
                       NormalSource normalsSource,
                       HdInterpolation normalsInterpolation,
                       bool doubleSided,
                       bool forceGeometryShader,
                       bool hasTopologicalVisibility,
                       bool blendWireframeColor,
                       HdCullStyle cullStyle,
                       HdMeshGeomStyle geomStyle,
                       float lineWidth,
                       bool hasMirroredTransform,
                       bool hasInstancer,
                       bool enableScalarOverride);

    // Note: it looks like gcc 4.8 has a problem issuing
    // a wrong warning as "array subscript is above array bounds"
    // when the default destructor is automatically generated at callers.
    // Having an empty destructor explicitly within this linkage apparently
    // avoids the issue.
    ~HdSt_MeshShaderKey();

    HdCullStyle GetCullStyle() const override { return cullStyle; }
    bool UseHardwareFaceCulling() const override {
        return useHardwareFaceCulling;
    }
    bool HasMirroredTransform() const override {
        return hasMirroredTransform;
    }
    bool IsDoubleSided() const override {
        return doubleSided;
    }

    HdPolygonMode GetPolygonMode() const override { return polygonMode; }
    float GetLineWidth() const override { return lineWidth; }
    HdSt_GeometricShader::PrimitiveType GetPrimitiveType() const override {
        return primType; 
    }

    HdSt_GeometricShader::PrimitiveType primType;
    HdCullStyle cullStyle;
    bool useHardwareFaceCulling;
    bool hasMirroredTransform;
    bool doubleSided;
    HdPolygonMode polygonMode;
    float lineWidth;

    TfToken const &GetGlslfxFilename() const override { return glslfx; }
    TfToken const *GetVS()  const override { return VS; }
    TfToken const *GetTCS() const override { return TCS; }
    TfToken const *GetTES() const override { return TES; }
    TfToken const *GetGS()  const override { return GS; }
    TfToken const *GetFS()  const override { return FS; }

    TfToken glslfx;
    TfToken VS[7];
    TfToken TCS[3];
    TfToken TES[4];
    TfToken GS[11];
    TfToken FS[15];
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_MESH_SHADER_KEY_H
