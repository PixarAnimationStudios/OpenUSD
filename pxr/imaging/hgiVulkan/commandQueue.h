//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIVULKAN_COMMAND_QUEUE_H
#define PXR_IMAGING_HGIVULKAN_COMMAND_QUEUE_H

#include "pxr/pxr.h"

#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"

#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>
#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkanCommandBuffer;
class HgiVulkanDevice;

/// \class HgiVulkanCommandQueue
///
/// The CommandQueue manages command buffers and their submission to the
/// GPU device queue.
///
class HgiVulkanCommandQueue final
{
public:
    // Holds one thread's command pool and list of command buffers .
    struct HgiVulkan_CommandPool
    {
        VkCommandPool vkCommandPool = nullptr;
        std::vector<HgiVulkanCommandBuffer*> commandBuffers;
    };

    using CommandPoolPtrMap =
        std::unordered_map<std::thread::id, HgiVulkan_CommandPool*>;

    /// Construct a new queue for the provided device.
    HGIVULKAN_API
    HgiVulkanCommandQueue(HgiVulkanDevice* device);

    HGIVULKAN_API
    ~HgiVulkanCommandQueue();

    /// Commits the provided command buffer to GPU queue for processing.
    /// After submission the command buffer must not be re-used by client.
    /// Thread safety: Submission must be externally synchronized. Clients
    /// should call HgiVulkan::SubmitToQueue.
    HGIVULKAN_API
    void SubmitToQueue(
        HgiVulkanCommandBuffer* cmdBuffer,
        HgiSubmitWaitType wait = HgiSubmitWaitTypeNoWait);

    /// Returns a command buffer that is ready to record commands.
    /// The ownership of the command buffer (ptr) remains with this queue. The
    /// caller should not delete it. Instead, submit it back to this queue
    /// when command recording into the buffer has finished.
    /// Thread safety: The returned command buffer may only be used by the
    /// calling thread. Calls to acquire a command buffer are thread safe.
    HGIVULKAN_API
    HgiVulkanCommandBuffer* AcquireCommandBuffer();

    /// Returns a resource command buffer that is ready to record commands.
    /// The ownership of the command buffer (ptr) remains with this queue. The
    /// caller should not delete or submit it. Resource command buffers are
    /// automatically submitted before regular command buffers.
    /// Thread safety: XXX Not thread safe. This call may only happen on the
    /// main-thread and only that thread may use this command buffer.
    HGIVULKAN_API
    HgiVulkanCommandBuffer* AcquireResourceCommandBuffer();

    /// Returns a bit key that holds the in-flight status of all cmd buffers.
    /// This is used for garbage collection to delay destruction of objects
    /// until the currently in-flight command buffers have been consumed.
    /// Thread safety: This call is thread safe.
    HGIVULKAN_API
    uint64_t GetInflightCommandBuffersBits();

    /// Returns the vulkan graphics queue.
    /// Thread safety: This call is thread safe.
    HGIVULKAN_API
    VkQueue GetVulkanGraphicsQueue() const;

    /// Loop all pools and reset any command buffers that have been consumed.
    /// Thread safety: This call is not thread safe. This function should be
    /// called once from main thread while no other threads are recording.
    HGIVULKAN_API
    void ResetConsumedCommandBuffers(
        HgiSubmitWaitType wait = HgiSubmitWaitTypeNoWait);

private:
    HgiVulkanCommandQueue() = delete;
    HgiVulkanCommandQueue & operator=(const HgiVulkanCommandQueue&) = delete;
    HgiVulkanCommandQueue(const HgiVulkanCommandQueue&) = delete;

    // Returns the command pool for a thread.
    // Thread safety: This call is thread safe.
    HgiVulkan_CommandPool* _AcquireThreadCommandPool(
        std::thread::id const& threadId);

    // Returns an id-bit that uniquely identifies the cmd buffer amongst all
    // in-flight cmd buffers. Returns an empty result if all bits have been
    // acquired, in which case the existing buffers must have their bit released
    // if no longer in flight.
    // Thread safety: This call is thread safe..
    std::optional<uint8_t> _AcquireInflightIdBit();

    // Set a command buffer as not in-flight.
    // Thread safety: This call is thread safe.
    void _ReleaseInflightBit(uint8_t inflightId);

    HgiVulkanDevice* _device;
    VkQueue _vkGfxQueue;
    CommandPoolPtrMap _commandPools;
    std::mutex _commandPoolsMutex;

    std::atomic<uint64_t> _inflightBits;
    std::atomic<uint8_t> _inflightCounter;

    std::thread::id _threadId;
    HgiVulkanCommandBuffer* _resourceCommandBuffer;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
