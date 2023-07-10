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

#include "pxr/imaging/hgiWebGPU/capabilities.h"
#include "pxr/imaging/hgiWebGPU/conversions.h"
#include "pxr/imaging/hgiWebGPU/sampler.h"
#include "pxr/imaging/hgiWebGPU/hgi.h"
#include "pxr/imaging/hgiWebGPU/api.h"

PXR_NAMESPACE_OPEN_SCOPE


HgiWebGPUSampler::HgiWebGPUSampler(
    HgiWebGPU *hgi,
    HgiSamplerDesc const& desc)
    : HgiSampler(desc)
    , _sampler(nullptr)
{
    wgpu::SamplerDescriptor samplerDesc;
    samplerDesc.label = desc.debugName.c_str();
    samplerDesc.magFilter = HgiWebGPUConversions::GetMinMagFilter(desc.magFilter);
    samplerDesc.minFilter = HgiWebGPUConversions::GetMinMagFilter(desc.minFilter);
    samplerDesc.mipmapFilter = HgiWebGPUConversions::GetMipFilter(desc.mipFilter);
    samplerDesc.addressModeU =
        HgiWebGPUConversions::GetSamplerAddressMode(desc.addressModeU);
    samplerDesc.addressModeV =
        HgiWebGPUConversions::GetSamplerAddressMode(desc.addressModeV);
    samplerDesc.addressModeW =
        HgiWebGPUConversions::GetSamplerAddressMode(desc.addressModeW);

    // TODO: Enabling this generates errors and failure to render
    //samplerDesc.compare = HgiWebGPUConversions::GetCompareFunction(desc.compareFunction);
    samplerDesc.compare = wgpu::CompareFunction::Undefined;

    // TODO: this should probably come from an appropriate place
    samplerDesc.maxAnisotropy = 4;
    wgpu::Device device = hgi->GetPrimaryDevice();
    _sampler = device.CreateSampler(&samplerDesc);
}

HgiWebGPUSampler::~HgiWebGPUSampler()
{
    if(_sampler)
    {
        _sampler.Release();
        _sampler = nullptr;
    }
}

uint64_t
HgiWebGPUSampler::GetRawResource() const
{
    return reinterpret_cast<uint64_t>(_sampler.Get());
}

wgpu::Sampler
HgiWebGPUSampler::GetSamplerHandle() const
{
    return _sampler;
}

PXR_NAMESPACE_CLOSE_SCOPE
