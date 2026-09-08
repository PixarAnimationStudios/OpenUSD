//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

// Ported from https://github.com/playcanvas/engine/blob/main/src/platform/graphics/webgpu/webgpu-resolver.js

#ifndef PXR_IMAGING_HGI_WEBGPU_DEPTH_RESOLVER_H
#define PXR_IMAGING_HGI_WEBGPU_DEPTH_RESOLVER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiWebGPU/texture.h"
#include <unordered_map>

#if defined(EMSCRIPTEN)
#include <webgpu/webgpu_cpp.h>
#else

#include <dawn/webgpu_cpp.h>

#endif

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HgiWebGPUDepthResolver
///
/// This class resolves the depth buffer from a multisampled texture to a
/// non-multisampled texture by taking the first sample.
/// This is necessary because there is no native support for multisampled
/// depth texture resolution in WebGPU.
class HgiWebGPUDepthResolver {
public:
    HgiWebGPUDepthResolver(wgpu::Device const &device);
    ~HgiWebGPUDepthResolver();

    HGIWEBGPU_API
    void resolveDepth(wgpu::CommandEncoder const &commandEncoder, HgiWebGPUTexture &sourceTexture,
                      HgiWebGPUTexture &destinationTexture);

private:
    wgpu::RenderPipeline _getResolverPipeline(wgpu::TextureFormat const &format);

    wgpu::Device _device;
    wgpu::ShaderModule _resolverShaderModule;
    std::unordered_map<wgpu::TextureFormat, wgpu::RenderPipeline> _pipelines;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HGI_WEBGPU_DEPTH_RESOLVER_H
