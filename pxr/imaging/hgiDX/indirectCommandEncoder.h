
//
// Copyright 2023 Pixar
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

#pragma once

#include "pxr/pxr.h"
#include "pxr/imaging/hgiDX/api.h"
#include "pxr/imaging/hgi/indirectCommandEncoder.h"

PXR_NAMESPACE_OPEN_SCOPE


struct HgiDXIndirectCommands : public HgiIndirectCommands
{
   HgiDXIndirectCommands(uint32_t drawCount,
                         HgiGraphicsPipelineHandle const& graphicsPipeline,
                         HgiResourceBindingsHandle const& resourceBindings,
                         HgiVertexBufferBindingVector const& vertexBindings,
                         HgiBufferHandle const& drawParameterBuffer,
                         HgiBufferHandle const& indexBuffer,
                         bool bIndexed,
                         uint32_t drawBufferByteOffset,
                         uint32_t stride,
                         uint32_t patchBaseVertexByteOffset);
   virtual ~HgiDXIndirectCommands() = default;

   HgiVertexBufferBindingVector const& _vertexBindings;
   HgiBufferHandle const& _drawParameterBuffer;
   HgiBufferHandle const& _indexBuffer;
   bool _bIndexed;
   uint32_t _drawBufferByteOffset;
   uint32_t _stride;
   uint32_t _patchBaseVertexByteOffset;
};


/// \class HgiDXIndirectCommandEncoder
///
/// DirectX implementation of HgiGraphicsEncoder.
///
class HgiDXIndirectCommandEncoder final : public HgiIndirectCommandEncoder
{
public:
   HGIDX_API
   virtual ~HgiDXIndirectCommandEncoder() override;

   /// Encodes a batch of draw commands from the drawParameterBuffer.
   /// Returns a HgiIndirectCommands which holds the necessary buffers and
   /// state for replaying the batch.
   HGIDX_API
   virtual HgiIndirectCommandsUniquePtr EncodeDraw(
                    HgiComputeCmds * computeCmds,
                    HgiGraphicsPipelineHandle const& pipeline,
                    HgiResourceBindingsHandle const& resourceBindings,
                    HgiVertexBufferBindingVector const& vertexBindings,
                    HgiBufferHandle const& drawParameterBuffer,
                    uint32_t drawBufferByteOffset,
                    uint32_t drawCount,
                    uint32_t stride) override;
    
    /// Encodes a batch of indexed draw commands from the drawParameterBuffer.
    /// Returns a HgiIndirectCommands which holds the necessary buffers and
    /// state for replaying the batch.
    HGIDX_API
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
                    uint32_t patchBaseVertexByteOffset) override;
    
    /// Excutes an indirect command batch from the HgiIndirectCommands
    /// structure.
    HGIDX_API
    virtual void ExecuteDraw(HgiGraphicsCmds * gfxCmds,
                             HgiIndirectCommands const* commands) override;

protected:
   friend class HgiDX;
    
   HGIDX_API
   HgiDXIndirectCommandEncoder(HgiDX* hgi);

private:
   HgiDXIndirectCommandEncoder() = delete;
   HgiDXIndirectCommandEncoder& operator=(const HgiDXIndirectCommandEncoder&) = delete;
   HgiDXIndirectCommandEncoder(const HgiDXIndirectCommandEncoder&) = delete;

private:
   HgiDX* _hgi;
   static HgiBufferHandle _dummyNullBuffer;
   static std::vector<uint32_t> _dummyEmptyVector;
};

PXR_NAMESPACE_CLOSE_SCOPE