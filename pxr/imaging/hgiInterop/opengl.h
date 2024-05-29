//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIINTEROP_HGIINTEROPOPENGL_H
#define PXR_IMAGING_HGIINTEROP_HGIINTEROPOPENGL_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/imaging/hgi/texture.h"
#include "pxr/imaging/hgiInterop/api.h"


PXR_NAMESPACE_OPEN_SCOPE

class VtValue;

/// \class HgiInteropOpenGL
///
/// Provides GL/GL interop.
///
class HgiInteropOpenGL final
{
public:
    HGIINTEROP_API
    HgiInteropOpenGL();

    HGIINTEROP_API
    ~HgiInteropOpenGL();

    /// Composite provided color (and optional depth) textures over app's
    /// framebuffer contents.
    HGIINTEROP_API
    void CompositeToInterop(
        HgiTextureHandle const &color,
        HgiTextureHandle const &depth,
        VtValue const &framebuffer,
        GfVec4i const& viewport);

private:
    uint32_t _vs;
    uint32_t _fsNoDepth;
    uint32_t _fsDepth;
    uint32_t _prgNoDepth;
    uint32_t _prgDepth;
    uint32_t _vertexBuffer;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
