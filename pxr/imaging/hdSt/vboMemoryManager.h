//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_VBO_MEMORY_MANAGER_H
#define PXR_IMAGING_HD_ST_VBO_MEMORY_MANAGER_H

#include "pxr/pxr.h"

#include "pxr/base/tf/envSetting.h"

#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/strategyBase.h"
#include "pxr/imaging/hdSt/bufferArrayRange.h"

#include "pxr/imaging/hgi/enums.h"

#include "pxr/imaging/hd/bufferArray.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/bufferSource.h"

#include <list>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

HDST_API
extern TfEnvSetting<int> HD_MAX_VBO_SIZE;

class HdStResourceRegistry;

/// \class HdStVBOMemoryManager
///
/// VBO memory manager.
///
class HdStVBOMemoryManager : public HdStAggregationStrategy
{
public:
    HdStVBOMemoryManager(HdStResourceRegistry *resourceRegistry)
    : HdStAggregationStrategy()
    , _resourceRegistry(resourceRegistry) {}

    /// Factory for creating HdBufferArray managed by
    /// HdStVBOMemoryManager aggregation.
    HDST_API
    virtual HdBufferArraySharedPtr CreateBufferArray(
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs,
        HdBufferArrayUsageHint usageHint);

    /// Factory for creating HdBufferArrayRange managed by
    /// HdStVBOMemoryManager aggregation.
    HDST_API
    virtual HdBufferArrayRangeSharedPtr CreateBufferArrayRange();

    /// Returns id for given bufferSpecs to be used for aggregation
    HDST_API
    virtual AggregationId ComputeAggregationId(
        HdBufferSpecVector const &bufferSpecs,
        HdBufferArrayUsageHint usageHint) const;

    /// Returns the buffer specs from a given buffer array
    virtual HdBufferSpecVector GetBufferSpecs(
        HdBufferArraySharedPtr const &bufferArray) const;

    /// Returns the size of the GPU memory used by the passed buffer array
    virtual size_t GetResourceAllocation(
        HdBufferArraySharedPtr const &bufferArray, 
        VtDictionary &result) const;

protected:
    class _StripedBufferArray;

    /// specialized buffer array range
    class _StripedBufferArrayRange : public HdStBufferArrayRange
    {
    public:
        /// Constructor.
        _StripedBufferArrayRange(HdStResourceRegistry* resourceRegistry)
         : HdStBufferArrayRange(resourceRegistry),
           _stripedBufferArray(nullptr),
           _elementOffset(0),
           _numElements(0),
           _capacity(0)
        {
        }

        /// Destructor.
        HDST_API
        ~_StripedBufferArrayRange() override;

        /// Returns true if this range is valid
        bool IsValid() const override {
            return (bool)_stripedBufferArray;
        }

        /// Returns true is the range has been assigned to a buffer
        HDST_API
        bool IsAssigned() const override;

        /// Returns true if this bar is marked as immutable.
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

        /// Returns the relative element offset in aggregated buffer
        int GetElementOffset() const override {
            return _elementOffset;
        }

        /// Returns the byte offset at which this range begins in the underlying
        /// buffer array for the given resource.
        int GetByteOffset(TfToken const& resourceName) const override;

        /// Returns the number of elements
        size_t GetNumElements() const override {
            return _numElements;
        }

        /// Returns the version of the buffer array.
        size_t GetVersion() const override {
            return _stripedBufferArray->GetVersion();
        }

        /// Increment the version of the buffer array.
        void IncrementVersion() override {
            _stripedBufferArray->IncrementVersion();
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
        void SetElementOffset(int offset) {
            _elementOffset = offset;
        }

        /// Set the number of elements for this range.
        void SetNumElements(int numElements) {
            _numElements = numElements;
        }

        /// Returns the capacity of allocated area
        int GetCapacity() const {
            return _capacity;
        }

        /// Set the capacity of allocated area for this range.
        void SetCapacity(int capacity) {
            _capacity = capacity;
        }

        /// Make this range invalid
        void Invalidate() {
            _stripedBufferArray = NULL;
        }

    protected:
        /// Returns the aggregation container
        HDST_API
        const void *_GetAggregation() const override;

    private:
        // Returns the byte offset at which the BAR begins for the resource.
        size_t _GetByteOffset(HdStBufferResourceSharedPtr const& resource)
            const;

        // holding a weak reference to container.
        // this pointer becomes null when the StripedBufferArray gets destructed,
        // in case if any drawItem still holds this bufferRange.
        _StripedBufferArray *_stripedBufferArray;
        int _elementOffset;
        size_t _numElements;
        int _capacity;
    };

    using _StripedBufferArraySharedPtr =
        std::shared_ptr<_StripedBufferArray>;
    using _StripedBufferArrayRangeSharedPtr =
        std::shared_ptr<_StripedBufferArrayRange>;
    using _StripedBufferArrayRangePtr = 
        std::weak_ptr<_StripedBufferArrayRange>;

    /// striped buffer array
    class _StripedBufferArray : public HdBufferArray
    {
    public:
        /// Constructor.
        HDST_API
        _StripedBufferArray(HdStResourceRegistry* resourceRegistry,
                            TfToken const &role,
                            HdBufferSpecVector const &bufferSpecs,
                            HdBufferArrayUsageHint usageHint);

        /// Destructor. It invalidates _rangeList
        HDST_API
        ~_StripedBufferArray() override;

        /// perform compaction if necessary. If it becomes empty, release all
        /// resources and returns true
        HDST_API
        bool GarbageCollect() override;

        /// Debug output
        HDST_API
        void DebugDump(std::ostream &out) const override;

        /// Performs reallocation.
        /// GLX context has to be set when calling this function.
        HDST_API
        void Reallocate(
            std::vector<HdBufferArrayRangeSharedPtr> const &ranges,
            HdBufferArraySharedPtr const &curRangeOwner) override;

        /// Returns the maximum number of elements capacity.
        HDST_API
        size_t GetMaxNumElements() const override;

        /// Mark to perform reallocation on Reallocate()
        void SetNeedsReallocation() {
            _needsReallocation = true;
        }

        /// Mark to perform compaction on GarbageCollect()
        void SetNeedsCompaction() {
            _needsCompaction = true;
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
        HdStBufferResourceNamedList const& GetResources() const 
            {return _resourceList;}

        /// Reconstructs the bufferspecs and returns it (for buffer splitting)
        HDST_API
        HdBufferSpecVector GetBufferSpecs() const;

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

        HdStResourceRegistry* _resourceRegistry;
        bool _needsCompaction;
        int _totalCapacity;
        size_t _maxBytesPerElement;
        HgiBufferUsage _bufferUsage;

        HdStBufferResourceNamedList _resourceList;

        // Helper routine to cast the range shared pointer.
        _StripedBufferArrayRangeSharedPtr _GetRangeSharedPtr(size_t idx) const {
            return std::static_pointer_cast<_StripedBufferArrayRange>(GetRange(idx).lock());
        }
    };
    
    HdStResourceRegistry* _resourceRegistry;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_VBO_MEMORY_MANAGER_H
