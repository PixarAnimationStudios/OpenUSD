//
// Copyright 2020 benmalartre
//
// Unlicensed
// 
#ifndef PXR_IMAGING_PLUGIN_LOFI_GL_CONVERSIONS_H
#define PXR_IMAGING_PLUGIN_LOFI_GL_CONVERSIONS_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/api.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hio/types.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE


class LoFiGLConversions {
public:
    /// Returns the size of glDataType.
    /// For example: sizeof(GLuint)
    LOFI_API
    static size_t GetComponentSize(int glDataType);

    LOFI_API
    static GLenum GetGlDepthFunc(HdCompareFunction func);

    LOFI_API
    static GLenum GetGlStencilFunc(HdCompareFunction func);

    LOFI_API
    static GLenum GetGlStencilOp(HdStencilOp op);

    LOFI_API
    static GLenum GetGlBlendOp(HdBlendOp op);

    LOFI_API
    static GLenum GetGlBlendFactor(HdBlendFactor factor);

    LOFI_API
    static HioFormat GetHioFormat(HdFormat inFormat);

    LOFI_API
    static int GetGLAttribType(HdType type);

    /// Return the name of the given type as represented in GLSL.
    LOFI_API
    static TfToken GetGLSLTypename(HdType type);

    /// Return a GLSL-safe, mangled name identifier.
    LOFI_API
    static TfToken GetGLSLIdentifier(TfToken const& identifier);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_GL_CONVERSIONS_H

