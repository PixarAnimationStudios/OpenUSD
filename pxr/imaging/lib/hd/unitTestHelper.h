//
// Copyright 2016 Pixar
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
#ifndef HD_UNIT_TEST_HELPER
#define HD_UNIT_TEST_HELPER

#include "pxr/pxr.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/lightingShader.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/unitTestDelegate.h"
#include "pxr/imaging/glf/glslfx.h"

#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/matrix4d.h"

#include <vector>
#include <boost/scoped_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


/// \class Hd_TestDriver
///
/// A unit test driver that exercises the core engine.
///
/// \note This test driver does NOT assume OpenGL is available; in the even
/// that is is not available, all OpenGL calls become no-ops, but all other work
/// is performed as usual.
///
class Hd_TestDriver {
public:
    Hd_TestDriver();
    Hd_TestDriver(TfToken const &reprName);

    /// Draw
    void Draw(bool withGuides=false);

    /// Draw with external renderPass
    void Draw(HdRenderPassSharedPtr const &renderPass);

    /// Set camera to renderpass
    void SetCamera(GfMatrix4d const &modelViewMatrix,
                   GfMatrix4d const &projectionMatrix,
                   GfVec4d const &viewport);

    /// Set cull style
    void SetCullStyle(HdCullStyle cullStyle);

    /// Returns the renderpass
    HdRenderPassSharedPtr const &GetRenderPass(bool withGuides=false);

    /// Returns the renderPassState
    HdRenderPassStateSharedPtr const &GetRenderPassState() const {
        return _renderPassState;
    }

    /// Returns the UnitTest delegate
    Hd_UnitTestDelegate& GetDelegate() { return _delegate; }

    /// Switch repr
    void SetRepr(TfToken const &reprName);

private:

    void _Init(TfToken const &reprName);

    HdEngine _engine;
    Hd_UnitTestDelegate _delegate;
    TfToken _reprName;
    HdRenderPassSharedPtr _geomPass;
    HdRenderPassSharedPtr _geomAndGuidePass;
    HdRenderPassStateSharedPtr _renderPassState;
};

/// \class Hd_TestLightingShader
///
/// A custom lighting shader for unit tests.
///
typedef boost::shared_ptr<class Hd_TestLightingShader> Hd_TestLightingShaderSharedPtr;

class Hd_TestLightingShader : public HdLightingShader {
public:
    Hd_TestLightingShader();
    virtual ~Hd_TestLightingShader();

    /// HdShaderCode overrides
    virtual ID ComputeHash() const;
    virtual std::string GetSource(TfToken const &shaderStageKey) const;
    virtual void BindResources(Hd_ResourceBinder const &binder, int program);
    virtual void UnbindResources(Hd_ResourceBinder const &binder, int program);
    virtual void AddBindings(HdBindingRequestVector *customBindings);

    /// HdLightingShader overrides
    virtual void SetCamera(GfMatrix4d const &worldToViewMatrix,
                           GfMatrix4d const &projectionMatrix);

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
    boost::scoped_ptr<GlfGLSLFX> _glslfx;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HD_UNIT_TEST_HELPER
