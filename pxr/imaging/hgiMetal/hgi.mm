//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/arch/defines.h"

#include "pxr/imaging/hgi/debugCodes.h"
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

struct HgiMetal::AutoReleasePool
{
#if !__has_feature(objc_arc)
    NSAutoreleasePool* _pool = nil;
    ~AutoReleasePool() {
        Drain();
    }

    void Init() {
        _pool = [[NSAutoreleasePool alloc] init];
    }

    void Drain() {
        if (_pool) {
            [_pool drain];
            _pool = nil;
        }
    }
#else
    void Init() {}
    void Drain() {}
#endif
};

HgiMetal::HgiMetal(id<MTLDevice> device)
: _device(device)
, _currentCmds(nullptr)
, _frameDepth(0)
, _workToFlush(false)
, _pool(std::make_unique<AutoReleasePool>())
{
    if (!_device) {
#if defined(ARCH_OS_OSX)
        if( TfGetenvBool("HGIMETAL_USE_INTEGRATED_GPU", false)) {
            auto devices = MTLCopyAllDevices();
            for (id<MTLDevice> d in devices) {
                if ([d isLowPower]) {
                    _device = d;
                    break;
                }
            }
        }
#endif
        if (!_device) {
            _device = MTLCreateSystemDefaultDevice();
        }
    }

    static int const commandBufferPoolSize = 256;

    _commandQueue = [_device newCommandQueueWithMaxCommandBufferCount:
                     commandBufferPoolSize];
    _commandBuffer = [_commandQueue commandBuffer];
    [_commandBuffer retain];

    _capabilities.reset(new HgiMetalCapabilities(_device));
    _indirectCommandEncoder.reset(new HgiMetalIndirectCommandEncoder(this));

    MTLArgumentDescriptor *argumentDescBuffer =
        [[MTLArgumentDescriptor alloc] init];
    argumentDescBuffer.dataType = MTLDataTypePointer;
    _argEncoderBuffer = [_device newArgumentEncoderWithArguments:
                        @[argumentDescBuffer]];
    [argumentDescBuffer release];

    MTLArgumentDescriptor *argumentDescSampler =
        [[MTLArgumentDescriptor alloc] init];
    argumentDescSampler.dataType = MTLDataTypeSampler;
    _argEncoderSampler = [_device newArgumentEncoderWithArguments:
                         @[argumentDescSampler]];
    [argumentDescSampler release];

    MTLArgumentDescriptor *argumentDescTexture =
        [[MTLArgumentDescriptor alloc] init];
    argumentDescTexture.dataType = MTLDataTypeTexture;
    _argEncoderTexture = [_device newArgumentEncoderWithArguments:
                         @[argumentDescTexture]];
    [argumentDescTexture release];

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
    [_commandBuffer waitUntilCompleted];
    [_commandBuffer release];
    [_captureScopeFullFrame release];
    [_commandQueue release];
    [_argEncoderBuffer release];
    [_argEncoderSampler release];
    [_argEncoderTexture release];
    
    {
        std::lock_guard<std::mutex> lock(_freeArgMutex);
        while(_freeArgBuffers.size()) {
            [_freeArgBuffers.top() release];
            _freeArgBuffers.pop();
        }
    }
}

bool
HgiMetal::IsBackendSupported() const
{
    // Want Metal 2.0 and Metal Shading Language 2.2 or higher.
    if (@available(macOS 10.15, ios 13.0, *)) {
        return true;
    }

    TF_DEBUG(HGI_DEBUG_IS_SUPPORTED).Msg(
        "HgiMetal unsupported due to OS version\n");

    return false;
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
    HgiMetalGraphicsCmds* gfxCmds(new HgiMetalGraphicsCmds(this, desc));
    return HgiGraphicsCmdsUniquePtr(gfxCmds);
}

HgiComputeCmdsUniquePtr
HgiMetal::CreateComputeCmds(
    HgiComputeCmdsDesc const& desc)
{
    HgiComputeCmds* computeCmds = new HgiMetalComputeCmds(this, desc);
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

    if (_workToFlush) {
        [_commandBuffer
         addCompletedHandler:[texHandle](id<MTLCommandBuffer> cmdBuffer)
         {
            delete texHandle.Get();
        }];
    } else {
        _TrashObject(&texHandle);
    }
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

HgiMetalCapabilities const*
HgiMetal::GetCapabilities() const
{
    return _capabilities.get();
}

HgiMetalIndirectCommandEncoder*
HgiMetal::GetIndirectCommandEncoder() const
{
    return _indirectCommandEncoder.get();
}

void
HgiMetal::StartFrame()
{
    _pool->Init();

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

    _pool->Drain();
}

id<MTLCommandQueue>
HgiMetal::GetQueue() const
{
    return _commandQueue;
}

id<MTLCommandBuffer>
HgiMetal::GetPrimaryCommandBuffer(HgiCmds *requester, bool flush)
{
    if (_workToFlush) {
        if (_currentCmds && requester != _currentCmds) {
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

void
HgiMetal::SetHasWork()
{
    _workToFlush = true;
}

int
HgiMetal::GetAPIVersion() const
{
    return GetCapabilities()->GetAPIVersion();
}

void
HgiMetal::CommitPrimaryCommandBuffer(
    CommitCommandBufferWaitType waitType,
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
    // If there are active arg buffers on this command buffer, add a callback
    // to release them back to the free pool.
    if (!_activeArgBuffers.empty()) {
        _ActiveArgBuffers argBuffersToFree;
        argBuffersToFree.swap(_activeArgBuffers);

        [_commandBuffer
         addCompletedHandler:^(id<MTLCommandBuffer> cmdBuffer)
         {
            std::lock_guard<std::mutex> lock(_freeArgMutex);
            for (id<MTLBuffer> argBuffer : argBuffersToFree) {
                _freeArgBuffers.push(argBuffer);
            }
         }];
    }

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

id<MTLArgumentEncoder>
HgiMetal::GetBufferArgumentEncoder() const
{
    return _argEncoderBuffer;
}

id<MTLArgumentEncoder>
HgiMetal::GetSamplerArgumentEncoder() const
{
    return _argEncoderSampler;
}

id<MTLArgumentEncoder>
HgiMetal::GetTextureArgumentEncoder() const
{
    return _argEncoderTexture;
}

id<MTLBuffer>
HgiMetal::GetArgBuffer()
{
    MTLResourceOptions options = _capabilities->defaultStorageMode;
    id<MTLBuffer> buffer;

    {
        std::lock_guard<std::mutex> lock(_freeArgMutex);
        if (_freeArgBuffers.empty()) {
            buffer = [_device newBufferWithLength:HgiMetalArgumentOffsetSize
                                          options:options];
        }
        else {
            buffer = _freeArgBuffers.top();
            _freeArgBuffers.pop();
            memset(buffer.contents, 0x00, buffer.length);
        }
    }

    if (!_commandBuffer) {
        TF_CODING_ERROR("_commandBuffer is null");
    }

    // Keep track of active arg buffers to reuse after command buffer commit.
    _activeArgBuffers.push_back(buffer);

    return buffer;
}

bool
HgiMetal::_SubmitCmds(HgiCmds* cmds, HgiSubmitWaitType wait)
{
    TRACE_FUNCTION();

    if (cmds) {
        Hgi::_SubmitCmds(cmds, wait);
        if (cmds == _currentCmds) {
            _currentCmds = nullptr;
        }
    }

    return _workToFlush;
}

PXR_NAMESPACE_CLOSE_SCOPE
