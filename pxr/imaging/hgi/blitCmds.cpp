//
// Copyright 2019 Pixar
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
#include "pxr/imaging/hgi/blitCmds.h"
#include "pxr/imaging/hgi/blitCmdsOps.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiBlitCmds::HgiBlitCmds() = default;

HgiBlitCmds::~HgiBlitCmds() = default;

void
HgiBlitCmds::QueueCopyBufferCpuToGpu(HgiBufferCpuToGpuOp const& copyOp)
{
    if (copyOp.byteSize == 0 ||
        !copyOp.cpuSourceBuffer ||
        !copyOp.gpuDestinationBuffer)
    {
        return;
    }

    // Place the data into the staging buffer
    uint8_t * const cpuStaging = static_cast<uint8_t*>(
        copyOp.gpuDestinationBuffer->GetCPUStagingAddress());
    uint8_t const* const srcData =
        static_cast<uint8_t const*>(copyOp.cpuSourceBuffer) +
        copyOp.sourceByteOffset;
    memcpy(cpuStaging + copyOp.destinationByteOffset, srcData, copyOp.byteSize);

    auto const &it = queuedBuffers.find(copyOp.gpuDestinationBuffer.Get());
    if (it != queuedBuffers.end()) {
        BufferFlushListEntry &bufferEntry = it->second;
        if (copyOp.destinationByteOffset == bufferEntry.end) {
            // Accumulate the copy
            bufferEntry.end += copyOp.byteSize;
        } else {
            // This buffer copy doesn't contiguously extend the queued copy
            // Submit the accumulated work to date
            HgiBufferCpuToGpuOp op;
            op.cpuSourceBuffer = cpuStaging;
            op.sourceByteOffset = bufferEntry.start;
            op.gpuDestinationBuffer = copyOp.gpuDestinationBuffer;
            op.destinationByteOffset = bufferEntry.start;
            op.byteSize = bufferEntry.end - bufferEntry.start;
            CopyBufferCpuToGpu(op);

            // Update this entry for our new pending copy
            bufferEntry.start = copyOp.destinationByteOffset;
            bufferEntry.end = copyOp.destinationByteOffset + copyOp.byteSize;
        }
    } else {
        uint64_t const start = copyOp.destinationByteOffset;
        uint64_t const end = copyOp.destinationByteOffset + copyOp.byteSize;
        queuedBuffers.emplace(copyOp.gpuDestinationBuffer.Get(),
            BufferFlushListEntry(copyOp.gpuDestinationBuffer, start, end));
    }
}

void
HgiBlitCmds::FlushQueuedCopies()
{
    HgiBufferCpuToGpuOp op;
    for(auto &copy: queuedBuffers) {
        BufferFlushListEntry const &entry = copy.second;
        op.cpuSourceBuffer = entry.buffer->GetCPUStagingAddress();
        op.sourceByteOffset = entry.start;
        op.gpuDestinationBuffer = entry.buffer;
        op.destinationByteOffset = entry.start;
        op.byteSize = entry.end - entry.start;
        CopyBufferCpuToGpu(op);
    }
    queuedBuffers.clear();
}

PXR_NAMESPACE_CLOSE_SCOPE
