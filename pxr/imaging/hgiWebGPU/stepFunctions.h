//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

    template<typename PassEncoder>
    HGIWEBGPU_API
    void SetVertexBufferOffsets(
        PassEncoder const &encoder,
        uint32_t baseInstance) {
        static_assert(std::is_same_v<PassEncoder, wgpu::RenderPassEncoder> ||
                      std::is_same_v<PassEncoder, wgpu::RenderBundleEncoder>,
                      "encoder parameter must be wgpu::RenderPassEncoder,"
                      "or wgpu::RenderBundleEncoder");
        for (auto const & stepFunction : _vertexBufferDescs) {
            uint32_t const offset = stepFunction.vertexStride * baseInstance +
                                    stepFunction.byteOffset;

            encoder.SetVertexBuffer(stepFunction.bindingIndex, stepFunction.buffer, offset, WGPU_WHOLE_SIZE);
        }
    }

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
