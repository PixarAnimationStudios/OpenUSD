//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// ported from https://github.com/toji/web-texture-tool/blob/main/src/webgpu-mipmap-generator.js

#ifndef PXR_IMAGING_HGI_WEBGPU_MIPMAPGENERATOR_H
#define PXR_IMAGING_HGI_WEBGPU_MIPMAPGENERATOR_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/texture.h"

#include <unordered_map>
#if defined(EMSCRIPTEN)
#include <webgpu/webgpu_cpp.h>
#else
#include <dawn/webgpu_cpp.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

class HgiWebGPUMipmapGenerator {
  public:
    HgiWebGPUMipmapGenerator(wgpu::Device const &device);
    ~HgiWebGPUMipmapGenerator();
    wgpu::Texture generateMipmap(wgpu::Texture const &level0Texture, const HgiTextureDesc &level0TextureDesc);

  private:
    wgpu::RenderPipeline _getMipmapPipeline(wgpu::TextureFormat const &format);

    wgpu::Device _device;
    wgpu::Sampler _sampler;
    wgpu::ShaderModule _mipmapShaderModule;
    std::unordered_map<wgpu::TextureFormat, wgpu::RenderPipeline> _pipelines;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HGI_WEBGPU_MIPMAPGENERATOR_H
