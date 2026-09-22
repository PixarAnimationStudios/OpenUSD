//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_WEBGPU_RESOURCEBINDINGS_H
#define PXR_IMAGING_HGI_WEBGPU_RESOURCEBINDINGS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/resourceBindings.h"
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgiWebGPU/pipelineBindGroups.h"
#include "pxr/imaging/hgiWebGPU/shaderSection.h"

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

    template<typename PassEncoder, typename = std::enable_if_t<std::is_same_v<PassEncoder, wgpu::RenderPassEncoder> ||
                                                               std::is_same_v<PassEncoder, wgpu::ComputePassEncoder> ||
                                                               std::is_same_v<PassEncoder, wgpu::RenderBundleEncoder>
                                                               >>
    void BindResources(
        wgpu::Device const &device,
        PassEncoder const &passEncoder,
        HgiWebGPUPipelineBindGroups const &pipelineBindGroups,
        wgpu::BindGroupEntry const &constantBindGroupEntry,
        bool isConstantDirty) {
        _CreateBindGroups(device, pipelineBindGroups, constantBindGroupEntry, isConstantDirty);
        if (_bindGroup && _textureBindGroup && _samplerBindGroup) {
            passEncoder.SetBindGroup(HgiWebGPUBufferShaderSection::bindingSet, _bindGroup, 0, nullptr);
            passEncoder.SetBindGroup(HgiWebGPUTextureShaderSection::bindingSet, _textureBindGroup, 0, nullptr);
            passEncoder.SetBindGroup(HgiWebGPUSamplerShaderSection::bindingSet, _samplerBindGroup, 0, nullptr);
            passEncoder.SetBindGroup(HgiWebGPUBufferShaderSection::constantsBindingSet, _constantBindGroup, 0, nullptr);
        } else if (_bindGroup || _textureBindGroup || _samplerBindGroup ) {
            TF_CODING_ERROR("All binding groups should have been initialized at the same time");
        }
    }

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
        HgiWebGPUPipelineBindGroups const &pipelineBindGroups,
        wgpu::BindGroupEntry const &constantBindGroupEntry,
        bool isConstantDirty);

    struct _BindGroupCacheEntry
    {
        // Key
        wgpu::BindGroupLayout bufferLayout;
        wgpu::BindGroupLayout constantLayout;
        wgpu::BindGroupLayout textureLayout;
        wgpu::BindGroupLayout samplerLayout;
        // Value
        wgpu::BindGroup buffer;
        wgpu::BindGroup constant;
        wgpu::BindGroup texture;
        wgpu::BindGroup sampler;

        bool operator==(const _BindGroupCacheEntry& other) const
        {
            return other.bufferLayout.Get() == bufferLayout.Get() &&
            other.constantLayout.Get() == constantLayout.Get() &&
            other.textureLayout.Get() == textureLayout.Get() &&
            other.samplerLayout.Get() == samplerLayout.Get();
        }
    };

    // Currently active bind groups for BindResources()
    wgpu::BindGroup _bindGroup;
    wgpu::BindGroup _textureBindGroup;
    wgpu::BindGroup _samplerBindGroup;
    wgpu::BindGroup _constantBindGroup;

    std::vector<_BindGroupCacheEntry> _bindGroupCache;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
