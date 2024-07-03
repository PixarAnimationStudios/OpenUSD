//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiMetal/buffer.h"
#include "pxr/imaging/hgiMetal/computeCmds.h"
#include "pxr/imaging/hgiMetal/computePipeline.h"
#include "pxr/imaging/hgiMetal/conversions.h"
#include "pxr/imaging/hgiMetal/diagnostic.h"
#include "pxr/imaging/hgiMetal/hgi.h"
#include "pxr/imaging/hgiMetal/resourceBindings.h"
#include "pxr/imaging/hgiMetal/texture.h"

#include "pxr/base/arch/defines.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiMetalComputeCmds::HgiMetalComputeCmds(
    HgiMetal* hgi,
    HgiComputeCmdsDesc const& desc)
    : HgiComputeCmds()
    , _hgi(hgi)
    , _pipelineState(nullptr)
    , _commandBuffer(nil)
    , _argumentBuffer(nil)
    , _encoder(nil)
    , _secondaryCommandBuffer(false)
    , _dispatchMethod(desc.dispatchMethod)
{
    _CreateEncoder();
}

HgiMetalComputeCmds::~HgiMetalComputeCmds()
{
    TF_VERIFY(_encoder == nil, "Encoder created, but never commited.");
}

void
HgiMetalComputeCmds::_CreateEncoder()
{
    if (!_encoder) {
        _commandBuffer = _hgi->GetPrimaryCommandBuffer(this, false);
        if (_commandBuffer == nil) {
            _commandBuffer = _hgi->GetSecondaryCommandBuffer();
            _secondaryCommandBuffer = true;
        }
        MTLDispatchType dispatchType =
            (_dispatchMethod == HgiComputeDispatchConcurrent)
                ? MTLDispatchTypeConcurrent
                : MTLDispatchTypeSerial;
        _encoder = [_commandBuffer
                        computeCommandEncoderWithDispatchType:dispatchType];
    }
}

void
HgiMetalComputeCmds::_CreateArgumentBuffer()
{
    if (!_argumentBuffer) {
        _argumentBuffer = _hgi->GetArgBuffer();
    }
}

void
HgiMetalComputeCmds::BindPipeline(HgiComputePipelineHandle pipeline)
{
    _CreateEncoder();
    _pipelineState = static_cast<HgiMetalComputePipeline*>(pipeline.Get());
    _pipelineState->BindPipeline(_encoder);
}

void
HgiMetalComputeCmds::BindResources(HgiResourceBindingsHandle r)
{
    if (HgiMetalResourceBindings* rb=
        static_cast<HgiMetalResourceBindings*>(r.Get()))
    {
        _CreateEncoder();
        _CreateArgumentBuffer();

        rb->BindResources(_hgi, _encoder, _argumentBuffer);
    }
}

void
HgiMetalComputeCmds::SetConstantValues(
    HgiComputePipelineHandle pipeline,
    uint32_t bindIndex,
    uint32_t byteSize,
    const void* data)
{
    _CreateEncoder();
    _CreateArgumentBuffer();

    HgiMetalResourceBindings::SetConstantValues(
        _argumentBuffer, HgiShaderStageCompute, bindIndex, byteSize, data);
}

void
HgiMetalComputeCmds::Dispatch(int dimX, int dimY)
{
    if (dimX == 0 || dimY == 0) {
        return;
    }

    uint32_t maxTotalThreads =
        [_pipelineState->GetMetalPipelineState() maxTotalThreadsPerThreadgroup];
    uint32_t exeWidth =
        [_pipelineState->GetMetalPipelineState() threadExecutionWidth];

    uint32_t thread_width, thread_height;
    thread_width = MIN(maxTotalThreads, exeWidth);
    if (dimY == 1) {
        thread_height = 1;
    }
    else {
        thread_width = exeWidth;
        thread_height = maxTotalThreads / thread_width;
    }

    if (_argumentBuffer.storageMode != MTLStorageModeShared &&
        [_argumentBuffer respondsToSelector:@selector(didModifyRange:)]) {
        NSRange range = NSMakeRange(0, _argumentBuffer.length);

        ARCH_PRAGMA_PUSH
        ARCH_PRAGMA_INSTANCE_METHOD_NOT_FOUND
        [_argumentBuffer didModifyRange:range];
        ARCH_PRAGMA_POP
    }

    [_encoder dispatchThreads:MTLSizeMake(dimX, dimY, 1)
        threadsPerThreadgroup:MTLSizeMake(MIN(thread_width, dimX),
                                          MIN(thread_height, dimY), 1)];

    if (!_secondaryCommandBuffer) {
        _hgi->SetHasWork();
    }

    _argumentBuffer = nil;
}

void
HgiMetalComputeCmds::PushDebugGroup(const char* label)
{
    _CreateEncoder();
    HGIMETAL_DEBUG_PUSH_GROUP(_encoder, label)
}

void
HgiMetalComputeCmds::PopDebugGroup()
{
    if (_encoder) {
        HGIMETAL_DEBUG_POP_GROUP(_encoder)
    }
}

void
HgiMetalComputeCmds::InsertMemoryBarrier(HgiMemoryBarrier barrier)
{
    if (TF_VERIFY(barrier == HgiMemoryBarrierAll)) {
        _CreateEncoder();
        MTLBarrierScope scope = MTLBarrierScopeBuffers|
                                MTLBarrierScopeTextures;
        [_encoder memoryBarrierWithScope:scope];
    }
}

HgiComputeDispatch
HgiMetalComputeCmds::GetDispatchMethod() const
{
    return _dispatchMethod;
}

bool
HgiMetalComputeCmds::_Submit(Hgi* hgi, HgiSubmitWaitType wait)
{
    if (_encoder) {
        [_encoder endEncoding];
        _encoder = nil;

        HgiMetal::CommitCommandBufferWaitType waitType;
        switch(wait) {
            case HgiSubmitWaitTypeNoWait:
                waitType = HgiMetal::CommitCommandBuffer_NoWait;
                break;
            case HgiSubmitWaitTypeWaitUntilCompleted:
                waitType = HgiMetal::CommitCommandBuffer_WaitUntilCompleted;
                break;
        }

        if (_secondaryCommandBuffer) {
            _hgi->CommitSecondaryCommandBuffer(_commandBuffer, waitType);
        }
        else {
            _hgi->CommitPrimaryCommandBuffer(waitType);
        }
    }
    
    if (_secondaryCommandBuffer) {
        _hgi->ReleaseSecondaryCommandBuffer(_commandBuffer);
    }
    _commandBuffer = nil;
    _argumentBuffer = nil;

    return true;
}

HGIMETAL_API
id<MTLComputeCommandEncoder> HgiMetalComputeCmds::GetEncoder()
{
    _CreateEncoder();
    return _encoder;
}

PXR_NAMESPACE_CLOSE_SCOPE
