//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_POINTS_SHADER_KEY_H
#define PXR_IMAGING_HD_ST_POINTS_SHADER_KEY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hdSt/shaderKey.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

struct HdSt_PointsShaderKey : public HdSt_ShaderKey
{
    HDST_API
    HdSt_PointsShaderKey(bool nativeRoundPoints);
    HDST_API
    ~HdSt_PointsShaderKey();

    TfToken const &GetGlslfxFilename() const override { return glslfx; }
    TfToken const *GetVS() const override { return VS; }
    // Skip TCS, TES and GS stages
    TfToken const *GetFS() const override { return FS; }

    HdSt_GeometricShader::PrimitiveType GetPrimitiveType() const override {
        return HdSt_GeometricShader::PrimitiveType::PRIM_POINTS; 
    }

    TfToken glslfx;
    TfToken VS[7];
    TfToken FS[7];
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDST_POINTS_SHADER_KEY
