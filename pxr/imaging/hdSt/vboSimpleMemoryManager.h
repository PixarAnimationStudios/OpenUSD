//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_VBO_SIMPLE_MEMORY_MANAGER_H
#define PXR_IMAGING_HD_ST_VBO_SIMPLE_MEMORY_MANAGER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/bufferArrayRange.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/strategyBase.h"

#include "pxr/imaging/hd/bufferArray.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/bufferSource.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdStResourceRegistry;

/// \class HdStVBOSimpleMemoryManager
///
/// VBO simple memory manager.
///
/// This class doesn't perform any aggregation.
///
class HdStVBOSimpleMemoryManager : public HdStAggregationStrategy
{
public:
    HdStVBOSimpleMemoryManager(HdStResourceRegistry* resourceRegistry)
        : _resourceRegistry(resourceRegistry) {}
    
    /// Factory for creating HdBufferArray managed by
    /// HdStVBOSimpleMemoryManager.
    HDST_API
    virtual HdBufferArraySharedPtr CreateBufferArray(
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs,
        HdBufferArrayUsageHint usageHint);

    /// Factory for creating HdBufferArrayRange
    HDST_API
    virtual HdBufferArrayRangeSharedPtr CreateBufferArrayRange();

    /// Returns id for given bufferSpecs to be used for aggregation
    HDST_API
    virtual HdStAggregationStrategy::AggregationId ComputeAggregationId(
        HdBufferSpecVector const &bufferSpecs,
        HdBufferArrayUsageHint usageHint) const;

    /// Returns the buffer specs from a given buffer array
    HDST_API
    virtual HdBufferSpecVector GetBufferSpecs(
        HdBufferArraySharedPtr const &bufferArray) const;

    /// Returns the size of the GPU memory used by the passed buffer array
    HDST_API
    virtual size_t GetResourceAllocation(
        HdBufferArraySharedPtr const &bufferArray, 
        VtDictionary &result) const;

protected:
    class _SimpleBufferArray;

    /// \class _SimpleBufferArrayRange
    ///
    /// Specialized buffer array range for SimpleBufferArray.
    ///
    class _SimpleBufferArrayRange final : public HdStBufferArrayRange
    {
    public:
        /// Constructor.
        _SimpleBufferArrayRange(HdStResourceRegistry* resourceRegistry)
            : HdStBufferArrayRange(resourceRegistry)
            , _bufferArray(nullptr)
            , _numElements(0) {
        }

        /// Returns true if this range is valid
        bool IsValid() const override {
            return (bool)_bufferArray;
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
        bool Resize(int numElements) override {
            _numElements = numElements;
            return _bufferArray->Resize(numElements);
        }

        /// Copy source data into buffer
        HDST_API
        void CopyData(HdBufferSourceSharedPtr const &bufferSource) override;

        /// Read back the buffer content
        HDST_API
        VtValue ReadData(TfToken const &name) const override;

        /// Returns the offset at which this range begins in the underlying
        /// buffer array in terms of elements.
        int GetElementOffset() const override {
            return 0;
        }

        /// Returns the byte offset at which this range begins in the underlying
        /// buffer array for the given resource.
        int GetByteOffset(TfToken const& resourceName) const override {
            TF_UNUSED(resourceName);
            return 0;
        }

        /// Returns the number of elements allocated
        size_t GetNumElements() const override {
            return _numElements;
        }

        /// Returns the capacity of allocated area for this range
        int GetCapacity() const {
            return _bufferArray->GetCapacity();
        }

        /// Returns the version of the buffer array.
        size_t GetVersion() const override {
            return _bufferArray->GetVersion();
        }

        /// Increment the version of the buffer array.
        void IncrementVersion() override {
            _bufferArray->IncrementVersion();
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

        /// Make this range invalid
        void Invalidate() {
            _bufferArray = NULL;
        }

    protected:
        /// Returns the aggregation container
        HDST_API
        const void *_GetAggregation() const override;

        /// Adds a new, named GPU resource and returns it.
        HDST_API
        HdStBufferResourceSharedPtr _AddResource(TfToken const& name,
                                                   HdTupleType tupleType,
                                                   int offset,
                                                   int stride);

    private:
        _SimpleBufferArray * _bufferArray;
        size_t _numElements;
    };

    using _SimpleBufferArraySharedPtr =
        std::shared_ptr<_SimpleBufferArray>;
    using _SimpleBufferArrayRangeSharedPtr =
        std::shared_ptr<_SimpleBufferArrayRange>;
    using _SimpleBufferArrayRangePtr =
        std::weak_ptr<_SimpleBufferArrayRange>;

    /// \class _SimpleBufferArray
    ///
    /// Simple buffer array (non-aggregated).
    ///
    class _SimpleBufferArray final : public HdBufferArray
    {
    public:
        /// Constructor.
        HDST_API
        _SimpleBufferArray(HdStResourceRegistry* resourceRegistry,
                           TfToken const &role,
                           HdBufferSpecVector const &bufferSpecs,
                           HdBufferArrayUsageHint usageHint);

        /// Destructor. It invalidates _range
        HDST_API
        ~_SimpleBufferArray() override;

        /// perform compaction if necessary, returns true if it becomes empty.
        HDST_API
        bool GarbageCollect() override;

        /// Debug output
        HDST_API
        void DebugDump(std::ostream &out) const override;

        /// Set to resize buffers. Actual reallocation happens on Reallocate()
        HDST_API
        bool Resize(int numElements);

        /// Performs reallocation.
        /// GLX context has to be set when calling this function.
        HDST_API
        void Reallocate(
                std::vector<HdBufferArrayRangeSharedPtr> const &ranges,
                HdBufferArraySharedPtr const &curRangeOwner) override;

        /// Returns the maximum number of elements capacity.
        HDST_API
        size_t GetMaxNumElements() const override;

        /// Returns current capacity. It could be different from numElements.
        int GetCapacity() const {
            return _capacity;
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
        HdStResourceRegistry* const _resourceRegistry;
        int _capacity;
        size_t _maxBytesPerElement;
        HgiBufferUsage _bufferUsage;

        HdStBufferResourceNamedList _resourceList;

        _SimpleBufferArrayRangeSharedPtr _GetRangeSharedPtr() const {
            return GetRangeCount() > 0
                ? std::static_pointer_cast<_SimpleBufferArrayRange>(GetRange(0).lock())
                : _SimpleBufferArrayRangeSharedPtr();
        }
    };
    
    HdStResourceRegistry* const _resourceRegistry;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_VBO_SIMPLE_MEMORY_MANAGER_H
