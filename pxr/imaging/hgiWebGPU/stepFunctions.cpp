//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hgiWebGPU/stepFunctions.h"
#include "pxr/imaging/hgiWebGPU/buffer.h"

#include "pxr/imaging/hgi/graphicsPipeline.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiWebGPUStepFunctions::HgiWebGPUStepFunctions()
    : _drawBufferIndex(0)
{
    static const size_t _maxStepFunctionDescs = 4;
    _vertexBufferDescs.reserve(_maxStepFunctionDescs);
}

HgiWebGPUStepFunctions::HgiWebGPUStepFunctions(
    HgiGraphicsPipelineDesc const &graphicsDesc,
    HgiVertexBufferBindingVector const &bindings)
{
    static const size_t _maxStepFunctionDescs = 4;
    _vertexBufferDescs.reserve(_maxStepFunctionDescs);
    
    Init(graphicsDesc);
    Bind(bindings);
}

void
HgiWebGPUStepFunctions::Init(HgiGraphicsPipelineDesc const &graphicsDesc)
{
    _vertexBufferDescs.clear();

    for (size_t index = 0; index < graphicsDesc.vertexBuffers.size(); index++) {
        auto const & vbo = graphicsDesc.vertexBuffers[index];
        if (vbo.vertexStepFunction ==
                    HgiVertexBufferStepFunctionPerDrawCommand) {
            _vertexBufferDescs.emplace_back(
                        index, 0, vbo.vertexStride);
            _drawBufferIndex = index;
        } else if (vbo.vertexStepFunction ==
                    HgiVertexBufferStepFunctionPerPatchControlPoint) {
            _patchBaseDescs.emplace_back(
                index, 0, vbo.vertexStride);
        }
    }
}
    
void
HgiWebGPUStepFunctions::Bind(HgiVertexBufferBindingVector const &bindings)
{
    for (HgiVertexBufferBinding const &binding : bindings) {
        HgiBufferDesc const& desc = binding.buffer->GetDescriptor();

        TF_VERIFY(desc.usage & HgiBufferUsageVertex);

        for (auto & stepFunction : _vertexBufferDescs) {
            if (stepFunction.bindingIndex == binding.index) {
                stepFunction.byteOffset = binding.byteOffset;
                HgiWebGPUBuffer* buf=static_cast<HgiWebGPUBuffer*>(binding.buffer.Get());
                stepFunction.buffer = buf->GetBufferHandle();
            }
        }

    }
}

PXR_NAMESPACE_CLOSE_SCOPE
