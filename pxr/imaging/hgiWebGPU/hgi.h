//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_WEBGPU_HGIWEBGPU_H
#define PXR_IMAGING_HGI_WEBGPU_HGIWEBGPU_H

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgiWebGPU/capabilities.h"
#include "pxr/imaging/hgiWebGPU/depthResolver.h"
#include "pxr/imaging/hgiWebGPU/mipmapGenerator.h"
#include "pxr/pxr.h"
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

using HgiWebGPUCallback = std::function<void(void)>;

struct QueryFrame {
  wgpu::Buffer resultBuffer;
  wgpu::Buffer resolveBuffer;
  wgpu::QuerySet querySet;
  std::shared_ptr<uint64_t> id;
  std::string label;
};

static size_t queryFrameCounter = 0;
/// \class HgiWebGPU
///
/// WebGPU implementation of the Hydra Graphics Interface.
///
class HgiWebGPU final : public Hgi {
public:
  HGIWEBGPU_API
  HgiWebGPU();

  HGIWEBGPU_API
  ~HgiWebGPU() override;

  HGIWEBGPU_API
  bool IsBackendSupported() const override;

  HGIWEBGPU_API
  HgiGraphicsCmdsUniquePtr
  CreateGraphicsCmds(HgiGraphicsCmdsDesc const &desc) override;

  HGIWEBGPU_API
  HgiComputeCmdsUniquePtr
  CreateComputeCmds(HgiComputeCmdsDesc const &desc) override;

  HGIWEBGPU_API
  HgiBlitCmdsUniquePtr CreateBlitCmds() override;

  HGIWEBGPU_API
  void DestroyTexture(HgiTextureHandle *texHandle) override;

  HGIWEBGPU_API
  void DestroyTextureView(HgiTextureViewHandle *viewHandle) override;

  HGIWEBGPU_API
  HgiSamplerHandle CreateSampler(HgiSamplerDesc const &desc) override;

  HGIWEBGPU_API
  void DestroySampler(HgiSamplerHandle *smpHandle) override;

  HGIWEBGPU_API
  void DestroyBuffer(HgiBufferHandle *texHandle) override;

  HGIWEBGPU_API
  HgiShaderFunctionHandle
  CreateShaderFunction(HgiShaderFunctionDesc const &desc) override;

  HGIWEBGPU_API
  void
  DestroyShaderFunction(HgiShaderFunctionHandle *shaderFunctionHandle) override;

  HGIWEBGPU_API
  HgiShaderProgramHandle
  CreateShaderProgram(HgiShaderProgramDesc const &desc) override;

  HGIWEBGPU_API
  void
  DestroyShaderProgram(HgiShaderProgramHandle *shaderProgramHandle) override;

  HGIWEBGPU_API
  void DestroyResourceBindings(HgiResourceBindingsHandle *resHandle) override;

  HGIWEBGPU_API
  HgiGraphicsPipelineHandle
  CreateGraphicsPipeline(HgiGraphicsPipelineDesc const &pipeDesc) override;

  HGIWEBGPU_API
  void DestroyGraphicsPipeline(HgiGraphicsPipelineHandle *pipeHandle) override;

  HGIWEBGPU_API
  HgiComputePipelineHandle
  CreateComputePipeline(HgiComputePipelineDesc const &pipeDesc) override;

  HGIWEBGPU_API
  void DestroyComputePipeline(HgiComputePipelineHandle *pipeHandle) override;

  HGIWEBGPU_API
  TfToken const &GetAPIName() const override;

  HGIWEBGPU_API
  HgiWebGPUCapabilities const *GetCapabilities() const override;

  HGIWEBGPU_API
  HgiIndirectCommandEncoder *GetIndirectCommandEncoder() const override;

  HGIWEBGPU_API
  void StartFrame() override;

  HGIWEBGPU_API
  void EndFrame() override;

  HGIWEBGPU_API
  void GarbageCollect() override;

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
  wgpu::Texture GenerateMipmap(const wgpu::Texture &texture,
                               const HgiTextureDesc &textureDescriptor);

  HGIWEBGPU_API
  void ResolveDepth(wgpu::CommandEncoder const &commandEncoder,
                    HgiWebGPUTexture &sourceTexture,
                    HgiWebGPUTexture &destinationTexture);
#if !defined(EMSCRIPTEN)
  HGIWEBGPU_API
  void QueryValue();

  HGIWEBGPU_API
  wgpu::PassTimestampWrites GetRenderTimestampWrites();

  HGIWEBGPU_API
  void ResolveQuery(wgpu::CommandEncoder &commandEncoder,
                    const std::string &label);
#endif
protected:
  HGIWEBGPU_API
  bool _SubmitCmds(HgiCmds *cmds, HgiSubmitWaitType wait) override;

  HGIWEBGPU_API
  HgiTextureHandle _CreateTexture(HgiTextureDesc const &desc) override;

  HGIWEBGPU_API
  HgiTextureViewHandle
  _CreateTextureView(HgiTextureViewDesc const &desc) override;

  HGIWEBGPU_API
  HgiBufferHandle _CreateBuffer(HgiBufferDesc const &desc) override;

  HGIWEBGPU_API
  HgiResourceBindingsHandle
  _CreateResourceBindings(HgiResourceBindingsDesc const &desc) override;

private:
  HgiWebGPU &operator=(const HgiWebGPU &) = delete;
  HgiWebGPU(const HgiWebGPU &) = delete;
  void _PerformGarbageCollection();

  // Invalidates the resource handle and destroys the object.
  template <class T> void _TrashObject(HgiHandle<T> *handle) {
    delete handle->Get();
    *handle = HgiHandle<T>();
  }
#if !defined(EMSCRIPTEN)
  QueryFrame _CreateQueryObjects();
  void _ProcessNextInflightQuery();
#endif

  wgpu::Device _device;
  wgpu::Queue _commandQueue;
  HgiWebGPUDepthResolver _depthResolver;
  HgiWebGPUMipmapGenerator _mipmapGenerator;

  std::unique_ptr<HgiWebGPUCapabilities> _capabilities;
  std::vector<HgiWebGPUCallback> _garbageCollectionHandlers;
  std::vector<HgiWebGPUCallback> _preSubmitHandlers;
  std::vector<wgpu::CommandBuffer> _commandBuffers;

  bool _workToFlush;

  std::vector<QueryFrame> _availableQueries;
  std::shared_ptr<QueryFrame> _inflightQuery = nullptr;
  std::unordered_map<uint64_t, QueryFrame> _pendingQueries;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
