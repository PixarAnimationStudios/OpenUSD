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
#include "pxr/imaging/hdSt/dispatchBuffer.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/imaging/hf/perfLog.h"

#include "pxr/imaging/hgi/blitCmds.h"
#include "pxr/imaging/hgi/blitCmdsOps.h"
#include "pxr/imaging/hgi/buffer.h"
#include "pxr/imaging/hgi/hgi.h"

PXR_NAMESPACE_OPEN_SCOPE

class Hd_DispatchBufferArrayRange : public HdStBufferArrayRange
{
public:
    /// Constructor.
    Hd_DispatchBufferArrayRange(
        HdStResourceRegistry* resourceRegistry, HdStDispatchBuffer *buffer)
      : HdStBufferArrayRange(resourceRegistry)
      , _buffer(buffer) {
    }

    /// Returns true if this range is valid
    bool IsValid() const override {
        return true;
    }

    /// Returns true is the range has been assigned to a buffer
    bool IsAssigned() const override {
        return _buffer != nullptr;
    }

    /// Dispatch buffer array range is always mutable
    bool IsImmutable() const override {
        return false;
    }

    /// Resize memory area for this range. Returns true if it causes container
    /// buffer reallocation.
    bool Resize(int numElements) override {
        TF_CODING_ERROR("Hd_DispatchBufferArrayRange doesn't support this operation");
        return false;
    }

    /// Copy source data into buffer
    void CopyData(HdBufferSourceSharedPtr const &bufferSource) override {
        TF_CODING_ERROR("Hd_DispatchBufferArrayRange doesn't support this operation");
    }

    /// Read back the buffer content
    VtValue ReadData(TfToken const &name) const override {
        TF_CODING_ERROR("Hd_DispatchBufferArrayRange doesn't support this operation");
        return VtValue();
    }

    /// Returns the offset at which this range begins in the underlying buffer 
    /// array in terms of elements.
    int GetElementOffset() const override {
        TF_CODING_ERROR("Hd_DispatchBufferArrayRange doesn't support this operation");
        return 0;
    }

    /// Returns the byte offset at which this range begins in the underlying
    /// buffer array for the given resource.
    int GetByteOffset(TfToken const& resourceName) const override {
        TF_CODING_ERROR("Hd_DispatchBufferArrayRange doesn't support this operation");
        return 0;
    }

    /// Returns the number of elements allocated
    size_t GetNumElements() const override {
        TF_CODING_ERROR("Hd_DispatchBufferArrayRange doesn't support this operation");
        return 0;
    }

    /// Returns the capacity of allocated area for this range
    virtual int GetCapacity() const {
        TF_CODING_ERROR("Hd_DispatchBufferArrayRange doesn't support this operation");
        return 0;
    }

    /// Returns the version of the buffer array.
    virtual size_t GetVersion() const {
        TF_CODING_ERROR("Hd_DispatchBufferArrayRange doesn't support this operation");
        return 0;
    }

    /// Increment the version of the buffer array.
    void IncrementVersion() override {
        TF_CODING_ERROR("Hd_DispatchBufferArrayRange doesn't support this operation");
    }

    /// Returns the max number of elements
    size_t GetMaxNumElements() const override {
        TF_CODING_ERROR("Hd_DispatchBufferArrayRange doesn't support this operation");
        return 1;
    }

    /// Returns the usage hint from the underlying buffer array
    HdBufferArrayUsageHint GetUsageHint() const override {
        return _buffer->GetUsageHint();
    }

    /// Returns the GPU resource. If the buffer array contains more than one
    /// resource, this method raises a coding error.
    HdStBufferResourceSharedPtr GetResource() const override {
        return _buffer->GetResource();
    }

    /// Returns the named GPU resource.
    HdStBufferResourceSharedPtr GetResource(TfToken const& name) override {
        return _buffer->GetResource(name);
    }

    /// Returns the list of all named GPU resources for this bufferArrayRange.
    HdStBufferResourceNamedList const& GetResources() const override {
        return _buffer->GetResources();
    }

    /// Sets the buffer array associated with this buffer;
    void SetBufferArray(HdBufferArray *bufferArray) override {
        TF_CODING_ERROR("Hd_DispatchBufferArrayRange doesn't support this operation");
    }

    /// Debug dump
    void DebugDump(std::ostream &out) const override {
    }

    /// Make this range invalid
    void Invalidate() {
        TF_CODING_ERROR("Hd_DispatchBufferArrayRange doesn't support this operation");
    }

protected:
    /// Returns the aggregation container
    const void *_GetAggregation() const override {
        return this;
    }

private:
    HdStDispatchBuffer *_buffer;
};


HdStDispatchBuffer::HdStDispatchBuffer(
    HdStResourceRegistry* resourceRegistry,
    TfToken const &role,
    int count,
    unsigned int commandNumUints)
 : HdBufferArray(role, TfToken(), HdBufferArrayUsageHint())
 , _resourceRegistry(resourceRegistry)
 , _count(count)
 , _commandNumUints(commandNumUints)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    size_t stride = commandNumUints * sizeof(uint32_t);
    size_t dataSize = count * stride;
    
    // just allocate uninitialized
    HgiBufferDesc bufDesc;
    bufDesc.usage = HgiBufferUsageUniform;
    bufDesc.byteSize = dataSize;
    HgiBufferHandle newId = _resourceRegistry->GetHgi()->CreateBuffer(bufDesc);

    // monolithic resource
    _entireResource = HdStBufferResourceSharedPtr(
        new HdStBufferResource(
            role, {HdTypeInt32, 1},
            /*offset=*/0, stride));
    _entireResource->SetAllocation(newId, dataSize);

    // create a buffer array range, which aggregates all views
    // (will be added by AddBufferResourceView)
    _bar = HdStBufferArrayRangeSharedPtr(
        new Hd_DispatchBufferArrayRange(resourceRegistry, this));
}

HdStDispatchBuffer::~HdStDispatchBuffer()
{
    HgiBufferHandle& id = _entireResource->GetId();
    _resourceRegistry->GetHgi()->DestroyBuffer(&id);
    _entireResource->SetAllocation(HgiBufferHandle(), 0);
}

void
HdStDispatchBuffer::CopyData(std::vector<uint32_t> const &data)
{
    if (!TF_VERIFY(data.size()*sizeof(uint32_t) == static_cast<size_t>(_entireResource->GetSize())))
        return;

    HD_PERF_COUNTER_INCR(HdStPerfTokens->copyBufferCpuToGpu);

    // Use blit op to copy over the data.
    Hgi* hgi = _resourceRegistry->GetHgi();
    HgiBlitCmdsUniquePtr blitCmds = hgi->CreateBlitCmds();
    HgiBufferCpuToGpuOp blitOp;
    blitOp.byteSize = _entireResource->GetSize();
    blitOp.cpuSourceBuffer = data.data();
    blitOp.sourceByteOffset = 0;
    blitOp.gpuDestinationBuffer = _entireResource->GetId();
    blitOp.destinationByteOffset = 0;
    blitCmds->CopyBufferCpuToGpu(blitOp);
    hgi->SubmitCmds(blitCmds.get());
}

void
HdStDispatchBuffer::AddBufferResourceView(
    TfToken const &name, HdTupleType tupleType, int offset)
{
    size_t stride = _commandNumUints * sizeof(uint32_t);

    // add a binding view (resource binder iterates and automatically binds)
    HdStBufferResourceSharedPtr view =
        _AddResource(name, tupleType, offset, stride);

    // this is just a view, not consuming memory
    view->SetAllocation(_entireResource->GetId(), /*size=*/0);
}


bool
HdStDispatchBuffer::GarbageCollect()
{
    TF_CODING_ERROR("HdStDispatchBuffer doesn't support this operation");
    return false;
}

void
HdStDispatchBuffer::Reallocate(
    std::vector<HdBufferArrayRangeSharedPtr> const &,
    HdBufferArraySharedPtr const &)
{
    TF_CODING_ERROR("HdStDispatchBuffer doesn't support this operation");
}

void
HdStDispatchBuffer::DebugDump(std::ostream &out) const
{
    /*nothing*/
}

HdStBufferResourceSharedPtr
HdStDispatchBuffer::GetResource() const
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
HdStDispatchBuffer::GetResource(TfToken const& name)
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

HdStBufferResourceSharedPtr
HdStDispatchBuffer::_AddResource(TfToken const& name,
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
        new HdStBufferResource(GetRole(), tupleType,
                                 offset, stride));

    _resourceList.emplace_back(name, bufferRes);
    return bufferRes;
}


PXR_NAMESPACE_CLOSE_SCOPE

