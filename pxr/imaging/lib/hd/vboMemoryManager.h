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
#ifndef HD_VBO_MEMORY_MANAGER_H
#define HD_VBO_MEMORY_MANAGER_H

#include "pxr/pxr.h"
#include <boost/shared_ptr.hpp>
#include <list>

#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/token.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/bufferArray.h"
#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/strategyBase.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \class HdVBOMemoryManager
///
/// VBO memory manager.
///
class HdVBOMemoryManager : public HdAggregationStrategy {
public:
    /// Factory for creating HdBufferArray managed by
    /// HdVBOMemoryManager aggregation.
    virtual HdBufferArraySharedPtr CreateBufferArray(
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs);

    /// Factory for creating HdBufferArrayRange managed by
    /// HdVBOMemoryManager aggregation.
    virtual HdBufferArrayRangeSharedPtr CreateBufferArrayRange();

    /// Returns id for given bufferSpecs to be used for aggregation
    virtual AggregationId ComputeAggregationId(
        HdBufferSpecVector const &bufferSpecs) const;

    /// Returns an instance of memory manager
    static HdVBOMemoryManager& GetInstance() {
        return TfSingleton<HdVBOMemoryManager>::GetInstance();
    }

protected:
    friend class TfSingleton<HdVBOMemoryManager>;
    class _StripedBufferArray;

    /// specialized buffer array range
    class _StripedBufferArrayRange : public HdBufferArrayRange {
    public:
        /// Constructor.
        _StripedBufferArrayRange()
         : _stripedBufferArray(nullptr),
           _offset(0),
           _numElements(0),
           _capacity(0)
        {
        }

        /// Destructor.
        virtual ~_StripedBufferArrayRange();

        /// Returns true if this range is valid
        virtual bool IsValid() const {
            return (bool)_stripedBufferArray;
        }

        /// Returns true is the range has been assigned to a buffer
        virtual bool IsAssigned() const;


        /// Resize memory area for this range. Returns true if it causes container
        /// buffer reallocation.
        virtual bool Resize(int numElements);

        /// Copy source data into buffer
        virtual void CopyData(HdBufferSourceSharedPtr const &bufferSource);

        /// Read back the buffer content
        virtual VtValue ReadData(TfToken const &name) const;

        /// Returns the relative offset in aggregated buffer
        virtual int GetOffset() const {
            return _offset;
        }

        /// Returns the index for this range
        virtual int GetIndex() const {
            // note: range doesn't store index, so we need to sweep rangeLists
            // to find the index of this range.
            TF_CODING_ERROR("vboMemoryManager doesn't support GetIndex() for "
                            "memory and performance reasons\n");
            return 0;
        }

        /// Returns the number of elements
        virtual int GetNumElements() const {
            return _numElements;
        }

        /// Returns the version of the buffer array.
        virtual size_t GetVersion() const {
            return _stripedBufferArray->GetVersion();
        }

        /// Increment the version of the buffer array.
        virtual void IncrementVersion() {
            _stripedBufferArray->IncrementVersion();
        }

        /// Returns the max number of elements
        virtual size_t GetMaxNumElements() const;

        /// Returns the GPU resource. If the buffer array contains more than one
        /// resource, this method raises a coding error.
        virtual HdBufferResourceSharedPtr GetResource() const;

        /// Returns the named GPU resource.
        virtual HdBufferResourceSharedPtr GetResource(TfToken const& name);

        /// Returns the list of all named GPU resources for this bufferArrayRange.
        virtual HdBufferResourceNamedList const& GetResources() const;

        /// Sets the buffer array assosiated with this buffer;
        virtual void SetBufferArray(HdBufferArray *bufferArray);

        /// Debug dump
        virtual void DebugDump(std::ostream &out) const;

        /// Set the relative offset for this range.
        void SetOffset(int offset) {
            _offset = offset;
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
        virtual const void *_GetAggregation() const;

    private:
        // holding a weak reference to container.
        // this pointer becomes null when the StripedBufferArray gets destructed,
        // in case if any drawItem still holds this bufferRange.
        _StripedBufferArray *_stripedBufferArray;
        int _offset;
        int _numElements;
        int _capacity;
    };

    typedef boost::shared_ptr<_StripedBufferArrayRange>
        _StripedBufferArrayRangeSharedPtr;
    typedef boost::weak_ptr<_StripedBufferArrayRange>
        _StripedBufferArrayRangePtr;

    /// striped buffer array
    class _StripedBufferArray : public HdBufferArray {
    public:
        /// Constructor.
        _StripedBufferArray(TfToken const &role, HdBufferSpecVector const &bufferSpecs);

        /// Destructor. It invalidates _rangeList
        virtual ~_StripedBufferArray();

        /// perform compaction if necessary. If it becomes empty, release all
        /// resources and returns true
        virtual bool GarbageCollect();

        /// Debug output
        virtual void DebugDump(std::ostream &out) const;

        /// Performs reallocation.
        /// GLX context has to be set when calling this function.
        virtual void Reallocate(
            std::vector<HdBufferArrayRangeSharedPtr> const &ranges,
            HdBufferArraySharedPtr const &curRangeOwner);

        /// Returns the maximum number of elements capacity.
        virtual size_t GetMaxNumElements() const;

        /// Mark to perform reallocation on Reallocate()
        void SetNeedsReallocation() {
            _needsReallocation = true;
        }

        /// Mark to perform compaction on GarbageCollect()
        void SetNeedsCompaction() {
            _needsCompaction = true;
        }

    protected:
        void _DeallocateResources();

    private:

        bool _needsCompaction;
        int _totalCapacity;
        size_t _maxBytesPerElement;

        // Helpper routine to cast the range shared pointer.
        _StripedBufferArrayRangeSharedPtr _GetRangeSharedPtr(size_t idx) const {
            return boost::static_pointer_cast<_StripedBufferArrayRange>(GetRange(idx).lock());
        }
    };
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HD_VBO_MEMORY_MANAGER_H
