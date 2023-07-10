//
// Copyright 2022 Pixar
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
#ifndef PXR_IMAGING_HGI_WEBGPU_STEP_FUNCTIONS_H
#define PXR_IMAGING_HGI_WEBGPU_STEP_FUNCTIONS_H

#include "pxr/pxr.h"

#include "pxr/imaging/hgi/resourceBindings.h"
#include "pxr/imaging/hgiWebGPU/buffer.h"
#include "pxr/imaging/hgiWebGPU/api.h"

#include <cstdint>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// Some aspects of drawing command primitive input assembly work
// differently on WebGPU than other graphics APIs.
//
// 1) WebGPU does not support a vertex attrib divisor, so in order to
// have vertex attributes which advance once per draw command we use
// a constant vertex buffer step function and advance the vertex buffer
// binding offset explicitly by executing setVertexBuffer for
// the vertex buffers associated with "perDrawCommand" vertex attributes.

struct HgiGraphicsPipelineDesc;

/// \struct HgiWebGPUStepFunctionDesc
///
/// For passing in vertex buffer step function parameters.
///
struct HgiWebGPUStepFunctionDesc
{
    HgiWebGPUStepFunctionDesc(
            uint32_t bindingIndex,
            uint32_t byteOffset,
            uint32_t vertexStride)
        : bindingIndex(bindingIndex)
        , byteOffset(byteOffset)
        , vertexStride(vertexStride)
        , buffer(nullptr)
        { }
    uint32_t bindingIndex;
    uint32_t byteOffset;
    uint32_t vertexStride;
    wgpu::Buffer buffer;
};

using HgiWebGPUStepFunctionDescVector = std::vector<HgiWebGPUStepFunctionDesc>;

class HgiWebGPUStepFunctions
{
public:
    HGIWEBGPU_API
    HgiWebGPUStepFunctions();

    HGIWEBGPU_API
    HgiWebGPUStepFunctions(
        HgiGraphicsPipelineDesc const &graphicsDesc,
        HgiVertexBufferBindingVector const &bindings);

    HGIWEBGPU_API
    void Init(HgiGraphicsPipelineDesc const &graphicsDesc);

    HGIWEBGPU_API
    void Bind(HgiVertexBufferBindingVector const &bindings);

    HGIWEBGPU_API
    void SetVertexBufferOffsets(
        wgpu::RenderPassEncoder const &encoder,
        uint32_t baseInstance);

    HGIWEBGPU_API
    HgiWebGPUStepFunctionDescVector const &GetPatchBaseDescs() const
    {
        return _patchBaseDescs;
    }

    HGIWEBGPU_API
    uint32_t GetDrawBufferIndex() const
    {
        return _drawBufferIndex;
    }

private:
    HgiWebGPUStepFunctionDescVector _vertexBufferDescs;
    HgiWebGPUStepFunctionDescVector _patchBaseDescs;
    uint32_t _drawBufferIndex;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
