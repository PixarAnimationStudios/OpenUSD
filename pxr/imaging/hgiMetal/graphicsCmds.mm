//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/imaging/hgiMetal/buffer.h"
#include "pxr/imaging/hgiMetal/conversions.h"
#include "pxr/imaging/hgiMetal/diagnostic.h"
#include "pxr/imaging/hgiMetal/graphicsCmds.h"
#include "pxr/imaging/hgiMetal/graphicsPipeline.h"
#include "pxr/imaging/hgiMetal/hgi.h"
#include "pxr/imaging/hgiMetal/indirectCommandEncoder.h"
#include "pxr/imaging/hgiMetal/resourceBindings.h"
#include "pxr/imaging/hgiMetal/texture.h"

#include "pxr/base/work/dispatcher.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/withScopedParallelism.h"

#include "pxr/base/arch/defines.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiMetalGraphicsCmds::CachedEncoderState::CachedEncoderState()
{
    ResetCachedEncoderState();
}

void
HgiMetalGraphicsCmds::CachedEncoderState::ResetCachedEncoderState()
{
    vertexBindings.clear();

    resourceBindings = nil;
    graphicsPipeline = nil;
    argumentBuffer = nil;
}

static void
_SetVertexBindings(id<MTLRenderCommandEncoder> encoder,
                   HgiVertexBufferBindingVector const &bindings)
{
    for (HgiVertexBufferBinding const binding : bindings)
    {
        if (binding.buffer) {
            HgiMetalBuffer* mtlBuffer =
                static_cast<HgiMetalBuffer*>(binding.buffer.Get());

            [encoder setVertexBuffer:mtlBuffer->GetBufferId()
                              offset:binding.byteOffset
                             atIndex:binding.index];
        }
    }
}

HgiMetalGraphicsCmds::HgiMetalGraphicsCmds(
    HgiMetal* hgi,
    HgiGraphicsCmdsDesc const& desc)
    : HgiGraphicsCmds()
    , _hgi(hgi)
    , _renderPassDescriptor(nil)
    , _parallelEncoder(nil)
    , _argumentBuffer(nil)
    , _descriptor(desc)
    , _primitiveType(HgiPrimitiveTypeTriangleList)
    , _primitiveIndexSize(0)
    , _drawBufferBindingIndex(0)
    , _debugLabel(nil)
    , _viewportSet(false)
    , _scissorRectSet(false)
    , _enableParallelEncoder(false)
    , _primitiveTypeChanged(false)
    , _maxNumEncoders(1)
{
    TF_VERIFY(desc.colorTextures.size() == desc.colorAttachmentDescs.size());
    
    if (!desc.colorResolveTextures.empty() &&
            desc.colorResolveTextures.size() !=
                desc.colorTextures.size()) {
        TF_CODING_ERROR("color and resolve texture count mismatch.");
        return;
    }

    if (desc.depthResolveTexture && !desc.depthTexture) {
        TF_CODING_ERROR("DepthResolve texture without depth texture.");
        return;
    }
    
    _renderPassDescriptor = [[MTLRenderPassDescriptor alloc] init];

    // The GPU culling pass is only a vertex shader, so it doesn't have any
    // render targets bound to it.  To prevent an API validation error, set
    // some default values for the target.
    if (!desc.HasAttachments()) {
        _renderPassDescriptor.renderTargetWidth = 256;
        _renderPassDescriptor.renderTargetHeight = 256;
        _renderPassDescriptor.defaultRasterSampleCount = 1;
    }

    // Color attachments
    bool resolvingColor = !desc.colorResolveTextures.empty();
    bool hasClear = false;
    for (size_t i=0; i<desc.colorAttachmentDescs.size(); i++) {
        HgiAttachmentDesc const &hgiColorAttachment =
            desc.colorAttachmentDescs[i];
        MTLRenderPassColorAttachmentDescriptor *metalColorAttachment =
            _renderPassDescriptor.colorAttachments[i];

        if (hgiColorAttachment.loadOp == HgiAttachmentLoadOpClear) {
            hasClear = true;
        }
        
        if (@available(macos 100.100, ios 8.0, *)) {
            metalColorAttachment.loadAction = MTLLoadActionLoad;
        }
        else {
            metalColorAttachment.loadAction =
                HgiMetalConversions::GetAttachmentLoadOp(
                    hgiColorAttachment.loadOp);
        }

        metalColorAttachment.storeAction =
            HgiMetalConversions::GetAttachmentStoreOp(
                hgiColorAttachment.storeOp);
        if (hgiColorAttachment.loadOp == HgiAttachmentLoadOpClear) {
            GfVec4f const& clearColor = hgiColorAttachment.clearValue;
            metalColorAttachment.clearColor =
                MTLClearColorMake(
                    clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
        }
        
        HgiMetalTexture *colorTexture =
            static_cast<HgiMetalTexture*>(desc.colorTextures[i].Get());

        TF_VERIFY(
            colorTexture->GetDescriptor().format == hgiColorAttachment.format);
        metalColorAttachment.texture = colorTexture->GetTextureId();
        
        if (resolvingColor) {
            HgiMetalTexture *resolveTexture =
                static_cast<HgiMetalTexture*>(desc.colorResolveTextures[i].Get());

            metalColorAttachment.resolveTexture =
                resolveTexture->GetTextureId();

            if (hgiColorAttachment.storeOp == HgiAttachmentStoreOpStore) {
                metalColorAttachment.storeAction =
                    MTLStoreActionStoreAndMultisampleResolve;
            }
            else {
                metalColorAttachment.storeAction =
                    MTLStoreActionMultisampleResolve;
            }
        }
    }

    // Depth attachment
    if (desc.depthTexture) {
        HgiAttachmentDesc const &hgiDepthAttachment =
            desc.depthAttachmentDesc;
        MTLRenderPassDepthAttachmentDescriptor *metalDepthAttachment =
            _renderPassDescriptor.depthAttachment;

        if (hgiDepthAttachment.loadOp == HgiAttachmentLoadOpClear) {
            hasClear = true;
        }

        metalDepthAttachment.loadAction =
            HgiMetalConversions::GetAttachmentLoadOp(
                hgiDepthAttachment.loadOp);
        metalDepthAttachment.storeAction =
            HgiMetalConversions::GetAttachmentStoreOp(
                hgiDepthAttachment.storeOp);
        
        metalDepthAttachment.clearDepth = hgiDepthAttachment.clearValue[0];
        
        HgiMetalTexture *depthTexture =
            static_cast<HgiMetalTexture*>(desc.depthTexture.Get());
        
        TF_VERIFY(
            depthTexture->GetDescriptor().format == hgiDepthAttachment.format);
        metalDepthAttachment.texture = depthTexture->GetTextureId();
        
        if (desc.depthResolveTexture) {
            HgiMetalTexture *resolveTexture =
                static_cast<HgiMetalTexture*>(desc.depthResolveTexture.Get());

            metalDepthAttachment.resolveTexture =
                resolveTexture->GetTextureId();
            
            if (hgiDepthAttachment.storeOp == HgiAttachmentStoreOpStore) {
                metalDepthAttachment.storeAction =
                    MTLStoreActionStoreAndMultisampleResolve;
            }
            else {
                metalDepthAttachment.storeAction =
                    MTLStoreActionMultisampleResolve;
            }
        }
        
        // Stencil attachment
        if (depthTexture->GetDescriptor().format == HgiFormatFloat32UInt8) {
            MTLRenderPassStencilAttachmentDescriptor *stencilAttachment =
                _renderPassDescriptor.stencilAttachment;
            stencilAttachment.loadAction = metalDepthAttachment.loadAction;
            stencilAttachment.storeAction = metalDepthAttachment.storeAction;
            stencilAttachment.clearStencil =
                static_cast<uint32_t>(hgiDepthAttachment.clearValue[1]);
            stencilAttachment.texture = metalDepthAttachment.texture;
            
            if (desc.depthResolveTexture) {
                stencilAttachment.resolveTexture =
                    metalDepthAttachment.resolveTexture;
                stencilAttachment.stencilResolveFilter =
                    MTLMultisampleStencilResolveFilterDepthResolvedSample;
                stencilAttachment.storeAction =
                    metalDepthAttachment.storeAction;
            }
        }
    }
    
    _enableParallelEncoder = _hgi->GetCapabilities()->useParallelEncoder;
    
    if (_enableParallelEncoder) {
        _maxNumEncoders = WorkGetPhysicalConcurrencyLimit() / 2;
    }
    else {
        _maxNumEncoders = 1;
    }
    
    if (hasClear) {
        GetEncoder();
        _CreateArgumentBuffer();
    }
}

HgiMetalGraphicsCmds::~HgiMetalGraphicsCmds()
{
    TF_VERIFY(_encoders.empty(), "Encoder created, but never commited.");
    
    [_renderPassDescriptor release];
    if (_debugLabel) {
        [_debugLabel release];
    }
}

void
HgiMetalGraphicsCmds::EnableParallelEncoder(bool enable)
{
    _enableParallelEncoder = enable;
}

void
HgiMetalGraphicsCmds::_VegaIndirectFix()
{
    if (!_primitiveTypeChanged) {
        return;
    }
    
    if (!_hgi->GetCapabilities()->requiresIndirectDrawFix) {
        return;
    }
    // Fix for Vega in macOS before 12.0.  There is state leakage between
    // indirect draw of different prim types which results in a GPU crash.
    // Flush with a null draw through the direct path.
    id<MTLRenderCommandEncoder> encoder = GetEncoder();
    MTLPrimitiveType mtlType =
        HgiMetalConversions::GetPrimitiveType(_primitiveType);
    [encoder drawPrimitives:mtlType
            vertexStart:0
            vertexCount:0];
}

uint32_t
HgiMetalGraphicsCmds::_GetNumEncoders()
{
    return (uint32_t)_encoders.size();
}

void
HgiMetalGraphicsCmds::_SetCachedEncoderState(id<MTLRenderCommandEncoder> encoder)
{
    if (_viewportSet) {
        [encoder setViewport:_CachedEncState.viewport];
    }
    if (_scissorRectSet) {
        [encoder setScissorRect:_CachedEncState.scissorRect];
    }
    if (_CachedEncState.graphicsPipeline) {
        _CachedEncState.graphicsPipeline->BindPipeline(encoder);
    }
    if (_CachedEncState.resourceBindings) {
        _CachedEncState.resourceBindings->BindResources(_hgi,
                                                        encoder,
                                                        _CachedEncState.argumentBuffer);
    }

    _SetVertexBindings(encoder, _CachedEncState.vertexBindings);
}

void
HgiMetalGraphicsCmds::_SetNumberParallelEncoders(uint32_t numEncoders)
{
    // Put a lock around the creation to prevent two requests colliding
    // (this should never happen...)
    std::lock_guard<std::mutex> lock(_encoderLock);
    
    uint32_t const numActiveEncoders = _GetNumEncoders();
    
    // Check if we already have enough
    if (numEncoders <= numActiveEncoders) {
        return;
    }
    
    if (_enableParallelEncoder) {
        // Do we need to create a parallel encoder
        if (!_parallelEncoder) {
            _parallelEncoder = [
                _hgi->GetPrimaryCommandBuffer(this, false)
                parallelRenderCommandEncoderWithDescriptor:_renderPassDescriptor];
            
                if (_debugLabel) {
                    [_parallelEncoder pushDebugGroup:_debugLabel];
                }
        }
        // Create any missing encoders
        for (uint32_t i = numActiveEncoders; i < numEncoders; i++) {
            id<MTLRenderCommandEncoder> encoder =
                [_parallelEncoder renderCommandEncoder];
            _encoders.push_back(encoder);
        }
    }
    else {
        if (numEncoders > 1) {
            TF_CODING_ERROR("Only 1 encoder supported");
        }
        if (numActiveEncoders >= 1) {
            return;
        }
        
        id<MTLRenderCommandEncoder> encoder = 
            [_hgi->GetPrimaryCommandBuffer(this, false)
             renderCommandEncoderWithDescriptor:_renderPassDescriptor];
        if (_debugLabel) {
            [encoder pushDebugGroup:_debugLabel];
        }
        _encoders.push_back(encoder);
    }
    
    for (uint32_t i = numActiveEncoders; i < numEncoders; i++) {
        // Setup any relevant state for the new encoder(s)
        _SetCachedEncoderState(_encoders[i]);
    }
    
    if (_debugLabel) {
        [_debugLabel release];
        _debugLabel = nil;
    }
}

id<MTLRenderCommandEncoder>
HgiMetalGraphicsCmds::GetEncoder(uint32_t encoderIndex)
{
    uint32_t numActiveEncoders = _GetNumEncoders();
    
    // Do we need to create an intial encoder
    if (!numActiveEncoders) {
        _SetNumberParallelEncoders(1);
        numActiveEncoders = _GetNumEncoders();
    }
    
    // Check if we have this encoder (it's OK not to have it)
    if (encoderIndex >= numActiveEncoders) {
        TF_CODING_ERROR("Invalid render encoder index specified");
        return nil;
    }
    
    return _encoders[encoderIndex];
}

void
HgiMetalGraphicsCmds::_CreateArgumentBuffer()
{
    if (!_argumentBuffer) {
        _argumentBuffer = _hgi->GetArgBuffer();
    }
}

void
HgiMetalGraphicsCmds::_SyncArgumentBuffer()
{
    if (_argumentBuffer) {
#if defined(ARCH_OS_OSX)
        if (_argumentBuffer.storageMode != MTLStorageModeShared &&
            [_argumentBuffer respondsToSelector:@selector(didModifyRange:)]) {

            ARCH_PRAGMA_PUSH
            ARCH_PRAGMA_INSTANCE_METHOD_NOT_FOUND
            [_argumentBuffer didModifyRange:{0, _argumentBuffer.length}];
            ARCH_PRAGMA_POP
        }
#endif
        _argumentBuffer = nil;
    }
}

void
HgiMetalGraphicsCmds::SetViewport(GfVec4i const& vp)
{
    double x = vp[0];
    double y = vp[1];
    double w = vp[2];
    double h = vp[3];

    // Viewport is inverted in the y. Along with the front face winding order
    // being inverted.
    // This combination allows us to emulate the OpenGL coordinate space on
    // Metal
    _CachedEncState.viewport = (MTLViewport){x, y+h, w, -h, 0.0, 1.0};
    
    for (auto& encoder : _encoders) {
        [encoder setViewport:_CachedEncState.viewport];
    }
    
    _viewportSet = true;
}

void
HgiMetalGraphicsCmds::SetScissor(GfVec4i const& sc)
{
    uint32_t x = sc[0];
    uint32_t y = sc[1];
    uint32_t w = sc[2];
    uint32_t h = sc[3];
    
    _CachedEncState.scissorRect = (MTLScissorRect){x, y, w, h};
    
    for (auto& encoder : _encoders) {
        [encoder setScissorRect:_CachedEncState.scissorRect];
    }
    
    _scissorRectSet = true;
}

void
HgiMetalGraphicsCmds::BindPipeline(HgiGraphicsPipelineHandle pipeline)
{
    _primitiveTypeChanged =
        (_primitiveType != pipeline->GetDescriptor().primitiveType);
    _primitiveType = pipeline->GetDescriptor().primitiveType;

    _primitiveIndexSize =
        pipeline->GetDescriptor().tessellationState.primitiveIndexSize;
    
    _stepFunctions.Init(pipeline->GetDescriptor());

    _CachedEncState.graphicsPipeline =
        static_cast<HgiMetalGraphicsPipeline*>(pipeline.Get());
    
    if (_CachedEncState.graphicsPipeline) {
        for (auto& encoder : _encoders) {
            _CachedEncState.graphicsPipeline->BindPipeline(encoder);
        }
    }
}

void
HgiMetalGraphicsCmds::BindResources(HgiResourceBindingsHandle r)
{
    _CreateArgumentBuffer();

    _CachedEncState.resourceBindings =
        static_cast<HgiMetalResourceBindings*>(r.Get());
    _CachedEncState.argumentBuffer = _argumentBuffer;
    
    if (_CachedEncState.resourceBindings) {
        for (auto& encoder : _encoders) {
            _CachedEncState.resourceBindings->BindResources(_hgi,
                                                            encoder,
                                                            _argumentBuffer);
        }
    }
}

void
HgiMetalGraphicsCmds::SetConstantValues(
    HgiGraphicsPipelineHandle pipeline,
    HgiShaderStage stages,
    uint32_t bindIndex,
    uint32_t byteSize,
    const void* data)
{
    _CreateArgumentBuffer();

    HgiMetalResourceBindings::SetConstantValues(
        _argumentBuffer, stages, bindIndex, byteSize, data);
}

void
HgiMetalGraphicsCmds::BindVertexBuffers(
    HgiVertexBufferBindingVector const &bindings)
{
    _stepFunctions.Bind(bindings);

    _CachedEncState.vertexBindings.insert(
        _CachedEncState.vertexBindings.end(),
        bindings.begin(), bindings.end());

    for (auto& encoder : _encoders) {
        _SetVertexBindings(encoder, bindings);
    }
}

void
HgiMetalGraphicsCmds::Draw(
    uint32_t vertexCount,
    uint32_t baseVertex,
    uint32_t instanceCount,
    uint32_t baseInstance)
{
    _SyncArgumentBuffer();

    MTLPrimitiveType type=HgiMetalConversions::GetPrimitiveType(_primitiveType);
    id<MTLRenderCommandEncoder> encoder = GetEncoder();

    _stepFunctions.SetVertexBufferOffsets(encoder, baseInstance);

    if (_primitiveType == HgiPrimitiveTypePatchList) {
        const NSUInteger controlPointCount = _primitiveIndexSize;
        [encoder drawPatches:controlPointCount
                  patchStart:0
                  patchCount:vertexCount/controlPointCount
            patchIndexBuffer:NULL
      patchIndexBufferOffset:0
               instanceCount:instanceCount
                baseInstance:baseInstance];
    } else {
        if (instanceCount == 1) {
            [encoder drawPrimitives:type
                        vertexStart:baseVertex
                        vertexCount:vertexCount];
        } else {
            [encoder drawPrimitives:type
                        vertexStart:baseVertex
                        vertexCount:vertexCount
                      instanceCount:instanceCount
                       baseInstance:baseInstance];
        }
    }

    _hgi->SetHasWork();
}

void
HgiMetalGraphicsCmds::DrawIndirect(
    HgiBufferHandle const& drawParameterBuffer,
    uint32_t drawBufferByteOffset,
    uint32_t drawCount,
    uint32_t stride)
{
    MTLPrimitiveType mtlType =
        HgiMetalConversions::GetPrimitiveType(_primitiveType);
    id<MTLBuffer> drawBufferId =
        static_cast<HgiMetalBuffer*>(drawParameterBuffer.Get())->GetBufferId();

    _SyncArgumentBuffer();
    static const uint32_t _drawCallsPerThread = 256;
    const uint32_t numEncoders = std::min(
                                 std::max(drawCount / _drawCallsPerThread, 1U),
                                 _maxNumEncoders);
    const uint32_t normalCount = drawCount / numEncoders;
    const uint32_t finalCount = normalCount
                              + (drawCount - normalCount * numEncoders);

    _SetNumberParallelEncoders(numEncoders);

    WorkWithScopedParallelism([&]() {
        WorkDispatcher wd;
        
        for (uint32_t i = 0; i < numEncoders; ++i) {
            const uint32_t encoderOffset = normalCount * i;
            // If this is the last encoder then ensure that we have all prims.
            const uint32_t encoderCount = (i == numEncoders - 1)
                                        ? finalCount : normalCount;
            wd.Run([&, i, encoderOffset, encoderCount]() {
                id<MTLRenderCommandEncoder> encoder = GetEncoder(i);
                
                if (_primitiveType == HgiPrimitiveTypePatchList) {
                    const NSUInteger controlPointCount = _primitiveIndexSize;
                    for (uint32_t offset = encoderOffset;
                         offset < encoderOffset + encoderCount;
                         ++offset) {
                        _stepFunctions.SetVertexBufferOffsets(encoder, offset);
                        const uint32_t bufferOffset = drawBufferByteOffset
                                                    + (offset * stride);
                        [encoder drawPatches:controlPointCount
                            patchIndexBuffer:NULL
                      patchIndexBufferOffset:0
                              indirectBuffer:drawBufferId
                        indirectBufferOffset:bufferOffset];
                    }
                }
                else {
                    _VegaIndirectFix();

                    for (uint32_t offset = encoderOffset;
                         offset < encoderOffset + encoderCount;
                         ++offset) {
                        _stepFunctions.SetVertexBufferOffsets(encoder, offset);
                        const uint32_t bufferOffset = drawBufferByteOffset
                                                    + (offset * stride);
                        
                        [encoder drawPrimitives:mtlType
                                 indirectBuffer:drawBufferId
                           indirectBufferOffset:bufferOffset];
                    }
                }
            });
        }
    });

    _hgi->SetHasWork();
}

void
HgiMetalGraphicsCmds::DrawIndexed(
    HgiBufferHandle const& indexBuffer,
    uint32_t indexCount,
    uint32_t indexBufferByteOffset,
    uint32_t baseVertex,
    uint32_t instanceCount,
    uint32_t baseInstance)
{
    _SyncArgumentBuffer();

    HgiMetalBuffer* indexBuf = static_cast<HgiMetalBuffer*>(indexBuffer.Get());

    MTLPrimitiveType mtlType =
        HgiMetalConversions::GetPrimitiveType(_primitiveType);

    id<MTLRenderCommandEncoder> encoder = GetEncoder();
        
    _stepFunctions.SetVertexBufferOffsets(encoder, baseInstance);

    if (_primitiveType == HgiPrimitiveTypePatchList) {
        const NSUInteger controlPointCount = _primitiveIndexSize;

        _stepFunctions.SetPatchBaseOffsets(encoder, baseVertex);

        [encoder drawIndexedPatches:controlPointCount
                         patchStart:indexBufferByteOffset / sizeof(uint32_t)
                         patchCount:indexCount
                   patchIndexBuffer:nil
             patchIndexBufferOffset:0
            controlPointIndexBuffer:indexBuf->GetBufferId()
      controlPointIndexBufferOffset:0
                      instanceCount:instanceCount
                       baseInstance:baseInstance];
    } else {
        _VegaIndirectFix();

        [encoder drawIndexedPrimitives:mtlType
                            indexCount:indexCount
                             indexType:MTLIndexTypeUInt32
                           indexBuffer:indexBuf->GetBufferId()
                     indexBufferOffset:indexBufferByteOffset
                         instanceCount:instanceCount
                            baseVertex:baseVertex
                          baseInstance:baseInstance];
    }

    _hgi->SetHasWork();
}

void
HgiMetalGraphicsCmds::DrawIndexedIndirect(
    HgiBufferHandle const& indexBuffer,
    HgiBufferHandle const& drawParameterBuffer,
    uint32_t drawBufferByteOffset,
    uint32_t drawCount,
    uint32_t stride,
    std::vector<uint32_t> const& drawParameterBufferUInt32,
    uint32_t patchBaseVertexByteOffset)
{
    MTLPrimitiveType mtlType =
        HgiMetalConversions::GetPrimitiveType(_primitiveType);
    id<MTLBuffer> drawBufferId =
        static_cast<HgiMetalBuffer*>(drawParameterBuffer.Get())->GetBufferId();
    id<MTLBuffer> indexBufferId =
        static_cast<HgiMetalBuffer*>(indexBuffer.Get())->GetBufferId();

    _SyncArgumentBuffer();
    static const uint32_t _drawCallsPerThread = 256;
    const uint32_t numEncoders = std::min(
                                 std::max(drawCount / _drawCallsPerThread, 1U),
                                 _maxNumEncoders);
    const uint32_t normalCount = drawCount / numEncoders;
    const uint32_t finalCount = normalCount
                              + (drawCount - normalCount * numEncoders);

    _SetNumberParallelEncoders(numEncoders);

    WorkWithScopedParallelism([&]() {
        WorkDispatcher wd;
        
        for (uint32_t i = 0; i < numEncoders; ++i) {
            const uint32_t encoderOffset = normalCount * i;
            // If this is the last encoder then ensure that we have all prims.
            const uint32_t encoderCount = (i == numEncoders - 1)
                                        ? finalCount : normalCount;
            wd.Run([&, i, encoderOffset, encoderCount]() {
                id<MTLRenderCommandEncoder> encoder = GetEncoder(i);
                
                if (_primitiveType == HgiPrimitiveTypePatchList) {
                    const NSUInteger controlPointCount = _primitiveIndexSize;
                    
                    for (uint32_t offset = encoderOffset;
                         offset < encoderOffset + encoderCount;
                         ++offset) {
                            _stepFunctions.SetVertexBufferOffsets(
                                encoder, offset);

                            const uint32_t baseVertexIndex =
                                (patchBaseVertexByteOffset +
                                 offset * stride) / sizeof(uint32_t);
                            const uint32_t baseVertex =
                                drawParameterBufferUInt32[baseVertexIndex];

                            _stepFunctions.SetPatchBaseOffsets(
                                encoder, baseVertex);

                            const uint32_t bufferOffset = drawBufferByteOffset
                                                        + (offset * stride);
                        [encoder drawIndexedPatches:controlPointCount
                                   patchIndexBuffer:nil
                             patchIndexBufferOffset:0
                            controlPointIndexBuffer:indexBufferId
                      controlPointIndexBufferOffset:0
                                     indirectBuffer:drawBufferId
                               indirectBufferOffset:bufferOffset];
                    }
                }
                else {
                    _VegaIndirectFix();
                    
                    for (uint32_t offset = encoderOffset;
                         offset < encoderOffset + encoderCount;
                         ++offset) {
                        _stepFunctions.SetVertexBufferOffsets(encoder, offset);

                        const uint32_t bufferOffset = drawBufferByteOffset
                                                    + (offset * stride);
                        
                        [encoder drawIndexedPrimitives:mtlType
                                             indexType:MTLIndexTypeUInt32
                                           indexBuffer:indexBufferId
                                     indexBufferOffset:0
                                        indirectBuffer:drawBufferId
                                  indirectBufferOffset:bufferOffset];
                    }
                }
            });
        }
    });

    _hgi->SetHasWork();
}

void
HgiMetalGraphicsCmds::PushDebugGroup(const char* label)
{
    if (!HgiMetalDebugEnabled()) {
        return;
    }
    if (_parallelEncoder) {
        HGIMETAL_DEBUG_PUSH_GROUP(_parallelEncoder, label)
    }
    else if (!_encoders.empty()) {
        HGIMETAL_DEBUG_PUSH_GROUP(GetEncoder(), label)
    }
    else {
        _debugLabel = [@(label) copy];
    }
}

void
HgiMetalGraphicsCmds::PopDebugGroup()
{
    if (_parallelEncoder) {
        HGIMETAL_DEBUG_POP_GROUP(_parallelEncoder)
    }
    else if (!_encoders.empty()) {
        HGIMETAL_DEBUG_POP_GROUP(GetEncoder());
    }
    if (_debugLabel) {
        [_debugLabel release];
        _debugLabel = nil;
    }
}

void
HgiMetalGraphicsCmds::InsertMemoryBarrier(HgiMemoryBarrier barrier)
{
    TF_VERIFY(barrier==HgiMemoryBarrierAll, "Unknown barrier");
    
    // Apple Silicon only support memory barriers between vertex stages after
    // macOS 12.3.
    // For iOS we may want to introduce an alternative path
#if defined(ARCH_OS_OSX)
    if (@available(macOS 12.3, ios 16.0,  *)) {
        MTLBarrierScope scope = MTLBarrierScopeBuffers;
        MTLRenderStages srcStages = MTLRenderStageVertex;
        MTLRenderStages dstStages = MTLRenderStageVertex;

        for (auto& encoder : _encoders) {
            [encoder memoryBarrierWithScope:scope
                                afterStages:srcStages
                               beforeStages:dstStages];
        }
    }
#endif
}

static
HgiMetal::CommitCommandBufferWaitType
_ToHgiMetal(const HgiSubmitWaitType wait)
{
    switch(wait) {
        case HgiSubmitWaitTypeNoWait:
            return HgiMetal::CommitCommandBuffer_NoWait;
        case HgiSubmitWaitTypeWaitUntilCompleted:
            return HgiMetal::CommitCommandBuffer_WaitUntilCompleted;
    }

    TF_CODING_ERROR("Bad enum value for HgiSubmitWaitType");
    return HgiMetal::CommitCommandBuffer_WaitUntilCompleted;
}

bool
HgiMetalGraphicsCmds::_Submit(Hgi* hgi, HgiSubmitWaitType wait)
{
    if (_parallelEncoder) {
        WorkParallelForEach(_encoders.begin(),
                            _encoders.end(),
                            [](auto& encoder) {
                                [encoder endEncoding];
                            });
        [_parallelEncoder endEncoding];
        _parallelEncoder = nil;

        _hgi->CommitPrimaryCommandBuffer(_ToHgiMetal(wait));
    }
    else if (!_encoders.empty()) {
        for (auto &encoder : _encoders) {
            [encoder endEncoding];
        }
        
        _hgi->CommitPrimaryCommandBuffer(_ToHgiMetal(wait));
    }

    std::lock_guard<std::mutex> lock(_encoderLock);
    
    _argumentBuffer = nil;
    _encoders.clear();
    _CachedEncState.ResetCachedEncoderState();
    
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
