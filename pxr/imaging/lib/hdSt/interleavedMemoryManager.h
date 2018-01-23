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
#ifndef HDST_INTERLEAVED_VBO_MEMORY_MANAGER_H
#define HDST_INTERLEAVED_VBO_MEMORY_MANAGER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/bufferArrayRangeGL.h"
#include "pxr/imaging/hd/bufferArray.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/resource.h"
#include "pxr/imaging/hd/strategyBase.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/token.h"

#include <list>

PXR_NAMESPACE_OPEN_SCOPE


/// \class HdStInterleavedMemoryManager
///
/// Interleaved memory manager (base class).
///
class HdStInterleavedMemoryManager : public HdAggregationStrategy {
protected:
    class _StripedInterleavedBuffer;

    /// specialized buffer array range
    class _StripedInterleavedBufferRange : public HdStBufferArrayRangeGL {
    public:
        /// Constructor.
        _StripedInterleavedBufferRange() :
        _stripedBuffer(nullptr), _index(NOT_ALLOCATED), _numElements(1) {
        }

        /// Destructor.
        HDST_API
        virtual ~_StripedInterleavedBufferRange();

        /// Returns true if this range is valid
        virtual bool IsValid() const {
            // note: a range is valid even its index is NOT_ALLOCATED.
            return (bool)_stripedBuffer;
        }

        /// Returns true is the range has been assigned to a buffer
        HDST_API
        virtual bool IsAssigned() const;

        /// Returns true if this range is marked as immutable.
        virtual bool IsImmutable() const;

        /// Resize memory area for this range. Returns true if it causes container
        /// buffer reallocation.
        HDST_API
        virtual bool Resize(int numElements);

        /// Copy source data into buffer
        HDST_API
        virtual void CopyData(HdBufferSourceSharedPtr const &bufferSource);

        /// Read back the buffer content
        HDST_API
        virtual VtValue ReadData(TfToken const &name) const;

        /// Returns the relative offset in aggregated buffer
        virtual int GetOffset() const {
            if (!TF_VERIFY(_stripedBuffer) ||
                !TF_VERIFY(_index != NOT_ALLOCATED)) return 0;
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

        /// Returns the max number of elements
        HDST_API
        virtual size_t GetMaxNumElements() const;

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

        /// Sets the buffer array assosiated with this buffer;
        HDST_API
        virtual void SetBufferArray(HdBufferArray *bufferArray);

        /// Debug dump
        HDST_API
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
        HDST_API
        virtual const void *_GetAggregation() const;

    private:
        enum { NOT_ALLOCATED = -1 };
        _StripedInterleavedBuffer *_stripedBuffer;
        int _index;
        int _numElements;
    };

    typedef std::shared_ptr<_StripedInterleavedBuffer>
        _StripedInterleavedBufferSharedPtr;
    typedef std::shared_ptr<_StripedInterleavedBufferRange>
        _StripedInterleavedBufferRangeSharedPtr;
    typedef std::weak_ptr<_StripedInterleavedBufferRange>
        _StripedInterleavedBufferRangePtr;

    /// striped buffer
    class _StripedInterleavedBuffer : public HdBufferArray {
    public:
        /// Constructor.
        HDST_API
        _StripedInterleavedBuffer(TfToken const &role,
                                  HdBufferSpecVector const &bufferSpecs,
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
        int GetStride() const {
            return _stride;
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
        /// resource. In HD_SAFE_MODE it checkes all underlying GL buffers
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
        bool _needsCompaction;
        int _stride;
        int _bufferOffsetAlignment;  // ranged binding offset alignment
        size_t _maxSize;             // maximum size of single buffer

        HdStBufferResourceGLNamedList _resourceList;

        _StripedInterleavedBufferRangeSharedPtr _GetRangeSharedPtr(size_t idx) const {
            return std::static_pointer_cast<_StripedInterleavedBufferRange>(GetRange(idx).lock());
        }

    };

    /// Factory for creating HdBufferArrayRange
    virtual HdBufferArrayRangeSharedPtr CreateBufferArrayRange();

    /// Returns the buffer specs from a given buffer array
    virtual HdBufferSpecVector GetBufferSpecs(
        HdBufferArraySharedPtr const &bufferArray) const;

    /// Returns the size of the GPU memory used by the passed buffer array
    virtual size_t GetResourceAllocation(
        HdBufferArraySharedPtr const &bufferArray, 
        VtDictionary &result) const;
};

class HdStInterleavedUBOMemoryManager : public HdStInterleavedMemoryManager {
public:
    /// Factory for creating HdBufferArray managed by
    /// HdStVBOMemoryManager aggregation.
    HDST_API
    virtual HdBufferArraySharedPtr CreateBufferArray(
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs);

    /// Returns id for given bufferSpecs to be used for aggregation
    HDST_API
    virtual AggregationId ComputeAggregationId(
        HdBufferSpecVector const &bufferSpecs) const;
};

class HdStInterleavedSSBOMemoryManager : public HdStInterleavedMemoryManager {
public:
    /// Factory for creating HdBufferArray managed by
    /// HdStVBOMemoryManager aggregation.
    HDST_API
    virtual HdBufferArraySharedPtr CreateBufferArray(
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs);

    /// Returns id for given bufferSpecs to be used for aggregation
    HDST_API
    virtual AggregationId ComputeAggregationId(
        HdBufferSpecVector const &bufferSpecs) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDST_INTERLEAVED_VBO_MEMORY_MANAGER_H
