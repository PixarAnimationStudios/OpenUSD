//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_EXT_GPU_BUFFER_ARRAY_RANGE_H
#define PXR_IMAGING_HD_ST_EXT_GPU_BUFFER_ARRAY_RANGE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/bufferArrayRange.h"
#include "pxr/imaging/hdSt/bufferResource.h"
#include "pxr/imaging/hdSt/extBufferDesc.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hgi/buffer.h"

#include <atomic>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HdStExtGpuBuffer;

/// \class HdStExtGpuBufferArrayRange
///
/// A lightweight HdStBufferArrayRange that wraps externally-owned GPU
/// buffers for zero-copy direct binding.  Instead of copying data into
/// a Storm-managed buffer, Storm's draw dispatch binds the external
/// HgiBufferHandle directly.
///
/// This BAR does not own the underlying GPU memory — the producer
/// manages its lifetime.  On the import route it does share ownership of the
/// consumer-side buffer aliasing that memory, since the import consumes a
/// device-memory reference that has to be returned when nobody is bound to it
/// any more.
///
class HdStExtGpuBufferArrayRange final : public HdStBufferArrayRange
{
public:
    HDST_API
    HdStExtGpuBufferArrayRange(
        HdStResourceRegistry *resourceRegistry);

    HDST_API
    ~HdStExtGpuBufferArrayRange() override;

    /// Bind an external buffer resource by primvar name, using whichever route
    /// the consumer's routing step resolved: the buffer it imported from the
    /// producer's memory, a backend buffer adopting the producer's native
    /// handle, or a plain non-owning wrapper around it.
    HDST_API
    void SetExternalResource(
        TfToken const &name,
        HdStExtGpuBufferDesc const &desc);

    /// Reset all external resources and prepare for re-population via
    /// SetExternalResource.
    HDST_API
    void ReleaseExternalResources();

    /// Update existing external resources in-place when the resource names
    /// and count match.  Avoids heap allocation by reusing existing
    /// HdStExtGpuBuffer and HdStBufferResource objects.
    /// Returns true if in-place update succeeded, false if a full rebuild
    /// (Release + Set) is needed.
    HDST_API
    bool UpdateExternalResources(
        HdBufferSourceSharedPtrVector const &sources);

    /// Merge a subset of external resources into this BAR.
    /// For each source, if a resource with the same name already exists,
    /// update it in-place; otherwise append it as a new resource.
    /// Unlike UpdateExternalResources, this does not require the source
    /// count or names to match exactly — existing resources not present
    /// in \p sources are preserved unchanged.
    /// Returns true if all sources were successfully merged.
    HDST_API
    bool MergeExternalResources(
        HdBufferSourceSharedPtrVector const &sources);

    // ---- HdBufferArrayRange pure virtuals ----

    HDST_API bool IsValid() const override;
    HDST_API bool IsAssigned() const override;
    HDST_API bool IsImmutable() const override;
    HDST_API bool RequiresStaging() const override;
    HDST_API bool Resize(int numElements) override;
    HDST_API void CopyData(HdBufferSourceSharedPtr const &bufferSource) override;
    HDST_API VtValue ReadData(TfToken const &name) const override;
    HDST_API int GetElementOffset() const override;
    HDST_API int GetByteOffset(TfToken const &resourceName) const override;
    HDST_API size_t GetNumElements() const override;
    HDST_API size_t GetVersion() const override;
    HDST_API void IncrementVersion() override;
    HDST_API size_t GetMaxNumElements() const override;
    HDST_API HdBufferArrayUsageHint GetUsageHint() const override;
    HDST_API void SetBufferArray(HdBufferArray *bufferArray) override;
    HDST_API void DebugDump(std::ostream &out) const override;

    // ---- HdStBufferArrayRange pure virtuals ----

    HDST_API HdStBufferResourceSharedPtr GetResource() const override;
    HDST_API HdStBufferResourceSharedPtr GetResource(
        TfToken const &name) override;
    HDST_API HdStBufferResourceNamedList const &GetResources() const override;

protected:
    HDST_API const void *_GetAggregation() const override;

private:
    // One wrapper per external resource. Exactly one field is set, and the
    // field says who frees it:
    //  - generic:  GL path -- a plain non-owning HgiBuffer we `delete` on
    //    release (bound via the resource binder's generic GetRawResource path).
    //  - native:   a real backend buffer adopted via Hgi (e.g. Vulkan, whose
    //    vertex binding downcasts to the concrete HgiBuffer); freed via the
    //    resource registry's Hgi->DestroyBuffer.
    //  - imported: a real backend buffer aliasing memory imported from a
    //    foreign device, shared with any other range naming the same producer
    //    allocation (imports are too expensive to redo per Sync). Refcounted:
    //    releasing this reference frees the buffer if we were its last holder.
    struct _OwnedExtBuffer {
        HdStExtGpuBuffer *generic = nullptr;
        HgiBufferHandle   native;
        HdSt_ImportedExtGpuBufferSharedPtr imported;
    };

    // Release every owned wrapper (generic via delete, native via Hgi,
    // imported by dropping our share of it) and clear.
    void _DestroyOwnedBuffers();

    HdStBufferResourceNamedList _resources;
    std::vector<_OwnedExtBuffer> _ownedExternalGpuBuffers;
    size_t _numElements;
    std::atomic<size_t> _version;
    bool _valid;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_EXT_GPU_BUFFER_ARRAY_RANGE_H
