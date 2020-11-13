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

#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/matrix4d.h"

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

using HgiUniquePtr = std::unique_ptr<class Hgi>;

/// \class HdSt_TestDriver
///
/// A unit test driver that exercises the core engine.
///
/// \note This test driver does NOT assume OpenGL is available; in the even
/// that is is not available, all OpenGL calls become no-ops, but all other work
/// is performed as usual.
///
class HdSt_TestDriver final {
public:
    HdSt_TestDriver();
    HdSt_TestDriver(TfToken const &reprName);
    HdSt_TestDriver(HdReprSelector const &reprToken);
    ~HdSt_TestDriver();

    /// Draw
    void Draw(bool withGuides=false);

    /// Draw with external renderPass
    void Draw(HdRenderPassSharedPtr const &renderPass, bool withGuides);

    /// Set camera to renderpass
    void SetCamera(GfMatrix4d const &modelViewMatrix,
                   GfMatrix4d const &projectionMatrix,
                   GfVec4d const &viewport);
    
    void SetCameraClipPlanes(std::vector<GfVec4d> const& clipPlanes);

    /// Set cull style
    void SetCullStyle(HdCullStyle cullStyle);

    /// Returns the renderpass
    HdRenderPassSharedPtr const &GetRenderPass();

    /// Returns the renderPassState
    HdStRenderPassStateSharedPtr const &GetRenderPassState() const {
        return _renderPassState;
    }

    /// Returns the UnitTest delegate
    HdUnitTestDelegate& GetDelegate() { return *_sceneDelegate; }

    /// Switch repr
    void SetRepr(HdReprSelector const &reprToken);

private:

    void _Init(HdReprSelector const &reprToken);

    // Hgi and HdDriver should be constructed before HdEngine to ensure they
    // are destructed last. Hgi may be used during engine/delegate destruction.
    HgiUniquePtr _hgi;
    HdDriver _hgiDriver;

    HdEngine _engine;
    HdStRenderDelegate   _renderDelegate;
    HdRenderIndex       *_renderIndex;
    HdUnitTestDelegate *_sceneDelegate;

    SdfPath _cameraId;
    HdReprSelector _reprToken;

    HdRenderPassSharedPtr _renderPass;
    HdStRenderPassStateSharedPtr _renderPassState;
    HdRprimCollection          _collection;
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
