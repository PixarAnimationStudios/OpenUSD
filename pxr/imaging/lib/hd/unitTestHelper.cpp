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
#include "pxr/imaging/hd/unitTestHelper.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/glf/diagnostic.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/frustum.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/staticTokens.h"

#include <boost/functional/hash.hpp>

#include <string>
#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (l0dir)
    (l0color)
    (l1dir)
    (l1color)
    (sceneAmbient)
    (vec3)
);

class Hd_DrawTask final : public HdTask
{
public:
    Hd_DrawTask(HdRenderPassSharedPtr const &renderPass,
                HdRenderPassStateSharedPtr const &renderPassState)
    : HdTask()
    , _renderPass(renderPass)
    , _renderPassState(renderPassState)
    {
    }

protected:
    virtual void _Sync(HdTaskContext* ctx) override
    {
        _renderPass->Sync();
        _renderPassState->Sync();
    }

    virtual void _Execute(HdTaskContext* ctx) override
    {
        _renderPassState->Bind();
        _renderPass->Execute(_renderPassState);
        _renderPassState->Unbind();
    }

private:
    HdRenderPassSharedPtr _renderPass;
    HdRenderPassStateSharedPtr _renderPassState;
};

template <typename T>
static VtArray<T>
_BuildArray(T values[], int numValues)
{
    VtArray<T> result(numValues);
    std::copy(values, values+numValues, result.begin());
    return result;
}

Hd_TestDriver::Hd_TestDriver()
 : _engine()
 , _renderDelegate()
 , _renderIndex(nullptr)
 , _sceneDelegate(nullptr)
 , _reprName()
 , _geomPass()
 , _geomAndGuidePass()
 , _renderPassState(new HdRenderPassState())
{
    TfToken reprName = HdTokens->hull;
    if (TfGetenv("HD_ENABLE_SMOOTH_NORMALS", "CPU") == "CPU" ||
        TfGetenv("HD_ENABLE_SMOOTH_NORMALS", "CPU") == "GPU") {
        reprName = HdTokens->smoothHull;
    }
    _Init(reprName);
}

Hd_TestDriver::Hd_TestDriver(TfToken const &reprName)
 : _engine()
 , _renderDelegate()
 , _renderIndex(nullptr)
 , _sceneDelegate(nullptr)
 , _reprName()
 , _geomPass()
 , _geomAndGuidePass()
 , _renderPassState(new HdRenderPassState())
{
    _Init(reprName);
}

Hd_TestDriver::~Hd_TestDriver()
{
    delete _sceneDelegate;
    delete _renderIndex;
}

void
Hd_TestDriver::_Init(TfToken const &reprName)
{
    _renderIndex = HdRenderIndex::New(&_renderDelegate);
    TF_VERIFY(_renderIndex != nullptr);

    _sceneDelegate = new Hd_UnitTestDelegate(_renderIndex,
                                             SdfPath::AbsoluteRootPath());

    _reprName = reprName;

    GfMatrix4d viewMatrix = GfMatrix4d().SetIdentity();
    viewMatrix *= GfMatrix4d().SetTranslate(GfVec3d(0.0, 1000.0, 0.0));
    viewMatrix *= GfMatrix4d().SetRotate(GfRotation(GfVec3d(1.0, 0.0, 0.0), -90.0));

    GfFrustum frustum;
    frustum.SetPerspective(45, true, 1, 1.0, 10000.0);
    GfMatrix4d projMatrix = frustum.ComputeProjectionMatrix();

    SetCamera(viewMatrix, projMatrix, GfVec4d(0, 0, 512, 512));

    // set depthfunc to GL default
    _renderPassState->SetDepthFunc(HdCmpFuncLess);
}

void
Hd_TestDriver::Draw(bool withGuides)
{
    Draw(GetRenderPass(withGuides));
}

void
Hd_TestDriver::Draw(HdRenderPassSharedPtr const &renderPass)
{
    HdTaskSharedPtrVector tasks = {
        boost::make_shared<Hd_DrawTask>(renderPass, _renderPassState)
    };
    _engine.Execute(_sceneDelegate->GetRenderIndex(), tasks);

    GLF_POST_PENDING_GL_ERRORS();
}

void
Hd_TestDriver::SetCamera(GfMatrix4d const &modelViewMatrix,
                         GfMatrix4d const &projectionMatrix,
                         GfVec4d const &viewport)
{
    _renderPassState->SetCamera(modelViewMatrix,
                                projectionMatrix,
                                viewport);
}

void
Hd_TestDriver::SetCullStyle(HdCullStyle cullStyle)
{
    _renderPassState->SetCullStyle(cullStyle);
}

HdRenderPassSharedPtr const &
Hd_TestDriver::GetRenderPass(bool withGuides)
{
    if (withGuides) {
        if (!_geomAndGuidePass) 
            _geomAndGuidePass = HdRenderPassSharedPtr(
                new HdRenderPass(&_sceneDelegate->GetRenderIndex(),
                                 HdRprimCollection(
                                     Hd_UnitTestTokens->geometryAndGuides,
                                     _reprName)));
        return _geomAndGuidePass;
    } else {
        if (!_geomPass)
            _geomPass = HdRenderPassSharedPtr(
                new HdRenderPass(&_sceneDelegate->GetRenderIndex(),
                                 HdRprimCollection(
                                     HdTokens->geometry,
                                     _reprName)));
        return _geomPass;
    }
}

void
Hd_TestDriver::SetRepr(TfToken const &reprName)
{
    _reprName = reprName;

    if (_geomAndGuidePass) {
        _geomAndGuidePass->SetRprimCollection(
            HdRprimCollection(Hd_UnitTestTokens->geometryAndGuides, _reprName));
    }
    if (_geomPass) {
        _geomPass->SetRprimCollection(
            HdRprimCollection(HdTokens->geometry, _reprName));
    }
}

// --------------------------------------------------------------------------

Hd_TestLightingShader::Hd_TestLightingShader()
{
    const char *lightingShader =
        "-- glslfx version 0.1                                              \n"
        "-- configuration                                                   \n"
        "{\"techniques\": {\"default\": {\"fragmentShader\" : {             \n"
        " \"source\": [\"TestLighting.Lighting\"]                           \n"
        "}}}}                                                               \n"
        "-- glsl TestLighting.Lighting                                      \n"
        "vec3 FallbackLighting(vec3 Peye, vec3 Neye, vec3 color) {          \n"
        "    vec3 n = normalize(Neye);                                      \n"
        "    return HdGet_sceneAmbient()                                    \n"
        "      + color * HdGet_l0color() * max(0.0, dot(n, HdGet_l0dir()))  \n"
        "      + color * HdGet_l1color() * max(0.0, dot(n, HdGet_l1dir())); \n"
        "}                                                                  \n";

    _lights[0].dir   = GfVec3f(0, 0, 1);
    _lights[0].color = GfVec3f(1, 1, 1);
    _lights[1].dir   = GfVec3f(0, 0, 1);
    _lights[1].color = GfVec3f(0, 0, 0);
    _sceneAmbient    = GfVec3f(0.04, 0.04, 0.04);

    std::stringstream ss(lightingShader);
    _glslfx.reset(new GlfGLSLFX(ss));
}

Hd_TestLightingShader::~Hd_TestLightingShader()
{
}

/* virtual */
Hd_TestLightingShader::ID
Hd_TestLightingShader::ComputeHash() const
{
    HD_TRACE_FUNCTION();

    size_t hash = _glslfx->GetHash();
    return (ID)hash;
}

/* virtual */
std::string
Hd_TestLightingShader::GetSource(TfToken const &shaderStageKey) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    std::string source = _glslfx->GetSource(shaderStageKey);
    return source;
}

/* virtual */
void
Hd_TestLightingShader::SetCamera(GfMatrix4d const &worldToViewMatrix,
                                 GfMatrix4d const &projectionMatrix)
{
    for (int i = 0; i < 2; ++i) {
        _lights[i].eyeDir
            = worldToViewMatrix.TransformDir(_lights[i].dir).GetNormalized();
    }
}

/* virtual */
void
Hd_TestLightingShader::BindResources(Hd_ResourceBinder const &binder,
                                      int program)
{
    binder.BindUniformf(_tokens->l0dir,   3, _lights[0].eyeDir.GetArray());
    binder.BindUniformf(_tokens->l0color, 3, _lights[0].color.GetArray());
    binder.BindUniformf(_tokens->l1dir,   3, _lights[1].eyeDir.GetArray());
    binder.BindUniformf(_tokens->l1color, 3, _lights[1].color.GetArray());
    binder.BindUniformf(_tokens->sceneAmbient, 3, _sceneAmbient.GetArray());
}

/* virtual */
void
Hd_TestLightingShader::UnbindResources(Hd_ResourceBinder const &binder,
                                        int program)
{
}

/*virtual*/
void
Hd_TestLightingShader::AddBindings(HdBindingRequestVector *customBindings)
{
    customBindings->push_back(
        HdBindingRequest(HdBinding::UNIFORM, _tokens->l0dir, _tokens->vec3));
    customBindings->push_back(
        HdBindingRequest(HdBinding::UNIFORM, _tokens->l0color, _tokens->vec3));
    customBindings->push_back(
        HdBindingRequest(HdBinding::UNIFORM, _tokens->l1dir, _tokens->vec3));
    customBindings->push_back(
        HdBindingRequest(HdBinding::UNIFORM, _tokens->l1color, _tokens->vec3));
    customBindings->push_back(
        HdBindingRequest(HdBinding::UNIFORM, _tokens->sceneAmbient, _tokens->vec3));
}

void
Hd_TestLightingShader::SetSceneAmbient(GfVec3f const &color)
{
    _sceneAmbient = color;
}

void
Hd_TestLightingShader::SetLight(int light,
                                GfVec3f const &dir, GfVec3f const &color)
{
    if (light < 2) {
        _lights[light].dir = dir;
        _lights[light].eyeDir = dir;
        _lights[light].color = color;
    }
}


PXR_NAMESPACE_CLOSE_SCOPE

