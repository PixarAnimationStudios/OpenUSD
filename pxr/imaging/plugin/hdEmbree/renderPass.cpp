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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hdEmbree/renderPass.h"

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
    , _lastRenderedVersion(0)
    , _width(0)
    , _height(0)
    , _viewMatrix(1.0f) // == identity
    , _projMatrix(1.0f) // == identity
    , _attachments()
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
    // If the attachments array is empty, the render thread is rendering into
    // _colorBuffer and _depthBuffer.  _converged is set to their convergence
    // state just before blit, so use that as our answer.
    if (_attachments.size() == 0) {
        return _converged;
    }

    // Otherwise, check the convergence of all attachments.
    for (size_t i = 0; i < _attachments.size(); ++i) {
        if ((_attachments[i].renderBuffer != nullptr) &&
            !_attachments[i].renderBuffer->IsConverged()) {
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
    if (_lastRenderedVersion != currentSceneVersion) {
        needStartRender = true;
        _lastRenderedVersion = currentSceneVersion;
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

    // Determine whether we need to update the renderer attachments.
    //
    // It's possible for the passed in attachments to be empty, but that's
    // never a legal state for the renderer, so if that's the case we add
    // a color attachment that we can blit to the GL framebuffer. In order
    // to check whether we need to add this color attachment, we check both
    // the passed in attachments and also whether the renderer currently has
    // bound attachments.
    HdRenderPassAttachmentVector attachments =
        renderPassState->GetAttachments();
    if (_attachments != attachments || _renderer->GetAttachments().empty()) {
        _attachments = attachments;

        _renderThread->StopRender();
        if (attachments.size() == 0) {
            // No attachment means we should render to the GL framebuffer
            HdRenderPassAttachment cbAttach;
            cbAttach.aovName = HdAovTokens->color;
            cbAttach.renderBuffer = &_colorBuffer;
            cbAttach.clearValue =
                VtValue(GfVec4f(0.0707f, 0.0707f, 0.0707f, 1.0f));
            attachments.push_back(cbAttach);
            HdRenderPassAttachment dbAttach;
            dbAttach.aovName = HdAovTokens->depth;
            dbAttach.renderBuffer = &_depthBuffer;
            dbAttach.clearValue =
                VtValue(1.0f);
            attachments.push_back(dbAttach);
        }
        _renderer->SetAttachments(attachments);
        // In general, the render thread clears attachments, but make sure
        // they are cleared initially on this thread.
        _renderer->Clear();
        needStartRender = true;
    }

    // If there are no attachments, we should blit our local color buffer to
    // the GL framebuffer.
    if (_attachments.size() == 0) {
        _converged = _colorBuffer.IsConverged() && _depthBuffer.IsConverged();
        _colorBuffer.Resolve();
        uint8_t *cdata = _colorBuffer.Map();
        if (cdata) {
            _compositor.UpdateColor(_width, _height, cdata);
            _colorBuffer.Unmap();
        }
        _depthBuffer.Resolve();
        uint8_t *ddata = _depthBuffer.Map();
        if (ddata) {
            _compositor.UpdateDepth(_width, _height, ddata);
            _depthBuffer.Unmap();
        }
        _compositor.Draw();
    }

    // Only start a new render if something in the scene has changed.
    if (needStartRender) {
        _converged = false;
        _renderer->MarkAttachmentsUnconverged();
        _renderThread->StartRender();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
