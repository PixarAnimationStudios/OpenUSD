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
#include "pxr/imaging/hdSt/resourceBinder.h"

#include "pxr/base/tf/staticTokens.h"

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

    // Collection names
    (testCollection)
);

HdSt_DrawTask::HdSt_DrawTask(
    HdRenderPassSharedPtr const &renderPass,
    HdStRenderPassStateSharedPtr const &renderPassState,
    const TfTokenVector &renderTags)
  : HdTask(SdfPath::EmptyPath())
  , _renderPass(renderPass)
  , _renderPassState(renderPassState)
  , _renderTags(renderTags)
{
}

void
HdSt_DrawTask::Sync(
    HdSceneDelegate*,
    HdTaskContext*,
    HdDirtyBits*)
{
    _renderPass->Sync();
}

void
HdSt_DrawTask::Prepare(HdTaskContext* ctx,
                       HdRenderIndex* renderIndex)
{
    _renderPassState->Prepare(
        renderIndex->GetResourceRegistry());
}

void
HdSt_DrawTask::Execute(HdTaskContext* ctx)
{
    _renderPass->Execute(_renderPassState, GetRenderTags());
}

template <typename T>
static VtArray<T>
_BuildArray(T values[], int numValues)
{
    VtArray<T> result(numValues);
    std::copy(values, values+numValues, result.begin());
    return result;
}

// --------------------------------------------------------------------------

HdSt_TestDriver::HdSt_TestDriver()
{
    _CreateRenderPassState();

    // Init sets up the camera in the render pass state and
    // thus needs to be called after render pass state has been setup.
    _Init();
}

HdSt_TestDriver::HdSt_TestDriver(TfToken const &reprName)
{
    _CreateRenderPassState();

    // Init sets up the camera in the render pass state and
    // thus needs to be called after render pass state has been setup.
    _Init(HdReprSelector(reprName));
}

HdSt_TestDriver::HdSt_TestDriver(HdReprSelector const &reprSelector)
{
    _CreateRenderPassState();

    // Init sets up the camera in the render pass state and
    // thus needs to be called after render pass state has been setup.
    _Init(reprSelector);
}

void
HdSt_TestDriver::_CreateRenderPassState()
{
    _renderPassStates = {
        std::dynamic_pointer_cast<HdStRenderPassState>(
            _GetRenderDelegate()->CreateRenderPassState()) };
    // set depthfunc to GL default
    _renderPassStates[0]->SetDepthFunc(HdCmpFuncLess);
}

HdRenderPassSharedPtr const &
HdSt_TestDriver::GetRenderPass()
{
    if (_renderPasses.empty()) {
        std::shared_ptr<HdSt_RenderPass> renderPass =
            std::make_shared<HdSt_RenderPass>(
                &GetDelegate().GetRenderIndex(),
                _GetCollection());

        _renderPasses.push_back(std::move(renderPass));
    }
    return _renderPasses[0];
}

void
HdSt_TestDriver::Draw(bool withGuides)
{
    Draw(GetRenderPass(), withGuides);
}

void
HdSt_TestDriver::Draw(HdRenderPassSharedPtr const &renderPass, bool withGuides)
{
    static const TfTokenVector geometryTags {
        HdRenderTagTokens->geometry };
    static const TfTokenVector geometryAndGuideTags {
        HdRenderTagTokens->geometry,
        HdRenderTagTokens->guide };

    HdTaskSharedPtrVector tasks = {
        std::make_shared<HdSt_DrawTask>(
            renderPass,
            _renderPassStates[0],
            withGuides ? geometryAndGuideTags : geometryTags) };
    _GetEngine()->Execute(&GetDelegate().GetRenderIndex(), &tasks);
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
    _glslfx.reset(new HioGlslfx(ss));
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
HdSt_TestLightingShader::BindResources(const int program,
                                       HdSt_ResourceBinder const &binder,
                                       HdRenderPassState const &state)
{
    binder.BindUniformf(_tokens->l0dir,   3, _lights[0].eyeDir.GetArray());
    binder.BindUniformf(_tokens->l0color, 3, _lights[0].color.GetArray());
    binder.BindUniformf(_tokens->l1dir,   3, _lights[1].eyeDir.GetArray());
    binder.BindUniformf(_tokens->l1color, 3, _lights[1].color.GetArray());
    binder.BindUniformf(_tokens->sceneAmbient, 3, _sceneAmbient.GetArray());
}

/* virtual */
void
HdSt_TestLightingShader::UnbindResources(const int program,
                                         HdSt_ResourceBinder const &binder,
                                         HdRenderPassState const &state)
{
}

/*virtual*/
void
HdSt_TestLightingShader::AddBindings(HdBindingRequestVector *customBindings)
{
    customBindings->emplace_back(
        HdBinding::UNIFORM, _tokens->l0dir, HdTypeFloatVec3);
    customBindings->emplace_back(
        HdBinding::UNIFORM, _tokens->l0color, HdTypeFloatVec3);
    customBindings->emplace_back(
        HdBinding::UNIFORM, _tokens->l1dir, HdTypeFloatVec3);
    customBindings->emplace_back(
        HdBinding::UNIFORM, _tokens->l1color, HdTypeFloatVec3);
    customBindings->emplace_back(
        HdBinding::UNIFORM, _tokens->sceneAmbient, HdTypeFloatVec3);
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

