//
// Copyright 2017 Pixar
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
#ifndef PXR_IMAGING_HD_ST_UNIT_TEST_HELPER_H
#define PXR_IMAGING_HD_ST_UNIT_TEST_HELPER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/lightingShader.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdSt/renderPassState.h"

#include "pxr/imaging/hd/unitTestDelegate.h"
#include "pxr/imaging/hd/driver.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hio/glslfx.h"
#include "pxr/imaging/hgiInterop/hgiInterop.h"

#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/matrix4d.h"

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

using HgiUniquePtr = std::unique_ptr<class Hgi>;

/// \class HdSt_TestDriverBase
///
/// A base class for unit test drivers that creates all core components but no
/// render passes.
///
/// \note This test driver does NOT assume OpenGL is available; in the event
/// that is is not available, all OpenGL calls become no-ops, but all other work
/// is performed as usual.
///
class HdSt_TestDriverBase
{
public:
    HdSt_TestDriverBase();
    virtual ~HdSt_TestDriverBase();

    /// Set camera to renderpass
    void SetCamera(GfMatrix4d const &viewMatrix,
                   GfMatrix4d const &projectionMatrix,
                   CameraUtilFraming const &framing);
    
    void SetCameraClipPlanes(std::vector<GfVec4d> const& clipPlanes);

    /// Set cull style
    void SetCullStyle(HdCullStyle cullStyle);

    /// Returns the UnitTest delegate
    HdUnitTestDelegate& GetDelegate() { return *_sceneDelegate; }

    /// Switch repr
    void SetRepr(HdReprSelector const &reprSelector);

    void SetupAovs(int width, int height);

    void UpdateAovDimensions(int width, int height);

    bool WriteToFile(std::string const & attachment,
                     std::string const & filename);

    void Present(int width, int height, uint32_t framebuffer);

    void SetClearColor(GfVec4f const &clearColor) {
        _clearColor = clearColor;
    }

    void SetClearDepth(float clearDepth) {
        _clearDepth = clearDepth;
    }

protected:
    void _Init();
    void _Init(HdReprSelector const &reprSelector);

    SdfPath _GetAovPath(TfToken const &aov) const;
    void _AddRenderBuffer(SdfPath const &id, 
        HdRenderBufferDescriptor const &desc);

    const HdRprimCollection &_GetCollection() const { return _collection; }
    HdStRenderDelegate * _GetRenderDelegate() { return &_renderDelegate; }
    HdEngine * _GetEngine() { return &_engine; }

    std::vector<HdRenderPassSharedPtr> _renderPasses;
    std::vector<HdStRenderPassStateSharedPtr> _renderPassStates;

    HdRenderPassAovBindingVector _aovBindings;
    SdfPathVector _aovBufferIds;

private:
    // Hgi and HdDriver should be constructed before HdEngine to ensure they
    // are destructed last. Hgi may be used during engine/delegate destruction.
    HgiUniquePtr _hgi;
    HdDriver _hgiDriver;

    HdEngine _engine;
    HdStRenderDelegate   _renderDelegate;
    HdRenderIndex       *_renderIndex;
    HdUnitTestDelegate *_sceneDelegate;

    HgiInterop _interop;

    SdfPath _cameraId;
    HdReprSelector _reprSelector;

    HdRprimCollection          _collection;

    GfVec4f _clearColor;
    float _clearDepth;
};

/// \class HdSt_DrawTask
///
/// A simple task to execute a render pass.
///
class HdSt_DrawTask final : public HdTask
{
public:
    HdSt_DrawTask(HdRenderPassSharedPtr const &renderPass,
                  HdStRenderPassStateSharedPtr const &renderPassState,
                  const TfTokenVector &renderTags);
    
    void Sync(HdSceneDelegate*,
              HdTaskContext*,
              HdDirtyBits*) override;

    void Prepare(HdTaskContext* ctx,
                 HdRenderIndex* renderIndex) override;

    void Execute(HdTaskContext* ctx) override;

    const TfTokenVector &GetRenderTags() const override
    {
        return _renderTags;
    }

private:
    HdRenderPassSharedPtr const _renderPass;
    HdStRenderPassStateSharedPtr const _renderPassState;
    TfTokenVector const _renderTags;

    HdSt_DrawTask() = delete;
    HdSt_DrawTask(const HdSt_DrawTask &) = delete;
    HdSt_DrawTask &operator =(const HdSt_DrawTask &) = delete;
};

/// \class HdSt_TestDriver
///
/// A unit test driver that exercises the core engine.
///
class HdSt_TestDriver final : public HdSt_TestDriverBase
{
public:
    HdSt_TestDriver();
    HdSt_TestDriver(TfToken const &reprName);
    HdSt_TestDriver(HdReprSelector const &reprSelector);

    /// Draw
    void Draw(bool withGuides=false);

    /// Draw with external renderPass
    void Draw(HdRenderPassSharedPtr const &renderPass, bool withGuides);

    const HdStRenderPassStateSharedPtr &GetRenderPassState() const {
        return _renderPassStates[0];
    }

    const HdRenderPassSharedPtr &GetRenderPass();

private:
    void _CreateRenderPassState();
};

/// \class HdSt_TestLightingShader
///
/// A custom lighting shader for unit tests.
///
using HdSt_TestLightingShaderSharedPtr =
    std::shared_ptr<class HdSt_TestLightingShader>;

class HdSt_TestLightingShader : public HdStLightingShader
{
public:
    HDST_API
    HdSt_TestLightingShader();
    HDST_API
    ~HdSt_TestLightingShader() override;

    /// HdStShaderCode overrides
    HDST_API
    ID ComputeHash() const override;
    std::string GetSource(TfToken const &shaderStageKey) const override;
    HDST_API
    void BindResources(int program,
                       HdSt_ResourceBinder const &binder,
                       HdRenderPassState const &state) override;
    HDST_API
    void UnbindResources(int program,
                         HdSt_ResourceBinder const &binder,
                         HdRenderPassState const &state) override;
    void AddBindings(HdBindingRequestVector *customBindings) override;

    /// HdStLightingShader overrides
    void SetCamera(GfMatrix4d const &worldToViewMatrix,
                   GfMatrix4d const &projectionMatrix) override;

    void SetSceneAmbient(GfVec3f const &color);
    void SetLight(int light, GfVec3f const &dir, GfVec3f const &color);

private:
    struct Light {
        GfVec3f dir;
        GfVec3f eyeDir;
        GfVec3f color;
    };
    Light _lights[2];
    GfVec3f _sceneAmbient;
    std::unique_ptr<HioGlslfx> _glslfx;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_UNIT_TEST_HELPER_H
