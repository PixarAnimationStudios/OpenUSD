//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_INTERLEAVED_MEMORY_MANAGER_H
#define PXR_IMAGING_HD_ST_INTERLEAVED_MEMORY_MANAGER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/bufferArrayRange.h"
#include "pxr/imaging/hdSt/strategyBase.h"

#include "pxr/imaging/hd/bufferArray.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hgi/buffer.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/token.h"

#include <memory>
#include <list>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

class HdStResourceRegistry;
struct HgiBufferCpuToGpuOp;

/// \class HdStInterleavedMemoryManager
///
/// Interleaved memory manager (base class).
///
class HdStInterleavedMemoryManager : public HdStAggregationStrategy {
protected:
    class _StripedInterleavedBuffer;

    /// specialized buffer array range
    class _StripedInterleavedBufferRange : public HdStBufferArrayRange
    {
    public:
        /// Constructor.
        _StripedInterleavedBufferRange(HdStResourceRegistry* resourceRegistry)
        : HdStBufferArrayRange(resourceRegistry)
        , _stripedBuffer(nullptr)
        , _index(NOT_ALLOCATED)
        , _numElements(1)
        , _capacity(0) {}

        /// Destructor.
        HDST_API
        ~_StripedInterleavedBufferRange() override;

        /// Returns true if this range is valid
        bool IsValid() const override {
            // note: a range is valid even its index is NOT_ALLOCATED.
            return (bool)_stripedBuffer;
        }

        /// Returns true is the range has been assigned to a buffer
        HDST_API
        bool IsAssigned() const override;

        /// Returns true if this range is marked as immutable.
        bool IsImmutable() const override;

        /// Returns true if this needs a staging buffer for CPU to GPU copies.
        bool RequiresStaging() const override;

        /// Resize memory area for this range. Returns true if it causes container
        /// buffer reallocation.
        HDST_API
        bool Resize(int numElements) override;

        /// Copy source data into buffer
        HDST_API
        void CopyData(HdBufferSourceSharedPtr const &bufferSource) override;

        /// Read back the buffer content
        HDST_API
        VtValue ReadData(TfToken const &name) const override;

        /// Returns the offset at which this range begins in the underlying  
        /// buffer array in terms of elements.
        int GetElementOffset() const override {
            return _index;
        }

        /// Returns the byte offset at which this range begins in the underlying
        /// buffer array for the given resource.
        int GetByteOffset(TfToken const& resourceName) const override {
            TF_UNUSED(resourceName);
            if (!TF_VERIFY(_stripedBuffer) ||
                !TF_VERIFY(_index != NOT_ALLOCATED)) return 0;
            return _stripedBuffer->GetStride() * _index;
        }

        /// Returns the number of elements
        size_t GetNumElements() const override {
            return _numElements;
        }

        /// Returns the version of the buffer array.
        size_t GetVersion() const override {
            return _stripedBuffer->GetVersion();
        }

        int GetElementStride() const override {
            return _stripedBuffer->GetElementStride();
        }

        /// Increment the version of the buffer array.
        void IncrementVersion() override {
            _stripedBuffer->IncrementVersion();
        }

        /// Returns the max number of elements
        HDST_API
        size_t GetMaxNumElements() const override;

        /// Returns the usage hint from the underlying buffer array
        HDST_API
        HdBufferArrayUsageHint GetUsageHint() const override;

        /// Returns the GPU resource. If the buffer array contains more than one
        /// resource, this method raises a coding error.
        HDST_API
        HdStBufferResourceSharedPtr GetResource() const override;

        /// Returns the named GPU resource.
        HDST_API
        HdStBufferResourceSharedPtr GetResource(TfToken const& name) override;

        /// Returns the list of all named GPU resources for this bufferArrayRange.
        HDST_API
        HdStBufferResourceNamedList const& GetResources() const override;

        /// Sets the buffer array associated with this buffer;
        HDST_API
        void SetBufferArray(HdBufferArray *bufferArray) override;

        /// Debug dump
        HDST_API
        void DebugDump(std::ostream &out) const override;

        /// Set the relative offset for this range.
        void SetIndex(int index) {
            _index = index;
        }

        /// Make this range invalid
        void Invalidate() {
            _stripedBuffer = nullptr;
        }

         /// Returns the capacity of allocated area
        int GetCapacity() const {
            return _capacity;
        }

        /// Set the capacity of allocated area for this range.
        void SetCapacity(int capacity) {
            _capacity = capacity;
        }

    protected:
        /// Returns the aggregation container
        HDST_API
        const void *_GetAggregation() const override;

    private:
        enum { NOT_ALLOCATED = -1 };
        _StripedInterleavedBuffer *_stripedBuffer;
        int _index;
        size_t _numElements;
        int _capacity;
    };

    using _StripedInterleavedBufferSharedPtr =
        std::shared_ptr<_StripedInterleavedBuffer>;
    using _StripedInterleavedBufferRangeSharedPtr =
        std::shared_ptr<_StripedInterleavedBufferRange>;
    using _StripedInterleavedBufferRangePtr =
        std::weak_ptr<_StripedInterleavedBufferRange>;

    /// striped buffer
    class _StripedInterleavedBuffer : public HdBufferArray {
    public:
        /// Constructor.
        HDST_API
        _StripedInterleavedBuffer(HdStInterleavedMemoryManager* mgr,
                                  HdStResourceRegistry* resourceRegistry,
                                  TfToken const &role,
                                  HdBufferSpecVector const &bufferSpecs,
                                  HdBufferArrayUsageHint usageHint,
                                  int bufferOffsetAlignment,
                                  int structAlignment,
                                  size_t maxSize,
                                  TfToken const &garbageCollectionPerfToken);

        /// Destructor. It invalidates _rangeList
        HDST_API
        virtual ~_StripedInterleavedBuffer();

        /// perform compaction if necessary, returns true if it becomes empty.
        HDST_API
        virtual bool GarbageCollect();

        /// Debug output
        HDST_API
        virtual void DebugDump(std::ostream &out) const;

        /// Performs reallocation.
        /// GLX context has to be set when calling this function.
        HDST_API
        virtual void Reallocate(
                std::vector<HdBufferArrayRangeSharedPtr> const &ranges,
                HdBufferArraySharedPtr const &curRangeOwner);

        /// Mark to perform reallocation on Reallocate()
        void SetNeedsReallocation() {
            _needsReallocation = true;
        }

        /// Mark to perform compaction on GarbageCollect()
        void SetNeedsCompaction() {
            _needsCompaction = true;
        }

        /// Returns the stride.
        size_t GetStride() const {
            return _stride;
        }
        
        size_t GetElementStride() const {
            return _elementStride;
        }

        /// Returns the GPU resource. If the buffer array contains more
        /// than one resource, this method raises a coding error.
        HDST_API
        HdStBufferResourceSharedPtr GetResource() const;

        /// Returns the named GPU resource. This method returns the first found
        /// resource. In HD_SAFE_MODE it checks all underlying GL buffers
        /// in _resourceMap and raises a coding error if there are more than
        /// one GL buffers exist.
        HDST_API
        HdStBufferResourceSharedPtr GetResource(TfToken const& name);

        /// Returns the list of all named GPU resources for this bufferArray.
        HdStBufferResourceNamedList const& GetResources() const {return _resourceList;}

        /// Reconstructs the bufferspecs and returns it (for buffer splitting)
        HDST_API
        HdBufferSpecVector GetBufferSpecs() const;

        HdStInterleavedMemoryManager*
        GetManager() const {
            return _manager;
        }

    protected:
        HDST_API
        void _DeallocateResources();

        /// Adds a new, named GPU resource and returns it.
        HDST_API
        HdStBufferResourceSharedPtr _AddResource(TfToken const& name,
                                                   HdTupleType tupleType,
                                                   int offset,
                                                   int stride);

    private:
        HdStInterleavedMemoryManager* _manager;
        HdStResourceRegistry* const _resourceRegistry;
        bool _needsCompaction;
        size_t _stride;
        int _bufferOffsetAlignment;  // ranged binding offset alignment
        size_t _maxSize;             // maximum size of single buffer

        // _elementStride is similar to _stride but does account for any buffer
        // offset alignment. If there are multiple elements in a buffer, this 
        // will be the actual byte distance between the two values. 
        // For example, imagine there are three buffers (A, B, C) in a buffer 
        // array, and each buffer has two elements. 
        // +------------------------------------------------------------+
        // | a1 | b1 | c1 | a2 | b2 | c2 | padding for offset alignment |
        // +------------------------------------------------------------+
        // The _stride will be the size of a1 + b1 + c1 + padding, while the
        // _elementStride will be the size of a1 + b1 + c1.
        size_t _elementStride;

        HgiBufferUsage _bufferUsage; 

        HdStBufferResourceNamedList _resourceList;

        _StripedInterleavedBufferRangeSharedPtr _GetRangeSharedPtr(size_t idx) const {
            return std::static_pointer_cast<_StripedInterleavedBufferRange>(GetRange(idx).lock());
        }

    };
    
    HdStInterleavedMemoryManager(HdStResourceRegistry* resourceRegistry)
        : _resourceRegistry(resourceRegistry) {}

    /// Factory for creating HdBufferArrayRange
    HdBufferArrayRangeSharedPtr CreateBufferArrayRange() override;

    /// Returns the buffer specs from a given buffer array
    HdBufferSpecVector GetBufferSpecs(
        HdBufferArraySharedPtr const &bufferArray) const override;

    /// Returns the size of the GPU memory used by the passed buffer array
    size_t GetResourceAllocation(
        HdBufferArraySharedPtr const &bufferArray, 
        VtDictionary &result) const override;
    
    HdStResourceRegistry* const _resourceRegistry;
};

class HdStInterleavedUBOMemoryManager : public HdStInterleavedMemoryManager {
public:
    HdStInterleavedUBOMemoryManager(HdStResourceRegistry* resourceRegistry)
    : HdStInterleavedMemoryManager(resourceRegistry) {}

    /// Factory for creating HdBufferArray managed by
    /// HdStVBOMemoryManager aggregation.
    HDST_API
    virtual HdBufferArraySharedPtr CreateBufferArray(
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs,
        HdBufferArrayUsageHint usageHint);

    /// Returns id for given bufferSpecs to be used for aggregation
    HDST_API
    virtual AggregationId ComputeAggregationId(
        HdBufferSpecVector const &bufferSpecs,
        HdBufferArrayUsageHint usageHint) const;
};

class HdStInterleavedSSBOMemoryManager : public HdStInterleavedMemoryManager {
public:
    HdStInterleavedSSBOMemoryManager(HdStResourceRegistry* resourceRegistry)
    : HdStInterleavedMemoryManager(resourceRegistry) {}

    /// Factory for creating HdBufferArray managed by
    /// HdStVBOMemoryManager aggregation.
    HDST_API
    virtual HdBufferArraySharedPtr CreateBufferArray(
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs,
        HdBufferArrayUsageHint usageHint);

    /// Returns id for given bufferSpecs to be used for aggregation
    HDST_API
    virtual AggregationId ComputeAggregationId(
        HdBufferSpecVector const &bufferSpecs,
        HdBufferArrayUsageHint usageHint) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_INTERLEAVED_MEMORY_MANAGER_H
