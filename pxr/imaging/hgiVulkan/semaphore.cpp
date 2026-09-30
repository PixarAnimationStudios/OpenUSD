//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiVulkan/buffer.h"
#include "pxr/imaging/hgiVulkan/capabilities.h"
#include "pxr/imaging/hgiVulkan/commandQueue.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"
#include "pxr/imaging/hgiVulkan/semaphore.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"

#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

std::shared_ptr<HgiVulkanSemaphore>
HgiVulkanSemaphore::Create(
    HgiVulkanDevice *device,
    HgiSemaphoreKind kind)
{
    if (!device) {
        return nullptr;
    }

    // No VkExportSemaphoreCreateInfo, and no supportsNativeInterop check: a
    // semaphore nobody outside this device will name needs neither.
    VkSemaphoreTypeCreateInfo typeInfo =
        { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
    typeInfo.semaphoreType = (kind == HgiSemaphoreKindTimeline)
        ? VK_SEMAPHORE_TYPE_TIMELINE : VK_SEMAPHORE_TYPE_BINARY;
    typeInfo.initialValue = 0;

    VkSemaphoreCreateInfo createInfo =
        { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    createInfo.flags = 0;
    createInfo.pNext = &typeInfo;

    VkSemaphore vkSemaphore = VK_NULL_HANDLE;
    HGIVULKAN_VERIFY_VK_RESULT(
        vkCreateSemaphore(device->GetVulkanDevice(), &createInfo,
            HgiVulkanAllocator(), &vkSemaphore));
    if (vkSemaphore == VK_NULL_HANDLE) {
        return nullptr;
    }

    return std::shared_ptr<HgiVulkanSemaphore>(new HgiVulkanSemaphore(
        device, vkSemaphore, kind, /*externalHandle*/ 0));
}

std::shared_ptr<HgiVulkanSemaphore>
HgiVulkanSemaphore::CreateExportable(
    HgiVulkanDevice *device,
    HgiSemaphoreKind kind)
{
    if (!device ||
            !device->GetDeviceCapabilities().supportsNativeInterop) {
        return nullptr;
    }

    VkExportSemaphoreCreateInfo exportInfo =
        { VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO };
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    exportInfo.handleTypes =
        VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    exportInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
#else
    return nullptr;
#endif

    VkSemaphoreTypeCreateInfo typeInfo =
        { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
    typeInfo.semaphoreType = (kind == HgiSemaphoreKindTimeline)
        ? VK_SEMAPHORE_TYPE_TIMELINE : VK_SEMAPHORE_TYPE_BINARY;
    typeInfo.initialValue = 0;
    typeInfo.pNext = &exportInfo;

    VkSemaphoreCreateInfo createInfo =
        { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    createInfo.flags = 0;
    createInfo.pNext = &typeInfo;

    VkSemaphore vkSemaphore = VK_NULL_HANDLE;
    HGIVULKAN_VERIFY_VK_RESULT(
        vkCreateSemaphore(device->GetVulkanDevice(), &createInfo,
            HgiVulkanAllocator(), &vkSemaphore));
    if (vkSemaphore == VK_NULL_HANDLE) {
        return nullptr;
    }

    uint64_t externalHandle = 0;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    if (device->vkGetSemaphoreWin32HandleKHR) {
        VkSemaphoreGetWin32HandleInfoKHR getInfo =
            { VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR };
        getInfo.semaphore = vkSemaphore;
        getInfo.handleType =
            VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
        HANDLE handle = nullptr;
        device->vkGetSemaphoreWin32HandleKHR(
            device->GetVulkanDevice(), &getInfo, &handle);
        externalHandle =
            static_cast<uint64_t>(reinterpret_cast<uintptr_t>(handle));
    }
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    if (device->vkGetSemaphoreFdKHR) {
        VkSemaphoreGetFdInfoKHR getInfo =
            { VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR };
        getInfo.semaphore = vkSemaphore;
        getInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
        int fd = -1;
        device->vkGetSemaphoreFdKHR(
            device->GetVulkanDevice(), &getInfo, &fd);
        if (fd >= 0) {
            externalHandle = static_cast<uint64_t>(fd);
        }
    }
#endif

    if (!externalHandle) {
        // An exportable semaphore nobody can import is not what was asked
        // for; failing here is better than handing back a useless object.
        vkDestroySemaphore(device->GetVulkanDevice(), vkSemaphore,
            HgiVulkanAllocator());
        return nullptr;
    }

    return std::shared_ptr<HgiVulkanSemaphore>(new HgiVulkanSemaphore(
        device, vkSemaphore, kind, externalHandle));
}

std::shared_ptr<HgiVulkanSemaphore>
HgiVulkanSemaphore::Import(
    HgiVulkanDevice *device,
    uint64_t externalHandle,
    HgiExternalHandleType handleType,
    HgiSemaphoreKind kind)
{
    if (!device || !externalHandle) {
        return nullptr;
    }
    if (!device->GetDeviceCapabilities().supportsNativeInterop) {
        return nullptr;
    }
    if (handleType != HgiGetPlatformExternalHandleType()) {
        TF_WARN("HgiVulkan cannot import external semaphore handle type %d on "
                "this platform", static_cast<int>(handleType));
        return nullptr;
    }
    // The queue's pending wait and signal lists carry no values, so a timeline
    // semaphore would be waited on as if it were binary.
    if (kind != HgiSemaphoreKindBinary) {
        TF_WARN("HgiVulkan can only import binary external semaphores");
        return nullptr;
    }

    VkSemaphoreCreateInfo createInfo =
        { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkSemaphore vkSemaphore = VK_NULL_HANDLE;
    HGIVULKAN_VERIFY_VK_RESULT(
        vkCreateSemaphore(device->GetVulkanDevice(), &createInfo,
            HgiVulkanAllocator(), &vkSemaphore));
    if (vkSemaphore == VK_NULL_HANDLE) {
        return nullptr;
    }

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    VkImportSemaphoreWin32HandleInfoKHR importInfo =
        { VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR };
    importInfo.semaphore = vkSemaphore;
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
    importInfo.semaphore = vkSemaphore;
    importInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
    // The import takes over the fd.
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
        vkDestroySemaphore(device->GetVulkanDevice(), vkSemaphore,
            HgiVulkanAllocator());
        return nullptr;
    }

    return std::shared_ptr<HgiVulkanSemaphore>(new HgiVulkanSemaphore(
        device, vkSemaphore, kind, /*externalHandle*/ 0));
}

HgiVulkanSemaphore::HgiVulkanSemaphore(
    HgiVulkanDevice *device,
    VkSemaphore vkSemaphore,
    HgiSemaphoreKind kind,
    uint64_t externalHandle)
    : HgiSemaphore(kind)
    , _device(device)
    , _vkSemaphore(vkSemaphore)
    , _externalHandle(externalHandle)
{
}

HgiVulkanSemaphore::~HgiVulkanSemaphore()
{
    _ReleaseResources();
}

void
HgiVulkanSemaphore::_ReleaseResources()
{
    if (_vkSemaphore == VK_NULL_HANDLE) {
        return;
    }
    // Not destroyed here: this runs on whichever thread drops the last
    // reference (ImportSemaphores replacing a pair, say), and a submission in
    // flight may still name the semaphore. The queue destroys it on the main
    // thread once those command buffers retire, or at device teardown.
    HgiVulkanCommandQueue *queue = _device->GetCommandQueue();
    queue->RemovePendingSemaphore(_vkSemaphore);
    queue->DestroySemaphoreDeferred(_vkSemaphore);
    _vkSemaphore = VK_NULL_HANDLE;
}

// The Vulkan buffer behind \p buffer, or null for one that needs no
// queue-family ownership transfer: registered and adopted buffers never leave
// this device, so only imported and exportable ones pass through
// VK_QUEUE_FAMILY_EXTERNAL.
static HgiVulkanBuffer *
_GetExternallySharedVulkanBuffer(HgiExternalBuffer *buffer)
{
    if (!buffer) {
        return nullptr;
    }
    HgiVulkanBuffer *vkBuffer =
        static_cast<HgiVulkanBuffer *>(buffer->GetBuffer().Get());
    return vkBuffer && vkBuffer->SharesQueueFamilyExternal()
        ? vkBuffer : nullptr;
}

void
HgiVulkanSemaphore::EncodeWait(
    uint64_t /*value*/,
    std::vector<HgiExternalBuffer *> const &buffers)
{
    if (_vkSemaphore == VK_NULL_HANDLE) {
        return;
    }
    // The semaphore supplies the memory dependency. Ownership of a shared
    // buffer is separate: the producer released it to VK_QUEUE_FAMILY_EXTERNAL
    // when it published, so take it back. The acquires are recorded into the
    // same submission that waits, which orders them after the wait.
    for (HgiExternalBuffer *buffer : buffers) {
        if (HgiVulkanBuffer *vkBuffer =
                _GetExternallySharedVulkanBuffer(buffer)) {
            vkBuffer->AcquireExternalOwnership();
        }
    }
    _device->GetCommandQueue()->AddPendingWaitSemaphore(_vkSemaphore);
}

void
HgiVulkanSemaphore::EncodeSignal(
    uint64_t /*value*/,
    std::vector<HgiExternalBuffer *> const &buffers)
{
    if (_vkSemaphore == VK_NULL_HANDLE) {
        return;
    }
    // Hand shared buffers back before signalling, so the producer can
    // acquire them once it has waited. The releases are drained by the Flush
    // that carries this signal.
    for (HgiExternalBuffer *buffer : buffers) {
        if (HgiVulkanBuffer *vkBuffer =
                _GetExternallySharedVulkanBuffer(buffer)) {
            vkBuffer->ReleaseExternalOwnership();
        }
    }
    _device->GetCommandQueue()->AddPendingSignalSemaphore(_vkSemaphore);
}

PXR_NAMESPACE_CLOSE_SCOPE
