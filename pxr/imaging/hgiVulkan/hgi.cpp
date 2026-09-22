//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgi/debugCodes.h"
#include "pxr/imaging/hgiVulkan/blitCmds.h"
#include "pxr/imaging/hgiVulkan/buffer.h"
#include "pxr/imaging/hgiVulkan/capabilities.h"
#include "pxr/imaging/hgiVulkan/commandBuffer.h"
#include "pxr/imaging/hgiVulkan/commandQueue.h"
#include "pxr/imaging/hgiVulkan/computeCmds.h"
#include "pxr/imaging/hgiVulkan/computePipeline.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"
#include "pxr/imaging/hgiVulkan/garbageCollector.h"
#include "pxr/imaging/hgiVulkan/graphicsCmds.h"
#include "pxr/imaging/hgiVulkan/graphicsPipeline.h"
#include "pxr/imaging/hgiVulkan/hgi.h"
#include "pxr/imaging/hgiVulkan/instance.h"
#include "pxr/imaging/hgiVulkan/resourceBindings.h"
#include "pxr/imaging/hgiVulkan/sampler.h"
#include "pxr/imaging/hgiVulkan/shaderFunction.h"
#include "pxr/imaging/hgiVulkan/shaderProgram.h"
#include "pxr/imaging/hgiVulkan/texture.h"

#include "pxr/base/trace/trace.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"
#include "pxr/imaging/hgiVulkan/debugCodes.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    TfType t = TfType::Define<HgiVulkan, TfType::Bases<Hgi> >();
    t.SetFactory<HgiFactory<HgiVulkan>>();
}

HgiVulkan::HgiVulkan()
    : _instance(new HgiVulkanInstance())
    , _device(new HgiVulkanDevice(_instance))
    , _garbageCollector(new HgiVulkanGarbageCollector(this))
    , _threadId(std::this_thread::get_id())
    , _frameDepth(0)
{
}

HgiVulkan::~HgiVulkan()
{
    // Before the device idles and goes: an arena owns Vulkan objects, and its
    // teardown wants a live device to destroy them through.
    _DestroyExternalBufferArenas();

    if (HgiVulkanCommandQueue* queue = _device->GetCommandQueue()) {
        // Wait for command buffers to complete, then reset command buffers for
        // each device's queue.
        queue->ResetConsumedCommandBuffers(
            HgiSubmitWaitTypeWaitUntilCompleted);

        // Wait for all devices and perform final garbage collection.
        _device->WaitForIdle();
        _garbageCollector->PerformGarbageCollection(_device);
    }

    delete _garbageCollector;
    delete _device;
    delete _instance;
}

bool
HgiVulkan::IsBackendSupported() const
{
    // Check if we at least found a usable device.
    if (!_device->GetVulkanDevice()) {
        return false;
    }

    // Want Vulkan 1.2 or higher.
    const uint32_t apiVersion = GetCapabilities()->GetAPIVersion();
    const uint32_t majorVersion = VK_VERSION_MAJOR(apiVersion);
    const uint32_t minorVersion = VK_VERSION_MINOR(apiVersion);

    bool support = (majorVersion > 1) ||
        ((majorVersion == 1) && (minorVersion >= 2));
    if (!support) {
        TF_DEBUG(HGI_DEBUG_IS_SUPPORTED).Msg(
            "HgiVulkan unsupported due to Vulkan API version: %d.%d "
            "(must be >= 1.2)\n",
            majorVersion, minorVersion);
    }
    return support;
}

/* Multi threaded */
HgiGraphicsCmdsUniquePtr
HgiVulkan::CreateGraphicsCmds(
    HgiGraphicsCmdsDesc const& desc)
{
    HgiVulkanGraphicsCmds* cmds(new HgiVulkanGraphicsCmds(this, desc));
    return HgiGraphicsCmdsUniquePtr(cmds);
}

/* Multi threaded */
HgiBlitCmdsUniquePtr
HgiVulkan::CreateBlitCmds()
{
    return HgiBlitCmdsUniquePtr(new HgiVulkanBlitCmds(this));
}

HgiComputeCmdsUniquePtr
HgiVulkan::CreateComputeCmds(
    HgiComputeCmdsDesc const& desc)
{
    HgiVulkanComputeCmds* cmds(new HgiVulkanComputeCmds(this, desc));
    return HgiComputeCmdsUniquePtr(cmds);
}

/* Multi threaded */
HgiTextureHandle
HgiVulkan::_CreateTexture(HgiTextureDesc const & desc)
{
    return HgiTextureHandle(
        new HgiVulkanTexture(this, desc,
            /*optimalTiling=*/ true, /*interop=*/false), GetUniqueId());
}

/* Multi threaded */
HgiTextureHandle
HgiVulkan::CreateTextureForInterop(
    HgiTextureDesc const & desc,
    bool optimalTiling)
{
    return HgiTextureHandle(
        new HgiVulkanTexture(this, desc,
            optimalTiling, /*interop=*/true), GetUniqueId());
}

/* Multi threaded */
void
HgiVulkan::DestroyTexture(HgiTextureHandle* texHandle)
{
    TrashObject(texHandle, GetGarbageCollector()->GetTextureList());
}

/* Multi threaded */
HgiTextureViewHandle
HgiVulkan::_CreateTextureView(HgiTextureViewDesc const & desc)
{
    HgiTextureHandle src = HgiTextureHandle(
        new HgiVulkanTexture(this, desc), GetUniqueId());
    HgiTextureView* view = new HgiTextureView(desc);
    view->SetViewTexture(src);
    return HgiTextureViewHandle(view, GetUniqueId());
}

void
HgiVulkan::DestroyTextureView(HgiTextureViewHandle* viewHandle)
{
    // Trash the texture inside the view and invalidate the view handle.
    HgiTextureHandle texHandle = (*viewHandle)->GetViewTexture();
    TrashObject(&texHandle, GetGarbageCollector()->GetTextureList());
    (*viewHandle)->SetViewTexture(HgiTextureHandle());
    delete viewHandle->Get();
    *viewHandle = HgiTextureViewHandle();
}

/* Multi threaded */
HgiSamplerHandle
HgiVulkan::CreateSampler(HgiSamplerDesc const & desc)
{
    return HgiSamplerHandle(
        new HgiVulkanSampler(GetPrimaryDevice(), desc),
        GetUniqueId());
}

/* Multi threaded */
void
HgiVulkan::DestroySampler(HgiSamplerHandle* smpHandle)
{
    TrashObject(smpHandle, GetGarbageCollector()->GetSamplerList());
}

/* Multi threaded */
HgiBufferHandle
HgiVulkan::_CreateBuffer(HgiBufferDesc const & desc)
{
    return HgiBufferHandle(
        new HgiVulkanBuffer(this, desc),
        GetUniqueId());
}

/* Multi threaded */
void
HgiVulkan::DestroyBuffer(HgiBufferHandle* bufHandle)
{
    TrashObject(bufHandle, GetGarbageCollector()->GetBufferList());
}

/* Multi threaded */
HgiShaderFunctionHandle
HgiVulkan::CreateShaderFunction(HgiShaderFunctionDesc const& desc)
{
    return HgiShaderFunctionHandle(
        new HgiVulkanShaderFunction(GetPrimaryDevice(), this, desc,
        GetCapabilities()->GetShaderVersion()), GetUniqueId());
}

/* Multi threaded */
void
HgiVulkan::DestroyShaderFunction(HgiShaderFunctionHandle* shaderFnHandle)
{
    TrashObject(shaderFnHandle, GetGarbageCollector()->GetShaderFunctionList());
}

/* Multi threaded */
HgiShaderProgramHandle
HgiVulkan::CreateShaderProgram(HgiShaderProgramDesc const& desc)
{
    return HgiShaderProgramHandle(
        new HgiVulkanShaderProgram(GetPrimaryDevice(), desc),
        GetUniqueId());
}

/* Multi threaded */
void
HgiVulkan::DestroyShaderProgram(HgiShaderProgramHandle* shaderPrgHandle)
{
    TrashObject(shaderPrgHandle, GetGarbageCollector()->GetShaderProgramList());
}

/* Multi threaded */
HgiResourceBindingsHandle
HgiVulkan::_CreateResourceBindings(HgiResourceBindingsDesc const& desc)
{
    return HgiResourceBindingsHandle(
        new HgiVulkanResourceBindings(GetPrimaryDevice(), desc),
        GetUniqueId());
}

/* Multi threaded */
void
HgiVulkan::DestroyResourceBindings(HgiResourceBindingsHandle* resHandle)
{
    TrashObject(resHandle, GetGarbageCollector()->GetResourceBindingsList());
}

HgiGraphicsPipelineHandle
HgiVulkan::CreateGraphicsPipeline(HgiGraphicsPipelineDesc const& desc)
{
    return HgiGraphicsPipelineHandle(
        new HgiVulkanGraphicsPipeline(GetPrimaryDevice(), desc),
        GetUniqueId());
}

void
HgiVulkan::DestroyGraphicsPipeline(HgiGraphicsPipelineHandle* pipeHandle)
{
    TrashObject(pipeHandle, GetGarbageCollector()->GetGraphicsPipelineList());
}

HgiComputePipelineHandle
HgiVulkan::CreateComputePipeline(HgiComputePipelineDesc const& desc)
{
    return HgiComputePipelineHandle(
        new HgiVulkanComputePipeline(GetPrimaryDevice(), desc),
        GetUniqueId());
}

void
HgiVulkan::DestroyComputePipeline(HgiComputePipelineHandle* pipeHandle)
{
    TrashObject(pipeHandle, GetGarbageCollector()->GetComputePipelineList());
}

/* Multi threaded */
TfToken const&
HgiVulkan::GetAPIName() const {
    return HgiTokens->Vulkan;
}

/* Multi threaded */
HgiVulkanCapabilities const*
HgiVulkan::GetCapabilities() const
{
    return &_device->GetDeviceCapabilities();
}


HgiIndirectCommandEncoder*
HgiVulkan::GetIndirectCommandEncoder() const
{
    return nullptr;
}

/* Single threaded */
void
HgiVulkan::StartFrame()
{
    // Please read important usage limitations for Hgi::StartFrame

    if (_frameDepth++ == 0) {
        HgiVulkanBeginQueueLabel(GetPrimaryDevice(), "Full Hydra Frame");

        // Order the application's writes ahead of everything this frame is
        // about to do that reads a shared buffer. The wait rides the next
        // submission on this queue, which is the frame's own work.
        _EncodeExternalBufferAppDoneWaits();
    }
}

/* Single threaded */
void
HgiVulkan::EndFrame()
{
    // Please read important usage limitations for Hgi::EndFrame

    if (--_frameDepth == 0) {
        // Tell the application this frame has finished reading its shared
        // buffers. Before _EndFrameSync(), for two reasons: that call resets
        // consumed command buffers, and the flush inside this sweep acquires
        // one and submits it, which belongs to the frame now ending; and
        // _EndFrameSync() also reclaims external buffers, which must not
        // happen before the signal that releases them.
        _EncodeExternalBufferHgiDoneSignals();

        _EndFrameSync();
        HgiVulkanEndQueueLabel(GetPrimaryDevice());
    }
}

void
HgiVulkan::GarbageCollect()
{
    if (ARCH_UNLIKELY(_threadId != std::this_thread::get_id())) {
        TF_CODING_ERROR("Secondary thread violation");
        return;
    }
    HgiVulkanDevice* device = GetPrimaryDevice();

    // Advance what "retired" means before asking anything whether it has.
    // Whether an object may be deleted is decided by which command buffers are
    // still in flight, and those bits are only cleared here -- so without this
    // GarbageCollect() cannot free a single thing, its own trashed objects
    // included. That matters because EndFrame is optional: a client driving
    // Hydra with its own task list may never call it (Storm's own unit test
    // driver does not), and would then accumulate every deferred deletion for
    // the lifetime of the device.
    //
    // Safe here: this is main-thread-only, which GarbageCollect already
    // requires, and the default is non-blocking -- it reclaims only command
    // buffers the GPU has already finished with.
    device->GetCommandQueue()->ResetConsumedCommandBuffers();

    // Perform garbage collection for each device.
    _garbageCollector->PerformGarbageCollection(device);

    // External buffers nobody references any more, once the GPU has retired
    // the work that named them.
    _GarbageCollectExternalBufferArenas();
}

/* Multi threaded */
HgiVulkanInstance*
HgiVulkan::GetVulkanInstance() const
{
    return _instance;
}

/* Multi threaded */
HgiVulkanDevice*
HgiVulkan::GetPrimaryDevice() const
{
    return _device;
}

/* Multi threaded */
HgiVulkanGarbageCollector*
HgiVulkan::GetGarbageCollector() const
{
    return _garbageCollector;
}

/* Single threaded */
bool
HgiVulkan::_SubmitCmds(HgiCmds* cmds, HgiSubmitWaitType wait)
{
    TRACE_FUNCTION();

    // XXX The device queue is externally synchronized so we would at minimum
    // need a mutex here to ensure only one thread submits cmds at a time.
    // However, since we currently call garbage collection here and because
    // we only have one resource command buffer, we cannot support submitting
    // cmds from secondary threads until those issues are resolved.
    if (ARCH_UNLIKELY(_threadId != std::this_thread::get_id())) {
        TF_CODING_ERROR("Secondary threads should not submit cmds");
        return false;
    }

    // Submit Cmds work
    bool result = false;
    if (cmds) {
        result = Hgi::_SubmitCmds(cmds, wait);
    }

    // XXX If client does not call StartFrame / EndFrame we perform end of frame
    // cleanup after each SubmitCmds. This is more frequent than ideal and also
    // prevents us from making SubmitCmds thread-safe.
    if (_frameDepth==0) {
        _EndFrameSync();
    }

    return result;
}

/* Single threaded */
void
HgiVulkan::_FlushSemaphoreSignals()
{
    HgiVulkanDevice* device = GetPrimaryDevice();
    HgiVulkanCommandQueue* queue = device->GetCommandQueue();

    // Record a full barrier ahead of the signal.
    //
    // Without it this is a submission carrying a semaphore signal and no
    // commands, and nothing then orders that signal after the draws submitted
    // earlier: submission order is not itself an execution dependency in
    // Vulkan, so the signal could fire while the draws are still reading the
    // shared buffer -- the very hazard the signal exists to prevent, moved
    // rather than fixed.
    //
    // A pipeline barrier is what supplies the dependency. Its first
    // synchronization scope is every command submitted earlier in submission
    // order on this queue, INCLUDING earlier submissions, so the barrier
    // happens-after the draws; the signal then happens-after the barrier
    // because it belongs to this submission. ALL_COMMANDS on both sides
    // because the reads we are ordering against are whatever the frame drew.
    //
    // It passed on one NVIDIA driver without this, which is not evidence it
    // was correct -- only that that driver happened to schedule the way we
    // hoped.
    HgiVulkanCommandBuffer* cb = queue->AcquireResourceCommandBuffer();
    VkMemoryBarrier barrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask =
        VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    barrier.dstAccessMask =
        VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    vkCmdPipelineBarrier(
        cb->GetVulkanCommandBuffer(),
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0,
        1, &barrier,
        0, nullptr,
        0, nullptr);

    // Flush() is what actually calls vkQueueSubmit. It appends the resource
    // command buffer we just recorded into, and consumes the pending signal
    // list, so the barrier and the signal ride the same submission.
    queue->Flush(HgiSubmitWaitTypeNoWait);
}

void
HgiVulkan::_EndFrameSync()
{
    // The garbage collector and command buffer reset must happen on the
    // main-thread when no threads are recording.
    if (ARCH_UNLIKELY(_threadId != std::this_thread::get_id())) {
        TF_CODING_ERROR("Secondary thread violation");
        return;
    }

    HgiVulkanDevice* device = GetPrimaryDevice();
    HgiVulkanCommandQueue* queue = device->GetCommandQueue();

    // Reset command buffers for each device's queue.
    queue->ResetConsumedCommandBuffers();

    // Perform garbage collection for each device.
    _garbageCollector->PerformGarbageCollection(device);

    // External buffers retire on the same predicate as every other Vulkan
    // resource -- which command buffers are still in flight -- so they have to
    // be swept where those bits have just been cleared. Sweeping only from
    // GarbageCollect() would mean nothing reclaims them in normal operation:
    // Storm calls that just once, from its resource registry's destructor.
    _GarbageCollectExternalBufferArenas();

    if (TfDebug::IsEnabled(HGIVULKAN_DUMP_VMA_STATS)) {
        TfDebug::Disable(HGIVULKAN_DUMP_VMA_STATS);
        device->DumpMemoryStats();
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
