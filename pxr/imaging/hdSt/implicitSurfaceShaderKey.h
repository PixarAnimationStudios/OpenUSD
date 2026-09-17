//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_IMPLICIT_SURFACE_SHADER_KEY_H
#define PXR_IMAGING_HD_ST_IMPLICIT_SURFACE_SHADER_KEY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hdSt/shaderKey.h"
#include "pxr/imaging/hgi/tokens.h"
#include "pxr/base/tf/smallVector.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

struct HdSt_ImplicitSurfaceShaderKey : public HdSt_ShaderKey
{
    HDST_API
    HdSt_ImplicitSurfaceShaderKey(const HdCullStyle cullStyle,
                                  const bool doubleSided,
                                  const uint32_t vertexCount,
                                  TfToken const &primType);
    HDST_API
    ~HdSt_ImplicitSurfaceShaderKey() = default;

    HdCullStyle GetCullStyle() const override { return cullStyle; }

    bool IsDoubleSided() const override { return doubleSided; }

    uint32_t GetVertexCountFallback() const override { return vertexCount; }

    const TfToken GetDepthQualifier() const override {
        return HgiShaderKeywordTokens->hdDepthGreater;
    }

    // We need to manually handle user-defined clip planes in the FS for
    // native implicits since that's where the actual value of Peye is
    // calculated.
    bool UseHardwareClipPlanes() const override {
        return false;
    }

    HdSt_GeometricShader::PrimitiveType GetPrimitiveType() const override {
        return HdSt_GeometricShader::PrimitiveType::PRIM_MESH_COARSE_TRIANGLES;
    }

    const HdCullStyle cullStyle;
    const bool doubleSided;
    const uint32_t vertexCount;

    TfToken const &GetGlslfxFilename() const override { return glslfx; }
    TfToken const *GetVS() const override { return VS.data(); }
    // Skip TCS, TES and GS stages
    TfToken const *GetFS() const override { return FS.data(); }

    TfToken glslfx;
    TfSmallVector<TfToken, 11> VS;
    TfSmallVector<TfToken, 18> FS;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_IMPLICIT_SURFACE_SHADER_KEY_H
