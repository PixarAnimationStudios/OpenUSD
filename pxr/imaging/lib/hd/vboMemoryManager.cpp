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
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/iterator.h"

#include "pxr/imaging/hd/bufferResource.h"
#include "pxr/imaging/hd/glUtils.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderContextCaps.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vboMemoryManager.h"

#include "pxr/imaging/hf/perfLog.h"

TF_INSTANTIATE_SINGLETON(HdVBOMemoryManager);

TF_DEFINE_ENV_SETTING(HD_MAX_VBO_SIZE, (1*1024*1024*1024),
                      "Maximum aggregated VBO size");

// ---------------------------------------------------------------------------
//  HdVBOMemoryManager
// ---------------------------------------------------------------------------
HdBufferArraySharedPtr
HdVBOMemoryManager::CreateBufferArray(
    TfToken const &role,
    HdBufferSpecVector const &bufferSpecs)
{
    return boost::make_shared<HdVBOMemoryManager::_StripedBufferArray>(
        role, bufferSpecs);
}


HdBufferArrayRangeSharedPtr
HdVBOMemoryManager::CreateBufferArrayRange()
{
    return boost::make_shared<_StripedBufferArrayRange>();
}


HdAggregationStrategy::AggregationId
HdVBOMemoryManager::ComputeAggregationId(HdBufferSpecVector const &bufferSpecs) const
{
    uint32_t hash = 0;
    TF_FOR_ALL(it, bufferSpecs) {
        size_t nameHash = it->name.Hash();
        hash = ArchHash((const char*)&nameHash, sizeof(nameHash), hash);
        hash = ArchHash((const char*)&(it->glDataType), sizeof(it->glDataType), hash);
        hash = ArchHash((const char*)&(it->numComponents), sizeof(it->numComponents), hash);
    }
    // promote to size_t
    return (AggregationId)hash;
}

// ---------------------------------------------------------------------------
//  _StripedBufferArray
// ---------------------------------------------------------------------------
HdVBOMemoryManager::_StripedBufferArray::_StripedBufferArray(
    TfToken const &role,
    HdBufferSpecVector const &bufferSpecs)
    : HdBufferArray(role, HdPerfTokens->garbageCollectedVbo),
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
        int stride = HdConversions::GetComponentSize(it->glDataType) *
            it->numComponents;
        _AddResource(it->name,
                     it->glDataType,
                     it->numComponents,
                     it->arraySize,
                     /*offset=*/0,
                     /*stride=*/stride);
    }

    // VBO Memory Manage supports an effectivly limitless set of ranges
    _SetMaxNumRanges(std::numeric_limits<size_t>::max());

    // compute max bytes / elements
    TF_FOR_ALL (it, GetResources()) {
        HdBufferResourceSharedPtr const &bres = it->second;
        _maxBytesPerElement = std::max(
            _maxBytesPerElement,
            bres->GetNumComponents() * bres->GetComponentSize());
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


HdVBOMemoryManager::_StripedBufferArray::~_StripedBufferArray()
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
HdVBOMemoryManager::_StripedBufferArray::GarbageCollect()
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
HdVBOMemoryManager::_StripedBufferArray::Reallocate(
    std::vector<HdBufferArrayRangeSharedPtr> const &ranges,
    HdBufferArraySharedPtr const &curRangeOwner)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // XXX: make sure glcontext
    HdRenderContextCaps const &caps = HdRenderContextCaps::GetInstance();

    HD_PERF_COUNTER_INCR(HdPerfTokens->vboRelocated);

    if (!TF_VERIFY(GetResources().size() ==
                      curRangeOwner->GetResources().size())) {
        TF_CODING_ERROR("Resource mismatch when reallocating buffer array");
        return;
    }

    if (TfDebug::IsEnabled(HD_SAFE_MODE)) {
        HdBufferResourceNamedList::size_type bresIdx = 0;
        TF_FOR_ALL(bresIt, GetResources()) {
            TF_VERIFY(curRangeOwner->GetResources()[bresIdx++].second ==
                      curRangeOwner->GetResource(bresIt->first));
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
    HdBufferResourceNamedList const& resources = GetResources();
    for (size_t bresIdx=0; bresIdx<resources.size(); ++bresIdx) {
        HdBufferResourceSharedPtr const &bres = resources[bresIdx].second;
        HdBufferResourceSharedPtr const &curRes =
                curRangeOwner->GetResources()[bresIdx].second;

        int bytesPerElement =
            bres->GetNumComponents() * bres->GetComponentSize();
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
                HdGLBufferRelocator relocator(curId, newId);
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
HdVBOMemoryManager::_StripedBufferArray::_DeallocateResources()
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
HdVBOMemoryManager::_StripedBufferArray::GetMaxNumElements() const
{
    static size_t vboMaxSize = TfGetEnvSetting(HD_MAX_VBO_SIZE);
    return vboMaxSize / _maxBytesPerElement;
}

void
HdVBOMemoryManager::_StripedBufferArray::DebugDump(std::ostream &out) const
{
    out << "  HdVBOMemoryManager\n";
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

// ---------------------------------------------------------------------------
//  _StripedBufferArrayRange
// ---------------------------------------------------------------------------
HdVBOMemoryManager::_StripedBufferArrayRange::~_StripedBufferArrayRange()
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
HdVBOMemoryManager::_StripedBufferArrayRange::IsAssigned() const
{
    return (_stripedBufferArray != nullptr);
}


bool
HdVBOMemoryManager::_StripedBufferArrayRange::Resize(int numElements)
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
        _stripedBufferArray->SetNeedsReallocation();
        needsReallocation = true;
    }
#endif

    _numElements = numElements;
    return needsReallocation;
}
void
HdVBOMemoryManager::_StripedBufferArrayRange::CopyData(
    HdBufferSourceSharedPtr const &bufferSource)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(_stripedBufferArray)) return;

    HdBufferResourceSharedPtr VBO =
        _stripedBufferArray->GetResource(bufferSource->GetName());

    if (!TF_VERIFY((VBO && VBO->GetId()),
                      "VBO doesn't exist for %s",
                      bufferSource->GetName().GetText())) {
        return;
    }

    // datatype of bufferSource has to match with bufferResource
    if (!TF_VERIFY(bufferSource->GetGLComponentDataType() == VBO->GetGLDataType(),
                      "%s: 0x%x != 0x%x\n",
                      bufferSource->GetName().GetText(),
                      bufferSource->GetGLComponentDataType(),
                      VBO->GetGLDataType()) ||
        !TF_VERIFY(bufferSource->GetNumComponents() == VBO->GetNumComponents(),
                      "%s: %d != %d\n",
                      bufferSource->GetName().GetText(),
                      bufferSource->GetNumComponents(),
                      VBO->GetNumComponents())) {
        return;
    }

    HdRenderContextCaps const &caps = HdRenderContextCaps::GetInstance();
    if (glBufferSubData) {
        int bytesPerElement =
            VBO->GetNumComponents() * VBO->GetComponentSize();

        // overrun check. for graceful handling of erroneous assets,
        // issue warning here and continue to copy for the valid range.
        size_t dstSize = _numElements * bytesPerElement;
        size_t srcSize = bufferSource->GetSize();
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
HdVBOMemoryManager::_StripedBufferArrayRange::ReadData(TfToken const &name) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    VtValue result;
    if (!TF_VERIFY(_stripedBufferArray)) return result;

    HdBufferResourceSharedPtr VBO = _stripedBufferArray->GetResource(name);

    if (!VBO || (VBO->GetId() == 0 && _numElements > 0)) {
        TF_CODING_ERROR("VBO doesn't exist for %s", name.GetText());
        return result;
    }

    GLintptr vboOffset = VBO->GetNumComponents() *
        HdConversions::GetComponentSize(VBO->GetGLDataType()) *
        _offset;

    result = HdGLUtils::ReadBuffer(VBO->GetId(),
                                   VBO->GetGLDataType(),
                                   VBO->GetNumComponents(),
                                   VBO->GetArraySize(),
                                   vboOffset,
                                   /*stride=*/0,  // not interleaved.
                                   _numElements);

    return result;
}

size_t
HdVBOMemoryManager::_StripedBufferArrayRange::GetMaxNumElements() const
{
    return _stripedBufferArray->GetMaxNumElements();
}

HdBufferResourceSharedPtr
HdVBOMemoryManager::_StripedBufferArrayRange::GetResource() const
{
    if (!TF_VERIFY(_stripedBufferArray)) return HdBufferResourceSharedPtr();

    return _stripedBufferArray->GetResource();
}

HdBufferResourceSharedPtr
HdVBOMemoryManager::_StripedBufferArrayRange::GetResource(TfToken const& name)
{
    if (!TF_VERIFY(_stripedBufferArray)) return HdBufferResourceSharedPtr();

    return _stripedBufferArray->GetResource(name);
}

HdBufferResourceNamedList const&
HdVBOMemoryManager::_StripedBufferArrayRange::GetResources() const
{
    if (!TF_VERIFY(_stripedBufferArray)) {
        static HdBufferResourceNamedList empty;
        return empty;
    }
    return _stripedBufferArray->GetResources();
}

void
HdVBOMemoryManager::_StripedBufferArrayRange::SetBufferArray(HdBufferArray *bufferArray)
{
    _stripedBufferArray = static_cast<_StripedBufferArray *>(bufferArray);    
}

void
HdVBOMemoryManager::_StripedBufferArrayRange::DebugDump(std::ostream &out) const
{
    out << "[StripedBAR] offset = " << _offset
        << ", numElements = " << _numElements
        << ", capacity = " << _capacity
        << "\n";
}

const void *
HdVBOMemoryManager::_StripedBufferArrayRange::_GetAggregation() const
{
    return _stripedBufferArray;
}
