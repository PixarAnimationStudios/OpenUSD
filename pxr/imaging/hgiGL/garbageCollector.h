//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_GL_GARBAGE_COLLECTOR_H
#define PXR_IMAGING_HGI_GL_GARBAGE_COLLECTOR_H

#include "pxr/pxr.h"
#include "pxr/base/tf/diagnostic.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgiGL/api.h"

#include <mutex>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HgiGL;

/// \class HgiGLGarbageCollector
///
/// Handles garbage collection of opengl objects by delaying their destruction
/// until those objects are no longer used.
///
class HgiGLGarbageCollector final
{
public:
    HGIGL_API
    HgiGLGarbageCollector();

    HGIGL_API
    ~HgiGLGarbageCollector();

    /// Destroys the objects inside the garbage collector.
    /// Thread safety: This call is not thread safe and must be called from
    /// the thread that has the opengl context bound while no other threads are
    /// destroying objects (e.g. during EndFrame).
    HGIGL_API
    void PerformGarbageCollection();

    /// Returns a garbage collection vector for a type of handle.
    /// Thread safety: The returned vector is a thread_local vector so this call
    /// is thread safe as long as the vector is only used by the calling thread.
    HgiBufferHandleVector* GetBufferList();
    HgiTextureHandleVector* GetTextureList();
    HgiSamplerHandleVector* GetSamplerList();
    HgiShaderFunctionHandleVector* GetShaderFunctionList();
    HgiShaderProgramHandleVector* GetShaderProgramList();
    HgiResourceBindingsHandleVector* GetResourceBindingsList();
    HgiGraphicsPipelineHandleVector* GetGraphicsPipelineList();
    HgiComputePipelineHandleVector* GetComputePipelineList();

private:
    HgiGLGarbageCollector & operator =
        (const HgiGLGarbageCollector&) = delete;
    HgiGLGarbageCollector(const HgiGLGarbageCollector&) = delete;

    /// Returns a thread_local vector in which to store a object handle.
    /// Thread safety: The returned vector is a thread_local vector so this call
    /// is thread safe as long as the vector is only used by the calling thread.
    template<class T>
    T* _GetThreadLocalStorageList(std::vector<T*>* collector);

    // List of all the per-thread-vectors of objects that need to be destroyed.
    // The vectors are static (shared across HGIs), because we use thread_local
    // in _GetThreadLocalStorageList which makes us share the garbage collector
    // vectors across Hgi instances.
    static std::vector<HgiBufferHandleVector*> _bufferList;
    static std::vector<HgiTextureHandleVector*> _textureList;
    static std::vector<HgiSamplerHandleVector*> _samplerList;
    static std::vector<HgiShaderFunctionHandleVector*> _shaderFunctionList;
    static std::vector<HgiShaderProgramHandleVector*> _shaderProgramList;
    static std::vector<HgiResourceBindingsHandleVector*> _resourceBindingsList;
    static std::vector<HgiGraphicsPipelineHandleVector*> _graphicsPipelineList;
    static std::vector<HgiComputePipelineHandleVector*> _computePipelineList;

    bool _isDestroying;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
