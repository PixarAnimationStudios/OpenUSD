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
#include "pxr/imaging/hgi/externalBuffer.h"

#include <atomic>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdStExtGpuBufferArrayRange
///
/// An HdStBufferArrayRange that binds application-owned GPU buffers directly,
/// with no copy: Storm's draw dispatch reads the producer's buffers where they
/// already are.
///
/// It holds a strong reference to each buffer for as long as it is bound --
/// which is until the scene updates and replaces it -- and owns nothing else.
/// The GPU memory belongs to the producer or to the arena that allocated it;
/// see HgiExternalBuffer.
///
/// \section Not a real range
///
/// Most of the HdBufferArrayRange contract assumes the range owns storage it
/// can grow and write into. This one owns none, so the operations that would
/// mutate storage -- Resize, CopyData, SetBufferArray -- are coding errors
/// rather than silent no-ops. Reaching them means a caller aggregated an
/// external source together with an ordinary one, or handed this range to a
/// memory manager, and the aggregation path is supposed to make that
/// impossible; a silent no-op would leave data quietly unwritten instead of
/// saying so.
///
class HdStExtGpuBufferArrayRange final : public HdStBufferArrayRange
{
public:
    HDST_API
    HdStExtGpuBufferArrayRange(
        HdStResourceRegistry *resourceRegistry);

    HDST_API
    ~HdStExtGpuBufferArrayRange() override;

    /// Bind the buffer described by \p desc under \p name, appending a new
    /// resource.
    HDST_API
    void SetExternalResource(
        TfToken const &name,
        HdStExtGpuBufferDesc const &desc);

    /// Drop every bound resource, releasing this range's references to the
    /// buffers.
    HDST_API
    void ReleaseExternalResources();

    /// Rebind \p sources in place, reusing the existing HdStBufferResource
    /// objects so that draw batches are not invalidated.
    ///
    /// Updates a resource whose name is already bound and appends one that is
    /// not, leaving bound resources absent from \p sources alone. Returns
    /// false when a source cannot be applied in place at all -- an immutable
    /// property such as the tuple type or the element offset changed -- in
    /// which case the caller rebuilds the range from scratch.
    HDST_API
    bool UpdateExternalResources(
        HdBufferSourceSharedPtrVector const &sources);

    /// The distinct arenas the currently bound buffers came from, appended to
    /// \p arenas. Storm brackets its access to these with the arenas'
    /// semaphores at commit time; see HdStResourceRegistry.
    HDST_API
    void GetArenas(std::vector<HgiExternalBufferArena *> *arenas) const;

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
    // Index of the resource bound under \p name, or -1.
    int _FindResource(TfToken const &name) const;

    // Rebind the resource at \p index to \p desc, or return false if an
    // immutable property of the HdStBufferResource would have to change.
    bool _UpdateResource(size_t index, HdStExtGpuBufferDesc const &desc);

    HdStBufferResourceNamedList _resources;
    // One strong reference per entry in _resources, same order: what keeps the
    // bound buffers alive for as long as they are bound.
    std::vector<HgiExternalBufferSharedPtr> _externalBuffers;
    size_t _numElements;
    std::atomic<size_t> _version;
    bool _valid;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_EXT_GPU_BUFFER_ARRAY_RANGE_H
