//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_WEBGPU_BASE_GRAPHICS_CMDS_H
#define PXR_IMAGING_HGI_WEBGPU_BASE_GRAPHICS_CMDS_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgiWebGPU/conversions.h"
#include "pxr/imaging/hgiWebGPU/hgi.h"
#include "pxr/imaging/hgiWebGPU/diagnostic.h"
#include "pxr/imaging/hgiWebGPU/graphicsPipeline.h"
#include "pxr/imaging/hgiWebGPU/resourceBindings.h"
#include "pxr/imaging/hgiWebGPU/stepFunctions.h"
#include "pxr/imaging/hgiWebGPU/texture.h"
#include "pxr/imaging/hgi/graphicsCmds.h"
#include <cstdint>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

struct HgiGraphicsCmdsDesc;
class HgiWebGPUGraphicsPipeline;

/// \class HgiWebGPUBaseGraphicsCmds
///
/// WebGPU implementation of HgiGraphicsEncoder.
///
template<typename Base, typename PassEncoder>
class HgiWebGPUBaseGraphicsCmds : public Base
{
    static_assert(std::is_base_of_v<HgiGraphicsCmds, Base>,
        "Base must derive from HgiGraphicsCmds");
    static_assert(std::is_same_v<PassEncoder, wgpu::RenderPassEncoder> ||
        std::is_same_v<PassEncoder, wgpu::RenderBundleEncoder>,
        "PassEncoder must be either wgpu::RenderPassEncoder or "
        "wgpu::RenderBundleEncoder");

public:
    HGIWEBGPU_API
    ~HgiWebGPUBaseGraphicsCmds() override {
        _commandBuffer = nullptr;
    }

    HGIWEBGPU_API
    void InsertMemoryBarrier(HgiMemoryBarrier barrier) override {}

    HGIWEBGPU_API
    void PushDebugGroup(const char* label,
        const GfVec4f& color = HgiCmds::s_graphicsDebugColor) override {
        if (_passEncoder) {
            HgiWebGPUBeginLabel(_passEncoder, label);
        } else {
            HgiWebGPUBeginLabel(_commandEncoder, label);
        }
        _debugGroupLabels.push_back(label);
    }

    HGIWEBGPU_API
    void PopDebugGroup() override {
        if (_passEncoder) {
            HgiWebGPUEndLabel(_passEncoder);
        } else {
            HgiWebGPUEndLabel(_commandEncoder);
        }
        _debugGroupLabels.pop_back();
    }

    HGIWEBGPU_API
    void InsertDebugMarker(
        const char* label,
        const GfVec4f& color = HgiCmds::s_markerDebugColor) override {
    }

    HGIWEBGPU_API
    void BindPipeline(HgiGraphicsPipelineHandle pipeline) override {
        _stepFunctions.Init(pipeline->GetDescriptor());

        _pipeline = dynamic_cast<HgiWebGPUGraphicsPipeline *>(pipeline.Get());

        _passEncoder.SetPipeline(_pipeline->GetPipeline());
    }

    HGIWEBGPU_API
    void BindResources(HgiResourceBindingsHandle resources) override {
        // delay until the pipeline is set and the render pass has begun
        _resourceBindings = resources;
    }

    HGIWEBGPU_API
    void BindVertexBuffers(
        HgiVertexBufferBindingVector const &bindings) override {
        _stepFunctions.Bind(bindings);
        std::vector<wgpu::Buffer> buffers;
        std::vector<uint32_t> bufferOffsets;
        for (HgiVertexBufferBinding const &binding : bindings) {
            HgiWebGPUBuffer* buf=dynamic_cast<HgiWebGPUBuffer*>(binding.buffer.Get());
            wgpu::Buffer wgpuBuf = buf->GetBufferHandle();
            if (wgpuBuf) {
                buffers.push_back(wgpuBuf);
                bufferOffsets.push_back(binding.byteOffset);
            }
        }
        for( uint32_t i=0; i<buffers.size(); i++ ) {
            _passEncoder.SetVertexBuffer(i, buffers[i], bufferOffsets[i]);
        }
    }

    HGIWEBGPU_API
    void Draw(
        uint32_t vertexCount,
        uint32_t baseVertex,
        uint32_t instanceCount,
        uint32_t baseInstance) override {
        _BindResources();
        _passEncoder.Draw(vertexCount, instanceCount, baseVertex, 0);
    }

    HGIWEBGPU_API
    void DrawIndirect(
        HgiBufferHandle const& drawParameterBuffer,
        uint32_t drawBufferOffset,
        uint32_t drawCount,
        uint32_t stride) override {
        _BindResources();

        HgiWebGPUBuffer* drawBuf =
                static_cast<HgiWebGPUBuffer*>(drawParameterBuffer.Get());

        for (uint32_t baseInstance = 0; baseInstance < drawCount; baseInstance++) {
            _stepFunctions.SetVertexBufferOffsets(_passEncoder, baseInstance);
            _passEncoder.DrawIndirect(drawBuf->GetBufferHandle(), baseInstance * stride);
        }
    }

    HGIWEBGPU_API
    void DrawIndexed(
        HgiBufferHandle const& indexBuffer,
        uint32_t indexCount,
        uint32_t indexBufferByteOffset,
        uint32_t baseVertex,
        uint32_t instanceCount,
        uint32_t baseInstance) override {
        TF_VERIFY(instanceCount>0);

        _BindResources();
        _stepFunctions.SetVertexBufferOffsets(_passEncoder, baseInstance);
        HgiWebGPUBuffer* ibo = dynamic_cast<HgiWebGPUBuffer*>(indexBuffer.Get());
        uint32_t const baseIndex = indexBufferByteOffset / sizeof(uint32_t);
        if (!_lastIndexBuffer || _lastIndexBuffer.Get() != ibo->GetBufferHandle().Get())
        {
            _passEncoder.SetIndexBuffer(ibo->GetBufferHandle(), wgpu::IndexFormat::Uint32, 0,
                                              ibo->GetByteSizeOfResource());
            _lastIndexBuffer = ibo->GetBufferHandle();
        }
        // The baseInstance must be set to 0 for WebGPU as it is not passed into the shader. There is no matching builtin.
        // Hydra computes the current instance index in the shader by subtracting the baseInstance from the instance index,
        // resulting in range of 0 to instanceCount for accessing data in buffers.
        // If baseInstance is passed here, the current instance index in the shader will be incorrect,
        // as it will start from the baseInstance.
        _passEncoder.DrawIndexed(indexCount, instanceCount, baseIndex, baseVertex, 0);
    }

    HGIWEBGPU_API
    void DrawIndexedIndirect(
        HgiBufferHandle const& indexBuffer,
        HgiBufferHandle const& drawParameterBuffer,
        uint32_t drawBufferByteOffset,
        uint32_t drawCount,
        uint32_t stride,
        std::vector<uint32_t> const& drawParameterBufferUInt32,
        uint32_t patchBaseVertexByteOffset) override {
        _BindResources();

        HgiWebGPUBuffer* ibo = static_cast<HgiWebGPUBuffer*>(indexBuffer.Get());
        HgiWebGPUBuffer* drawBuf =
                static_cast<HgiWebGPUBuffer*>(drawParameterBuffer.Get());

        _passEncoder.SetIndexBuffer(ibo->GetBufferHandle(), wgpu::IndexFormat::Uint32, 0, ibo->GetByteSizeOfResource() - drawBufferByteOffset);

        for (uint32_t baseInstance = 0; baseInstance < drawCount; baseInstance++) {
            _stepFunctions.SetVertexBufferOffsets(_passEncoder, baseInstance);
            _passEncoder.DrawIndexedIndirect(drawBuf->GetBufferHandle(), baseInstance * stride);
        }
    }

    HGIWEBGPU_API
    void SetConstantValues(
            HgiGraphicsPipelineHandle ,
            HgiShaderStage ,
            uint32_t bindIndex,
    uint32_t byteSize,
    const void* data) override
    {
        // This is a simplified assumption, since at this point we don't know the structure of the uniform
        static const size_t alignment = 16;
        wgpu::Device device = _hgi->GetPrimaryDevice();
        wgpu::BufferDescriptor bufferDesc;
        bufferDesc.label = static_cast<std::string>("uniform").c_str();
        bufferDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
        bufferDesc.size = ((byteSize + alignment - 1) / alignment) * alignment;;
        wgpu::Buffer constantBuffer = device.CreateBuffer(&bufferDesc);
        wgpu::Queue queue = device.GetQueue();
        queue.WriteBuffer(constantBuffer, 0, data, byteSize);

        _constantBindGroupEntry.binding = bindIndex;
        _constantBindGroupEntry.buffer = constantBuffer;
        _constantBindGroupEntry.size = bufferDesc.size;
        _pushConstantsDirty = true;
    }

protected:
    friend class HgiWebGPU;

    // WebGPU doesn't support MSAA for integer texture formats (e.g. primId,
    // instanceId). Their sampleCount is forced to 1 at creation time. This
    // function detects that case and redirects the whole render pass to use the
    // non-MSAA resolve textures as primary targets, so all attachments agree on
    // sampleCount = 1 without any downstream special-casing.
    static HgiGraphicsCmdsDesc _FallbackToNonMSAAForIntegerTargets(
        HgiGraphicsCmdsDesc const& desc)
    {
        if (desc.colorResolveTextures.empty()) {
            return desc;
        }
        const auto* first = dynamic_cast<const HgiWebGPUTexture*>(
            desc.colorTextures.empty() ?
                nullptr : desc.colorTextures.front().Get());
        if (!first || first->GetDescriptor().sampleCount != 1) {
            return desc;
        }

        HgiGraphicsCmdsDesc newDesc = desc;
        newDesc.colorTextures = desc.colorResolveTextures;
        newDesc.colorResolveTextures.clear();
        if (newDesc.depthResolveTexture) {
            newDesc.depthTexture = newDesc.depthResolveTexture;
            newDesc.depthResolveTexture = {};
        }
        return newDesc;
    }

    HGIWEBGPU_API
    HgiWebGPUBaseGraphicsCmds(
        HgiWebGPU* hgi,
        HgiGraphicsCmdsDesc const& desc)
            : _hgi(hgi)
            , _descriptor(_FallbackToNonMSAAForIntegerTargets(desc))
            , _passEncoder(nullptr)
            , _commandEncoder(nullptr)
            , _commandBuffer(nullptr)
            , _pipeline(nullptr)
            , _renderPassStarted(false)
            , _pushConstantsDirty(false)
            , _hasWork(false)
    {
        _constantBindGroupEntry = {};
        _constantBindGroupEntry.size = 0;

        auto depthTarget = static_cast<HgiWebGPUTexture *>(_descriptor.depthTexture.Get());
        _renderPassStarted = depthTarget || !_descriptor.colorTextures.empty();
        _CreateCommandEncoder();
    }

    HGIWEBGPU_API
    bool _Submit(Hgi* hgi, HgiSubmitWaitType wait) override {
        // End render pass
        _EndRenderPass();

        HgiWebGPU *wgpuHgi = static_cast<HgiWebGPU *>(hgi);

        wgpuHgi->EnqueueCommandBuffer(_commandBuffer);
        wgpuHgi->QueueSubmit();

        _commandBuffer = nullptr;
        return _hasWork;

    }

    HGIWEBGPU_API
    virtual void _EndRenderPass() = 0;

    HGIWEBGPU_API
    void _BindResources() {
        if (!_pipeline) {
            TF_CODING_ERROR("No pipeline bound");
            return;
        }

        if (_resourceBindings) {
            HgiWebGPUResourceBindings *resourceBinding =
                    static_cast<HgiWebGPUResourceBindings *>(_resourceBindings.Get());
            wgpu::RenderPipeline pipelineHandle = _pipeline->GetPipeline();
            resourceBinding->BindResources(_hgi->GetPrimaryDevice(), _passEncoder,
                                           _pipeline->GetPipelineBindGroups(), _constantBindGroupEntry,
                                           _pushConstantsDirty);
            _pushConstantsDirty = false;
            _resourceBindings = HgiResourceBindingsHandle();
        }

        _hasWork = true;
        if (!_debugGroupLabels.empty())
            _lastDrawLabel = _debugGroupLabels.back();
    }

    HGIWEBGPU_API
    void _CreateCommandEncoder() {
        if (!_commandEncoder) {
            wgpu::Device device = _hgi->GetPrimaryDevice();
            _commandEncoder = device.CreateCommandEncoder();
            TF_VERIFY(_commandEncoder);
        }
    }

    HgiWebGPU* _hgi{};
    wgpu::BindGroupEntry _constantBindGroupEntry;
    HgiGraphicsCmdsDesc _descriptor;
    PassEncoder _passEncoder;
    wgpu::CommandEncoder _commandEncoder;
    wgpu::CommandBuffer _commandBuffer;
    wgpu::Buffer _lastIndexBuffer;
    HgiWebGPUGraphicsPipeline *_pipeline{};
    std::string _debugLabel;
    bool _renderPassStarted;
    bool _pushConstantsDirty{};
    HgiResourceBindingsHandle _resourceBindings;
    HgiWebGPUStepFunctions _stepFunctions;
    bool _hasWork;
    std::vector<std::string > _debugGroupLabels;
    std::string _lastDrawLabel;

private:
    HgiWebGPUBaseGraphicsCmds() = delete;
    HgiWebGPUBaseGraphicsCmds & operator=(const HgiWebGPUBaseGraphicsCmds&) = delete;
    HgiWebGPUBaseGraphicsCmds(const HgiWebGPUBaseGraphicsCmds&) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
