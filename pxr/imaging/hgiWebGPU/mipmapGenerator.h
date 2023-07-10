//
// Copyright 2022 Pixar
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

class WebGPUMipmapGenerator {
  public:
    WebGPUMipmapGenerator(wgpu::Device const &device);
    ~WebGPUMipmapGenerator();
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
