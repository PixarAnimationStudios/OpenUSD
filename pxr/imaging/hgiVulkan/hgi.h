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
#ifndef PXR_IMAGING_HGIVULKAN_HGI_H
#define PXR_IMAGING_HGIVULKAN_HGI_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/commandQueue.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"

#include <thread>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkanGarbageCollector;
class HgiVulkanInstance;


/// \class HgiVulkan
///
/// Vulkan implementation of the Hydra Graphics Interface.
///
class HgiVulkan final : public Hgi
{
public:
    HGIVULKAN_API
    HgiVulkan();

    HGIVULKAN_API
    ~HgiVulkan() override;

    HGIVULKAN_API
    HgiGraphicsCmdsUniquePtr CreateGraphicsCmds(
        HgiGraphicsCmdsDesc const& desc) override;

    HGIVULKAN_API
    HgiBlitCmdsUniquePtr CreateBlitCmds() override;

    HGIVULKAN_API
    HgiComputeCmdsUniquePtr CreateComputeCmds() override;

    HGIVULKAN_API
    HgiTextureHandle CreateTexture(HgiTextureDesc const & desc) override;

    HGIVULKAN_API
    void DestroyTexture(HgiTextureHandle* texHandle) override;

    HGIVULKAN_API
    HgiTextureViewHandle CreateTextureView(
        HgiTextureViewDesc const& desc) override;

    HGIVULKAN_API
    void DestroyTextureView(HgiTextureViewHandle* viewHandle) override;

    HGIVULKAN_API
    HgiSamplerHandle CreateSampler(HgiSamplerDesc const & desc) override;

    HGIVULKAN_API
    void DestroySampler(HgiSamplerHandle* smpHandle) override;

    HGIVULKAN_API
    HgiBufferHandle CreateBuffer(HgiBufferDesc const & desc) override;

    HGIVULKAN_API
    void DestroyBuffer(HgiBufferHandle* bufHandle) override;

    HGIVULKAN_API
    HgiShaderFunctionHandle CreateShaderFunction(
        HgiShaderFunctionDesc const& desc) override;

    HGIVULKAN_API
    void DestroyShaderFunction(
        HgiShaderFunctionHandle* shaderFunctionHandle) override;

    HGIVULKAN_API
    HgiShaderProgramHandle CreateShaderProgram(
        HgiShaderProgramDesc const& desc) override;

    HGIVULKAN_API
    void DestroyShaderProgram(
        HgiShaderProgramHandle* shaderProgramHandle) override;

    HGIVULKAN_API
    HgiResourceBindingsHandle CreateResourceBindings(
        HgiResourceBindingsDesc const& desc) override;

    HGIVULKAN_API
    void DestroyResourceBindings(HgiResourceBindingsHandle* resHandle) override;

    HGIVULKAN_API
    HgiGraphicsPipelineHandle CreateGraphicsPipeline(
        HgiGraphicsPipelineDesc const& pipeDesc) override;

    HGIVULKAN_API
    void DestroyGraphicsPipeline(
        HgiGraphicsPipelineHandle* pipeHandle) override;

    HGIVULKAN_API
    HgiComputePipelineHandle CreateComputePipeline(
        HgiComputePipelineDesc const& pipeDesc) override;

    HGIVULKAN_API
    void DestroyComputePipeline(HgiComputePipelineHandle* pipeHandle) override;

    HGIVULKAN_API
    TfToken const& GetAPIName() const override;

    HGIVULKAN_API
    void StartFrame() override;

    HGIVULKAN_API
    void EndFrame() override;

    //
    // HgiVulkan specific
    //

    /// Returns the Hgi vulkan instance.
    /// Thread safety: Yes.
    HGIVULKAN_API
    HgiVulkanInstance* GetVulkanInstance() const;

    /// Returns the primary (presentation) vulkan device.
    /// Thread safety: Yes.
    HGIVULKAN_API
    HgiVulkanDevice* GetPrimaryDevice() const;

    /// Returns the garbage collector.
    /// Thread safety: Yes.
    HGIVULKAN_API
    HgiVulkanGarbageCollector* GetGarbageCollector() const;

    /// Invalidates the resource handle and places the object in the garbage
    /// collector vector for future destruction.
    /// This is helpful to avoid destroying GPU resources still in-flight.
    template<class T, class H>
    void TrashObject(H* handle, std::vector<T*>* collector)
    {
        T* object = static_cast<T*>(handle->Get());
        HgiVulkanDevice* device = object->GetDevice();
        HgiVulkanCommandQueue* queue = device->GetCommandQueue();
        object->GetInflightBits() = queue->GetInflightCommandBuffersBits();
        collector->push_back(object);
        *handle = H();
    }

protected:
    HGIVULKAN_API
    bool _SubmitCmds(HgiCmds* cmds, HgiSubmitWaitType wait) override;

private:
    HgiVulkan & operator=(const HgiVulkan&) = delete;
    HgiVulkan(const HgiVulkan&) = delete;

    // Perform low frequency actions, such as garbage collection.
    // Thread safety: No. Must be called from main thread.
    void _EndFrameSync();

    HgiVulkanInstance* _instance;
    HgiVulkanDevice* _device;
    HgiVulkanGarbageCollector* _garbageCollector;
    std::thread::id _threadId;
    int _frameDepth;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
