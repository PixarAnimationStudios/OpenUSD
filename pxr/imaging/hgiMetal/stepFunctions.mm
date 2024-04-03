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
