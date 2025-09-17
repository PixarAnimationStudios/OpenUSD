//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hgiMetal/stepFunctions.h"

#include "pxr/imaging/hgi/graphicsPipeline.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiMetalStepFunctions::HgiMetalStepFunctions()
    : _drawBufferIndex(0)
{
    static const size_t _maxStepFunctionDescs = 4;
    _vertexBufferDescs.reserve(_maxStepFunctionDescs);
    _patchBaseDescs.reserve(_maxStepFunctionDescs);
}

HgiMetalStepFunctions::HgiMetalStepFunctions(
    HgiGraphicsPipelineDesc const &graphicsDesc,
    HgiVertexBufferBindingVector const &bindings)
{
    static const size_t _maxStepFunctionDescs = 4;
    _vertexBufferDescs.reserve(_maxStepFunctionDescs);
    _patchBaseDescs.reserve(_maxStepFunctionDescs);
    
    Init(graphicsDesc);
    Bind(bindings);
}

void
HgiMetalStepFunctions::Init(HgiGraphicsPipelineDesc const &graphicsDesc)
{
    _vertexBufferDescs.clear();
    _patchBaseDescs.clear();

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
HgiMetalStepFunctions::Bind(HgiVertexBufferBindingVector const &bindings)
{
    for (HgiVertexBufferBinding const &binding : bindings) {
        if (binding.buffer) {
            HgiBufferDesc const& desc = binding.buffer->GetDescriptor();

            TF_VERIFY(desc.usage & HgiBufferUsageVertex);

            for (auto & stepFunction : _vertexBufferDescs) {
                if (stepFunction.bindingIndex == binding.index) {
                    stepFunction.byteOffset = binding.byteOffset;
                }
            }
            for (auto & stepFunction : _patchBaseDescs) {
                if (stepFunction.bindingIndex == binding.index) {
                    stepFunction.byteOffset = binding.byteOffset;
                }
            }
        }
    }
}

void
HgiMetalStepFunctions::SetVertexBufferOffsets(
    id<MTLRenderCommandEncoder> encoder,
    uint32_t baseInstance)
{
    for (auto const & stepFunction : _vertexBufferDescs) {
        uint32_t const offset = stepFunction.vertexStride * baseInstance +
                                stepFunction.byteOffset;

        [encoder setVertexBufferOffset:offset
                               atIndex:stepFunction.bindingIndex];
     }
}

void
HgiMetalStepFunctions::SetPatchBaseOffsets(
    id<MTLRenderCommandEncoder> encoder,
    uint32_t baseVertex)
{
    for (auto const & stepFunction : _patchBaseDescs) {
        uint32_t const offset = stepFunction.vertexStride * baseVertex +
                                stepFunction.byteOffset;

        [encoder setVertexBufferOffset:offset
                               atIndex:stepFunction.bindingIndex];
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
