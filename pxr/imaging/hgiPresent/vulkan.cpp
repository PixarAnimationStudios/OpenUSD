//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiPresent/vulkan.h"

#include "pxr/base/gf/range1d.h"

#include "pxr/imaging/hgiVulkan/conversions.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"
#include "pxr/imaging/hgiVulkan/hgi.h"
#include "pxr/imaging/hgiVulkan/instance.h"
#include "pxr/imaging/hgiVulkan/texture.h"

#include "pxr/imaging/hgiPresent/debugCodes.h"

#include <iostream>

// This code uses the Vulkan APIs directly since it only works with HgiVulkan.
// It doesn't really makes much sense to use the Hgi abstraction if it's always
// HgiVulkan: it just adds overhead. Unless we go the full mile abstracting
// over surfaces, swapchains and other presentation concerns, using the Hgi
// layer doesn't have any real benefits. Implementing it would be a lot of
// additional work to replace not that many lines of API specific code.

namespace {
PXR_NAMESPACE_USING_DIRECTIVE

TfToken const &
_VkColorSpaceEnumToName(VkColorSpaceKHR colorSpace)
{
    switch (colorSpace) {
    case VK_COLOR_SPACE_BT709_LINEAR_EXT:
    case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT:
        return GfColorSpaceNames->LinearRec709;
    case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
    case VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT:
        return GfColorSpaceNames->SRGBRec709;
    case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT:
        return GfColorSpaceNames->SRGBP3D65;
    case VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT:
        return GfColorSpaceNames->LinearDisplayP3;
    case VK_COLOR_SPACE_BT709_NONLINEAR_EXT:
        // Neither a gamma of 1.8 nor 2.2 are good approximations
        // for the true BT.709 transfer functions. Actually the
        // in between value of 2.0 is a much better approximation,
        // see: https://registry.khronos.org/DataFormat/specs/1.3/dataformat.1.3.html#TRANSFER_ITU_INVOETF
        // We'll settle for 2.2 since that seems like the most common.
        return GfColorSpaceNames->G22Rec709;
    case VK_COLOR_SPACE_BT2020_LINEAR_EXT:
        return GfColorSpaceNames->LinearRec2020;
    case VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT:
        return GfColorSpaceNames->LinearAdobeRGB;
    case VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT:
        return GfColorSpaceNames->G22AdobeRGB;
    case VK_COLOR_SPACE_PASS_THROUGH_EXT:
        return GfColorSpaceNames->Raw;
    default:
        return GfColorSpaceNames->Unknown;
    }
}

bool
_FormatConvertsColorSpace(VkFormat format, const TfToken &srcColorSpace,
    const TfToken &dstColorSpace)
{
    switch (format) {
    case VK_FORMAT_R8G8B8_SRGB:
    case VK_FORMAT_B8G8R8_SRGB:
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_B8G8R8A8_SRGB:
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
        return srcColorSpace == GfColorSpaceNames->LinearRec709 &&
            dstColorSpace == GfColorSpaceNames->SRGBRec709;
    default:
        return srcColorSpace == dstColorSpace;
    }
}

/// Properties of the color volume that is defined by a format.
/// These are used to compare different color spaces against a
/// preferred one and pick the closest match.
struct _ColorVolume
{
    // Min and max value for a color component.
    // These are assumed to all be the same.
    GfRange1d axisRange;
    // Difference between 1 and the next representable
    // value of a color component. An absolute reference
    // value is used because floating point formats have
    // value relative resolution. For formats with varying
    // resolution per component, the smallest color one is used.
    double epsilon;
    // The number of dimension, either 3 or 4 (+1 for alpha).
    uint32_t dimensions;

    explicit operator bool() const
    {
        return !axisRange.IsEmpty();
    }

    // The result of matching this color volume against another one.
    struct Match
    {
        // Precentage of the range covered by the other color volume.
        double coverage;
        // How many bits of data are loss by the other color volume.
        float dataLoss;
        // How many bits of data are wasted by the other color volume.
        float dataSlack;
        // How many extra dimensions the other color volume has.
        uint32_t extraDimensions;

        // Allow for total ordering of color volume matches by comparing
        // the match results, breaking ties according to importance of the
        // match property: prefer wasting data over losing it.
        bool operator<(const Match &other) const
        {
            if (coverage != other.coverage) {
                return coverage > other.coverage;
            }
            if (dataLoss != other.dataLoss) {
                return dataLoss < other.dataLoss;
            }
            if (dataSlack != other.dataSlack) {
                return dataSlack < other.dataSlack;
            }
            if (extraDimensions != other.extraDimensions) {
                return extraDimensions < other.extraDimensions;
            }
            return false;
        }
    };

    // Match this color volume against another one, so they can be ranked.
    Match
    ComputeMatch(const _ColorVolume &other) const
    {
        Match score{};
        score.coverage = GfRange1d::Intersection(axisRange, other.axisRange).
            GetSize() / axisRange.GetSize();
        // Equivalent to log2(other.epsilon) - other.epsilon(epsilon)
        const auto dataDiff = static_cast<float>(std::log2(
            other.epsilon / epsilon));
        score.dataLoss = std::max(dataDiff, 0.f);
        score.dataSlack = std::max(-dataDiff, 0.f);
        score.extraDimensions = other.dimensions - dimensions;
        return score;
    }
};

/// Get the color volume that is defined by a format.
_ColorVolume
_GetColorVolume(VkFormat format)
{
    constexpr auto makeNormVolume = [](bool signed_, uint32_t bits,
        uint32_t dimensions) {
        return _ColorVolume{{signed_ ? -1. : 0., 1.},
            (signed_ ? 2. : 1.) / (1 << bits), dimensions};
    };

    // The Vulkan format list is very long, but most formats don't make sense
    // for presentation. While the API says almost nothing about what can be
    // supported, we can make some reasonable assumptions and apply some of our
    // own criteria. We ignore:
    //   - Compressed formats
    //   - Extension formats
    //   - Integer and scaled format
    //   - Any format that isn't RGB or RGBA
    //   - 64 bit formats
    // We could probably also ignore signed normalized formats, but color
    // spaces with negative values are a thing, so we opt to keep them.
    switch (format) {
    case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
    case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
        return makeNormVolume(false, 4, 4);
    case VK_FORMAT_R5G6B5_UNORM_PACK16:
    case VK_FORMAT_B5G6R5_UNORM_PACK16:
        return makeNormVolume(false, 5, 3);
    case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
    case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
    case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
        return makeNormVolume(false, 5, 4);
    case VK_FORMAT_R8G8B8_UNORM:
    case VK_FORMAT_R8G8B8_SRGB:
    case VK_FORMAT_B8G8R8_UNORM:
    case VK_FORMAT_B8G8R8_SRGB:
        return makeNormVolume(false, 8, 3);
    case VK_FORMAT_R8G8B8_SNORM:
    case VK_FORMAT_B8G8R8_SNORM:
        return makeNormVolume(true, 8, 3);
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_SRGB:
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
        return makeNormVolume(false, 8, 4);
    case VK_FORMAT_R8G8B8A8_SNORM:
    case VK_FORMAT_B8G8R8A8_SNORM:
    case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
        return makeNormVolume(true, 8, 4);
    case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        return makeNormVolume(false, 10, 4);
    case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
    case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
        return makeNormVolume(true, 10, 4);
    case VK_FORMAT_R16G16B16_UNORM:
        return makeNormVolume(false, 16, 3);
    case VK_FORMAT_R16G16B16_SNORM:
        return makeNormVolume(true, 16, 3);
    case VK_FORMAT_R16G16B16_SFLOAT:
        return {{-PXR_HALF_MAX, PXR_HALF_MAX}, PXR_HALF_EPSILON, 3};
    case VK_FORMAT_R16G16B16A16_UNORM:
        return makeNormVolume(false, 16, 4);
    case VK_FORMAT_R16G16B16A16_SNORM:
        return makeNormVolume(true, 16, 4);
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return {{-PXR_HALF_MAX, PXR_HALF_MAX}, PXR_HALF_EPSILON, 4};
    case VK_FORMAT_R32G32B32_SFLOAT:
        return {
            {-std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max()},
            std::numeric_limits<float>::epsilon(), 3};
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return {
            {-std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max()},
            std::numeric_limits<float>::epsilon(), 4};
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
        // 5 bit exponent, 5 bit mantissa: 2^exponent − 15 * (1 + mantissa / 32)
        return {{0., (1 << 15) * (1 + 31. / 32)}, 1. / 32, 3};
    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
        // 5 bit exponent, 9 bit mantissa: 2^exponent − 15 * (1 + mantissa / 512)
        return {{0., (1 << 15) * (1 + 511. / 512)}, 1. / 512, 3};
    default:
        return {};
    }
}

// Set common properties and call vkCmdPipelineBarrier
void
_AddImageMemoryBarrier(VkCommandBuffer commandBuffer,
    VkImageMemoryBarrier barrierInfo, VkPipelineStageFlags src,
    VkPipelineStageFlags dst)
{
    if (barrierInfo.oldLayout == barrierInfo.newLayout) {
        return;
    }

    barrierInfo.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrierInfo.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrierInfo.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrierInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrierInfo.subresourceRange.levelCount = 1;
    barrierInfo.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(commandBuffer, src, dst, 0, 0, nullptr, 0, nullptr, 1,
        &barrierInfo);
}
}


PXR_NAMESPACE_OPEN_SCOPE


HgiPresentWindowVulkan::HgiPresentWindowVulkan(HgiVulkan *hgi,
    HgiPresentWindowParams const &params)
    : HgiPresentImpl(hgi)
    , _hgiVulkan{hgi}
    , _params(params)
{
    if (!_hgiVulkan) {
        TF_CODING_ERROR("hgi must be a valid HgiVulkan pointer");
        return;
    }

    const auto hgiDevice = _hgiVulkan->GetPrimaryDevice();
    if (!hgiDevice->SupportsPresentation()) {
        TF_WARN("Vulkan presentation is unsupported: presentation is disabled");
        return;
    }

    const auto instance = _hgiVulkan->GetVulkanInstance()->GetVulkanInstance();
    const auto device = hgiDevice->GetVulkanDevice();

    _surface = std::visit([instance](const auto &window) {
        using Window = std::decay_t<decltype(window)>;
        VkSurfaceKHR surface{};
#if defined(VK_USE_PLATFORM_METAL_EXT)
        if constexpr (std::is_same_v<Window, HgiPresentMetalWindowHandle>) {
            VkMetalSurfaceCreateInfoEXT surfaceCreateInfo{};
            surfaceCreateInfo.sType =
                VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
            surfaceCreateInfo.pLayer = window.layer;
            HGIVULKAN_VERIFY_VK_RESULT(
                vkCreateMetalSurfaceEXT(instance, &surfaceCreateInfo,
                    HgiVulkanAllocator(), &surface));
        } else
#endif
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        if constexpr (std::is_same_v<Window, HgiPresentWin32WindowHandle>) {
            VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{};
            surfaceCreateInfo.sType =
                VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            surfaceCreateInfo.hinstance = window.instance;
            surfaceCreateInfo.hwnd = window.window;
            HGIVULKAN_VERIFY_VK_RESULT(vkCreateWin32SurfaceKHR(instance,
                &surfaceCreateInfo, HgiVulkanAllocator(), &surface));
        } else
#endif
#if defined(VK_USE_PLATFORM_XLIB_KHR)
        if constexpr (std::is_same_v<Window, HgiPresentXlibWindowHandle>) {
            VkXlibSurfaceCreateInfoKHR createSurfaceInfo{};
            createSurfaceInfo.sType =
                VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
            createSurfaceInfo.dpy = window.display;
            createSurfaceInfo.window = window.window;
            HGIVULKAN_VERIFY_VK_RESULT(vkCreateXlibSurfaceKHR(instance,
                &createSurfaceInfo, HgiVulkanAllocator(), &surface));
        } else
#endif
        {
            static_assert(std::is_same_v<Window, HgiPresentNullWindowHandle>);
        }
        return surface;
    }, _params.window);

    if (!_surface) {
        TF_WARN(
            "Vulkan surface could not be created: presentation is disabled");
        return;
    }

    VkCommandPoolCreateInfo commandPoolCreateInfo{};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.queueFamilyIndex = hgiDevice->
        GetGfxQueueFamilyIndex();
    commandPoolCreateInfo.flags =
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(device, &commandPoolCreateInfo, HgiVulkanAllocator(),
        &_commandPool);

    VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
    commandBufferAllocateInfo.sType =
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandPool = _commandPool;
    commandBufferAllocateInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(device, &commandBufferAllocateInfo,
        &_presentCommandBuffer);

    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    HGIVULKAN_VERIFY_VK_RESULT(
        vkCreateSemaphore(device, &semaphoreCreateInfo, HgiVulkanAllocator(),
            &_imageAcquiredSemaphore));
    HGIVULKAN_VERIFY_VK_RESULT(
        vkCreateSemaphore(device, &semaphoreCreateInfo, HgiVulkanAllocator(),
            &_blitCompleteSemaphore));
}

HgiPresentWindowVulkan::~HgiPresentWindowVulkan()
{
    const auto hgiDevice = _hgiVulkan->GetPrimaryDevice();
    const auto device = hgiDevice->GetVulkanDevice();

    _DestroySurfaceResources(_hgiVulkan->GetVulkanInstance()->
        GetVulkanInstance(), device);
    // Destroying the pool frees the present command buffer
    vkDestroyCommandPool(device, _commandPool, HgiVulkanAllocator());
    vkDestroySemaphore(device, _imageAcquiredSemaphore, HgiVulkanAllocator());
    vkDestroySemaphore(device, _blitCompleteSemaphore, HgiVulkanAllocator());
}

void HgiPresentWindowVulkan::_DestroySurfaceResources(VkInstance instance,
    VkDevice device)
{
    _DestroySwapchainResources(device);

    vkDestroySurfaceKHR(instance, _surface, HgiVulkanAllocator());
    _surface = nullptr;
    _validSurfaceProperties = false;
}

void
HgiPresentWindowVulkan::_ResolveSurfaceProperties(VkPhysicalDevice physicalDevice)
{
    if (!_surface || _validSurfaceProperties) {
        return;
    }

    VkSurfaceCapabilitiesKHR surfaceCapabilities{};
    HGIVULKAN_VERIFY_VK_RESULT(
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, _surface, &
            surfaceCapabilities));

    // 0 means unlimited (until you run out of memory).
    // We don't really need more than 2.
    const auto maxImageCount = surfaceCapabilities.maxImageCount == 0 ?
        std::max(2u, surfaceCapabilities.minImageCount) :
        surfaceCapabilities.maxImageCount;
    _imageCount = std::clamp(2u, surfaceCapabilities.minImageCount,
        maxImageCount);

    if (!(surfaceCapabilities.supportedUsageFlags &
        VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
        // According to the Vulkan capability database,
        // 100% of drivers support this so we should never get here.
        TF_WARN("Vulkan surface blit is unsupported: presentation is disabled");
        return;
    }

    if (surfaceCapabilities.supportedCompositeAlpha &
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) {
        // Defer to the window if we can. This bit is probably only ever set
        // alone according to the spec's wording:
        //  The way in which the presentation engine treats the alpha component
        //  in the images is unknown to the Vulkan API. Instead, the application
        //  is responsible for setting the composite alpha blending mode using
        //  native window system commands.
        _alphaComposition = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    } else if (surfaceCapabilities.supportedCompositeAlpha &
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
        _alphaComposition = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    } else {
        TF_WARN(
            "Vulkan opaque or inherit alpha composition unsupported: presentation is disabled");
        return;
    }

    _presentMode = VK_PRESENT_MODE_FIFO_KHR;
    if (!_params.wantVsync) {
        uint32_t presentModeCount{};
        HGIVULKAN_VERIFY_VK_RESULT(
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, _surface,
                & presentModeCount, nullptr));
        std::vector<VkPresentModeKHR> presentModes{presentModeCount};
        HGIVULKAN_VERIFY_VK_RESULT(
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, _surface,
                & presentModeCount, presentModes.data()));
        if (std::find(presentModes.begin(), presentModes.end(),
            VK_PRESENT_MODE_IMMEDIATE_KHR) != presentModes.end()) {
            _presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }

    if (!(surfaceCapabilities.supportedTransforms &
        VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)) {
        // According to the Vulkan capability database,
        // 100% of drivers support this so we should never get here.
        TF_WARN(
            "Vulkan surface transform is unsupported: presentation is disabled");
        return;
    }

    // Note: while it's possible to get the surface size from
    // surfaceCapabilities.currentExtent, it's not guaranteed to be
    // available, so we still need to rely on the window size coming
    // from the present task.
    _swapchainExtent.width = std::clamp(static_cast<uint32_t>(_srcTextureSize[0]),
        surfaceCapabilities.minImageExtent.width,
        surfaceCapabilities.maxImageExtent.width);
    _swapchainExtent.height = std::clamp(static_cast<uint32_t>(_srcTextureSize[1]),
        surfaceCapabilities.minImageExtent.height,
        surfaceCapabilities.maxImageExtent.height);

    const auto minComponents = HgiGetComponentCount(
        _params.preferredSurfaceFormat);
    const auto preferredColorVolume = _GetColorVolume(
        HgiVulkanConversions::GetFormat(_params.preferredSurfaceFormat));

    uint32_t formatCount{};
    HGIVULKAN_VERIFY_VK_RESULT(
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, _surface, &
            formatCount, nullptr));
    std::vector<VkSurfaceFormatKHR> formats{formatCount};
    HGIVULKAN_VERIFY_VK_RESULT(
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, _surface, &
            formatCount, formats.data()));

    using CandidateFormat = std::pair<VkSurfaceFormatKHR, _ColorVolume::Match>;
    std::vector<CandidateFormat> candidateFormats;
    for (const auto formatAndColorSpace : formats) {
        const auto [format, colorSpace] = formatAndColorSpace;
        if (_VkColorSpaceEnumToName(colorSpace) != _params.surfaceColorSpace) {
            continue;
        }

        const auto colorVolume = _GetColorVolume(format);
        if (!colorVolume || colorVolume.dimensions < minComponents || !
            _FormatConvertsColorSpace(format, _params.srcColorSpace,
                _params.surfaceColorSpace)) {
            continue;
        }

        candidateFormats.emplace_back(formatAndColorSpace,
            preferredColorVolume.ComputeMatch(colorVolume));
    }

    if (candidateFormats.empty()) {
        TF_WARN(
            "No compatible Vulkan surface format found: presentation is disabled");
        return;
    }

    // We'll assume that the driver will return formats in some order of
    // preference, so use stable sorting to preserve it when matches are tied.
    std::stable_sort(candidateFormats.begin(), candidateFormats.end(),
        [](const CandidateFormat &a, const CandidateFormat &b) {
            return a.second < b.second;
        });

    if (TfDebug::IsEnabled(HGIPRESENT_DUMP_CANDIDATE_SURFACE_FORMATS)) {
        std::cout << "Candidate surface formats (first won):\n";
        for (const auto &[format, match] : candidateFormats) {
            std::cout << "    " << format.format <<
                ", " << format.colorSpace << "\n";
        }
        std::cout << std::endl;
    }

    _imageFormat = candidateFormats.front().first;

    _validSurfaceProperties = true;
}

void
HgiPresentWindowVulkan::_CreateSwapchainResources(VkDevice device)
{
    if (!_surface || _validSwapchainResources) {
        return;
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = _surface;
    swapchainCreateInfo.minImageCount = _imageCount;
    swapchainCreateInfo.imageFormat = _imageFormat.format;
    swapchainCreateInfo.imageColorSpace = _imageFormat.colorSpace;
    swapchainCreateInfo.imageExtent = _swapchainExtent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainCreateInfo.compositeAlpha = _alphaComposition;
    swapchainCreateInfo.presentMode = _presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;

    HGIVULKAN_VERIFY_VK_RESULT(vkCreateSwapchainKHR(device,
        &swapchainCreateInfo, HgiVulkanAllocator(), &_swapchain));

    if (!_swapchain) {
        return;
    }

    uint32_t imageCount{};
    HGIVULKAN_VERIFY_VK_RESULT(
        vkGetSwapchainImagesKHR(device, _swapchain, &imageCount, nullptr));
    _swapchainImages.resize(imageCount);
    HGIVULKAN_VERIFY_VK_RESULT(
        vkGetSwapchainImagesKHR(device, _swapchain, &imageCount,
            _swapchainImages.data()));

    _validSwapchainResources = !_swapchainImages.empty() &&
        std::all_of(_swapchainImages.begin(), _swapchainImages.end(),
            [](VkImage image) {
                return image != nullptr;
            });
}

void
HgiPresentWindowVulkan::_DestroySwapchainResources(VkDevice device)
{
    // Destroying the swapchain destroys its images
    vkDestroySwapchainKHR(device, _swapchain, HgiVulkanAllocator());
    _swapchainImages.clear();
    _swapchain = nullptr;

    _validSwapchainResources = false;
}

bool
HgiPresentWindowVulkan::IsFormatSupported(HgiFormat colorFormat) const
{
    return HgiIsFloatFormat(colorFormat) && !HgiIsCompressed(colorFormat);
}

bool
HgiPresentWindowVulkan::IsValid() const
{
    return _surface && _validSurfaceProperties && _validSwapchainResources;
}

void
HgiPresentWindowVulkan::Present(HgiTextureHandle const &hgiSrcTexture,
    HgiTextureHandle const &)
{
    const auto hgiDevice = _hgiVulkan->GetPrimaryDevice();
    const auto device = hgiDevice->GetVulkanDevice();

    const auto srcTextureDesc = hgiSrcTexture->GetDescriptor();
    const GfVec2i srcTextureSize{srcTextureDesc.dimensions[0],
        srcTextureDesc.dimensions[1]};

    if (srcTextureSize != _srcTextureSize) {
        _srcTextureSize = srcTextureSize;
        _validSurfaceProperties = false;
        _DestroySwapchainResources(device);
    }

    _ResolveSurfaceProperties(hgiDevice->GetVulkanPhysicalDevice());
    _CreateSwapchainResources(device);
    if (!_validSurfaceProperties || !_validSwapchainResources) {
        return;
    }

    const auto srcTexture = dynamic_cast<HgiVulkanTexture *>(hgiSrcTexture.
        Get());
    if (!srcTexture) {
        TF_CODING_ERROR("srcColor must be a valid HgiVulkan pointer");
        return;
    }

    if (const auto result = vkAcquireNextImageKHR(device, _swapchain,
        std::numeric_limits<uint64_t>::max(), _imageAcquiredSemaphore,
        nullptr, &_swapchainImageIndex); result == VK_ERROR_OUT_OF_DATE_KHR) {
        _validSurfaceProperties = false;
        _DestroySwapchainResources(device);
        return;
    } else if (result == VK_ERROR_SURFACE_LOST_KHR) {
        _DestroySurfaceResources(_hgiVulkan->GetVulkanInstance()->
            GetVulkanInstance(), device);
    } else if (result != VK_SUBOPTIMAL_KHR) {
        HGIVULKAN_VERIFY_VK_RESULT(result);
    }

    const auto hgiQueue = hgiDevice->GetCommandQueue();
    const auto queue = hgiQueue->GetVulkanGraphicsQueue();

    VkCommandBufferBeginInfo commandBufferBeginInfo{};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(_presentCommandBuffer, &commandBufferBeginInfo);

    VkImageMemoryBarrier barrierInfo{};
    barrierInfo.image = srcTexture->GetImage();
    barrierInfo.oldLayout = srcTexture->GetImageLayout();
    barrierInfo.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrierInfo.srcAccessMask = 0;
    barrierInfo.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    _AddImageMemoryBarrier(_presentCommandBuffer, barrierInfo,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

    barrierInfo.image = _swapchainImages[_swapchainImageIndex];
    barrierInfo.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrierInfo.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrierInfo.srcAccessMask = 0;
    barrierInfo.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    _AddImageMemoryBarrier(_presentCommandBuffer, barrierInfo,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

    VkImageBlit imageBlit{};
    imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBlit.srcSubresource.layerCount = 1;
    imageBlit.srcOffsets[0].x = 0;
    imageBlit.srcOffsets[1].x = srcTextureSize[0];
    // Y range is swapped to vertically flip the image.
    imageBlit.srcOffsets[0].y = srcTextureSize[1];
    imageBlit.srcOffsets[1].y = 0;
    imageBlit.srcOffsets[1].z = 1;
    imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBlit.dstSubresource.layerCount = 1;
    imageBlit.dstOffsets[0].x = 0;
    imageBlit.dstOffsets[1].x = static_cast<int32_t>(_swapchainExtent.width);
    imageBlit.dstOffsets[0].y = 0;
    imageBlit.dstOffsets[1].y = static_cast<int32_t>(_swapchainExtent.height);
    imageBlit.dstOffsets[1].z = 1;

    // We blit instead of using a potentially faster copy for two main reasons:
    //   1. We have to apply a vertical flip due to the Storm rendering
    //      coordinate system. Copy can't do that for us. The flip would need
    //      to have been done earlier in the pipeline. But as it currently
    //      stands we have no idea what the image contents are, so we assume
    //      the standard Storm coordinates.
    //   2. We have no guarantee that the image format will exactly match the
    //      surface format. In most probability we'll need to do a conversion,
    //      and blit is the best option for that. If it weren't for the need
    //      to flip we could use a copy when formats are matching.
    vkCmdBlitImage(_presentCommandBuffer, srcTexture->GetImage(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        _swapchainImages[_swapchainImageIndex],
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

    barrierInfo.image = srcTexture->GetImage();
    barrierInfo.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrierInfo.newLayout = srcTexture->GetImageLayout();
    barrierInfo.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrierInfo.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    _AddImageMemoryBarrier(_presentCommandBuffer, barrierInfo,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

    barrierInfo.image = _swapchainImages[_swapchainImageIndex];
    barrierInfo.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrierInfo.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrierInfo.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrierInfo.dstAccessMask = 0;
    _AddImageMemoryBarrier(_presentCommandBuffer, barrierInfo,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

    vkEndCommandBuffer(_presentCommandBuffer);

    const VkPipelineStageFlags waitBit{VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_presentCommandBuffer;
    submitInfo.pSignalSemaphores = &_blitCompleteSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &_imageAcquiredSemaphore;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitDstStageMask = &waitBit;

    HGIVULKAN_VERIFY_VK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, nullptr));

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &_blitCompleteSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &_swapchain;
    presentInfo.pImageIndices = &_swapchainImageIndex;

    if (const auto result = vkQueuePresentKHR(queue, &presentInfo); result ==
        VK_ERROR_OUT_OF_DATE_KHR) {
        _validSurfaceProperties = false;
        _DestroySwapchainResources(device);
    } else if (result == VK_ERROR_SURFACE_LOST_KHR) {
        _DestroySurfaceResources(_hgiVulkan->GetVulkanInstance()->
            GetVulkanInstance(), device);
    }  else if (result != VK_SUBOPTIMAL_KHR) {
        HGIVULKAN_VERIFY_VK_RESULT(result);
    }

    // We need to force a sync here because we don't have the synchronization
    // mechanism to prevent the AOV from being reused before presentation is
    // finished.
    HGIVULKAN_VERIFY_VK_RESULT(vkQueueWaitIdle(queue));
}


PXR_NAMESPACE_CLOSE_SCOPE
