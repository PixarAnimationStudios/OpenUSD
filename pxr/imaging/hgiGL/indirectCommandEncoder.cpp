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

#include "pxr/imaging/hgiGL/indirectCommandEncoder.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiGLIndirectCommandEncoder::HgiGLIndirectCommandEncoder() = default;

HgiIndirectCommandsUniquePtr
HgiGLIndirectCommandEncoder::EncodeDraw(
    HgiComputeCmds * computeCmds,
    HgiGraphicsPipelineHandle const& pipeline,
    HgiResourceBindingsHandle const& resourceBindings,
    HgiVertexBufferBindingVector const& vertexBindings,
    HgiBufferHandle const& drawParameterBuffer,
    uint32_t drawBufferByteOffset,
    uint32_t drawCount,
    uint32_t stride)
{
    // No implementation
    return nullptr;
}

HgiIndirectCommandsUniquePtr
HgiGLIndirectCommandEncoder::EncodeDrawIndexed(
    HgiComputeCmds * computeCmds,
    HgiGraphicsPipelineHandle const& pipeline,
    HgiResourceBindingsHandle const& resourceBindings,
    HgiVertexBufferBindingVector const& vertexBindings,
    HgiBufferHandle const& indexBuffer,
    HgiBufferHandle const& drawParameterBuffer,
    uint32_t drawBufferByteOffset,
    uint32_t drawCount,
    uint32_t stride,
    uint32_t patchBaseVertexByteOffset)
{
    // No implementation
    return nullptr;
}

void
HgiGLIndirectCommandEncoder::ExecuteDraw(
    HgiGraphicsCmds * gfxCmds,
    HgiIndirectCommands const* commands)
{
    // No implementation
}
PXR_NAMESPACE_CLOSE_SCOPE
