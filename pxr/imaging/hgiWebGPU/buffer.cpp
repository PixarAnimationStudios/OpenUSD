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
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/base/arch/defines.h"

#include "pxr/imaging/hgiWebGPU/hgi.h"
#include "pxr/imaging/hgiWebGPU/capabilities.h"
#include "pxr/imaging/hgiWebGPU/conversions.h"
#include "pxr/imaging/hgiWebGPU/buffer.h"


PXR_NAMESPACE_OPEN_SCOPE

HgiWebGPUBuffer::HgiWebGPUBuffer(HgiWebGPU *hgi, HgiBufferDesc const & desc)
    : HgiBuffer(desc)
    , _hgi(hgi)
    , _bufferHandle(nullptr)
{
    if (desc.byteSize == 0) {
        TF_CODING_ERROR("Buffers must have a non-zero length");
    }

    wgpu::BufferDescriptor bufferDesc;
    bufferDesc.label = desc.debugName.c_str();
    bufferDesc.usage = HgiWebGPUConversions::GetBufferUsage(desc.usage);

    if( desc.usage & HgiBufferUsageIndex32 || desc.usage & HgiBufferUsageVertex || desc.usage & HgiBufferUsageUniform || desc.usage & HgiBufferUsageStorage)
        bufferDesc.usage |= wgpu::BufferUsage::CopyDst;
    else
        bufferDesc.usage |= wgpu::BufferUsage::MapRead;

    // TODO: We basically need to add all usages since we don't know at this point in time in which stage this will be used.
    // For example, the pxr/imaging/hdSt/vboMemoryManager.cpp#351 sets HgiBufferUsageUniform | HgiBufferUsageVertex
    // for the points buffer, which is then later used in the pxr/imaging/hdSt/smoothNormals.cpp#75 with a resource
    // binding of the type storage, resulting in incompatible types
    bufferDesc.usage |= wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage | wgpu::BufferUsage::Index;

    bufferDesc.size = desc.byteSize;
    wgpu::Device device = hgi->GetPrimaryDevice();
    _bufferHandle = device.CreateBuffer(&bufferDesc);

    _cpuStagingBuffer.resize(desc.byteSize);

    if (desc.initialData) {
        wgpu::Queue queue = hgi->GetQueue();
        queue.WriteBuffer(_bufferHandle, 0, desc.initialData, desc.byteSize);
        memcpy(_cpuStagingBuffer.data(), desc.initialData, desc.byteSize);
    }
    
    _descriptor.initialData = nullptr;
}

HgiWebGPUBuffer::~HgiWebGPUBuffer()
{
    if (_bufferHandle) {
        _bufferHandle.Unmap();
        _bufferHandle.Destroy();
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
    return static_cast<void *>(_cpuStagingBuffer.data());
}

PXR_NAMESPACE_CLOSE_SCOPE
