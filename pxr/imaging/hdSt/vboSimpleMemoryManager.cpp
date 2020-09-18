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
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/iterator.h"

#include "pxr/imaging/hdSt/bufferResource.h"
#include "pxr/imaging/hdSt/glUtils.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hdSt/vboSimpleMemoryManager.h"

#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/blitCmds.h"
#include "pxr/imaging/hgi/blitCmdsOps.h"
#include "pxr/imaging/hgi/buffer.h"

#include "pxr/imaging/hf/perfLog.h"

#include <atomic>

#include <boost/functional/hash.hpp>

PXR_NAMESPACE_OPEN_SCOPE


extern TfEnvSetting<int> HD_MAX_VBO_SIZE;

// ---------------------------------------------------------------------------
//  HdStVBOSimpleMemoryManager
// ---------------------------------------------------------------------------

HdBufferArraySharedPtr
HdStVBOSimpleMemoryManager::CreateBufferArray(
    TfToken const &role,
    HdBufferSpecVector const &bufferSpecs,
    HdBufferArrayUsageHint usageHint)
{
    return std::make_shared<HdStVBOSimpleMemoryManager::_SimpleBufferArray>(
        _resourceRegistry, role, bufferSpecs, usageHint);
}

HdBufferArrayRangeSharedPtr
HdStVBOSimpleMemoryManager::CreateBufferArrayRange()
{
    return std::make_shared<HdStVBOSimpleMemoryManager::_SimpleBufferArrayRange>
                (_resourceRegistry);
}

HdAggregationStrategy::AggregationId
HdStVBOSimpleMemoryManager::ComputeAggregationId(
    HdBufferSpecVector const &bufferSpecs,
    HdBufferArrayUsageHint usageHint) const
{
    // Always returns different value
    static std::atomic_uint id(0);

    AggregationId hash = id++;  // Atomic

    return hash;
}

/// Returns the buffer specs from a given buffer array
HdBufferSpecVector 
HdStVBOSimpleMemoryManager::GetBufferSpecs(
    HdBufferArraySharedPtr const &bufferArray) const
{
    _SimpleBufferArraySharedPtr bufferArray_ =
        std::static_pointer_cast<_SimpleBufferArray> (bufferArray);
    return bufferArray_->GetBufferSpecs();
}

/// Returns the size of the GPU memory used by the passed buffer array
size_t 
HdStVBOSimpleMemoryManager::GetResourceAllocation(
    HdBufferArraySharedPtr const &bufferArray, 
    VtDictionary &result) const 
{ 
    std::set<uint64_t> idSet;
    size_t gpuMemoryUsed = 0;

    _SimpleBufferArraySharedPtr bufferArray_ =
        std::static_pointer_cast<_SimpleBufferArray> (bufferArray);

    TF_FOR_ALL(resIt, bufferArray_->GetResources()) {
        HdStBufferResourceSharedPtr const & resource = resIt->second;

        // XXX Reallocate inserts an empty (invalid) handle for empty buffers.
        HgiBufferHandle buffer = resource->GetId();
        uint64_t id = buffer ? buffer->GetRawResource() : 0;

        // XXX avoid double counting of resources shared within a buffer
        if (id > 0 && idSet.count(id) == 0) {
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
//  _SimpleBufferArray
// ---------------------------------------------------------------------------
HdStVBOSimpleMemoryManager::_SimpleBufferArray::_SimpleBufferArray(
    HdStResourceRegistry* resourceRegistry,
    TfToken const &role,
    HdBufferSpecVector const &bufferSpecs,
    HdBufferArrayUsageHint usageHint)
 : HdBufferArray(role, TfToken(), usageHint)
 , _resourceRegistry(resourceRegistry)
 , _capacity(0)
 , _maxBytesPerElement(0)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // populate BufferResources
    TF_FOR_ALL(it, bufferSpecs) {
        int stride = HdDataSizeOfTupleType(it->tupleType);
        _AddResource(it->name, it->tupleType, /*offset=*/0, stride);
    }

    _SetMaxNumRanges(1);

    // compute max bytes / elements
    TF_FOR_ALL (it, GetResources()) {
        HdStBufferResourceSharedPtr const &bres = it->second;
        _maxBytesPerElement = std::max(
            _maxBytesPerElement,
            HdDataSizeOfTupleType(bres->GetTupleType()));
    }
}

HdStBufferResourceSharedPtr
HdStVBOSimpleMemoryManager::_SimpleBufferArray::_AddResource(
    TfToken const& name,
    HdTupleType tupleType,
                            int offset,
                            int stride)
{
    HD_TRACE_FUNCTION();
    if (TfDebug::IsEnabled(HD_SAFE_MODE)) {
        // duplication check
        HdStBufferResourceSharedPtr bufferRes = GetResource(name);
        if (!TF_VERIFY(!bufferRes)) {
            return bufferRes;
        }
    }

    HdStBufferResourceSharedPtr bufferRes = HdStBufferResourceSharedPtr(
        new HdStBufferResource(GetRole(), tupleType, offset, stride));
    _resourceList.emplace_back(name, bufferRes);
    return bufferRes;
}

HdStVBOSimpleMemoryManager::_SimpleBufferArray::~_SimpleBufferArray()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // invalidate buffer array range
    // (the range may still be held by drawItems)
    _SimpleBufferArrayRangeSharedPtr range = _GetRangeSharedPtr();
    if (range) {
        range->Invalidate();
    }
}


bool
HdStVBOSimpleMemoryManager::_SimpleBufferArray::GarbageCollect()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // no range referring this buffer = empty
    if (GetRangeCount() > 0 && GetRange(0).expired()) {
        _DeallocateResources();
        HD_PERF_COUNTER_INCR(HdPerfTokens->garbageCollectedVbo);
        return true;
    }
    return false;
}

void
HdStVBOSimpleMemoryManager::_SimpleBufferArray::DebugDump(std::ostream &out) const
{
    out << "  HdStVBOSimpleMemoryManager";
    out << "  total capacity = " << _capacity << "\n";
}

bool
HdStVBOSimpleMemoryManager::_SimpleBufferArray::Resize(int numElements)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // see the comment in
    // HdStVBOMemoryManager::_StripedBufferArrayRange::Resize(int numElements)
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
HdStVBOSimpleMemoryManager::_SimpleBufferArray::Reallocate(
    std::vector<HdBufferArrayRangeSharedPtr> const & ranges,
    HdBufferArraySharedPtr const &curRangeOwner)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HD_PERF_COUNTER_INCR(HdPerfTokens->vboRelocated);

    if (!TF_VERIFY(curRangeOwner == shared_from_this())) {
        TF_CODING_ERROR("HdStVBOSimpleMemoryManager can't reassign ranges");
        return;
    }

    if (ranges.size() > 1) {
        TF_CODING_ERROR("HdStVBOSimpleMemoryManager can't take multiple ranges");
        return;
    }
    _SetRangeList(ranges);

    _SimpleBufferArrayRangeSharedPtr range = _GetRangeSharedPtr();

    if (!range) {
        TF_CODING_ERROR("_SimpleBufferArrayRange expired unexpectedly.");
        return;
    }

    int numElements = range->GetNumElements();

    // Use blit work to record resource copy commands.
    Hgi* hgi = _resourceRegistry->GetHgi();
    HgiBlitCmds* blitCmds = _resourceRegistry->GetGlobalBlitCmds();
    blitCmds->PushDebugGroup(__ARCH_PRETTY_FUNCTION__);
    
    TF_FOR_ALL (bresIt, GetResources()) {
        HdStBufferResourceSharedPtr const &bres = bresIt->second;

        // XXX:Arrays: We should use HdDataSizeOfTupleType() here, to
        // add support for array types.
        int bytesPerElement = HdDataSizeOfType(bres->GetTupleType().type);
        size_t bufferSize = bytesPerElement * numElements;

        HgiBufferHandle oldId = bres->GetId();
        HgiBufferHandle newId;

        if(bufferSize > 0) {
            HgiBufferDesc bufDesc;
            bufDesc.byteSize = bufferSize;
            bufDesc.usage = HgiBufferUsageUniform;
            newId = hgi->CreateBuffer(bufDesc);
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
        size_t copySize = std::min(oldSize, newSize) * bytesPerElement;
        if (copySize > 0 && oldId) {
            HD_PERF_COUNTER_INCR(HdStPerfTokens->copyBufferGpuToGpu);

            HgiBufferGpuToGpuOp blitOp;
            blitOp.gpuSourceBuffer = oldId;
            blitOp.gpuDestinationBuffer = newId;
            blitOp.byteSize = copySize;
            blitCmds->CopyBufferGpuToGpu(blitOp);
        }

        // delete old buffer
        if (oldId) {
            hgi->DestroyBuffer(&oldId);
        }

        bres->SetAllocation(newId, bufferSize);
    }

    blitCmds->PopDebugGroup();

    _capacity = numElements;
    _needsReallocation = false;

    // increment version to rebuild dispatch buffers.
    IncrementVersion();
}

size_t
HdStVBOSimpleMemoryManager::_SimpleBufferArray::GetMaxNumElements() const
{
    static size_t vboMaxSize = TfGetEnvSetting(HD_MAX_VBO_SIZE);
    return vboMaxSize / _maxBytesPerElement;
}

void
HdStVBOSimpleMemoryManager::_SimpleBufferArray::_DeallocateResources()
{
    Hgi* hgi = _resourceRegistry->GetHgi();
    TF_FOR_ALL (it, GetResources()) {
        hgi->DestroyBuffer(&it->second->GetId());
    }
}

HdStBufferResourceSharedPtr
HdStVBOSimpleMemoryManager::_SimpleBufferArray::GetResource() const
{
    HD_TRACE_FUNCTION();

    if (_resourceList.empty()) return HdStBufferResourceSharedPtr();

    if (TfDebug::IsEnabled(HD_SAFE_MODE)) {
        // make sure this buffer array has only one resource.
        HgiBufferHandle const& id = _resourceList.begin()->second->GetId();
        TF_FOR_ALL (it, _resourceList) {
            if (it->second->GetId() != id) {
                TF_CODING_ERROR("GetResource(void) called on"
                                "HdBufferArray having multiple GPU resources");
            }
        }
    }

    // returns the first item
    return _resourceList.begin()->second;
}

HdStBufferResourceSharedPtr
HdStVBOSimpleMemoryManager::_SimpleBufferArray::GetResource(TfToken const& name)
{
    HD_TRACE_FUNCTION();

    // linear search.
    // The number of buffer resources should be small (<10 or so).
    for (HdStBufferResourceNamedList::iterator it = _resourceList.begin();
         it != _resourceList.end(); ++it) {
        if (it->first == name) return it->second;
    }
    return HdStBufferResourceSharedPtr();
}

HdBufferSpecVector
HdStVBOSimpleMemoryManager::_SimpleBufferArray::GetBufferSpecs() const
{
    HdBufferSpecVector result;
    result.reserve(_resourceList.size());
    TF_FOR_ALL (it, _resourceList) {
        result.emplace_back(it->first, it->second->GetTupleType());
    }
    return result;
}

// ---------------------------------------------------------------------------
//  _SimpleBufferArrayRange
// ---------------------------------------------------------------------------
bool
HdStVBOSimpleMemoryManager::_SimpleBufferArrayRange::IsAssigned() const
{
    return (_bufferArray != nullptr);
}

bool
HdStVBOSimpleMemoryManager::_SimpleBufferArrayRange::IsImmutable() const
{
    return (_bufferArray != nullptr)
         && _bufferArray->IsImmutable();
}

void
HdStVBOSimpleMemoryManager::_SimpleBufferArrayRange::CopyData(
    HdBufferSourceSharedPtr const &bufferSource)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(_bufferArray)) return;

    int offset = 0;

    HdStBufferResourceSharedPtr VBO =
        _bufferArray->GetResource(bufferSource->GetName());

    if (!VBO || !VBO->GetId()) {
        TF_CODING_ERROR("VBO doesn't exist for %s",
                        bufferSource->GetName().GetText());
        return;
    }

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

    size_t vboOffset = bytesPerElement * offset;

    HD_PERF_COUNTER_INCR(HdStPerfTokens->copyBufferCpuToGpu);

    HgiBufferCpuToGpuOp blitOp;
    blitOp.cpuSourceBuffer = bufferSource->GetData();
    blitOp.gpuDestinationBuffer = VBO->GetId();
    
    blitOp.sourceByteOffset = 0;
    blitOp.byteSize = srcSize;
    blitOp.destinationByteOffset = vboOffset;

    HgiBlitCmds* blitCmds = GetResourceRegistry()->GetGlobalBlitCmds();
    blitCmds->CopyBufferCpuToGpu(blitOp);
}

VtValue
HdStVBOSimpleMemoryManager::_SimpleBufferArrayRange::ReadData(TfToken const &name) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(_bufferArray)) return VtValue();

    HdStBufferResourceSharedPtr VBO = _bufferArray->GetResource(name);

    if (!VBO || (!VBO->GetId() && _numElements > 0)) {
        TF_CODING_ERROR("VBO doesn't exist for %s", name.GetText());
        return VtValue();
    }

    return HdStGLUtils::ReadBuffer(VBO->GetId()->GetRawResource(),
                                   VBO->GetTupleType(),
                                 /*offset=*/0,
                                 /*stride=*/0,  // not interleaved.
                                 _numElements);
}

size_t
HdStVBOSimpleMemoryManager::_SimpleBufferArrayRange::GetMaxNumElements() const
{
    return _bufferArray->GetMaxNumElements();
}

HdBufferArrayUsageHint
HdStVBOSimpleMemoryManager::_SimpleBufferArrayRange::GetUsageHint() const
{
    if (!TF_VERIFY(_bufferArray)) {
        return HdBufferArrayUsageHint();
    }

    return _bufferArray->GetUsageHint();
}

HdStBufferResourceSharedPtr
HdStVBOSimpleMemoryManager::_SimpleBufferArrayRange::GetResource() const
{
    if (!TF_VERIFY(_bufferArray)) return HdStBufferResourceSharedPtr();

    return _bufferArray->GetResource();
}

HdStBufferResourceSharedPtr
HdStVBOSimpleMemoryManager::_SimpleBufferArrayRange::GetResource(TfToken const& name)
{
    if (!TF_VERIFY(_bufferArray)) return HdStBufferResourceSharedPtr();
    return _bufferArray->GetResource(name);
}

HdStBufferResourceNamedList const&
HdStVBOSimpleMemoryManager::_SimpleBufferArrayRange::GetResources() const
{
    if (!TF_VERIFY(_bufferArray)) {
        static HdStBufferResourceNamedList empty;
        return empty;
    }
    return _bufferArray->GetResources();
}

void
HdStVBOSimpleMemoryManager::_SimpleBufferArrayRange::SetBufferArray(HdBufferArray *bufferArray)
{
    _bufferArray = static_cast<_SimpleBufferArray *>(bufferArray);    
}

void
HdStVBOSimpleMemoryManager::_SimpleBufferArrayRange::DebugDump(std::ostream &out) const
{
    out << "[SimpleBAR] numElements = " << _numElements
        << "\n";
}

const void *
HdStVBOSimpleMemoryManager::_SimpleBufferArrayRange::_GetAggregation() const
{
    return _bufferArray;
}

PXR_NAMESPACE_CLOSE_SCOPE

