//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIVULKAN_SYNC_TRACKER_H
#define PXR_IMAGING_HGIVULKAN_SYNC_TRACKER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkanCapabilities;
class HgiVulkanTexture;
struct HgiResourceBindingsDesc;


/// \struct HgiVulkanResourceUse
///
/// Records a resource use by a command.
///
struct HgiVulkanResourceUse
{
    VkPipelineStageFlags2 stages = 0;
    VkAccessFlags2 access = 0;
    bool write = false;
};


/// \struct HgiVulkanResourceState
///
/// The current state of a resource, which is the combination of all its uses so
/// far.
///
struct HgiVulkanResourceState
{
    VkPipelineStageFlags2 writeStages = 0;
    VkAccessFlags2 writeAccess = 0;
    VkPipelineStageFlags2 readStages = 0;
    VkAccessFlags2 readAccess = 0;

    /// The most recent image layout. Only recorded for textures.
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
};


/// \class HgiVulkanResourceTracker
///
/// Derives the memory dependencies for a command from the resources it touches
/// and the state each was left in, and emits it as a single global barrier.
///
/// Main usage pattern is to call Use() on every resource the command needs,
/// then call Flush() to record the barrier, followed by the actual command,
/// then call Restore() on what Flush() returned:
///
/// \code
/// tracker.Use(...);
/// auto restore = tracker.Flush(cb);
/// vkCmd...(cb, ...);
/// restore.Restore(cb);
/// \endcode
///
/// If there's no image layout transition involved, then use
/// FlushWithoutRestore() to avoid having to call Restore().
///
class HgiVulkanResourceTracker final
{
public:
    /// \class PendingRestores
    ///
    /// You must call Restore() on functions that return this object, after
    /// recording the command, otherwise it's a coding error.
    ///
    class [[nodiscard]] PendingRestores final
    {
    public:
        PendingRestores() = default;

        HGIVULKAN_API
        PendingRestores(PendingRestores&& other) noexcept;

        HGIVULKAN_API
        PendingRestores& operator=(PendingRestores&& other) noexcept;

        HGIVULKAN_API
        ~PendingRestores();

        /// Apply necessary state restorations after the command.
        /// Must not be called inside a render pass.
        HGIVULKAN_API
        void Restore(VkCommandBuffer cb);

    private:
        friend class HgiVulkanResourceTracker;

        struct _Entry
        {
            HgiVulkanTexture* texture;
            VkImageLayout layout;
        };

        std::vector<_Entry> _entries;
    };

    HGIVULKAN_API
    explicit HgiVulkanResourceTracker(HgiVulkanCapabilities const* capabilities);

    /// Add a resource use against its current state, and record the necessary
    /// dependencies to this tracker.
    HGIVULKAN_API
    void Use(HgiVulkanResourceState* state, HgiVulkanResourceUse const& use);

    /// Like Use(), but also does a layout transition on an image. Layouts are
    /// restored after Flush() by PendingRestores.
    HGIVULKAN_API
    void UseImage(
        HgiVulkanTexture* texture,
        HgiVulkanResourceUse const& use,
        VkImageLayout layout);

    /// Calls Use() on each resource binding. For convenience.
    HGIVULKAN_API
    void UseResourceBindings(HgiResourceBindingsDesc const& desc);

    /// Encodes the barrier for all the Uses() accumulated so far, for the next
    /// command. You must call Restore() on the returned PendingRestores object
    /// after encoding the command.
    /// Must not be called inside a render pass.
    HGIVULKAN_API
    PendingRestores Flush(VkCommandBuffer cb);

    /// Like Flush(), when there's no image layout transition involved.
    /// Otherwise it's a coding error.
    HGIVULKAN_API
    void FlushWithoutRestore(VkCommandBuffer cb);

    /// Drop the accumulated dependencies. Layout transitions cannot be dropped,
    /// otherwise it's a coding error.
    HGIVULKAN_API
    void Discard();

    /// Returns true if no dependencies are necessary (even if uses are recorded
    /// this could still be empty when dependencies are unnecessary).
    HGIVULKAN_API
    bool IsEmpty() const;

    /// Records an image layout change. Unlike with UseImage(), this won't be
    /// restored.
    HGIVULKAN_API
    static void ChangeLayout(
        HgiVulkanTexture* texture,
        VkImageLayout layout,
        VkCommandBuffer cb);

private:
    HgiVulkanCapabilities const* _capabilities;

    VkPipelineStageFlags2 _srcStages = 0;
    VkAccessFlags2 _srcAccess = 0;
    VkPipelineStageFlags2 _dstStages = 0;
    VkAccessFlags2 _dstAccess = 0;
    std::vector<VkImageMemoryBarrier2> _imageBarriers;

    PendingRestores _restores;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
