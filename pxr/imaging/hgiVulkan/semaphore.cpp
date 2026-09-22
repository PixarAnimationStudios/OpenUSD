//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
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
    if (_vkSemaphore == VK_NULL_HANDLE) {
        return;
    }
    // A semaphore may still be referenced by a submission in flight, and
    // unlike a buffer there is no per-object trash list to defer it through.
    // The arena only destroys these at teardown, so idling the device here is
    // the cost of being certain rather than a per-frame cost.
    _device->WaitForIdle();
    vkDestroySemaphore(_device->GetVulkanDevice(), _vkSemaphore,
        HgiVulkanAllocator());
    _vkSemaphore = VK_NULL_HANDLE;
}

void
HgiVulkanSemaphore::EncodeWait(
    uint64_t /*value*/,
    std::vector<HgiExternalBuffer *> const & /*buffers*/)
{
    // Vulkan needs no buffer list: the semaphore establishes the memory
    // dependency on its own.
    if (_vkSemaphore == VK_NULL_HANDLE) {
        return;
    }
    _device->GetCommandQueue()->AddPendingWaitSemaphore(_vkSemaphore);
}

void
HgiVulkanSemaphore::EncodeSignal(
    uint64_t /*value*/,
    std::vector<HgiExternalBuffer *> const & /*buffers*/)
{
    if (_vkSemaphore == VK_NULL_HANDLE) {
        return;
    }
    _device->GetCommandQueue()->AddPendingSignalSemaphore(_vkSemaphore);
}

PXR_NAMESPACE_CLOSE_SCOPE
