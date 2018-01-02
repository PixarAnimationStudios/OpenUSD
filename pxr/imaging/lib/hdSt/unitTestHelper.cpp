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
#include "pxr/imaging/hdSt/unitTestHelper.h"
#include "pxr/imaging/hdSt/renderPass.h"

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

class HdSt_DrawTask final : public HdTask
{
public:
    HdSt_DrawTask(HdRenderPassSharedPtr const &renderPass,
                  HdStRenderPassStateSharedPtr const &renderPassState)
    : HdTask()
    , _renderPass(renderPass)
    , _renderPassState(renderPassState)
    {
    }

protected:
    virtual void _Sync(HdTaskContext* ctx) override
    {
        _renderPass->Sync();
        _renderPassState->Sync(
            _renderPass->GetRenderIndex()->GetResourceRegistry());
    }

    virtual void _Execute(HdTaskContext* ctx) override
    {
        _renderPassState->Bind();
        _renderPass->Execute(_renderPassState);
        _renderPassState->Unbind();
    }

private:
    HdRenderPassSharedPtr _renderPass;
    HdStRenderPassStateSharedPtr _renderPassState;
};

template <typename T>
static VtArray<T>
_BuildArray(T values[], int numValues)
{
    VtArray<T> result(numValues);
    std::copy(values, values+numValues, result.begin());
    return result;
}

HdSt_TestDriver::HdSt_TestDriver()
 : _engine()
 , _renderDelegate()
 , _renderIndex(nullptr)
 , _sceneDelegate(nullptr)
 , _reprName()
 , _geomPass()
 , _geomAndGuidePass()
 , _renderPassState(
    boost::dynamic_pointer_cast<HdStRenderPassState>(
        _renderDelegate.CreateRenderPassState()))
{
    TfToken reprName = HdTokens->hull;
    if (TfGetenv("HD_ENABLE_SMOOTH_NORMALS", "CPU") == "CPU" ||
        TfGetenv("HD_ENABLE_SMOOTH_NORMALS", "CPU") == "GPU") {
        reprName = HdTokens->smoothHull;
    }
    _Init(reprName);
}

HdSt_TestDriver::HdSt_TestDriver(TfToken const &reprName)
 : _engine()
 , _renderDelegate()
 , _renderIndex(nullptr)
 , _sceneDelegate(nullptr)
 , _reprName()
 , _geomPass()
 , _geomAndGuidePass()
 , _renderPassState(
    boost::dynamic_pointer_cast<HdStRenderPassState>(
        _renderDelegate.CreateRenderPassState()))
{
    _Init(reprName);
}

HdSt_TestDriver::~HdSt_TestDriver()
{
    delete _sceneDelegate;
    delete _renderIndex;
}

void
HdSt_TestDriver::_Init(TfToken const &reprName)
{
    _renderIndex = HdRenderIndex::New(&_renderDelegate);
    TF_VERIFY(_renderIndex != nullptr);

    _sceneDelegate = new HdSt_UnitTestDelegate(_renderIndex,
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
HdSt_TestDriver::Draw(bool withGuides)
{
    Draw(GetRenderPass(withGuides));
}

void
HdSt_TestDriver::Draw(HdRenderPassSharedPtr const &renderPass)
{
    HdTaskSharedPtrVector tasks = {
        boost::make_shared<HdSt_DrawTask>(renderPass, _renderPassState)
    };
    _engine.Execute(_sceneDelegate->GetRenderIndex(), tasks);

    GLF_POST_PENDING_GL_ERRORS();
}

void
HdSt_TestDriver::SetCamera(GfMatrix4d const &modelViewMatrix,
                         GfMatrix4d const &projectionMatrix,
                         GfVec4d const &viewport)
{
    _renderPassState->SetCamera(modelViewMatrix,
                                projectionMatrix,
                                viewport);
}

void
HdSt_TestDriver::SetCullStyle(HdCullStyle cullStyle)
{
    _renderPassState->SetCullStyle(cullStyle);
}

HdRenderPassSharedPtr const &
HdSt_TestDriver::GetRenderPass(bool withGuides)
{
    if (withGuides) {
        if (!_geomAndGuidePass){
            TfTokenVector renderTags;
            renderTags.push_back(HdTokens->geometry);
            renderTags.push_back(HdTokens->guide);
            
            HdRprimCollection col = HdRprimCollection(
                                     HdTokens->geometry,
                                     _reprName);
            col.SetRenderTags(renderTags);
            _geomAndGuidePass = HdRenderPassSharedPtr(
                new HdSt_RenderPass(&_sceneDelegate->GetRenderIndex(), col));
        }
        return _geomAndGuidePass;
    } else {
        if (!_geomPass){
            TfTokenVector renderTags;
            renderTags.push_back(HdTokens->geometry);

            HdRprimCollection col = HdRprimCollection(
                                        HdTokens->geometry,
                                        _reprName);
            col.SetRenderTags(renderTags);
            _geomPass = HdRenderPassSharedPtr(
                new HdSt_RenderPass(&_sceneDelegate->GetRenderIndex(), col));
        }
        return _geomPass;
    }
}

void
HdSt_TestDriver::SetRepr(TfToken const &reprName)
{
    _reprName = reprName;

    if (_geomAndGuidePass) {
        TfTokenVector renderTags;
        renderTags.push_back(HdTokens->geometry);
        renderTags.push_back(HdTokens->guide);
        
        HdRprimCollection col = HdRprimCollection(
                                 HdTokens->geometry,
                                 _reprName);
        col.SetRenderTags(renderTags);
        _geomAndGuidePass->SetRprimCollection(col);
    }
    if (_geomPass) {
        TfTokenVector renderTags;
        renderTags.push_back(HdTokens->geometry);
        
        HdRprimCollection col = HdRprimCollection(
                                 HdTokens->geometry,
                                 _reprName);
        col.SetRenderTags(renderTags);
        _geomPass->SetRprimCollection(col);
    }
}

// --------------------------------------------------------------------------

HdSt_TestLightingShader::HdSt_TestLightingShader()
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

HdSt_TestLightingShader::~HdSt_TestLightingShader()
{
}

/* virtual */
HdSt_TestLightingShader::ID
HdSt_TestLightingShader::ComputeHash() const
{
    HD_TRACE_FUNCTION();

    size_t hash = _glslfx->GetHash();
    return (ID)hash;
}

/* virtual */
std::string
HdSt_TestLightingShader::GetSource(TfToken const &shaderStageKey) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    std::string source = _glslfx->GetSource(shaderStageKey);
    return source;
}

/* virtual */
void
HdSt_TestLightingShader::SetCamera(GfMatrix4d const &worldToViewMatrix,
                                 GfMatrix4d const &projectionMatrix)
{
    for (int i = 0; i < 2; ++i) {
        _lights[i].eyeDir
            = worldToViewMatrix.TransformDir(_lights[i].dir).GetNormalized();
    }
}

/* virtual */
void
HdSt_TestLightingShader::BindResources(Hd_ResourceBinder const &binder,
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
HdSt_TestLightingShader::UnbindResources(Hd_ResourceBinder const &binder,
                                        int program)
{
}

/*virtual*/
void
HdSt_TestLightingShader::AddBindings(HdBindingRequestVector *customBindings)
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
HdSt_TestLightingShader::SetSceneAmbient(GfVec3f const &color)
{
    _sceneAmbient = color;
}

void
HdSt_TestLightingShader::SetLight(int light,
                                GfVec3f const &dir, GfVec3f const &color)
{
    if (light < 2) {
        _lights[light].dir = dir;
        _lights[light].eyeDir = dir;
        _lights[light].color = color;
    }
}


PXR_NAMESPACE_CLOSE_SCOPE

