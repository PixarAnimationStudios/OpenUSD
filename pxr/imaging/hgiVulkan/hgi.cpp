//
// Copyright 2020 Pixar
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
#include "pxr/imaging/hgiVulkan/blitCmds.h"
#include "pxr/imaging/hgiVulkan/buffer.h"
#include "pxr/imaging/hgiVulkan/commandQueue.h"
#include "pxr/imaging/hgiVulkan/computeCmds.h"
#include "pxr/imaging/hgiVulkan/computePipeline.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"
#include "pxr/imaging/hgiVulkan/garbageCollector.h"
#include "pxr/imaging/hgiVulkan/graphicsCmds.h"
#include "pxr/imaging/hgiVulkan/graphicsPipeline.h"
#include "pxr/imaging/hgiVulkan/hgi.h"
#include "pxr/imaging/hgiVulkan/instance.h"
#include "pxr/imaging/hgiVulkan/resourceBindings.h"
#include "pxr/imaging/hgiVulkan/sampler.h"
#include "pxr/imaging/hgiVulkan/shaderFunction.h"
#include "pxr/imaging/hgiVulkan/shaderProgram.h"
#include "pxr/imaging/hgiVulkan/texture.h"

#include "pxr/base/trace/trace.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    TfType t = TfType::Define<HgiVulkan, TfType::Bases<Hgi> >();
    t.SetFactory<HgiFactory<HgiVulkan>>();
}

HgiVulkan::HgiVulkan()
    : _instance(new HgiVulkanInstance())
    , _device(new HgiVulkanDevice(_instance))
    , _garbageCollector(new HgiVulkanGarbageCollector(this))
    , _threadId(std::this_thread::get_id())
    , _frameDepth(0)
{
}

HgiVulkan::~HgiVulkan()
{
    // Wait for all devices and perform final garbage collection.
    _device->WaitForIdle();
    _garbageCollector->PerformGarbageCollection(_device);
    delete _garbageCollector;
    delete _device;
    delete _instance;
}

/* Multi threaded */
HgiGraphicsCmdsUniquePtr
HgiVulkan::CreateGraphicsCmds(
    HgiGraphicsCmdsDesc const& desc)
{
    HgiVulkanGraphicsCmds* cmds(new HgiVulkanGraphicsCmds(this, desc));
    return HgiGraphicsCmdsUniquePtr(cmds);
}

/* Multi threaded */
HgiBlitCmdsUniquePtr
HgiVulkan::CreateBlitCmds()
{
    return HgiBlitCmdsUniquePtr(new HgiVulkanBlitCmds(this));
}

HgiComputeCmdsUniquePtr
HgiVulkan::CreateComputeCmds()
{
    HgiVulkanComputeCmds* cmds(new HgiVulkanComputeCmds(this));
    return HgiComputeCmdsUniquePtr(cmds);
}

/* Multi threaded */
HgiTextureHandle
HgiVulkan::CreateTexture(HgiTextureDesc const & desc)
{
    return HgiTextureHandle(
        new HgiVulkanTexture(this, GetPrimaryDevice(), desc),
        GetUniqueId());
}

/* Multi threaded */
void
HgiVulkan::DestroyTexture(HgiTextureHandle* texHandle)
{
    TrashObject(texHandle, GetGarbageCollector()->GetTextureList());
}

/* Multi threaded */
HgiTextureViewHandle
HgiVulkan::CreateTextureView(HgiTextureViewDesc const & desc)
{
    if (!desc.sourceTexture) {
        TF_CODING_ERROR("Source texture is null");
    }

    HgiTextureHandle src = HgiTextureHandle(
        new HgiVulkanTexture(this, GetPrimaryDevice(),desc), GetUniqueId());
    HgiTextureView* view = new HgiTextureView(desc);
    view->SetViewTexture(src);
    return HgiTextureViewHandle(view, GetUniqueId());
}

void
HgiVulkan::DestroyTextureView(HgiTextureViewHandle* viewHandle)
{
    // Trash the texture inside the view and invalidate the view handle.
    HgiTextureHandle texHandle = (*viewHandle)->GetViewTexture();
    TrashObject(&texHandle, GetGarbageCollector()->GetTextureList());
    (*viewHandle)->SetViewTexture(HgiTextureHandle());
    delete viewHandle->Get();
    *viewHandle = HgiTextureViewHandle();
}

/* Multi threaded */
HgiSamplerHandle
HgiVulkan::CreateSampler(HgiSamplerDesc const & desc)
{
    return HgiSamplerHandle(
        new HgiVulkanSampler(GetPrimaryDevice(), desc),
        GetUniqueId());
}

/* Multi threaded */
void
HgiVulkan::DestroySampler(HgiSamplerHandle* smpHandle)
{
    TrashObject(smpHandle, GetGarbageCollector()->GetSamplerList());
}

/* Multi threaded */
HgiBufferHandle
HgiVulkan::CreateBuffer(HgiBufferDesc const & desc)
{
    return HgiBufferHandle(
        new HgiVulkanBuffer(this, GetPrimaryDevice(), desc),
        GetUniqueId());
}

/* Multi threaded */
void
HgiVulkan::DestroyBuffer(HgiBufferHandle* bufHandle)
{
    TrashObject(bufHandle, GetGarbageCollector()->GetBufferList());
}

/* Multi threaded */
HgiShaderFunctionHandle
HgiVulkan::CreateShaderFunction(HgiShaderFunctionDesc const& desc)
{
    return HgiShaderFunctionHandle(
        new HgiVulkanShaderFunction(GetPrimaryDevice(), desc),
        GetUniqueId());
}

/* Multi threaded */
void
HgiVulkan::DestroyShaderFunction(HgiShaderFunctionHandle* shaderFnHandle)
{
    TrashObject(shaderFnHandle, GetGarbageCollector()->GetShaderFunctionList());
}

/* Multi threaded */
HgiShaderProgramHandle
HgiVulkan::CreateShaderProgram(HgiShaderProgramDesc const& desc)
{
    return HgiShaderProgramHandle(
        new HgiVulkanShaderProgram(GetPrimaryDevice(), desc),
        GetUniqueId());
}

/* Multi threaded */
void
HgiVulkan::DestroyShaderProgram(HgiShaderProgramHandle* shaderPrgHandle)
{
    TrashObject(shaderPrgHandle, GetGarbageCollector()->GetShaderProgramList());
}

/* Multi threaded */
HgiResourceBindingsHandle
HgiVulkan::CreateResourceBindings(HgiResourceBindingsDesc const& desc)
{
    return HgiResourceBindingsHandle(
        new HgiVulkanResourceBindings(GetPrimaryDevice(), desc),
        GetUniqueId());
}

/* Multi threaded */
void
HgiVulkan::DestroyResourceBindings(HgiResourceBindingsHandle* resHandle)
{
    TrashObject(resHandle, GetGarbageCollector()->GetResourceBindingsList());
}

HgiGraphicsPipelineHandle
HgiVulkan::CreateGraphicsPipeline(HgiGraphicsPipelineDesc const& desc)
{
    return HgiGraphicsPipelineHandle(
        new HgiVulkanGraphicsPipeline(GetPrimaryDevice(), desc),
        GetUniqueId());
}

void
HgiVulkan::DestroyGraphicsPipeline(HgiGraphicsPipelineHandle* pipeHandle)
{
    TrashObject(pipeHandle, GetGarbageCollector()->GetGraphicsPipelineList());
}

HgiComputePipelineHandle
HgiVulkan::CreateComputePipeline(HgiComputePipelineDesc const& desc)
{
    return HgiComputePipelineHandle(
        new HgiVulkanComputePipeline(GetPrimaryDevice(), desc),
        GetUniqueId());
}

void
HgiVulkan::DestroyComputePipeline(HgiComputePipelineHandle* pipeHandle)
{
    TrashObject(pipeHandle, GetGarbageCollector()->GetComputePipelineList());
}

/* Multi threaded */
TfToken const&
HgiVulkan::GetAPIName() const {
    return HgiTokens->Vulkan;
}

/* Single threaded */
void
HgiVulkan::StartFrame()
{
    // Please read important usage limitations for Hgi::StartFrame

    if (_frameDepth++ == 0) {
        HgiVulkanBeginQueueLabel(GetPrimaryDevice(), "Full Hydra Frame");
    }
}

/* Single threaded */
void
HgiVulkan::EndFrame()
{
    // Please read important usage limitations for Hgi::EndFrame

    if (--_frameDepth == 0) {
        _EndFrameSync();
        HgiVulkanEndQueueLabel(GetPrimaryDevice());
    }
}

/* Multi threaded */
HgiVulkanInstance*
HgiVulkan::GetVulkanInstance() const
{
    return _instance;
}

/* Multi threaded */
HgiVulkanDevice*
HgiVulkan::GetPrimaryDevice() const
{
    return _device;
}

/* Multi threaded */
HgiVulkanGarbageCollector*
HgiVulkan::GetGarbageCollector() const
{
    return _garbageCollector;
}

/* Single threaded */
bool
HgiVulkan::_SubmitCmds(HgiCmds* cmds, HgiSubmitWaitType wait)
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
    if (_frameDepth==0) {
        _EndFrameSync();
    }

    return result;
}

/* Single threaded */
void
HgiVulkan::_EndFrameSync()
{
    // The garbage collector and command buffer reset must happen on the
    // main-thread when no threads are recording.
    if (ARCH_UNLIKELY(_threadId != std::this_thread::get_id())) {
        TF_CODING_ERROR("Secondary thread violation");
        return;
    }

    HgiVulkanDevice* device = GetPrimaryDevice();
    HgiVulkanCommandQueue* queue = device->GetCommandQueue();

    // Reset command buffers for each device's queue.
    queue->ResetConsumedCommandBuffers();

    // Perform garbage collection for each device.
    _garbageCollector->PerformGarbageCollection(device);
}


PXR_NAMESPACE_CLOSE_SCOPE
