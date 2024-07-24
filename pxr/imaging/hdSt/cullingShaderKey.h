//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_CULLING_SHADER_KEY_H
#define PXR_IMAGING_HD_ST_CULLING_SHADER_KEY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hdSt/shaderKey.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE


struct HdSt_CullingShaderKey : public HdSt_ShaderKey
{
    HdSt_CullingShaderKey(bool instancing, bool tinyCull, bool counting);
    ~HdSt_CullingShaderKey();

    TfToken const &GetGlslfxFilename() const override { return glslfx; }
    TfToken const *GetVS() const override { return VS; }

    bool IsFrustumCullingPass() const override { return true; }
    HdSt_GeometricShader::PrimitiveType GetPrimitiveType() const override {
        return HdSt_GeometricShader::PrimitiveType::PRIM_POINTS; 
    }

    TfToken glslfx;
    TfToken VS[6];
};

struct HdSt_CullingComputeShaderKey : public HdSt_ShaderKey
{
    HdSt_CullingComputeShaderKey(bool instancing, bool tinyCull, bool counting);
    ~HdSt_CullingComputeShaderKey();

    TfToken const &GetGlslfxFilename() const override { return glslfx; }
    TfToken const *GetCS() const override { return CS; }

    bool IsFrustumCullingPass() const override { return true; }
    HdSt_GeometricShader::PrimitiveType GetPrimitiveType() const override {
        return HdSt_GeometricShader::PrimitiveType::PRIM_COMPUTE;
    }

    TfToken glslfx;
    TfToken CS[6];
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_CULLING_SHADER_KEY_H
