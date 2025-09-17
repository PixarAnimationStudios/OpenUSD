//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_RESOURCEBINDINGS_H
#define PXR_IMAGING_HGI_RESOURCEBINDINGS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/buffer.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/handle.h"
#include "pxr/imaging/hgi/sampler.h"
#include "pxr/imaging/hgi/texture.h"
#include "pxr/imaging/hgi/types.h"

#include <string>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE





/// \struct HgiBufferBindDesc
///
/// Describes the binding information of a buffer (or array of buffers).
///
/// <ul>
/// <li>buffers:
///   The buffer(s) to be bound.
///   If there are more than one buffer, the buffers will be put in an
///   array-of-buffers. Please note that different platforms have varying
///   limits to max buffers in an array.</li>
/// <li>offsets:
///    Offset (in bytes) where data begins from the start of the buffer.
///    There is an offset corresponding to each buffer in 'buffers'.</li>
/// <li>sizes:
///    Size (in bytes) of the range of data in the buffer to bind.
///    There is a size corresponding to each buffer in 'buffers'.
///    If sizes is empty or the size for a buffer is specified as zero,
///    then the entire buffer is bound.
///    If the offset for a buffer is non-zero, then a non-zero size must
///    also be specified.</li>
/// <li>resourceType:
///    The type of buffer(s) that is to be bound.
///    All buffers in the array must have the same type.
///    Vertex, index and indirect buffers are not bound to a resourceSet.
///    They are instead passed to the draw command.</li>
/// <li>bindingIndex:
///    Binding location for the buffer(s).</li>
/// <li>stageUsage:
///    What shader stage(s) the buffer will be used in.</li>
/// <li>writable:
///    Whether the buffer binding should be non-const.</li>
/// </ul>
///
struct HgiBufferBindDesc
{
    HGI_API
    HgiBufferBindDesc();

    HgiBufferHandleVector buffers;
    std::vector<uint32_t> offsets;
    std::vector<uint32_t> sizes;
    HgiBindResourceType resourceType;
    uint32_t bindingIndex;
    HgiShaderStage stageUsage;
    bool writable;
};
using HgiBufferBindDescVector = std::vector<HgiBufferBindDesc>;

HGI_API
bool operator==(
    const HgiBufferBindDesc& lhs,
    const HgiBufferBindDesc& rhs);

HGI_API
bool operator!=(
    const HgiBufferBindDesc& lhs,
    const HgiBufferBindDesc& rhs);

/// \struct HgiTextureBindDesc
///
/// Describes the binding information of a texture (or array of textures).
///
/// <ul>
/// <li>textures:
///   The texture(s) to be bound.
///   If there are more than one texture, the textures will be put in an
///   array-of-textures (not texture-array). Please note that different
///   platforms have varying limits to max textures in an array.</li>
/// <li>samplers:
///   (optional) The sampler(s) to be bound for each texture in `textures`.
///   If empty a default sampler (clamp_to_edge, linear) should be used. </li>
/// <li>resourceType:
///    The type of the texture(s) that is to be bound.
///    All textures in the array must have the same type.</li>
/// <li>bindingIndex:
///    Binding location for the texture</li>
/// <li>stageUsage:
///    What shader stage(s) the texture will be used in.</li>
/// <li>writable:
///    Whether the texture binding should be non-const.</li>
/// </ul>
///
struct HgiTextureBindDesc
{
    HGI_API
    HgiTextureBindDesc();

    HgiTextureHandleVector textures;
    HgiSamplerHandleVector samplers;
    HgiBindResourceType resourceType;
    uint32_t bindingIndex;
    HgiShaderStage stageUsage;
    bool writable;
};
using HgiTextureBindDescVector = std::vector<HgiTextureBindDesc>;

HGI_API
bool operator==(
    const HgiTextureBindDesc& lhs,
    const HgiTextureBindDesc& rhs);

HGI_API
bool operator!=(
    const HgiTextureBindDesc& lhs,
    const HgiTextureBindDesc& rhs);

/// \struct HgiResourceBindingsDesc
///
/// Describes a set of resources that are bound to the GPU during encoding.
///
/// <ul>
/// <li>buffers:
///   The buffers to be bound (E.g. uniform or shader storage).</li>
/// <li>textures:
///   The textures to be bound.</li>
/// </ul>
///
struct HgiResourceBindingsDesc
{
    HGI_API
    HgiResourceBindingsDesc();

    std::string debugName;
    HgiBufferBindDescVector buffers;
    HgiTextureBindDescVector textures;
};

HGI_API
bool operator==(
    const HgiResourceBindingsDesc& lhs,
    const HgiResourceBindingsDesc& rhs);

HGI_API
bool operator!=(
    const HgiResourceBindingsDesc& lhs,
    const HgiResourceBindingsDesc& rhs);


///
/// \class HgiResourceBindings
///
/// Represents a collection of buffers, texture and vertex attributes that will
/// be used by an cmds object (and pipeline).
///
class HgiResourceBindings
{
public:
    HGI_API
    virtual ~HgiResourceBindings();

    /// The descriptor describes the object.
    HGI_API
    HgiResourceBindingsDesc const& GetDescriptor() const;

protected:
    HGI_API
    HgiResourceBindings(HgiResourceBindingsDesc const& desc);

    HgiResourceBindingsDesc _descriptor;

private:
    HgiResourceBindings() = delete;
    HgiResourceBindings & operator=(const HgiResourceBindings&) = delete;
    HgiResourceBindings(const HgiResourceBindings&) = delete;
};

using HgiResourceBindingsHandle = HgiHandle<HgiResourceBindings>;
using HgiResourceBindingsHandleVector = std::vector<HgiResourceBindingsHandle>;

/// \struct HgiVertexBufferBinding
///
/// Describes a buffer to be bound during encoding.
///
/// <ul>
/// <li>buffer:
///   The buffer to be bound (e.g. uniform, storage, vertex).</li>
/// <li>byteOffset:
///   The byte offset into the buffer from where the data will be bound.</li>
/// <li>index:
///   The binding index to which the buffer will be bound.</li>
/// </ul>
///
struct HgiVertexBufferBinding
{
    HGI_API
    HgiVertexBufferBinding(HgiBufferHandle const &buffer,
                           uint32_t byteOffset,
                           uint32_t index)
        : buffer(buffer)
        , byteOffset(byteOffset)
        , index(index)
    {
    }

    HgiBufferHandle buffer;
    uint32_t byteOffset;
    uint32_t index;
};

using HgiVertexBufferBindingVector = std::vector<HgiVertexBufferBinding>;


PXR_NAMESPACE_CLOSE_SCOPE

#endif
