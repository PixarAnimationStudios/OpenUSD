//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_EXT_GPU_IMPORTED_BUFFER_H
#define PXR_IMAGING_HD_ST_EXT_GPU_IMPORTED_BUFFER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hgi/buffer.h"

#include "pxr/base/tf/token.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <tuple>

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;
class HdStResourceRegistry;

/// \class HdSt_ImportedExtGpuBuffer
///
/// Shared owner of a buffer aliasing memory that a foreign device allocated,
/// handed out by HdStResourceRegistry::GetOrCreateImportedExtGpuBuffer.
///
/// Imports are shared and expensive: two prims naming the same producer
/// allocation get the same buffer, and each import also consumes a
/// device-memory reference that only destruction returns.  So the registry
/// caches them on Key below, but holds only weak references; every holder along
/// the chain from routing to draw -- the HdStExtGpuBufferDesc, the
/// HdStExtGpuBufferSource built from it, and the HdStExtGpuBufferArrayRange it
/// is committed to -- holds a strong one.  The last holder to let go destroys
/// the buffer and removes it from the cache.  That way nothing is pinned for
/// the life of the renderer, and no holder can be left pointing at a buffer
/// somebody else decided to free.
///
class HdSt_ImportedExtGpuBuffer
{
public:
    /// The producer-stable allocation identity imports are shared on: (device,
    /// handle type, offset within the block, buffer size, OS handle).
    using Key = std::tuple<TfToken, TfToken, size_t, size_t, uint64_t>;

    HdSt_ImportedExtGpuBuffer(
        HgiBufferHandle const &handle,
        Key const &key,
        Hgi *hgi,
        HdStResourceRegistry *registry)
        : _handle(handle)
        , _key(key)
        , _hgi(hgi)
        , _registry(registry)
    {
    }

    HDST_API
    ~HdSt_ImportedExtGpuBuffer();

    HdSt_ImportedExtGpuBuffer(HdSt_ImportedExtGpuBuffer const &) = delete;
    HdSt_ImportedExtGpuBuffer &operator=(
        HdSt_ImportedExtGpuBuffer const &) = delete;

    /// The buffer aliasing the producer's memory.
    HgiBufferHandle const &GetHandle() const { return _handle; }

    /// Stop reporting to the registry, which is being destroyed.  Only reached
    /// when a holder outlives the renderer.
    void DetachFromRegistry() { _registry = nullptr; }

private:
    HgiBufferHandle _handle;
    Key _key;
    Hgi *_hgi;
    HdStResourceRegistry *_registry;
};

using HdSt_ImportedExtGpuBufferSharedPtr =
    std::shared_ptr<HdSt_ImportedExtGpuBuffer>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_EXT_GPU_IMPORTED_BUFFER_H
