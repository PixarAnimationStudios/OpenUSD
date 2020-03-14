//
// Copyright 2019 Pixar
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
#ifndef PXR_IMAGING_HGI_HGI_H
#define PXR_IMAGING_HGI_HGI_H

#include "pxr/pxr.h"
#include "pxr/base/tf/type.h"

#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/buffer.h"
#include "pxr/imaging/hgi/pipeline.h"
#include "pxr/imaging/hgi/resourceBindings.h"
#include "pxr/imaging/hgi/shaderFunction.h"
#include "pxr/imaging/hgi/shaderProgram.h"
#include "pxr/imaging/hgi/texture.h"
#include "pxr/imaging/hgi/types.h"

#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

class HgiImmediateCommandBuffer;

/// \class Hgi
///
/// Hydra Graphics Interface.
/// Hgi is used to communicate with one or more physical gpu devices.
///
/// Hgi provides API to create/destroy resources that a gpu device owns.
/// The lifetime of resources is not managed by Hgi, so it is up to the caller
/// to destroy resources and ensure those resources are no longer used.
///
/// Commands are recorded via an encoder that is acquired from a command buffer.
/// Command buffers can work in immediate or deferred mode.
/// An immediate command buffer assumes its encoders will execute commands
/// without delay in the graphics backend.
/// A deferred command buffer records commands to be executed at a later time
/// in the graphics backend.
///
/// XXX We currently only support one immediate command buffer since most code
/// in HdSt was written via OpenGL's immediate style API.
///
///
class Hgi 
{
public:
    HGI_API
    Hgi();

    HGI_API
    virtual ~Hgi();

    // Helper function to return a Hgi object for the current platform.
    // For example on Linux this may return HgiGL while on macOS HgiMetal.
    // Caller, usually the application, owns the lifetime of the returned Hgi 
    // pointer and must destroy it during shutdown.
    HGI_API
    static Hgi* GetPlatformDefaultHgi();

    //
    // Command Buffers
    //

    /// Returns the immediate command buffer.
    HGI_API
    virtual HgiImmediateCommandBuffer& GetImmediateCommandBuffer() = 0;

    //
    // Resource API
    //

    /// Create a texture in rendering backend.
    HGI_API
    virtual HgiTextureHandle CreateTexture(HgiTextureDesc const & desc) = 0;

    /// Destroy a texture in rendering backend.
    HGI_API
    virtual void DestroyTexture(HgiTextureHandle* texHandle) = 0;

    /// Create a buffer in rendering backend.
    HGI_API
    virtual HgiBufferHandle CreateBuffer(HgiBufferDesc const & desc) = 0;

    /// Destroy a buffer in rendering backend.
    HGI_API
    virtual void DestroyBuffer(HgiBufferHandle* bufHandle) = 0;

    /// Create a new shader function.
    HGI_API
    virtual HgiShaderFunctionHandle CreateShaderFunction(
        HgiShaderFunctionDesc const& desc) = 0;

    /// Destroy a shader function.
    HGI_API
    virtual void DestroyShaderFunction(
        HgiShaderFunctionHandle* shaderFunctionHandle) = 0;

    /// Create a new shader program.
    HGI_API
    virtual HgiShaderProgramHandle CreateShaderProgram(
        HgiShaderProgramDesc const& desc) = 0;

    /// Destroy a shader program.
    /// Note that this does NOT automatically destroy the shader functions in
    /// the program since shader functions may be used by more than one program.
    HGI_API
    virtual void DestroyShaderProgram(
        HgiShaderProgramHandle* shaderProgramHandle) = 0;

    /// Create a new resource binding object.
    HGI_API
    virtual HgiResourceBindingsHandle CreateResourceBindings(
        HgiResourceBindingsDesc const& desc) = 0;

    /// Destroy a resource binding object.
    HGI_API
    virtual void DestroyResourceBindings(
        HgiResourceBindingsHandle* resHandle) = 0;

    /// Create a new pipeline state object
    HGI_API
    virtual HgiPipelineHandle CreatePipeline(
        HgiPipelineDesc const& pipeDesc) = 0;

    /// Destroy a pipeline state object
    HGI_API
    virtual void DestroyPipeline(HgiPipelineHandle* pipeHandle) = 0;

protected:
    // Returns a unique id for handle creation.
    HGI_API
    uint64_t GetUniqueId();

    // Destroys the underlying object that is represented by the handle.
    template<class T>
    void DestroyObject(HgiHandle<T>* handle) {
        handle->_Destroy();
    }

private:
    Hgi & operator=(const Hgi&) = delete;
    Hgi(const Hgi&) = delete;

    std::atomic<uint64_t> _uniqueIdCounter;
};


///
/// Hgi factory for plugin system
///
class HgiFactoryBase : public TfType::FactoryBase {
public:
    virtual Hgi* New() const = 0;
};

template <class T>
class HgiFactory : public HgiFactoryBase {
public:
    Hgi* New() const {
        return new T;
    }
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
