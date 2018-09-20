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

#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/glContext.h"

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
    , _colorBufferConverged(false)
    , _texture(0)
    , _framebuffer(0)
{
    _CreateBlitResources();
}

HdEmbreeRenderPass::~HdEmbreeRenderPass()
{
    // Make sure the render thread's not running, in case it's writing
    // to _colorBuffer.
    _renderThread->StopRender();
    _DestroyBlitResources();
}

void
HdEmbreeRenderPass::_CreateBlitResources()
{
    // Store the context we created the FBO on, for sanity checking.
    _owningContext = GlfGLContext::GetCurrentGLContext();

    glGenFramebuffers(1, &_framebuffer);
    GLint restoreReadFB, restoreDrawFB;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &restoreReadFB);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &restoreDrawFB);
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);

    glGenTextures(1, &_texture);
    GLint restoreTexture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &restoreTexture);
    glBindTexture(GL_TEXTURE_2D, _texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           _texture, 0);

    glBindTexture(GL_TEXTURE_2D, restoreTexture);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, restoreReadFB);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, restoreDrawFB);

    GLF_POST_PENDING_GL_ERRORS();
}

void
HdEmbreeRenderPass::_DestroyBlitResources()
{
    // If the owning context is gone, we don't need to do anything. Otherwise,
    // make sure the owning context is active before deleting the FBO.
    if (!_owningContext->IsValid()) {
        return;
    }
    GlfGLContextScopeHolder contextHolder(_owningContext);

    glDeleteTextures(1, &_texture);
    glDeleteFramebuffers(1, &_framebuffer);

    GLF_POST_PENDING_GL_ERRORS();
}

void
HdEmbreeRenderPass::_Blit(unsigned int width, unsigned int height,
                          const uint8_t *data)
{
    if (!TF_VERIFY(_owningContext->IsCurrent())) {
        // If we're rendering with a different context than the render pass
        // was created with, recreate the FBO.

        if (_owningContext->IsValid()) {
            GlfGLContextScopeHolder contextHolder(_owningContext);
            glDeleteFramebuffers(1, &_framebuffer);
        }
        _owningContext = GlfGLContext::GetCurrentGLContext();
        glGenFramebuffers(1, &_framebuffer);
        GLint restoreReadFB, restoreDrawFB;
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &restoreReadFB);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &restoreDrawFB);
        glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                _texture, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, restoreReadFB);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, restoreDrawFB);
    }

    // We blit with glTexImage2D/glBlitFramebuffer... We can't use
    // glDrawPixels because it interacts poorly with the threaded update of
    // the renderer's color buffer. glTexImage2D has much better defined
    // semantics around synchronicity.
    GLint restoreTexture, restoreReadFB;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &restoreTexture);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &restoreReadFB);
    glBindTexture(GL_TEXTURE_2D, _texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, _framebuffer);
    glBlitFramebuffer(0, 0, width, height,
                      0, 0, width, height,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, restoreReadFB);
    glBindTexture(GL_TEXTURE_2D, restoreTexture);

    GLF_POST_PENDING_GL_ERRORS();
}

bool
HdEmbreeRenderPass::IsConverged() const
{
    // If the attachments array is empty, the render thread is rendering into
    // _colorBuffer, so we check its convergence flag (sampled at blit time).
    if (_attachments.size() == 0) {
        return _colorBufferConverged;
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
            // No attachment means we should render to the GL colorbuffer
            HdRenderPassAttachment cbAttach;
            cbAttach.aovName = HdAovTokens->color;
            cbAttach.renderBuffer = &_colorBuffer;
            cbAttach.clearValue =
                VtValue(GfVec4f(0.0707f, 0.0707f, 0.0707f, 1.0f));
            attachments.push_back(cbAttach);
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
        _colorBufferConverged = _colorBuffer.IsConverged();
        _colorBuffer.Resolve();
        uint8_t *data = _colorBuffer.Map();
        if (data) {
            _Blit(_width, _height, data);
            _colorBuffer.Unmap();
        }
    }

    // Only start a new render if something in the scene has changed.
    if (needStartRender) {
        _colorBufferConverged = false;
        _renderThread->StartRender();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
