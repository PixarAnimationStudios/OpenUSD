//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/extGpuBuffer.h"
#include "pxr/base/tf/diagnostic.h"

#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

uint64_t
HdSt_GetNextExtGpuBufferHandleId()
{
    // Single shared counter for all external GPU buffer alias handles. The
    // reserved tag bit (see HdStExternalHandleIdBit) keeps these ids in a region
    // disjoint from Hgi's own sequential ids and from other external kinds --
    // HgiHandle equality is id-only, so distinct buffers must not share an id.
    static std::atomic<uint64_t> nextId{1};
    return (uint64_t(1) << HdStExtGpuBufferHandleIdBit) | nextId.fetch_add(1);
}

HdStExtGpuBuffer::HdStExtGpuBuffer(
    uint64_t rawHandle,
    size_t byteSize)
    : HgiBuffer(HgiBufferDesc())
    , _rawHandle(rawHandle)
    , _byteSize(byteSize)
{
    _descriptor.byteSize = byteSize;
    // Match HdStExtGpuBufferArrayRange::GetUsageHint(): external buffers may be
    // bound as vertex attributes or read as storage (e.g. by GPU computations),
    // so advertise both rather than vertex-only.
    _descriptor.usage = HgiBufferUsageVertex | HgiBufferUsageStorage;
    _descriptor.debugName = "ExtGpuBuffer";
}

HdStExtGpuBuffer::~HdStExtGpuBuffer() = default;

size_t
HdStExtGpuBuffer::GetByteSizeOfResource() const
{
    return _byteSize;
}

uint64_t
HdStExtGpuBuffer::GetRawResource() const
{
    return _rawHandle;
}

void *
HdStExtGpuBuffer::GetCPUStagingAddress()
{
    TF_CODING_ERROR("CPU staging not available for external GPU buffers");
    return nullptr;
}

void
HdStExtGpuBuffer::UpdateRawHandle(uint64_t rawHandle, size_t byteSize)
{
    _rawHandle = rawHandle;
    _byteSize = byteSize;
    _descriptor.byteSize = byteSize;
}

PXR_NAMESPACE_CLOSE_SCOPE
