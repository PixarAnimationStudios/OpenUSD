//
// Copyright 2020 Pixar
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
///    This if an offset for each buffer in 'buffers'.</li>
/// <li>resourceType:
///    The type of buffer(s) that is to be bound.
///    All buffers in the array must have the same type.
///    Vertex, index and indirect buffers are not bound to a resourceSet.
///    They are instead passed to the draw command.</li>
/// <li>bindingIndex:
///    Binding location for the buffer(s).</li>
/// <li>stageUsage:
///    What shader stage(s) the buffer will be used in.</li>
/// </ul>
///
struct HgiBufferBindDesc
{
    HGI_API
    HgiBufferBindDesc();

    HgiBufferHandleVector buffers;
    std::vector<uint32_t> offsets;
    HgiBindResourceType resourceType;
    uint32_t bindingIndex;
    HgiShaderStage stageUsage;
};
using HgiBufferBindDescVector = std::vector<HgiBufferBindDesc>;

HGI_API
bool operator==(
    const HgiBufferBindDesc& lhs,
    const HgiBufferBindDesc& rhs);

HGI_API
inline bool operator!=(
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


PXR_NAMESPACE_CLOSE_SCOPE

#endif
