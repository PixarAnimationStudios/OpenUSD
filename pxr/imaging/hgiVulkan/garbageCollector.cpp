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
#include "pxr/imaging/hgiVulkan/buffer.h"
#include "pxr/imaging/hgiVulkan/commandQueue.h"
#include "pxr/imaging/hgiVulkan/computePipeline.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/garbageCollector.h"
#include "pxr/imaging/hgiVulkan/graphicsPipeline.h"
#include "pxr/imaging/hgiVulkan/hgi.h"
#include "pxr/imaging/hgiVulkan/resourceBindings.h"
#include "pxr/imaging/hgiVulkan/sampler.h"
#include "pxr/imaging/hgiVulkan/shaderFunction.h"
#include "pxr/imaging/hgiVulkan/shaderProgram.h"
#include "pxr/imaging/hgiVulkan/texture.h"

#include "pxr/base/arch/hints.h"
#include "pxr/base/tf/diagnostic.h"


PXR_NAMESPACE_OPEN_SCOPE

std::vector<HgiVulkanBufferVector*> 
    HgiVulkanGarbageCollector::_bufferList;
std::vector<HgiVulkanTextureVector*> 
    HgiVulkanGarbageCollector::_textureList;
std::vector<HgiVulkanSamplerVector*> 
    HgiVulkanGarbageCollector::_samplerList;
std::vector<HgiVulkanShaderFunctionVector*> 
    HgiVulkanGarbageCollector::_shaderFunctionList;
std::vector<HgiVulkanShaderProgramVector*> 
    HgiVulkanGarbageCollector::_shaderProgramList;
std::vector<HgiVulkanResourceBindingsVector*> 
    HgiVulkanGarbageCollector::_resourceBindingsList;
std::vector<HgiVulkanGraphicsPipelineVector*> 
    HgiVulkanGarbageCollector::_graphicsPipelineList;
std::vector<HgiVulkanComputePipelineVector*> 
    HgiVulkanGarbageCollector::_computePipelineList;


template<class T>
static void _EmptyTrash(
    std::vector<std::vector<T*>*>* list,
    VkDevice vkDevice,
    uint64_t queueInflightBits)
{
    // Loop the garbage vectors of each thread
    for (auto vec : *list) {
        for (size_t i=vec->size(); i-- > 0;) {
            T* object = (*vec)[i];

            // Each device has its own queue, so its own set of inflight bits.
            // We must only destroy objects that belong to this device & queue.
            // (The garbage collector collects objects from all devices)
            if (vkDevice != object->GetDevice()->GetVulkanDevice()) {
                continue;
            }

            // See comments in PerformGarbageCollection.
            if ((queueInflightBits & object->GetInflightBits()) == 0) {
                delete object;
                std::iter_swap(vec->begin() + i, vec->end() - 1);
                vec->pop_back();
            }
        }
    }
}

HgiVulkanGarbageCollector::HgiVulkanGarbageCollector(HgiVulkan* hgi)
    : _hgi(hgi)
    , _isDestroying(false)
{
}

HgiVulkanGarbageCollector::~HgiVulkanGarbageCollector() = default;

/* Multi threaded */
HgiVulkanBufferVector*
HgiVulkanGarbageCollector::GetBufferList()
{
    return _GetThreadLocalStorageList(&_bufferList);
}

/* Multi threaded */
HgiVulkanTextureVector*
HgiVulkanGarbageCollector::GetTextureList()
{
    return _GetThreadLocalStorageList(&_textureList);
}

/* Multi threaded */
HgiVulkanSamplerVector*
HgiVulkanGarbageCollector::GetSamplerList()
{
    return _GetThreadLocalStorageList(&_samplerList);
}

/* Multi threaded */
HgiVulkanShaderFunctionVector*
HgiVulkanGarbageCollector::GetShaderFunctionList()
{
    return _GetThreadLocalStorageList(&_shaderFunctionList);
}

/* Multi threaded */
HgiVulkanShaderProgramVector*
HgiVulkanGarbageCollector::GetShaderProgramList()
{
    return _GetThreadLocalStorageList(&_shaderProgramList);
}

/* Multi threaded */
HgiVulkanResourceBindingsVector*
HgiVulkanGarbageCollector::GetResourceBindingsList()
{
    return _GetThreadLocalStorageList(&_resourceBindingsList);
}

/* Multi threaded */
HgiVulkanGraphicsPipelineVector*
HgiVulkanGarbageCollector::GetGraphicsPipelineList()
{
    return _GetThreadLocalStorageList(&_graphicsPipelineList);
}

/* Multi threaded */
HgiVulkanComputePipelineVector*
HgiVulkanGarbageCollector::GetComputePipelineList()
{
    return _GetThreadLocalStorageList(&_computePipelineList);
}

/* Single threaded */
void
HgiVulkanGarbageCollector::PerformGarbageCollection(HgiVulkanDevice* device)
{
    // Garbage Collection notes:
    //
    // When the client requests objects to be destroyed (Eg. Hgi::DestroyBuffer)
    // we put objects into this garbage collector. At that time we also store
    // the bits of the command buffers that are 'in-flight'.
    // We have to delay destroying the vulkan resources until there are no
    // command buffers using the resource.
    // Instead of tracking complex dependencies between objects and cmd buffers
    // we simply assume that all in-flight command buffers might be using the
    // destroyed object and wait until those command buffers have been
    // consumed by the GPU.
    //
    // In _EmptyTrash we try to delete objects in the garbage collector.
    // We compare the bits of the queue and the object to decide if we can
    // delete the object. Example:
    //
    //    Each command buffer takes up one bit (where 1 means "in-flight").
    //    Queue currently in-flight cmd buf bits:   01001011101
    //    In-flight bits when obj was trashed:      00100000100
    //    Bitwise & result:                         00000000100
    //
    // Conclusion: object cannot yet be destroyed. One command buffer that was
    // in-flight during the destruction request is still in-flight and might
    // still be using the object on the GPU.

    _isDestroying = true;

    // Check what command buffers are in-flight on the device queue.
    HgiVulkanCommandQueue* queue = device->GetCommandQueue();
    uint64_t queueBits = queue->GetInflightCommandBuffersBits();
    VkDevice vkDevice = device->GetVulkanDevice();

    _EmptyTrash(&_bufferList, vkDevice, queueBits);
    _EmptyTrash(&_textureList, vkDevice, queueBits);
    _EmptyTrash(&_samplerList, vkDevice, queueBits);
    _EmptyTrash(&_shaderFunctionList, vkDevice, queueBits);
    _EmptyTrash(&_shaderProgramList, vkDevice, queueBits);
    _EmptyTrash(&_resourceBindingsList, vkDevice, queueBits);
    _EmptyTrash(&_graphicsPipelineList, vkDevice, queueBits);
    _EmptyTrash(&_computePipelineList, vkDevice, queueBits);

    _isDestroying = false;
}

template<class T>
T* HgiVulkanGarbageCollector::_GetThreadLocalStorageList(std::vector<T*>* collector)
{
    if (ARCH_UNLIKELY(_isDestroying)) {
        TF_CODING_ERROR("Cannot destroy object during garbage collection ");
        while(_isDestroying);
    }

    // Only lock and create a new garbage vector if we dont have one in TLS.
    // Using TLS means this we store per type T, not per T and Hgi instance.
    // So if you call garbage collect on one Hgi, it destroys objects across
    // all Hgi's. This should be ok since we only call the destructor of the
    // garbage object.
    thread_local T* _tls = nullptr;
    static std::mutex garbageMutex;

    if (!_tls) {
        _tls = new T();
        std::lock_guard<std::mutex> guard(garbageMutex);
        collector->push_back(_tls);
    }
    return _tls;
}


PXR_NAMESPACE_CLOSE_SCOPE
