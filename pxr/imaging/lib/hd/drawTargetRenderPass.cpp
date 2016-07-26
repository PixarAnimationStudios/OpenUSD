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

#include "pxr/imaging/hd/drawTargetRenderPass.h"
#include "pxr/imaging/hd/drawTargetRenderPassState.h"
#include "pxr/imaging/hd/camera.h"
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

HdDrawTargetRenderPass::HdDrawTargetRenderPass(HdRenderIndex *index)
 : _renderPass(index)
 , _renderPassState(new HdRenderPassState())
 , _drawTargetRenderPassState(nullptr)
 , _drawTarget()
 , _drawTargetContext()
 , _simpleLightingShader(new HdSimpleLightingShader())
 , _viewMatrix(1)
 , _projectionMatrix(1)
 , _collectionObjectVersion(0)
{
}


HdDrawTargetRenderPass::~HdDrawTargetRenderPass()
{
}

void
HdDrawTargetRenderPass::SetDrawTarget(const GlfDrawTargetRefPtr &drawTarget)
{
    // XXX: The Draw Target may have been created on a different GL
    // context, so create a local copy here to use on this context.
    _drawTarget = GlfDrawTarget::New(drawTarget);
    _drawTargetContext = GlfGLContext::GetCurrentGLContext();

}


void
HdDrawTargetRenderPass::SetRenderPassState(
    HdDrawTargetRenderPassState *drawTargetRenderPassState)
{
    _drawTargetRenderPassState = drawTargetRenderPassState;
}



void
HdDrawTargetRenderPass::SetRprimCollection(HdRprimCollection const& col)
{
    _renderPass.SetRprimCollection(col);
}

void
HdDrawTargetRenderPass::_Sync(HdTaskContext* ctx)
{
    static const GfMatrix4d yflip = GfMatrix4d().SetScale(
        GfVec3d(1.0, -1.0, 1.0));

    const SdfPath &cameraId = _drawTargetRenderPassState->GetCamera();

    // XXX: Need to detect when camera changes and only update if
    // needed
    HdCameraSharedPtr camera
        = _renderPass.GetRenderIndex()->GetCamera(cameraId);

    if (!camera) {
        // Render pass should not have been added to task list.
        TF_CODING_ERROR("Invalid camera for render pass: %s",
                        cameraId.GetText());
        return;
    }

    VtValue viewMatrixVt  = camera->Get(HdShaderTokens->worldToViewMatrix);
    VtValue projMatrixVt  = camera->Get(HdShaderTokens->projectionMatrix);
    _viewMatrix = viewMatrixVt.Get<GfMatrix4d>();
    const GfMatrix4d &projMatrix = projMatrixVt.Get<GfMatrix4d>();
    _projectionMatrix = projMatrix * yflip;

    GfVec2i const &resolution = _drawTarget->GetSize();
    GfVec4d viewport(0, 0, resolution[0], resolution[1]);

    _renderPassState->SetCamera(_viewMatrix, _projectionMatrix, viewport);

    // Update the internal lighting context so it knows about the new camera
    // position.
    GlfSimpleLightingContextRefPtr lightingContext;
    _GetTaskContextData(ctx, HdTokens->lightingContext, &lightingContext);
    _UpdateLightingContext(lightingContext);

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
    _renderPassState->Sync();
}

void
HdDrawTargetRenderPass::_Execute(HdTaskContext* ctx)
{
    if (!_drawTarget) {
        return;
    }

    _drawTarget->Bind();

    _ClearBuffers();

    GfVec2i const &resolution = _drawTarget->GetSize();

    // XXX: Should the Raster State or Renderpass set this?
    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0, 0, resolution[0], resolution[1]);

    // Perform actual draw
    _renderPass.Execute(_renderPassState);

    glPopAttrib();

    _drawTarget->Unbind();
}

void 
HdDrawTargetRenderPass::_ClearBuffers()
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

void
HdDrawTargetRenderPass::_UpdateLightingContext(GlfSimpleLightingContextRefPtr &lightingContext)
{
    GlfSimpleLightingContextRefPtr const& simpleLightingContext = 
                            _simpleLightingShader->GetLightingContext();

    if (lightingContext) {
        simpleLightingContext->SetUseLighting(
                                        lightingContext->GetUseLighting());
        simpleLightingContext->SetLights(lightingContext->GetLights());
        simpleLightingContext->SetMaterial(lightingContext->GetMaterial());
        simpleLightingContext->SetSceneAmbient(
                                        lightingContext->GetSceneAmbient());
        simpleLightingContext->SetShadows(lightingContext->GetShadows());
        simpleLightingContext->SetUseColorMaterialDiffuse(
                            lightingContext->GetUseColorMaterialDiffuse());
    }

    simpleLightingContext->SetCamera(_viewMatrix, _projectionMatrix);
    _renderPassState->SetLightingShader(_simpleLightingShader);
}
