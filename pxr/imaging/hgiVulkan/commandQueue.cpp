//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiVulkan/commandBuffer.h"
#include "pxr/imaging/hgiVulkan/commandQueue.h"
#include "pxr/imaging/hgiVulkan/device.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"

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

    HGIVULKAN_VERIFY_VK_RESULT(
        vkCreateCommandPool(
            device->GetVulkanDevice(),
            &poolCreateInfo,
            HgiVulkanAllocator(),
            &pool)
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
    , _timelineNextVal(1)
    , _timelineCachedVal(0)
{
    // Acquire the graphics queue
    const uint32_t firstQueueInFamily = 0;
    vkGetDeviceQueue(
        device->GetVulkanDevice(),
        device->GetGfxQueueFamilyIndex(),
        firstQueueInFamily,
        &_vkGfxQueue);

    VkSemaphoreTypeCreateInfo timelineCreateInfo;
    timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    timelineCreateInfo.pNext = NULL;
    timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    timelineCreateInfo.initialValue = 0;

    VkSemaphoreCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    createInfo.pNext = &timelineCreateInfo;
    createInfo.flags = 0;

    HGIVULKAN_VERIFY_VK_RESULT(
        vkCreateSemaphore(
            device->GetVulkanDevice(),
            &createInfo, 
            HgiVulkanAllocator(),
            &_timelineSemaphore));
}

HgiVulkanCommandQueue::~HgiVulkanCommandQueue()
{
    for (auto const& it : _commandPools) {
        _DestroyCommandPool(_device, it.second);
    }
    _commandPools.clear();

    vkDestroySemaphore(
        _device->GetVulkanDevice(),
        _timelineSemaphore,
        HgiVulkanAllocator());
}

/* Externally synchronized */
void
HgiVulkanCommandQueue::SubmitToQueue(
    HgiVulkanCommandBuffer* cb,
    HgiSubmitWaitType wait)
{
    // If we have resource commands buffer those before work commands.
    if (_resourceCommandBuffer) {
        _resourceCommandBuffer->EndCommandBuffer();
        _resourceCommandBuffer->SetCompletedTimelineValue(
            _timelineNextVal);
        _queuedBuffers.push_back(_resourceCommandBuffer);

        _resourceCommandBuffer = nullptr;
    }

    // XXX Ideally EndCommandBuffer is called on the thread that used it since
    // this can be a heavy operation. However, currently Hgi does not provide
    // a 'EndRecording' function on its Hgi*Cmds that clients must call.
    cb->EndCommandBuffer();
    cb->SetCompletedTimelineValue(_timelineNextVal);
    _queuedBuffers.push_back(cb);

    // Optional blocking flush and wait.
    if (wait == HgiSubmitWaitTypeWaitUntilCompleted) {
        Flush(wait);
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
        if (cb->IsReset()) {
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
    std::optional<uint8_t> inflightId = _AcquireInflightIdBit();

    // No id available: check if any command buffers are no longer in-flight,
    // and release their bit. Spin until we can acquire a bit.
    if (!inflightId) {
        do {
            // To avoid a hot loop with high CPU usage, sleep a bit.
            // We want to sleep as little as possible, but the actual
            // sleep time is system dependent. This is unfortunate and
            // will cause framerate hitches, but if we got here in the
            // first place it's because the device is overloaded and things
            // are not going well.
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            for (HgiVulkanCommandBuffer* cb : pool->commandBuffers) {
                if (cb->UpdateInFlightStatus(HgiSubmitWaitTypeNoWait) ==
                    HgiVulkanCommandBuffer::InFlightUpdateResultFinishedFlight)
                {
                    _ReleaseInflightBit(cb->GetInflightId());
                }
            }

            inflightId = _AcquireInflightIdBit();
        } while (!inflightId);
    }

    // Begin recording to ensure the caller has exclusive access to cmd buffer.
    cmdBuf->BeginCommandBuffer(*inflightId);
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
    // See _AcquireInflightIdBit for explanation of memory order.
    return _inflightBits.load(std::memory_order_relaxed);
}

/* Multi threaded */
VkQueue
HgiVulkanCommandQueue::GetVulkanGraphicsQueue() const
{
    return _vkGfxQueue;
}

/* Single threaded */
void
HgiVulkanCommandQueue::ResetConsumedCommandBuffers(HgiSubmitWaitType wait)
{
    // Lock the command pool map from concurrent access since we may insert.
    std::lock_guard<std::mutex> guard(_commandPoolsMutex);

    // Loop all pools and reset any command buffers that have been consumed.
    for (auto it : _commandPools) {
        HgiVulkan_CommandPool* pool = it.second;
        for (HgiVulkanCommandBuffer* cb : pool->commandBuffers) {
            if (cb->ResetIfConsumedByGPU(wait)) {
                _ReleaseInflightBit(cb->GetInflightId());
            }
        }
    }
}

/* Single threaded */
void
HgiVulkanCommandQueue::Flush(
    HgiSubmitWaitType wait,
    VkSemaphore signalSemaphore)
{
    std::vector<VkCommandBuffer> commandBuffers;
    commandBuffers.reserve(_queuedBuffers.size());
    for (auto& buffer : _queuedBuffers) {
        commandBuffers.push_back(buffer->GetVulkanCommandBuffer());
    }

    const VkSemaphore semaphoreSignal[2] =
        { _timelineSemaphore, signalSemaphore };
    const uint64_t semaphoreSignalValues[2] =
        { _timelineNextVal, 0 };
    const uint32_t semaphoreSignalCount = signalSemaphore ? 2 : 1;

    VkTimelineSemaphoreSubmitInfo timelineInfo;
    timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
    timelineInfo.pNext = NULL;
    timelineInfo.waitSemaphoreValueCount = 0;
    timelineInfo.pWaitSemaphoreValues = nullptr;
    timelineInfo.signalSemaphoreValueCount = semaphoreSignalCount;
    timelineInfo.pSignalSemaphoreValues = semaphoreSignalValues;

    VkSubmitInfo workInfo;
    workInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    workInfo.pNext = &timelineInfo;
    workInfo.pWaitDstStageMask = 0;
    workInfo.waitSemaphoreCount = 0;
    workInfo.pWaitSemaphores = nullptr;
    workInfo.commandBufferCount = commandBuffers.size();
    workInfo.pCommandBuffers = commandBuffers.data();
    workInfo.signalSemaphoreCount = semaphoreSignalCount;
    workInfo.pSignalSemaphores = semaphoreSignal;

    HGIVULKAN_VERIFY_VK_RESULT(
        vkQueueSubmit(_vkGfxQueue, 1, &workInfo, VK_NULL_HANDLE));
    
    if (wait == HgiSubmitWaitTypeWaitUntilCompleted) {
        VkSemaphoreWaitInfo waitInfo;
        waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        waitInfo.pNext = nullptr;
        waitInfo.flags = 0;
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &_timelineSemaphore;
        waitInfo.pValues = &_timelineNextVal;
        HGIVULKAN_VERIFY_VK_RESULT(
            vkWaitSemaphores(_device->GetVulkanDevice(),
                &waitInfo, UINT64_MAX));

        for (auto& buffer : _queuedBuffers) {
            // When the client waits for the cmd buf to finish on GPU they will
            // expect to have the CompletedHandlers run. For example when the
            // client wants to do a GPU->CPU read back (memcpy)
            buffer->RunAndClearCompletedHandlers();
        }

        _timelineCachedVal = _timelineNextVal;
    }
    _queuedBuffers.clear();

    _timelineNextVal++;
}

/* Single threaded */
bool
HgiVulkanCommandQueue::IsTimelinePastValue(uint64_t desiredValue, bool wait)
{
    if (_timelineCachedVal >= desiredValue) {
        return true;
    }
    if (_timelineNextVal == desiredValue) {
        Flush(HgiSubmitWaitTypeNoWait);
    }
    HGIVULKAN_VERIFY_VK_RESULT(
        vkGetSemaphoreCounterValue(
            _device->GetVulkanDevice(),
            _timelineSemaphore,
            &_timelineCachedVal));
    if (_timelineCachedVal >= desiredValue) {
        return true;
    }
    if (wait) {
        VkSemaphoreWaitInfo waitInfo;
        waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        waitInfo.pNext = nullptr;
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &_timelineSemaphore;
        waitInfo.pValues = &desiredValue;
        HGIVULKAN_VERIFY_VK_RESULT(
            vkWaitSemaphores(
                _device->GetVulkanDevice(),
                &waitInfo,
                UINT64_MAX));
        _timelineCachedVal = desiredValue;
        return true;
    }
    return false;
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
std::optional<uint8_t>
HgiVulkanCommandQueue::_AcquireInflightIdBit()
{
    // Command buffers can be acquired by threads, so we need to do an id
    // acquire that is thread safe. We search for the next zero bit in a
    // 64bit word. This means we can track the in-flight status of up to 64
    // consecutive command buffer usages. This becomes important in garbage
    // collection and is explained more there.
    const uint8_t nextBitIndex = 0x3F & _inflightCounter.fetch_add(1,
        std::memory_order_relaxed);
    const uint64_t previousBits =
        (static_cast<uint64_t>(1) << nextBitIndex) - 1;

    // We need to set the bit atomically since this function can be called by
    // multiple threads. Try to set the value and if it fails (another thread
    // may have updated the `expected` value!), we re-apply our bit and try
    // again. Relaxed memory order since this isn't used to order read/writes.
    // If no bits are available, then exit with nothing. The caller will try
    // to free some bits by updating the in-flight status of the existing 
    // buffers.
    uint64_t freeBit;
    uint64_t expected = _inflightBits.load(std::memory_order_relaxed);
    uint64_t desired;
    do {
        // Don't re-use lower bits if possible: mask them as used.
        // _inflightCounter will wrap around when we run out.
        const uint64_t usedBits = expected | previousBits;
        freeBit = ~usedBits & (usedBits + 1);
        if (freeBit == 0) {
            return std::nullopt;
        }

        expected &= ~freeBit;
        desired = expected | freeBit;
    } while (!_inflightBits.compare_exchange_weak(expected, desired,
        std::memory_order_relaxed));

    // Based on: https://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightParallel
    // C++20: use std::countr_zero instead
    uint8_t id = 63;
    if (freeBit & 0x00000000FFFFFFFF) id -= 32;
    if (freeBit & 0x0000FFFF0000FFFF) id -= 16;
    if (freeBit & 0x00FF00FF00FF00FF) id -= 8;
    if (freeBit & 0x0F0F0F0F0F0F0F0F) id -= 4;
    if (freeBit & 0x3333333333333333) id -= 2;
    if (freeBit & 0x5555555555555555) id -= 1;

    return id;
}

/* Multi threaded */
void
HgiVulkanCommandQueue::_ReleaseInflightBit(uint8_t id)
{
    // We need to set the bit atomically since this function can be called by
    // multiple threads. Try to set the value and if it fails (another thread
    // may have updated the `expected` value!), we re-apply our bit and try
    // again. Relaxed memory order since this isn't used to order read/writes.
    uint64_t expected = _inflightBits.load(std::memory_order_relaxed);
    uint64_t desired;
    do {
        desired = expected & ~(1ULL << id);
    } while (!_inflightBits.compare_exchange_weak( expected, desired,
        std::memory_order_relaxed));
}

PXR_NAMESPACE_CLOSE_SCOPE
