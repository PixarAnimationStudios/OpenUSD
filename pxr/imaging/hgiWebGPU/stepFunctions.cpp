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

void
HgiWebGPUStepFunctions::SetVertexBufferOffsets(
    wgpu::RenderPassEncoder const &encoder,
    uint32_t baseInstance)
{
    for (auto const & stepFunction : _vertexBufferDescs) {
        uint32_t const offset = stepFunction.vertexStride * baseInstance +
                                stepFunction.byteOffset;

        encoder.SetVertexBuffer(stepFunction.bindingIndex, stepFunction.buffer, offset, WGPU_WHOLE_SIZE);
     }
}

PXR_NAMESPACE_CLOSE_SCOPE
