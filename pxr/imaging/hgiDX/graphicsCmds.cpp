
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
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/imaging/hgiDX/buffer.h"
#include "pxr/imaging/hgiDX/device.h"
#include "pxr/imaging/hgiDX/graphicsCmds.h"
#include "pxr/imaging/hgiDX/graphicsPipeline.h"
#include "pxr/imaging/hgiDX/resourceBindings.h"
#include "pxr/imaging/hgiDX/texture.h"
#include "pxr/imaging/hgiDX/shaderProgram.h"

PXR_NAMESPACE_OPEN_SCOPE


HgiDXGraphicsCmds::HgiDXGraphicsCmds(HgiDX* hgi, HgiGraphicsCmdsDesc const& desc)
   : _hgi(hgi)
   , _descriptor(desc)
{
   // We do not acquire the command buffer here, because the Cmds object may
   // have been created on the main thread, but used on a secondary thread.
   // We need to acquire a command buffer for the thread that is doing the
   // recording so we postpone acquiring cmd buffer until first use of Cmds.

   if (_descriptor.HasAttachments()) {
      _ops.push_back(_ClearRenderTargetsOp(_hgi, _descriptor, _renderTargetDescs, _dsvDesc, _mapRenderTarget2ColorResolveTx));
   }
}

HgiDXGraphicsCmds::~HgiDXGraphicsCmds()
{
}

void
HgiDXGraphicsCmds::PushDebugGroup(const char* label)
{
   //
   // TODO: try to find a way to implement this somehow so that it
   // becomes visible in "renderdoc", 
   // maybe something like this: PIXBeginEvent / PIXEndEvent + PIXSetMarker
   // https://devblogs.microsoft.com/pix/winpixeventruntime/
   //HgiDXBeginLabel(_hgi->GetPrimaryDevice(), _commandBuffer, label);
   //TF_WARN("PushDebugGroup,  Not implemented yet.");
}

void
HgiDXGraphicsCmds::PopDebugGroup()
{
   //
   // TODO: impl - see above
   //HgiDXEndLabel(_hgi->GetPrimaryDevice(), _commandBuffer);
   //TF_WARN("PopDebugGroup,  Not implemented yet.");
}

void
HgiDXGraphicsCmds::SetViewport(GfVec4i const& vp) //[left, BOTTOM, width, height]
{
   m_screenViewport.TopLeftX = m_screenViewport.TopLeftY = 0.f;
   m_screenViewport.Width = vp[2];
   m_screenViewport.Height = vp[3];

   m_screenViewport.MinDepth = D3D12_MIN_DEPTH;
   m_screenViewport.MaxDepth = D3D12_MAX_DEPTH;
   _bViewportSet = true;

   if (!_bScissorsSet)
   {
      m_scissorRect.left = m_scissorRect.top = 0;
      m_scissorRect.right = static_cast<LONG>(vp[2]);
      m_scissorRect.bottom = static_cast<LONG>(vp[3]);
   }
}

void
HgiDXGraphicsCmds::SetScissor(GfVec4i const& sc)
{
   ID3D12GraphicsCommandList* pCmdList = _hgi->GetPrimaryDevice()->GetCommandList(HgiDXDevice::eCommandType::kGraphics);
   if (nullptr != pCmdList)
   {
      m_scissorRect.left = m_scissorRect.top = 0;
      m_scissorRect.right = static_cast<LONG>(sc[2]);
      m_scissorRect.bottom = static_cast<LONG>(sc[3]);

      _bScissorsSet = true;

      if (_bViewportSet)
      {
         m_screenViewport.TopLeftX = m_screenViewport.TopLeftY = 0.f;
         m_screenViewport.Width = sc[2];
         m_screenViewport.Height = sc[3];

         m_screenViewport.MinDepth = D3D12_MIN_DEPTH;
         m_screenViewport.MaxDepth = D3D12_MAX_DEPTH;
      }
   }
   else
      TF_WARN("Failed to acquire command list. Cannot set scissor.");
}

void
HgiDXGraphicsCmds::BindPipeline(HgiGraphicsPipelineHandle pipeline)
{
   _pPipeline = dynamic_cast<HgiDXGraphicsPipeline*>(pipeline.Get());
}

void
HgiDXGraphicsCmds::BindResources(HgiResourceBindingsHandle res)
{
   _resBindings = res;
}

void
HgiDXGraphicsCmds::SetConstantValues(HgiGraphicsPipelineHandle pipeline,
                                     HgiShaderStage stages,
                                     uint32_t bindIndex,
                                     uint32_t byteSize,
                                     const void* data)
{
   //
   // TODO: Impl
   TF_WARN("Const values binding not implemented yet.");
}

void
HgiDXGraphicsCmds::BindVertexBuffers(HgiVertexBufferBindingVector const& bindings)
{
   //
   // delay executing code that relies on a const& obj without copy-ing it is risky,
   // but since this is a prototype at this stage and since the OpenGL HGI does this also
   // I'll take a leap of faith...

   _vertBindings = &bindings;
}

void
HgiDXGraphicsCmds::Draw(uint32_t vertexCount,
                        uint32_t baseVertex,
                        uint32_t instanceCount,
                        uint32_t baseInstance)
{
   // TODO: Impl
   TF_WARN("Draw command version not implemented yet.");
}

void
HgiDXGraphicsCmds::DrawIndirect(HgiBufferHandle const& drawParameterBuffer,
                                uint32_t drawBufferByteOffset,
                                uint32_t drawCount,
                                uint32_t stride)
{
   // TODO: Impl
   TF_WARN("Draw (indirect) command version not implemented yet.");
}

void
HgiDXGraphicsCmds::DrawIndexed(HgiBufferHandle const& indexBuffer,
                               uint32_t indexCount,
                               uint32_t indexBufferByteOffset,
                               uint32_t baseVertex,
                               uint32_t instanceCount,
                               uint32_t baseInstance)
{
   HgiDX* pHgi = _hgi;
   _ApplyPendingUpdates();

   _ops.push_back(
      [pHgi, &indexBuffer, indexCount, indexBufferByteOffset, baseVertex, instanceCount, baseInstance] {
         ID3D12GraphicsCommandList* pCmdList = pHgi->GetPrimaryDevice()->GetCommandList(HgiDXDevice::eCommandType::kGraphics);
         if (nullptr != pCmdList)
         {
            HgiDXBuffer* pIdxBuffer = dynamic_cast<HgiDXBuffer*>(indexBuffer.Get());
            if (nullptr != pIdxBuffer)
            {
               pIdxBuffer->UpdateResourceState(pCmdList, D3D12_RESOURCE_STATE_INDEX_BUFFER);

               D3D12_INDEX_BUFFER_VIEW ibv;
               ibv.BufferLocation = pIdxBuffer->GetGPUVirtualAddress();
               ibv.SizeInBytes = pIdxBuffer->GetByteSizeOfResource();

               //
               // TODO: we definitely need a better way to get this format...
               ibv.Format = DXGI_FORMAT_R32_UINT;

               pCmdList->IASetIndexBuffer(&ibv);

               pCmdList->DrawIndexedInstanced(indexCount, instanceCount, indexBufferByteOffset, baseVertex, baseInstance);
            }
            else
               TF_WARN("Unrecognized indices buffer type. Cannot bind to pipeline.");
         }
         else
            TF_WARN("Failed to acquire command list. Cannot draw.");
      });
}

void
HgiDXGraphicsCmds::DrawIndexedIndirect(HgiBufferHandle const& indexBuffer,
                                       HgiBufferHandle const& drawParameterBuffer,
                                       uint32_t drawBufferByteOffset,
                                       uint32_t drawCount,
                                       uint32_t stride,
                                       std::vector<uint32_t> const& /*drawParameterBufferUInt32*/,
                                       uint32_t /*patchBaseVertexByteOffset*/)
{
   HgiDX* pHgi = _hgi;
   HgiDXGraphicsPipeline* pPipeline = _pPipeline;
   _ApplyPendingUpdates();

   _ops.push_back(
      [pHgi, pPipeline, &indexBuffer, &drawParameterBuffer, drawBufferByteOffset, drawCount, stride] {
      ID3D12GraphicsCommandList* pCmdList = pHgi->GetPrimaryDevice()->GetCommandList(HgiDXDevice::eCommandType::kGraphics);
      if (nullptr != pCmdList)
      {
         HgiDXBuffer* pIdxBuffer = dynamic_cast<HgiDXBuffer*>(indexBuffer.Get());
         HgiDXBuffer* pDrawParamBuffer = dynamic_cast<HgiDXBuffer*>(drawParameterBuffer.Get());
         if ((nullptr != pIdxBuffer) && (nullptr != pDrawParamBuffer))
         {
            //
            // buffers debug code
#ifdef DEBUG_BUFFERS
            std::stringstream buffer;
            buffer << "Info: Binding buffer: " << pIdxBuffer->GetResource() << ", as index buffer" << ",on thread : " << std::this_thread::get_id();
            TF_STATUS(buffer.str());
            buffer.str(std::string());
            buffer << "Info: Binding buffer: " << pDrawParamBuffer->GetResource() << ", as indirect param buffer" << ",on thread : " << std::this_thread::get_id();
            TF_STATUS(buffer.str());
#endif

            pIdxBuffer->UpdateResourceState(pCmdList, D3D12_RESOURCE_STATE_INDEX_BUFFER);
            pDrawParamBuffer->UpdateResourceState(pCmdList, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

            D3D12_INDEX_BUFFER_VIEW ibv;
            ibv.BufferLocation = pIdxBuffer->GetGPUVirtualAddress();
            ibv.SizeInBytes = pIdxBuffer->GetByteSizeOfResource();

            //
            // TODO: we definitely need a better way to get this size... or do we?
            ibv.Format = DXGI_FORMAT_R32_UINT;

            pCmdList->IASetIndexBuffer(&ibv);

            ID3D12CommandSignature* pIndirectSig = pPipeline->GetIndirectCommandSignature(stride);
            if (nullptr != pIndirectSig)
            {
               TF_STATUS("Info: Posting draw (ExecuteIndirect) command.");
               pCmdList->ExecuteIndirect(pIndirectSig,
                                         drawCount,
                                         pDrawParamBuffer->GetResource(),
                                         0,
                                         nullptr, // TODO: if this is ever valuable, I need to set it up myself
                                         0);

               //
               // experimentally, also submit this draw before I start preparing for the next
               //
               // TODO: maybe there is an opportunity here to parallelize draw by using different queues?
               // Also, at some point we should review all these decisions of 
               // work distribution & submitting lists
               pHgi->GetPrimaryDevice()->SubmitCommandList(HgiDXDevice::eCommandType::kGraphics);
            }
            else
               TF_WARN("Invalid indirect command signature. Failed to draw.");
         }
         else
            TF_WARN("Unrecognized indices buffer or draw param type. Cannot bind to pipeline.");
      }
      else
         TF_WARN("Failed to acquire command list. Cannot draw.");
   });
}

void
HgiDXGraphicsCmds::InsertMemoryBarrier(HgiMemoryBarrier barrier)
{
   //
   // In DirectX we set memory barriers for each resource 
   // when we transition it to the proper state before using it
   // And also we have a fence to ensure all commands are executed
   // when submiting a command list.
   // I do not think this is in any way necessary, and if it is,
   // it should probably map to the command list wait (WaitForCPU)
}

void 
HgiDXGraphicsCmds::_ApplyPendingUpdates()
{
   HgiDX* pHgi = _hgi;

   //
   // Because some of the resources setup before this stage could involve buffers copy 
   // and resources states transitioning
   // (which are currently only executed on the graphics queue)
   // I want to make sure these buffers are really ready before we start using them to draw
   //_hgi->GetPrimaryDevice()->SubmitCommandList(HgiDXDevice::eCommandType::kCopy);
   _ops.push_back([pHgi] {
      pHgi->GetPrimaryDevice()->SubmitCommandList(HgiDXDevice::eCommandType::kGraphics); 
   });

   //
   // Bind the pipeline
   HgiDXGraphicsPipeline* pPipeline = _pPipeline;
   _ops.push_back([pPipeline] {
      pPipeline->BindPipeline(); 
   });

   _ops.push_back(_SetupRenderTargetsOp(_hgi, _renderTargetDescs, _dsvDesc));

   //
   // Setup the viewport
   if (_bViewportSet || _bScissorsSet)
   {
      D3D12_VIEWPORT* vp(&m_screenViewport);
      D3D12_RECT* sc(&m_scissorRect);
      _ops.push_back(_SetupViewportOp(pHgi, vp, sc));
   }
   else
      TF_WARN("Viewport & scissor information missing -> not set.");

   //
   // Bind the vertex buffers
   _ops.push_back(_BindVertexBuffersOp(_hgi, _pPipeline, *_vertBindings));
   
   //
   // bind the root params
   _ops.push_back(_BindRootParamsOp(_hgi, _pPipeline, _resBindings->GetDescriptor()));

}

HgiDXGfxFunction 
HgiDXGraphicsCmds::_ClearRenderTargetsOp(HgiDX* pHgi,
                                         const HgiGraphicsCmdsDesc& desc,
                                         std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& renderTargetDescs,
                                         D3D12_CPU_DESCRIPTOR_HANDLE& dsvDesc,
                                         std::map<HgiDXTexture*, HgiDXTexture*>& mapRenderTarget2ColorResolveTx)
{
   return [pHgi, desc, &renderTargetDescs, &dsvDesc, &mapRenderTarget2ColorResolveTx] {
      ID3D12GraphicsCommandList* pCmdList = pHgi->GetPrimaryDevice()->GetCommandList(HgiDXDevice::eCommandType::kGraphics);
      if (nullptr != pCmdList)
      {
         HgiTextureHandleVector colorTextures = desc.colorTextures;

         if (colorTextures.size() > 4)
            TF_WARN("Potentially too many render target textures, maybe not handled properly yet.");

         HgiTextureHandleVector colorResolveTextures = desc.colorResolveTextures;
         HgiAttachmentDescVector colorDescs = desc.colorAttachmentDescs;
         uint32_t nColorResolveTxNum = colorResolveTextures.size();
         uint32_t nColorDescsNum = colorDescs.size();
         TF_VERIFY(nColorDescsNum == colorTextures.size());

         uint32_t nIdx = 0;
         for (HgiTextureHandle tx : colorTextures)
         {
            HgiDXTexture* pDxTexRTV = dynamic_cast<HgiDXTexture*>(tx.Get());
            pDxTexRTV->UpdateResourceState(pCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

            D3D12_CPU_DESCRIPTOR_HANDLE rtvDesc = pDxTexRTV->GetRenderTargetView(renderTargetDescs.size());
            renderTargetDescs.push_back(rtvDesc);

            if (nIdx < nColorDescsNum)
            {
               const HgiAttachmentDesc& desc = colorDescs[nIdx];
               if (desc.loadOp == HgiAttachmentLoadOpClear)
                  pCmdList->ClearRenderTargetView(rtvDesc, desc.clearValue.data(), 0, nullptr);
            }

            if (nIdx < nColorResolveTxNum)
            {
               HgiDXTexture* pDxTexResolve = dynamic_cast<HgiDXTexture*>(colorResolveTextures[nIdx].Get());
               mapRenderTarget2ColorResolveTx.insert(std::pair<HgiDXTexture*, HgiDXTexture*>(pDxTexRTV, pDxTexResolve));
            }

            nIdx++;
         }

         HgiDXTexture* pDxTexDSV = dynamic_cast<HgiDXTexture*>(desc.depthTexture.Get());
         if (nullptr != pDxTexDSV)
         {
            pDxTexDSV->UpdateResourceState(pCmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
            dsvDesc = pDxTexDSV->GetDepthStencilView(0);

            HgiAttachmentDesc depthDesc = desc.depthAttachmentDesc;
            if (depthDesc.loadOp == HgiAttachmentLoadOpClear)
            {
               if (depthDesc.usage & HgiTextureUsageBitsStencilTarget)
                  pCmdList->ClearDepthStencilView(dsvDesc, D3D12_CLEAR_FLAG_DEPTH, depthDesc.clearValue[0], depthDesc.clearValue[1], 0, nullptr);
               else
                  pCmdList->ClearDepthStencilView(dsvDesc, D3D12_CLEAR_FLAG_DEPTH, depthDesc.clearValue[0], 0, 0, nullptr);
            }
         }
      }
      else
         TF_WARN("Failed to acquire command list. Cannot setup render targets.");
   };
}

HgiDXGfxFunction 
HgiDXGraphicsCmds::_SetupRenderTargetsOp(HgiDX* pHgi, 
                                         std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& renderTargetDescs,
                                         D3D12_CPU_DESCRIPTOR_HANDLE& dsvDesc)
{
   return [pHgi, &renderTargetDescs, &dsvDesc] {
      ID3D12GraphicsCommandList* pCmdList = pHgi->GetPrimaryDevice()->GetCommandList(HgiDXDevice::eCommandType::kGraphics);
      if (nullptr != pCmdList)
      {
         //
         // DirectX does not allow different rendering targets at the same time, e.g. one with 4 MS and one with 1 (at the same time)
         // so what I'll do is render to the render target and if I have to resolve to the single sample texture 
         // in a separate step
         pCmdList->OMSetRenderTargets(renderTargetDescs.size(), renderTargetDescs.data(), FALSE, &dsvDesc);
      }
      else
         TF_WARN("Failed to acquire command list. Cannot setup render targets.");
   };
}

HgiDXGfxFunction
HgiDXGraphicsCmds::_SetupViewportOp(HgiDX* pHgi, D3D12_VIEWPORT* vp, D3D12_RECT* sc)
{
   return [pHgi, vp, sc] {
      ID3D12GraphicsCommandList* pCmdList = pHgi->GetPrimaryDevice()->GetCommandList(HgiDXDevice::eCommandType::kGraphics);
      if (nullptr != pCmdList)
      {
         pCmdList->RSSetViewports(1, vp);
         pCmdList->RSSetScissorRects(1, sc);
      }
      else
         TF_WARN("Failed to acquire command list. Cannot setup viewport & scissors.");
   };
}

HgiDXGfxFunction
HgiDXGraphicsCmds::_BindVertexBuffersOp(HgiDX* pHgi,
                                        HgiDXGraphicsPipeline* pPipeline,
                                        const HgiVertexBufferBindingVector& vertBindings)
{
   return [pHgi, pPipeline, vertBindings] {
      ID3D12GraphicsCommandList* pCmdList = pHgi->GetPrimaryDevice()->GetCommandList(HgiDXDevice::eCommandType::kGraphics);
      if (nullptr != pCmdList && nullptr != pPipeline)
      {
         uint32_t nBuffers = vertBindings.size();
         const HgiVertexBufferDescVector& vertBufDeclarations = pPipeline->GetDescriptor().vertexBuffers;

         if(nBuffers != vertBufDeclarations.size())
            TF_WARN("Vertex buffers declaartions do not match the bindings.");

         std::vector<D3D12_VERTEX_BUFFER_VIEW> vertBufs;
         
         for (const HgiVertexBufferDesc& vertBuffDecl : vertBufDeclarations)
         {
            uint32_t nDeclaredBindingSlot = vertBuffDecl.bindingIndex; // in DX terms this is binding slot

            uint32_t nStride = vertBuffDecl.vertexStride;
            uint32_t nNumAttr = vertBuffDecl.vertexAttributes.size();

            const HgiVertexBufferBinding& vbb = vertBindings[nDeclaredBindingSlot];
            HgiDXBuffer* pDxbuff = dynamic_cast<HgiDXBuffer*>(vbb.buffer.Get());

            static bool bDebug = false;
            if (bDebug)
               HgiDXBuffer::SetWatchBuffer(pDxbuff);
               

            pDxbuff->UpdateResourceState(pCmdList, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            uint32_t byteOffset = vbb.byteOffset;

            //
            // buffers debug code
#ifdef DEBUG_BUFFERS
            std::stringstream buffer;
            buffer << "Info: Binding buffer: " << pDxbuff->GetResource() << ", as input vertex buffer" << ",on thread : " << std::this_thread::get_id();
            TF_STATUS(buffer.str());
#endif

            //
            // Seems I need to sync data with slots, not with number of inputs & binding index
            D3D12_VERTEX_BUFFER_VIEW vbv;
            vbv.BufferLocation = pDxbuff->GetGPUVirtualAddress();
            vbv.SizeInBytes = pDxbuff->GetByteSizeOfResource();
            vbv.StrideInBytes = nStride;

            vertBufs.push_back(vbv);
         }

         pCmdList->IASetVertexBuffers(0, vertBufs.size(), vertBufs.data());
      }
      else
         TF_WARN("Failed to acquire command list or vertex bindings or pipeline. Cannot bind vertex buffer(s).");
   };
}

HgiDXGfxFunction 
HgiDXGraphicsCmds::_BindRootParamsOp(HgiDX* pHgi,
                                     HgiDXGraphicsPipeline* pPipeline,
                                     const HgiResourceBindingsDesc& resBindingsDesc)
{
   return [pHgi, pPipeline, resBindingsDesc] {
      ID3D12GraphicsCommandList* pCmdList = pHgi->GetPrimaryDevice()->GetCommandList(HgiDXDevice::eCommandType::kGraphics);
      if (nullptr != pCmdList && nullptr != pPipeline)
      {
         const HgiGraphicsPipelineDesc& gpd = pPipeline->GetDescriptor();
         HgiDXShaderProgram* pShaderProgram = dynamic_cast<HgiDXShaderProgram*>(gpd.shaderProgram.Get());

         if (nullptr != pShaderProgram)
            HgiDXResourceBindings::BindRootParams(pCmdList, pShaderProgram, resBindingsDesc.buffers, false);
         else
            TF_WARN("Failed to acquire shader program or bindings resources. Cannot bind root params buffer(s).");
      }
      else
         TF_WARN("Failed to acquire command list or pipeline. Cannot bind root params buffer(s).");
   };
}

bool 
HgiDXGraphicsCmds::_Submit(Hgi* hgi, HgiSubmitWaitType wait)
{
   if (_ops.empty()) {
      return false;
   }

   TF_STATUS("Submitting %d graphics operations.", _ops.size());

   for (HgiDXGfxFunction const& f : _ops) {
      f();
   }

   _hgi->GetPrimaryDevice()->SubmitCommandList(HgiDXDevice::eCommandType::kGraphics);

   static bool bDebug = false;
   if (bDebug)
   {
      HgiDXBuffer* pWatchBuff = HgiDXBuffer::GetWatchBuffer();
      if(nullptr != pWatchBuff)
         pWatchBuff->InspectBufferContents();
   }

   _SetSubmitted();

   //
   // And now resolve the buffers (if needed)
   // (msaa -> single sample, compatible to target window)
   if (_mapRenderTarget2ColorResolveTx.size())
   {
      ID3D12GraphicsCommandList* pCmdList = _hgi->GetPrimaryDevice()->GetCommandList(HgiDXDevice::eCommandType::kGraphics);
      if (nullptr != pCmdList)
      {
         auto it = _mapRenderTarget2ColorResolveTx.begin();
         auto itEnd = _mapRenderTarget2ColorResolveTx.end();
         while (it != itEnd)
         {
            it->second->Resolve(pCmdList, it->first);

            it++;
         }

         //
         // and submit again
         _hgi->GetPrimaryDevice()->SubmitCommandList(HgiDXDevice::eCommandType::kGraphics);
      }
      else
         TF_WARN("Failed to acquire command list. Cannot resolve render target.");
   }

   return true;
}


PXR_NAMESPACE_CLOSE_SCOPE
