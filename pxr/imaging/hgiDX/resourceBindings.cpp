
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
HgiDXResourceBindings::BindRootParams(ID3D12GraphicsCommandList* pCmdList, 
                                      HgiDXShaderProgram* pShaderProgram, 
                                      const HgiBufferBindDescVector& bindBuffersDescs,
                                      bool bCompute)
{
   if ((nullptr != pCmdList) && (nullptr != pShaderProgram))
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
               bool bFound = pShaderProgram->GetInfo(nBindingIdx, rpi, false);

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
                     pDxBuff->UpdateResourceState(pCmdList, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                     if (bCompute)
                     {
                        pCmdList->SetComputeRootConstantBufferView(rpi.nBindingIdx, pDxBuff->GetGPUVirtualAddress() + nOffset);
                     }
                     else
                     {
                        pCmdList->SetGraphicsRootConstantBufferView(rpi.nBindingIdx, pDxBuff->GetGPUVirtualAddress() + nOffset);
                     }
                  }
                  else if (rpi.bWritable)
                  {
                     if (bCompute)
                     {
                        pDxBuff->UpdateResourceState(pCmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                        pCmdList->SetComputeRootUnorderedAccessView(rpi.nBindingIdx, pDxBuff->GetGPUVirtualAddress() + nOffset);
                     }
                     else
                     {
                        pDxBuff->UpdateResourceState(pCmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                        pCmdList->SetGraphicsRootUnorderedAccessView(rpi.nBindingIdx, pDxBuff->GetGPUVirtualAddress() + nOffset);
                     }
                  }
                  else
                  {
                     if (bCompute)
                     {
                        pDxBuff->UpdateResourceState(pCmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                        pCmdList->SetComputeRootShaderResourceView(rpi.nBindingIdx, pDxBuff->GetGPUVirtualAddress() + nOffset);
                     }
                     else
                     {
                        pDxBuff->UpdateResourceState(pCmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                        pCmdList->SetGraphicsRootShaderResourceView(rpi.nBindingIdx, pDxBuff->GetGPUVirtualAddress() + nOffset);
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
HgiDXResourceBindings::UnBindRootParams(ID3D12GraphicsCommandList* pCmdList,
                                        HgiDXShaderProgram* pShaderProgram,
                                        const HgiBufferBindDescVector& bindBuffersDescs,
                                        bool bCompute)
{
   if ((nullptr != pCmdList) && (nullptr != pShaderProgram))
   {
      for (const HgiBufferBindDesc& bd : bindBuffersDescs)
      {
         int nBindingIdx = bd.bindingIndex;

         const HgiBufferHandle& bh = bd.buffers[0];
         HgiDXBuffer* pDxBuff = dynamic_cast<HgiDXBuffer*>(bh.Get());
         if (nullptr != pDxBuff)
            pDxBuff->UpdateResourceState(pCmdList, D3D12_RESOURCE_STATE_COPY_DEST);

         DXShaderInfo::RootParamInfo rpi;
         bool bFound = pShaderProgram->GetInfo(nBindingIdx, rpi, false);
         if (bFound)
         {
            if (rpi.bConst)
            {
               if (bCompute)
                  pCmdList->SetComputeRootConstantBufferView(rpi.nBindingIdx, 0);
               else
                  pCmdList->SetGraphicsRootConstantBufferView(rpi.nBindingIdx, 0);
            }
            else if (rpi.bWritable)
            {
               if (bCompute)
                  pCmdList->SetComputeRootUnorderedAccessView(rpi.nBindingIdx, 0);
               else
                  pCmdList->SetGraphicsRootUnorderedAccessView(rpi.nBindingIdx, 0);
            }
            else
            {
               if (bCompute)
                  pCmdList->SetComputeRootShaderResourceView(rpi.nBindingIdx, 0);
               else
                  pCmdList->SetGraphicsRootShaderResourceView(rpi.nBindingIdx, 0);
            }
         }
         else
            TF_WARN("Failed to find buffer by suggested binding index. Cannot unbind.");
      }
   }
   else
      TF_WARN("Invalid comand list of shader program. Cannot bind resources.");
}

PXR_NAMESPACE_CLOSE_SCOPE
