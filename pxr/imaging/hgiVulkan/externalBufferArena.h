//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIVULKAN_EXTERNAL_BUFFER_ARENA_H
#define PXR_IMAGING_HGIVULKAN_EXTERNAL_BUFFER_ARENA_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/externalBuffer.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"
#include "pxr/imaging/hgi/externalBufferArena.h"

#include <cstdint>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;
class HgiVulkanDevice;

/// \class HgiVulkanExternalBufferArena
///
/// The arena through which an application shares GPU buffers with an
/// HgiVulkan consumer. Obtain one from Hgi::GetExternalBufferArena:
///
/// \code
///     auto arena = hgi->GetExternalBufferArena<HgiVulkanExternalBufferArena>();
///     if (!arena) { /* interop unavailable -- copy instead */ }
/// \endcode
///
/// \section Direction
///
/// An arena belongs to the *consuming* backend, which is what keeps hgiVulkan
/// and hgiGL independent of each other. The routes a Vulkan consumer has:
///
/// | producer | how | notes |
/// | -------- | --- | ----- |
/// | Vulkan, same logical device | RegisterBuffer / AdoptBuffer | passthrough |
/// | Vulkan, another device | ImportBuffer | imports the exported allocation |
/// | OpenGL | -- | impossible; GL cannot export |
///
/// RegisterBuffer and AdoptBuffer take the handle at face value and bind it on
/// the consumer's device, so the VkBuffer must have been minted by that same
/// logical device. Passing one from a different VkDevice interprets the handle
/// in the wrong namespace and binds an unrelated object -- nothing here can
/// detect it, because a VkBuffer carries no evidence of which device made it.
/// Use ImportBuffer across devices, which shares memory rather than handles.
///
/// An OpenGL producer cannot feed a Vulkan consumer at all, and no entry point
/// here pretends otherwise: OpenGL has no way to export an allocation for
/// another API to import. Such an application has to let Hgi own the memory
/// (AllocateBuffer, which allocates exportable and hands back the import
/// description) or copy through the CPU.
///
/// \section Ownership
///
/// RegisterBuffer and AdoptBuffer differ only in who destroys the VkBuffer.
/// Register for a buffer the application still uses and recycles on its own
/// schedule -- destroying it from here would be a double free. Adopt only for
/// one the application is genuinely handing over. See
/// HgiExternalBuffer::SetKeepalive for what the registration contract does and
/// does not guarantee.
///
class HgiVulkanExternalBufferArena final : public HgiExternalBufferArena
{
public:
    /// Whether \p hgi can consume external buffers through this arena: an
    /// HgiVulkan whose device supports the external-memory extensions. See
    /// Hgi::GetExternalBufferArena.
    HGIVULKAN_API
    static bool IsSupportedBy(Hgi *hgi);

    /// Constructed by Hgi::GetExternalBufferArena.
    HGIVULKAN_API
    HgiVulkanExternalBufferArena(Hgi *hgi);

    HGIVULKAN_API
    ~HgiVulkanExternalBufferArena() override;

    /// Allocate an exportable buffer the arena owns and frees. The result's
    /// GetExportInfo() describes the memory so an application in any API can
    /// import it -- which makes this the route for a producer that cannot
    /// export its own allocations, OpenGL included.
    HGIVULKAN_API
    HgiExternalBufferSharedPtr AllocateBuffer(
        size_t byteSize,
        HgiBufferUsage usage,
        std::string const &debugName = std::string()) override;

    /// Share the existing \p vkBuffer WITHOUT transferring ownership: the
    /// arena binds and reads it, and never destroys it. \p vkBuffer must
    /// belong to this arena's source device.
    HGIVULKAN_API
    HgiExternalBufferSharedPtr RegisterBuffer(
        VkBuffer vkBuffer,
        size_t byteSize,
        HgiBufferUsage usage);

    /// Hand the existing \p vkBuffer over to the arena, which destroys it once
    /// nothing references it and the GPU has retired the work that named it.
    /// Only correct for a buffer the application will never touch again.
    HGIVULKAN_API
    HgiExternalBufferSharedPtr AdoptBuffer(
        VkBuffer vkBuffer,
        size_t byteSize,
        HgiBufferUsage usage);

    /// Import the foreign allocation described by \p desc and bind a VkBuffer
    /// over it. Returns null when the import fails, in which case the caller
    /// copies instead.
    HGIVULKAN_API
    HgiExternalBufferSharedPtr ImportBuffer(
        HgiVulkanImportBufferDesc const &desc);

    /// Create the arena's semaphore pair for use on this device only, and
    /// nowhere else. For an application that shares the consumer's logical
    /// device: it reads the pair through GetAppDoneVkSemaphore() /
    /// GetHgiDoneVkSemaphore() and uses it natively, so nothing needs
    /// exporting and no interop handles are minted.
    ///
    /// Prefer this over CreateExportableSemaphores() when nothing crosses a
    /// device or API boundary -- an exportable semaphore whose OS handle is
    /// never used is a handle leaked for no reason.
    ///
    /// \p kind must be HgiSemaphoreKindBinary. Timeline is refused, and the
    /// reason is HgiVulkanSemaphore rather than the device: it encodes through
    /// the command queue's pending wait and signal lists, which carry no
    /// values, so a timeline semaphore would reach vkQueueSubmit without a
    /// VkTimelineSemaphoreSubmitInfo. Nothing is lost by this today -- the
    /// arena's epoch guard already collapses several render passes in one
    /// frame into a single wait and a single signal, which is what a timeline
    /// semaphore would otherwise have been wanted for.
    ///
    /// Returns false, leaving the arena unsynchronized, if the semaphores
    /// cannot be created or \p kind is not binary.
    HGIVULKAN_API
    bool CreateSemaphores(HgiSemaphoreKind kind);

    /// Create the arena's semaphore pair as EXPORTABLE Vulkan semaphores and
    /// return their OS handles in \p outAppDoneHandle and
    /// \p outHgiDoneHandle, for the application to import. Use this when Hgi
    /// should own the semaphores; use ImportSemaphores when the application
    /// already has its own.
    ///
    /// \p kind must be HgiSemaphoreKindBinary; see CreateSemaphores for why.
    /// That is also all an OpenGL application could import in any case --
    /// GL_EXT_semaphore has no timeline form.
    ///
    /// Returns false and leaves the arena unsynchronized if the device cannot
    /// create exportable semaphores, or \p kind is not binary.
    HGIVULKAN_API
    bool CreateExportableSemaphores(
        HgiSemaphoreKind kind,
        uint64_t *outAppDoneHandle,
        uint64_t *outHgiDoneHandle);

    /// Import the application's exported semaphore pair, replacing whatever
    /// this arena was using. Either handle may be 0 for "none". Returns false,
    /// and leaves the arena unsynchronized, when the device cannot import
    /// them.
    HGIVULKAN_API
    bool ImportSemaphores(
        uint64_t appDoneHandle,
        uint64_t hgiDoneHandle,
        HgiExternalHandleType handleType,
        HgiSemaphoreKind kind) override;

    /// The arena's semaphore pair as native VkSemaphores, or VK_NULL_HANDLE
    /// when this arena has none.
    ///
    /// The application's half of the bracket is its own API call -- Hgi has no
    /// access to the application's queue and so cannot encode a wait on its
    /// behalf. There is deliberately no EncodeHgiDoneWait() for that reason;
    /// these accessors are what the application needs instead.
    ///
    /// Contract: signal app-done after finishing a write and before calling
    /// into Hgi, then NotifyAppDone(); wait on hgi-done before overwriting a
    /// shared buffer.
    ///
    /// For a producer on THIS logical device, which is the case these are for.
    /// A producer on another device, or in another API, cannot use a
    /// VkSemaphore at all -- a semaphore is namespaced to the device that
    /// created it -- and wants CreateExportableSemaphores() or
    /// ImportSemaphores(), which trade in OS handles.
    HGIVULKAN_API
    VkSemaphore GetAppDoneVkSemaphore() const;

    HGIVULKAN_API
    VkSemaphore GetHgiDoneVkSemaphore() const;

protected:
    /// Shared by CreateSemaphores and CreateExportableSemaphores; the only
    /// difference between them is whether the pair is exportable and whether
    /// the caller wants the handles back.
    HGIVULKAN_API
    bool _CreateSemaphorePair(
        HgiSemaphoreKind kind,
        bool exportable,
        uint64_t *outAppDoneHandle,
        uint64_t *outHgiDoneHandle);

    HGIVULKAN_API
    uint64_t _CaptureSubmissionStamp() override;

    HGIVULKAN_API
    bool _IsSubmissionRetired(uint64_t stamp) override;

private:
    HgiVulkanExternalBufferArena() = delete;
    HgiVulkanExternalBufferArena(
        const HgiVulkanExternalBufferArena &) = delete;
    HgiVulkanExternalBufferArena & operator=(
        const HgiVulkanExternalBufferArena &) = delete;

    HgiVulkanDevice *_GetDevice() const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HGIVULKAN_EXTERNAL_BUFFER_ARENA_H
