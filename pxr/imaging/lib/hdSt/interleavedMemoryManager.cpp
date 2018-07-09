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
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/contextCaps.h"

#include "pxr/imaging/hdSt/interleavedMemoryManager.h"
#include "pxr/imaging/hdSt/bufferResourceGL.h"
#include "pxr/imaging/hdSt/glUtils.h"

#include <boost/make_shared.hpp>
#include <vector>

#include "pxr/base/arch/hash.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/iterator.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdSt/glConversions.h"

#include "pxr/imaging/hf/perfLog.h"

PXR_NAMESPACE_OPEN_SCOPE

// ---------------------------------------------------------------------------
//  HdStInterleavedMemoryManager
// ---------------------------------------------------------------------------
HdBufferArrayRangeSharedPtr
HdStInterleavedMemoryManager::CreateBufferArrayRange()
{
    return (boost::make_shared<_StripedInterleavedBufferRange>());
}

/// Returns the buffer specs from a given buffer array
HdBufferSpecVector 
HdStInterleavedMemoryManager::GetBufferSpecs(
    HdBufferArraySharedPtr const &bufferArray) const
{
    _StripedInterleavedBufferSharedPtr bufferArray_ =
        boost::static_pointer_cast<_StripedInterleavedBuffer> (bufferArray);
    return bufferArray_->GetBufferSpecs();
}

/// Returns the size of the GPU memory used by the passed buffer array
size_t 
HdStInterleavedMemoryManager::GetResourceAllocation(
    HdBufferArraySharedPtr const &bufferArray, 
    VtDictionary &result) const 
{ 
    std::set<GLuint> idSet;
    size_t gpuMemoryUsed = 0;

    _StripedInterleavedBufferSharedPtr bufferArray_ =
        boost::static_pointer_cast<_StripedInterleavedBuffer> (bufferArray);

    TF_FOR_ALL(resIt, bufferArray_->GetResources()) {
        HdStBufferResourceGLSharedPtr const & resource = resIt->second;

        // XXX avoid double counting of resources shared within a buffer
        GLuint id = resource->GetId();
        if (idSet.count(id) == 0) {
            idSet.insert(id);

            std::string const & role = resource->GetRole().GetString();
            size_t size = size_t(resource->GetSize());

            if (result.count(role)) {
                size_t currentSize = result[role].Get<size_t>();
                result[role] = VtValue(currentSize + size);
            } else {
                result[role] = VtValue(size);
            }

            gpuMemoryUsed += size;
        }
    }

    return gpuMemoryUsed;
}

// ---------------------------------------------------------------------------
//  HdStInterleavedUBOMemoryManager
// ---------------------------------------------------------------------------
HdBufferArraySharedPtr
HdStInterleavedUBOMemoryManager::CreateBufferArray(
    TfToken const &role,
    HdBufferSpecVector const &bufferSpecs)
{
    GlfContextCaps &caps = GlfContextCaps::GetInstance();

    return boost::make_shared<
        HdStInterleavedMemoryManager::_StripedInterleavedBuffer>(
            role, bufferSpecs,
            caps.uniformBufferOffsetAlignment,
            /*structAlignment=*/sizeof(float)*4,
            caps.maxUniformBlockSize,
            HdPerfTokens->garbageCollectedUbo);
}

HdAggregationStrategy::AggregationId
HdStInterleavedUBOMemoryManager::ComputeAggregationId(
    HdBufferSpecVector const &bufferSpecs) const
{
    static size_t salt = ArchHash(__FUNCTION__, sizeof(__FUNCTION__));
    size_t result = salt;
    for (HdBufferSpec const &spec : bufferSpecs) {
        size_t const params[] = { 
            spec.name.Hash(),
            (size_t) spec.tupleType.type,
            spec.tupleType.count
        };
        boost::hash_combine(result,
                ArchHash((char const*)params, sizeof(size_t) * 3));
    }
    // promote to size_t
    return (AggregationId)result;
}

// ---------------------------------------------------------------------------
//  HdStInterleavedSSBOMemoryManager
// ---------------------------------------------------------------------------
HdBufferArraySharedPtr
HdStInterleavedSSBOMemoryManager::CreateBufferArray(
    TfToken const &role,
    HdBufferSpecVector const &bufferSpecs)
{
    GlfContextCaps &caps = GlfContextCaps::GetInstance();

    return boost::make_shared<
        HdStInterleavedMemoryManager::_StripedInterleavedBuffer>(
            role, bufferSpecs,
            /*bufferOffsetAlignment=*/0,
            /*structAlignment=*/0,
            caps.maxShaderStorageBlockSize,
            HdPerfTokens->garbageCollectedSsbo);
}

HdAggregationStrategy::AggregationId
HdStInterleavedSSBOMemoryManager::ComputeAggregationId(
    HdBufferSpecVector const &bufferSpecs) const
{
    static size_t salt = ArchHash(__FUNCTION__, sizeof(__FUNCTION__));
    size_t result = salt;
    for (HdBufferSpec const &spec : bufferSpecs) {
        size_t const params[] = { 
            spec.name.Hash(),
            (size_t) spec.tupleType.type,
            spec.tupleType.count
        };
        boost::hash_combine(result,
                ArchHash((char const*)params, sizeof(size_t) * 3));
    }
    return result;
}

// ---------------------------------------------------------------------------
//  _StripedInterleavedBuffer
// ---------------------------------------------------------------------------

static inline int
_ComputePadding(int alignment, int currentOffset)
{
    return ((alignment - (currentOffset & (alignment - 1))) & (alignment - 1));
}

static inline int
_ComputeAlignment(HdTupleType tupleType)
{
    const HdType componentType = HdGetComponentType(tupleType.type);
    const int numComponents = HdGetComponentCount(tupleType.type);
    const int componentSize = HdDataSizeOfType(componentType);

    // This is simplified to treat arrays of int and floats
    // as vectors. The padding rules state that if we have
    // an array of 2 ints, it would get aligned to the size
    // of a vec4, where as a vec2 of ints or floats is aligned
    // to the size of a vec2. Since we don't know if something is
    // an array or vector, we are treating them as vectors.
    //
    // XXX:Arrays: Now that we do know whether a value is an array
    // or vector, we can update this to do the right thing.

    // Matrices are treated as an array of vec4s, so the
    // max num components we are looking at is 4
    int alignComponents = std::min(numComponents, 4); 

    // single elements and vec2's are allowed, but
    // vec3's get rounded up to vec4's
    if(alignComponents == 3) {
        alignComponents = 4;
    }

    return componentSize * alignComponents;
}

HdStInterleavedMemoryManager::_StripedInterleavedBuffer::_StripedInterleavedBuffer(
    TfToken const &role,
    HdBufferSpecVector const &bufferSpecs,
    int bufferOffsetAlignment = 0,
    int structAlignment = 0,
    size_t maxSize = 0,
    TfToken const &garbageCollectionPerfToken = HdPerfTokens->garbageCollectedUbo)
    : HdBufferArray(role, garbageCollectionPerfToken),
      _needsCompaction(false),
      _stride(0),
      _bufferOffsetAlignment(bufferOffsetAlignment),
      _maxSize(maxSize)

{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    /*
       interleaved uniform buffer layout (for example)

                .--range["color"].offset
                v
      .--------------------------------------------------.
      | Xf      : Color      || Xf       : Color   || ...|
      '--------------------------------------------------'
       ^------- stride ------^
       ^---- one element ----^
    */

    /*
     do std140/std430 packing (GL spec section 7.6.2.2)
      When using the "std430" storage layout, shader storage
      blocks will be laid out in buffer storage identically to uniform and
      shader storage blocks using the "std140" layout, except that the base
      alignment of arrays of scalars and vectors in rule (4) and of structures
      in rule (9) are not rounded up a multiple of the base alignment of a vec4.
     */

    TF_FOR_ALL(it, bufferSpecs) {
        // Figure out the alignment we need for this type of data
        int alignment = _ComputeAlignment(it->tupleType);
        _stride += _ComputePadding(alignment, _stride);

        // We need to save the max alignment size for later because the
        // stride for our struct needs to be aligned to this
        structAlignment = std::max(structAlignment, alignment);

        _stride += HdDataSizeOfTupleType(it->tupleType);
    }

    // Our struct stride needs to be aligned to the max alignment needed within
    // our struct.
    _stride += _ComputePadding(structAlignment, _stride);

    // and also aligned if bufferOffsetAlignment exists (for UBO binding)
    if (_bufferOffsetAlignment > 0) {
        _stride += _ComputePadding(_bufferOffsetAlignment, _stride);
    }

    TF_VERIFY(_stride > 0);

    TF_DEBUG_MSG(HD_BUFFER_ARRAY_INFO,
                 "Create interleaved buffer array: stride = %d\n", _stride);

    // populate BufferResources, interleaved
    int offset = 0;
    TF_FOR_ALL(it, bufferSpecs) {
        // Figure out alignment for this data member
        int alignment = _ComputeAlignment(it->tupleType);
        // Add any needed padding to fixup alignment
        offset += _ComputePadding(alignment, offset);

        _AddResource(it->name, it->tupleType, offset, _stride);

        TF_DEBUG_MSG(HD_BUFFER_ARRAY_INFO,
                     "  %s : offset = %d, alignment = %d\n",
                     it->name.GetText(), offset, alignment);

        offset += HdDataSizeOfTupleType(it->tupleType);
    }

    _SetMaxNumRanges(_maxSize / _stride);

    TF_VERIFY(_stride + offset);
}

HdStBufferResourceGLSharedPtr
HdStInterleavedMemoryManager::_StripedInterleavedBuffer::_AddResource(
    TfToken const& name,
    HdTupleType tupleType,
    int offset,
    int stride)
{
    HD_TRACE_FUNCTION();

    if (TfDebug::IsEnabled(HD_SAFE_MODE)) {
        // duplication check
        HdStBufferResourceGLSharedPtr bufferRes = GetResource(name);
        if (!TF_VERIFY(!bufferRes)) {
            return bufferRes;
        }
    }

    HdStBufferResourceGLSharedPtr bufferRes = HdStBufferResourceGLSharedPtr(
        new HdStBufferResourceGL(GetRole(), tupleType, offset, stride));

    _resourceList.emplace_back(name, bufferRes);
    return bufferRes;
}


HdStInterleavedMemoryManager::_StripedInterleavedBuffer::~_StripedInterleavedBuffer()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // invalidate buffer array ranges in range list
    // (these ranges may still be held by drawItems)
    size_t rangeCount = GetRangeCount();
    for (size_t rangeIdx = 0; rangeIdx < rangeCount; ++rangeIdx) {
        _StripedInterleavedBufferRangeSharedPtr range = _GetRangeSharedPtr(rangeIdx);

        if (range)
        {
            range->Invalidate();
        }
    }
}

bool
HdStInterleavedMemoryManager::_StripedInterleavedBuffer::GarbageCollect()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (_needsCompaction) {
        RemoveUnusedRanges();

        std::vector<HdBufferArrayRangeSharedPtr> ranges;
        size_t rangeCount = GetRangeCount();
        ranges.reserve(rangeCount);
        for (size_t i = 0; i < rangeCount; ++i) {
            HdBufferArrayRangeSharedPtr range = GetRange(i).lock();
            if (range)
                ranges.push_back(range);
        }
        Reallocate(ranges, shared_from_this());
    }

    if (GetRangeCount() == 0) {
        _DeallocateResources();
        return true;
    }

    return false;
}

void
HdStInterleavedMemoryManager::_StripedInterleavedBuffer::Reallocate(
    std::vector<HdBufferArrayRangeSharedPtr> const &ranges,
    HdBufferArraySharedPtr const &curRangeOwner)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    GLF_GROUP_FUNCTION();

    // XXX: make sure glcontext

    HD_PERF_COUNTER_INCR(HdPerfTokens->vboRelocated);

    // Calculate element count
    size_t elementCount = 0;
    TF_FOR_ALL (it, ranges) {
        HdBufferArrayRangeSharedPtr const &range = *it;
        if (!range) {
            TF_CODING_ERROR("Expired range found in the reallocation list");
        }
        elementCount += (*it)->GetNumElements();
    }
    size_t totalSize = elementCount * _stride;

    // update range list (should be done before early exit)
    _SetRangeList(ranges);

    // If there is no data to reallocate, it is the caller's responsibility to
    // deallocate the underlying resource. 
    //
    // XXX: There is an issue here if the caller does not deallocate
    // after this return, we will hold onto unused GPU resources until the next
    // reallocation. Perhaps we should free the buffer here to avoid that
    // situation.
    if (totalSize == 0)
        return;

    // resize each BufferResource
    // all HdBufferSources are sharing same VBO

    // allocate new one
    // curId and oldId will be different when we are adopting ranges
    // from another buffer array.
    GLuint newId = 0;
    GLuint oldId = GetResources().begin()->second->GetId();

    _StripedInterleavedBufferSharedPtr curRangeOwner_ =
        boost::static_pointer_cast<_StripedInterleavedBuffer> (curRangeOwner);

    GLuint curId = curRangeOwner_->GetResources().begin()->second->GetId();

    if (glGenBuffers) {
        glGenBuffers(1, &newId);

        GlfContextCaps const &caps = GlfContextCaps::GetInstance();
        if (caps.directStateAccessEnabled) {
            glNamedBufferDataEXT(newId, totalSize, /*data=*/NULL, GL_STATIC_DRAW);
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, newId);
            glBufferData(GL_ARRAY_BUFFER, totalSize, /*data=*/NULL, GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        // if old buffer exists, copy unchanged data
        if (curId) {
            int index = 0;

            size_t rangeCount = GetRangeCount();

            // pre-pass to combine consecutive buffer range relocation
            HdStGLBufferRelocator relocator(curId, newId);
            for (size_t rangeIdx = 0; rangeIdx < rangeCount; ++rangeIdx) {
                _StripedInterleavedBufferRangeSharedPtr range = _GetRangeSharedPtr(rangeIdx);

                if (!range) {
                    TF_CODING_ERROR("_StripedInterleavedBufferRange expired "
                                    "unexpectedly.");
                    continue;
                }
                int oldIndex = range->GetIndex();
                if (oldIndex >= 0) {
                    // copy old data
                    GLintptr readOffset = oldIndex * _stride;
                    GLintptr writeOffset = index * _stride;
                    GLsizeiptr copySize = _stride * range->GetNumElements();

                    relocator.AddRange(readOffset, writeOffset, copySize);
                }

                range->SetIndex(index);
                index += range->GetNumElements();
            }

            // buffer copy
            relocator.Commit();

        } else {
            // just set index
            int index = 0;

            size_t rangeCount = GetRangeCount();
            for (size_t rangeIdx = 0; rangeIdx < rangeCount; ++rangeIdx) {
                _StripedInterleavedBufferRangeSharedPtr range = _GetRangeSharedPtr(rangeIdx);
                if (!range) {
                    TF_CODING_ERROR("_StripedInterleavedBufferRange expired "
                                    "unexpectedly.");
                    continue;
                }

                range->SetIndex(index);
                index += range->GetNumElements();
            }
        }
        if (oldId) {
            // delete old buffer
            glDeleteBuffers(1, &oldId);
        }
    } else {
        // for unit test
        static int id = 1;
        newId = id++;
    }

    // update id to all buffer resources
    TF_FOR_ALL(it, GetResources()) {
        it->second->SetAllocation(newId, totalSize);
    }

    _needsReallocation = false;
    _needsCompaction = false;

    // increment version to rebuild dispatch buffers.
    IncrementVersion();
}

void
HdStInterleavedMemoryManager::_StripedInterleavedBuffer::_DeallocateResources()
{
    HdStBufferResourceGLSharedPtr resource = GetResource();
    if (resource) {
        GLuint id = resource->GetId();
        if (id) {
            if (glDeleteBuffers) {
                glDeleteBuffers(1, &id);
            }
            resource->SetAllocation(0, 0);
        }
    }
}

void
HdStInterleavedMemoryManager::_StripedInterleavedBuffer::DebugDump(std::ostream &out) const
{
    out << "  HdStInterleavedMemoryManager\n";
    out << "    Range entries " << GetRangeCount() << ":\n";

    size_t rangeCount = GetRangeCount();
    for (size_t rangeIdx = 0; rangeIdx < rangeCount; ++rangeIdx) {
        _StripedInterleavedBufferRangeSharedPtr range = _GetRangeSharedPtr(rangeIdx);

        if (range) {
            out << "      " << rangeIdx << *range;
        }
    }
}

HdStBufferResourceGLSharedPtr
HdStInterleavedMemoryManager::_StripedInterleavedBuffer::GetResource() const
{
    HD_TRACE_FUNCTION();

    if (_resourceList.empty()) return HdStBufferResourceGLSharedPtr();

    if (TfDebug::IsEnabled(HD_SAFE_MODE)) {
        // make sure this buffer array has only one resource.
        GLuint id = _resourceList.begin()->second->GetId();
        TF_FOR_ALL (it, _resourceList) {
            if (it->second->GetId() != id) {
                TF_CODING_ERROR("GetResource(void) called on"
                                "HdBufferArray having multiple GL resources");
            }
        }
    }

    // returns the first item
    return _resourceList.begin()->second;
}

HdStBufferResourceGLSharedPtr
HdStInterleavedMemoryManager::_StripedInterleavedBuffer::GetResource(TfToken const& name)
{
    HD_TRACE_FUNCTION();

    // linear search.
    // The number of buffer resources should be small (<10 or so).
    for (HdStBufferResourceGLNamedList::iterator it = _resourceList.begin();
         it != _resourceList.end(); ++it) {
        if (it->first == name) return it->second;
    }
    return HdStBufferResourceGLSharedPtr();
}

HdBufferSpecVector
HdStInterleavedMemoryManager::_StripedInterleavedBuffer::GetBufferSpecs() const
{
    HdBufferSpecVector result;
    result.reserve(_resourceList.size());
    TF_FOR_ALL (it, _resourceList) {
        result.emplace_back(it->first, it->second->GetTupleType());
    }
    return result;
}

// ---------------------------------------------------------------------------
//  _StripedInterleavedBufferRange
// ---------------------------------------------------------------------------
HdStInterleavedMemoryManager::_StripedInterleavedBufferRange::~_StripedInterleavedBufferRange()
{
    // Notify that hosting buffer array needs to be garbage collected.
    //
    // Don't do any substantial work here.
    //
    if (_stripedBuffer) {
        _stripedBuffer->SetNeedsCompaction();
    }
}


bool
HdStInterleavedMemoryManager::_StripedInterleavedBufferRange::IsAssigned() const
{
    return (_stripedBuffer != nullptr);
}

bool
HdStInterleavedMemoryManager::_StripedInterleavedBufferRange::IsImmutable() const
{
    return (_stripedBuffer != nullptr)
         && _stripedBuffer->IsImmutable();
}


bool
HdStInterleavedMemoryManager::_StripedInterleavedBufferRange::Resize(int numElements)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(_stripedBuffer)) return false;

    // interleaved BAR never needs to be resized, since numElements in buffer
    // resources is always 1. Note that the arg numElements of this function
    // could be more than 1 for static array.
    // ignore Resize request.

    // XXX: this could be a problem if a client allows to change the array size
    //      dynamically -- e.g. instancer nesting level changes.
    //
    return false;
}

void
HdStInterleavedMemoryManager::_StripedInterleavedBufferRange::CopyData(
    HdBufferSourceSharedPtr const &bufferSource)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(_stripedBuffer)) return;

    HdStBufferResourceGLSharedPtr VBO =
        _stripedBuffer->GetResource(bufferSource->GetName());

    if (!VBO || VBO->GetId() == 0) {
        TF_CODING_ERROR("VBO doesn't exist for %s",
                        bufferSource->GetName().GetText());
        return;
    }
    GLF_GROUP_FUNCTION();

    // overrun check
    // XXX:Arrays:  Note that we only check tuple type here, not arity.
    // This code allows N-tuples and N-element arrays to be interchanged.
    // It would seem better to have upstream buffers adjust their tuple
    // arity as needed.
    if (!TF_VERIFY(
        bufferSource->GetTupleType().type == VBO->GetTupleType().type,
        "'%s': (%s (%i) x %zu) != (%s (%i) x %zu)\n",
        bufferSource->GetName().GetText(),
        TfEnum::GetName(bufferSource->GetTupleType().type).c_str(),
        bufferSource->GetTupleType().type,
        bufferSource->GetTupleType().count,
        TfEnum::GetName(VBO->GetTupleType().type).c_str(),
        VBO->GetTupleType().type,
        VBO->GetTupleType().count)) {
        return;
    }

    GlfContextCaps const &caps = GlfContextCaps::GetInstance();
    if (glBufferSubData != NULL) {
        int vboStride = VBO->GetStride();
        GLintptr vboOffset = VBO->GetOffset() + vboStride * _index;
        int dataSize = HdDataSizeOfTupleType(VBO->GetTupleType());
        const unsigned char *data =
            (const unsigned char*)bufferSource->GetData();

        for (size_t i = 0; i < _numElements; ++i) {
            HD_PERF_COUNTER_INCR(HdPerfTokens->glBufferSubData);

            // XXX: MapBuffer?
            // XXX
            // using glNamedBuffer against UBO randomly triggers a crash at
            // glXSwapBuffers on driver 319.32. It doesn't occur on 331.49.
            // XXX: move this workaround into renderContextCaps.
            if (false && caps.directStateAccessEnabled) {
                glNamedBufferSubDataEXT(VBO->GetId(), vboOffset, dataSize, data);
            } else {
                glBindBuffer(GL_ARRAY_BUFFER, VBO->GetId());
                glBufferSubData(GL_ARRAY_BUFFER, vboOffset, dataSize, data);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
            vboOffset += vboStride;
            data += dataSize;
        }
    }
}

VtValue
HdStInterleavedMemoryManager::_StripedInterleavedBufferRange::ReadData(
    TfToken const &name) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    VtValue result;
    if (!TF_VERIFY(_stripedBuffer)) return result;

    HdStBufferResourceGLSharedPtr VBO = _stripedBuffer->GetResource(name);

    if (!VBO || VBO->GetId() == 0) {
        TF_CODING_ERROR("VBO doesn't exist for %s", name.GetText());
        return result;
    }

    result = HdStGLUtils::ReadBuffer(VBO->GetId(),
                                   VBO->GetTupleType(),
                                   VBO->GetOffset() + VBO->GetStride() * _index,
                                   VBO->GetStride(),
                                   _numElements);

    return result;
}

size_t
HdStInterleavedMemoryManager::_StripedInterleavedBufferRange::GetMaxNumElements() const
{
    return _stripedBuffer->GetMaxNumElements();
}

HdStBufferResourceGLSharedPtr
HdStInterleavedMemoryManager::_StripedInterleavedBufferRange::GetResource() const
{
    if (!TF_VERIFY(_stripedBuffer)) return HdStBufferResourceGLSharedPtr();

    return _stripedBuffer->GetResource();
}

HdStBufferResourceGLSharedPtr
HdStInterleavedMemoryManager::_StripedInterleavedBufferRange::GetResource(
    TfToken const& name)
{
    if (!TF_VERIFY(_stripedBuffer))
        return HdStBufferResourceGLSharedPtr();

    // don't use GetResource(void) as a shortcut even an interleaved buffer
    // is sharing one underlying GL resource. We may need an appropriate
    // offset depending on name.
    return _stripedBuffer->GetResource(name);
}

HdStBufferResourceGLNamedList const&
HdStInterleavedMemoryManager::_StripedInterleavedBufferRange::GetResources() const
{
    if (!TF_VERIFY(_stripedBuffer)) {
        static HdStBufferResourceGLNamedList empty;
        return empty;
    }
    return _stripedBuffer->GetResources();
}

void
HdStInterleavedMemoryManager::_StripedInterleavedBufferRange::SetBufferArray(HdBufferArray *bufferArray)
{
    _stripedBuffer = static_cast<_StripedInterleavedBuffer *>(bufferArray);
}

void
HdStInterleavedMemoryManager::_StripedInterleavedBufferRange::DebugDump(
    std::ostream &out) const
{
    out << "[StripedIBR] index = " << _index
        << "\n";
}

const void *
HdStInterleavedMemoryManager::_StripedInterleavedBufferRange::_GetAggregation() const
{
    return _stripedBuffer;
}

PXR_NAMESPACE_CLOSE_SCOPE

