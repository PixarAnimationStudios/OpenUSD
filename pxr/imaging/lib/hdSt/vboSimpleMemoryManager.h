//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef HDST_VBO_SIMPLE_MEMORY_MANAGER_H
#define HDST_VBO_SIMPLE_MEMORY_MANAGER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/bufferArrayRangeGL.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/strategyBase.h"
#include "pxr/imaging/hd/bufferArray.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/bufferSource.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \class HdStVBOSimpleMemoryManager
///
/// VBO simple memory manager.
///
/// This class doesn't perform any aggregation.
///
class HdStVBOSimpleMemoryManager : public HdAggregationStrategy {
public:
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
    virtual HdAggregationStrategy::AggregationId ComputeAggregationId(
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
    class _SimpleBufferArray;

    /// \class _SimpleBufferArrayRange
    ///
    /// Specialized buffer array range for SimpleBufferArray.
    ///
    class _SimpleBufferArrayRange : public HdStBufferArrayRangeGL
    {
    public:
        /// Constructor.
        _SimpleBufferArrayRange() :
            _bufferArray(nullptr), _numElements(0) {
        }

        /// Returns true if this range is valid
        virtual bool IsValid() const {
            return (bool)_bufferArray;
        }

        /// Returns true is the range has been assigned to a buffer
        HDST_API
        virtual bool IsAssigned() const;

        /// Returns true if this range is marked as immutable.
        virtual bool IsImmutable() const;

        /// Resize memory area for this range. Returns true if it causes container
        /// buffer reallocation.
        virtual bool Resize(int numElements) {
            _numElements = numElements;
            return _bufferArray->Resize(numElements);
        }

        /// Copy source data into buffer
        HDST_API
        virtual void CopyData(HdBufferSourceSharedPtr const &bufferSource);

        /// Read back the buffer content
        HDST_API
        virtual VtValue ReadData(TfToken const &name) const;

        /// Returns the relative offset in aggregated buffer
        virtual int GetOffset() const {
            return 0;
        }

        /// Returns the index in aggregated buffer
        virtual int GetIndex() const {
            return 0;
        }

        /// Returns the number of elements allocated
        virtual size_t GetNumElements() const {
            return _numElements;
        }

        /// Returns the capacity of allocated area for this range
        virtual int GetCapacity() const {
            return _bufferArray->GetCapacity();
        }

        /// Returns the version of the buffer array.
        virtual size_t GetVersion() const {
            return _bufferArray->GetVersion();
        }

        /// Increment the version of the buffer array.
        virtual void IncrementVersion() {
            _bufferArray->IncrementVersion();
        }

        /// Returns the max number of elements
        HDST_API
        virtual size_t GetMaxNumElements() const;

        /// Returns the usage hint from the underlying buffer array
        HDST_API
        virtual HdBufferArrayUsageHint GetUsageHint() const override;

        /// Returns the GPU resource. If the buffer array contains more than one
        /// resource, this method raises a coding error.
        HDST_API
        virtual HdStBufferResourceGLSharedPtr GetResource() const;

        /// Returns the named GPU resource.
        HDST_API
        virtual HdStBufferResourceGLSharedPtr GetResource(TfToken const& name);

        /// Returns the list of all named GPU resources for this bufferArrayRange.
        HDST_API
        virtual HdStBufferResourceGLNamedList const& GetResources() const;

        /// Sets the buffer array associated with this buffer;
        HDST_API
        virtual void SetBufferArray(HdBufferArray *bufferArray);

        /// Debug dump
        HDST_API
        virtual void DebugDump(std::ostream &out) const;

        /// Make this range invalid
        void Invalidate() {
            _bufferArray = NULL;
        }

    protected:
        /// Returns the aggregation container
        HDST_API
        virtual const void *_GetAggregation() const;

        /// Adds a new, named GPU resource and returns it.
        HDST_API
        HdStBufferResourceGLSharedPtr _AddResource(TfToken const& name,
                                                   HdTupleType tupleType,
                                                   int offset,
                                                   int stride);

    private:
        _SimpleBufferArray * _bufferArray;
        size_t _numElements;
    };

    typedef boost::shared_ptr<_SimpleBufferArray>
        _SimpleBufferArraySharedPtr;
    typedef boost::shared_ptr<_SimpleBufferArrayRange>
        _SimpleBufferArrayRangeSharedPtr;
    typedef boost::weak_ptr<_SimpleBufferArrayRange>
        _SimpleBufferArrayRangePtr;

    /// \class _SimpleBufferArray
    ///
    /// Simple buffer array (non-aggregated).
    ///
    class _SimpleBufferArray : public HdBufferArray
    {
    public:
        /// Constructor.
        HDST_API
        _SimpleBufferArray(TfToken const &role,
                           HdBufferSpecVector const &bufferSpecs,
                           HdBufferArrayUsageHint usageHint);

        /// Destructor. It invalidates _range
        HDST_API
        virtual ~_SimpleBufferArray();

        /// perform compaction if necessary, returns true if it becomes empty.
        HDST_API
        virtual bool GarbageCollect();

        /// Debug output
        HDST_API
        virtual void DebugDump(std::ostream &out) const;

        /// Set to resize buffers. Actual reallocation happens on Reallocate()
        HDST_API
        bool Resize(int numElements);

        /// Performs reallocation.
        /// GLX context has to be set when calling this function.
        HDST_API
        virtual void Reallocate(
                std::vector<HdBufferArrayRangeSharedPtr> const &ranges,
                HdBufferArraySharedPtr const &curRangeOwner);

        /// Returns the maximum number of elements capacity.
        HDST_API
        virtual size_t GetMaxNumElements() const;

        /// Returns current capacity. It could be different from numElements.
        int GetCapacity() const {
            return _capacity;
        }

        /// TODO: We need to distinguish between the primvar types here, we should
        /// tag each HdBufferSource and HdBufferResource with Constant, Uniform,
        /// Varying, Vertex, or FaceVarying and provide accessors for the specific
        /// buffer types.

        /// Returns the GPU resource. If the buffer array contains more than one
        /// resource, this method raises a coding error.
        HDST_API
        HdStBufferResourceGLSharedPtr GetResource() const;

        /// Returns the named GPU resource. This method returns the first found
        /// resource. In HD_SAFE_MODE it checks all underlying GL buffers
        /// in _resourceMap and raises a coding error if there are more than
        /// one GL buffers exist.
        HDST_API
        HdStBufferResourceGLSharedPtr GetResource(TfToken const& name);

        /// Returns the list of all named GPU resources for this bufferArray.
        HdStBufferResourceGLNamedList const& GetResources() const {return _resourceList;}

        /// Reconstructs the bufferspecs and returns it (for buffer splitting)
        HDST_API
        HdBufferSpecVector GetBufferSpecs() const;

    protected:
        HDST_API
        void _DeallocateResources();

        /// Adds a new, named GPU resource and returns it.
        HDST_API
        HdStBufferResourceGLSharedPtr _AddResource(TfToken const& name,
                                                   HdTupleType tupleType,
                                                   int offset,
                                                   int stride);
    private:
        int _capacity;
        size_t _maxBytesPerElement;

        HdStBufferResourceGLNamedList _resourceList;

        _SimpleBufferArrayRangeSharedPtr _GetRangeSharedPtr() const {
            return GetRangeCount() > 0
                ? boost::static_pointer_cast<_SimpleBufferArrayRange>(GetRange(0).lock())
                : _SimpleBufferArrayRangeSharedPtr();
        }
    };
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDST_VBO_SIMPLE_MEMORY_MANAGER_H
