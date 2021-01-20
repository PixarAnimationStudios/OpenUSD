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
#ifndef PXR_IMAGING_HGI_VULKAN_PIPELINE_CACHE_H
#define PXR_IMAGING_HGI_VULKAN_PIPELINE_CACHE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"


PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkanDevice;

/// \class HgiVulkanPipelineCache
///
/// Wrapper for Vulkan pipeline cache.
///
class HgiVulkanPipelineCache final
{
public:
    HGIVULKAN_API
    HgiVulkanPipelineCache(HgiVulkanDevice* device);

    HGIVULKAN_API
    ~HgiVulkanPipelineCache();

    /// Returns the vulkan pipeline cache.
    HGIVULKAN_API
    VkPipelineCache GetVulkanPipelineCache() const;

private:
    HgiVulkanPipelineCache() = delete;
    HgiVulkanPipelineCache & operator=(const HgiVulkanPipelineCache&) = delete;
    HgiVulkanPipelineCache(const HgiVulkanPipelineCache&) = delete;

    HgiVulkanDevice* _device;
    VkPipelineCache _vkPipelineCache;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
