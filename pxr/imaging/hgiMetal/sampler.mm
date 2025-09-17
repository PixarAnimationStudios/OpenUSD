//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include <Metal/Metal.h>

#include "pxr/imaging/hgiMetal/conversions.h"
#include "pxr/imaging/hgiMetal/diagnostic.h"
#include "pxr/imaging/hgiMetal/hgi.h"
#include "pxr/imaging/hgiMetal/sampler.h"

PXR_NAMESPACE_OPEN_SCOPE


HgiMetalSampler::HgiMetalSampler(HgiMetal *hgi, HgiSamplerDesc const& desc)
    : HgiSampler(desc)
    , _samplerId(nil)
    , _label(nil)
{
    MTLSamplerDescriptor* smpDesc = [MTLSamplerDescriptor new];

    smpDesc.sAddressMode =
        HgiMetalConversions::GetSamplerAddressMode(desc.addressModeU);
    smpDesc.tAddressMode =
        HgiMetalConversions::GetSamplerAddressMode(desc.addressModeV);
    smpDesc.rAddressMode =
        HgiMetalConversions::GetSamplerAddressMode(desc.addressModeW);
    smpDesc.minFilter = HgiMetalConversions::GetMinMagFilter(desc.minFilter);
    smpDesc.magFilter = HgiMetalConversions::GetMinMagFilter(desc.magFilter);
    smpDesc.mipFilter = HgiMetalConversions::GetMipFilter(desc.mipFilter);
    smpDesc.supportArgumentBuffers = true;
    smpDesc.borderColor = HgiMetalConversions::GetBorderColor(desc.borderColor);
    smpDesc.compareFunction = 
        HgiMetalConversions::GetCompareFunction(desc.compareFunction);

    if ((desc.minFilter != HgiSamplerFilterNearest ||
         desc.mipFilter == HgiMipFilterLinear) &&
         desc.magFilter != HgiSamplerFilterNearest) {
        static const uint32_t maxAnisotropy = 16;
        smpDesc.maxAnisotropy = std::min<float>({
            static_cast<float>(maxAnisotropy),
            static_cast<float>(desc.maxAnisotropy),
            static_cast<float>(TfGetEnvSetting(HGI_MAX_ANISOTROPY))});
    }
    
    HGIMETAL_DEBUG_LABEL(smpDesc, _descriptor.debugName.c_str());
    
    _samplerId= [hgi->GetPrimaryDevice() newSamplerStateWithDescriptor:smpDesc];

    [smpDesc release];
}

HgiMetalSampler::~HgiMetalSampler()
{
    if (_label) {
        [_label release];
        _label = nil;
    }

    if (_samplerId != nil) {
        [_samplerId release];
        _samplerId = nil;
    }
}

uint64_t
HgiMetalSampler::GetRawResource() const
{
    return (uint64_t) _samplerId;
}

id<MTLSamplerState>
HgiMetalSampler::GetSamplerId() const
{
    return _samplerId;
}

PXR_NAMESPACE_CLOSE_SCOPE
