//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiVulkan/resourceTracker.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/imaging/hgi/resourceBindings.h"
#include "pxr/imaging/hgiVulkan/buffer.h"
#include "pxr/imaging/hgiVulkan/capabilities.h"
#include "pxr/imaging/hgiVulkan/conversions.h"
#include "pxr/imaging/hgiVulkan/texture.h"

#include <algorithm>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// Can't use HgiBufferBindDesc::writable because no API user sets it correctly.
// Would be nice to fix that, but for now conservatively derive it from the
// type.
bool
_MaybeWritten(HgiBindResourceType type)
{
    switch (type) {
    case HgiBindResourceTypeSampler:
    case HgiBindResourceTypeSampledImage:
    case HgiBindResourceTypeCombinedSamplerImage:
    case HgiBindResourceTypeUniformBuffer:
        return false;
    default:
        return true;
    }
}

VkAccessFlags2
_GetAccessFlag(HgiBindResourceType type)
{
    switch (type) {
    case HgiBindResourceTypeUniformBuffer:
        return VK_ACCESS_2_UNIFORM_READ_BIT;
    case HgiBindResourceTypeSampledImage:
    case HgiBindResourceTypeCombinedSamplerImage:
        return VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
    default:
        break;
    }

    return _MaybeWritten(type) ? VK_ACCESS_2_SHADER_STORAGE_READ_BIT |
            VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT :
                                 VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
}

VkImageMemoryBarrier2
_InitImageBarrier(
    HgiVulkanTexture* texture, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier2 barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = texture->GetImage();
    barrier.subresourceRange.aspectMask =
        HgiVulkanConversions::GetImageAspectFlag(
            texture->GetDescriptor().usage);
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    return barrier;
}

// Transition the layout without knowing what the next use will be.
VkImageMemoryBarrier2
_TransitionInPlace(HgiVulkanTexture* texture, VkImageLayout layout)
{
    HgiVulkanResourceState* state = texture->GetState();

    VkImageMemoryBarrier2 barrier =
        _InitImageBarrier(texture, state->layout, layout);

    const VkPipelineStageFlags2 stages = state->writeStages | state->readStages;
    barrier.srcStageMask = stages;
    barrier.srcAccessMask = state->writeAccess;

    if (stages) {
        // No access needed: transition writes are made available implicitly,
        // and the next use's barrier makes them visible.
        barrier.dstStageMask = stages;
        state->writeStages = stages;
        state->readStages = 0;
        state->readAccess = 0;
    } else {
        // No previous access (new image): nothing to chain, so order it before
        // everything instead. Nothing to record.
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.dstAccessMask =
            VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
    }

    state->layout = layout;
    return barrier;
}

void
_EmitImageBarriers(
    VkCommandBuffer cb, std::vector<VkImageMemoryBarrier2> const& barriers)
{
    if (barriers.empty()) {
        return;
    }

    VkDependencyInfo dependencyInfo = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependencyInfo.imageMemoryBarrierCount =
        static_cast<uint32_t>(barriers.size());
    dependencyInfo.pImageMemoryBarriers = barriers.data();

    vkCmdPipelineBarrier2(cb, &dependencyInfo);
}

VkPipelineStageFlags2
_GetPipelineStages(
    HgiShaderStage stages, HgiVulkanCapabilities const* capabilities)
{
    if (!TF_VERIFY(capabilities)) {
        return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    }

    VkPhysicalDeviceFeatures const& features =
        capabilities->vkDeviceFeatures2.features;

    VkPipelineStageFlags2 result = 0;
    if (stages & HgiShaderStageVertex) {
        result |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
    }
    if (stages & HgiShaderStageFragment) {
        result |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    }
    if (stages & HgiShaderStageCompute) {
        result |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    }
    if (features.tessellationShader) {
        if (stages &
            (HgiShaderStageTessellationControl |
                HgiShaderStagePostTessellationControl)) {
            result |= VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT;
        }
        if (stages &
            (HgiShaderStageTessellationEval |
                HgiShaderStagePostTessellationVertex)) {
            result |= VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT;
        }
    }
    if (features.geometryShader && (stages & HgiShaderStageGeometry)) {
        result |= VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT;
    }

    // Conservative fallback
    return result ? result : VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
}

} // namespace

HgiVulkanResourceTracker::HgiVulkanResourceTracker(
    HgiVulkanCapabilities const* capabilities)
    : _capabilities(capabilities)
{
}

void
HgiVulkanResourceTracker::UseResourceBindings(
    HgiResourceBindingsDesc const& desc)
{
    for (HgiBufferBindDesc const& bind : desc.buffers) {
        HgiVulkanResourceUse use;
        use.stages = _GetPipelineStages(bind.stageUsage, _capabilities);
        use.access = _GetAccessFlag(bind.resourceType);
        use.write = _MaybeWritten(bind.resourceType);

        for (HgiBufferHandle const& handle : bind.buffers) {
            if (const auto buffer =
                    dynamic_cast<HgiVulkanBuffer*>(handle.Get())) {
                Use(buffer->GetState(), use);
            }
        }
    }

    for (HgiTextureBindDesc const& bind : desc.textures) {
        HgiVulkanResourceUse use;
        use.stages = _GetPipelineStages(bind.stageUsage, _capabilities);
        use.access = _GetAccessFlag(bind.resourceType);
        use.write = _MaybeWritten(bind.resourceType);

        for (HgiTextureHandle const& handle : bind.textures) {
            if (const auto texture =
                    dynamic_cast<HgiVulkanTexture*>(handle.Get())) {
                Use(texture->GetState(), use);
            }
        }
    }
}

void
HgiVulkanResourceTracker::Use(
    HgiVulkanResourceState* state, HgiVulkanResourceUse const& use)
{
    if (!state || !use.stages) {
        return;
    }

    // RAW, WAW: finish writes and make visible
    if (state->writeStages) {
        _srcStages |= state->writeStages;
        _srcAccess |= state->writeAccess;
        _dstStages |= use.stages;
        _dstAccess |= use.access;
    }

    if (use.write) {
        // WAR: just finish reads (visibility isn't needed)
        if (state->readStages) {
            _srcStages |= state->readStages;
            _dstStages |= use.stages;
        }

        state->writeStages = use.stages;
        state->writeAccess = use.access;
        state->readStages = 0;
        state->readAccess = 0;
    } else {
        // RAR: just record, no barrier needed
        state->readStages |= use.stages;
        state->readAccess |= use.access;
    }
}

void
HgiVulkanResourceTracker::UseImage(HgiVulkanTexture* texture,
    HgiVulkanResourceUse const& use, VkImageLayout layout)
{
    if (!texture || !use.stages) {
        return;
    }

    HgiVulkanResourceState* state = texture->GetState();
    if (state->layout == layout) {
        Use(state, use);
        return;
    }

    const VkImage image = texture->GetImage();
    const auto pending = std::find_if(_imageBarriers.begin(),
        _imageBarriers.end(), [image](VkImageMemoryBarrier2 const& barrier) {
            return barrier.image == image;
        });

    if (pending != _imageBarriers.end()) {
        // If there's already an existing barrier, we'll merge into it.
        // For the layout we'll have to use GENERAL.
        pending->newLayout = VK_IMAGE_LAYOUT_GENERAL;
        pending->dstStageMask |= use.stages;
        pending->dstAccessMask |= use.access;
        state->layout = VK_IMAGE_LAYOUT_GENERAL;
        state->writeStages |= use.stages;
        if (use.write) {
            state->writeAccess |= use.access;
        } else {
            state->readStages |= use.stages;
            state->readAccess |= use.access;
        }
        return;
    }

    // Layout transitions imply reading and writing the whole image, so we have
    // to wait on all previous reads and writes to finish.
    VkImageMemoryBarrier2 barrier =
        _InitImageBarrier(texture, state->layout, layout);
    barrier.srcStageMask = state->writeStages | state->readStages;
    barrier.srcAccessMask = state->writeAccess;
    barrier.dstStageMask = use.stages;
    barrier.dstAccessMask = use.access;
    _imageBarriers.push_back(barrier);

    const bool restorePending =
        std::any_of(_restores._entries.begin(), _restores._entries.end(),
            [image](PendingRestores::_Entry const& entry) {
                return entry.texture->GetImage() == image;
            });
    if (!restorePending) {
        _restores._entries.push_back({texture, state->layout});
    }

    // Later uses must also wait on the transition: record it as a write in
    // the use's stages, so they chain through them.
    state->layout = layout;
    state->writeStages = use.stages;
    state->writeAccess = use.write ? use.access : 0;
    state->readStages = use.write ? 0 : use.stages;
    state->readAccess = use.write ? 0 : use.access;
}

HgiVulkanResourceTracker::PendingRestores
HgiVulkanResourceTracker::Flush(VkCommandBuffer cb)
{
    const bool emitMemoryBarrier = (_srcStages != 0 || _dstStages != 0);
    if (!emitMemoryBarrier && _imageBarriers.empty()) {
        Discard();
        return std::move(_restores);
    }

    VkMemoryBarrier2 memoryBarrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    memoryBarrier.srcStageMask = _srcStages;
    memoryBarrier.srcAccessMask = _srcAccess;
    memoryBarrier.dstStageMask = _dstStages;
    memoryBarrier.dstAccessMask = _dstAccess;

    VkDependencyInfo dependencyInfo = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    if (emitMemoryBarrier) {
        dependencyInfo.memoryBarrierCount = 1;
        dependencyInfo.pMemoryBarriers = &memoryBarrier;
    }
    dependencyInfo.imageMemoryBarrierCount =
        static_cast<uint32_t>(_imageBarriers.size());
    dependencyInfo.pImageMemoryBarriers = _imageBarriers.data();

    vkCmdPipelineBarrier2(cb, &dependencyInfo);

    _imageBarriers.clear();
    Discard();
    return std::move(_restores);
}

void
HgiVulkanResourceTracker::FlushWithoutRestore(VkCommandBuffer cb)
{
    PendingRestores restores = Flush(cb);
    if (!restores._entries.empty()) {
        TF_CODING_ERROR("Layout transitions flushed without a restore");
        restores._entries.clear();
    }
}

HgiVulkanResourceTracker::PendingRestores::PendingRestores(
    PendingRestores&& other) noexcept
    : _entries(std::move(other._entries))
{
    other._entries.clear();
}

HgiVulkanResourceTracker::PendingRestores&
HgiVulkanResourceTracker::PendingRestores::operator=(
    PendingRestores&& other) noexcept
{
    if (this != &other) {
        if (!_entries.empty()) {
            TF_CODING_ERROR("Layout transitions overwritten before Restore()");
        }
        _entries = std::move(other._entries);
        other._entries.clear();
    }
    return *this;
}

HgiVulkanResourceTracker::PendingRestores::~PendingRestores()
{
    if (!_entries.empty()) {
        TF_CODING_ERROR("Layout transitions dropped without Restore()");
    }
}

void
HgiVulkanResourceTracker::PendingRestores::Restore(VkCommandBuffer cb)
{
    std::vector<VkImageMemoryBarrier2> barriers;
    for (_Entry const& entry : _entries) {
        if (entry.texture->GetState()->layout != entry.layout) {
            barriers.push_back(_TransitionInPlace(entry.texture, entry.layout));
        }
    }
    _entries.clear();

    _EmitImageBarriers(cb, barriers);
}

void
HgiVulkanResourceTracker::Discard()
{
    if (!_imageBarriers.empty()) {
        TF_CODING_ERROR(
            "Discarding layout transitions that were never recorded");
        _imageBarriers.clear();
    }

    _srcStages = 0;
    _srcAccess = 0;
    _dstStages = 0;
    _dstAccess = 0;
}

bool
HgiVulkanResourceTracker::IsEmpty() const
{
    return _srcStages == 0 && _dstStages == 0 && _imageBarriers.empty();
}

void
HgiVulkanResourceTracker::ChangeLayout(
    HgiVulkanTexture* texture, VkImageLayout layout, VkCommandBuffer cb)
{
    if (!texture || texture->GetState()->layout == layout) {
        return;
    }

    _EmitImageBarriers(cb, {_TransitionInPlace(texture, layout)});
}

PXR_NAMESPACE_CLOSE_SCOPE
