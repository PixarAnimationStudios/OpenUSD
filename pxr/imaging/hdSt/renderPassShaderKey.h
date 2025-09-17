//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_RENDER_PASS_SHADER_KEY_H
#define PXR_IMAGING_HD_ST_RENDER_PASS_SHADER_KEY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hdSt/shaderKey.h"
#include "pxr/imaging/hd/aov.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

struct HdSt_RenderPassShaderKey : public HdSt_ShaderKey
{
    HDST_API
    HdSt_RenderPassShaderKey(const HdRenderPassAovBindingVector& aovBindings);
    HDST_API
    ~HdSt_RenderPassShaderKey();

    TfToken const &GetGlslfxFilename() const override { return glslfx; }
    TfToken const *GetVS() const override { return VS; }
    TfToken const *GetPTCS() const override { return PTCS; }
    TfToken const *GetPTVS() const override { return PTVS; }
    TfToken const *GetTCS() const override { return TCS; }
    TfToken const *GetTES() const override { return TES; }
    TfToken const *GetGS() const override { return GS; }
    TfToken const *GetFS() const override { return FS; }

    HDST_API
    std::string GetGlslfxString() const override;

    // Unused for HdSt_RenderPassShaderKey.
    HdSt_GeometricShader::PrimitiveType GetPrimitiveType() const override {
        return HdSt_GeometricShader::PrimitiveType::PRIM_POINTS; 
    }

    TfToken glslfx;
    TfToken VS[3];
    TfToken PTCS[2];
    TfToken PTVS[3];
    TfToken TCS[2];
    TfToken TES[3];
    TfToken GS[3];
    TfToken FS[10];
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_RENDER_PASS_SHADER_KEY_H