//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/diagnostic.h"

#include "pxr/imaging/hgiVulkan/pipelineCache.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiVulkanPipelineCache::HgiVulkanPipelineCache(
    HgiVulkanDevice* device)
    : _device(device)
    , _vkPipelineCache(nullptr)
{
    // xxx we need to add a pipeline cache to avoid app having to keep compiling
    // shader micro-code for every pipeline combination. We except that the
    // spir-V shader code is not compiled for the target device until this point
    // where we create the pipeline. So a pipeline cache can be helpful.
}

HgiVulkanPipelineCache::~HgiVulkanPipelineCache()
{
}

VkPipelineCache
HgiVulkanPipelineCache::GetVulkanPipelineCache() const
{
    return _vkPipelineCache;
}


PXR_NAMESPACE_CLOSE_SCOPE
