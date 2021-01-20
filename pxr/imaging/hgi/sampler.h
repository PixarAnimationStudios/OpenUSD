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
#ifndef PXR_IMAGING_HGI_SAMPLER_H
#define PXR_IMAGING_HGI_SAMPLER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/handle.h"
#include "pxr/imaging/hgi/types.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \struct HgiSamplerDesc
///
/// Describes the properties needed to create a GPU sampler.
///
/// <ul>
/// <li>debugName:
///   This label can be applied as debug label for GPU debugging.</li>
/// <li>magFilter:
///    The (magnification) filter used to combine pixels when the sample area is
///    smaller than a pixel.</li>
/// <li>minFilter:
///    The (minification) filter used to combine pixels when the sample area is
///    larger than a pixel.</li>
/// <li> mipFilter:
///    The filter used for combining pixels between two mipmap levels.</li>
/// <li>addressMode***: 
///    Wrapping modes.</li>
/// </ul>
///
struct HgiSamplerDesc
{
    HgiSamplerDesc()
        : magFilter(HgiSamplerFilterNearest)
        , minFilter(HgiSamplerFilterNearest)
        , mipFilter(HgiMipFilterNotMipmapped)
        , addressModeU(HgiSamplerAddressModeClampToEdge)
        , addressModeV(HgiSamplerAddressModeClampToEdge)
        , addressModeW(HgiSamplerAddressModeClampToEdge)
    {}

    std::string debugName;
    HgiSamplerFilter magFilter;
    HgiSamplerFilter minFilter;
    HgiMipFilter mipFilter;
    HgiSamplerAddressMode addressModeU;
    HgiSamplerAddressMode addressModeV;
    HgiSamplerAddressMode addressModeW;
};

HGI_API
bool operator==(
    const HgiSamplerDesc& lhs,
    const HgiSamplerDesc& rhs);

HGI_API
bool operator!=(
    const HgiSamplerDesc& lhs,
    const HgiSamplerDesc& rhs);


///
/// \class HgiSampler
///
/// Represents a graphics platform independent GPU sampler resource that
/// perform texture sampling operations.
/// Samplers should be created via Hgi::CreateSampler.
///
class HgiSampler
{
public:
    HGI_API
    virtual ~HgiSampler();

    /// The descriptor describes the object.
    HGI_API
    HgiSamplerDesc const& GetDescriptor() const;

    /// This function returns the handle to the Hgi backend's gpu resource, cast
    /// to a uint64_t. Clients should avoid using this function and instead
    /// use Hgi base classes so that client code works with any Hgi platform.
    /// For transitioning code to Hgi, it can however we useful to directly
    /// access a platform's internal resource handles.
    /// There is no safety provided in using this. If you by accident pass a
    /// HgiMetal resource into an OpenGL call, bad things may happen.
    /// In OpenGL this returns the GLuint resource name.
    /// In Metal this returns the id<MTLSamplerState> as uint64_t.
    /// In Vulkan this returns the VkSampler as uint64_t.
    HGI_API
    virtual uint64_t GetRawResource() const = 0;

protected:
    HGI_API
    HgiSampler(HgiSamplerDesc const& desc);

    HgiSamplerDesc _descriptor;

private:
    HgiSampler() = delete;
    HgiSampler & operator=(const HgiSampler&) = delete;
    HgiSampler(const HgiSampler&) = delete;
};

using HgiSamplerHandle = HgiHandle<HgiSampler>;
using HgiSamplerHandleVector = std::vector<HgiSamplerHandle>;


PXR_NAMESPACE_CLOSE_SCOPE

#endif
