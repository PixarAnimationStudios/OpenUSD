//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiVulkan/capabilities.h"
#include "pxr/imaging/hgiVulkan/commandQueue.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/externalBufferArena.h"
#include "pxr/imaging/hgiVulkan/hgi.h"
#include "pxr/imaging/hgiVulkan/semaphore.h"

#include "pxr/imaging/hgi/tokens.h"

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

HgiVulkanExternalBufferArena::HgiVulkanExternalBufferArena(
    Hgi *hgi,
    uint64_t rawSourceDevice)
    : HgiExternalBufferArena(hgi, rawSourceDevice)
{
    // No semaphores by default. A producer on this same device and queue is
    // already ordered by submission order; one that needs more calls
    // CreateExportableSemaphores or ImportSemaphores.
}

HgiVulkanExternalBufferArena::~HgiVulkanExternalBufferArena() = default;

HgiVulkanDevice *
HgiVulkanExternalBufferArena::_GetDevice() const
{
    return static_cast<HgiVulkan *>(GetHgi())->GetPrimaryDevice();
}

HgiExternalBufferSharedPtr
HgiVulkanExternalBufferArena::AllocateBuffer(
    size_t byteSize,
    HgiBufferUsage usage,
    std::string const &debugName)
{
    return _Register(HgiVulkanExternalBuffer::_CreateAllocated(
        this, _GetNextBufferHandleId(), byteSize, usage, debugName));
}

HgiExternalBufferSharedPtr
HgiVulkanExternalBufferArena::RegisterBuffer(
    VkBuffer vkBuffer,
    size_t byteSize,
    HgiBufferUsage usage)
{
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
    return _Register(HgiVulkanExternalBuffer::_CreateAdopted(
        this, _GetNextBufferHandleId(), vkBuffer, byteSize, usage,
        /*takeOwnership*/ true));
}

HgiExternalBufferSharedPtr
HgiVulkanExternalBufferArena::ImportBuffer(
    HgiVulkanImportBufferDesc const &desc)
{
    return _Register(HgiVulkanExternalBuffer::_CreateImported(
        this, _GetNextBufferHandleId(), desc));
}

bool
HgiVulkanExternalBufferArena::CreateExportableSemaphores(
    HgiSemaphoreKind kind,
    uint64_t *outAppDoneHandle,
    uint64_t *outHgiDoneHandle)
{
    if (outAppDoneHandle) {
        *outAppDoneHandle = 0;
    }
    if (outHgiDoneHandle) {
        *outHgiDoneHandle = 0;
    }

    HgiVulkanDevice *device = _GetDevice();
    HgiVulkanSemaphoreSharedPtr appDone =
        HgiVulkanSemaphore::CreateExportable(device, kind);
    if (!appDone) {
        return false;
    }
    HgiVulkanSemaphoreSharedPtr hgiDone =
        HgiVulkanSemaphore::CreateExportable(device, kind);
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

bool
HgiVulkanExternalBufferArena::ImportSemaphores(
    uint64_t appDoneHandle,
    uint64_t hgiDoneHandle,
    HgiExternalHandleType handleType,
    HgiSemaphoreKind kind)
{
    HgiVulkanDevice *device = _GetDevice();

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
    return _GetDevice()->GetCommandQueue()->GetInflightCommandBuffersBits();
}

bool
HgiVulkanExternalBufferArena::_IsSubmissionRetired(uint64_t stamp)
{
    // Retired once none of the command buffers that were in flight at stamp
    // time still are. The bits clear as command buffers are consumed and
    // reset, so this needs no waiting and no submission of its own.
    const uint64_t inflight =
        _GetDevice()->GetCommandQueue()->GetInflightCommandBuffersBits();
    return (inflight & stamp) == 0;
}

PXR_NAMESPACE_CLOSE_SCOPE
