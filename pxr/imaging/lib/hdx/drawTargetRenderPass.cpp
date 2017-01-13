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
#include "pxr/imaging/glf/glContext.h"

#include "pxr/imaging/hdx/drawTargetRenderPass.h"
#include "pxr/imaging/hdx/drawTargetRenderPassState.h"
#include "pxr/imaging/hd/renderPassState.h"

static
void
_ClearBuffer(GLenum buffer, GLint drawBuffer, const VtValue &value)
{
    // XXX: There has to be a better way to handle the different formats.
    if (value.IsHolding<int>()) {
        glClearBufferiv(buffer, drawBuffer, &value.UncheckedGet<int>());
    } else if (value.IsHolding<GfVec2i>()) {
        glClearBufferiv(buffer, drawBuffer, value.UncheckedGet<GfVec2i>().GetArray());
    } else if (value.IsHolding<GfVec3i>()) {
        glClearBufferiv(buffer, drawBuffer, value.UncheckedGet<GfVec3i>().GetArray());
    } else if (value.IsHolding<GfVec4i>()) {
        glClearBufferiv(buffer, drawBuffer, value.UncheckedGet<GfVec4i>().GetArray());
    } else if (value.IsHolding<float>()) {
        glClearBufferfv(buffer, drawBuffer, &value.UncheckedGet<float>());
    } else if (value.IsHolding<GfVec2f>()) {
        glClearBufferfv(buffer, drawBuffer, value.UncheckedGet<GfVec2f>().GetArray());
    } else if (value.IsHolding<GfVec3f>()) {
        glClearBufferfv(buffer, drawBuffer, value.UncheckedGet<GfVec3f>().GetArray());
    } else if (value.IsHolding<GfVec4f>()) {
        glClearBufferfv(buffer, drawBuffer, value.UncheckedGet<GfVec4f>().GetArray());
    } else {
      TF_CODING_ERROR("Unsupported clear value type: %s",
                      value.GetTypeName().c_str());
    }
}

HdxDrawTargetRenderPass::HdxDrawTargetRenderPass(HdRenderIndex *index)
 : _renderPass(index)
 , _drawTargetRenderPassState(nullptr)
 , _drawTarget()
 , _drawTargetContext()
 , _collectionObjectVersion(0)
{
}


HdxDrawTargetRenderPass::~HdxDrawTargetRenderPass()
{
}

void
HdxDrawTargetRenderPass::SetDrawTarget(const GlfDrawTargetRefPtr &drawTarget)
{
    // XXX: The Draw Target may have been created on a different GL
    // context, so create a local copy here to use on this context.
    _drawTarget = GlfDrawTarget::New(drawTarget);
    _drawTargetContext = GlfGLContext::GetCurrentGLContext();
}

void
HdxDrawTargetRenderPass::SetRenderPassState(
    const HdxDrawTargetRenderPassState *drawTargetRenderPassState)
{
    _drawTargetRenderPassState = drawTargetRenderPassState;
}

void
HdxDrawTargetRenderPass::SetRprimCollection(HdRprimCollection const& col)
{
    _renderPass.SetRprimCollection(col);
}

void
HdxDrawTargetRenderPass::Sync()
{
    // Update the collection object if necessary.
    unsigned int newCollectionVersion =
        _drawTargetRenderPassState->GetRprimCollectionVersion();

    if (_collectionObjectVersion != newCollectionVersion) {
        SetRprimCollection(_drawTargetRenderPassState->GetRprimCollection());

        _collectionObjectVersion = newCollectionVersion;
    }

    // Check the draw target is still valid on the context.
    if (!TF_VERIFY(_drawTargetContext == GlfGLContext::GetCurrentGLContext())) {
        SetDrawTarget(_drawTarget);
    }

    _renderPass.Sync();
}

void
HdxDrawTargetRenderPass::Execute(
    HdRenderPassStateSharedPtr const &renderPassState)
{
    if (!_drawTarget) {
        return;
    }

    _drawTarget->Bind();

    _ClearBuffers();

    GfVec2i const &resolution = _drawTarget->GetSize();

    // XXX: Should the Raster State or Renderpass set and restore this?
    // save the current viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glViewport(0, 0, resolution[0], resolution[1]);

    // Perform actual draw
    _renderPass.Execute(renderPassState);

    // restore viewport
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

    _drawTarget->Unbind();
}

void 
HdxDrawTargetRenderPass::_ClearBuffers()
{
    float depthValue = _drawTargetRenderPassState->GetDepthClearValue();
    glClearBufferfv(GL_DEPTH, 0, &depthValue);

    size_t numAttachments = _drawTargetRenderPassState->GetNumColorAttachments();
    for (size_t attachmentNum = 0;
         attachmentNum < numAttachments;
         ++attachmentNum)
    {
        const VtValue &clearColor =
            _drawTargetRenderPassState->GetColorClearValue(attachmentNum);

        _ClearBuffer(GL_COLOR, attachmentNum, clearColor);
    }
}

GlfDrawTargetRefPtr 
HdxDrawTargetRenderPass::GetDrawTarget()
{
    return _drawTarget;
}
