//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_WEBGPU_CAPABILITIES_H
#define PXR_IMAGING_HGI_WEBGPU_CAPABILITIES_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgi/capabilities.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HgiWebGPUCapabilities
///
/// Reports the capabilities of the WebGPU device.
///
class HgiWebGPUCapabilities final : public HgiCapabilities
{
public:
    HGIWEBGPU_API
    ~HgiWebGPUCapabilities() override;
    
    HGIWEBGPU_API
    int GetAPIVersion() const override;
    
    HGIWEBGPU_API
    int GetShaderVersion() const override;

    HGIWEBGPU_API
    const wgpu::Limits& GetLimits() const
    {
        return _limits;
    }

protected:
    friend class HgiWebGPU;

    HGIWEBGPU_API
    HgiWebGPUCapabilities(wgpu::Device device);

    wgpu::Limits _limits;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
