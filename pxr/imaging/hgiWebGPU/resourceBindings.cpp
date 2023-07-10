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
#include "pxr/base/tf/diagnostic.h"

#include "pxr/imaging/hgiWebGPU/buffer.h"
#include "pxr/imaging/hgiWebGPU/capabilities.h"
#include "pxr/imaging/hgiWebGPU/conversions.h"
#include "pxr/imaging/hgiWebGPU/resourceBindings.h"
#include "pxr/imaging/hgiWebGPU/sampler.h"
#include "pxr/imaging/hgiWebGPU/texture.h"

#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgiWebGPU/graphicsCmds.h"
#include "pxr/imaging/hgiWebGPU/hgi.h"
#include "pxr/imaging/hgiWebGPU/graphicsPipeline.h"
#include "pxr/imaging/hgiWebGPU/shaderSection.h"

PXR_NAMESPACE_OPEN_SCOPE

// OpenGL has separate bindings for each buffer and image type.
// UBO, SSBO, sampler2D, etc all start at bindingIndex 0. So we expect
// Hgi clients might specify OpenGL style bindingIndex for each. This
// assumes that Hgi codeGen does the same for WGSL.
const static bool reorder = false;

static wgpu::BindGroup _CreateBindGroup(
        wgpu::Device const &device,
        wgpu::BindGroupLayout const &bindGroupLayout,
        std::vector<wgpu::BindGroupEntry> const &entries) {
    wgpu::BindGroupDescriptor bindGroupDesc;
    bindGroupDesc.layout = bindGroupLayout;
    bindGroupDesc.entryCount = entries.size();
    bindGroupDesc.entries = entries.data();
    return device.CreateBindGroup(&bindGroupDesc);
}

static std::vector<wgpu::BindGroupEntry>
_CreateBindGroupEntries(HgiBufferBindDescVector const &buffers)
{
    std::vector<wgpu::BindGroupEntry> bindings;
    //
    // Create a buffer bindgroup entry for each resource
    //

    // Buffers
    for (HgiBufferBindDesc const &b : buffers)
    {
        if (!TF_VERIFY(b.buffers.size() == 1)) continue;
        HgiWebGPUBuffer *buf = static_cast<HgiWebGPUBuffer *>(b.buffers.front().Get());
        wgpu::BindGroupEntry d;
        uint32_t bi = reorder ? (uint32_t)bindings.size() : b.bindingIndex;
        d.binding = bi;
        d.buffer = buf->GetBufferHandle();
        d.offset = b.offsets.front();
        d.size = (b.sizes.size() == 0 || (b.sizes.size() > 0 && b.sizes.front() == 0)) ? buf->GetByteSizeOfResource() : b.sizes.front();
        bindings.push_back(d);
    }

    return bindings;
}

static std::vector<wgpu::BindGroupEntry>
_CreateTextureBindGroupEntries(HgiTextureBindDescVector const &textures) {
    std::vector<wgpu::BindGroupEntry> textureBindings;
    // Textures
    for (HgiTextureBindDesc const &t : textures)
    {
        uint32_t bi = reorder ? (uint32_t)textureBindings.size() : t.bindingIndex;
        // TODO: What should be done with t.resourceType
        // WebGPU only supports textures in combination with samplers
        TF_VERIFY(t.textures.size() == t.samplers.size());
        HgiWebGPUTexture *texture = static_cast<HgiWebGPUTexture *>(t.textures.front().Get());
        wgpu::BindGroupEntry texEntry;
        texEntry.binding = bi;
        texEntry.textureView = texture->GetTextureView();
        textureBindings.push_back(texEntry);
    }
    return textureBindings;
}

static std::vector<wgpu::BindGroupEntry>
_CreateSamplerBindGroupEntries(HgiTextureBindDescVector const &textures) {
    std::vector<wgpu::BindGroupEntry> textureBindings;
    // Textures
    for (HgiTextureBindDesc const &t : textures)
    {
        uint32_t bi = reorder ? (uint32_t)textureBindings.size() : t.bindingIndex;
        // TODO: What should be done with t.resourceType
        // WebGPU only supports textures in combination with samplers
        TF_VERIFY(t.textures.size() == t.samplers.size());
        HgiWebGPUSampler *sampler = static_cast<HgiWebGPUSampler *>(t.samplers.front().Get());
        wgpu::BindGroupEntry samplerEntry;
        samplerEntry.binding = bi;
        samplerEntry.sampler = sampler->GetSamplerHandle();
        textureBindings.push_back(samplerEntry);
    }
    return textureBindings;
}

HgiWebGPUResourceBindings::HgiWebGPUResourceBindings(
    HgiResourceBindingsDesc const &desc)
    : HgiResourceBindings(desc)
    , _bindGroup(nullptr)
    , _textureBindGroup(nullptr)
    , _samplerBindGroup(nullptr)
    , _firstInstance(true)
{
    _bindings = _CreateBindGroupEntries(desc.buffers);
    _textureBindings = _CreateTextureBindGroupEntries(desc.textures);
    _samplerBindings = _CreateSamplerBindGroupEntries(desc.textures);
}

HgiWebGPUResourceBindings::~HgiWebGPUResourceBindings()
{
}

void
HgiWebGPUResourceBindings::_CreateBindGroups(
    wgpu::Device const &device,
    std::vector<wgpu::BindGroupLayout> const &bindGroupLayoutList,
    wgpu::BindGroupEntry const &constantBindGroupEntry,
    bool isConstantDirty)
{
    if (_firstInstance) {
        _firstInstance = false;
        if (constantBindGroupEntry.size > 0) {
            _bindings.push_back(constantBindGroupEntry);
        }
        _bindGroup = _CreateBindGroup(device, bindGroupLayoutList[HgiWebGPUBufferShaderSection::bindingSet], _bindings);
    } else if (isConstantDirty){
        _bindings.back() = constantBindGroupEntry;
        _bindGroup = _CreateBindGroup(device, bindGroupLayoutList[HgiWebGPUBufferShaderSection::bindingSet], _bindings);
    }
    // if we haven't yet created a bind group then create one with the provided layout
    // and the bind group entries we created earlier
    if (_textureBindGroup == nullptr && _samplerBindGroup == nullptr) {
        _textureBindGroup = _CreateBindGroup(device, bindGroupLayoutList[HgiWebGPUTextureShaderSection::bindingSet], _textureBindings);
        _samplerBindGroup = _CreateBindGroup(device, bindGroupLayoutList[HgiWebGPUSamplerShaderSection::bindingSet], _samplerBindings);
    } else if (_textureBindGroup == nullptr || _samplerBindGroup == nullptr) {
        TF_CODING_ERROR("Texture and Sample binding groups should have been initialized at the same time");
    }
}

template <typename PassEncoder, typename>
void HgiWebGPUResourceBindings::BindResources(
        wgpu::Device const &device,
        PassEncoder const &passEncoder,
        std::vector<wgpu::BindGroupLayout> const &bindGroupLayoutList,
        wgpu::BindGroupEntry const &constantBindGroupEntry,
        bool isConstantDirty)
{
    _CreateBindGroups(device, bindGroupLayoutList, constantBindGroupEntry, isConstantDirty);
    if (_bindGroup && _textureBindGroup && _samplerBindGroup) {
        passEncoder.SetBindGroup(HgiWebGPUBufferShaderSection::bindingSet, _bindGroup, 0, nullptr);
        passEncoder.SetBindGroup(HgiWebGPUTextureShaderSection::bindingSet, _textureBindGroup, 0, nullptr);
        passEncoder.SetBindGroup(HgiWebGPUSamplerShaderSection::bindingSet, _samplerBindGroup, 0, nullptr);
    } else if (_bindGroup || _textureBindGroup || _samplerBindGroup ) {
        TF_CODING_ERROR("All binding groups should have been initialized at the same time");
    }
}

template void HgiWebGPUResourceBindings::BindResources(
        wgpu::Device const &device,
        wgpu::RenderPassEncoder const &passEncoder,
        std::vector<wgpu::BindGroupLayout> const &bindGroupLayoutList,
        wgpu::BindGroupEntry const &constantBindGroupEntry,
        bool isConstantDirty);

template void HgiWebGPUResourceBindings::BindResources(
        wgpu::Device const &device,
        wgpu::ComputePassEncoder const &passEncoder,
        std::vector<wgpu::BindGroupLayout> const &bindGroupLayoutList,
        wgpu::BindGroupEntry const &constantBindGroupEntry,
        bool isConstantDirty);

PXR_NAMESPACE_CLOSE_SCOPE
