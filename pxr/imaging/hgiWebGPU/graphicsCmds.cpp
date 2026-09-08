//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/imaging/hgiWebGPU/buffer.h"
#include "pxr/imaging/hgiWebGPU/conversions.h"
#include "pxr/imaging/hgiWebGPU/debugCodes.h"
#include "pxr/imaging/hgiWebGPU/diagnostic.h"
#include "pxr/imaging/hgiWebGPU/graphicsCmds.h"
#include "pxr/imaging/hgiWebGPU/hgi.h"
#include "pxr/imaging/hgiWebGPU/graphicsPipeline.h"
#include "pxr/imaging/hgiWebGPU/resourceBindings.h"
#include "pxr/imaging/hgiWebGPU/texture.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiWebGPUGraphicsCmds::HgiWebGPUGraphicsCmds(
        HgiWebGPU* hgi,
        HgiGraphicsCmdsDesc const& desc)
        : HgiWebGPUBaseGraphicsCmds(hgi, desc)
{
    wgpu::RenderPassDescriptor renderPassDesc;
    std::vector<wgpu::RenderPassColorAttachment> colorAttachments;
    for( size_t i=0; i<_descriptor.colorTextures.size(); ++i )
    {
        auto colorTarget = static_cast<HgiWebGPUTexture *>(_descriptor.colorTextures[i].Get());
        HgiAttachmentDesc colorAttachmentDesc = _descriptor.colorAttachmentDescs[i];

        wgpu::RenderPassColorAttachment colorDesc;
        colorDesc.view = colorTarget->GetTextureView();
        colorDesc.loadOp  = HgiWebGPUConversions::GetAttachmentLoadOp(colorAttachmentDesc.loadOp);
        colorDesc.storeOp = HgiWebGPUConversions::GetAttachmentStoreOp(colorAttachmentDesc.storeOp);
        colorDesc.clearValue.r = colorAttachmentDesc.clearValue[0];
        colorDesc.clearValue.g = colorAttachmentDesc.clearValue[1];
        colorDesc.clearValue.b = colorAttachmentDesc.clearValue[2];
        colorDesc.clearValue.a = colorAttachmentDesc.clearValue[3];
        if (_descriptor.colorResolveTextures.size() > i &&
                colorTarget->GetDescriptor().sampleCount > 1) {
            auto colorResolveTarget = static_cast<HgiWebGPUTexture *>(_descriptor.colorResolveTextures[i].Get());
            colorDesc.resolveTarget = colorResolveTarget->GetTextureView();
        }
        colorAttachments.push_back(colorDesc);
    }
    renderPassDesc.colorAttachmentCount = colorAttachments.size();
    renderPassDesc.colorAttachments = colorAttachments.data();

    auto depthTarget = dynamic_cast<HgiWebGPUTexture *>(_descriptor.depthTexture.Get());
    wgpu::RenderPassDepthStencilAttachment depthStencilDesc;
    if( depthTarget )
    {
        depthStencilDesc.depthLoadOp = HgiWebGPUConversions::GetAttachmentLoadOp(_descriptor.depthAttachmentDesc.loadOp);
        depthStencilDesc.depthStoreOp = HgiWebGPUConversions::GetAttachmentStoreOp(_descriptor.depthAttachmentDesc.storeOp);

        if (depthTarget->GetDescriptor().format == HgiFormatFloat32UInt8 ) {
            depthStencilDesc.stencilLoadOp = depthStencilDesc.depthLoadOp;
            depthStencilDesc.stencilStoreOp = depthStencilDesc.depthStoreOp;
        }

        // Webgpu doesn't support depth resolution with multisampling. We will do the depth resolution
        //  from _descriptor.depthResolveTexture after finishing the render pass if applicable
        depthStencilDesc.view = depthTarget->GetTextureView();
        // depth is a single channel, using first component
        depthStencilDesc.depthClearValue = _descriptor.depthAttachmentDesc.clearValue[0];

        renderPassDesc.depthStencilAttachment = &depthStencilDesc;
    }
#if !defined(EMSCRIPTEN)
    if (_IsTimestampsEnabled()) {
        wgpu::PassTimestampWrites timestampWrites = _hgi->GetRenderTimestampWrites();
        renderPassDesc.timestampWrites = &timestampWrites;
    }
#endif

    _passEncoder = _commandEncoder.BeginRenderPass(&renderPassDesc);

    if (!_descriptor.colorTextures.empty()) {
        auto size = _descriptor.colorTextures[0]->GetDescriptor().dimensions;
        SetViewport(GfVec4i(0, 0, size[0], size[1]));
        SetScissor(GfVec4i(0, 0, size[0], size[1]));
    }
}

bool HgiWebGPUGraphicsCmds::_IsTimestampsEnabled() {
    return _hgi->GetCapabilities()->IsSet(HgiDeviceCapabilitiesBitsCppShaderPadding) &&
    TfDebug::IsEnabled(HGIWEBGPU_DEBUG_TIMESTAMPS);

}

void
HgiWebGPUGraphicsCmds::SetViewport(GfVec4i const& vp)
{
    const auto offsetX = static_cast<float>(vp[0]);
    const auto offsetY = static_cast<float>(vp[1]);
    const auto width = static_cast<float>(vp[2]);
    const auto height = static_cast<float>(vp[3]);

    _passEncoder.SetViewport(offsetX, offsetY, width, height, 0.f, 1.f);
}

void
HgiWebGPUGraphicsCmds::SetScissor(GfVec4i const& sc)
{
    uint32_t w(sc[2]);
    uint32_t h(sc[3]);

    _passEncoder.SetScissorRect(sc[0], sc[1], w, h);
}

bool
HgiWebGPUGraphicsCmds::_Submit(Hgi *hgi, HgiSubmitWaitType wait) {
    // End render pass
    _EndRenderPass();

    HgiWebGPU *wgpuHgi = static_cast<HgiWebGPU *>(hgi);

    wgpuHgi->EnqueueCommandBuffer(_commandBuffer);
    wgpuHgi->QueueSubmit();
#if !defined(EMSCRIPTEN)
    if (_hasWork && _IsTimestampsEnabled()) {
        wgpuHgi->QueryValue();
    }
#endif

    _commandBuffer = nullptr;

    return _hasWork;
}

void
HgiWebGPUGraphicsCmds::_EndRenderPass()
{
    if (_renderPassStarted) {
        // release any resources
        _passEncoder.End();
        _passEncoder = nullptr;
        _lastIndexBuffer = nullptr;

#if !defined(EMSCRIPTEN)
        if (_hasWork && _IsTimestampsEnabled()) {
            _hgi->ResolveQuery(_commandEncoder,
                                _lastDrawLabel.empty() ? _pipeline->GetDescriptor().debugName : _lastDrawLabel);
        }
#endif

        // Always resolve depth in order to catch the case where nothing opaque is drawn which would leave the depth resolve texture
        // in an uncleared stage. We need to make sure that depth resolve and depth have been initialized with the same clear value.
        auto depthTarget = dynamic_cast<HgiWebGPUTexture *>(_descriptor.depthTexture.Get());
        if (depthTarget) {
            HgiTextureDesc depthDesc = depthTarget->GetDescriptor();
            if(depthDesc.sampleCount > 1 && _descriptor.depthResolveTexture) {
                auto depthResolveTarget = dynamic_cast<HgiWebGPUTexture *>(_descriptor.depthResolveTexture.Get());
                _hgi->ResolveDepth(_commandEncoder, *depthTarget, *depthResolveTarget);
            }
        }

        _commandBuffer = _commandEncoder.Finish();
        _commandEncoder = nullptr;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
