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

    /// Set the capacity for the staging buffer.  Only applied once first call
    /// to StageCopy is called.
    /// Cannot be called if there have already been calls to StageCopy for this
    /// commit.
    HDST_API
    void Resize(size_t totalSize);

    /// Submit a CPU to GPU copy operation to be added to the staging buffer.
    /// The contents are memcpy'd over into the staging buffer during this call
    /// and a GPU to GPU blit is queued up to do the final copy to destination.
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
    size_t _head;
    size_t _capacity;
    size_t _activeSlot;
    bool _tripleBuffered;
    std::vector<HgiBufferGpuToGpuOp> _gpuCopyOps;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_STAGING_BUFFER_H
