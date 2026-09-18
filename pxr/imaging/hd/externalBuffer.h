//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_EXTERNAL_BUFFER_H
#define PXR_IMAGING_HD_EXTERNAL_BUFFER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"

#include <cstdint>
#include <iosfwd>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

/// Forward declaration of the Hgi type, on purpose.
///
/// The payload below names an Hgi object but hd neither includes nor links
/// hgi: only a producer and the consuming renderer ever dereference it, and
/// both of those live above the boundary where hgi is available. In between,
/// the value is transported opaquely, which is what keeps this from turning
/// into an hd-on-hgi dependency.
class HgiExternalBuffer;

/// \class HdExternalBufferPtr
///
/// A WEAK reference to an HgiExternalBuffer, as carried on an
/// HdExtGpuBufferSchema.
///
/// Weak rather than strong, and that is the whole design. Scene indices cache,
/// flatten and copy the containers they pass along, and any of those caches
/// may outlive the geometry it described; a strong reference in the scene
/// description would let a forgotten cache entry pin GPU memory for the life
/// of the renderer. The arena that created the buffer holds the reference that
/// keeps it alive, and a renderer takes its own for exactly as long as it is
/// consuming -- so an expired pointer here means "that buffer is gone, fall
/// back", not a dangling read.
///
/// This is a struct rather than a bare std::weak_ptr so it can carry the
/// equality and streaming operators VtValue expects of anything it holds;
/// weak_ptr has neither.
///
struct HdExternalBufferPtr
{
    HdExternalBufferPtr() = default;

    explicit HdExternalBufferPtr(
        std::weak_ptr<HgiExternalBuffer> const &buffer_)
        : buffer(buffer_)
    {
    }

    /// The buffer, or an expired reference.
    std::weak_ptr<HgiExternalBuffer> buffer;

    /// Whether both point at the same buffer, including the case where both
    /// have expired but referred to the same one. Ownership-based, so this
    /// needs no access to the pointed-to type.
    bool operator==(HdExternalBufferPtr const &rhs) const {
        return !buffer.owner_before(rhs.buffer) &&
               !rhs.buffer.owner_before(buffer);
    }

    bool operator!=(HdExternalBufferPtr const &rhs) const {
        return !(*this == rhs);
    }
};

template <class HashState>
inline void
TfHashAppend(HashState &h, HdExternalBufferPtr const &ptr)
{
    // The buffer's address identifies it. Taken by locking rather than from
    // the control block, because weak_ptr::owner_hash is C++26; locking is
    // legal for an incomplete type, since the pointer is only copied and
    // destroyed here, never dereferenced.
    //
    // An expired reference hashes as null, so two buffers that have both gone
    // away collide. That is acceptable for a hash and consistent with
    // equality, which compares ownership.
    h.Append(reinterpret_cast<uintptr_t>(ptr.buffer.lock().get()));
}

HD_API
std::ostream &operator<<(std::ostream &out, HdExternalBufferPtr const &ptr);

/// A data source carrying the buffer a producer is sharing. See
/// HdExtGpuBufferSchema.
using HdExternalBufferDataSource =
    HdTypedSampledDataSource<HdExternalBufferPtr>;
using HdExternalBufferDataSourceHandle = HdExternalBufferDataSource::Handle;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_EXTERNAL_BUFFER_H
