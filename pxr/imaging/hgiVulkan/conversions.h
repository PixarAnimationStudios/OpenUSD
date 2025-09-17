//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIVULKAN_CONVERSIONS_H
#define PXR_IMAGING_HGIVULKAN_CONVERSIONS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/types.h"

#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HgiVulkanConversions
///
/// Converts from Hgi types to Vulkan types.
///
class HgiVulkanConversions final
{
public:
    HGIVULKAN_API
    static VkFormat GetFormat(HgiFormat inFormat, bool depthFormat = false);

    HGIVULKAN_API
    static HgiFormat GetFormat(VkFormat inFormat);

    HGIVULKAN_API
    static VkImageAspectFlags GetImageAspectFlag(HgiTextureUsage usage);

    HGIVULKAN_API
    static VkImageUsageFlags GetTextureUsage(HgiTextureUsage tu);

    HGIVULKAN_API
    static VkFormatFeatureFlags GetFormatFeature(HgiTextureUsage tu);

    HGIVULKAN_API
    static VkAttachmentLoadOp GetLoadOp(HgiAttachmentLoadOp op);

    HGIVULKAN_API
    static VkAttachmentStoreOp GetStoreOp(HgiAttachmentStoreOp op);

    HGIVULKAN_API
    static VkSampleCountFlagBits GetSampleCount(HgiSampleCount sc);

    HGIVULKAN_API
    static VkShaderStageFlags GetShaderStages(HgiShaderStage ss);

    HGIVULKAN_API
    static VkBufferUsageFlags GetBufferUsage(HgiBufferUsage bu);

    HGIVULKAN_API
    static VkCullModeFlags GetCullMode(HgiCullMode cm);

    HGIVULKAN_API
    static VkPolygonMode GetPolygonMode(HgiPolygonMode pm);

    HGIVULKAN_API
    static VkFrontFace GetWinding(HgiWinding wd);

    HGIVULKAN_API
    static VkDescriptorType GetDescriptorType(HgiBindResourceType rt);

    HGIVULKAN_API
    static VkBlendFactor GetBlendFactor(HgiBlendFactor bf);

    HGIVULKAN_API
    static VkBlendOp GetBlendEquation(HgiBlendOp bo);

    HGIVULKAN_API
    static VkCompareOp GetDepthCompareFunction(HgiCompareFunction cf);

    HGIVULKAN_API
    static VkImageType GetTextureType(HgiTextureType tt);

    HGIVULKAN_API
    static VkImageViewType GetTextureViewType(HgiTextureType tt);

    HGIVULKAN_API
    static VkSamplerAddressMode GetSamplerAddressMode(HgiSamplerAddressMode a);

    HGIVULKAN_API
    static VkFilter GetMinMagFilter(HgiSamplerFilter mf);

    HGIVULKAN_API
    static VkSamplerMipmapMode GetMipFilter(HgiMipFilter mf);
    
    HGIVULKAN_API
    static VkBorderColor GetBorderColor(HgiBorderColor bc);

    HGIVULKAN_API
    static VkComponentSwizzle GetComponentSwizzle(HgiComponentSwizzle cs);

    HGIVULKAN_API
    static VkPrimitiveTopology GetPrimitiveType(HgiPrimitiveType pt);

    HGIVULKAN_API
    static std::string GetImageLayoutFormatQualifier(HgiFormat inFormat);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif

