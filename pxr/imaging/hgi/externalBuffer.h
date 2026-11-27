//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_EXTERNAL_BUFFER_H
#define PXR_IMAGING_HGI_EXTERNAL_BUFFER_H

#include "pxr/pxr.h"
#include "pxr/base/arch/defines.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/buffer.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;
class HgiExternalBufferArena;

/// \enum HgiExternalHandleType
///
/// How an OS-shareable handle to external memory or an external semaphore
/// should be interpreted.  The kind is never inferred from the handle value --
/// a Win32 NT handle and a POSIX fd are both small integers.
enum HgiExternalHandleType
{
    HgiExternalHandleTypeOpaqueWin32 = 0,
    HgiExternalHandleTypeOpaqueFd,
};

/// The handle type this platform's graphics APIs mint and accept: a Win32 NT
/// handle on Windows, a POSIX fd everywhere else.
///
/// Descriptors default their handle type to this rather than to a fixed
/// enumerator, because the failure mode of the alternative is silent.  Both
/// forms are small integers, so a descriptor left saying "Win32" on Linux
/// does not look wrong -- it reaches an import that refuses a handle type it
/// cannot use, on one platform only, and the caller is told nothing about
/// which field was at fault.
///
/// Note that the two also differ in ownership, which is why the type cannot
/// just be inferred late: an imported fd is CONSUMED by the import, while an
/// imported Win32 handle is not, and the exporter must close its own copy.
constexpr HgiExternalHandleType
HgiGetPlatformExternalHandleType()
{
#if defined(ARCH_OS_WINDOWS)
    return HgiExternalHandleTypeOpaqueWin32;
#else
    return HgiExternalHandleTypeOpaqueFd;
#endif
}

/// \struct HgiExternalBufferExportDesc
///
/// Describes the memory behind a buffer Hgi allocated, so an application in
/// another API can import and alias the same memory.  The counterpart of a
/// backend's own import descriptor, from the other side, and the generic form
/// of it: every field here is either an OS handle or a plain size, so an
/// importer needs to know nothing about the backend that produced it.
///
/// Filled by HgiExternalBuffer::GetExportDesc, and meaningful only for a
/// buffer from HgiExternalBufferArena::AllocateBuffer -- the arena allocates
/// those exportable precisely so they can be handed out.  A registered,
/// adopted or imported buffer has nothing to export: its memory came from
/// somewhere else.
///
struct HgiExternalBufferExportDesc
{
    /// OS-shareable handle naming the memory, or 0 when the buffer cannot be
    /// exported.  On Windows the recipient closes it when done; on Linux the
    /// importing call consumes it.
    uint64_t externalHandle = 0;

    /// How to interpret externalHandle.
    HgiExternalHandleType handleType = HgiGetPlatformExternalHandleType();

    /// Size of the whole memory block, and the buffer's offset within it.
    ///
    /// Both matter, because backends suballocate: an importer that allocates
    /// only the buffer's size gets a different allocation rather than this
    /// one.  The importer aliases memoryBlockSize bytes and binds its own
    /// buffer at memoryOffset.
    size_t memoryBlockSize = 0;
    size_t memoryOffset = 0;

    /// Identifies the block this allocation was suballocated from.  Two
    /// descriptors carrying the same value name the same memory.
    ///
    /// An importer needs this, because externalHandle names the BLOCK rather
    /// than the buffer: importing per buffer imports the whole block once per
    /// buffer, which for a few dozen buffers sharing one block is enough to
    /// exhaust the importing API.  Nothing else here distinguishes two blocks
    /// -- sizes collide freely, and the handle is a fresh value on every call
    /// -- and comparing handles is not portable.
    ///
    /// Opaque, comparable only among descriptors from one arena, and 0 when
    /// the buffer cannot be exported.
    uint64_t memoryBlockId = 0;

    /// Whether this is a dedicated allocation.  The importer has to match it;
    /// the parameter is not advisory, and mismatching it fails the import.
    bool dedicated = false;
};

/// \class HgiExternalBuffer
///
/// An object that produces an HgiBuffer on demand for memory the application
/// -- not Hgi -- allocated, and that owns whatever interop objects were needed
/// to get there.
///
/// This is the single abstraction the scene description carries, replacing the
/// raw handles, OS memory handles, device identities and per-buffer semaphores
/// an earlier design published as separate schema fields.  All of that becomes
/// a detail of the concrete subclass and of the HgiExternalBufferArena that
/// created it, which is what lets VK-to-VK sharing, VK-to-GL import and
/// GL-to-GL passthrough be three subclasses instead of three sets of fields.
///
/// \section Ownership
///
/// Instances are always owned by their arena through a std::shared_ptr and
/// handed out as such: the arena keeps a strong reference for reuse and
/// diagnostics, a renderer takes a strong reference for as long as it is
/// actively consuming the buffer, and the *scene description carries only a
/// weak reference* so a stale scene-index cache cannot pin GPU memory (see
/// HdExtGpuBufferSchema).  Destruction therefore happens when the last strong
/// reference drops -- but only from
/// HgiExternalBufferArena::GarbageCollect(), which defers it until the GPU
/// work that could still name the buffer has retired.
///
/// What is destroyed is the *wrapper* plus any Hgi-side interop objects it
/// built (an imported GL buffer, an imported memory object).  Whether the
/// underlying native allocation goes with it is the arena's decision: a buffer
/// the arena allocated is the arena's to free, and one it merely registered on
/// the application's behalf is not.
///
class HgiExternalBuffer
{
public:
    HGI_API
    virtual ~HgiExternalBuffer();

    /// The Hgi buffer to bind or blit through, valid for the lifetime of this
    /// object.  Returns an empty handle when the resource could not be made
    /// usable by the consuming backend -- an import that failed, say -- in
    /// which case the consumer must fall back to its own copy.
    ///
    /// Do not destroy the returned handle; this object owns it.
    HGI_API
    virtual HgiBufferHandle GetBuffer() const = 0;

    /// The size in bytes of the native allocation behind this buffer.
    size_t GetByteSize() const {
        return _byteSize;
    }

    /// Describe this buffer's memory so an application in another API can
    /// import and alias it, and return whether it can be exported at all.
    ///
    /// True only for a buffer from HgiExternalBufferArena::AllocateBuffer, on
    /// a backend that can export: those are allocated exportable for exactly
    /// this purpose.  A registered, adopted or imported buffer returns false,
    /// because its memory came from somewhere else and is not Hgi's to hand
    /// out.  False leaves \p outDesc untouched.
    ///
    /// This is the generic half of interop, and the direction a producer in
    /// another API needs.  A backend may also describe the same allocation in
    /// its own type system, where an importer on the same API can use it
    /// directly; this one is for code that must stay backend-agnostic.
    ///
    /// Default: unsupported.
    HGI_API
    virtual bool GetExportDesc(HgiExternalBufferExportDesc *outDesc) const;

    /// The arena that created this buffer.  A consumer must check this before
    /// binding: several Hgi instances can consume one scene index (two
    /// viewports, say), and a buffer belonging to another Hgi's arena is a
    /// valid-looking object whose handle names an object on a different
    /// device.  Treat "not from my Hgi" as not directly bindable and copy
    /// instead.
    HgiExternalBufferArena *GetArena() const {
        return _arena;
    }

    /// The Hgi that consumes buffers from this arena.  Convenience for the
    /// check described on GetArena().
    HGI_API
    Hgi *GetHgi() const;

    /// Attach an opaque reference this buffer holds for as long as it exists
    /// and releases when it is destroyed.  Hgi never interprets it.
    ///
    /// This is how an application whose buffer object is already reference
    /// counted meets the contract that a registered native buffer stays alive
    /// until Hgi is done with it: hand over a shared_ptr to that object and
    /// the guarantee holds by construction.
    ///
    /// It does not help an application whose buffer is owned by something it
    /// cannot refcount or defer -- a pooled viewport buffer the host
    /// application recycles on its own schedule -- and it says nothing about
    /// *reuse*: keeping an object alive does not stop its owner overwriting
    /// the bytes a submitted draw still reads.  Such a producer needs a
    /// post-retire release signal, which is deliberately not part of this API
    /// yet; until then the contract is the unenforced promise that the
    /// registered buffer outlives Hgi's use of it.
    ///
    /// The deleter runs on whichever thread drops the last reference, which is
    /// the thread that runs GarbageCollect(), so it must be thread-safe and
    /// should enqueue rather than call GPU APIs inline.
    HGI_API
    void SetKeepalive(std::shared_ptr<void> keepalive);

    /// The reference attached by SetKeepalive, or null.
    HGI_API
    std::shared_ptr<void> const &GetKeepalive() const;

protected:
    HGI_API
    HgiExternalBuffer(HgiExternalBufferArena *arena, size_t byteSize);

    /// Destroy the backend objects now, leaving GetBuffer() empty.
    /// HgiExternalBufferArena calls this when its Hgi is torn down while the
    /// buffer is still referenced: the last reference would otherwise destroy
    /// them later, through a device that no longer exists.  Called only once
    /// the device is idle.  Must be idempotent; the destructor may call it
    /// again.  Default: nothing to release.
    HGI_API
    virtual void _ReleaseResources();

private:
    friend class HgiExternalBufferArena;

    HgiExternalBuffer() = delete;
    HgiExternalBuffer(const HgiExternalBuffer &) = delete;
    HgiExternalBuffer & operator=(const HgiExternalBuffer &) = delete;

    HgiExternalBufferArena *_arena;
    size_t _byteSize;
    std::shared_ptr<void> _keepalive;
};

using HgiExternalBufferSharedPtr = std::shared_ptr<HgiExternalBuffer>;
using HgiExternalBufferWeakPtr = std::weak_ptr<HgiExternalBuffer>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HGI_EXTERNAL_BUFFER_H
