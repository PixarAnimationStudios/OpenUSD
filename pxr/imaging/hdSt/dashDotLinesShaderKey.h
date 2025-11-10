//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_DASH_DOT_LINES_SHADER_KEY_H
#define PXR_IMAGING_HD_ST_DASH_DOT_LINES_SHADER_KEY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hdSt/shaderKey.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdSt_DashDotLinesShaderKey
///
/// The shader key for the HdStDashDotLines.
/// 
struct HdSt_DashDotLinesShaderKey : public HdSt_ShaderKey
{
    HDST_API
    HdSt_DashDotLinesShaderKey(const TfToken& shapeDetail,
                               bool screenSpacePattern,
                               float lineWidth,
                               bool hasTexture,
                               bool hasAuthoredTopologicalVisibility);

    HDST_API
    ~HdSt_DashDotLinesShaderKey();

    TfToken const &GetGlslfxFilename() const override { return glslfx; }
    TfToken const *GetVS() const override  { return VS; }
    TfToken const *GetFS() const override { return FS; }

    HdSt_GeometricShader::PrimitiveType GetPrimitiveType() const override { 
        return primType; 
    }

    bool UseMetalTessellation() const override {
        return false;
    }

    HdPolygonMode GetPolygonMode() const override 
    { 
        return (primType == HdSt_GeometricShader::PrimitiveType::PRIM_BASIS_CURVES_LINES) ? HdPolygonModeLine : HdSt_ShaderKey::GetPolygonMode();
    }

    float GetLineWidth() const override
    {
        return (primType == HdSt_GeometricShader::PrimitiveType::PRIM_BASIS_CURVES_LINES) ? _lineWidth : HdSt_ShaderKey::GetLineWidth();
    }

    HdSt_GeometricShader::PrimitiveType primType;
    TfToken glslfx;
    TfToken VS[6];
    TfToken FS[9];

    float _lineWidth;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDST_DASH_DOT_LINES_SHADER_KEY
