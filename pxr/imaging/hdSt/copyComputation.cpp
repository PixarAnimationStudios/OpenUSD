//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/copyComputation.h"
#include "pxr/imaging/hdSt/bufferArrayRange.h"
#include "pxr/imaging/hdSt/bufferResource.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/imaging/hgi/blitCmds.h"
#include "pxr/imaging/hgi/blitCmdsOps.h"

#include "pxr/imaging/hf/perfLog.h"

PXR_NAMESPACE_OPEN_SCOPE


HdStCopyComputationGPU::HdStCopyComputationGPU(
    HdBufferArrayRangeSharedPtr const &src, TfToken const &name)
    : _src(src), _name(name)
{
}

void
HdStCopyComputationGPU::Execute(HdBufferArrayRangeSharedPtr const &range_,
                                HdResourceRegistry *resourceRegistry)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdStBufferArrayRangeSharedPtr srcRange =
        std::static_pointer_cast<HdStBufferArrayRange> (_src);
    HdStBufferArrayRangeSharedPtr dstRange =
        std::static_pointer_cast<HdStBufferArrayRange> (range_);

    HdStBufferResourceSharedPtr srcRes = srcRange->GetResource(_name);
    HdStBufferResourceSharedPtr dstRes = dstRange->GetResource(_name);

    if (!TF_VERIFY(srcRes)) {
        return;
    }
    if (!TF_VERIFY(dstRes)) {
        return;
    }

    int srcResSize = HdDataSizeOfTupleType(srcRes->GetTupleType()) *
                     srcRange->GetNumElements();
    int dstResSize = HdDataSizeOfTupleType(dstRes->GetTupleType()) *
                     dstRange->GetNumElements();

    if (!TF_VERIFY(srcResSize <= dstResSize)) {
        // The number of elements in the BAR *can* differ during migration.
        // One example is during mesh refinement when migration is necessary,
        // and we copy only the unrefined data over.
        TF_CODING_ERROR("Migration error for %s: Source resource (%d) size is "
            "larger than destination resource size (%d)\n",
            _name.GetText(), srcResSize, dstResSize);
        return;
    }

    size_t readOffset = srcRange->GetByteOffset(_name) + srcRes->GetOffset();
    size_t writeOffset = dstRange->GetByteOffset(_name) + dstRes->GetOffset();
    size_t copySize = srcResSize;

    // Unfortunately at the time the copy computation is added, we don't
    // know if the source buffer has 0 length.  So we can get here with
    // a zero sized copy.
    if (srcResSize > 0) {

        // If the buffer's have 0 size, resources for them would not have
        // be allocated, so the check for resource allocation has been moved
        // until after the copy size check.

        if (!TF_VERIFY(srcRes->GetHandle())) {
            return;
        }
        if (!TF_VERIFY(dstRes->GetHandle())) {
            return;
        }

        HD_PERF_COUNTER_INCR(HdStPerfTokens->copyBufferGpuToGpu);

        HdStResourceRegistry* hdStResourceRegistry =
            static_cast<HdStResourceRegistry*>(resourceRegistry);

        HgiBufferGpuToGpuOp blitOp;
        blitOp.gpuSourceBuffer = srcRes->GetHandle();
        blitOp.gpuDestinationBuffer = dstRes->GetHandle();
        blitOp.sourceByteOffset = readOffset;
        blitOp.byteSize = copySize;
        blitOp.destinationByteOffset = writeOffset;

        HgiBlitCmds* blitCmds = hdStResourceRegistry->GetGlobalBlitCmds();
        blitCmds->CopyBufferGpuToGpu(blitOp);
    }
}

int
HdStCopyComputationGPU::GetNumOutputElements() const
{
    return _src->GetNumElements();
}

void
HdStCopyComputationGPU::GetBufferSpecs(HdBufferSpecVector *specs) const
{
    HdStBufferArrayRangeSharedPtr srcRange =
        std::static_pointer_cast<HdStBufferArrayRange> (_src);

    specs->emplace_back(_name, srcRange->GetResource(_name)->GetTupleType());
}

PXR_NAMESPACE_CLOSE_SCOPE

