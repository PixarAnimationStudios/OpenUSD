
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
#include "pxr/imaging/hgiDX/capabilities.h"
#include "pxr/imaging/hgiDX/computeCmds.h"
#include "pxr/imaging/hgiDX/computePipeline.h"
#include "pxr/imaging/hgiDX/device.h"
#include "pxr/imaging/hgiDX/graphicsCmds.h"
#include "pxr/imaging/hgiDX/graphicsPipeline.h"
#include "pxr/imaging/hgiDX/hgi.h"
#include "pxr/imaging/hgiDX/indirectCommandEncoder.h"
#include "pxr/imaging/hgiDX/memoryHelper.h"
#include "pxr/imaging/hgiDX/presentation.h"
#include "pxr/imaging/hgiDX/resourceBindings.h"
#include "pxr/imaging/hgiDX/sampler.h"
#include "pxr/imaging/hgiDX/shaderFunction.h"
#include "pxr/imaging/hgiDX/shaderProgram.h"
#include "pxr/imaging/hgiDX/texture.h"
#include "pxr/imaging/hgiDX/textureConverter.h"

#include "pxr/base/trace/trace.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
   TfType t = TfType::Define<HgiDX, TfType::Bases<Hgi> >();
   t.SetFactory<HgiFactory<HgiDX>>();
}

HgiDX::HgiDX()
   : _threadId(std::this_thread::get_id())
   , _frameDepth(0)
{
   _device = std::make_unique<HgiDXDevice>();
   _txConverter.reset(new HgiDXTextureConverter(this));
   _presentation.reset(new HgiDXPresentation(_device.get(), _txConverter.get()));
   _indirectEncoder.reset(new HgiDXIndirectCommandEncoder(this));
   _memHelper.reset(new HgiDXMemoryHelper());
   
}

HgiDX::~HgiDX()
{
   // Wait for all devices and perform final garbage collection or whatever...
   _device->WaitForIdle();
}

template<class THandle, class TDXObj>
void
ThrashObject(THandle* pDXObjectHandle)
{
   if (nullptr != pDXObjectHandle)
   {
      TDXObj* pDXObj = dynamic_cast<TDXObj*>(pDXObjectHandle->Get());

      //
      // let's keep things simple for now
      delete pDXObj;

      //
      // inspired from the vulkan implementation, 
      // makes sense to cleanup the proxy
      *pDXObjectHandle = THandle();
   }
}

bool
HgiDX::IsBackendSupported() const
{
   return true;
}

/* Multi threaded */
HgiGraphicsCmdsUniquePtr
HgiDX::CreateGraphicsCmds(HgiGraphicsCmdsDesc const& desc)
{
   HgiGraphicsCmdsUniquePtr ret;
   ret.reset(new HgiDXGraphicsCmds(this, desc));
   return ret;
}

/* Multi threaded */
HgiBlitCmdsUniquePtr
HgiDX::CreateBlitCmds()
{
   HgiBlitCmdsUniquePtr ret;
   ret.reset(new HgiDXBlitCmds(this));
   return ret;
}

HgiComputeCmdsUniquePtr
HgiDX::CreateComputeCmds(HgiComputeCmdsDesc const& desc)
{
   HgiComputeCmdsUniquePtr ret;
   ret.reset(new HgiDXComputeCmds(this, desc));
   return ret;
}

/* Multi threaded */
HgiTextureHandle
HgiDX::CreateTexture(HgiTextureDesc const& desc)
{
   uint64_t nUniqueId = GetUniqueId();
   return HgiTextureHandle(new HgiDXTexture(this, GetPrimaryDevice(), desc), nUniqueId);
}

/* Multi threaded */
void
HgiDX::DestroyTexture(HgiTextureHandle* pTexHandle)
{
   ThrashObject<HgiTextureHandle, HgiDXTexture>(pTexHandle);
}

/* Multi threaded */
HgiTextureViewHandle
HgiDX::CreateTextureView(HgiTextureViewDesc const& desc)
{
   if (!desc.sourceTexture) {
      TF_CODING_ERROR("Source texture is null");
   }

   uint64_t nTxUniqueId = GetUniqueId();
   HgiTextureHandle src = HgiTextureHandle(new HgiDXTexture(this, GetPrimaryDevice(), desc), nTxUniqueId);
   HgiTextureView* view = new HgiTextureView(desc);
   view->SetViewTexture(src);
   uint64_t nTVUniqueId = GetUniqueId();
   return HgiTextureViewHandle(view, nTVUniqueId);
}

void
HgiDX::DestroyTextureView(HgiTextureViewHandle* pViewHandle)
{
   //
   // this is a bit more complex, will not use the template for it
   if (nullptr != pViewHandle)
   {
      HgiTextureView* pTV = pViewHandle->Get();

      HgiDXTexture* pDXTex = dynamic_cast<HgiDXTexture*>(pTV->GetViewTexture().Get());

      //
      // let's keep things simple for now
      delete pDXTex;
      pTV->SetViewTexture(HgiTextureHandle());

      delete pTV;
      *pViewHandle = HgiTextureViewHandle();
   }
}

/* Multi threaded */
HgiSamplerHandle
HgiDX::CreateSampler(HgiSamplerDesc const& desc)
{
   uint64_t nUniqueId = GetUniqueId();
   return HgiSamplerHandle(new HgiDXSampler(GetPrimaryDevice(), desc), nUniqueId);
}

/* Multi threaded */
void
HgiDX::DestroySampler(HgiSamplerHandle* pSmpHandle)
{
   ThrashObject<HgiSamplerHandle, HgiDXSampler>(pSmpHandle);
}

/* Multi threaded */
HgiBufferHandle
HgiDX::CreateBuffer(HgiBufferDesc const& desc)
{
   uint64_t nUniqueId = GetUniqueId();
   return HgiBufferHandle(new HgiDXBuffer(GetPrimaryDevice(), desc), nUniqueId);
}

/* Multi threaded */
void
HgiDX::DestroyBuffer(HgiBufferHandle* pBufHandle)
{
   ThrashObject<HgiBufferHandle, HgiDXBuffer>(pBufHandle);
}

/* Multi threaded */
HgiShaderFunctionHandle
HgiDX::CreateShaderFunction(HgiShaderFunctionDesc const& desc)
{
   uint64_t nUniqueId = GetUniqueId();
   return HgiShaderFunctionHandle(
      new HgiDXShaderFunction(
         GetPrimaryDevice(), this, desc, GetCapabilities()->GetShaderVersion()), nUniqueId);
}

/* Multi threaded */
void
HgiDX::DestroyShaderFunction(HgiShaderFunctionHandle* pShaderFnHandle)
{
   ThrashObject<HgiShaderFunctionHandle, HgiDXShaderFunction>(pShaderFnHandle);
}

/* Multi threaded */
HgiShaderProgramHandle
HgiDX::CreateShaderProgram(HgiShaderProgramDesc const& desc)
{
   uint64_t nUniqueId = GetUniqueId();
   return HgiShaderProgramHandle(new HgiDXShaderProgram(GetPrimaryDevice(), desc), nUniqueId);
}

/* Multi threaded */
void
HgiDX::DestroyShaderProgram(HgiShaderProgramHandle* pShaderPrgHandle)
{
   ThrashObject<HgiShaderProgramHandle, HgiDXShaderProgram>(pShaderPrgHandle);
}

/* Multi threaded */
HgiResourceBindingsHandle
HgiDX::CreateResourceBindings(HgiResourceBindingsDesc const& desc)
{
   uint64_t nUniqueId = GetUniqueId();
   return HgiResourceBindingsHandle(new HgiDXResourceBindings(GetPrimaryDevice(), desc), nUniqueId);
}

/* Multi threaded */
void
HgiDX::DestroyResourceBindings(HgiResourceBindingsHandle* pResHandle)
{
   ThrashObject<HgiResourceBindingsHandle, HgiDXResourceBindings>(pResHandle);
}

HgiGraphicsPipelineHandle
HgiDX::CreateGraphicsPipeline(HgiGraphicsPipelineDesc const& desc)
{
   uint64_t nUniqueId = GetUniqueId();
   return HgiGraphicsPipelineHandle(new HgiDXGraphicsPipeline(GetPrimaryDevice(), desc), nUniqueId);
}

void
HgiDX::DestroyGraphicsPipeline(HgiGraphicsPipelineHandle* pPipeHandle)
{
   ThrashObject<HgiGraphicsPipelineHandle, HgiDXGraphicsPipeline>(pPipeHandle);
}

HgiComputePipelineHandle
HgiDX::CreateComputePipeline(HgiComputePipelineDesc const& desc)
{
   uint64_t nUniqueId = GetUniqueId();
   return HgiComputePipelineHandle(new HgiDXComputePipeline(GetPrimaryDevice(), desc), nUniqueId);
}

void
HgiDX::DestroyComputePipeline(HgiComputePipelineHandle* pPipeHandle)
{
   ThrashObject<HgiComputePipelineHandle, HgiDXComputePipeline>(pPipeHandle);
}

/* Multi threaded */
TfToken const&
HgiDX::GetAPIName() const {
   return HgiTokens->DirectX;
}

/* Multi threaded */
HgiCapabilities const*
HgiDX::GetCapabilities() const
{
   return &_device->GetDeviceCapabilities();
}

/* Single threaded */
void
HgiDX::StartFrame()
{
   // Please read important usage limitations for Hgi::StartFrame

   //
   // TODO: do I want to do something here?
   if (_frameDepth++ == 0) {
      //HgiDXBeginQueueLabel(GetPrimaryDevice(), "Full Hydra Frame");
   }
}

/* Single threaded */
void
HgiDX::EndFrame()
{
   // Please read important usage limitations for Hgi::EndFrame

   //
   // TODO: do I want to do something here?
   if (--_frameDepth == 0) {
      _EndFrameSync();
      //HgiDXEndQueueLabel(GetPrimaryDevice());
   }
}

/* Multi threaded */
HgiDXDevice*
HgiDX::GetPrimaryDevice() const
{
   return _device.get();
}

/* Single threaded */
bool
HgiDX::_SubmitCmds(HgiCmds* cmds, HgiSubmitWaitType wait)
{
   TRACE_FUNCTION();

   // XXX The device queue is externally synchronized so we would at minimum
   // need a mutex here to ensure only one thread submits cmds at a time.
   // However, since we currently call garbage collection here and because
   // we only have one resource command buffer, we cannot support submitting
   // cmds from secondary threads until those issues are resolved.
   if (ARCH_UNLIKELY(_threadId != std::this_thread::get_id())) {
      TF_CODING_ERROR("Secondary threads should not submit cmds");
      return false;
   }

   // Submit Cmds work
   bool result = false;
   if (cmds) {
      result = Hgi::_SubmitCmds(cmds, wait);
   }

   // XXX If client does not call StartFrame / EndFrame we perform end of frame
   // cleanup after each SubmitCmds. This is more frequent than ideal and also
   // prevents us from making SubmitCmds thread-safe.
   if (_frameDepth == 0) {
      _EndFrameSync();
   }

   return result;
}

/* Single threaded */
void
HgiDX::_EndFrameSync()
{
   // The garbage collector and command buffer reset must happen on the
   // main-thread when no threads are recording.
   if (ARCH_UNLIKELY(_threadId != std::this_thread::get_id())) {
      TF_CODING_ERROR("Secondary thread violation");
      return;
   }

   //
   // TODO: should I do somthing here? 
   // If not let's clean it up...
}

HgiIndirectCommandEncoder* 
HgiDX::GetIndirectCommandEncoder() const
{
   return _indirectEncoder.get();
}

HgiDXPresentation*
HgiDX::GetPresentation()
{
   return _presentation.get();
}

HgiCustomInterop* 
HgiDX::GetCustomInterop()
{
   return _presentation.get();
}

HgiMemoryHelper* 
HgiDX::GetMemoryHelper()
{
   return _memHelper.get();
}

HgiDXTextureConverter*
HgiDX::GetTxConverter()
{
   return _txConverter.get();
}

PXR_NAMESPACE_CLOSE_SCOPE
