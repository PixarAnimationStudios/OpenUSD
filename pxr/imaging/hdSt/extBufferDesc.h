//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_EXT_BUFFER_DESC_H
#define PXR_IMAGING_HD_ST_EXT_BUFFER_DESC_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/extGpuBufferSchema.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hgi/buffer.h"
#include "pxr/imaging/hgi/externalBuffer.h"
#include "pxr/imaging/hgi/externalBufferArena.h"

#include <cstddef>
#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;

/// \struct HdStExtGpuBufferDesc
///
/// Storm-private, decoded form of an HdExtGpuBufferSchema: the stream's layout
/// plus a STRONG reference to the buffer it lives in.
///
/// The strong reference is the point. What travels through the scene index is
/// weak, so a stale scene-index cache cannot pin GPU memory; Storm upgrades it
/// here, at the moment it decides to consume the buffer, and holds it for as
/// long as it might bind or read it. Nothing else is needed to keep the buffer
/// alive, and nothing here may free it -- which is what replaced the earlier
/// arrangement of native handles, device identities, an import cache and a
/// separate refcounting object.
///
struct HdStExtGpuBufferDesc
{
    /// Element type and tuple count. Named tupleType to match HdBufferSpec and
    /// HdBufferSource::GetTupleType(); comes from the schema's elementType.
    HdTupleType tupleType = {HdTypeInvalid, 0};

    size_t numElements = 0;

    /// This stream's offset within the buffer, and the bytes between its
    /// elements. A stride wider than the element size means interleaved data,
    /// which cannot be aliased and has to be copied element by element.
    size_t byteOffset = 0;
    size_t byteStride = 0;

    /// Whether the producer permits binding the buffer directly. Permission
    /// rather than instruction: Storm may still copy, and does when the layout
    /// or the buffer's provenance rules out aliasing.
    bool allowDirectBind = false;

    /// The buffer being shared, owned for as long as this descriptor lives.
    HgiExternalBufferSharedPtr externalBuffer;

    /// The buffer to bind or blit from, or an empty handle when the resource
    /// turned out to be unusable.
    HgiBufferHandle GetHandle() const
    {
        return externalBuffer ? externalBuffer->GetBuffer() : HgiBufferHandle();
    }

    /// The arena the buffer came from, whose semaphores bracket Storm's access
    /// to it. Null when there is no buffer.
    HgiExternalBufferArena *GetArena() const
    {
        return externalBuffer ? externalBuffer->GetArena() : nullptr;
    }

    /// Decode the renderer-agnostic HdExtGpuBufferSchema for a consumer
    /// running on \p hgi.
    ///
    /// Returns nullopt -- and the caller falls back to the CPU primvar -- when
    /// the schema is incomplete, when the weak reference has expired because
    /// the producer withdrew the buffer, when the buffer belongs to a
    /// different Hgi, or when it has no usable GPU resource.
    ///
    /// The foreign-Hgi check is not hypothetical: several renderers can consume
    /// one scene index -- two viewports on different devices being the ordinary
    /// case -- and a buffer from another Hgi's arena is a valid-looking object
    /// whose handle names something on a device we cannot reach.
    static std::optional<HdStExtGpuBufferDesc>
    FromSchema(const HdExtGpuBufferSchema &schema, Hgi *hgi)
    {
        if (!schema || !schema.IsComplete() || !hgi) {
            return std::nullopt;
        }

        const HdExternalBufferDataSourceHandle resourceDs =
            schema.GetExternalResource();
        if (!resourceDs) {
            return std::nullopt;
        }

        HdStExtGpuBufferDesc d;

        // Upgrade weak to strong for as long as we hold this descriptor. An
        // expired reference is not an error: a producer is entitled to
        // withdraw a buffer between publishing it and our getting here.
        d.externalBuffer = resourceDs->GetTypedValue(0.0f).buffer.lock();
        if (!d.externalBuffer) {
            return std::nullopt;
        }
        if (d.externalBuffer->GetHgi() != hgi) {
            return std::nullopt;
        }
        if (!d.externalBuffer->GetBuffer()) {
            return std::nullopt;
        }

        d.numElements = schema.GetNumElements()->GetTypedValue(0.0f);
        d.tupleType = schema.GetElementType()->GetTypedValue(0.0f);

        // Layout members are individually optional; absent means 0 / false.
        if (const HdSizetDataSourceHandle s = schema.GetByteOffset()) {
            d.byteOffset = s->GetTypedValue(0.0f);
        }
        if (const HdSizetDataSourceHandle s = schema.GetByteStride()) {
            d.byteStride = s->GetTypedValue(0.0f);
        }
        if (const HdBoolDataSourceHandle s = schema.GetAllowDirectBind()) {
            d.allowDirectBind = s->GetTypedValue(0.0f);
        }

        // Bounds check now, before anything is committed to it: the producer
        // told us how big the buffer is by way of the resource itself, so this
        // no longer depends on an optional schema member being present.
        const size_t elemSize = HdDataSizeOfTupleType(d.tupleType);
        const size_t stride = d.byteStride > 0 ? d.byteStride : elemSize;
        if (d.byteOffset + d.numElements * stride >
                d.externalBuffer->GetByteSize()) {
            return std::nullopt;
        }

        return d;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_EXT_BUFFER_DESC_H
