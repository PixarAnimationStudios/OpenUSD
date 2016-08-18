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
#ifndef HD_INTERLEAVED_VBO_MEMORY_MANAGER_H
#define HD_INTERLEAVED_VBO_MEMORY_MANAGER_H

#include <boost/shared_ptr.hpp>
#include <list>

#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/token.h"
#include "pxr/imaging/hd/bufferArray.h"
#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/resource.h"
#include "pxr/imaging/hd/strategyBase.h"
#include "pxr/imaging/hd/tokens.h"

/// \class HdInterleavedMemoryManager
///
/// Interleaved memory manager (base class).
///
class HdInterleavedMemoryManager : public HdAggregationStrategy {
protected:
    friend class TfSingleton<HdInterleavedMemoryManager>;
    class _StripedInterleavedBuffer;

    /// specialized buffer array range
    class _StripedInterleavedBufferRange : public HdBufferArrayRange {
    public:
        /// Constructor.
        _StripedInterleavedBufferRange() :
        _stripedBuffer(nullptr), _index(NOT_ALLOCATED), _numElements(1) {
        }

        /// Destructor.
        virtual ~_StripedInterleavedBufferRange();

        /// Returns true if this range is valid
        virtual bool IsValid() const {
            // note: a range is valid even its index is NOT_ALLOCATED.
            return (bool)_stripedBuffer;
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
            if (not TF_VERIFY(_stripedBuffer) or
                not TF_VERIFY(_index != NOT_ALLOCATED)) return 0;
            return _stripedBuffer->GetStride() * _index;
        }

        /// Returns the index for this range
        virtual int GetIndex() const {
            return _index;
        }

        /// Returns the number of elements
        virtual int GetNumElements() const {
            return _numElements;
        }

        /// Returns the version of the buffer array.
        virtual size_t GetVersion() const {
            return _stripedBuffer->GetVersion();
        }

        /// Increment the version of the buffer array.
        virtual void IncrementVersion() {
            _stripedBuffer->IncrementVersion();
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

        /// Set the relative offset for this range.
        void SetIndex(int index) {
            _index = index;
        }

        /// Make this range invalid
        void Invalidate() {
            _stripedBuffer = nullptr;
        }

    protected:
        /// Returns the aggregation container
        virtual const void *_GetAggregation() const;

    private:
        enum { NOT_ALLOCATED = -1 };
        _StripedInterleavedBuffer *_stripedBuffer;
        int _index;
        int _numElements;
    };

    typedef boost::shared_ptr<_StripedInterleavedBufferRange>
        _StripedInterleavedBufferRangeSharedPtr;
    typedef boost::weak_ptr<_StripedInterleavedBufferRange>
        _StripedInterleavedBufferRangePtr;

    /// striped buffer
    class _StripedInterleavedBuffer : public HdBufferArray {
    public:
        /// Constructor.
        _StripedInterleavedBuffer(TfToken const &role,
                                  HdBufferSpecVector const &bufferSpecs,
                                  int bufferOffsetAlignment,
                                  int structAlignment,
                                  size_t maxSize,
                                  TfToken const &garbageCollectionPerfToken);

        /// Destructor. It invalidates _rangeList
        virtual ~_StripedInterleavedBuffer();

        /// perform compaction if necessary, returns true if it becomes empty.
        virtual bool GarbageCollect();

        /// Debug output
        virtual void DebugDump(std::ostream &out) const;

        /// Performs reallocation.
        /// GLX context has to be set when calling this function.
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
        int GetStride() const {
            return _stride;
        }

    protected:
        void _DeallocateResources();

    private:

        bool _needsCompaction;
        int _stride;
        int _bufferOffsetAlignment;  // ranged binding offset alignment
        size_t _maxSize;             // maximum size of single buffer

        _StripedInterleavedBufferRangeSharedPtr _GetRangeSharedPtr(size_t idx) const {
            return boost::static_pointer_cast<_StripedInterleavedBufferRange>(GetRange(idx).lock());
        }

    };

    /// Factory for creating HdBufferArrayRange
    virtual HdBufferArrayRangeSharedPtr CreateBufferArrayRange();
};

class HdInterleavedUBOMemoryManager : public HdInterleavedMemoryManager {
public:
    /// Factory for creating HdBufferArray managed by
    /// HdVBOMemoryManager aggregation.
    virtual HdBufferArraySharedPtr CreateBufferArray(
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs);

    /// Returns id for given bufferSpecs to be used for aggregation
    virtual AggregationId ComputeAggregationId(
        HdBufferSpecVector const &bufferSpecs) const;

    /// Returns an instance of memory manager
    static HdInterleavedUBOMemoryManager& GetInstance() {
        return TfSingleton<HdInterleavedUBOMemoryManager>::GetInstance();
    }
protected:
    friend class TfSingleton<HdInterleavedUBOMemoryManager>;
};

HDLIB_API_TEMPLATE_CLASS(TfSingleton<HdInterleavedUBOMemoryManager>);

class HdInterleavedSSBOMemoryManager : public HdInterleavedMemoryManager {
public:
    /// Factory for creating HdBufferArray managed by
    /// HdVBOMemoryManager aggregation.
    virtual HdBufferArraySharedPtr CreateBufferArray(
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs);

    /// Returns id for given bufferSpecs to be used for aggregation
    virtual AggregationId ComputeAggregationId(
        HdBufferSpecVector const &bufferSpecs) const;

    /// Returns an instance of memory manager
    static HdInterleavedSSBOMemoryManager& GetInstance() {
        return TfSingleton<HdInterleavedSSBOMemoryManager>::GetInstance();
    }
protected:
    friend class TfSingleton<HdInterleavedSSBOMemoryManager>;
};

HDLIB_API_TEMPLATE_CLASS(TfSingleton<HdInterleavedSSBOMemoryManager>);

#endif  // HD_INTERLEAVED_VBO_MEMORY_MANAGER_H
