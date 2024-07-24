//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
        NormalSourceFlatGeometric,
        NormalSourceFlatScreenSpace,
    };

    HdSt_MeshShaderKey(HdSt_GeometricShader::PrimitiveType primType,
                       TfToken shadingTerminal,
                       NormalSource normalsSource,
                       HdInterpolation normalsInterpolation,
                       HdCullStyle cullStyle,
                       HdMeshGeomStyle geomStyle,
                       HdSt_GeometricShader::FvarPatchType fvarPatchType,
                       float lineWidth,
                       bool doubleSided,
                       bool hasBuiltinBarycentrics,
                       bool hasMetalTessellation,
                       bool hasCustomDisplacement,
                       bool hasPerFaceInterpolation,
                       bool hasTopologicalVisibility,
                       bool blendWireframeColor,
                       bool hasMirroredTransform,
                       bool hasInstancer,
                       bool enableScalarOverride,
                       bool pointsShadingEnabled,
                       bool forceOpaqueEdges);

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
    bool UseMetalTessellation() const override {
        return useMetalTessellation;
    }

    HdPolygonMode GetPolygonMode() const override { return polygonMode; }
    float GetLineWidth() const override { return lineWidth; }
    HdSt_GeometricShader::PrimitiveType GetPrimitiveType() const override {
        return primType; 
    }
    HdSt_GeometricShader::FvarPatchType GetFvarPatchType() const override {
        return fvarPatchType; 
    }

    HdSt_GeometricShader::PrimitiveType primType;
    HdCullStyle cullStyle;
    bool useHardwareFaceCulling;
    bool hasMirroredTransform;
    bool doubleSided;
    bool useMetalTessellation;
    HdPolygonMode polygonMode;
    float lineWidth;
    HdSt_GeometricShader::FvarPatchType fvarPatchType;

    TfToken const &GetGlslfxFilename() const override { return glslfx; }
    TfToken const *GetVS()  const override { return VS; }
    TfToken const *GetTCS() const override { return TCS; }
    TfToken const *GetTES() const override { return TES; }
    TfToken const *GetPTCS()  const override { return PTCS; }
    TfToken const *GetPTVS()  const override { return PTVS; }
    TfToken const *GetGS()  const override { return GS; }
    TfToken const *GetFS()  const override { return FS; }

    TfToken glslfx;
    TfToken VS[7];
    TfToken TCS[4];
    TfToken TES[4];
    TfToken PTCS[4];
    TfToken PTVS[12];
    TfToken GS[10];
    TfToken FS[22];
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_MESH_SHADER_KEY_H
