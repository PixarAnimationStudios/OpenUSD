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
#ifndef PXR_IMAGING_HGI_METAL_HGIMETAL_H
#define PXR_IMAGING_HGI_METAL_HGIMETAL_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiMetal/api.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#import <Metal/Metal.h>

PXR_NAMESPACE_OPEN_SCOPE

class HgiMetalCapabilities;

enum {
    APIVersion_Metal1_0 = 0,
    APIVersion_Metal2_0,
    APIVersion_Metal3_0
};

/// \class HgiMetal
///
/// Metal implementation of the Hydra Graphics Interface.
///
class HgiMetal final : public Hgi
{
public:
    enum CommitCommandBufferWaitType {
        CommitCommandBuffer_NoWait = 0,
        CommitCommandBuffer_WaitUntilScheduled,
        CommitCommandBuffer_WaitUntilCompleted,
    };
    
    HGIMETAL_API
    HgiMetal(id<MTLDevice> device = nil);

    HGIMETAL_API
    ~HgiMetal() override;

    HGIMETAL_API
    HgiGraphicsCmdsUniquePtr CreateGraphicsCmds(
        HgiGraphicsCmdsDesc const& desc) override;
    
    HGIMETAL_API
    HgiComputeCmdsUniquePtr CreateComputeCmds() override;

    HGIMETAL_API
    HgiBlitCmdsUniquePtr CreateBlitCmds() override;

    HGIMETAL_API
    HgiTextureHandle CreateTexture(HgiTextureDesc const & desc) override;

    HGIMETAL_API
    void DestroyTexture(HgiTextureHandle* texHandle) override;

    HGIMETAL_API
    HgiSamplerHandle CreateSampler(HgiSamplerDesc const & desc) override;

    HGIMETAL_API
    void DestroySampler(HgiSamplerHandle* smpHandle) override;

    HGIMETAL_API
    HgiBufferHandle CreateBuffer(HgiBufferDesc const & desc) override;

    HGIMETAL_API
    void DestroyBuffer(HgiBufferHandle* texHandle) override;

    HGIMETAL_API
    HgiShaderFunctionHandle CreateShaderFunction(
        HgiShaderFunctionDesc const& desc) override;

    HGIMETAL_API
    void DestroyShaderFunction(
        HgiShaderFunctionHandle* shaderFunctionHandle) override;

    HGIMETAL_API
    HgiShaderProgramHandle CreateShaderProgram(
        HgiShaderProgramDesc const& desc) override;

    HGIMETAL_API
    void DestroyShaderProgram(
        HgiShaderProgramHandle* shaderProgramHandle) override;

    HGIMETAL_API
    HgiResourceBindingsHandle CreateResourceBindings(
        HgiResourceBindingsDesc const& desc) override;

    HGIMETAL_API
    void DestroyResourceBindings(HgiResourceBindingsHandle* resHandle) override;

    HGIMETAL_API
    HgiGraphicsPipelineHandle CreateGraphicsPipeline(
        HgiGraphicsPipelineDesc const& pipeDesc) override;

    HGIMETAL_API
    void DestroyGraphicsPipeline(
        HgiGraphicsPipelineHandle* pipeHandle) override;

    HGIMETAL_API
    HgiComputePipelineHandle CreateComputePipeline(
        HgiComputePipelineDesc const& pipeDesc) override;

    HGIMETAL_API
    void DestroyComputePipeline(HgiComputePipelineHandle* pipeHandle) override;

    HGIMETAL_API
    TfToken const& GetAPIName() const override;
    
    HGIMETAL_API
    void StartFrame() override;

    HGIMETAL_API
    void EndFrame() override;

    //
    // HgiMetal specific
    //

    /// Returns the primary Metal device.
    HGIMETAL_API
    id<MTLDevice> GetPrimaryDevice() const;

    HGIMETAL_API
    id<MTLCommandQueue> GetQueue() const {
        return _commandQueue;
    }
    
    HGIMETAL_API
    id<MTLCommandBuffer> GetCommandBuffer(bool flush = true) {
        if (flush) {
            _workToFlush = true;
        }
        return _commandBuffer;
    }
    
    HGIMETAL_API
    int GetAPIVersion() const {
        return _apiVersion;
    }
    
    HGIMETAL_API
    HgiMetalCapabilities const & GetCapabilities() const {
        return *_capabilities;
    }
    
    HGIMETAL_API
    void CommitCommandBuffer(
        CommitCommandBufferWaitType waitType = CommitCommandBuffer_NoWait,
        bool forceNewBuffer = false);

protected:
    HGIMETAL_API
    bool _SubmitCmds(HgiCmds* cmds) override;

private:
    HgiMetal & operator=(const HgiMetal&) = delete;
    HgiMetal(const HgiMetal&) = delete;

    // Invalidates the resource handle and destroys the object.
    // Metal's internal garbage collection will handle the rest.
    template<class T>
    void _TrashObject(HgiHandle<T>* handle) {
        delete handle->Get();
        *handle = HgiHandle<T>();
    }

    id<MTLDevice> _device;
    id<MTLCommandQueue> _commandQueue;
    id<MTLCommandBuffer> _commandBuffer;
    id<MTLCaptureScope> _captureScopeFullFrame;

    std::unique_ptr<HgiMetalCapabilities> _capabilities;

    int _frameDepth;
    int _apiVersion;
    bool _workToFlush;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
