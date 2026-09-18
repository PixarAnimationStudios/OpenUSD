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
///     auto arena = hgi->GetExternalBufferArena<HgiVulkanExternalBufferArena>(
///         uint64_t(myVkDevice));
///     if (!arena) { /* interop unavailable -- copy instead */ }
/// \endcode
///
/// \section Direction
///
/// An arena belongs to the *consuming* backend and is keyed on the
/// *producing* device, which is what keeps hgiVulkan and hgiGL independent of
/// each other. The routes a Vulkan consumer has:
///
/// | producer | how | notes |
/// | -------- | --- | ----- |
/// | Vulkan, same logical device | RegisterBuffer / AdoptBuffer | passthrough |
/// | Vulkan, another device | ImportBuffer | imports the exported allocation |
/// | OpenGL | -- | impossible; GL cannot export |
///
/// Passing a VkBuffer from a *different* VkDevice to RegisterBuffer is the
/// hazard this arena's key exists to prevent: the handle would be interpreted
/// in the wrong namespace and bind an unrelated object. Ask for the arena
/// belonging to the device that minted the handle and the question cannot
/// arise. Use ImportBuffer across devices, which shares memory rather than
/// handles.
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

    /// Constructed by Hgi::GetExternalBufferArena; \p rawSourceDevice is a
    /// uint64 cast of the producer's VkDevice (or of its device object, for a
    /// producer in another API whose memory we import).
    HGIVULKAN_API
    HgiVulkanExternalBufferArena(Hgi *hgi, uint64_t rawSourceDevice);

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

    /// Create the arena's semaphore pair as EXPORTABLE Vulkan semaphores and
    /// return their OS handles in \p outAppDoneHandle and
    /// \p outHgiDoneHandle, for the application to import. Use this when Hgi
    /// should own the semaphores; use ImportSemaphores when the application
    /// already has its own.
    ///
    /// Prefer HgiSemaphoreKindTimeline when the application is also Vulkan: a
    /// timeline wait can be satisfied more than once, so several render passes
    /// in one application frame do not each need a separate signal. An
    /// application on OpenGL must take binary, which is all GL can import.
    ///
    /// Returns false and leaves the arena unsynchronized if the device cannot
    /// create exportable semaphores.
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

protected:
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
