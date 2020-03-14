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
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/plugin/hdEmbree/renderDelegate.h"
#include "pxr/imaging/plugin/hdEmbree/renderPass.h"

PXR_NAMESPACE_OPEN_SCOPE

HdEmbreeRenderPass::HdEmbreeRenderPass(HdRenderIndex *index,
                                       HdRprimCollection const &collection,
                                       HdRenderThread *renderThread,
                                       HdEmbreeRenderer *renderer,
                                       std::atomic<int> *sceneVersion)
    : HdRenderPass(index, collection)
    , _renderThread(renderThread)
    , _renderer(renderer)
    , _sceneVersion(sceneVersion)
    , _lastSceneVersion(0)
    , _lastSettingsVersion(0)
    , _width(0)
    , _height(0)
    , _viewMatrix(1.0f) // == identity
    , _projMatrix(1.0f) // == identity
    , _aovBindings()
    , _colorBuffer(SdfPath::EmptyPath())
    , _depthBuffer(SdfPath::EmptyPath())
    , _converged(false)
{
}

HdEmbreeRenderPass::~HdEmbreeRenderPass()
{
    // Make sure the render thread's not running, in case it's writing
    // to _colorBuffer/_depthBuffer.
    _renderThread->StopRender();
}

bool
HdEmbreeRenderPass::IsConverged() const
{
    // If the aov binding array is empty, the render thread is rendering into
    // _colorBuffer and _depthBuffer.  _converged is set to their convergence
    // state just before blit, so use that as our answer.
    if (_aovBindings.size() == 0) {
        return _converged;
    }

    // Otherwise, check the convergence of all attachments.
    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        if (_aovBindings[i].renderBuffer &&
            !_aovBindings[i].renderBuffer->IsConverged()) {
            return false;
        }
    }
    return true;
}

void
HdEmbreeRenderPass::_Execute(HdRenderPassStateSharedPtr const& renderPassState,
                             TfTokenVector const &renderTags)
{
    // XXX: Add collection and renderTags support.
    // XXX: Add clip planes support.

    // Determine whether the scene has changed since the last time we rendered.
    bool needStartRender = false;
    int currentSceneVersion = _sceneVersion->load();
    if (_lastSceneVersion != currentSceneVersion) {
        needStartRender = true;
        _lastSceneVersion = currentSceneVersion;
    }

    // Likewise the render settings.
    HdRenderDelegate *renderDelegate = GetRenderIndex()->GetRenderDelegate();
    int currentSettingsVersion = renderDelegate->GetRenderSettingsVersion();
    if (_lastSettingsVersion != currentSettingsVersion) {
        _renderThread->StopRender();
        _lastSettingsVersion = currentSettingsVersion;

        _renderer->SetSamplesToConvergence(
            renderDelegate->GetRenderSetting<int>(
                HdRenderSettingsTokens->convergedSamplesPerPixel, 1));

        bool enableAmbientOcclusion =
            renderDelegate->GetRenderSetting<bool>(
                HdEmbreeRenderSettingsTokens->enableAmbientOcclusion, false);
        if (enableAmbientOcclusion) {
            _renderer->SetAmbientOcclusionSamples(
                renderDelegate->GetRenderSetting<int>(
                    HdEmbreeRenderSettingsTokens->ambientOcclusionSamples, 0));
        } else {
            _renderer->SetAmbientOcclusionSamples(0);
        }

        _renderer->SetEnableSceneColors(
            renderDelegate->GetRenderSetting<bool>(
                HdEmbreeRenderSettingsTokens->enableSceneColors, true));

        needStartRender = true;
    }

    GfVec4f vp = renderPassState->GetViewport();

    // Determine whether we need to update the renderer camera.
    GfMatrix4d view = renderPassState->GetWorldToViewMatrix();
    GfMatrix4d proj = renderPassState->GetProjectionMatrix();
    if (_viewMatrix != view || _projMatrix != proj) {
        _viewMatrix = view;
        _projMatrix = proj;

        _renderThread->StopRender();
        _renderer->SetCamera(_viewMatrix, _projMatrix);
        needStartRender = true;
    }

    // Determine whether we need to update the renderer viewport.
    if (_width != vp[2] || _height != vp[3]) {
        _width = vp[2];
        _height = vp[3];

        _renderThread->StopRender();
        _renderer->SetViewport(_width, _height);
        _colorBuffer.Allocate(GfVec3i(_width, _height, 1), HdFormatUNorm8Vec4,
                              /*multiSampled=*/true);
        _depthBuffer.Allocate(GfVec3i(_width, _height, 1), HdFormatFloat32,
                              /*multiSampled=*/false);
        needStartRender = true;
    }

    // Determine whether we need to update the renderer AOV bindings.
    //
    // It's possible for the passed in bindings to be empty, but that's
    // never a legal state for the renderer, so if that's the case we add
    // a color and depth aov.
    //
    // If the renderer AOV bindings are empty, force a bindings update so that
    // we always get a chance to add color/depth on the first time through.
    HdRenderPassAovBindingVector aovBindings =
        renderPassState->GetAovBindings();
    if (_aovBindings != aovBindings || _renderer->GetAovBindings().empty()) {
        _aovBindings = aovBindings;

        _renderThread->StopRender();
        if (aovBindings.size() == 0) {
            HdRenderPassAovBinding colorAov;
            colorAov.aovName = HdAovTokens->color;
            colorAov.renderBuffer = &_colorBuffer;
            colorAov.clearValue =
                VtValue(GfVec4f(0.0707f, 0.0707f, 0.0707f, 1.0f));
            aovBindings.push_back(colorAov);
            HdRenderPassAovBinding depthAov;
            depthAov.aovName = HdAovTokens->depth;
            depthAov.renderBuffer = &_depthBuffer;
            depthAov.clearValue = VtValue(1.0f);
            aovBindings.push_back(depthAov);
        }
        _renderer->SetAovBindings(aovBindings);
        // In general, the render thread clears aov bindings, but make sure
        // they are cleared initially on this thread.
        _renderer->Clear();
        needStartRender = true;
    }

    TF_VERIFY(!_aovBindings.empty(), "No aov bindings to render into");

    // Only start a new render if something in the scene has changed.
    if (needStartRender) {
        _converged = false;
        _renderer->MarkAovBuffersUnconverged();
        _renderThread->StartRender();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
