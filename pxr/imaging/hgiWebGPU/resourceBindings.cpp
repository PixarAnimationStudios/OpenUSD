//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/diagnostic.h"

#include "pxr/imaging/hgiWebGPU/buffer.h"
#include "pxr/imaging/hgiWebGPU/capabilities.h"
#include "pxr/imaging/hgiWebGPU/conversions.h"
#include "pxr/imaging/hgiWebGPU/resourceBindings.h"
#include "pxr/imaging/hgiWebGPU/sampler.h"
#include "pxr/imaging/hgiWebGPU/texture.h"

#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgiWebGPU/graphicsCmds.h"
#include "pxr/imaging/hgiWebGPU/graphicsPipeline.h"
#include "pxr/imaging/hgiWebGPU/hgi.h"
#include "pxr/imaging/hgiWebGPU/shaderSection.h"

PXR_NAMESPACE_OPEN_SCOPE

static wgpu::BindGroup
_CreateBindGroup(wgpu::Device const& device,
    wgpu::BindGroupLayout const& bindGroupLayout,
    std::vector<wgpu::BindGroupEntry> const& entries)
{
    wgpu::BindGroupDescriptor bindGroupDesc;
    bindGroupDesc.layout = bindGroupLayout;
    bindGroupDesc.entryCount = entries.size();
    bindGroupDesc.entries = entries.data();
    return device.CreateBindGroup(&bindGroupDesc);
}

static std::vector<wgpu::BindGroupEntry>
_CreateBindGroupEntries(HgiBufferBindDescVector const& buffers)
{
    std::vector<wgpu::BindGroupEntry> bindings;
    //
    // Create a buffer bindgroup entry for each resource
    //

    // Buffers
    for (HgiBufferBindDesc const& b : buffers) {
        if (!TF_VERIFY(b.buffers.size() == 1))
            continue;
        auto* buf = static_cast<HgiWebGPUBuffer*>(b.buffers.front().Get());
        wgpu::BindGroupEntry d;
        d.binding = b.bindingIndex;
        d.buffer = buf->GetBufferHandle();
        d.offset = b.offsets.front();
        bindings.push_back(d);
    }

    return bindings;
}

static void
_CreateTextureAndSamplerBindGroupEntries(
    HgiTextureBindDescVector const& textures,
    std::vector<wgpu::BindGroupEntry>& textureBindings,
    std::vector<wgpu::BindGroupEntry>& samplerBindings)
{
    for (HgiTextureBindDesc const& t : textures) {
        TF_VERIFY(t.textures.size() == t.samplers.size());
        // Each element of an array texture is a separate binding slot.
        for (size_t i = 0; i < t.textures.size(); i++) {
            auto* texture = static_cast<HgiWebGPUTexture*>(t.textures[i].Get());
            auto* sampler = static_cast<HgiWebGPUSampler*>(t.samplers[i].Get());
            wgpu::BindGroupEntry texEntry, samplerEntry;
            texEntry.binding = samplerEntry.binding = t.bindingIndex + i;
            texEntry.textureView = t.writable ?
                texture->GetStorageTextureView() :
                texture->GetTextureView();
            samplerEntry.sampler = sampler->GetSamplerHandle();
            textureBindings.push_back(texEntry);
            samplerBindings.push_back(samplerEntry);
        }
    }
}

HgiWebGPUResourceBindings::HgiWebGPUResourceBindings(
    HgiResourceBindingsDesc const& desc)
    : HgiResourceBindings(desc)
    , _bindGroup(nullptr)
    , _textureBindGroup(nullptr)
    , _samplerBindGroup(nullptr)
    , _constantBindGroup(nullptr)
{
    _bindings = _CreateBindGroupEntries(desc.buffers);
    _CreateTextureAndSamplerBindGroupEntries(
        desc.textures, _textureBindings, _samplerBindings);
}

HgiWebGPUResourceBindings::~HgiWebGPUResourceBindings() {}

void
HgiWebGPUResourceBindings::_CreateBindGroups(wgpu::Device const& device,
    HgiWebGPUPipelineBindGroups const& pipelineBindGroups,
    wgpu::BindGroupEntry const& constantBindGroupEntry, bool isConstantDirty)
{
    const auto& layouts = pipelineBindGroups.GetLayouts();
    const wgpu::BindGroupLayout bufferLayout =
        layouts[HgiWebGPUBufferShaderSection::bindingSet];
    const wgpu::BindGroupLayout constantLayout =
        layouts[HgiWebGPUBufferShaderSection::constantsBindingSet];
    const wgpu::BindGroupLayout textureLayout =
        layouts[HgiWebGPUTextureShaderSection::bindingSet];
    const wgpu::BindGroupLayout samplerLayout =
        layouts[HgiWebGPUSamplerShaderSection::bindingSet];

    _BindGroupCacheEntry entry{
        bufferLayout, constantLayout, textureLayout, samplerLayout};
    if (const auto it =
            std::find(_bindGroupCache.begin(), _bindGroupCache.end(), entry);
        it != _bindGroupCache.end()) {
        entry = *it;
    }

    if (!entry.buffer) { // Cache miss
        entry.buffer = _CreateBindGroup(device, bufferLayout,
            pipelineBindGroups.FilterLiveBufferBindings(_bindings));
        entry.texture =
            _CreateBindGroup(device, textureLayout, _textureBindings);
        entry.sampler =
            _CreateBindGroup(device, samplerLayout, _samplerBindings);
        entry.constant = constantBindGroupEntry.size > 0 && isConstantDirty ?
            _CreateBindGroup(device, constantLayout, {constantBindGroupEntry}) :
            _CreateBindGroup(device, constantLayout, {});
        _bindGroupCache.push_back(entry);
    } else if (isConstantDirty && constantBindGroupEntry.size > 0) {
        entry.constant =
            _CreateBindGroup(device, constantLayout, {constantBindGroupEntry});
    }

    _bindGroup = entry.buffer;
    _textureBindGroup = entry.texture;
    _samplerBindGroup = entry.sampler;
    _constantBindGroup = entry.constant;
}

PXR_NAMESPACE_CLOSE_SCOPE
