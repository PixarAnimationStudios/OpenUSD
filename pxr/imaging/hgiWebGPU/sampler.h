//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_WEBGPU_SAMPLER_H
#define PXR_IMAGING_HGI_WEBGPU_SAMPLER_H

#include "pxr/imaging/hgi/sampler.h"
#include "pxr/imaging/hgiWebGPU/api.h"


PXR_NAMESPACE_OPEN_SCOPE

class HgiWebGPU;

///
/// \class HgiWebGPUSampler
///
/// WebGPU implementation of HgiSampler
///
class HgiWebGPUSampler final : public HgiSampler
{
public:
    HGIWEBGPU_API
    ~HgiWebGPUSampler() override;

    HGIWEBGPU_API
    uint64_t GetRawResource() const override;

    HGIWEBGPU_API
    wgpu::Sampler GetSamplerHandle() const;

protected:
    friend class HgiWebGPU;

    HGIWEBGPU_API
    HgiWebGPUSampler(
        HgiWebGPU *hgi,
        HgiSamplerDesc const& desc);

private:
    HgiWebGPUSampler() = delete;
    HgiWebGPUSampler & operator=(const HgiWebGPUSampler&) = delete;
    HgiWebGPUSampler(const HgiWebGPUSampler&) = delete;

    wgpu::Sampler _sampler;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif