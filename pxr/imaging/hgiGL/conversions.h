//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_GL_CONVERSIONS_H
#define PXR_IMAGING_HGI_GL_CONVERSIONS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiGL/api.h"
#include "pxr/imaging/garch/glApi.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/types.h"
#include "pxr/base/gf/vec4f.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HgiGLConversions
///
/// Converts from Hgi types to GL types.
///
class HgiGLConversions final
{
public:
    HGIGL_API
    static void GetFormat(
        HgiFormat inFormat,
        HgiTextureUsage inUsage,
        GLenum *outFormat,
        GLenum *outType,
        GLenum *outInternalFormat = nullptr);

    HGIGL_API
    static GLenum GetFormatType(HgiFormat inFormat);

    HGIGL_API
    static bool IsVertexAttribIntegerFormat(HgiFormat inFormat);

    HGIGL_API
    static std::vector<GLenum> GetShaderStages(HgiShaderStage ss);

    HGIGL_API
    static GLenum GetCullMode(HgiCullMode cm);

    HGIGL_API
    static GLenum GetPolygonMode(HgiPolygonMode pm);

    HGIGL_API
    static GLenum GetBlendFactor(HgiBlendFactor bf);

    HGIGL_API
    static GLenum GetBlendEquation(HgiBlendOp bo);

    HGIGL_API
    static GLenum GetCompareFunction(HgiCompareFunction cf);

    HGIGL_API
    static GLenum GetStencilOp(HgiStencilOp op);

    HGIGL_API
    static GLenum GetTextureType(HgiTextureType tt);

    HGIGL_API
    static GLenum GetSamplerAddressMode(HgiSamplerAddressMode am);

    HGIGL_API
    static GLenum GetMagFilter(HgiSamplerFilter mf);

    HGIGL_API
    static GLenum GetMinFilter(
        HgiSamplerFilter minFilter, 
        HgiMipFilter mipFilter);

    HGIGL_API
    static GfVec4f GetBorderColor(HgiBorderColor borderColor);

    HGIGL_API
    static GLenum GetComponentSwizzle(HgiComponentSwizzle);

    HGIGL_API
    static GLenum GetPrimitiveType(HgiPrimitiveType pt);
    
    HGIGL_API
    static std::string GetImageLayoutFormatQualifier(HgiFormat inFormat);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif

