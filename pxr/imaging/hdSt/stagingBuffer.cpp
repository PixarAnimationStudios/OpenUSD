//
// Copyright 2021 Pixar
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
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/stagingBuffer.h"

#include "pxr/imaging/hgi/blitCmds.h"
#include "pxr/imaging/hgi/capabilities.h"
#include "pxr/imaging/hgi/hgi.h"

PXR_NAMESPACE_OPEN_SCOPE

HdStStagingBuffer::HdStStagingBuffer(HdStResourceRegistry *resourceRegistry)
    : _resourceRegistry(resourceRegistry)
    , _head(0)
    , _capacity(0)
    , _activeSlot(0)
{
    _tripleBuffered = resourceRegistry->GetHgi()->GetCapabilities()->
                          IsSet(HgiDeviceCapabilitiesBitsUnifiedMemory);
}

HdStStagingBuffer::~HdStStagingBuffer()
{
    Deallocate();
}

void
HdStStagingBuffer::Deallocate()
{
    Hgi* hgi = _resourceRegistry->GetHgi();

    for (size_t i = 0; i < MULTIBUFFERING; ++i) {
        if (_handles[i]) {
            hgi->DestroyBuffer(&_handles[i]);
        }
    }

    _capacity = 0;
    _activeSlot = 0;
}

void
HdStStagingBuffer::Resize(size_t capacity)
{
    // Only change the capacity if there aren't any queued copy operations.
    if (_head != 0) {
        TF_CODING_ERROR("Cannot change size of staging buffer during Commit");
        return;
    }

    _capacity = capacity;
}

void
HdStStagingBuffer::StageCopy(HgiBufferCpuToGpuOp const &copyOp)
{
    if (copyOp.byteSize == 0 ||
        !copyOp.cpuSourceBuffer ||
        !copyOp.gpuDestinationBuffer)
    {
        return;
    }

    // When the to-be-copied data is 'large' doing the extra memcpy into the
    // stating buffer to avoid many small GPU buffer upload can be more
    // expensive than just submitting the CPU to GPU copy operation directly.
    // The value of 'queueThreshold' is estimated (when is the extra memcpy
    // into the staging buffer slower than immediately issuing a gpu upload)
    static const int queueThreshold = 512*1024;
    if (!_tripleBuffered && copyOp.byteSize > queueThreshold) {
        HgiBlitCmds* blitCmds = _resourceRegistry->GetGlobalBlitCmds();
        blitCmds->CopyBufferCpuToGpu(copyOp);
        return;
    }

    HgiBufferHandle buffer = _handles[_activeSlot];
    constexpr size_t recoveryRatio = 4;

    // If there is no buffer in the active slot or it is either too small or
    // substantially larger than the required size, recreate it.
    if (!buffer ||
        buffer->GetDescriptor().byteSize < _capacity ||
        buffer->GetDescriptor().byteSize > _capacity * recoveryRatio) {
        HgiBufferDesc bufferDesc;
        bufferDesc.byteSize = _capacity;

        Hgi* hgi = _resourceRegistry->GetHgi();

        if (buffer) {
            hgi->DestroyBuffer(&buffer);
        }

        _handles[_activeSlot] = hgi->CreateBuffer(bufferDesc);
        buffer = _handles[_activeSlot];
    }

    size_t capacity = buffer->GetDescriptor().byteSize;
    uint8_t *cpuStaging = static_cast<uint8_t*>(buffer->GetCPUStagingAddress());

    if (TF_VERIFY(_head + copyOp.byteSize <= capacity)) {
        // Copy source into the staging buffer.
        char const *sourceData =
            static_cast<char const *>(copyOp.cpuSourceBuffer);
        memcpy(cpuStaging + _head,
               sourceData + copyOp.sourceByteOffset,
               copyOp.byteSize);

        bool aggregated = false;

        // If this copy is contiguous with last staged one then aggregate them.
        if (!_gpuCopyOps.empty()) {
            HgiBufferGpuToGpuOp &lastCopy = _gpuCopyOps.back();
            size_t lastCopyEnd = lastCopy.destinationByteOffset
                               + lastCopy.byteSize;

            if (lastCopy.gpuDestinationBuffer == copyOp.gpuDestinationBuffer &&
                lastCopyEnd == copyOp.destinationByteOffset) {
                lastCopy.byteSize += copyOp.byteSize;
                aggregated = true;
            }
        }

        if (!aggregated) {
            // Create a GPU to GPU blit operation to do the final copy.
            HgiBufferGpuToGpuOp gpuCopy;

            gpuCopy.gpuSourceBuffer = buffer;
            gpuCopy.sourceByteOffset = _head;
            gpuCopy.byteSize = copyOp.byteSize;
            gpuCopy.gpuDestinationBuffer = copyOp.gpuDestinationBuffer;
            gpuCopy.destinationByteOffset = copyOp.destinationByteOffset;

            _gpuCopyOps.push_back(gpuCopy);
        }

        _head += copyOp.byteSize;
    }
}

void
HdStStagingBuffer::Flush()
{
    if (_head == 0) {
        _gpuCopyOps.clear();
        return;
    }

    HgiBlitCmds* blitCmds = _resourceRegistry->GetGlobalBlitCmds();

    blitCmds->PushDebugGroup(__ARCH_PRETTY_FUNCTION__);

    if (!_tripleBuffered) {
        // If this isn't UMA then blit the staging buffer to GPU.
        HgiBufferCpuToGpuOp op;
        HgiBufferHandle buffer = _handles[_activeSlot];
        uint8_t* const cpuStaging = static_cast<uint8_t* const>(
            buffer->GetCPUStagingAddress());

        op.cpuSourceBuffer = cpuStaging;
        op.sourceByteOffset = 0;
        op.gpuDestinationBuffer = buffer;
        op.destinationByteOffset = 0;
        op.byteSize = _head;
        blitCmds->CopyBufferCpuToGpu(op);
        blitCmds->InsertMemoryBarrier(HgiMemoryBarrierAll);
    }

    for (auto const &copyOp : _gpuCopyOps) {
        blitCmds->CopyBufferGpuToGpu(copyOp);
    }

    blitCmds->PopDebugGroup();

    _gpuCopyOps.clear();
    _head = 0;

    if (_tripleBuffered) {
        _activeSlot++;
        _activeSlot = (_activeSlot < MULTIBUFFERING) ? _activeSlot : 0;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
