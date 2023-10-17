//
// Copyright 2022 Pixar
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
#ifndef PXR_IMAGING_HGI_WEBGPU_HGIWEBGPU_H
#define PXR_IMAGING_HGI_WEBGPU_HGIWEBGPU_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgiWebGPU/capabilities.h"
#include "pxr/imaging/hgiWebGPU/depthResolver.h"
#include "pxr/imaging/hgiWebGPU/mipmapGenerator.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

using HgiWebGPUCallback = std::function<void(void)>;

/// \class HgiWebGPU
///
/// WebGPU implementation of the Hydra Graphics Interface.
///
class HgiWebGPU final : public Hgi
{
public:
    HGIWEBGPU_API
    HgiWebGPU();

    HGIWEBGPU_API
    ~HgiWebGPU() override;

    HGIWEBGPU_API
    bool IsBackendSupported() const override;

    HGIWEBGPU_API
    HgiGraphicsCmdsUniquePtr CreateGraphicsCmds(
        HgiGraphicsCmdsDesc const& desc) override;
    
    HGIWEBGPU_API
    HgiComputeCmdsUniquePtr CreateComputeCmds(
            HgiComputeCmdsDesc const& desc) override;

    HGIWEBGPU_API
    HgiBlitCmdsUniquePtr CreateBlitCmds() override;

    HGIWEBGPU_API
    HgiTextureHandle CreateTexture(HgiTextureDesc const & desc) override;

    HGIWEBGPU_API
    void DestroyTexture(HgiTextureHandle* texHandle) override;

    HGIWEBGPU_API
    HgiTextureViewHandle CreateTextureView(
        HgiTextureViewDesc const& desc) override;

    HGIWEBGPU_API
    void DestroyTextureView(HgiTextureViewHandle* viewHandle) override;

    HGIWEBGPU_API
    HgiSamplerHandle CreateSampler(HgiSamplerDesc const & desc) override;

    HGIWEBGPU_API
    void DestroySampler(HgiSamplerHandle* smpHandle) override;

    HGIWEBGPU_API
    HgiBufferHandle CreateBuffer(HgiBufferDesc const & desc) override;

    HGIWEBGPU_API
    void DestroyBuffer(HgiBufferHandle* texHandle) override;

    HGIWEBGPU_API
    HgiShaderFunctionHandle CreateShaderFunction(
        HgiShaderFunctionDesc const& desc) override;

    HGIWEBGPU_API
    void DestroyShaderFunction(
        HgiShaderFunctionHandle* shaderFunctionHandle) override;

    HGIWEBGPU_API
    HgiShaderProgramHandle CreateShaderProgram(
        HgiShaderProgramDesc const& desc) override;

    HGIWEBGPU_API
    void DestroyShaderProgram(
        HgiShaderProgramHandle* shaderProgramHandle) override;

    HGIWEBGPU_API
    HgiResourceBindingsHandle CreateResourceBindings(
        HgiResourceBindingsDesc const& desc) override;

    HGIWEBGPU_API
    void DestroyResourceBindings(HgiResourceBindingsHandle* resHandle) override;

    HGIWEBGPU_API
    HgiGraphicsPipelineHandle CreateGraphicsPipeline(
        HgiGraphicsPipelineDesc const& pipeDesc) override;

    HGIWEBGPU_API
    void DestroyGraphicsPipeline(
        HgiGraphicsPipelineHandle* pipeHandle) override;

    HGIWEBGPU_API
    HgiComputePipelineHandle CreateComputePipeline(
        HgiComputePipelineDesc const& pipeDesc) override;

    HGIWEBGPU_API
    void DestroyComputePipeline(HgiComputePipelineHandle* pipeHandle) override;

    HGIWEBGPU_API
    TfToken const& GetAPIName() const override;

    HGIWEBGPU_API
    HgiWebGPUCapabilities const* GetCapabilities() const override;

    HGIWEBGPU_API
    HgiIndirectCommandEncoder* GetIndirectCommandEncoder() const override;

    HGIWEBGPU_API
    void StartFrame() override;

    HGIWEBGPU_API
    void EndFrame() override;

    HGIWEBGPU_API
    wgpu::Device GetPrimaryDevice() const;

    HGIWEBGPU_API
    wgpu::Queue GetQueue() const;

    HGIWEBGPU_API
    void EnqueueCommandBuffer(wgpu::CommandBuffer const &commandBuffer);

    HGIWEBGPU_API
    void QueueSubmit();

    HGIWEBGPU_API
    int GetAPIVersion() const;

    HGIWEBGPU_API
    wgpu::Texture GenerateMipmap(const wgpu::Texture& texture, const HgiTextureDesc& textureDescriptor);

    HGIWEBGPU_API
    void ResolveDepth(wgpu::CommandEncoder const &commandEncoder, HgiWebGPUTexture &sourceTexture,
                                              HgiWebGPUTexture &destinationTexture);

protected:
    HGIWEBGPU_API
    bool _SubmitCmds(HgiCmds* cmds, HgiSubmitWaitType wait) override;

private:
    HgiWebGPU & operator=(const HgiWebGPU&) = delete;
    HgiWebGPU(const HgiWebGPU&) = delete;
    void _PerformGarbageCollection();

    // Invalidates the resource handle and destroys the object.
    template<class T>
    void _TrashObject(HgiHandle<T>* handle) {
        delete handle->Get();
        *handle = HgiHandle<T>();
    }

    wgpu::Device _device;
    wgpu::Queue _commandQueue;
    HgiCmds* _currentCmds;
    HgiWebGPUDepthResolver _depthResolver;
    HgiWebGPUMipmapGenerator _mipmapGenerator;

    std::unique_ptr<HgiWebGPUCapabilities> _capabilities;
    std::vector<HgiWebGPUCallback> _garbageCollectionHandlers;
    std::vector<HgiWebGPUCallback> _preSubmitHandlers;
    std::vector<wgpu::CommandBuffer> _commandBuffers;

    bool _workToFlush;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
