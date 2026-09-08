//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/base/arch/defines.h"

#include "pxr/imaging/hgiWebGPU/hgi.h"
#include "pxr/imaging/hgiWebGPU/capabilities.h"
#include "pxr/imaging/hgiWebGPU/conversions.h"
#include "pxr/imaging/hgiWebGPU/buffer.h"


PXR_NAMESPACE_OPEN_SCOPE

HgiWebGPUBuffer::HgiWebGPUBuffer(HgiWebGPU *hgi, HgiBufferDesc const & desc)
    : HgiBuffer(desc)
    , _bufferHandle(nullptr)
    , _cpuStaging(nullptr)
{
    if (desc.byteSize == 0) {
        TF_CODING_ERROR("Buffers must have a non-zero length");
    }

    wgpu::BufferDescriptor bufferDesc;
    bufferDesc.label = desc.debugName.c_str();
    bufferDesc.usage = HgiWebGPUConversions::GetBufferUsage(desc.usage);

    // There is no information on how the buffer will be used after creation so, we add the possibility to use it
    // as a src or destination for copy operations.
    bufferDesc.usage |= wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;

    bufferDesc.size = desc.byteSize;
    wgpu::Device device = hgi->GetPrimaryDevice();
    _bufferHandle = device.CreateBuffer(&bufferDesc);

    if (desc.initialData) {
        wgpu::Queue queue = hgi->GetQueue();
        queue.WriteBuffer(_bufferHandle, 0, desc.initialData, desc.byteSize);
    }
    
    _descriptor.initialData = nullptr;
}

HgiWebGPUBuffer::~HgiWebGPUBuffer()
{
    _bufferHandle = nullptr;

    if (_cpuStaging) {
        free(_cpuStaging);
        _cpuStaging = nullptr;
    }
}

size_t
HgiWebGPUBuffer::GetByteSizeOfResource() const
{
    return _descriptor.byteSize;
}

uint64_t
HgiWebGPUBuffer::GetRawResource() const
{
    return (uint64_t) _bufferHandle.Get();
}

void*
HgiWebGPUBuffer::GetCPUStagingAddress()
{
    if (!_cpuStaging) {
        _cpuStaging = malloc(_descriptor.byteSize);
    }

    // This lets the client code memcpy into the cpu staging buffer directly.
    // The staging data must be explicitly copied to the GPU buffer
    // via CopyBufferCpuToGpu cmd by the client.
    return _cpuStaging;
}

PXR_NAMESPACE_CLOSE_SCOPE
