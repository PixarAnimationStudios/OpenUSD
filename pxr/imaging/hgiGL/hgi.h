//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_GL_HGI_H
#define PXR_IMAGING_HGI_GL_HGI_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiGL/api.h"
#include "pxr/imaging/hgiGL/capabilities.h"
#include "pxr/imaging/hgiGL/garbageCollector.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include <functional>
#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HgiGLDevice;

using HgiGLOpsFn = std::function<void(void)>;
using HgiGLOpsVector = std::vector<HgiGLOpsFn>;
using HgiGLContextArenaHandle = HgiHandle<class HgiGLContextArena>;

/// \class HgiGL
///
/// OpenGL implementation of the Hydra Graphics Interface.
///
/// \section GL Context Management
/// HgiGL expects any GL context(s) to be externally managed.
/// When HgiGL is constructed and during any of its resource create / destroy
/// calls and during command recording operations, it expects that an OpenGL
/// context is valid and current.
///
/// When an application uses the same HgiGL instance from multiple GL contexts,
/// the expectations are that:
/// 1. The application has set up sharing amongst the various GL contexts. This
///    ensures that any non-container resources created may be shared amongst
///    the contexts. These shared resources may be safely deleted from
///    any context in the share group.
///
/// 2. A context arena (see relevant API below) is used per GL context to
///    manage container resources that can't be shared amongst GL contexts.
///    Currently, HgiGL's support is limited to framebuffer objects.
///
/// In the absence of an application provided context arena, the default arena
/// is used with the implied expectation that the same GL context is valid
/// and current for the lifetime of the HgiGL instance.
///
class HgiGL final : public Hgi
{
public:
    HGIGL_API
    HgiGL();

    HGIGL_API
    ~HgiGL() override;

    /// ------------------------------------------------------------------------
    /// Virtual API
    /// ------------------------------------------------------------------------

    HGIGL_API
    bool IsBackendSupported() const override;

    HGIGL_API
    HgiGraphicsCmdsUniquePtr CreateGraphicsCmds(
        HgiGraphicsCmdsDesc const& desc) override;

    HGIGL_API
    HgiBlitCmdsUniquePtr CreateBlitCmds() override;

    HGIGL_API
    HgiComputeCmdsUniquePtr CreateComputeCmds(
        HgiComputeCmdsDesc const& desc) override;

    HGIGL_API
    HgiTextureHandle CreateTexture(HgiTextureDesc const & desc) override;

    HGIGL_API
    void DestroyTexture(HgiTextureHandle* texHandle) override;

    HGIGL_API
    HgiTextureViewHandle CreateTextureView(
        HgiTextureViewDesc const& desc) override;

    HGIGL_API
    void DestroyTextureView(HgiTextureViewHandle* viewHandle) override;

    HGIGL_API
    HgiSamplerHandle CreateSampler(HgiSamplerDesc const & desc) override;

    HGIGL_API
    void DestroySampler(HgiSamplerHandle* smpHandle) override;

    HGIGL_API
    HgiBufferHandle CreateBuffer(HgiBufferDesc const & desc) override;

    HGIGL_API
    void DestroyBuffer(HgiBufferHandle* bufHandle) override;

    HGIGL_API
    HgiShaderFunctionHandle CreateShaderFunction(
        HgiShaderFunctionDesc const& desc) override;

    HGIGL_API
    void DestroyShaderFunction(
        HgiShaderFunctionHandle* shaderFunctionHandle) override;

    HGIGL_API
    HgiShaderProgramHandle CreateShaderProgram(
        HgiShaderProgramDesc const& desc) override;

    HGIGL_API
    void DestroyShaderProgram(
        HgiShaderProgramHandle* shaderProgramHandle) override;

    HGIGL_API
    HgiResourceBindingsHandle CreateResourceBindings(
        HgiResourceBindingsDesc const& desc) override;

    HGIGL_API
    void DestroyResourceBindings(HgiResourceBindingsHandle* resHandle) override;

    HGIGL_API
    HgiGraphicsPipelineHandle CreateGraphicsPipeline(
        HgiGraphicsPipelineDesc const& pipeDesc) override;

    HGIGL_API
    void DestroyGraphicsPipeline(
        HgiGraphicsPipelineHandle* pipeHandle) override;

    HGIGL_API
    HgiComputePipelineHandle CreateComputePipeline(
        HgiComputePipelineDesc const& pipeDesc) override;

    HGIGL_API
    void DestroyComputePipeline(HgiComputePipelineHandle* pipeHandle) override;

    HGIGL_API
    TfToken const& GetAPIName() const override;

    HGIGL_API
    HgiGLCapabilities const* GetCapabilities() const override;

    HGIGL_API
    HgiIndirectCommandEncoder* GetIndirectCommandEncoder() const override;

    HGIGL_API
    void StartFrame() override;

    HGIGL_API
    void EndFrame() override;

    HGIGL_API
    void GarbageCollect() override;

    /// ------------------------------------------------------------------------
    // HgiGL specific API
    /// ------------------------------------------------------------------------

    // Returns the opengl device.
    HGIGL_API
    HgiGLDevice* GetPrimaryDevice() const;

    /// ------------------------------------------------------------------------
    /// Context arena API
    /// Please refer to \ref "GL Context Management" for usage expectations.
    ///
    /// Creates and return a context arena object handle.
    HGIGL_API
    HgiGLContextArenaHandle CreateContextArena();

    /// Destroy a context arena.
    /// Note: The context arena must be unset (by calling SetContextArena with
    ///       an empty handle) prior to destruction.
    HGIGL_API
    void DestroyContextArena(HgiGLContextArenaHandle* arenaHandle);
    
    /// Set the context arena to manage container resources (currently limited to
    /// framebuffer objects) for graphics commands submitted subsequently.
    HGIGL_API
    void SetContextArena(HgiGLContextArenaHandle const& arenaHandle);
    // -------------------------------------------------------------------------

protected:
    HGIGL_API
    bool _SubmitCmds(HgiCmds* cmds, HgiSubmitWaitType wait) override;

private:
    HgiGL & operator=(const HgiGL&) = delete;
    HgiGL(const HgiGL&) = delete;

    /// Invalidates the resource handle and places the object in the garbage
    /// collector vector for future destruction.
    /// This is helpful to avoid destroying GPU resources still in-flight.
    template<class T>
    void _TrashObject(
        HgiHandle<T>* handle, std::vector<HgiHandle<T>>* collector) {
        collector->push_back(HgiHandle<T>(handle->Get(), /*id*/0));
        *handle = HgiHandle<T>();
    }

    HgiGLDevice* _device;
    std::unique_ptr<HgiGLCapabilities> _capabilities;
    HgiGLGarbageCollector _garbageCollector;
    int _frameDepth;
};

/// ----------------------------------------------------------------------------
/// API Version & History 
/// ----------------------------------------------------------------------------
/// 1 -> 2: Added Context Arena API
///
#define HGIGL_API_VERSION 2

PXR_NAMESPACE_CLOSE_SCOPE

#endif
