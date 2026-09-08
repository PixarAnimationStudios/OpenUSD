//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_WEBGPU_BUFFER_H
#define PXR_IMAGING_HGI_WEBGPU_BUFFER_H

#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu.h>


#include "pxr/pxr.h"
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgi/buffer.h"


PXR_NAMESPACE_OPEN_SCOPE


class HgiWebGPU;

/// \class HgiWebGPUBuffer
///
/// Represents a WebGPU GPU buffer resource.
///
class HgiWebGPUBuffer final : public HgiBuffer {
public:
    HGIWEBGPU_API
    HgiWebGPUBuffer(HgiWebGPU *hgi, HgiBufferDesc const & desc);

    HGIWEBGPU_API
    ~HgiWebGPUBuffer() override;

    HGIWEBGPU_API
    size_t GetByteSizeOfResource() const override;

    HGIWEBGPU_API
    uint64_t GetRawResource() const override;
    
    HGIWEBGPU_API
    void* GetCPUStagingAddress() override;

    wgpu::Buffer GetBufferHandle() const {return _bufferHandle;}

private:
    HgiWebGPUBuffer() = delete;
    HgiWebGPUBuffer & operator=(const HgiWebGPUBuffer&) = delete;
    HgiWebGPUBuffer(const HgiWebGPUBuffer&) = delete;

    wgpu::Buffer _bufferHandle;
    void* _cpuStaging;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
