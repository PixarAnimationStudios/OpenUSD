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

#include <boost/make_shared.hpp>
#include <vector>

#include "pxr/base/arch/hash.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/enum.h"

#include "pxr/imaging/hdSt/bufferResourceGL.h"
#include "pxr/imaging/hdSt/glUtils.h"
#include "pxr/imaging/hdSt/renderContextCaps.h"
#include "pxr/imaging/hdSt/vboMemoryManager.h"
#include "pxr/imaging/hdSt/glConversions.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hf/perfLog.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_ENV_SETTING(HD_MAX_VBO_SIZE, (1*1024*1024*1024),
                      "Maximum aggregated VBO size");

// ---------------------------------------------------------------------------
//  HdStVBOMemoryManager
// ---------------------------------------------------------------------------
HdBufferArraySharedPtr
HdStVBOMemoryManager::CreateBufferArray(
    TfToken const &role,
    HdBufferSpecVector const &bufferSpecs)
{
    return boost::make_shared<HdStVBOMemoryManager::_StripedBufferArray>(
        role, bufferSpecs, _isImmutable);
}


HdBufferArrayRangeSharedPtr
HdStVBOMemoryManager::CreateBufferArrayRange()
{
    return boost::make_shared<_StripedBufferArrayRange>();
}


HdAggregationStrategy::AggregationId
HdStVBOMemoryManager::ComputeAggregationId(HdBufferSpecVector const &bufferSpecs) const
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


/// Returns the buffer specs from a given buffer array
HdBufferSpecVector 
HdStVBOMemoryManager::GetBufferSpecs(
    HdBufferArraySharedPtr const &bufferArray) const
{
    _StripedBufferArraySharedPtr bufferArray_ =
        boost::static_pointer_cast<_StripedBufferArray> (bufferArray);
    return bufferArray_->GetBufferSpecs();
}


/// Returns the size of the GPU memory used by the passed buffer array
size_t 
HdStVBOMemoryManager::GetResourceAllocation(
    HdBufferArraySharedPtr const &bufferArray, 
    VtDictionary &result) const 
{ 
    std::set<GLuint> idSet;
    size_t gpuMemoryUsed = 0;

    _StripedBufferArraySharedPtr bufferArray_ =
        boost::static_pointer_cast<_StripedBufferArray> (bufferArray);

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
//  _StripedBufferArray
// ---------------------------------------------------------------------------
HdStVBOMemoryManager::_StripedBufferArray::_StripedBufferArray(
    TfToken const &role,
    HdBufferSpecVector const &bufferSpecs,
    bool isImmutable)
    : HdBufferArray(role, HdPerfTokens->garbageCollectedVbo, isImmutable),
      _needsCompaction(false),
      _totalCapacity(0),
      _maxBytesPerElement(0)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    /*
       non-interleaved non-uniform buffer array (for example)
          .------------------------------------------------------.
     vec3 | pos.x (prim0)         ||  pos.x (prim1)       || ... |
          |     y                 ||      y               ||     |
          |     z                 ||      z               ||     |
          '------------------------------------------------------'
          .------------------------------------------------------.
     vec4 | color.r (prim0)       ||  color.r (prim1)     || ... |
          |       g               ||        g             ||     |
          |       b               ||        b             ||     |
          |       a               ||        a             ||     |
          '------------------------------------------------------'
           ^--range0.numElements--^^--range1.numElements--^
                                   |
           ^-^                     ^--range1.offset
            stride
    */

    // populate BufferResources
    TF_FOR_ALL(it, bufferSpecs) {
        int stride = HdDataSizeOfTupleType(it->tupleType);
        _AddResource(it->name, it->tupleType, /*offset*/0, stride);
    }

    // VBO Memory Manage supports an effectivly limitless set of ranges
    _SetMaxNumRanges(std::numeric_limits<size_t>::max());

    // compute max bytes / elements
    TF_FOR_ALL (it, GetResources()) {
        _maxBytesPerElement = std::max(
            _maxBytesPerElement,
            HdDataSizeOfTupleType(it->second->GetTupleType()));
    }

    // GetMaxNumElements() will crash with a divide by 0
    // error if _maxBytesPerElement is 0.
    //
    // This can happen if bufferSpecs was empty and thus
    // no resources were added.   If means something went
    // wrong earlier and we are just trying to survive.
    if (!TF_VERIFY(_maxBytesPerElement != 0))
    {
        _maxBytesPerElement = 1;
    }
}

HdStBufferResourceGLSharedPtr
HdStVBOMemoryManager::_StripedBufferArray::_AddResource(TfToken const& name,
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

HdStVBOMemoryManager::_StripedBufferArray::~_StripedBufferArray()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // invalidate buffer array ranges in range list
    // (these ranges may still be held by drawItems)
    size_t rangeCount = GetRangeCount();
    for (size_t rangeIdx = 0; rangeIdx < rangeCount; ++rangeIdx) {
        _StripedBufferArrayRangeSharedPtr range = _GetRangeSharedPtr(rangeIdx);

        if (range) {
            range->Invalidate();
        }
    }
}


bool
HdStVBOMemoryManager::_StripedBufferArray::GarbageCollect()
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
HdStVBOMemoryManager::_StripedBufferArray::Reallocate(
    std::vector<HdBufferArrayRangeSharedPtr> const &ranges,
    HdBufferArraySharedPtr const &curRangeOwner)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // XXX: make sure glcontext
    HdStRenderContextCaps const &caps = HdStRenderContextCaps::GetInstance();

    HD_PERF_COUNTER_INCR(HdPerfTokens->vboRelocated);

    _StripedBufferArraySharedPtr curRangeOwner_ =
        boost::static_pointer_cast<_StripedBufferArray> (curRangeOwner);

    if (!TF_VERIFY(GetResources().size() ==
                      curRangeOwner_->GetResources().size())) {
        TF_CODING_ERROR("Resource mismatch when reallocating buffer array");
        return;
    }

    if (TfDebug::IsEnabled(HD_SAFE_MODE)) {
        HdStBufferResourceGLNamedList::size_type bresIdx = 0;
        TF_FOR_ALL(bresIt, GetResources()) {
            TF_VERIFY(curRangeOwner_->GetResources()[bresIdx++].second ==
                      curRangeOwner_->GetResource(bresIt->first));
        }
    }

    // count up total elements and update new offsets
    size_t totalNumElements = 0;
    std::vector<size_t> newOffsets;
    newOffsets.reserve(ranges.size());

    TF_FOR_ALL (it, ranges) {
        HdBufferArrayRangeSharedPtr const &range = *it;
        if (!range) {
            TF_CODING_ERROR("Expired range found in the reallocation list");
            continue;
        }

        // save new offset
        newOffsets.push_back(totalNumElements);

        // XXX: always tightly pack for now.
        totalNumElements += range->GetNumElements();
    }

    // update range list (should be done before early exit)
    _SetRangeList(ranges);

    // If there is no data to reallocate, it is the caller's responsibility to
    // deallocate the underlying resource. 
    //
    // XXX: There is an issue here if the caller does not deallocate
    // after this return, we will hold onto unused GPU resources until the next
    // reallocation. Perhaps we should free the buffer here to avoid that
    // situation.
    if (totalNumElements == 0)
        return;

    _totalCapacity = totalNumElements;

    // resize each BufferResource
    HdStBufferResourceGLNamedList const& resources = GetResources();
    for (size_t bresIdx=0; bresIdx<resources.size(); ++bresIdx) {
        HdStBufferResourceGLSharedPtr const &bres = resources[bresIdx].second;
        HdStBufferResourceGLSharedPtr const &curRes =
                curRangeOwner_->GetResources()[bresIdx].second;

        int bytesPerElement = HdDataSizeOfTupleType(bres->GetTupleType());
        TF_VERIFY(bytesPerElement > 0);
        GLsizeiptr bufferSize = bytesPerElement * _totalCapacity;

        // allocate new one
        // curId and oldId will be different when we are adopting ranges
        // from another buffer array.
        GLuint newId = 0;
        GLuint oldId = bres->GetId();
        GLuint curId = curRes->GetId();

        if (glGenBuffers) {
            glGenBuffers(1, &newId);

            if (ARCH_LIKELY(caps.directStateAccessEnabled)) {
                glNamedBufferDataEXT(newId,
                                     bufferSize, /*data=*/NULL, GL_STATIC_DRAW);
            } else {
                glBindBuffer(GL_ARRAY_BUFFER, newId);
                glBufferData(GL_ARRAY_BUFFER,
                             bufferSize, /*data=*/NULL, GL_STATIC_DRAW);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }

            // if old buffer exists, copy unchanged data
            if (curId) {
                std::vector<size_t>::iterator newOffsetIt = newOffsets.begin();

                // pre-pass to combine consecutive buffer range relocation
                HdStGLBufferRelocator relocator(curId, newId);
                TF_FOR_ALL (it, ranges) {
                    _StripedBufferArrayRangeSharedPtr range =
                        boost::static_pointer_cast<_StripedBufferArrayRange>(*it);
                    if (!range) {
                        TF_CODING_ERROR("_StripedBufferArrayRange "
                                        "expired unexpectedly.");
                        continue;
                    }

                    // copy the range. There are three cases:
                    //
                    // 1. src length (capacity) == dst length (numElements)
                    //   Copy the entire range
                    //
                    // 2. src length < dst length
                    //   Enlarging the range. This typically happens when
                    //   applying quadrangulation/subdivision to populate
                    //   additional data at the end of source data.
                    //
                    // 3. src length > dst length
                    //   Shrinking the range. When the garbage collection
                    //   truncates ranges.
                    //
                    int oldSize = range->GetCapacity();
                    int newSize = range->GetNumElements();
                    GLsizeiptr copySize =
                        std::min(oldSize, newSize) * bytesPerElement;
                    int oldOffset = range->GetOffset();
                    if (copySize > 0) {
                        GLintptr readOffset = oldOffset * bytesPerElement;
                        GLintptr writeOffset = *newOffsetIt * bytesPerElement;

                        relocator.AddRange(readOffset, writeOffset, copySize);
                    }
                    ++newOffsetIt;
                }

                // buffer copy
                relocator.Commit();
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

        // update id of buffer resource
        bres->SetAllocation(newId, bufferSize);
    }

    // update ranges
    for (size_t idx = 0; idx < ranges.size(); ++idx) {
        _StripedBufferArrayRangeSharedPtr range =
            boost::static_pointer_cast<_StripedBufferArrayRange>(ranges[idx]);
        if (!range) {
            TF_CODING_ERROR("_StripedBufferArrayRange expired unexpectedly.");
            continue;
        }
        range->SetOffset(newOffsets[idx]);
        range->SetCapacity(range->GetNumElements());
    }
    _needsReallocation = false;
    _needsCompaction = false;

    // increment version to rebuild dispatch buffers.
    IncrementVersion();
}

void
HdStVBOMemoryManager::_StripedBufferArray::_DeallocateResources()
{
    TF_FOR_ALL (it, GetResources()) {
        GLuint id = it->second->GetId();
        if (id) {
            if (glDeleteBuffers) {
                glDeleteBuffers(1, &id);
            }
            it->second->SetAllocation(0, 0);
        }
    }
}

/*virtual*/
size_t
HdStVBOMemoryManager::_StripedBufferArray::GetMaxNumElements() const
{
    static size_t vboMaxSize = TfGetEnvSetting(HD_MAX_VBO_SIZE);
    return vboMaxSize / _maxBytesPerElement;
}

void
HdStVBOMemoryManager::_StripedBufferArray::DebugDump(std::ostream &out) const
{
    out << "  HdStVBOMemoryManager\n";
    out << "  total capacity = " << _totalCapacity << "\n";
    out << "    Range entries " << GetRangeCount() << ":\n";

    size_t rangeCount = GetRangeCount();
    for (size_t rangeIdx = 0; rangeIdx < rangeCount; ++rangeIdx) {
        _StripedBufferArrayRangeSharedPtr range = _GetRangeSharedPtr(rangeIdx);
        if (range) {
            out << "      " << rangeIdx << *range;
        }
    }
}

HdStBufferResourceGLSharedPtr
HdStVBOMemoryManager::_StripedBufferArray::GetResource() const
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
HdStVBOMemoryManager::_StripedBufferArray::GetResource(TfToken const& name)
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
HdStVBOMemoryManager::_StripedBufferArray::GetBufferSpecs() const
{
    HdBufferSpecVector result;
    result.reserve(_resourceList.size());
    TF_FOR_ALL (it, _resourceList) {
        result.emplace_back(it->first, it->second->GetTupleType());
    }
    return result;
}

// ---------------------------------------------------------------------------
//  _StripedBufferArrayRange
// ---------------------------------------------------------------------------
HdStVBOMemoryManager::_StripedBufferArrayRange::~_StripedBufferArrayRange()
{
    // Notify that hosting buffer array needs to be garbage collected.
    //
    // Don't do any substantial work here.
    //
    if (_stripedBufferArray) {
        _stripedBufferArray->SetNeedsCompaction();

        // notify source bufferArray to bump the version so that
        // drawbatches to be rebuilt.
        // Also note that the buffer migration takes place only in
        // this StripedBufferArray, not in other InterleavedVBO/SimpleVBO.
        _stripedBufferArray->IncrementVersion();
    }
}


bool
HdStVBOMemoryManager::_StripedBufferArrayRange::IsAssigned() const
{
    return (_stripedBufferArray != nullptr);
}

bool
HdStVBOMemoryManager::_StripedBufferArrayRange::IsImmutable() const
{
    return (_stripedBufferArray != nullptr)
         && _stripedBufferArray->IsImmutable();
}

bool
HdStVBOMemoryManager::_StripedBufferArrayRange::Resize(int numElements)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(_stripedBufferArray)) return false;

    bool needsReallocation = false;

    // XXX: varying topology points fix (bug 114080)
    //
    // MDI draw uses a dispatch buffer, and it includes numElements to be
    // drawn. When a topology is varying, numElements will change so the
    // dispatch buffer has to be rebuilt. Currently we depend on entire
    // buffer reallocation for index-drawing prims (e.g. meshes and curves)
    // with varying topology. We always allocate new BARs for them,
    // which is inefficient, and will be addressed later (bug 103767)
    //
    // However varying points have another problem: When it reduces its
    // number of points, it doesn't cause the reallocation in the below code
    // (disabled by #if 0) since points don't have an index buffer.
    //
    // These two problems have to be solved together by introducing more
    // robust mechanism which updates dispatch buffer partially to
    // reflect numElements correctly without having reallocation.
    // It needs more works, until then, we invoke reallocation whenever
    // numElements changes in an aggregated buffer, for the correctness
    // problem of points drawing (this is bug 114080).
    //
    // The varying mesh batch may suffer a performance regression
    // from this treatment, but it should be relatively small. Because the
    // topology buffer has already been reallocated on every changes as
    // described above and the primvar buffer is also reallocated in
    // GarbageCollect() before drawing (see HdEngine::Draw()).
    //
    // We need to revisit to clean this up soon.
    //
#if 0
    if (_capacity < numElements) {
        // mark entire buffer array to be relocated
        _stripedBufferArray->SetNeedsReallocation();
        needsReallocation = true;
    } else if (_capacity > numElements) {
        // mark the buffer array can be compacted
        _stripedBufferArray->SetNeedsCompaction();
    }
#else
    if (_capacity != numElements) {
        const size_t numMaxElements = GetMaxNumElements();

        if (static_cast<size_t>(numElements) > numMaxElements) {
            TF_WARN("Attempting to resize the BAR with 0x%x elements when the "
                    "max number of elements in the buffer array is 0x%lx. "
                    "Clamping BAR size to the latter.",
                     numElements, numMaxElements);

            numElements = numMaxElements;
        }
        _stripedBufferArray->SetNeedsReallocation();
        needsReallocation = true;
    }
#endif

    _numElements = numElements;
    return needsReallocation;
}
void
HdStVBOMemoryManager::_StripedBufferArrayRange::CopyData(
    HdBufferSourceSharedPtr const &bufferSource)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(_stripedBufferArray)) return;

    HdStBufferResourceGLSharedPtr VBO =
        _stripedBufferArray->GetResource(bufferSource->GetName());

    if (!TF_VERIFY((VBO && VBO->GetId()),
                      "VBO doesn't exist for %s",
                      bufferSource->GetName().GetText())) {
        return;
    }

    // datatype of bufferSource has to match with bufferResource
    if (!TF_VERIFY(bufferSource->GetTupleType() == VBO->GetTupleType(),
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

    HdStRenderContextCaps const &caps = HdStRenderContextCaps::GetInstance();
    if (glBufferSubData) {
        int bytesPerElement = HdDataSizeOfTupleType(VBO->GetTupleType());

        // overrun check. for graceful handling of erroneous assets,
        // issue warning here and continue to copy for the valid range.
        size_t dstSize = _numElements * bytesPerElement;
        size_t srcSize =
            bufferSource->GetNumElements() *
            HdDataSizeOfTupleType(bufferSource->GetTupleType());
        if (srcSize > dstSize) {
            TF_WARN("%s: size %ld is larger than the range (%ld)",
                    bufferSource->GetName().GetText(), srcSize, dstSize);
            srcSize = dstSize;
        }
        GLintptr vboOffset = bytesPerElement * _offset;

        HD_PERF_COUNTER_INCR(HdPerfTokens->glBufferSubData);

        if (ARCH_LIKELY(caps.directStateAccessEnabled)) {
            glNamedBufferSubDataEXT(VBO->GetId(),
                                    vboOffset,
                                    srcSize,
                                    bufferSource->GetData());
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, VBO->GetId());
            glBufferSubData(GL_ARRAY_BUFFER,
                            vboOffset,
                            srcSize,
                            bufferSource->GetData());
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }
}

VtValue
HdStVBOMemoryManager::_StripedBufferArrayRange::ReadData(TfToken const &name) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    VtValue result;
    if (!TF_VERIFY(_stripedBufferArray)) return result;

    HdStBufferResourceGLSharedPtr VBO = _stripedBufferArray->GetResource(name);

    if (!VBO || (VBO->GetId() == 0 && _numElements > 0)) {
        TF_CODING_ERROR("VBO doesn't exist for %s", name.GetText());
        return result;
    }

    GLintptr vboOffset = HdDataSizeOfTupleType(VBO->GetTupleType()) * _offset;

    result = HdStGLUtils::ReadBuffer(VBO->GetId(),
                                   VBO->GetTupleType(),
                                   vboOffset,
                                   /*stride=*/0, // not interleaved.
                                   _numElements);

    return result;
}

size_t
HdStVBOMemoryManager::_StripedBufferArrayRange::GetMaxNumElements() const
{
    return _stripedBufferArray->GetMaxNumElements();
}

HdStBufferResourceGLSharedPtr
HdStVBOMemoryManager::_StripedBufferArrayRange::GetResource() const
{
    if (!TF_VERIFY(_stripedBufferArray)) return HdStBufferResourceGLSharedPtr();

    return _stripedBufferArray->GetResource();
}

HdStBufferResourceGLSharedPtr
HdStVBOMemoryManager::_StripedBufferArrayRange::GetResource(TfToken const& name)
{
    if (!TF_VERIFY(_stripedBufferArray)) return HdStBufferResourceGLSharedPtr();

    return _stripedBufferArray->GetResource(name);
}

HdStBufferResourceGLNamedList const&
HdStVBOMemoryManager::_StripedBufferArrayRange::GetResources() const
{
    if (!TF_VERIFY(_stripedBufferArray)) {
        static HdStBufferResourceGLNamedList empty;
        return empty;
    }
    return _stripedBufferArray->GetResources();
}

void
HdStVBOMemoryManager::_StripedBufferArrayRange::SetBufferArray(HdBufferArray *bufferArray)
{
    _stripedBufferArray = static_cast<_StripedBufferArray *>(bufferArray);    
}

void
HdStVBOMemoryManager::_StripedBufferArrayRange::DebugDump(std::ostream &out) const
{
    out << "[StripedBAR] offset = " << _offset
        << ", numElements = " << _numElements
        << ", capacity = " << _capacity
        << "\n";
}

const void *
HdStVBOMemoryManager::_StripedBufferArrayRange::_GetAggregation() const
{
    return _stripedBufferArray;
}

PXR_NAMESPACE_CLOSE_SCOPE

