//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIPRESENT_VULKAN_H
#define PXR_IMAGING_HGIPRESENT_VULKAN_H

#include "pxr/pxr.h"

#include "pxr/base/tf/smallVector.h"

#include "pxr/imaging/hgiVulkan/vulkan.h"

#include "pxr/imaging/hgiPresent/presentImpl.h"


PXR_NAMESPACE_OPEN_SCOPE


class HgiVulkan;

/// \class HgiPresentWindowVulkan
///
/// Present to a Vulkan Window using the Vulkan WSI extensions.
///
class HgiPresentWindowVulkan final: public HgiPresentImpl
{
public:
    explicit HgiPresentWindowVulkan(HgiVulkan* hgi,
        HgiPresentWindowParams const &params);

    ~HgiPresentWindowVulkan() override;

    bool IsFormatSupported(HgiFormat colorFormat) const override;

    bool IsValid() const override;

    void Present(
        HgiTextureHandle const &hgiSrcTexture,
        HgiTextureHandle const &srcDepth) override;

private:
    void _DestroySurfaceResources(VkInstance instance,
        VkDevice device);

    void _ResolveSurfaceProperties(VkPhysicalDevice physicalDevice);

    void _CreateSwapchainResources(VkDevice device);

    void _DestroySwapchainResources(VkDevice device);

    HgiVulkan* _hgiVulkan{};
    HgiPresentWindowParams _params{};

    VkSurfaceKHR _surface{};

    VkCommandPool _commandPool{};
    VkCommandBuffer _presentCommandBuffer{};
    VkSemaphore _imageAcquiredSemaphore{};
    VkSemaphore _blitCompleteSemaphore{};

    GfVec2i _srcTextureSize{};

    uint32_t _imageCount{};
    VkCompositeAlphaFlagBitsKHR _alphaComposition{};
    VkPresentModeKHR _presentMode{};
    VkExtent2D _swapchainExtent{};
    VkSurfaceFormatKHR _imageFormat{};
    bool _validSurfaceProperties{false};

    VkSwapchainKHR _swapchain{};
    TfSmallVector<VkImage, 2> _swapchainImages;
    bool _validSwapchainResources{false};

    uint32_t _swapchainImageIndex{};
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
