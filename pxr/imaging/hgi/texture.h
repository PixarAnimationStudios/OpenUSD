//
// Copyright 2019 Pixar
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
#ifndef PXR_IMAGING_HGI_TEXTURE_H
#define PXR_IMAGING_HGI_TEXTURE_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/handle.h"
#include "pxr/imaging/hgi/types.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \struct HgiTextureDesc
///
/// Describes the properties needed to create a GPU texture.
///
/// <ul>
/// <li>debugName:
///   This label can be applied as debug label for GPU debugging.</li>
/// <li>usage:
///   Describes how the texture is intended to be used.</li>
/// <li>format:
///   The format of the texture.
/// <li>dimensions:
///   The resolution of the texture (width, height, depth).</li>
/// <li>type:
///   Type of texture (2D, 3D).</li>
/// <li>layerCount:
///   The number of layers (texture-arrays).</li>
/// <li>mipLevels:
///   The number of mips in texture.</li>
/// <li>sampleCount:
///   samples per texel (multi-sampling).</li>
/// <li>pixelsByteSize:
///   Byte size (length) of pixel data (i.e., initialData).</li>
/// <li>initialData:
///   CPU pointer to initialization pixels of the texture.
///   The memory is consumed immediately during the creation of the HgiTexture.
///   The application may alter or free this memory as soon as the constructor
///   of the HgiTexture has returned.
///   Data may optionally include pixels for each mip-level.</li>
/// </ul>
///
struct HgiTextureDesc
{
    HgiTextureDesc()
    : usage(0)
    , format(HgiFormatInvalid)
    , type(HgiTextureType2D)
    , dimensions(0)
    , layerCount(1)
    , mipLevels(1)
    , sampleCount(HgiSampleCount1)
    , pixelsByteSize(0)
    , initialData(nullptr)
    {}

    std::string debugName;
    HgiTextureUsage usage;
    HgiFormat format;
    HgiTextureType type;
    GfVec3i dimensions;
    uint16_t layerCount;
    uint16_t mipLevels;
    HgiSampleCount sampleCount;
    size_t pixelsByteSize;
    void const* initialData;
};

HGI_API
bool operator==(
    const HgiTextureDesc& lhs,
    const HgiTextureDesc& rhs);

HGI_API
bool operator!=(
    const HgiTextureDesc& lhs,
    const HgiTextureDesc& rhs);


///
/// \class HgiTexture
///
/// Represents a graphics platform independent GPU texture resource.
/// Textures should be created via Hgi::CreateTexture.
///
/// Base class for Hgi textures.
/// To the client (HdSt) texture resources are referred to via
/// opaque, stateless handles (HgTextureHandle).
///
class HgiTexture
{
public:
    HGI_API
    virtual ~HgiTexture();

    /// The descriptor describes the object.
    HGI_API
    HgiTextureDesc const& GetDescriptor() const;

    /// Returns the byte size of the GPU texture.
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
    /// In Metal this returns the id<MTLTexture> as uint64_t.
    /// In Vulkan this returns the VkImage as uint64_t.
    /// In DX12 this returns the ID3D12Resource pointer as uint64_t.
    HGI_API
    virtual uint64_t GetRawResource() const = 0;

protected:
    HGI_API
    HgiTexture(HgiTextureDesc const& desc);

    HgiTextureDesc _descriptor;

private:
    HgiTexture() = delete;
    HgiTexture & operator=(const HgiTexture&) = delete;
    HgiTexture(const HgiTexture&) = delete;
};


/// Explicitly instantiate and define texture handle
template class HgiHandle<class HgiTexture>;
using HgiTextureHandle = HgiHandle<class HgiTexture>;
using HgiTextureHandleVector = std::vector<HgiTextureHandle>;


PXR_NAMESPACE_CLOSE_SCOPE

#endif
