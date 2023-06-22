
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
#include "pxr/imaging/hgiDX/computeCmds.h"
#include "pxr/imaging/hgiDX/computePipeline.h"
#include "pxr/imaging/hgiDX/device.h"
#include "pxr/imaging/hgiDX/memoryHelper.h"
#include "pxr/imaging/hgiDX/resourceBindings.h"
#include "pxr/imaging/hgiDX/shaderProgram.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiDXComputeCmds::HgiDXComputeCmds(HgiDX* hgi, HgiComputeCmdsDesc const& desc)
    : HgiComputeCmds()
    , _hgi(hgi)
{
}

HgiDXComputeCmds::~HgiDXComputeCmds()
{
   for (HgiBufferHandle bh : _constValuesBuffers)
      _hgi->DestroyBuffer(&bh);
}

void
HgiDXComputeCmds::PushDebugGroup(const char* label)
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
HgiDXComputeCmds::PopDebugGroup()
{
   //
   // TODO: impl - see above
   //HgiDXEndLabel(_hgi->GetPrimaryDevice(), _commandBuffer);
   //TF_WARN("PopDebugGroup,  Not implemented yet.");
}

void
HgiDXComputeCmds::BindPipeline(HgiComputePipelineHandle pipeline)
{
   HgiDXComputePipeline* pDXPipeline = dynamic_cast<HgiDXComputePipeline*>(pipeline.Get());
   _pPipeline = pDXPipeline;

   _ops.push_back([pDXPipeline] {
      pDXPipeline->BindPipeline();
   });
}

void
HgiDXComputeCmds::BindResources(HgiResourceBindingsHandle res)
{
   _resBindings = res;
}

void
HgiDXComputeCmds::SetConstantValues(HgiComputePipelineHandle pipeline,
                                    uint32_t bindIndex,
                                    uint32_t byteSize,
                                    const void* data)
{
   //
   // My idea is to route this to the more "normal" workflow:
   // upload the const data to a gpu buffer,
   // and bind the gpu buffer to it's proper place.

   HgiBufferDesc bd;

   //
   // I saw some complaints in RenderDoc about the size of this buffer being less than what Dx would expect
   // e.g. 28 bytes vs expected: 32. 
   // I also noticed in some cases my Radeon card was quite sensitive to inadequate buffers sizes 
   // even when it should not have mattered, so I'll try to fix here at least half of the problem:
   // allocate a buffer large enough to not raise any driver eye brows.
   //
   // The other half (proper aligning and padding of memory data), if ever a real issue
   // will require a much more complex solution, because at this point, here I have no idea what kind of data
   // will be inside the buffer.
   //
   // TODO: It seems it would be more efficient to leave this buffer data in CPU
   // instead of wasting time to copy it to GPu for such a small scope / time.
   bd.byteSize = HgiDXMemoryHelper::RoundUp(byteSize);
   bd.debugName = "compute pipeline constant values";
   bd.vertexStride = byteSize;
   bd.initialData = nullptr;
   bd.usage = HgiBufferUsageBits::HgiBufferUsageStorage;

   HgiBufferHandle bh = _hgi->CreateBuffer(bd);
   _constValuesBuffers.push_back(bh);

   dynamic_cast<HgiDXBuffer*>(bh.Get())->UpdateData(data, byteSize, 0, 0);
}

void
HgiDXComputeCmds::Dispatch(int dimX, int dimY)
{
   //
   // resources binding
   HgiDX* pHgi = _hgi;
   HgiDXComputePipeline* pPipeline = _pPipeline;
   const HgiResourceBindingsDesc& resBindingsDesc = _resBindings->GetDescriptor();
   
   _ops.push_back(
      [pHgi, pPipeline, resBindingsDesc] {
         ID3D12GraphicsCommandList* pCmdList = pHgi->GetPrimaryDevice()->GetCommandList(HgiDXDevice::eCommandType::kCompute);
         if (nullptr != pCmdList && nullptr != pPipeline)
         {
            const HgiComputePipelineDesc& gpd = pPipeline->GetDescriptor();
            HgiDXShaderProgram* pShaderProgram = dynamic_cast<HgiDXShaderProgram*>(gpd.shaderProgram.Get());

            if (nullptr != pShaderProgram)
               HgiDXResourceBindings::BindRootParams(pCmdList, pShaderProgram, resBindingsDesc.buffers, true);
         }
         else
            TF_WARN("Failed to acquire command list or pipeline. Cannot bind root params buffer(s).");
   });

   //
   // const values binding
   const HgiBufferHandle& bh = _constValuesBuffers.back();
   HgiBufferBindDesc constValuesBind;

   //constValuesBind.bindingIndex = bindIndex;
   // One issue with this is that during the "code generation" phase we do not get a buffer declaration for this, but rather separate contents
   // However, even openGl groups these contents and pass them to a shader as one buffer
   // More than that, the reported binding Index is zero for this and it overlaps with another zero for something else (some other buffer)
   // Therefore:
   constValuesBind.bindingIndex = -2; // using my hardcoded "trick" buffer index; TODO: handle this better / cleaner - at least one static common value somewhere
   constValuesBind.buffers = std::vector<HgiBufferHandle>{ bh };
   constValuesBind.offsets = std::vector<uint32_t>{ 0 };
   constValuesBind.sizes = std::vector<uint32_t>{ (uint32_t)bh->GetDescriptor().byteSize };
   constValuesBind.writable = false;

   //
   // putting this into a vector just to avoid adding an override to the "bind" method called below
   std::vector<HgiBufferBindDesc> descs;
   descs.push_back(std::move(constValuesBind));

   _ops.push_back(
      [pHgi, pPipeline, descs] {
         ID3D12GraphicsCommandList* pCmdList = pHgi->GetPrimaryDevice()->GetCommandList(HgiDXDevice::eCommandType::kCompute);
         if (nullptr != pCmdList && nullptr != pPipeline)
         {
            const HgiComputePipelineDesc& gpd = pPipeline->GetDescriptor();
            HgiDXShaderProgram* pShaderProgram = dynamic_cast<HgiDXShaderProgram*>(gpd.shaderProgram.Get());

            if (nullptr != pShaderProgram)
               HgiDXResourceBindings::BindRootParams(pCmdList, pShaderProgram, descs, true);
         }
         else
            TF_WARN("Failed to acquire command list or pipeline. Cannot bind root params buffer(s).");
      });

   //
   // Dispatch
   _ops.push_back(_DispatchOp(_hgi, dimX, dimY));
}

HgiDXGfxFunction 
HgiDXComputeCmds::_DispatchOp(HgiDX* pHgi, int dimX, int dimY)
{
   return[pHgi, dimX, dimY] {
      
      //
      // Because some of the resources setup before this stage could involve buffers copy 
      // and resources states transitioning
      // (which are currently only executed on the graphics queue)
      // I want to make sure these buffers are really ready before we start using them to draw.
      // For compute commands case, even the buffers binding might use the graphics queue
      pHgi->GetPrimaryDevice()->SubmitCommandList(HgiDXDevice::eCommandType::kGraphics);

      ID3D12GraphicsCommandList* pCmdList = pHgi->GetPrimaryDevice()->GetCommandList(HgiDXDevice::eCommandType::kCompute);
      if (nullptr != pCmdList)
      {
         TF_STATUS("Info: Posting compute command.");
         pCmdList->Dispatch(dimX, dimY, 1);

         //
         // experimentally, also submit this Dispatch before I start preparing for the next
         //
         // TODO: maybe there is an opportunity here to parallelize compute by using different queues?
         pHgi->GetPrimaryDevice()->SubmitCommandList(HgiDXDevice::eCommandType::kCompute);
      }
      else
         TF_WARN("Failed to acquire command list. Cannot execute compute pipeline.");
   };
}

void
HgiDXComputeCmds::InsertMemoryBarrier(HgiMemoryBarrier barrier)
{
   //
   // Applying the same policy here as for graphicsCommands:
   // 
   // In DirectX we set memory barriers for each resource 
   // when we transition it to the proper state before using it
   // And also we have a fence to ensure all commands are executed
   // when submiting a command list.
   // I do not think this is in any way necessary, and if it is,
   // it should probably map to the command list wait (WaitForCPU)
   // which is nothing else than waiting for the fence mentioned above.
}

bool
HgiDXComputeCmds::_Submit(Hgi* hgi, HgiSubmitWaitType wait)
{
   if (_ops.empty()) {
      return false;
   }

   for (HgiDXGfxFunction const& f : _ops) {
      f();
   }

   _hgi->GetPrimaryDevice()->SubmitCommandList(HgiDXDevice::eCommandType::kCompute);

   //static bool bDebug = false;
   //if (bDebug)
   //{
   //   HgiDXBuffer* pWatchBuff = HgiDXBuffer::GetWatchBuffer();
   //   if (nullptr != pWatchBuff)
   //      pWatchBuff->InspectBufferContents();
   //}
   
   _SetSubmitted();

   return true;
}

HgiComputeDispatch 
HgiDXComputeCmds::GetDispatchMethod() const
{
   //
   // TODO: how about the other option - what does it mean, what does it do? Can we support that also?
   return HgiComputeDispatchSerial;
}

PXR_NAMESPACE_CLOSE_SCOPE
