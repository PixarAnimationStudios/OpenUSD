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
    static VkFormat GetFormat(HgiFormat inFormat);

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
    static VkComponentSwizzle GetComponentSwizzle(HgiComponentSwizzle cs);

    HGIVULKAN_API
    static VkPrimitiveTopology GetPrimitiveType(HgiPrimitiveType pt);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif

