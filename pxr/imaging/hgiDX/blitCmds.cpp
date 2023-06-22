
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
#include "pxr/imaging/hgiDX/blitCmds.h"
#include "pxr/imaging/hgiDX/buffer.h"
#include "pxr/imaging/hgiDX/device.h"
#include "pxr/imaging/hgiDX/hgi.h"
#include "pxr/imaging/hgiDX/texture.h"
#include "pxr/imaging/hgi/blitCmdsOps.h"



PXR_NAMESPACE_OPEN_SCOPE

HgiDXBlitCmds::HgiDXBlitCmds(HgiDX* hgi)
   : _hgi(hgi)
{
   // We do not acquire the command buffer here, because the Cmds object may
   // have been created on the main thread, but used on a secondary thread.
   // We need to acquire a command buffer for the thread that is doing the
   // recording so we postpone acquiring cmd buffer until first use of Cmds.
}

HgiDXBlitCmds::~HgiDXBlitCmds() = default;

void
HgiDXBlitCmds::PushDebugGroup(const char* label)
{
   //
   // TODO: try to find a way to implement this somehow so that it
   // becomes visible in "renderdoc", 
   // maybe something like this: PIXBeginEvent / PIXEndEvent + PIXSetMarker
   // https://devblogs.microsoft.com/pix/winpixeventruntime/
   //HgiDXBeginLabel(_hgi->GetPrimaryDevice(), _commandBuffer, label);
}

void
HgiDXBlitCmds::PopDebugGroup()
{
   //
   // TODO: see above
   //HgiDXEndLabel(_hgi->GetPrimaryDevice(), _commandBuffer);
}

void
HgiDXBlitCmds::CopyTextureGpuToCpu(HgiTextureGpuToCpuOp const& copyOp)
{
   HgiDXTexture* pTxSrc = dynamic_cast<HgiDXTexture*>(copyOp.gpuSourceTexture.Get());

   if (nullptr != pTxSrc)
   {
      pTxSrc->ReadbackData(copyOp.sourceTexelOffset, 
                           copyOp.mipLevel,
                           copyOp.cpuDestinationBuffer,
                           copyOp.destinationByteOffset,
                           copyOp.destinationBufferByteSize);
   }
   else
      TF_WARN("Invalid texture. Cannot execute data copy.");
}

void
HgiDXBlitCmds::CopyTextureCpuToGpu(HgiTextureCpuToGpuOp const& copyOp)
{
   TF_WARN("CopyTextureCpuToGpu,  Not implemented yet.");
}

void HgiDXBlitCmds::CopyBufferGpuToGpu(HgiBufferGpuToGpuOp const& copyOp)
{
   HgiDXBuffer* pBuffSrc = dynamic_cast<HgiDXBuffer*>(copyOp.gpuSourceBuffer.Get());
   HgiDXBuffer* pBuffDst = dynamic_cast<HgiDXBuffer*>(copyOp.gpuDestinationBuffer.Get());

   if (nullptr != pBuffSrc && nullptr != pBuffDst)
   {
      pBuffDst->UpdateData(pBuffSrc, copyOp.byteSize, copyOp.sourceByteOffset, copyOp.destinationByteOffset);
   }
   else
      TF_WARN("At least one of the buffers is invalid. Cannot execute data copy.");
}

void HgiDXBlitCmds::CopyBufferCpuToGpu(HgiBufferCpuToGpuOp const& copyOp)
{
   HgiDXBuffer* pBuff = dynamic_cast<HgiDXBuffer*>(copyOp.gpuDestinationBuffer.Get());

   if (nullptr != pBuff)
   {
      pBuff->UpdateData(copyOp.cpuSourceBuffer, copyOp.byteSize, copyOp.sourceByteOffset, copyOp.destinationByteOffset);
   }
   else
      TF_WARN("Invalid buffer. Cannot execute data copy.");
}

void
HgiDXBlitCmds::CopyBufferGpuToCpu(HgiBufferGpuToCpuOp const& copyOp)
{
   // TODO: impl
   TF_WARN("CopyBufferGpuToCpu,  Not implemented yet.");
}

void
HgiDXBlitCmds::CopyTextureToBuffer(HgiTextureToBufferOp const& copyOp)
{
   // TODO: impl
   TF_WARN("CopyTextureToBuffer,  Not implemented yet.");
}

void
HgiDXBlitCmds::CopyBufferToTexture(HgiBufferToTextureOp const& copyOp)
{
   // TODO: impl
   TF_WARN("CopyBufferToTexture,  Not implemented yet.");
}

void
HgiDXBlitCmds::GenerateMipMaps(HgiTextureHandle const& texture)
{
   // TODO: impl
   TF_WARN("GenerateMipMaps,  Not implemented yet.");
}

void
HgiDXBlitCmds::FillBuffer(HgiBufferHandle const& buffer, uint8_t value)
{
   // TODO: impl
   TF_WARN("FillBuffer,  Not implemented yet.");
}

bool 
HgiDXBlitCmds::_Submit(Hgi* hgi, HgiSubmitWaitType wait)
{
   //
   // In a sense, "submit" happens naturally for DirectX when we send the queue to be executed

   //
   // We currently do not really need to do even this as long as we make sure we know which 
   // queue is responsible to what and we have proper resources barriers 
   // 
   // On the other hand, if the calling engine thinks we should submit and wait for the 
   // copy commands at this time, we should probably do that.
   // TODO: review

   _SetSubmitted();
   return true;
}

void
HgiDXBlitCmds::InsertMemoryBarrier(HgiMemoryBarrier barrier)
{
   //
   // If my understanding is correct,
   // DirectX does not have the concept of a global memory barrier
   // the barriers are per each resource
   // TODO: review

   TF_STATUS("Info: Blit commands memory barrier -> Submit Graphics Cmd List.");

   //
   // I will do the copies on the graphics queue - the copy queue cannot transition resources...
   // In the meantime I realized there are 2 compromize solutions / alternatives:
   // 1. do the resources transition on the graphics queue and the operations wherever they make sense
   // (although I am unsure what could be gained of that)
   // 2. I could revert resources back to their previous state after using them 
   // which would probably put them always in a neutral state from which probably any queue could transition them 
   // to a state that makes sense. The downside would be that in some cases, I would make more 
   // resources transitions than necessary (not sure how much of a problem that would be)
   // TODO: review
   // 
   //_hgi->GetPrimaryDevice()->SubmitCommandList(HgiDXDevice::eCommandType::kCopy);
   _hgi->GetPrimaryDevice()->SubmitCommandList(HgiDXDevice::eCommandType::kGraphics);
}


PXR_NAMESPACE_CLOSE_SCOPE
