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
#include "pxr/imaging/hgiGL/garbageCollector.h"
#include "pxr/imaging/hgiGL/hgi.h"

#include "pxr/base/arch/hints.h"
#include "pxr/base/tf/diagnostic.h"


PXR_NAMESPACE_OPEN_SCOPE

std::vector<HgiBufferHandleVector*>
    HgiGLGarbageCollector::_bufferList;
std::vector<HgiTextureHandleVector*>
    HgiGLGarbageCollector::_textureList;
std::vector<HgiSamplerHandleVector*>
    HgiGLGarbageCollector::_samplerList;
std::vector<HgiShaderFunctionHandleVector*>
    HgiGLGarbageCollector::_shaderFunctionList;
std::vector<HgiShaderProgramHandleVector*>
    HgiGLGarbageCollector::_shaderProgramList;
std::vector<HgiResourceBindingsHandleVector*>
    HgiGLGarbageCollector::_resourceBindingsList;
std::vector<HgiGraphicsPipelineHandleVector*>
    HgiGLGarbageCollector::_graphicsPipelineList;
std::vector<HgiComputePipelineHandleVector*>
    HgiGLGarbageCollector::_computePipelineList;


template<class T>
static void _EmptyTrash(std::vector<std::vector<HgiHandle<T>>*>* list) {
    for (auto vec : *list) {
        for (auto objectHandle : *vec) {
            delete objectHandle.Get();
        }
        vec->clear();
        vec->shrink_to_fit();
    }
}

HgiGLGarbageCollector::HgiGLGarbageCollector(HgiGL* hgi)
    : _hgi(hgi)
    , _isDestroying(false)
{
}

HgiGLGarbageCollector::~HgiGLGarbageCollector()
{
    PerformGarbageCollection();
}

/* Multi threaded */
HgiBufferHandleVector*
HgiGLGarbageCollector::GetBufferList()
{
    return _GetThreadLocalStorageList(&_bufferList);
}

/* Multi threaded */
HgiTextureHandleVector*
HgiGLGarbageCollector::GetTextureList()
{
    return _GetThreadLocalStorageList(&_textureList);
}

/* Multi threaded */
HgiSamplerHandleVector*
HgiGLGarbageCollector::GetSamplerList()
{
    return _GetThreadLocalStorageList(&_samplerList);
}

/* Multi threaded */
HgiShaderFunctionHandleVector*
HgiGLGarbageCollector::GetShaderFunctionList()
{
    return _GetThreadLocalStorageList(&_shaderFunctionList);
}

/* Multi threaded */
HgiShaderProgramHandleVector*
HgiGLGarbageCollector::GetShaderProgramList()
{
    return _GetThreadLocalStorageList(&_shaderProgramList);
}

/* Multi threaded */
HgiResourceBindingsHandleVector*
HgiGLGarbageCollector::GetResourceBindingsList()
{
    return _GetThreadLocalStorageList(&_resourceBindingsList);
}

/* Multi threaded */
HgiGraphicsPipelineHandleVector*
HgiGLGarbageCollector::GetGraphicsPipelineList()
{
    return _GetThreadLocalStorageList(&_graphicsPipelineList);
}

/* Multi threaded */
HgiComputePipelineHandleVector*
HgiGLGarbageCollector::GetComputePipelineList()
{
    return _GetThreadLocalStorageList(&_computePipelineList);
}

/* Single threaded */
void
HgiGLGarbageCollector::PerformGarbageCollection()
{
    _isDestroying = true;

    _EmptyTrash(&_bufferList);
    _EmptyTrash(&_textureList);
    _EmptyTrash(&_samplerList);
    _EmptyTrash(&_shaderFunctionList);
    _EmptyTrash(&_shaderProgramList);
    _EmptyTrash(&_resourceBindingsList);
    _EmptyTrash(&_graphicsPipelineList);
    _EmptyTrash(&_computePipelineList);

    _isDestroying = false;
}

template<class T>
T* HgiGLGarbageCollector::_GetThreadLocalStorageList(std::vector<T*>* collector)
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
