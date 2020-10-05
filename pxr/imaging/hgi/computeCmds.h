//
// Copyright 2020 Pixar
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
#ifndef PXR_IMAGING_HGI_COMPUTE_CMDS_H
#define PXR_IMAGING_HGI_COMPUTE_CMDS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/computePipeline.h"
#include "pxr/imaging/hgi/resourceBindings.h"
#include "pxr/imaging/hgi/cmds.h"
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

using HgiComputeCmdsUniquePtr = std::unique_ptr<class HgiComputeCmds>;


/// \class HgiComputeCmds
///
/// A graphics API independent abstraction of compute commands.
/// HgiComputeCmds is a lightweight object that cannot be re-used after it has
/// been submitted. A new cmds object should be acquired for each frame.
///
class HgiComputeCmds : public HgiCmds
{
public:
    HGI_API
    ~HgiComputeCmds() override;

    /// Push a debug marker.
    HGI_API
    virtual void PushDebugGroup(const char* label) = 0;

    /// Pop the last debug marker.
    HGI_API
    virtual void PopDebugGroup() = 0;

    /// Bind a pipeline state object. Usually you call this right after calling
    /// CreateGraphicsCmds to set the graphics pipeline state.
    /// The resource bindings used when creating the pipeline must be compatible
    /// with the resources bound via BindResources().
    HGI_API
    virtual void BindPipeline(HgiComputePipelineHandle pipeline) = 0;

    /// Bind resources such as textures and uniform buffers.
    /// Usually you call this right after BindPipeline() and the resources bound
    /// must be compatible with the bound pipeline.
    HGI_API
    virtual void BindResources(HgiResourceBindingsHandle resources) = 0;

    /// Set Push / Function constants.
    /// `pipeline` is the compute pipeline that you are binding before the
    /// draw call. It contains the program used for the uniform buffer
    /// constant values for.
    /// `bindIndex` is the binding point index in the pipeline's shader
    /// to bind the data to.
    /// `byteSize` is the size of the data you are updating.
    /// `data` is the data you are copying into the push constants block.
    HGI_API
    virtual void SetConstantValues(
        HgiComputePipelineHandle pipeline,
        uint32_t bindIndex,
        uint32_t byteSize,
        const void* data) = 0;

    /// Execute a compute shader with provided thread group count in each
    /// dimension.
    HGI_API
    virtual void Dispatch(int dimX, int dimY) = 0;

    /// Inserts a barrier so that data written to memory by commands before
    /// the barrier is available to commands after the barrier.
    HGI_API
    virtual void MemoryBarrier(HgiMemoryBarrier barrier) = 0;

protected:
    HGI_API
    HgiComputeCmds();

private:
    HgiComputeCmds & operator=(const HgiComputeCmds&) = delete;
    HgiComputeCmds(const HgiComputeCmds&) = delete;
};



PXR_NAMESPACE_CLOSE_SCOPE

#endif
