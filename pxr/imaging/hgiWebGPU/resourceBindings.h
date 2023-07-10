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
#ifndef PXR_IMAGING_HGI_WEBGPU_RESOURCEBINDINGS_H
#define PXR_IMAGING_HGI_WEBGPU_RESOURCEBINDINGS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/resourceBindings.h"
#include "pxr/imaging/hgiWebGPU/api.h"

#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HgiWebGPUResourceBindings
///
/// WebGPU implementation of HgiResourceBindings.
///
///
class HgiWebGPUResourceBindings final : public HgiResourceBindings
{
public:
    HGIWEBGPU_API
    ~HgiWebGPUResourceBindings() override;

    template <typename PassEncoder, typename = std::enable_if_t<std::is_same_v<PassEncoder, wgpu::RenderPassEncoder> || std::is_same_v<PassEncoder, wgpu::ComputePassEncoder>>>
    void BindResources(
        wgpu::Device const &device,
        PassEncoder const &passEncoder,
        std::vector<wgpu::BindGroupLayout> const &bindGroupLayoutList,
        wgpu::BindGroupEntry const &constantBindGroupEntry,
        bool isConstantDirty);

protected:
    friend class HgiWebGPU;

    HGIWEBGPU_API
    HgiWebGPUResourceBindings(HgiResourceBindingsDesc const& desc);

    std::vector<wgpu::BindGroupEntry> _bindings;
    std::vector<wgpu::BindGroupEntry> _textureBindings;
    std::vector<wgpu::BindGroupEntry> _samplerBindings;

private:
    HgiWebGPUResourceBindings() = delete;
    HgiWebGPUResourceBindings & operator=(const HgiWebGPUResourceBindings&) = delete;
    HgiWebGPUResourceBindings(const HgiWebGPUResourceBindings&) = delete;
    
    void
    _CreateBindGroups(
        wgpu::Device const &device,
        std::vector<wgpu::BindGroupLayout> const &bindGroupLayoutList,
        wgpu::BindGroupEntry const &constantBindGroupEntry,
        bool isConstantDirty);

    wgpu::BindGroup _bindGroup;
    wgpu::BindGroup _textureBindGroup;
    wgpu::BindGroup _samplerBindGroup;
    bool _firstInstance;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
