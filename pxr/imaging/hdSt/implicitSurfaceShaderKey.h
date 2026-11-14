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
                                  TfToken const &primType,
                                  const HdImplicitSurfaceGeomStyle geomStyle,
                                  const bool blendWireframeColor,
                                  const bool forceOpaqueEdges,
                                  TfToken const &shadingTerminal,
                                  const bool pointsShadingEnabled,
                                  const bool nativeRoundPoints);
    HDST_API
    ~HdSt_ImplicitSurfaceShaderKey() = default;

    HdCullStyle GetCullStyle() const override { return cullStyle; }

    bool IsDoubleSided() const override { return doubleSided; }

    uint32_t GetVertexCountFallback() const override { return vertexCount; }

    const TfToken GetDepthQualifier() const override {
        return usesBillboard ? HgiShaderKeywordTokens->hdDepthGreater
                             : TfToken();
    }

    // We need to manually handle user-defined clip planes in the FS if we use
    // a billboard, since that's where the actual value of Peye is calculated.
    bool UseHardwareClipPlanes() const override {
        return !usesBillboard;
    }

    HdSt_GeometricShader::PrimitiveType GetPrimitiveType() const override {
        return usesBillboard
                ? HdSt_GeometricShader::PrimitiveType::PRIM_MESH_COARSE_TRIANGLES
                : HdSt_GeometricShader::PrimitiveType::PRIM_POINTS;
    }

    const HdCullStyle cullStyle;
    const bool doubleSided;
    const uint32_t vertexCount;
    const bool usesBillboard;

    TfToken const &GetGlslfxFilename() const override { return glslfx; }
    TfToken const *GetVS() const override { return VS.data(); }
    // Skip TCS, TES and GS stages
    TfToken const *GetFS() const override { return FS.data(); }

    TfToken glslfx;
    TfSmallVector<TfToken, 15> VS;
    TfSmallVector<TfToken, 29> FS;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_IMPLICIT_SURFACE_SHADER_KEY_H
