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
#ifndef HD_VBO_SIMPLE_MEMORY_MANAGER_H
#define HD_VBO_SIMPLE_MEMORY_MANAGER_H

#include "pxr/base/tf/singleton.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/strategyBase.h"
#include "pxr/imaging/hd/bufferArray.h"
#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/bufferSource.h"

/// \class HdVBOSimpleMemoryManager
///
/// VBO simple memory manager.
///
/// This class doesn't perform any aggregation.
///
class HdVBOSimpleMemoryManager : public HdAggregationStrategy {
public:
    /// Factory for creating HdBufferArray managed by
    /// HdVBOSimpleMemoryManager.
    virtual HdBufferArraySharedPtr CreateBufferArray(
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs);

    /// Factory for creating HdBufferArrayRange
    virtual HdBufferArrayRangeSharedPtr CreateBufferArrayRange();

    /// Returns id for given bufferSpecs to be used for aggregation
    virtual HdAggregationStrategy::AggregationId ComputeAggregationId(
        HdBufferSpecVector const &bufferSpecs) const;

    /// Returns an instance of resource registry
    static HdVBOSimpleMemoryManager& GetInstance() {
        return TfSingleton<HdVBOSimpleMemoryManager>::GetInstance();
    }

protected:
    friend class TfSingleton<HdVBOSimpleMemoryManager>;
    class _SimpleBufferArray;

    /// \class _SimpleBufferArrayRange
    ///
    /// Specialized buffer array range for SimpleBufferArray.
    ///
    class _SimpleBufferArrayRange : public HdBufferArrayRange
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
        virtual bool IsAssigned() const;

        /// Resize memory area for this range. Returns true if it causes container
        /// buffer reallocation.
        virtual bool Resize(int numElements) {
            _numElements = numElements;
            return _bufferArray->Resize(numElements);
        }

        /// Copy source data into buffer
        virtual void CopyData(HdBufferSourceSharedPtr const &bufferSource);

        /// Read back the buffer content
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
        virtual int GetNumElements() const {
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

        /// Make this range invalid
        void Invalidate() {
            _bufferArray = NULL;
        }

    protected:
        /// Returns the aggregation container
        virtual const void *_GetAggregation() const;

    private:
        _SimpleBufferArray * _bufferArray;
        int _numElements;
    };

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
        _SimpleBufferArray(TfToken const &role, HdBufferSpecVector const &bufferSpecs);

        /// Destructor. It invalidates _range
        virtual ~_SimpleBufferArray();

        /// perform compaction if necessary, returns true if it becomes empty.
        virtual bool GarbageCollect();

        /// Debug output
        virtual void DebugDump(std::ostream &out) const;

        /// Set to resize buffers. Actual reallocation happens on Reallocate()
        bool Resize(int numElements);

        /// Performs reallocation.
        /// GLX context has to be set when calling this function.
        virtual void Reallocate(
                std::vector<HdBufferArrayRangeSharedPtr> const &ranges,
                HdBufferArraySharedPtr const &curRangeOwner);

        /// Returns the maximum number of elements capacity.
        virtual size_t GetMaxNumElements() const;

        /// Returns current capacity. It could be different from numElements.
        int GetCapacity() const {
            return _capacity;
        }

    protected:
        void _DeallocateResources();

    private:
        int _capacity;
        size_t _maxBytesPerElement;

        _SimpleBufferArrayRangeSharedPtr _GetRangeSharedPtr() const {
            return GetRangeCount() > 0
                ? boost::static_pointer_cast<_SimpleBufferArrayRange>(GetRange(0).lock())
                : _SimpleBufferArrayRangeSharedPtr();
        }
    };
};

#endif  // HD_VBO_SIMPLE_MEMORY_MANAGER_H
