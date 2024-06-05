//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgi/sampler.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiSampler::HgiSampler(HgiSamplerDesc const& desc)
    : _descriptor(desc)
{
}

HgiSampler::~HgiSampler() = default;

HgiSamplerDesc const&
HgiSampler::GetDescriptor() const
{
    return _descriptor;
}

bool operator==(const HgiSamplerDesc& lhs,
    const HgiSamplerDesc& rhs)
{
    return  lhs.debugName == rhs.debugName &&
            lhs.magFilter == rhs.magFilter &&
            lhs.minFilter == rhs.minFilter &&
            lhs.mipFilter == rhs.mipFilter &&
            lhs.addressModeU == rhs.addressModeU &&
            lhs.addressModeV == rhs.addressModeV &&
            lhs.addressModeW == rhs.addressModeW
    ;
}

bool operator!=(const HgiSamplerDesc& lhs,
    const HgiSamplerDesc& rhs)
{
    return !(lhs == rhs);
}


PXR_NAMESPACE_CLOSE_SCOPE
