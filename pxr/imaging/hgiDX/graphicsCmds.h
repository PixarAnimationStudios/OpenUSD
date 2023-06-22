
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
#include "pxr/base/gf/vec4i.h"
#include "pxr/imaging/hgi/graphicsCmds.h"
#include "pxr/imaging/hgiDX/api.h"
#include "pxr/imaging/hgiDX/hgi.h"
#include <cstdint>
#include <functional>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

struct HgiGraphicsCmdsDesc;
class HgiDX;
class HgiDXBuffer;
class HgiDXCommandBuffer;
class HgiDXGraphicsPipeline;
class HgiDXTexture;


/// \class HgiDXGraphicsCmds
///
/// DirectX implementation of HgiGraphicsEncoder.
///
class HgiDXGraphicsCmds final : public HgiGraphicsCmds
{
public:
    HGIDX_API
    ~HgiDXGraphicsCmds() override;

    HGIDX_API
    void PushDebugGroup(const char* label) override;

    HGIDX_API
    void PopDebugGroup() override;

    HGIDX_API
    void SetViewport(GfVec4i const& vp) override;

    HGIDX_API
    void SetScissor(GfVec4i const& sc) override;

    HGIDX_API
    void BindPipeline(HgiGraphicsPipelineHandle pipeline) override;

    HGIDX_API
    void BindResources(HgiResourceBindingsHandle resources) override;

    HGIDX_API
    void SetConstantValues(HgiGraphicsPipelineHandle pipeline,
                           HgiShaderStage stages,
                           uint32_t bindIndex,
                           uint32_t byteSize,
                           const void* data) override;

    HGIDX_API
    void BindVertexBuffers(HgiVertexBufferBindingVector const& bindings) override;

    HGIDX_API
    void Draw(uint32_t vertexCount,
              uint32_t baseVertex,
              uint32_t instanceCount,
              uint32_t baseInstance) override;

    HGIDX_API
    void DrawIndirect(HgiBufferHandle const& drawParameterBuffer,
                      uint32_t drawBufferByteOffset,
                      uint32_t drawCount,
                      uint32_t stride) override;

    HGIDX_API
    void DrawIndexed(HgiBufferHandle const& indexBuffer,
                     uint32_t indexCount,
                     uint32_t indexBufferByteOffset,
                     uint32_t baseVertex,
                     uint32_t instanceCount,
                     uint32_t baseInstance) override;

    HGIDX_API
    void DrawIndexedIndirect(HgiBufferHandle const& indexBuffer,
                             HgiBufferHandle const& drawParameterBuffer,
                             uint32_t drawBufferByteOffset,
                             uint32_t drawCount,
                             uint32_t stride,
                             std::vector<uint32_t> const& drawParameterBufferUInt32,
                             uint32_t patchBaseVertexByteOffset) override;

    HGIDX_API
    void InsertMemoryBarrier(HgiMemoryBarrier barrier) override;

protected:
    friend class HgiDX;

    HGIDX_API
    HgiDXGraphicsCmds(HgiDX* hgi, HgiGraphicsCmdsDesc const& desc);
    bool _Submit(Hgi* hgi, HgiSubmitWaitType wait) override;


private:
    HgiDXGraphicsCmds() = delete;
    HgiDXGraphicsCmds & operator=(const HgiDXGraphicsCmds&) = delete;
    HgiDXGraphicsCmds(const HgiDXGraphicsCmds&) = delete;
    
    static HgiDXGfxFunction _ClearRenderTargetsOp(HgiDX* pHgi,
                                                  const HgiGraphicsCmdsDesc& desc,
                                                  std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& renderTargetDescs,
                                                  D3D12_CPU_DESCRIPTOR_HANDLE& dsvDesc,
                                                  std::map<HgiDXTexture*, HgiDXTexture*>& mapRenderTarget2ColorResolveTx);
    
    static HgiDXGfxFunction _SetupRenderTargetsOp(HgiDX* pHgi,
                                                  std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& renderTargetDescs,
                                                  D3D12_CPU_DESCRIPTOR_HANDLE& dsvDesc);
    static HgiDXGfxFunction _BindVertexBuffersOp(HgiDX* pHgi,
                                                 HgiDXGraphicsPipeline* pPipeline,
                                                 const HgiVertexBufferBindingVector& vertBindings);

    static HgiDXGfxFunction _SetupViewportOp(HgiDX* pHgi, D3D12_VIEWPORT* vp, D3D12_RECT* sc);

    static HgiDXGfxFunction _BindRootParamsOp(HgiDX* pHgi, 
                                              HgiDXGraphicsPipeline* pPipeline, 
                                              const HgiResourceBindingsDesc& resBindings);

    void _ApplyPendingUpdates();


private:
    HgiDX* _hgi;
    HgiGraphicsCmdsDesc _descriptor;
    HgiDXGraphicsPipeline* _pPipeline;
    HgiResourceBindingsHandle _resBindings;
    HgiVertexBufferBindingVector const* _vertBindings = nullptr;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> _renderTargetDescs;
    D3D12_CPU_DESCRIPTOR_HANDLE _dsvDesc;
    std::map<HgiDXTexture*, HgiDXTexture*> _mapRenderTarget2ColorResolveTx;

    bool _bViewportSet = false;
    bool _bScissorsSet = false;
    D3D12_VIEWPORT m_screenViewport;
    D3D12_RECT m_scissorRect;

    HgiDXGfxFunctionVector _ops;
};

PXR_NAMESPACE_CLOSE_SCOPE

