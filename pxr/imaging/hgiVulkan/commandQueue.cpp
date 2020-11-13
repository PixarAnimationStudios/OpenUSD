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
#include "pxr/imaging/hgiVulkan/commandBuffer.h"
#include "pxr/imaging/hgiVulkan/commandQueue.h"
#include "pxr/imaging/hgiVulkan/device.h"

#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

static HgiVulkanCommandQueue::HgiVulkan_CommandPool*
_CreateCommandPool(HgiVulkanDevice* device)
{
    VkCommandPoolCreateInfo poolCreateInfo =
        {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolCreateInfo.flags =
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |           // short lived
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // reset individually

    // If Graphics and Compute were to come from different queue families we
    // would need to use a different commandpool/buffer for gfx vs compute.
    poolCreateInfo.queueFamilyIndex = device->GetGfxQueueFamilyIndex();

    VkCommandPool pool = nullptr;

    TF_VERIFY(
        vkCreateCommandPool(
            device->GetVulkanDevice(),
            &poolCreateInfo,
            HgiVulkanAllocator(),
            &pool) == VK_SUCCESS
    );

    HgiVulkanCommandQueue::HgiVulkan_CommandPool* newPool =
        new HgiVulkanCommandQueue::HgiVulkan_CommandPool();

    newPool->vkCommandPool = pool;
    return newPool;
}

static void
_DestroyCommandPool(
    HgiVulkanDevice* device,
    HgiVulkanCommandQueue::HgiVulkan_CommandPool* pool)
{
    for (HgiVulkanCommandBuffer* cb : pool->commandBuffers) {
        delete cb;
    }
    pool->commandBuffers.clear();

    vkDestroyCommandPool(
        device->GetVulkanDevice(),
        pool->vkCommandPool,
        HgiVulkanAllocator());

    pool->vkCommandPool = nullptr;
    delete pool;
}

HgiVulkanCommandQueue::HgiVulkanCommandQueue(HgiVulkanDevice* device)
    : _device(device)
    , _vkGfxQueue(nullptr)
    , _inflightBits(0)
    , _inflightCounter(0)
    , _threadId(std::this_thread::get_id())
    , _resourceCommandBuffer(nullptr)
{
    // Acquire the graphics queue
    const uint32_t firstQueueInFamily = 0;
    vkGetDeviceQueue(
        device->GetVulkanDevice(),
        device->GetGfxQueueFamilyIndex(),
        firstQueueInFamily,
        &_vkGfxQueue);
}

HgiVulkanCommandQueue::~HgiVulkanCommandQueue()
{
    for (auto const& it : _commandPools) {
        _DestroyCommandPool(_device, it.second);
    }
    _commandPools.clear();
}

/* Externally synchronized */
void
HgiVulkanCommandQueue::SubmitToQueue(
    HgiVulkanCommandBuffer* cb,
    HgiSubmitWaitType wait)
{
    VkSemaphore semaphore = nullptr;

    // If we have resource commands submit those before work commands.
    // It would be more performant to submit both command buffers to the queue
    // at the same time, but we have to signal the fence for each since we use
    // the fence to determine when a command buffer can be reused.
    if (_resourceCommandBuffer) {
        _resourceCommandBuffer->EndCommandBuffer();
        VkCommandBuffer rcb = _resourceCommandBuffer->GetVulkanCommandBuffer();
        semaphore = _resourceCommandBuffer->GetVulkanSemaphore();
        VkFence rFence = _resourceCommandBuffer->GetVulkanFence();

        VkSubmitInfo resourceInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
        resourceInfo.commandBufferCount = 1;
        resourceInfo.pCommandBuffers = &rcb;
        resourceInfo.signalSemaphoreCount = 1;
        resourceInfo.pSignalSemaphores = &semaphore;

        TF_VERIFY(
            vkQueueSubmit(_vkGfxQueue, 1, &resourceInfo, rFence) == VK_SUCCESS
        );

        _resourceCommandBuffer = nullptr;
    }

    // XXX Ideally EndCommandBuffer is called on the thread that used it since
    // this can be a heavy operation. However, currently Hgi does not provide
    // a 'EndRecording' function on its Hgi*Cmds that clients must call.
    cb->EndCommandBuffer();
    VkCommandBuffer wcb = cb->GetVulkanCommandBuffer();
    VkFence wFence = cb->GetVulkanFence();

    VkSubmitInfo workInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    workInfo.commandBufferCount = 1;
    workInfo.pCommandBuffers = &wcb;
    if (semaphore) {
        workInfo.waitSemaphoreCount = 1;
        workInfo.pWaitSemaphores = &semaphore;
        VkPipelineStageFlags waitMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        workInfo.pWaitDstStageMask = &waitMask;
    }

    // Submit provided command buffers to GPU queue.
    // Record and submission order does not guarantee execution order.
    // VK docs: "Execution Model" & "Implicit Synchronization Guarantees".
    // The vulkan queue must be externally synchronized.
    TF_VERIFY(
        vkQueueSubmit(_vkGfxQueue, 1, &workInfo, wFence) == VK_SUCCESS
    );

    // Optional blocking wait
    if (wait == HgiSubmitWaitTypeWaitUntilCompleted) {
        static const uint64_t timeOut = 100000000000;
        VkDevice vkDevice = _device->GetVulkanDevice();
        TF_VERIFY(
            vkWaitForFences(vkDevice, 1, &wFence, VK_TRUE, timeOut)==VK_SUCCESS
        );
        // When the client waits for the cmd buf to finish on GPU they will
        // expect to have the CompletedHandlers run. For example when the
        // client wants to do a GPU->CPU read back (memcpy)
        cb->RunAndClearCompletedHandlers();
    }
}

/* Multi threaded */
HgiVulkanCommandBuffer*
HgiVulkanCommandQueue::AcquireCommandBuffer()
{
    // Find the thread's command pool.
    HgiVulkan_CommandPool* pool =
        _AcquireThreadCommandPool(std::this_thread::get_id());

    // Grab one of the available command buffers.
    HgiVulkanCommandBuffer* cmdBuf = nullptr;
    for (HgiVulkanCommandBuffer* cb : pool->commandBuffers) {
        if (!cb->IsInFlight()) {
            cmdBuf = cb;
            break;
        }
    }

    // If no command buffer was available, create a new one.
    if (!cmdBuf) {
        cmdBuf = new HgiVulkanCommandBuffer(_device, pool->vkCommandPool);
        pool->commandBuffers.push_back(cmdBuf);
    }

    // Acquire an unique id for this cmd buffer amongst inflight cmd buffers.
    uint8_t inflightId = _AcquireInflightIdBit();
    _SetInflightBit(inflightId, /*enabled*/ true);

    // Begin recording to ensure the caller has exclusive access to cmd buffer.
    cmdBuf->BeginCommandBuffer(inflightId);
    return cmdBuf;
}

/* Single threaded */
HgiVulkanCommandBuffer*
HgiVulkanCommandQueue::AcquireResourceCommandBuffer()
{
    // XXX We currently have only one resource command buffer. We can get away
    // with this since Hgi::Create* must currently happen on the main thread.
    // Once we change that, we must support resource command buffers on
    // secondary threads.
    TF_VERIFY(std::this_thread::get_id() == _threadId);

    if (!_resourceCommandBuffer) {
        _resourceCommandBuffer = AcquireCommandBuffer();
    }
    return _resourceCommandBuffer;
}

/* Multi threaded */
uint64_t
HgiVulkanCommandQueue::GetInflightCommandBuffersBits()
{
    return _inflightBits.load();
}

/* Multi threaded */
VkQueue
HgiVulkanCommandQueue::GetVulkanGraphicsQueue() const
{
    return _vkGfxQueue;
}

/* Single threaded */
void
HgiVulkanCommandQueue::ResetConsumedCommandBuffers()
{
    // Lock the command pool map from concurrent access since we may insert.
    std::lock_guard<std::mutex> guard(_commandPoolsMutex);

    // Loop all pools and reset any command buffers that have been consumed.
    for (auto it : _commandPools) {
        HgiVulkan_CommandPool* pool = it.second;
        for (HgiVulkanCommandBuffer* cb : pool->commandBuffers) {
            if (cb->ResetIfConsumedByGPU()) {
                _SetInflightBit(cb->GetInflightId(), /*enabled*/ false);
            }
        }
    }
}

/* Multi threaded */
HgiVulkanCommandQueue::HgiVulkan_CommandPool*
HgiVulkanCommandQueue::_AcquireThreadCommandPool(
    std::thread::id const& threadId)
{
    // Lock the command pool map from concurrent access since we may insert.
    std::lock_guard<std::mutex> guard(_commandPoolsMutex);

    auto it = _commandPools.find(threadId);
    if (it == _commandPools.end()) {
        HgiVulkan_CommandPool* newPool = _CreateCommandPool(_device);
        _commandPools[threadId] = newPool;
        return newPool;
    } else {
        return it->second;
    }
}

/* Multi threaded */
uint8_t
HgiVulkanCommandQueue::_AcquireInflightIdBit()
{
    // Command buffers can be acquired by threads, so we need to do an
    // increment that is thread safe. We circle back to the first bit after
    // all bits have been used once. These means we can track the in-flight
    // status of up to 64 consecutive command buffer usages.
    // This becomes important in garbage collection and is explained more there.
    return _inflightCounter.fetch_add(1) % 64;
}

/* Multi threaded */
void
HgiVulkanCommandQueue::_SetInflightBit(uint8_t id, bool enabled)
{
    // We need to set the bit atomically since this function can be called by
    // multiple threads. Try to set the value and if it fails (another thread
    // may have updated the `expected` value!), we re-apply our bit and
    // try again.
    uint64_t expect = _inflightBits.load();

    if (enabled) {
        // Spin if bit was already enabled. This means we have reached our max
        // of 64 command buffers and must wait until it becomes available.
        expect &= ~(1<<id);
        while (!_inflightBits.compare_exchange_weak(
            expect, expect | (1ULL<<id))) 
        {
            expect &= ~(1<<id);
        }
    } else {
        while (!_inflightBits.compare_exchange_weak(
            expect, expect & ~(1ULL<<id)));
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
