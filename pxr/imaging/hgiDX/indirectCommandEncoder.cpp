
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

#include "pch.h"
#include "pxr/imaging/hgiDX/indirectCommandEncoder.h"
#include "pxr/imaging/hgiDX/graphicsCmds.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiBufferHandle HgiDXIndirectCommandEncoder::_dummyNullBuffer;
std::vector<uint32_t> HgiDXIndirectCommandEncoder::_dummyEmptyVector;

HgiDXIndirectCommandEncoder::HgiDXIndirectCommandEncoder(HgiDX* hgi)
   :_hgi(hgi)
{

}

HgiDXIndirectCommandEncoder::~HgiDXIndirectCommandEncoder()
{

}

/// Encodes a batch of draw commands from the drawParameterBuffer.
/// Returns a HgiIndirectCommands which holds the necessary buffers and
/// state for replaying the batch.
HgiIndirectCommandsUniquePtr 
HgiDXIndirectCommandEncoder::EncodeDraw(HgiComputeCmds* computeCmds,
                                        HgiGraphicsPipelineHandle const& pipeline,
                                        HgiResourceBindingsHandle const& resourceBindings,
                                        HgiVertexBufferBindingVector const& vertexBindings,
                                        HgiBufferHandle const& drawParameterBuffer,
                                        uint32_t drawBufferByteOffset,
                                        uint32_t drawCount,
                                        uint32_t stride)
{
   HgiIndirectCommandsUniquePtr ret = 
      std::make_unique<HgiDXIndirectCommands>(drawCount, 
                                              pipeline, 
                                              resourceBindings, 
                                              vertexBindings, 
                                              drawParameterBuffer, 
                                              _dummyNullBuffer, 
                                              false,
                                              drawBufferByteOffset,
                                              stride,
                                              0);
   
   return ret;
}

/// Encodes a batch of indexed draw commands from the drawParameterBuffer.
/// Returns a HgiIndirectCommands which holds the necessary buffers and
/// state for replaying the batch.
HgiIndirectCommandsUniquePtr 
HgiDXIndirectCommandEncoder::EncodeDrawIndexed(HgiComputeCmds* computeCmds,
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
   HgiIndirectCommandsUniquePtr ret = 
      std::make_unique<HgiDXIndirectCommands>(drawCount, 
                                              pipeline, 
                                              resourceBindings, 
                                              vertexBindings, 
                                              drawParameterBuffer, 
                                              indexBuffer,
                                              true,
                                              drawBufferByteOffset,
                                              stride,
                                              patchBaseVertexByteOffset);

   return ret;
}

/// Executes an indirect command batch from the HgiIndirectCommands
/// structure.
void 
HgiDXIndirectCommandEncoder::ExecuteDraw(HgiGraphicsCmds* gfxCmds,
                                         HgiIndirectCommands const* commands)
{

   const HgiDXIndirectCommands* pDXIndirectCmds = dynamic_cast<const HgiDXIndirectCommands*>(commands);
   
   if (nullptr != pDXIndirectCmds)
   {
      gfxCmds->BindPipeline(pDXIndirectCmds->graphicsPipeline);
      gfxCmds->BindResources(pDXIndirectCmds->resourceBindings);
      gfxCmds->BindVertexBuffers(pDXIndirectCmds->_vertexBindings);

      //
      // render targets and viewport? hopefully they set it before calling here
      if (pDXIndirectCmds->_bIndexed)
         gfxCmds->DrawIndexedIndirect(pDXIndirectCmds->_indexBuffer,
                                      pDXIndirectCmds->_drawParameterBuffer,
                                      pDXIndirectCmds->_drawBufferByteOffset,
                                      pDXIndirectCmds->drawCount,
                                      pDXIndirectCmds->_stride,
                                      _dummyEmptyVector,
                                      pDXIndirectCmds->_patchBaseVertexByteOffset);
      else
         gfxCmds->DrawIndirect(pDXIndirectCmds->_drawParameterBuffer, 
                               pDXIndirectCmds->_drawBufferByteOffset, 
                               pDXIndirectCmds->drawCount, 
                               pDXIndirectCmds->_stride);
   }
   else
      TF_WARN("Invalid indirect commands information. Cannto execute.");
}


HgiDXIndirectCommands::HgiDXIndirectCommands(uint32_t drawCount,
                                             HgiGraphicsPipelineHandle const& graphicsPipeline,
                                             HgiResourceBindingsHandle const& resourceBindings,
                                             HgiVertexBufferBindingVector const& vertexBindings,
                                             HgiBufferHandle const& drawParameterBuffer,
                                             HgiBufferHandle const& indexBuffer,
                                             bool bIndexed,
                                             uint32_t drawBufferByteOffset,
                                             uint32_t stride,
                                             uint32_t patchBaseVertexByteOffset)
   :HgiIndirectCommands(drawCount, graphicsPipeline, resourceBindings)
   , _vertexBindings(vertexBindings)
   , _drawParameterBuffer(drawParameterBuffer)
   , _indexBuffer(indexBuffer)
   , _bIndexed(bIndexed)
   , _drawBufferByteOffset(drawBufferByteOffset)
   , _stride(stride)
   , _patchBaseVertexByteOffset(patchBaseVertexByteOffset)
{
}


PXR_NAMESPACE_CLOSE_SCOPE


