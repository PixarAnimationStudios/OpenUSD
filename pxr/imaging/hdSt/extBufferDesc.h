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
#include "pxr/imaging/hdSt/extGpuImportedBuffer.h"
#include "pxr/imaging/hd/extGpuBufferSchema.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hgi/buffer.h"

#include "pxr/base/tf/token.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

/// \struct HdStExtGpuBufferDesc
///
/// Storm-private, decoded form of an HdExtGpuBufferSchema.  This is the place
/// the external-buffer fields are flattened into a POD.  Constructed by Storm's
/// enrichment step during \c _PopulateVertexPrimvars via \c FromSchema and
/// never placed into \c VtValue.
///
struct HdStExtGpuBufferDesc
{
    // --- Decoded descriptor fields (the hd-level POD) ---

    /// Element type and tuple count.  Named tupleType to match HdBufferSpec /
    /// HdBufferSource::GetTupleType(); sourced from the schema's elementType
    /// member.
    HdTupleType tupleType = {HdTypeInvalid, 0};
    size_t   numElements = 0;
    size_t   byteOffset = 0;
    size_t   byteStride = 0;
    TfToken  backendApi;
    uint64_t rawHandle = 0;
    size_t   rawHandleByteSize = 0;
    bool     directBindable = false;

    /// The logical device whose handle namespace rawHandle belongs to (see
    /// Hgi::GetLogicalDeviceId).  0 means the producer did not say, which is
    /// treated as a match.
    uint64_t logicalDeviceId = 0;

    // --- Foreign-memory import cluster ---
    // Describes the OS-shareable memory allocation behind the buffer, for
    // consumers whose device cannot use rawHandle directly.  All zero/empty
    // when the producer publishes only a native handle.
    uint64_t externalMemoryHandle = 0;
    TfToken  externalHandleType;
    size_t   memoryBlockSize = 0;
    size_t   memoryOffset = 0;
    bool     dedicated = false;
    TfToken  deviceUuid;

    // Non-owning HgiBuffer wrapper around rawHandle, used as the source buffer
    // in GPU-to-GPU blit ops.  Owned by the HdStExtGpuBufferSource that
    // populates this descriptor.
    HgiBufferHandle cachedHgiHandle;

    // The consumer's own buffer aliasing the imported memory, resolved during
    // routing (HdSt_TryCreateExtGpuBufferSource) when the import path is taken
    // and null when the native handle is adopted instead.  Shared with any
    // other prim naming the same producer allocation; holding it here is what
    // keeps the import alive between routing and the commit that hands it to a
    // buffer array range.
    HdSt_ImportedExtGpuBufferSharedPtr importedBuffer;

    /// The imported buffer aliasing the producer's memory, or an empty handle
    /// when the native handle was adopted instead.
    HgiBufferHandle GetImportedHandle() const
    {
        return importedBuffer ? importedBuffer->GetHandle() : HgiBufferHandle();
    }

    // The data source(s) that named the producer's memory, retained and handed
    // to the buffer that binds it (see HgiBuffer::SetKeepalive).  A producer
    // whose data source owns its allocation gets lifetime tracking out of this
    // without any schema member: retention follows the published value, so a
    // scene index that substitutes a different handle releases the allocation
    // the old one named, which is the correct response.  A producer publishing
    // both routes has both retained, since which one we bind is decided after
    // this descriptor is built and its deleter may sit on either.
    std::shared_ptr<void> producerKeepalive;

    /// True when this descriptor carries enough information to import the
    /// producer's memory into another device.
    bool CanImport() const
    {
        return externalMemoryHandle != 0 &&
               !externalHandleType.IsEmpty() &&
               memoryBlockSize > 0;
    }

    /// Decode the renderer-agnostic HdExtGpuBufferSchema into this Storm-local
    /// POD.  Returns nullopt when the schema is undefined or incomplete
    /// (missing backendApi / numElements / elementType, or neither a rawHandle
    /// nor the import cluster), in which case the caller falls back to the CPU
    /// primvar path.  Optional members (byteOffset / byteStride /
    /// rawHandleByteSize / directBindable / logicalDeviceId / the import
    /// cluster) default to 0 / false / empty when absent.
    static std::optional<HdStExtGpuBufferDesc>
    FromSchema(const HdExtGpuBufferSchema &schema)
    {
        if (!schema || !schema.IsComplete()) {
            return std::nullopt;
        }

        HdStExtGpuBufferDesc d;
        d.backendApi  = schema.GetBackendApi()->GetTypedValue(0.0f);
        d.numElements = schema.GetNumElements()->GetTypedValue(0.0f);
        d.tupleType   = schema.GetElementType()->GetTypedValue(0.0f);

        // Both handles are optional individually: a producer names the buffer
        // by either route, or by both.  Every data source that names memory we
        // might bind is retained as the keepalive, because routing happens
        // after this and the producer's deleter may sit on either one.
        const HdUInt64DataSourceHandle rawDs = schema.GetRawHandle();
        const HdUInt64DataSourceHandle importDs =
            schema.GetExternalMemoryHandle();
        if (rawDs) {
            d.rawHandle = rawDs->GetTypedValue(0.0f);
        }
        if (importDs) {
            d.externalMemoryHandle = importDs->GetTypedValue(0.0f);
        }
        if (rawDs && importDs) {
            d.producerKeepalive = std::make_shared<
                std::pair<HdDataSourceBaseHandle, HdDataSourceBaseHandle>>(
                    rawDs, importDs);
        } else if (rawDs) {
            d.producerKeepalive = std::static_pointer_cast<void>(rawDs);
        } else if (importDs) {
            d.producerKeepalive = std::static_pointer_cast<void>(importDs);
        }
        if (const HdTokenDataSourceHandle s = schema.GetExternalHandleType()) {
            d.externalHandleType = s->GetTypedValue(0.0f);
        }
        if (const HdSizetDataSourceHandle s = schema.GetMemoryBlockSize()) {
            d.memoryBlockSize = s->GetTypedValue(0.0f);
        }
        if (const HdSizetDataSourceHandle s = schema.GetMemoryOffset()) {
            d.memoryOffset = s->GetTypedValue(0.0f);
        }
        if (const HdBoolDataSourceHandle s = schema.GetDedicated()) {
            d.dedicated = s->GetTypedValue(0.0f);
        }
        if (const HdTokenDataSourceHandle s = schema.GetDeviceUuid()) {
            d.deviceUuid = s->GetTypedValue(0.0f);
        }
        if (const HdUInt64DataSourceHandle s = schema.GetLogicalDeviceId()) {
            d.logicalDeviceId = s->GetTypedValue(0.0f);
        }

        if (const HdSizetDataSourceHandle s = schema.GetRawHandleByteSize()) {
            d.rawHandleByteSize = s->GetTypedValue(0.0f);
        }
        if (const HdSizetDataSourceHandle s = schema.GetByteOffset()) {
            d.byteOffset = s->GetTypedValue(0.0f);
        }
        if (const HdSizetDataSourceHandle s = schema.GetByteStride()) {
            d.byteStride = s->GetTypedValue(0.0f);
        }
        if (const HdBoolDataSourceHandle s = schema.GetDirectBindable()) {
            d.directBindable = s->GetTypedValue(0.0f);
        }
        return d;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_EXT_BUFFER_DESC_H
