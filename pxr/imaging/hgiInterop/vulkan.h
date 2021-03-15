//
// Copyright 2020 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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
