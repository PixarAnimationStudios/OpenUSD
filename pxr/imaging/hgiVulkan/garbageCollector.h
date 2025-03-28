//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIVULKAN_GARBAGE_COLLECTOR_H
#define PXR_IMAGING_HGIVULKAN_GARBAGE_COLLECTOR_H

#include "pxr/pxr.h"
#include "pxr/base/tf/diagnostic.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgiVulkan/api.h"

#include <mutex>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkan;
class HgiVulkanDevice;

using HgiVulkanBufferVector =
    std::vector<class HgiVulkanBuffer*>;
using HgiVulkanTextureVector =
    std::vector<class HgiVulkanTexture*>;
using HgiVulkanSamplerVector =
    std::vector<class HgiVulkanSampler*>;
using HgiVulkanShaderFunctionVector =
    std::vector<class HgiVulkanShaderFunction*>;
using HgiVulkanShaderProgramVector =
    std::vector<class HgiVulkanShaderProgram*>;
using HgiVulkanResourceBindingsVector =
    std::vector<class HgiVulkanResourceBindings*>;
using HgiVulkanGraphicsPipelineVector =
    std::vector<class HgiVulkanGraphicsPipeline*>;
using HgiVulkanComputePipelineVector =
    std::vector<class HgiVulkanComputePipeline*>;


/// \class HgiVulkanGarbageCollector
///
/// Handles garbage collection of vulkan objects by delaying their destruction
/// until those objects are no longer used.
///
class HgiVulkanGarbageCollector final
{
public:
    HGIVULKAN_API
    HgiVulkanGarbageCollector(HgiVulkan* hgi);

    HGIVULKAN_API
    ~HgiVulkanGarbageCollector();

    /// Destroys the objects inside the garbage collector.
    /// Thread safety: This call is not thread safe and should only be called
    /// while no other threads are destroying objects (e.g. during EndFrame).
    HGIVULKAN_API
    void PerformGarbageCollection(HgiVulkanDevice* device);

    /// Returns a garbage collection vector for a type of handle.
    /// Thread safety: The returned vector is a thread_local vector so this call
    /// is thread safe as long as the vector is only used by the calling thread.
    HGIVULKAN_API
    HgiVulkanBufferVector* GetBufferList();
    HGIVULKAN_API
    HgiVulkanTextureVector* GetTextureList();
    HGIVULKAN_API
    HgiVulkanSamplerVector* GetSamplerList();
    HGIVULKAN_API
    HgiVulkanShaderFunctionVector* GetShaderFunctionList();
    HGIVULKAN_API
    HgiVulkanShaderProgramVector* GetShaderProgramList();
    HGIVULKAN_API
    HgiVulkanResourceBindingsVector* GetResourceBindingsList();
    HGIVULKAN_API
    HgiVulkanGraphicsPipelineVector* GetGraphicsPipelineList();
    HGIVULKAN_API
    HgiVulkanComputePipelineVector* GetComputePipelineList();

private:
    HgiVulkanGarbageCollector & operator =
        (const HgiVulkanGarbageCollector&) = delete;
    HgiVulkanGarbageCollector(const HgiVulkanGarbageCollector&) = delete;

    /// Returns a thread_local vector in which to store a object handle.
    /// Thread safety: The returned vector is a thread_local vector so this call
    /// is thread safe as long as the vector is only used by the calling thread.
    template<class T>
    T* _GetThreadLocalStorageList(std::vector<T*>* collector);

    HgiVulkan* _hgi;

    // List of all the per-thread-vectors of objects that need to be destroyed.
    // The vectors are static (shared across HGIs), because we use thread_local
    // in _GetThreadLocalStorageList which makes us share the garbage collector
    // vectors across Hgi instances.
    static std::vector<HgiVulkanBufferVector*> _bufferList;
    static std::vector<HgiVulkanTextureVector*> _textureList;
    static std::vector<HgiVulkanSamplerVector*> _samplerList;
    static std::vector<HgiVulkanShaderFunctionVector*> _shaderFunctionList;
    static std::vector<HgiVulkanShaderProgramVector*> _shaderProgramList;
    static std::vector<HgiVulkanResourceBindingsVector*> _resourceBindingsList;
    static std::vector<HgiVulkanGraphicsPipelineVector*> _graphicsPipelineList;
    static std::vector<HgiVulkanComputePipelineVector*> _computePipelineList;

    bool _isDestroying;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
