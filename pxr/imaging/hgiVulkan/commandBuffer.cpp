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
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"
#include "pxr/base/tf/diagnostic.h"
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


HgiVulkanCommandBuffer::HgiVulkanCommandBuffer(
    HgiVulkanDevice* device,
    VkCommandPool pool)
    : _device(device)
    , _vkCommandPool(pool)
    , _vkCommandBuffer(nullptr)
    , _vkFence(nullptr)
    , _vkSemaphore(nullptr)
    , _isInFlight(false)
    , _isSubmitted(false)
    , _inflightId(0)
{
    VkDevice vkDevice = _device->GetVulkanDevice();

    // Create vulkan command buffer
    VkCommandBufferAllocateInfo allocInfo =
        {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandBufferCount = 1;
    allocInfo.commandPool = pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    TF_VERIFY(
        vkAllocateCommandBuffers(
            vkDevice,
            &allocInfo,
            &_vkCommandBuffer) == VK_SUCCESS
    );

    // Assign a debug label to command buffer
    uint64_t cmdBufHandle = (uint64_t)_vkCommandBuffer;
    std::string handleStr = std::to_string(cmdBufHandle);
    std::string cmdBufLbl = "HgiVulkan Command Buffer " + handleStr;

    HgiVulkanSetDebugName(
        _device,
        cmdBufHandle,
        VK_OBJECT_TYPE_COMMAND_BUFFER,
        cmdBufLbl.c_str());

    // CPU synchronization fence. So we known when the cmd buffer can be reused.
    VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceInfo.flags = 0; // Unsignaled starting state

    TF_VERIFY(
        vkCreateFence(
            vkDevice,
            &fenceInfo,
            HgiVulkanAllocator(),
            &_vkFence) == VK_SUCCESS
    );

    // Create semaphore for GPU-GPU synchronization
    VkSemaphoreCreateInfo semaCreateInfo =
        {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    TF_VERIFY(
        vkCreateSemaphore(
            vkDevice,
            &semaCreateInfo,
            HgiVulkanAllocator(),
            &_vkSemaphore) == VK_SUCCESS
    );

    // Assign a debug label to fence.
    std::string fenceLbl = "HgiVulkan Fence for Command Buffer: " + handleStr;
    HgiVulkanSetDebugName(
        _device,
        (uint64_t)_vkFence,
        VK_OBJECT_TYPE_FENCE,
        fenceLbl.c_str());
}

HgiVulkanCommandBuffer::~HgiVulkanCommandBuffer()
{
    VkDevice vkDevice = _device->GetVulkanDevice();
    vkDestroySemaphore(vkDevice, _vkSemaphore, HgiVulkanAllocator());
    vkDestroyFence(vkDevice, _vkFence, HgiVulkanAllocator());
    vkFreeCommandBuffers(vkDevice, _vkCommandPool, 1, &_vkCommandBuffer);
}

void
HgiVulkanCommandBuffer::BeginCommandBuffer(uint8_t inflightId)
{
    if (!_isInFlight) {

        VkCommandBufferBeginInfo beginInfo =
            {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        TF_VERIFY(
            vkBeginCommandBuffer(_vkCommandBuffer, &beginInfo) == VK_SUCCESS
        );

        _inflightId = inflightId;
        _isInFlight = true;
    }
}

bool
HgiVulkanCommandBuffer::IsInFlight() const
{
    return _isInFlight;
}

void
HgiVulkanCommandBuffer::EndCommandBuffer()
{
    if (_isInFlight) {
        TF_VERIFY(
            vkEndCommandBuffer(_vkCommandBuffer) == VK_SUCCESS
        );

        _isSubmitted = true;
    }
}

bool
HgiVulkanCommandBuffer::ResetIfConsumedByGPU()
{
    // Command buffer is already available (previously reset).
    // We do not have to test the fence or reset the cmd buffer.
    if (!_isInFlight) {
        return false;
    }

    // The command buffer is still recording. We should not test its fence until
    // we have submitted the command buffer to the queue (vulkan requirement).
    if (!_isSubmitted) {
        return false;
    }

    VkDevice vkDevice = _device->GetVulkanDevice();

    // Check the fence to see if the GPU has consumed the command buffer.
    // We cannnot reuse a command buffer until the GPU is finished with it.
    if (vkGetFenceStatus(vkDevice, _vkFence) == VK_NOT_READY){
        return false;
    }

    // GPU is done with command buffer, execute the custom fns the client wants
    // to see executed when cmd buf is consumed.
    RunAndClearCompletedHandlers();

    // GPU is done with command buffer, reset fence and command buffer.
    TF_VERIFY(
        vkResetFences(vkDevice, 1, &_vkFence)  == VK_SUCCESS
    );

    // It might be more efficient to reset the cmd pool instead of individual
    // command buffers. But we may not have a clear 'StartFrame' / 'EndFrame'
    // sequence in Hydra. If we did, we could reset the command pool(s) during
    // Beginframe. Instead we choose to reset each command buffer when it has
    // been consumed by the GPU.

    VkCommandBufferResetFlags flags = _GetCommandBufferResetFlags();
    TF_VERIFY(
        vkResetCommandBuffer(_vkCommandBuffer, flags) == VK_SUCCESS
    );

    // Command buffer may now be reused for new recordings / resource creation.
    _isInFlight = false;
    _isSubmitted = false;
    return true;
}

VkCommandBuffer
HgiVulkanCommandBuffer::GetVulkanCommandBuffer() const
{
    return _vkCommandBuffer;
}

VkCommandPool
HgiVulkanCommandBuffer::GetVulkanCommandPool() const
{
    return _vkCommandPool;
}

VkFence
HgiVulkanCommandBuffer::GetVulkanFence() const
{
    return _vkFence;
}

VkSemaphore
HgiVulkanCommandBuffer::GetVulkanSemaphore() const
{
    return _vkSemaphore;
}

uint8_t
HgiVulkanCommandBuffer::GetInflightId() const
{
    return _inflightId;
}

HgiVulkanDevice*
HgiVulkanCommandBuffer::GetDevice() const
{
    return _device;
}

void
HgiVulkanCommandBuffer::MemoryBarrier(HgiMemoryBarrier barrier)
{
    if (!_vkCommandBuffer) {
        return;
    }

    VkMemoryBarrier memoryBarrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};

    // XXX Flush / stall and invalidate all caches (big hammer!).
    // Ideally we would set more fine-grained barriers, but we
    // currently do not get enough information from Hgi to
    // know what src or dst access there is or what images or
    // buffers might be affected.
    TF_VERIFY(barrier==HgiMemoryBarrierAll, "Unsupported barrier");

    // Who might be generating the data we are interested in reading.
    memoryBarrier.srcAccessMask =
        VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

    // Who might be consuming the data that was writen.
    memoryBarrier.dstAccessMask =
        VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

    vkCmdPipelineBarrier(
        _vkCommandBuffer,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, // producer (what we wait for)
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, // consumer (what must wait)
        0,                                  // flags
        1,                                  // memoryBarrierCount
        &memoryBarrier,                     // memory barriers
        0, nullptr,                         // buffer barriers
        0, nullptr);                        // image barriers
}

void
HgiVulkanCommandBuffer::AddCompletedHandler(HgiVulkanCompletedHandler const& fn)
{
    _completedHandlers.push_back(fn);
}

void
HgiVulkanCommandBuffer::RunAndClearCompletedHandlers()
{
    for (HgiVulkanCompletedHandler& fn : _completedHandlers) {
        fn();
    }
    _completedHandlers.clear();
}

VkCommandBufferResetFlags
HgiVulkanCommandBuffer::_GetCommandBufferResetFlags()
{
    // For now we do not use VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT,
    // assuming similar memory requirements will be needed each frame.
    // Releasing resources can come at a performance cost.
    static const VkCommandBufferResetFlags flags = 0;
    return flags;
}

PXR_NAMESPACE_CLOSE_SCOPE
