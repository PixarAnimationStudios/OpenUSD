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
#include "pxr/base/arch/defines.h"

#include "pxr/imaging/hgiMetal/hgi.h"
#include "pxr/imaging/hgiMetal/buffer.h"
#include "pxr/imaging/hgiMetal/blitCmds.h"
#include "pxr/imaging/hgiMetal/computeCmds.h"
#include "pxr/imaging/hgiMetal/computePipeline.h"
#include "pxr/imaging/hgiMetal/capabilities.h"
#include "pxr/imaging/hgiMetal/conversions.h"
#include "pxr/imaging/hgiMetal/diagnostic.h"
#include "pxr/imaging/hgiMetal/graphicsCmds.h"
#include "pxr/imaging/hgiMetal/graphicsPipeline.h"
#include "pxr/imaging/hgiMetal/resourceBindings.h"
#include "pxr/imaging/hgiMetal/sampler.h"
#include "pxr/imaging/hgiMetal/shaderFunction.h"
#include "pxr/imaging/hgiMetal/shaderProgram.h"
#include "pxr/imaging/hgiMetal/texture.h"

#include "pxr/base/trace/trace.h"

#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType t = TfType::Define<HgiMetal, TfType::Bases<Hgi> >();
    t.SetFactory<HgiFactory<HgiMetal>>();
}

static int _GetAPIVersion()
{
    if (@available(macOS 10.15, ios 13.0, *)) {
        return APIVersion_Metal3_0;
    }
    if (@available(macOS 10.13, ios 11.0, *)) {
        return APIVersion_Metal2_0;
    }
    
    return APIVersion_Metal1_0;
}

HgiMetal::HgiMetal(id<MTLDevice> device)
: _device(device)
, _currentCmds(nullptr)
, _frameDepth(0)
, _apiVersion(_GetAPIVersion())
, _workToFlush(false)
{
    if (!_device) {
        if( TfGetenvBool("HGIMETAL_USE_INTEGRATED_GPU", false)) {
            _device = MTLCopyAllDevices()[1];
        }

        if (!_device) {
            _device = MTLCreateSystemDefaultDevice();
        }
    }
    
    static int const commandBufferPoolSize = 256;
    _commandQueue = [_device newCommandQueueWithMaxCommandBufferCount:
                     commandBufferPoolSize];
    _commandBuffer = [_commandQueue commandBuffer];
    [_commandBuffer retain];

    _capabilities.reset(
        new HgiMetalCapabilities(_device));

    HgiMetalSetupMetalDebug();
    
    _captureScopeFullFrame =
        [[MTLCaptureManager sharedCaptureManager]
            newCaptureScopeWithDevice:_device];
    _captureScopeFullFrame.label =
        [NSString stringWithFormat:@"Full Hydra Frame"];
    
    [[MTLCaptureManager sharedCaptureManager]
        setDefaultCaptureScope:_captureScopeFullFrame];
}

HgiMetal::~HgiMetal()
{
    [_commandBuffer commit];
    [_commandBuffer release];
    [_captureScopeFullFrame release];
    [_commandQueue release];
}

id<MTLDevice>
HgiMetal::GetPrimaryDevice() const
{
    return _device;
}

HgiGraphicsCmdsUniquePtr
HgiMetal::CreateGraphicsCmds(
    HgiGraphicsCmdsDesc const& desc)
{
    // XXX We should TF_CODING_ERROR here when there are no attachments, but
    // during the Hgi transition we allow it to render to global gl framebuffer.
    if (!desc.HasAttachments()) {
        // TF_CODING_ERROR("Graphics encoder desc has no attachments");
        return nullptr;
    }

    HgiMetalGraphicsCmds* encoder(
        new HgiMetalGraphicsCmds(this, desc));

    return HgiGraphicsCmdsUniquePtr(encoder);
}

HgiComputeCmdsUniquePtr
HgiMetal::CreateComputeCmds()
{
    HgiComputeCmds* computeCmds = new HgiMetalComputeCmds(this);
    if (!_currentCmds) {
        _currentCmds = computeCmds;
    }
    return HgiComputeCmdsUniquePtr(computeCmds);
}

HgiBlitCmdsUniquePtr
HgiMetal::CreateBlitCmds()
{
    HgiMetalBlitCmds* blitCmds = new HgiMetalBlitCmds(this);
    if (!_currentCmds) {
        _currentCmds = blitCmds;
    }
    return HgiBlitCmdsUniquePtr(blitCmds);
}

HgiTextureHandle
HgiMetal::CreateTexture(HgiTextureDesc const & desc)
{
    return HgiTextureHandle(new HgiMetalTexture(this, desc), GetUniqueId());
}

void
HgiMetal::DestroyTexture(HgiTextureHandle* texHandle)
{
    _TrashObject(texHandle);
}

HgiTextureViewHandle
HgiMetal::CreateTextureView(HgiTextureViewDesc const & desc)
{
    if (!desc.sourceTexture) {
        TF_CODING_ERROR("Source texture is null");
    }

    HgiTextureHandle src =
        HgiTextureHandle(new HgiMetalTexture(this, desc), GetUniqueId());
    HgiTextureView* view = new HgiTextureView(desc);
    view->SetViewTexture(src);
    return HgiTextureViewHandle(view, GetUniqueId());
}

void
HgiMetal::DestroyTextureView(HgiTextureViewHandle* viewHandle)
{
    // Trash the texture inside the view and invalidate the view handle.
    HgiTextureHandle texHandle = (*viewHandle)->GetViewTexture();
    _TrashObject(&texHandle);
    (*viewHandle)->SetViewTexture(HgiTextureHandle());
    delete viewHandle->Get();
    *viewHandle = HgiTextureViewHandle();
}

HgiSamplerHandle
HgiMetal::CreateSampler(HgiSamplerDesc const & desc)
{
    return HgiSamplerHandle(new HgiMetalSampler(this, desc), GetUniqueId());
}

void
HgiMetal::DestroySampler(HgiSamplerHandle* smpHandle)
{
    _TrashObject(smpHandle);
}

HgiBufferHandle
HgiMetal::CreateBuffer(HgiBufferDesc const & desc)
{
    return HgiBufferHandle(new HgiMetalBuffer(this, desc), GetUniqueId());
}

void
HgiMetal::DestroyBuffer(HgiBufferHandle* bufHandle)
{
    _TrashObject(bufHandle);
}

HgiShaderFunctionHandle
HgiMetal::CreateShaderFunction(HgiShaderFunctionDesc const& desc)
{
    return HgiShaderFunctionHandle(
        new HgiMetalShaderFunction(this, desc), GetUniqueId());
}

void
HgiMetal::DestroyShaderFunction(HgiShaderFunctionHandle* shaderFunctionHandle)
{
    _TrashObject(shaderFunctionHandle);
}

HgiShaderProgramHandle
HgiMetal::CreateShaderProgram(HgiShaderProgramDesc const& desc)
{
    return HgiShaderProgramHandle(
        new HgiMetalShaderProgram(desc), GetUniqueId());
}

void
HgiMetal::DestroyShaderProgram(HgiShaderProgramHandle* shaderProgramHandle)
{
    _TrashObject(shaderProgramHandle);
}


HgiResourceBindingsHandle
HgiMetal::CreateResourceBindings(HgiResourceBindingsDesc const& desc)
{
    return HgiResourceBindingsHandle(
        new HgiMetalResourceBindings(desc), GetUniqueId());
}

void
HgiMetal::DestroyResourceBindings(HgiResourceBindingsHandle* resHandle)
{
    _TrashObject(resHandle);
}

HgiGraphicsPipelineHandle
HgiMetal::CreateGraphicsPipeline(HgiGraphicsPipelineDesc const& desc)
{
    return HgiGraphicsPipelineHandle(
        new HgiMetalGraphicsPipeline(this, desc), GetUniqueId());
}

void
HgiMetal::DestroyGraphicsPipeline(HgiGraphicsPipelineHandle* pipeHandle)
{
    _TrashObject(pipeHandle);
}

HgiComputePipelineHandle
HgiMetal::CreateComputePipeline(HgiComputePipelineDesc const& desc)
{
    return HgiComputePipelineHandle(
        new HgiMetalComputePipeline(this, desc), GetUniqueId());
}

void
HgiMetal::DestroyComputePipeline(HgiComputePipelineHandle* pipeHandle)
{
    _TrashObject(pipeHandle);
}

TfToken const&
HgiMetal::GetAPIName() const {
    return HgiTokens->Metal;
}

void
HgiMetal::StartFrame()
{
    if (_frameDepth++ == 0) {
        [_captureScopeFullFrame beginScope];

        if ([[MTLCaptureManager sharedCaptureManager] isCapturing]) {
            // We need to grab a new command buffer otherwise the previous one
            // (if it was allocated at the end of the last frame) won't appear in
            // this frame's capture, and it will confuse us!
            CommitPrimaryCommandBuffer(CommitCommandBuffer_NoWait, true);
        }
    }
}

void
HgiMetal::EndFrame()
{
    if (--_frameDepth == 0) {
        [_captureScopeFullFrame endScope];
    }
}

id<MTLCommandQueue>
HgiMetal::GetQueue() const
{
    return _commandQueue;
}

id<MTLCommandBuffer>
HgiMetal::GetPrimaryCommandBuffer(bool flush)
{
    if (_workToFlush) {
        if (_currentCmds) {
            return nil;
        }
    }
    if (flush) {
        _workToFlush = true;
    }
    return _commandBuffer;
}

id<MTLCommandBuffer>
HgiMetal::GetSecondaryCommandBuffer()
{
    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    [commandBuffer retain];
    return commandBuffer;
}

int
HgiMetal::GetAPIVersion() const
{
    return _apiVersion;
}

HgiMetalCapabilities const &
HgiMetal::GetCapabilities() const
{
    return *_capabilities;
}

void
HgiMetal::CommitPrimaryCommandBuffer(CommitCommandBufferWaitType waitType,
                              bool forceNewBuffer)
{
    if (!_workToFlush && !forceNewBuffer) {
        return;
    }

    CommitSecondaryCommandBuffer(_commandBuffer, waitType);
    [_commandBuffer release];
    _commandBuffer = [_commandQueue commandBuffer];
    [_commandBuffer retain];

    _workToFlush = false;
}

void
HgiMetal::CommitSecondaryCommandBuffer(
    id<MTLCommandBuffer> commandBuffer,
    CommitCommandBufferWaitType waitType)
{
    [commandBuffer commit];
    if (waitType == CommitCommandBuffer_WaitUntilScheduled) {
        [commandBuffer waitUntilScheduled];
    }
    else if (waitType == CommitCommandBuffer_WaitUntilCompleted) {
        [commandBuffer waitUntilCompleted];
    }
}

void
HgiMetal::ReleaseSecondaryCommandBuffer(id<MTLCommandBuffer> commandBuffer)
{
    [commandBuffer release];
}

bool
HgiMetal::_SubmitCmds(HgiCmds* cmds, HgiSubmitWaitType wait)
{
    TRACE_FUNCTION();

    if (cmds) {
        _workToFlush = Hgi::_SubmitCmds(cmds, wait);
        if (cmds == _currentCmds) {
            _currentCmds = nullptr;
        }
    }

    return _workToFlush;
}

PXR_NAMESPACE_CLOSE_SCOPE
