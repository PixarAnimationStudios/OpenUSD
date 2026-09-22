//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIVULKAN_SEMAPHORE_H
#define PXR_IMAGING_HGIVULKAN_SEMAPHORE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"
#include "pxr/imaging/hgi/externalBuffer.h"
#include "pxr/imaging/hgi/semaphore.h"

#include <cstdint>
#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkanDevice;

/// \class HgiVulkanSemaphore
///
/// A VkSemaphore, optionally exportable to or imported from another API.
///
/// Waits and signals are appended to the command queue's pending lists rather
/// than issued, so they ride on the next submission -- which is what "encode"
/// means here and why nothing blocks the host.
///
/// Only binary semaphores can cross an API boundary to OpenGL, which has no
/// timeline form; a timeline semaphore is available when both sides are
/// Vulkan, and is the better choice there because a timeline wait can be
/// satisfied repeatedly, so several render passes in one application frame do
/// not each consume a separate signal.
///
class HgiVulkanSemaphore final : public HgiSemaphore
{
public:
    /// Create a semaphore on \p device whose signal state is EXPORTABLE to
    /// another API, so an application can import it and synchronize against
    /// Hgi. Returns null when the device has no external-semaphore support.
    /// GetExternalHandle() is the OS handle to hand the other API.
    HGIVULKAN_API
    /// Create a semaphore for use on \p device only.
    ///
    /// Nothing crosses a device or API boundary, so this needs no external
    /// handle and no interop support -- which is the whole difference from
    /// CreateExportable(). Use it when the application shares the consumer's
    /// logical device and can therefore wait on the VkSemaphore directly.
    HGIVULKAN_API
    static std::shared_ptr<HgiVulkanSemaphore> Create(
        HgiVulkanDevice *device,
        HgiSemaphoreKind kind);

    HGIVULKAN_API
    static std::shared_ptr<HgiVulkanSemaphore> CreateExportable(
        HgiVulkanDevice *device,
        HgiSemaphoreKind kind);

    /// Import the semaphore named by the OS handle \p externalHandle, which
    /// another API created as exportable. Returns null when the device cannot
    /// import it, when \p handleType is not this platform's type, or when
    /// \p kind is not binary -- the queue's pending wait and signal lists
    /// carry no timeline values, so a timeline import would wait on the wrong
    /// thing.
    ///
    /// Handle ownership follows the platform convention: an fd is taken over
    /// by the import, a Win32 handle stays the caller's to close.
    HGIVULKAN_API
    static std::shared_ptr<HgiVulkanSemaphore> Import(
        HgiVulkanDevice *device,
        uint64_t externalHandle,
        HgiExternalHandleType handleType,
        HgiSemaphoreKind kind);

    HGIVULKAN_API
    ~HgiVulkanSemaphore() override;

    HGIVULKAN_API
    void EncodeWait(
        uint64_t value,
        std::vector<HgiExternalBuffer *> const &buffers) override;

    HGIVULKAN_API
    void EncodeSignal(
        uint64_t value,
        std::vector<HgiExternalBuffer *> const &buffers) override;

    /// The underlying semaphore, for building a queue submission's wait or
    /// signal list.
    VkSemaphore GetVulkanSemaphore() const {
        return _vkSemaphore;
    }

    /// The OS handle another API imports to synchronize against this
    /// semaphore, or 0 when it was not created exportable.
    uint64_t GetExternalHandle() const {
        return _externalHandle;
    }

private:
    HgiVulkanSemaphore(
        HgiVulkanDevice *device,
        VkSemaphore vkSemaphore,
        HgiSemaphoreKind kind,
        uint64_t externalHandle);

    HgiVulkanSemaphore() = delete;
    HgiVulkanSemaphore(const HgiVulkanSemaphore &) = delete;
    HgiVulkanSemaphore & operator=(const HgiVulkanSemaphore &) = delete;

    HgiVulkanDevice *_device;
    VkSemaphore _vkSemaphore;
    uint64_t _externalHandle;
};

using HgiVulkanSemaphoreSharedPtr = std::shared_ptr<HgiVulkanSemaphore>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HGIVULKAN_SEMAPHORE_H
