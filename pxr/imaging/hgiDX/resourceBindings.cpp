
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
#include "pxr/base/tf/diagnostic.h"

#include "pxr/imaging/hgiDX/buffer.h"
#include "pxr/imaging/hgiDX/capabilities.h"
#include "pxr/imaging/hgiDX/device.h"
#include "pxr/imaging/hgiDX/hgi.h"
#include "pxr/imaging/hgiDX/resourceBindings.h"
#include "pxr/imaging/hgiDX/sampler.h"
#include "pxr/imaging/hgiDX/buffer.h"
#include "pxr/imaging/hgiDX/texture.h"
#include "pxr/imaging/hgiDX/shaderProgram.h"

#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE


HgiDXResourceBindings::HgiDXResourceBindings( HgiDXDevice* device, HgiResourceBindingsDesc const& desc)
    : HgiResourceBindings(desc)
    , _device(device)
{
}

HgiDXResourceBindings::~HgiDXResourceBindings()
{
}

void
HgiDXResourceBindings::BindResources()
{
}

HgiDXDevice*
HgiDXResourceBindings::GetDevice() const
{
    return _device;
}

//
// Implemented this here because I am calling it 3 times and every time I need to execute the exact same code
// (for now)
void 
HgiDXResourceBindings::BindRootParams(HgiDX* pHgi,
                                      HgiDXShaderProgram* pShaderProgram, 
                                      const HgiBufferBindDescVector& bindBuffersDescs,
                                      bool bCompute)
{
   ID3D12GraphicsCommandList* pGraphicsCmdList = pHgi->GetPrimaryDevice()->GetCommandList(HgiDXDevice::eCommandType::kGraphics);
   ID3D12GraphicsCommandList* pComputeCmdList = nullptr;
   if (bCompute)
      pComputeCmdList = pHgi->GetPrimaryDevice()->GetCommandList(HgiDXDevice::eCommandType::kCompute);

   if ((nullptr != pShaderProgram) && (nullptr != pGraphicsCmdList) && (!bCompute || (nullptr != pComputeCmdList)))
   {
      for (const HgiBufferBindDesc& bd : bindBuffersDescs)
      {
         int nBindingIdx = bd.bindingIndex;

         if (bd.buffers.size() > 1)
         {
            //
            // TODO: I do not understand why there is a collection of buffers here and 
            // how I could bind more than one to a single bind idx??
            TF_WARN("Unexpected number of buffers for a single binding. Probably incorrect binding follows.");
         }

         if (bd.buffers.size() > 0)
         {
            //
            // just handle the first one for now. When the warning above triggers... we'll do better...
            const HgiBufferHandle& bh = bd.buffers[0];

            HgiDXBuffer* pDxBuff = dynamic_cast<HgiDXBuffer*>(bh.Get());
            if (nullptr != pDxBuff)
            {
               DXShaderInfo::RootParamInfo rpi;
               //
               // Hard coded register space 0 for bow for buffers
               // TODO: find a nicer way
               bool bFound = pShaderProgram->GetInfo(nBindingIdx, 0, rpi, false);

               if (bFound)
               {
                  size_t nOffset = 0;
                  if(bd.offsets.size() > 0)
                     nOffset = bd.offsets[0];

                  //
                  // temp debug code
#ifdef DEBUG_BUFFERS
                  std::stringstream buffer;
                  buffer << "Info: Binding buffer: " << pDxBuff->GetResource() 
                     << ",as root param buffer: " << rpi.strName 
                     << ",GPU address: " << pDxBuff->GetGPUVirtualAddress()
                     << ",offset: " << nOffset
                     << ",on thread : " << std::this_thread::get_id();
                  TF_STATUS(buffer.str());
#endif

                  if (rpi.bConst)
                  {
                     pDxBuff->UpdateResourceState(pGraphicsCmdList, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                     if (bCompute)
                        pComputeCmdList->SetComputeRootConstantBufferView(rpi.nBindingIdx, pDxBuff->GetGPUVirtualAddress() + nOffset);
                     else
                        pGraphicsCmdList->SetGraphicsRootConstantBufferView(rpi.nBindingIdx, pDxBuff->GetGPUVirtualAddress() + nOffset);
                  }
                  else if (rpi.bWritable)
                  {
                     if (bCompute)
                     {
                        pDxBuff->UpdateResourceState(pGraphicsCmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                        pComputeCmdList->SetComputeRootUnorderedAccessView(rpi.nBindingIdx, pDxBuff->GetGPUVirtualAddress() + nOffset);
                     }
                     else
                     {
                        pDxBuff->UpdateResourceState(pGraphicsCmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                        pGraphicsCmdList->SetGraphicsRootUnorderedAccessView(rpi.nBindingIdx, pDxBuff->GetGPUVirtualAddress() + nOffset);
                     }
                  }
                  else
                  {
                     if (bCompute)
                     {
                        pDxBuff->UpdateResourceState(pGraphicsCmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                        pComputeCmdList->SetComputeRootShaderResourceView(rpi.nBindingIdx, pDxBuff->GetGPUVirtualAddress() + nOffset);
                     }
                     else
                     {
                        pDxBuff->UpdateResourceState(pGraphicsCmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                        pGraphicsCmdList->SetGraphicsRootShaderResourceView(rpi.nBindingIdx, pDxBuff->GetGPUVirtualAddress() + nOffset);
                     }
                  }
               }
               else
                  TF_WARN("Failed to find buffer by suggested binding index. Cannot assign to pipeline.");
            }
            else
               TF_WARN("Unrecognized buffer type. Cannot bind to pipeline.");
         }
      }
   }
   else
      TF_WARN("Invalid comand list of shader program. Cannot bind resources.");
}

void 
HgiDXResourceBindings::BindRootParams(HgiDX* pHgi,
                                      HgiDXShaderProgram* pShaderProgram,
                                      const HgiTextureBindDescVector& bindBuffersDescs,
                                      bool bCompute)
{
   //
   // temporarily stop binding textures, to see if this is what causes the crash
   //return;

   ID3D12GraphicsCommandList* pGraphicsCmdList = pHgi->GetPrimaryDevice()->GetCommandList(HgiDXDevice::eCommandType::kGraphics);
   ID3D12GraphicsCommandList* pComputeCmdList = nullptr;
   if (bCompute)
      pComputeCmdList = pHgi->GetPrimaryDevice()->GetCommandList(HgiDXDevice::eCommandType::kCompute);

   if ((nullptr != pShaderProgram) && (nullptr != pGraphicsCmdList) && (!bCompute || (nullptr != pComputeCmdList)))
   {
      int nTexturesToBind = bindBuffersDescs.size();
      if (nTexturesToBind > 0)
      {
         ID3D12DescriptorHeap* pTxHeap = pHgi->GetPrimaryDevice()->GetCbvSrvUavDescriptorHeap();
         ID3D12DescriptorHeap* pSamplersHeap = pHgi->GetPrimaryDevice()->GetSamplersDescriptorHeap();
         ID3D12DescriptorHeap* descHeaps[] = { pTxHeap, pSamplersHeap };

         if (bCompute)
            pComputeCmdList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);
         else
            pGraphicsCmdList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);

         for (int nTxIdx = 0; nTxIdx < nTexturesToBind; nTxIdx++)
         {
            const HgiTextureBindDesc& td = bindBuffersDescs[nTxIdx];

            int nBindingIdx = td.bindingIndex;

            int nTextures = td.textures.size();
            int nSamplers = td.samplers.size();

            //
            // TODO: not ready to deal with multiple textures yet
            // will handle simplest case for now
            if (nTextures > 1)
            {
               TF_WARN("Multiple textures in one desc not handled yet.");
            }

            if (nTextures > 0)
            {
               HgiDXTexture* pDxTx = dynamic_cast<HgiDXTexture*>(td.textures[0].Get());
               if (nullptr != pDxTx)
               {
                  DXShaderInfo::RootParamInfo rpi;
                  //
                  // Hard coded register space 1 for bow for buffers
                  // TODO: find a nicer way
                  bool bFound = pShaderProgram->GetInfo(nBindingIdx, 1, rpi, false);

                  if (bFound)
                  {
                     if (bCompute)
                        pDxTx->UpdateResourceState(pGraphicsCmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                     else
                        pDxTx->UpdateResourceState(pGraphicsCmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

                     //
                     // bind the texture
                     if (rpi.bWritable)
                     {
                        D3D12_GPU_DESCRIPTOR_HANDLE gpuDesc = pDxTx->GetGPUDescHandle(nTxIdx, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);

                        if (bCompute)
                           pComputeCmdList->SetComputeRootDescriptorTable(rpi.nBindingIdx, gpuDesc);
                        else
                           pGraphicsCmdList->SetGraphicsRootDescriptorTable(rpi.nBindingIdx, gpuDesc);
                     }
                     else //if (rpi.bConst) - I will only map textures to srv for now, I think CBV is not adequate for textures?! TODO: make sure
                     {
                        D3D12_GPU_DESCRIPTOR_HANDLE gpuDesc = pDxTx->GetGPUDescHandle(nTxIdx, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);

                        if (bCompute)
                           pComputeCmdList->SetComputeRootDescriptorTable(rpi.nBindingIdx, gpuDesc);
                        else
                           pGraphicsCmdList->SetGraphicsRootDescriptorTable(rpi.nBindingIdx, gpuDesc);
                     }

                     //
                     // bind the sampler
                     if (nSamplers > 0 && (rpi.nSamplerBindingIdx >= 0))
                     {
                        HgiDXSampler* pDxSamp = dynamic_cast<HgiDXSampler*>(td.samplers[0].Get());
                        if (nullptr != pDxSamp)
                        {
                           D3D12_GPU_DESCRIPTOR_HANDLE gpuDesc = pDxSamp->GetGPUDescHandle(nTxIdx);

                           if (bCompute)
                              pComputeCmdList->SetComputeRootDescriptorTable(rpi.nSamplerBindingIdx, gpuDesc);
                           else
                              pGraphicsCmdList->SetGraphicsRootDescriptorTable(rpi.nSamplerBindingIdx, gpuDesc);
                        }
                        else
                           TF_WARN("Trying to bind invalid sampler to shader resource.");
                     }
                     else
                        TF_WARN("Invalid sampler information for texture. Cannot bind to pipeline.");
                  }
                  else
                     TF_WARN("Failed to find texture by suggested binding index. Cannot assign to pipeline.");
               }
               else
                  TF_WARN("Trying to bind invalid texture to shader resource.");
            }
         }
      }
   }
   else
      TF_WARN("Invalid comand list of shader program. Cannot bind resources.");
}

void 
HgiDXResourceBindings::UnBindRootParams(HgiDX* pHgi,
                                        HgiDXShaderProgram* pShaderProgram,
                                        const HgiBufferBindDescVector& bindBuffersDescs,
                                        bool bCompute)
{
   ID3D12GraphicsCommandList* pGraphicsCmdList = pHgi->GetPrimaryDevice()->GetCommandList(HgiDXDevice::eCommandType::kGraphics);
   ID3D12GraphicsCommandList* pComputeCmdList = nullptr;
   if (bCompute)
      pComputeCmdList = pHgi->GetPrimaryDevice()->GetCommandList(HgiDXDevice::eCommandType::kCompute);

   if ((nullptr != pShaderProgram) && (nullptr != pGraphicsCmdList) && (!bCompute || (nullptr != pComputeCmdList)))
   {
      for (const HgiBufferBindDesc& bd : bindBuffersDescs)
      {
         int nBindingIdx = bd.bindingIndex;

         const HgiBufferHandle& bh = bd.buffers[0];
         HgiDXBuffer* pDxBuff = dynamic_cast<HgiDXBuffer*>(bh.Get());
         if (nullptr != pDxBuff)
            pDxBuff->UpdateResourceState(pGraphicsCmdList, D3D12_RESOURCE_STATE_COPY_DEST);

         DXShaderInfo::RootParamInfo rpi;
         //
         // TODO: deal better with hard-coded 0
         bool bFound = pShaderProgram->GetInfo(nBindingIdx, 0, rpi, false);
         if (bFound)
         {
            if (rpi.bConst)
            {
               if (bCompute)
                  pComputeCmdList->SetComputeRootConstantBufferView(rpi.nBindingIdx, 0);
               else
                  pGraphicsCmdList->SetGraphicsRootConstantBufferView(rpi.nBindingIdx, 0);
            }
            else if (rpi.bWritable)
            {
               if (bCompute)
                  pComputeCmdList->SetComputeRootUnorderedAccessView(rpi.nBindingIdx, 0);
               else
                  pGraphicsCmdList->SetGraphicsRootUnorderedAccessView(rpi.nBindingIdx, 0);
            }
            else
            {
               if (bCompute)
                  pComputeCmdList->SetComputeRootShaderResourceView(rpi.nBindingIdx, 0);
               else
                  pGraphicsCmdList->SetGraphicsRootShaderResourceView(rpi.nBindingIdx, 0);
            }
         }
         else
            TF_WARN("Failed to find buffer by suggested binding index. Cannot unbind.");
      }
   }
   else
      TF_WARN("Invalid comand list of shader program. Cannot bind resources.");
}

void 
HgiDXResourceBindings::UnBindRootParams(HgiDX* pHgi,
                                        HgiDXShaderProgram* pShaderProgram,
                                        const HgiTextureBindDescVector& bindBuffersDescs,
                                        bool bCompute)
{
   //
   // TODO
   TF_WARN("Not implemented yet.");
}


PXR_NAMESPACE_CLOSE_SCOPE
