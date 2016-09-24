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

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/glUtils.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderContextCaps.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vboSimpleMemoryManager.h"

#include <atomic>

#include <boost/functional/hash.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

TF_INSTANTIATE_SINGLETON(HdVBOSimpleMemoryManager);

extern TfEnvSetting<int> HD_MAX_VBO_SIZE;

// ---------------------------------------------------------------------------
//  HdVBOSimpleMemoryManager
// ---------------------------------------------------------------------------

HdBufferArraySharedPtr
HdVBOSimpleMemoryManager::CreateBufferArray(
    TfToken const &role,
    HdBufferSpecVector const &bufferSpecs)
{
    return boost::make_shared<HdVBOSimpleMemoryManager::_SimpleBufferArray>(
        role, bufferSpecs);
}

HdBufferArrayRangeSharedPtr
HdVBOSimpleMemoryManager::CreateBufferArrayRange()
{
    return boost::make_shared<HdVBOSimpleMemoryManager::_SimpleBufferArrayRange>();
}

HdAggregationStrategy::AggregationId
HdVBOSimpleMemoryManager::ComputeAggregationId(
    HdBufferSpecVector const &bufferSpecs) const
{
    // Always returns different value
    static std::atomic_uint id(0);

    AggregationId hash = id++;  // Atomic

    return hash;
}

// ---------------------------------------------------------------------------
//  _SimpleBufferArray
// ---------------------------------------------------------------------------
HdVBOSimpleMemoryManager::_SimpleBufferArray::_SimpleBufferArray(
    TfToken const &role,
    HdBufferSpecVector const &bufferSpecs)
    : HdBufferArray(role, TfToken()), _capacity(0), _maxBytesPerElement(0)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    // populate BufferResources
    TF_FOR_ALL(it, bufferSpecs) {
        int stride = static_cast<int>(
			HdConversions::GetComponentSize(it->glDataType) *
            it->numComponents);
        _AddResource(it->name,
                     it->glDataType,
                     it->numComponents,
                     it->arraySize,
                     /*offset=*/0,
                     /*stride=*/stride);
    }

    _SetMaxNumRanges(1);

    // compute max bytes / elements
    TF_FOR_ALL (it, GetResources()) {
        HdBufferResourceSharedPtr const &bres = it->second;
        _maxBytesPerElement = std::max(
            _maxBytesPerElement,
            bres->GetNumComponents() * bres->GetComponentSize());
    }
}

HdVBOSimpleMemoryManager::_SimpleBufferArray::~_SimpleBufferArray()
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    // invalidate buffer array range
    // (the range may still be held by drawItems)
    _SimpleBufferArrayRangeSharedPtr range = _GetRangeSharedPtr();
    if (range) {
        range->Invalidate();
    }
}


bool
HdVBOSimpleMemoryManager::_SimpleBufferArray::GarbageCollect()
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    // no range referring this buffer = empty
    if (GetRangeCount() > 0 and GetRange(0).expired()) {
        _DeallocateResources();
        HD_PERF_COUNTER_INCR(HdPerfTokens->garbageCollectedVbo);
        return true;
    }
    return false;
}

void
HdVBOSimpleMemoryManager::_SimpleBufferArray::DebugDump(std::ostream &out) const
{
    out << "  HdVBOSimpleMemoryManager";
    out << "  total capacity = " << _capacity << "\n";
}

bool
HdVBOSimpleMemoryManager::_SimpleBufferArray::Resize(int numElements)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    // see the comment in
    // HdVBOMemoryManager::_StripedBufferArrayRange::Resize(int numElements)
    // this change is for the unit test consistency.
    //
    // if (_capacity < numElements) {
    if (_capacity != numElements) {
        _needsReallocation = true;
        return true;
    }
    return false;
}

void
HdVBOSimpleMemoryManager::_SimpleBufferArray::Reallocate(
    std::vector<HdBufferArrayRangeSharedPtr> const & ranges,
    HdBufferArraySharedPtr const &curRangeOwner)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    // XXX: make sure glcontext
    HdRenderContextCaps const &caps = HdRenderContextCaps::GetInstance();

    HD_PERF_COUNTER_INCR(HdPerfTokens->vboRelocated);

    if (not TF_VERIFY(curRangeOwner == shared_from_this())) {
        TF_CODING_ERROR("HdVBOSimpleMemoryManager can't reassign ranges");
        return;
    }

    if (ranges.size() > 1) {
        TF_CODING_ERROR("HdVBOSimpleMemoryManager can't take multiple ranges");
        return;
    }
    _SetRangeList(ranges);

    _SimpleBufferArrayRangeSharedPtr range = _GetRangeSharedPtr();

    if (not range) {
        TF_CODING_ERROR("_SimpleBufferArrayRange expired unexpectedly.");
        return;
    }
    int numElements = range->GetNumElements();

    TF_FOR_ALL (bresIt, GetResources()) {
        HdBufferResourceSharedPtr const &bres = bresIt->second;

        int bytesPerElement = bres->GetNumComponents() * 
							  static_cast<int>(bres->GetComponentSize());
        GLsizeiptr bufferSize = bytesPerElement * numElements;

        if (glGenBuffers) {
            // allocate new one
            GLuint newId = 0;
            GLuint oldId = bres->GetId();

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
            GLsizeiptr copySize = std::min(oldSize, newSize) * bytesPerElement;
            if (copySize > 0) {
                HD_PERF_COUNTER_INCR(HdPerfTokens->glCopyBufferSubData);

                if (caps.copyBufferEnabled) {
                    if (ARCH_LIKELY(caps.directStateAccessEnabled)) {
                        glNamedCopyBufferSubDataEXT(oldId, newId, 0, 0, copySize);
                    } else {
                        glBindBuffer(GL_COPY_READ_BUFFER, oldId);
                        glBindBuffer(GL_COPY_WRITE_BUFFER, newId);
                        glCopyBufferSubData(GL_COPY_READ_BUFFER,
                                            GL_COPY_WRITE_BUFFER, 0, 0, copySize);
                        glBindBuffer(GL_COPY_READ_BUFFER, 0);
                        glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
                    }
                } else {
                    // driver issues workaround
                    std::vector<char> data(copySize);
                    glBindBuffer(GL_ARRAY_BUFFER, oldId);
                    glGetBufferSubData(GL_ARRAY_BUFFER, 0, copySize, &data[0]);
                    glBindBuffer(GL_ARRAY_BUFFER, newId);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, copySize, &data[0]);
                    glBindBuffer(GL_ARRAY_BUFFER, 0);
                }
            }

            // delete old buffer
            glDeleteBuffers(1, &oldId);

            bres->SetAllocation(newId, bufferSize);
        } else {
            // for unit test
            static int id = 1;
            bres->SetAllocation(id++, bufferSize);
        }
    }

    _capacity = numElements;
    _needsReallocation = false;

    // increment version to rebuild dispatch buffers.
    IncrementVersion();
}

size_t
HdVBOSimpleMemoryManager::_SimpleBufferArray::GetMaxNumElements() const
{
    static size_t vboMaxSize = TfGetEnvSetting(HD_MAX_VBO_SIZE);
    return vboMaxSize / _maxBytesPerElement;
}

void
HdVBOSimpleMemoryManager::_SimpleBufferArray::_DeallocateResources()
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

// ---------------------------------------------------------------------------
//  _SimpleBufferArrayRange
// ---------------------------------------------------------------------------
bool
HdVBOSimpleMemoryManager::_SimpleBufferArrayRange::IsAssigned() const
{
    return (_bufferArray != nullptr);
}

void
HdVBOSimpleMemoryManager::_SimpleBufferArrayRange::CopyData(
    HdBufferSourceSharedPtr const &bufferSource)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    if (not TF_VERIFY(_bufferArray)) return;

    int offset = 0;

    HdBufferResourceSharedPtr VBO =
        _bufferArray->GetResource(bufferSource->GetName());

    if (not VBO or VBO->GetId() == 0) {
        TF_CODING_ERROR("VBO doesn't exist for %s",
                        bufferSource->GetName().GetText());
        return;
    }
    HdRenderContextCaps const &caps = HdRenderContextCaps::GetInstance();

    if (glBufferSubData != NULL) {
        int bytesPerElement =
            VBO->GetNumComponents() * 
			static_cast<unsigned int>(VBO->GetComponentSize());
        // overrun check. for graceful handling of erroneous assets,
        // issue warning here and continue to copy for the valid range.
        size_t dstSize = _numElements * bytesPerElement;
        size_t srcSize = bufferSource->GetSize();
        if (srcSize > dstSize) {
            TF_WARN("%s: size %ld is larger than the range (%ld)",
                    bufferSource->GetName().GetText(), srcSize, dstSize);
            srcSize = dstSize;
        }

        GLintptr vboOffset = bytesPerElement * offset;

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
HdVBOSimpleMemoryManager::_SimpleBufferArrayRange::ReadData(TfToken const &name) const
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    if (not TF_VERIFY(_bufferArray)) return VtValue();

    HdBufferResourceSharedPtr VBO = _bufferArray->GetResource(name);

    if (not VBO or (VBO->GetId() == 0 and _numElements > 0)) {
        TF_CODING_ERROR("VBO doesn't exist for %s", name.GetText());
        return VtValue();
    }

    return HdGLUtils::ReadBuffer(VBO->GetId(),
                                 VBO->GetGLDataType(),
                                 VBO->GetNumComponents(),
                                 VBO->GetArraySize(),
                                 /*offset=*/0,
                                 /*stride=*/0,  // not interleaved.
                                 _numElements);
}

size_t
HdVBOSimpleMemoryManager::_SimpleBufferArrayRange::GetMaxNumElements() const
{
    return _bufferArray->GetMaxNumElements();
}

HdBufferResourceSharedPtr
HdVBOSimpleMemoryManager::_SimpleBufferArrayRange::GetResource() const
{
    if (not TF_VERIFY(_bufferArray)) return HdBufferResourceSharedPtr();

    return _bufferArray->GetResource();
}

HdBufferResourceSharedPtr
HdVBOSimpleMemoryManager::_SimpleBufferArrayRange::GetResource(TfToken const& name)
{
    if (not TF_VERIFY(_bufferArray)) return HdBufferResourceSharedPtr();
    return _bufferArray->GetResource(name);
}

HdBufferResourceNamedList const&
HdVBOSimpleMemoryManager::_SimpleBufferArrayRange::GetResources() const
{
    if (not TF_VERIFY(_bufferArray)) {
        static HdBufferResourceNamedList empty;
        return empty;
    }
    return _bufferArray->GetResources();
}

void
HdVBOSimpleMemoryManager::_SimpleBufferArrayRange::SetBufferArray(HdBufferArray *bufferArray)
{
    _bufferArray = static_cast<_SimpleBufferArray *>(bufferArray);    
}

void
HdVBOSimpleMemoryManager::_SimpleBufferArrayRange::DebugDump(std::ostream &out) const
{
    out << "[SimpleBAR] numElements = " << _numElements
        << "\n";
}

const void *
HdVBOSimpleMemoryManager::_SimpleBufferArrayRange::_GetAggregation() const
{
    return _bufferArray;
}
