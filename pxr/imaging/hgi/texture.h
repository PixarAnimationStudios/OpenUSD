//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

/// \struct HgiComponentMapping
///
/// Describes color component mapping.
///
/// <ul>
/// <li>r:
///   What component is used for red channel.
/// <li>g:
///   What component is used for green channel.
/// <li>b:
///   What component is used for blue channel.
/// <li>a:
///   What component is used for alpha channel.
/// </ul>
///
struct HgiComponentMapping
{
    HgiComponentSwizzle r;
    HgiComponentSwizzle g;
    HgiComponentSwizzle b;
    HgiComponentSwizzle a;
};

HGI_API
bool operator==(
    const HgiComponentMapping& lhs,
    const HgiComponentMapping& rhs);

HGI_API
bool operator!=(
    const HgiComponentMapping& lhs,
    const HgiComponentMapping& rhs);

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
/// <li>componentMapping:
///   The mapping of rgba components when accessing the texture.</li>
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
///   Data may optionally include pixels for each mip-level.
///   HgiGetMipInitialData can be used to get to each mip's data and describes
///   in more detail how mip dimensions are rounded.</li>
/// </ul>
///
struct HgiTextureDesc
{
    HgiTextureDesc()
    : usage(0)
    , format(HgiFormatInvalid)
    , componentMapping{
        HgiComponentSwizzleR,
        HgiComponentSwizzleG,
        HgiComponentSwizzleB,
        HgiComponentSwizzleA}
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
    HgiComponentMapping componentMapping;
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

    /// This function initiates a layout change process on this texture 
    /// resource. This feature is at the moment required explicitly by explicit 
    /// APIs like Vulkan.
    HGI_API
    virtual void SubmitLayoutChange(HgiTextureUsage newLayout) = 0;

protected:
    HGI_API
    static
    size_t _GetByteSizeOfResource(const HgiTextureDesc &descriptor);

    HGI_API
    HgiTexture(HgiTextureDesc const& desc);

    HgiTextureDesc _descriptor;

private:
    HgiTexture() = delete;
    HgiTexture & operator=(const HgiTexture&) = delete;
    HgiTexture(const HgiTexture&) = delete;
};


using HgiTextureHandle = HgiHandle<class HgiTexture>;
using HgiTextureHandleVector = std::vector<HgiTextureHandle>;


/// \struct HgiTextureViewDesc
///
/// Describes the properties needed to create a GPU texture view from an
/// existing GPU texture object.
///
/// <ul>
/// <li>debugName:
///   This label can be applied as debug label for GPU debugging.</li>
/// <li>format:
///   The format of the texture view. This format must be compatible with
///   the sourceTexture, but does not have to be the identical format.
///   Generally: All 8-, 16-, 32-, 64-, and 128-bit color formats are 
///   compatible with other formats with the same bit length.
///   For example HgiFormatFloat32Vec4 and HgiFormatInt32Vec4 are compatible.
/// <li>layerCount:
///   The number of layers (texture-arrays).</li>
/// <li>mipLevels:
///   The number of mips in texture.</li>
/// <li>sourceTexture:
///   Handle to the HgiTexture to be used as the source data backing.</li>
/// <li>sourceFirstLayer:
///   The layer index to use from the source texture as the first layer of the
///   view.</li>
/// <li>sourceFirstMip:
///   The mip index to ues from the source texture as the first mip of the
///   view.</li>
///   </ul>
///
struct HgiTextureViewDesc
{
    HgiTextureViewDesc()
    : format(HgiFormatInvalid)
    , layerCount(1)
    , mipLevels(1)
    , sourceTexture()
    , sourceFirstLayer(0)
    , sourceFirstMip(0)
    {}

    std::string debugName;
    HgiFormat format;
    uint16_t layerCount;
    uint16_t mipLevels;
    HgiTextureHandle sourceTexture;
    uint16_t sourceFirstLayer;
    uint16_t sourceFirstMip;
};

HGI_API
bool operator==(
    const HgiTextureViewDesc& lhs,
    const HgiTextureViewDesc& rhs);

HGI_API
bool operator!=(
    const HgiTextureViewDesc& lhs,
    const HgiTextureViewDesc& rhs);

///
/// \class HgiTextureView
///
/// Represents a graphics platform independent GPU texture view resource.
/// Texture Views should be created via Hgi::CreateTextureView.
///
/// A TextureView aliases the data of another texture and is a thin wrapper
/// around a HgiTextureHandle. The embeded texture handle is used to
/// add the texture to resource bindings for use in shaders.
///
/// For example when using a compute shader to fill the mip levels of a
/// texture, like a lightDome texture, we can use a texture view to give the 
/// shader access to a specific mip level of a sourceTexture via a TextureView.
///
/// Another example is to conserve resources by reusing a RGBAF32 texture as
/// a RGBAI32 texture once the F32 texture is no longer needed
/// (transient resources).
///
class HgiTextureView
{
public:
    HGI_API
    HgiTextureView(HgiTextureViewDesc const& desc);

    HGI_API
    virtual ~HgiTextureView();

    /// Set the handle to the texture that aliases another texture.
    HGI_API
    void SetViewTexture(HgiTextureHandle const& handle);

    /// Returns the handle to the texture that aliases another texture.
    HGI_API
    HgiTextureHandle const& GetViewTexture() const;

protected:
    HgiTextureHandle _viewTexture;

private:
    HgiTextureView() = delete;
    HgiTextureView & operator=(const HgiTextureView&) = delete;
    HgiTextureView(const HgiTextureView&) = delete;
};

using HgiTextureViewHandle = HgiHandle<class HgiTextureView>;
using HgiTextureViewHandleVector = std::vector<HgiTextureViewHandle>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
