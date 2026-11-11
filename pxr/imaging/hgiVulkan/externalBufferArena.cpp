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
#include "pxr/imaging/hgiVulkan/externalBufferArena.h"
#include "pxr/imaging/hgiVulkan/hgi.h"
#include "pxr/imaging/hgiVulkan/semaphore.h"

#include "pxr/imaging/hgi/tokens.h"

#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

bool
HgiVulkanExternalBufferArena::IsSupportedBy(Hgi *hgi)
{
    if (!hgi || hgi->GetAPIName() != HgiTokens->Vulkan) {
        return false;
    }
    HgiVulkanDevice *device =
        static_cast<HgiVulkan *>(hgi)->GetPrimaryDevice();
    return device &&
        device->GetDeviceCapabilities().supportsNativeInterop;
}

HgiVulkanExternalBufferArena::HgiVulkanExternalBufferArena(Hgi *hgi)
    : HgiExternalBufferArena(hgi)
{
    // No semaphores by default. A producer on this same device and queue is
    // already ordered by submission order; one that needs more calls
    // CreateExportableSemaphores or ImportSemaphores.
}

HgiVulkanExternalBufferArena::~HgiVulkanExternalBufferArena() = default;

HgiVulkanDevice *
HgiVulkanExternalBufferArena::_GetDevice() const
{
    // Null once Hgi has been torn down; see HgiExternalBufferArena::_Shutdown.
    Hgi *hgi = GetHgi();
    return hgi ? static_cast<HgiVulkan *>(hgi)->GetPrimaryDevice() : nullptr;
}

HgiExternalBufferSharedPtr
HgiVulkanExternalBufferArena::AllocateBuffer(
    size_t byteSize,
    HgiBufferUsage usage,
    std::string const &debugName)
{
    if (!_GetDevice()) {
        return nullptr;
    }
    return _Register(HgiVulkanExternalBuffer::_CreateAllocated(
        this, _GetNextBufferHandleId(), byteSize, usage, debugName));
}

HgiExternalBufferSharedPtr
HgiVulkanExternalBufferArena::RegisterBuffer(
    VkBuffer vkBuffer,
    size_t byteSize,
    HgiBufferUsage usage)
{
    if (!_GetDevice()) {
        return nullptr;
    }
    return _Register(HgiVulkanExternalBuffer::_CreateAdopted(
        this, _GetNextBufferHandleId(), vkBuffer, byteSize, usage,
        /*takeOwnership*/ false));
}

HgiExternalBufferSharedPtr
HgiVulkanExternalBufferArena::AdoptBuffer(
    VkBuffer vkBuffer,
    size_t byteSize,
    HgiBufferUsage usage)
{
    if (!_GetDevice()) {
        return nullptr;
    }
    return _Register(HgiVulkanExternalBuffer::_CreateAdopted(
        this, _GetNextBufferHandleId(), vkBuffer, byteSize, usage,
        /*takeOwnership*/ true));
}

HgiExternalBufferSharedPtr
HgiVulkanExternalBufferArena::ImportBuffer(
    HgiVulkanImportBufferDesc const &desc)
{
    if (!_GetDevice()) {
        return nullptr;
    }
    return _Register(HgiVulkanExternalBuffer::_CreateImported(
        this, _GetNextBufferHandleId(), desc));
}

bool
HgiVulkanExternalBufferArena::CreateSemaphores(HgiSemaphoreKind kind)
{
    return _CreateSemaphorePair(kind, /*exportable*/ false, nullptr, nullptr);
}

bool
HgiVulkanExternalBufferArena::CreateExportableSemaphores(
    HgiSemaphoreKind kind,
    uint64_t *outAppDoneHandle,
    uint64_t *outHgiDoneHandle)
{
    return _CreateSemaphorePair(
        kind, /*exportable*/ true, outAppDoneHandle, outHgiDoneHandle);
}

bool
HgiVulkanExternalBufferArena::_CreateSemaphorePair(
    HgiSemaphoreKind kind,
    bool exportable,
    uint64_t *outAppDoneHandle,
    uint64_t *outHgiDoneHandle)
{
    if (outAppDoneHandle) {
        *outAppDoneHandle = 0;
    }
    if (outHgiDoneHandle) {
        *outHgiDoneHandle = 0;
    }

    // Binary only, and not because Vulkan cannot do better. HgiVulkanSemaphore
    // encodes through the command queue's pending wait and signal lists, and
    // those carry no values, so a timeline semaphore would reach vkQueueSubmit
    // with no VkTimelineSemaphoreSubmitInfo -- invalid usage, not merely a
    // timeline treated as binary. ImportSemaphore refuses for the same reason
    // (hgiVulkan/semaphore.cpp); refusing here as well keeps all three
    // creation paths consistent, rather than returning success and an object
    // that cannot be used correctly.
    //
    // The arena's own value bookkeeping -- NotifyAppDone's value carried
    // through to EncodeWait, and an increasing value per signal -- is complete
    // and would not need to change if the queue ever learned to carry values.
    if (kind != HgiSemaphoreKindBinary) {
        TF_WARN("HgiVulkanExternalBufferArena can only create binary "
                "semaphores; the command queue's pending wait and signal "
                "lists carry no timeline values");
        return false;
    }

    HgiVulkanDevice *device = _GetDevice();
    if (!device) {
        return false;
    }
    auto make = [device, kind, exportable]() {
        return exportable
            ? HgiVulkanSemaphore::CreateExportable(device, kind)
            : HgiVulkanSemaphore::Create(device, kind);
    };

    HgiVulkanSemaphoreSharedPtr appDone = make();
    if (!appDone) {
        return false;
    }
    HgiVulkanSemaphoreSharedPtr hgiDone = make();
    if (!hgiDone) {
        return false;
    }

    if (outAppDoneHandle) {
        *outAppDoneHandle = appDone->GetExternalHandle();
    }
    if (outHgiDoneHandle) {
        *outHgiDoneHandle = hgiDone->GetExternalHandle();
    }

    _SetSemaphores(std::move(appDone), std::move(hgiDone));
    return true;
}

VkSemaphore
HgiVulkanExternalBufferArena::GetAppDoneVkSemaphore() const
{
    // The static_cast is sound rather than hopeful: this arena's semaphores
    // are only ever installed by CreateExportableSemaphores or
    // ImportSemaphores below, both of which build HgiVulkanSemaphores.
    HgiSemaphoreSharedPtr const &semaphore = GetAppDoneHgiSemaphore();
    return semaphore
        ? static_cast<HgiVulkanSemaphore *>(semaphore.get())
              ->GetVulkanSemaphore()
        : VK_NULL_HANDLE;
}

VkSemaphore
HgiVulkanExternalBufferArena::GetHgiDoneVkSemaphore() const
{
    HgiSemaphoreSharedPtr const &semaphore = GetHgiDoneHgiSemaphore();
    return semaphore
        ? static_cast<HgiVulkanSemaphore *>(semaphore.get())
              ->GetVulkanSemaphore()
        : VK_NULL_HANDLE;
}

bool
HgiVulkanExternalBufferArena::ImportSemaphores(
    uint64_t appDoneHandle,
    uint64_t hgiDoneHandle,
    HgiExternalHandleType handleType,
    HgiSemaphoreKind kind)
{
    HgiVulkanDevice *device = _GetDevice();
    if (!device) {
        return false;
    }

    HgiSemaphoreSharedPtr appDone;
    HgiSemaphoreSharedPtr hgiDone;

    if (appDoneHandle) {
        appDone = HgiVulkanSemaphore::Import(
            device, appDoneHandle, handleType, kind);
        if (!appDone) {
            return false;
        }
    }
    if (hgiDoneHandle) {
        hgiDone = HgiVulkanSemaphore::Import(
            device, hgiDoneHandle, handleType, kind);
        if (!hgiDone) {
            return false;
        }
    }

    _SetSemaphores(std::move(appDone), std::move(hgiDone));
    return true;
}

uint64_t
HgiVulkanExternalBufferArena::_CaptureSubmissionStamp()
{
    // The set of command buffers in flight right now, as a bitmask. Any of
    // them could still be reading the buffers this pass is retiring; none of
    // the ones submitted later can, because a rebind has to go through Sync
    // and Sync will not re-add a released buffer.
    //
    // This is the same predicate HgiVulkanGarbageCollector uses to decide when
    // a trashed object may be deleted, deliberately: reusing it means external
    // buffers retire on exactly the same schedule as every other Vulkan
    // resource, rather than on a second mechanism that has to agree with it.
    //
    // 0 when nothing is in flight, which retires immediately.
    HgiVulkanDevice *device = _GetDevice();
    return device
        ? device->GetCommandQueue()->GetInflightCommandBuffersBits() : 0;
}

bool
HgiVulkanExternalBufferArena::_IsSubmissionRetired(uint64_t stamp)
{
    // Retired once none of the command buffers that were in flight at stamp
    // time still are. The bits clear as command buffers are consumed and
    // reset, so this needs no waiting and no submission of its own.
    HgiVulkanDevice *device = _GetDevice();
    if (!device) {
        return true;
    }
    const uint64_t inflight =
        device->GetCommandQueue()->GetInflightCommandBuffersBits();
    return (inflight & stamp) == 0;
}

void
HgiVulkanExternalBufferArena::_AcquireBuffersWithoutWait(
    std::vector<HgiExternalBuffer *> const &buffers)
{
    if (!_GetDevice()) {
        return;
    }
    // The acquire half of what the hgi-done signal releases. With no app-done
    // semaphore the application orders its own writes, and ownership is all
    // there is to take back.
    for (HgiExternalBuffer *buffer : buffers) {
        if (!buffer) {
            continue;
        }
        if (HgiVulkanBuffer *vkBuffer =
                static_cast<HgiVulkanBuffer *>(buffer->GetBuffer().Get())) {
            vkBuffer->AcquireExternalOwnership();
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
