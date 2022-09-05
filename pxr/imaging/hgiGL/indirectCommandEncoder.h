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
#ifndef PXR_IMAGING_HGI_GL_INDIRECT_COMMAND_ENCODER_H
#define PXR_IMAGING_HGI_GL_INDIRECT_COMMAND_ENCODER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiGL/api.h"
#include "pxr/imaging/hgi/indirectCommandEncoder.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HgiGLIndirectGraphicsCmds
///
/// Empty OpenGL implementation of Indirect Command Buffers.
///
class HgiGLIndirectCommandEncoder final : public HgiIndirectCommandEncoder
{
public:
    HGIGL_API
    HgiGLIndirectCommandEncoder();
    
    HGIGL_API
    HgiIndirectCommandsUniquePtr EncodeDraw(
        HgiComputeCmds * computeCmds,
        HgiGraphicsPipelineHandle const& pipeline,
        HgiResourceBindingsHandle const& resourceBindings,
        HgiVertexBufferBindingVector const& vertexBindings,
        HgiBufferHandle const& drawParameterBuffer,
        uint32_t drawBufferByteOffset,
        uint32_t drawCount,
        uint32_t stride) override;
    
    HGIGL_API
    HgiIndirectCommandsUniquePtr EncodeDrawIndexed(
        HgiComputeCmds * computeCmds,
        HgiGraphicsPipelineHandle const& pipeline,
        HgiResourceBindingsHandle const& resourceBindings,
        HgiVertexBufferBindingVector const& vertexBindings,
        HgiBufferHandle const& indexBuffer,
        HgiBufferHandle const& drawParameterBuffer,
        uint32_t drawBufferByteOffset,
        uint32_t drawCount,
        uint32_t stride,
        uint32_t patchBaseVertexByteOffset) override;
    
    HGIGL_API
    void ExecuteDraw(
        HgiGraphicsCmds * gfxCmds,
        HgiIndirectCommands const* commands) override;

private:
    HgiGLIndirectCommandEncoder & operator=(const HgiGLIndirectCommandEncoder&) = delete;
    HgiGLIndirectCommandEncoder(const HgiGLIndirectCommandEncoder&) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
