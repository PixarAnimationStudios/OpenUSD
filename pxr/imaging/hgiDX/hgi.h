
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

#pragma once
#include "pxr/pxr.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/imaging/hgiDX/api.h"

#include <thread>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

struct HgiCustomInterop;

class HgiCapabilities;
class HgiDXDevice;
class HgiDXInstance;
class HgiDXPresentation;
class HgiDXIndirectCommandEncoder;
class HgiDXMemoryHelper;
class HgiDXTextureConverter;

using HgiDXGfxFunction = std::function<void(void)>;
using HgiDXGfxFunctionVector = std::vector<HgiDXGfxFunction>;

/// \class HgiDX
///
/// DirectX implementation of the Hydra Graphics Interface.
///
class HgiDX final : public Hgi
{
public:
    HGIDX_API
    HgiDX();

    HGIDX_API
    ~HgiDX() override;

    HGIDX_API
    bool IsBackendSupported() const override;

    HGIDX_API
    HgiGraphicsCmdsUniquePtr CreateGraphicsCmds(HgiGraphicsCmdsDesc const& desc) override;

    HGIDX_API
    HgiBlitCmdsUniquePtr CreateBlitCmds() override;

    HGIDX_API
    HgiComputeCmdsUniquePtr CreateComputeCmds(HgiComputeCmdsDesc const& desc) override;

    HGIDX_API
    HgiTextureHandle CreateTexture(HgiTextureDesc const & desc) override;

    HGIDX_API
    void DestroyTexture(HgiTextureHandle* texHandle) override;

    HGIDX_API
    HgiTextureViewHandle CreateTextureView(HgiTextureViewDesc const& desc) override;

    HGIDX_API
    void DestroyTextureView(HgiTextureViewHandle* viewHandle) override;

    HGIDX_API
    HgiSamplerHandle CreateSampler(HgiSamplerDesc const & desc) override;

    HGIDX_API
    void DestroySampler(HgiSamplerHandle* smpHandle) override;

    HGIDX_API
    HgiBufferHandle CreateBuffer(HgiBufferDesc const & desc) override;

    HGIDX_API
    void DestroyBuffer(HgiBufferHandle* bufHandle) override;

    HGIDX_API
    HgiShaderFunctionHandle CreateShaderFunction(HgiShaderFunctionDesc const& desc) override;

    HGIDX_API
    void DestroyShaderFunction(HgiShaderFunctionHandle* shaderFunctionHandle) override;

    HGIDX_API
    HgiShaderProgramHandle CreateShaderProgram(HgiShaderProgramDesc const& desc) override;

    HGIDX_API
    void DestroyShaderProgram(HgiShaderProgramHandle* shaderProgramHandle) override;

    HGIDX_API
    HgiResourceBindingsHandle CreateResourceBindings(HgiResourceBindingsDesc const& desc) override;

    HGIDX_API
    void DestroyResourceBindings(HgiResourceBindingsHandle* resHandle) override;

    HGIDX_API
    HgiGraphicsPipelineHandle CreateGraphicsPipeline(HgiGraphicsPipelineDesc const& pipeDesc) override;

    HGIDX_API
    void DestroyGraphicsPipeline(HgiGraphicsPipelineHandle* pipeHandle) override;

    HGIDX_API
    HgiComputePipelineHandle CreateComputePipeline(HgiComputePipelineDesc const& pipeDesc) override;

    HGIDX_API
    void DestroyComputePipeline(HgiComputePipelineHandle* pipeHandle) override;

    HGIDX_API
    TfToken const& GetAPIName() const override;

    HGIDX_API
    HgiCapabilities const* GetCapabilities() const override;
	
    HGIDX_API
    void StartFrame() override;

    HGIDX_API
    void EndFrame() override;

    HGIDX_API
    HgiIndirectCommandEncoder* GetIndirectCommandEncoder() const override;

    HGIDX_API
    HgiCustomInterop* GetCustomInterop() override;

    HGIDX_API
    virtual HgiMemoryHelper* GetMemoryHelper() override;

    //
    // HgiDX specific
    //

    /// Returns the primary (presentation) DirectX device.
    /// Thread safety: Yes.
    HGIDX_API
    HgiDXDevice* GetPrimaryDevice() const;

    HGIDX_API
    HgiDXPresentation* GetPresentation();
  
    HGIDX_API
    virtual HgiDXTextureConverter* GetTxConverter();

protected:
    HGIDX_API
    bool _SubmitCmds(HgiCmds* cmds, HgiSubmitWaitType wait) override;

private:
    HgiDX & operator=(const HgiDX&) = delete;
    HgiDX(const HgiDX&) = delete;

    // Perform low frequency actions, such as garbage collection.
    // Thread safety: No. Must be called from main thread.
    void _EndFrameSync();

    std::unique_ptr<HgiDXDevice> _device;
    std::thread::id _threadId;
    int _frameDepth;
    std::unique_ptr<HgiDXPresentation> _presentation;
    std::unique_ptr<HgiDXIndirectCommandEncoder> _indirectEncoder;
    std::unique_ptr<HgiDXMemoryHelper> _memHelper;
    std::unique_ptr<HgiDXTextureConverter> _txConverter;
};

PXR_NAMESPACE_CLOSE_SCOPE
