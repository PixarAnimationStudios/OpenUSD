//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_EXTERNAL_BUFFER_H
#define PXR_IMAGING_HGI_EXTERNAL_BUFFER_H

#include "pxr/pxr.h"
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

private:
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
