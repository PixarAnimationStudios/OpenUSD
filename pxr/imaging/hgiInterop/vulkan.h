//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIINTEROP_HGIINTEROPVULKAN_H
#define PXR_IMAGING_HGIINTEROP_HGIINTEROPVULKAN_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/imaging/hgi/texture.h"
#include "pxr/imaging/hgiInterop/api.h"


PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkan;
class VtValue;

/// \class HgiInteropVulkan
///
/// Provides Vulkan/GL interop.
///
class HgiInteropVulkan final
{
public:
    HGIINTEROP_API
    HgiInteropVulkan(Hgi* hgiVulkan);

    HGIINTEROP_API
    ~HgiInteropVulkan();

    /// Composite provided color (and optional depth) textures over app's
    /// framebuffer contents.
    HGIINTEROP_API
    void CompositeToInterop(
        HgiTextureHandle const &color,
        HgiTextureHandle const &depth,
        VtValue const &framebuffer,
        GfVec4i const& viewport);

private:
    HgiInteropVulkan() = delete;

    HgiVulkan* _hgiVulkan;
    uint32_t _vs;
    uint32_t _fsNoDepth;
    uint32_t _fsDepth;
    uint32_t _prgNoDepth;
    uint32_t _prgDepth;
    uint32_t _vertexBuffer;

    // XXX We tmp copy Vulkan's GPU texture to CPU and then to GL texture
    // Once we share GPU memory between Vulkan and GL we can remove this.
    uint32_t _glColorTex;
    uint32_t _glDepthTex;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
