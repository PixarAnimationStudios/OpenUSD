//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_SAMPLER_H
#define PXR_IMAGING_HGI_SAMPLER_H

#include "pxr/pxr.h"

#include "pxr/base/tf/envSetting.h"

#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/handle.h"
#include "pxr/imaging/hgi/types.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// Sets the maximum anisotropic filtering ratio for all samplers.
/// By default this is 16x. The actual value used depends on the
/// device limits. A value of 1 effectively disables anisotropic sampling.
///
HGI_API
extern TfEnvSetting<int> HGI_MAX_ANISOTROPY;

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
/// <li>borderColor: 
///    The border color for clamped texture values.</li>
/// <li>enableCompare: 
///    Enables sampler comparison against a reference value during lookups.</li>
/// <li>compareFunction: 
///    The comparison function to apply if sampler compare is enabled.</li>
/// <li>maxAnisotropy:
///    Maximum anisotropic filtering ratio. The default value of 16 corresponds
///    to the previously internal default value. The actual value used is subject
///    to the device maximum supported anisotropy and the HGI_MAX_ANISOTROPY
///    setting. A value of 1 effectively disables anisotropic sampling.</li>
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
        , borderColor(HgiBorderColorTransparentBlack)
        , enableCompare(false)
        , compareFunction(HgiCompareFunctionNever)
        , maxAnisotropy(16)
    {}

    std::string debugName;
    HgiSamplerFilter magFilter;
    HgiSamplerFilter minFilter;
    HgiMipFilter mipFilter;
    HgiSamplerAddressMode addressModeU;
    HgiSamplerAddressMode addressModeV;
    HgiSamplerAddressMode addressModeW;
    HgiBorderColor borderColor;
    bool enableCompare;
    HgiCompareFunction compareFunction;
    uint32_t maxAnisotropy;
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
