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
#ifndef PXR_IMAGING_HGI_INDIRECT_COMMAND_ENCODER_H
#define PXR_IMAGING_HGI_INDIRECT_COMMAND_ENCODER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/cmds.h"
#include "pxr/imaging/hgi/resourceBindings.h"
#include "pxr/imaging/hgi/graphicsPipeline.h"

#include <memory>
#include <stdint.h>

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;
class HgiComputeCmds;
class HgiGraphicsCmds;

struct HgiIndirectCommands
{
    HgiIndirectCommands(uint32_t drawCount,
                        HgiGraphicsPipelineHandle const &graphicsPipeline,
                        HgiResourceBindingsHandle const &resourceBindings)
        : drawCount(drawCount)
        , graphicsPipeline(graphicsPipeline)
        , resourceBindings(resourceBindings)
    {
    }

    virtual ~HgiIndirectCommands() = default;

    uint32_t drawCount;
    HgiGraphicsPipelineHandle graphicsPipeline;
    HgiResourceBindingsHandle resourceBindings;
};

using HgiIndirectCommandsUniquePtr = std::unique_ptr<HgiIndirectCommands>;

/// \class HgiIndirectCommandEncoder
///
/// XXX: Explain what this is....
///
class HgiIndirectCommandEncoder : public HgiCmds
{
public:
    HGI_API
    virtual ~HgiIndirectCommandEncoder();
    
    HGI_API
    virtual HgiIndirectCommandsUniquePtr EncodeDraw(
        HgiComputeCmds * computeCmds,
        HgiGraphicsPipelineHandle const& pipeline,
        HgiResourceBindingsHandle const& resourceBindings,
        HgiVertexBufferBindingVector const& vertexBindings,
        HgiBufferHandle const& drawParameterBuffer,
        uint32_t drawBufferByteOffset,
        uint32_t drawCount,
        uint32_t stride) = 0;
    
    HGI_API
    virtual HgiIndirectCommandsUniquePtr EncodeDrawIndexed(
        HgiComputeCmds * computeCmds,
        HgiGraphicsPipelineHandle const& pipeline,
        HgiResourceBindingsHandle const& resourceBindings,
        HgiVertexBufferBindingVector const& vertexBindings,
        HgiBufferHandle const& indexBuffer,
        HgiBufferHandle const& drawParameterBuffer,
        uint32_t drawBufferByteOffset,
        uint32_t drawCount,
        uint32_t stride,
        uint32_t patchBaseVertexByteOffset) = 0;

    HGI_API
    virtual void ExecuteDraw(
        HgiGraphicsCmds * gfxCmds,
        HgiIndirectCommands const* commands) = 0;

protected:
    HGI_API
    HgiIndirectCommandEncoder() = default;

private:
    HgiIndirectCommandEncoder & operator=(const HgiIndirectCommandEncoder&) = delete;
    HgiIndirectCommandEncoder(const HgiIndirectCommandEncoder&) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
