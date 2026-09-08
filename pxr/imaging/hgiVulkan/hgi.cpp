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

#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    TfType t = TfType::Define<HgiVulkan, TfType::Bases<Hgi> >();
    t.SetFactory<HgiFactory<HgiVulkan>>();
}

// The only external handle type this platform's VK_EXTERNAL_MEMORY_HANDLE_AUTO
// resolves to; imports of any other type are rejected rather than reinterpreted.
static HgiExternalHandleType
_PlatformExternalHandleType()
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    return HgiExternalHandleTypeOpaqueWin32;
#else
    return HgiExternalHandleTypeOpaqueFd;
#endif
}

// Hands out the process-unique ids GetLogicalDeviceId reports. Starts at 1 so
// no live device is ever numbered 0, which callers read as "unknown".
static uint64_t
_AllocateLogicalDeviceId()
{
    static std::atomic<uint64_t> counter{1};
    return counter.fetch_add(1, std::memory_order_relaxed);
}

HgiVulkan::HgiVulkan()
    : _instance(new HgiVulkanInstance())
    , _device(new HgiVulkanDevice(_instance))
    , _garbageCollector(new HgiVulkanGarbageCollector(this))
    , _threadId(std::this_thread::get_id())
    , _logicalDeviceId(_AllocateLogicalDeviceId())
    , _frameDepth(0)
{
}

HgiVulkan::~HgiVulkan()
{
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

HgiBufferHandle
HgiVulkan::CreateExternalBuffer(
    uint64_t rawHandle, size_t byteSize, HgiBufferUsage usage)
{
    // Adopt an externally-owned VkBuffer non-owningly so Storm can bind it.
    VkBuffer vkBuffer =
        reinterpret_cast<VkBuffer>(static_cast<uintptr_t>(rawHandle));
    return HgiBufferHandle(
        new HgiVulkanBuffer(this, vkBuffer, byteSize, usage),
        GetUniqueId());
}

HgiBufferHandle
HgiVulkan::CreateInteropBuffer(
    size_t byteSize, HgiBufferUsage usage, HgiInteropBufferInfo* outInfo)
{
    HgiBufferDesc desc;
    desc.byteSize = byteSize;
    desc.usage = usage;
    desc.debugName = "ExtGpuInteropBuffer";

    auto* buf = new HgiVulkanBuffer(this, desc, /*interop=*/true);

    if (outInfo) {
        *outInfo = HgiInteropBufferInfo();

        HgiVulkanDevice* device = GetPrimaryDevice();
        VmaAllocationInfo2 ai2 = {};
        vmaGetAllocationInfo2(
            device->GetVulkanMemoryAllocator(),
            buf->GetVulkanMemoryAllocation(),
            &ai2);

        outInfo->memoryBlockSize = ai2.blockSize;
        outInfo->memoryOffset = ai2.allocationInfo.offset;
        outInfo->dedicated = ai2.dedicatedMemory;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        outInfo->externalHandle = static_cast<uint64_t>(
            reinterpret_cast<uintptr_t>(device->GetWin32HandleForMemory(
                ai2.allocationInfo.deviceMemory)));
#endif
    }

    return HgiBufferHandle(buf, GetUniqueId());
}

HgiBufferHandle
HgiVulkan::CreateBufferFromExternalMemory(
    HgiExternalMemoryBufferDesc const& desc)
{
    if (!GetPrimaryDevice()->GetDeviceCapabilities().supportsNativeInterop) {
        return HgiBufferHandle();
    }
    if (desc.handleType != _PlatformExternalHandleType()) {
        TF_WARN("HgiVulkan cannot import external handle type %d on this "
                "platform", static_cast<int>(desc.handleType));
        return HgiBufferHandle();
    }
    if (!desc.externalHandle || !desc.byteSize || !desc.memoryBlockSize) {
        TF_WARN("Incomplete HgiExternalMemoryBufferDesc: handle=%llu "
                "byteSize=%zu memoryBlockSize=%zu",
                static_cast<unsigned long long>(desc.externalHandle),
                desc.byteSize, desc.memoryBlockSize);
        return HgiBufferHandle();
    }

    auto* buf = new HgiVulkanBuffer(this, desc);
    if (!buf->GetVulkanBuffer()) {
        delete buf;
        return HgiBufferHandle();
    }
    return HgiBufferHandle(buf, GetUniqueId());
}

uint64_t
HgiVulkan::CreateExternalSemaphore(uint64_t* outExternalHandle)
{
    if (outExternalHandle) {
        *outExternalHandle = 0;
    }

    HgiVulkanDevice* device = GetPrimaryDevice();

    VkExportSemaphoreCreateInfo exportInfo =
        { VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO };
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    exportInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    exportInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif

    VkSemaphoreCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    createInfo.flags = 0;
    createInfo.pNext = &exportInfo;
    VkSemaphore vkSem = VK_NULL_HANDLE;
    HGIVULKAN_VERIFY_VK_RESULT(
        vkCreateSemaphore(device->GetVulkanDevice(), &createInfo,
            nullptr, &vkSem));

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    if (outExternalHandle) {
        VkSemaphoreGetWin32HandleInfoKHR getInfo =
            { VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR };
        getInfo.semaphore = vkSem;
        getInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
        HANDLE handle = nullptr;
        device->vkGetSemaphoreWin32HandleKHR(
            device->GetVulkanDevice(), &getInfo, &handle);
        *outExternalHandle =
            static_cast<uint64_t>(reinterpret_cast<uintptr_t>(handle));
    }
#endif

    return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(vkSem));
}

uint64_t
HgiVulkan::ImportExternalSemaphore(
    uint64_t externalHandle,
    HgiExternalHandleType handleType,
    HgiSemaphoreKind kind)
{
    if (!externalHandle) {
        return 0;
    }
    if (handleType != _PlatformExternalHandleType()) {
        TF_WARN("HgiVulkan cannot import external semaphore handle type %d on "
                "this platform", static_cast<int>(handleType));
        return 0;
    }
    // Timeline import would additionally need per-submission wait/read values,
    // which the command queue's binary signal/wait lists do not carry.
    if (kind != HgiSemaphoreKindBinary) {
        TF_WARN("HgiVulkan can only import binary external semaphores");
        return 0;
    }

    HgiVulkanDevice* device = GetPrimaryDevice();
    if (!device->GetDeviceCapabilities().supportsNativeInterop) {
        return 0;
    }

    VkSemaphoreCreateInfo createInfo =
        { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkSemaphore vkSem = VK_NULL_HANDLE;
    HGIVULKAN_VERIFY_VK_RESULT(
        vkCreateSemaphore(device->GetVulkanDevice(), &createInfo,
            HgiVulkanAllocator(), &vkSem));

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    VkImportSemaphoreWin32HandleInfoKHR importInfo =
        { VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR };
    importInfo.semaphore = vkSem;
    importInfo.handleType =
        VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
    importInfo.handle =
        reinterpret_cast<HANDLE>(static_cast<uintptr_t>(externalHandle));
    const VkResult res = device->vkImportSemaphoreWin32HandleKHR
        ? device->vkImportSemaphoreWin32HandleKHR(
              device->GetVulkanDevice(), &importInfo)
        : VK_ERROR_EXTENSION_NOT_PRESENT;
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    VkImportSemaphoreFdInfoKHR importInfo =
        { VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR };
    importInfo.semaphore = vkSem;
    importInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
    importInfo.fd = static_cast<int>(externalHandle);
    const VkResult res = device->vkImportSemaphoreFdKHR
        ? device->vkImportSemaphoreFdKHR(
              device->GetVulkanDevice(), &importInfo)
        : VK_ERROR_EXTENSION_NOT_PRESENT;
#else
    const VkResult res = VK_ERROR_EXTENSION_NOT_PRESENT;
#endif

    if (res != VK_SUCCESS) {
        TF_WARN("Failed to import external semaphore (VkResult %d)",
                static_cast<int>(res));
        vkDestroySemaphore(device->GetVulkanDevice(), vkSem,
            HgiVulkanAllocator());
        return 0;
    }

    return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(vkSem));
}

std::string
HgiVulkan::GetDeviceUuid() const
{
    const uint8_t* uuid = GetPrimaryDevice()->GetDeviceCapabilities()
        .vkPhysicalDeviceIdProperties.deviceUUID;

    static const char* kHex = "0123456789abcdef";
    std::string out;
    out.reserve(VK_UUID_SIZE * 2);
    for (size_t i = 0; i < VK_UUID_SIZE; ++i) {
        out.push_back(kHex[uuid[i] >> 4]);
        out.push_back(kHex[uuid[i] & 0xf]);
    }
    return out;
}

uint64_t
HgiVulkan::GetLogicalDeviceId() const
{
    return _logicalDeviceId;
}

void
HgiVulkan::DestroyExternalSemaphore(uint64_t semaphore)
{
    if (!semaphore) {
        return;
    }
    VkSemaphore vkSem =
        reinterpret_cast<VkSemaphore>(static_cast<uintptr_t>(semaphore));
    HgiVulkanDevice* device = GetPrimaryDevice();
    device->WaitForIdle();
    vkDestroySemaphore(device->GetVulkanDevice(), vkSem, nullptr);
}

void
HgiVulkan::QueueWaitExternalSemaphore(uint64_t semaphore)
{
    if (!semaphore) {
        return;
    }
    VkSemaphore vkSem =
        reinterpret_cast<VkSemaphore>(static_cast<uintptr_t>(semaphore));
    GetPrimaryDevice()->GetCommandQueue()->AddPendingWaitSemaphore(vkSem);
}

void
HgiVulkan::QueueSignalExternalSemaphore(uint64_t semaphore)
{
    if (!semaphore) {
        return;
    }
    VkSemaphore vkSem =
        reinterpret_cast<VkSemaphore>(static_cast<uintptr_t>(semaphore));
    GetPrimaryDevice()->GetCommandQueue()->AddPendingSignalSemaphore(vkSem);
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
    }
}

/* Single threaded */
void
HgiVulkan::EndFrame()
{
    // Please read important usage limitations for Hgi::EndFrame

    if (--_frameDepth == 0) {
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

    // Perform garbage collection for each device.
    _garbageCollector->PerformGarbageCollection(device);
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

    if (TfDebug::IsEnabled(HGIVULKAN_DUMP_VMA_STATS)) {
        TfDebug::Disable(HGIVULKAN_DUMP_VMA_STATS);
        device->DumpMemoryStats();
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
