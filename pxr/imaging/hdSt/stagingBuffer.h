//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_STAGING_BUFFER_H
#define PXR_IMAGING_HD_ST_STAGING_BUFFER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hgi/blitCmdsOps.h"
#include "pxr/imaging/hgi/buffer.h"

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HdStResourceRegistry;

using HdStStagingBufferSharedPtr =
    std::shared_ptr<class HdStStagingBuffer>;

/// \class HdStStagingBuffer
///
/// Provides a staging buffer for CPU writes of triple-buffered resources.
/// None of the function calls are thread safe, they should be only accessed
/// from the commit call to ResourceRegistry.
///
class HdStStagingBuffer
{
public:
    HDST_API
    HdStStagingBuffer(HdStResourceRegistry *resourceRegistry);

    HDST_API
    ~HdStStagingBuffer();

    /// Destroys contained HgiBuffers and resets state to empty.
    HDST_API
    void Deallocate();

    /// Set the estimated upload size. OpenGL caps it to the staging limit.
    /// Allocation is deferred until StageCopy. Cannot be called with queued
    /// copies. On OpenGL, HDST_MAX_STAGING_BUFFER_SIZE accepts positive
    /// decimal byte limits representable by size_t. The default and fallback
    /// for invalid limits is 64 MiB. Other backends ignore this setting.
    HDST_API
    void Resize(size_t totalSize);

    /// Queue a CPU to GPU copy. On OpenGL, full staging batches are flushed
    /// and completed before their memory is reused, and oversized direct
    /// uploads are split at the same limit. Other backends retain their
    /// existing staging and submission behavior.
    HDST_API
    void StageCopy(HgiBufferCpuToGpuOp const &copyOp);

    /// Flush the queued GPU to GPU blits from the calls to StageCopy.  Resets
    /// the state for the next ResoureRegistry commit.
    HDST_API
    void Flush();

private:
    static constexpr int32_t MULTIBUFFERING = 3;

    HdStResourceRegistry *_resourceRegistry;
    HgiBufferHandle _handles[MULTIBUFFERING];
    std::vector<HgiBufferGpuToGpuOp> _gpuCopyOps;
    size_t _head;
    size_t _capacity;
    size_t _activeSlot;
    bool _isUma;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_STAGING_BUFFER_H
