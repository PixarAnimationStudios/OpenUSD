//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_BUFFER_H
#define PXR_IMAGING_HGI_BUFFER_H

#include <string>
#include <vector>

#include "pxr/pxr.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/handle.h"
#include "pxr/imaging/hgi/types.h"


PXR_NAMESPACE_OPEN_SCOPE


/// \struct HgiBufferDesc
///
/// Describes the properties needed to create a GPU buffer.
///
/// <ul>
/// <li>debugName:
///   This label can be applied as debug label for gpu debugging.</li>
/// <li>usage:
///   Bits describing the intended usage and properties of the buffer.</li>
/// <li>byteSize:
///   Length of buffer in bytes</li>
/// <Li>vertexStride:
///   The size of a vertex in a vertex buffer.
///   This property is only required for vertex buffers.</li>
/// <li>initialData:
///   CPU pointer to initialization data of buffer.
///   The memory is consumed immediately during the creation of the HgiBuffer.
///   The application may alter or free this memory as soon as the constructor
///   of the HgiBuffer has returned.</li>
/// </ul>
///
struct HgiBufferDesc
{
    HGI_API
    HgiBufferDesc()
    : usage(HgiBufferUsageUniform)
    , byteSize(0)
    , vertexStride(0)
    , initialData(nullptr)
    {}

    std::string debugName;
    HgiBufferUsage usage;
    size_t byteSize;
    uint32_t vertexStride;
    void const* initialData;
};

HGI_API
bool operator==(
    const HgiBufferDesc& lhs,
    const HgiBufferDesc& rhs);

HGI_API
bool operator!=(
    const HgiBufferDesc& lhs,
    const HgiBufferDesc& rhs);


///
/// \class HgiBuffer
///
/// Represents a graphics platform independent GPU buffer resource (base class).
/// Buffers should be created via Hgi::CreateBuffer.
/// The fill the buffer with data you supply `initialData` in the descriptor.
/// To update the data inside the buffer later on, use blitCmds.
///
class HgiBuffer
{
public:
    HGI_API
    virtual ~HgiBuffer();

    /// The descriptor describes the object.
    HGI_API
    HgiBufferDesc const& GetDescriptor() const;

    /// Returns the byte size of the GPU buffer.
    /// This can be helpful if the application wishes to tally up memory usage.
    HGI_API
    virtual size_t GetByteSizeOfResource() const = 0;

    /// This function returns the handle to the Hgi backend's gpu resource, cast
    /// to a uint64_t. Clients should avoid using this function and instead
    /// use Hgi base classes so that client code works with any Hgi platform.
    /// For transitioning code to Hgi, it can however we useful to directly
    /// access a platform's internal resource handles.
    /// There is no safety provided in using this. If you by accident pass a
    /// HgiMetal resource into an OpenGL call, bad things may happen.
    /// In OpenGL this returns the GLuint resource name.
    /// In Metal this returns the id<MTLBuffer> as uint64_t.
    /// In Vulkan this returns the VkBuffer as uint64_t.
    /// In DX12 this returns the ID3D12Resource pointer as uint64_t.
    HGI_API
    virtual uint64_t GetRawResource() const = 0;

    /// Returns the 'staging area' in which new buffer data is copied before
    /// it is flushed to GPU.
    /// Some implementations (e.g. Metal) may have build in support for
    /// queueing up CPU->GPU copies. Those implementations can return the
    /// CPU pointer to the buffer's content directly.
    /// The caller should not assume that the data from the CPU staging area
    /// is automatically flushed to the GPU. Instead, after copying is finished,
    /// the caller should use BlitCmds CopyBufferCpuToGpu to ensure the transfer
    /// from the staging area to the GPU is scheduled.
    HGI_API
    virtual void* GetCPUStagingAddress() = 0;

protected:
    HGI_API
    HgiBuffer(HgiBufferDesc const& desc);

    HgiBufferDesc _descriptor;

private:
    HgiBuffer() = delete;
    HgiBuffer & operator=(const HgiBuffer&) = delete;
    HgiBuffer(const HgiBuffer&) = delete;
};

using HgiBufferHandle = HgiHandle<HgiBuffer>;
using HgiBufferHandleVector = std::vector<HgiBufferHandle>;


PXR_NAMESPACE_CLOSE_SCOPE

#endif
