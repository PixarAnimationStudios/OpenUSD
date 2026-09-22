//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    if (desc.mipFilter == HgiMipFilter::HgiMipFilterNotMipmapped) {
        // We need to emulate this filter as there is no correspondence in the WebGPU API
        samplerDesc.lodMaxClamp = 0;
        samplerDesc.lodMinClamp = 0;
    }
    samplerDesc.mipmapFilter = HgiWebGPUConversions::GetMipFilter(desc.mipFilter);

    samplerDesc.addressModeU =
        HgiWebGPUConversions::GetSamplerAddressMode(desc.addressModeU);
    samplerDesc.addressModeV =
        HgiWebGPUConversions::GetSamplerAddressMode(desc.addressModeV);
    samplerDesc.addressModeW =
        HgiWebGPUConversions::GetSamplerAddressMode(desc.addressModeW);

    samplerDesc.compare = desc.enableCompare
        ? HgiWebGPUConversions::GetCompareFunction(desc.compareFunction)
        : wgpu::CompareFunction::Undefined;

    if ((desc.minFilter != HgiSamplerFilterNearest ||
         desc.mipFilter == HgiMipFilterLinear) &&
        desc.magFilter != HgiSamplerFilterNearest) {
        // WebGPU will clamp the value by the max supported in the platform
        samplerDesc.maxAnisotropy = 16;
    }
    wgpu::Device device = hgi->GetPrimaryDevice();
    _sampler = device.CreateSampler(&samplerDesc);
}

HgiWebGPUSampler::~HgiWebGPUSampler()
{
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
