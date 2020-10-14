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
#ifndef PXR_IMAGING_HGI_GL_HGI_H
#define PXR_IMAGING_HGI_GL_HGI_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiGL/api.h"
#include "pxr/imaging/hgiGL/garbageCollector.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include <functional>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HgiGLDevice;

using HgiGLOpsFn = std::function<void(void)>;
using HgiGLOpsVector = std::vector<HgiGLOpsFn>;


/// \class HgiGL
///
/// OpenGL implementation of the Hydra Graphics Interface.
///
/// HgiGL expects the GL context to be externally managed.
/// When HgiGL is constructed and during any of its resource create / destroy
/// calls and during command recording operations it expects that the OpenGL
/// context is valid and current.
///
class HgiGL final : public Hgi
{
public:
    HGIGL_API
    HgiGL();

    HGIGL_API
    ~HgiGL() override;

    HGIGL_API
    HgiGraphicsCmdsUniquePtr CreateGraphicsCmds(
        HgiGraphicsCmdsDesc const& desc) override;

    HGIGL_API
    HgiBlitCmdsUniquePtr CreateBlitCmds() override;

    HGIGL_API
    HgiComputeCmdsUniquePtr CreateComputeCmds() override;

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
    void StartFrame() override;

    HGIGL_API
    void EndFrame() override;

    //
    // HgiGL specific
    //

    /// Returns the opengl device.
    HGIGL_API
    HgiGLDevice* GetPrimaryDevice() const;

protected:
    HGIGL_API
    bool _SubmitCmds(HgiCmds* cmds, HgiSubmitWaitType wait) override;

private:
    HgiGL & operator=(const HgiGL&) = delete;
    HgiGL(const HgiGL&) = delete;

    // Invalidates the resource handle and places the object in the garbage
    // collector vector for future destruction.
    // This is helpful to avoid destroying GPU resources still in-flight.
    template<class T>
    void _TrashObject(
        HgiHandle<T>* handle, std::vector<HgiHandle<T>>* collector) {
        collector->push_back(HgiHandle<T>(handle->Get(), /*id*/0));
        *handle = HgiHandle<T>();
    }

    HgiGLDevice* _device;
    HgiGLGarbageCollector _garbageCollector;
    int _frameDepth;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
