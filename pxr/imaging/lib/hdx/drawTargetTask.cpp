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

#include "pxr/imaging/hdx/drawTargetTask.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hdx/debugCodes.h"

#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/drawTarget.h"
#include "pxr/imaging/hd/drawTargetRenderPass.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/simpleLightingShader.h"

#include "pxr/imaging/glf/drawTarget.h"

HdxDrawTargetTask::HdxDrawTargetTask(HdSceneDelegate* delegate,
                                     SdfPath const& id)
 : HdSceneTask(delegate, id)
 , _enableDrawTargets(false)
 , _currentDrawTargetSetVersion(0)
 , _renderPasses()
 , _depthBiasUseDefault(true)
 , _depthBiasEnable(false)
 , _depthBiasConstantFactor(0.0f)
 , _depthBiasSlopeFactor(1.0f)
 , _depthFunc(HdCmpFuncLEqual)
{
}

void
HdxDrawTargetTask::_Sync(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    TRACE_FUNCTION();
    TfAutoMallocTag2 tag("GlimRg", __PRETTY_FUNCTION__);

    HdChangeTracker::DirtyBits bits = _GetTaskDirtyBits();

    if (bits & HdChangeTracker::DirtyParams) {
        HdxDrawTargetTaskParams params;

        if (not _GetSceneDelegateValue(HdTokens->params, &params)) {
            return;
        }

        _enableDrawTargets       = params.enableDrawTargets;

        // Raster State
        // XXX: Update master raster state that is used by all passes?
        _wireframeColor          = params.wireframeColor;
        _enableLighting          = params.enableLighting;
        _overrideColor           = params.overrideColor;
        _alphaThreshold          = params.alphaThreshold;
        _tessLevel               = params.tessLevel;
        _drawingRange            = params.drawingRange;
        _cullStyle               = params.cullStyle;


        // Depth
        // XXX: Should be in raster state?
        _depthBiasUseDefault     = params.depthBiasUseDefault;
        _depthBiasEnable         = params.depthBiasEnable;
        _depthBiasConstantFactor = params.depthBiasConstantFactor;
        _depthBiasSlopeFactor    = params.depthBiasSlopeFactor;
        _depthFunc               = params.depthFunc;

        _cullStyle               = params.cullStyle;
        _geomStyle               = params.geomStyle;
        _complexity              = params.complexity;
        _hullVisibility          = params.hullVisibility;
        _surfaceVisibility       = params.surfaceVisibility;
    }

    if (not _enableDrawTargets) {
        return;
    }

    HdSceneDelegate* delegate = GetDelegate();
    HdRenderIndex &renderIndex = delegate->GetRenderIndex();
    HdChangeTracker& changeTracker = renderIndex.GetChangeTracker();

    unsigned drawTargetVersion = changeTracker.GetDrawTargetSetVersion();
    if (_currentDrawTargetSetVersion != drawTargetVersion) {
        const HdRenderIndex::HdDrawTargetView &drawTargets =
                                                   renderIndex.GetDrawTargets();

        _renderPasses.clear();

        size_t numDrawTargets =  drawTargets.size();
        _renderPasses.reserve(numDrawTargets);

        for (size_t drawTargetNum = 0;
             drawTargetNum < numDrawTargets;
             ++drawTargetNum) {

            HdDrawTargetSharedPtr drawTarget = drawTargets[drawTargetNum];
            if (drawTarget) {
                if (drawTarget->IsEnabled()) {
                    HdDrawTargetRenderPassUniquePtr pass(
                                      new HdDrawTargetRenderPass(&renderIndex));

                    pass->SetDrawTarget(drawTarget->GetGlfDrawTarget());
                    pass->SetRenderPassState(drawTarget->GetRenderPassState());

                    HdRenderPassStateSharedPtr renderPassState(new HdRenderPassState());
                    HdSimpleLightingShaderSharedPtr simpleLightingShader(new HdSimpleLightingShader());

                    _renderPasses.emplace_back(
                            RenderPassInfo {
                                    std::move(pass),
                                    renderPassState,
                                    simpleLightingShader,
                                    drawTarget,
                                    drawTarget->GetVersion()
                             });
                }
            }
        }
        _currentDrawTargetSetVersion = drawTargetVersion;
    } else {
        size_t numRenderPasses = _renderPasses.size();

        // Need to look for changes in individual draw targets.
        for (size_t renderPassIdx = 0;
             renderPassIdx < numRenderPasses;
             ++renderPassIdx) {
            RenderPassInfo &renderPassInfo =  _renderPasses[renderPassIdx];
            HdDrawTargetSharedPtr target = renderPassInfo.target.lock();
            unsigned int targetVersion = target->GetVersion();

            if (renderPassInfo.version != targetVersion) {

                renderPassInfo.pass->SetDrawTarget(target->GetGlfDrawTarget());

                renderPassInfo.version = targetVersion;
            }
        }
    }

    ///----------------------
    static const GfMatrix4d yflip = GfMatrix4d().SetScale(
        GfVec3d(1.0, -1.0, 1.0));

    // lighting context
    GlfSimpleLightingContextRefPtr lightingContext;
    _GetTaskContextData(ctx, HdTokens->lightingContext, &lightingContext);

    size_t numRenderPasses = _renderPasses.size();
    for (size_t renderPassIdx = 0;
         renderPassIdx < numRenderPasses;
         ++renderPassIdx) {

        RenderPassInfo &renderPassInfo =  _renderPasses[renderPassIdx];
        HdDrawTargetRenderPass *renderPass = renderPassInfo.pass.get();
        HdRenderPassStateSharedPtr &renderPassState = renderPassInfo.renderPassState;
        HdDrawTargetSharedPtr drawTarget = renderPassInfo.target.lock();

        const SdfPath &cameraId = drawTarget->GetRenderPassState()->GetCamera();

        // XXX: Need to detect when camera changes and only update if
        // needed
        HdCameraSharedPtr camera
            = GetDelegate()->GetRenderIndex().GetCamera(cameraId);

        if (!camera) {
            // Render pass should not have been added to task list.
            TF_CODING_ERROR("Invalid camera for render pass: %s",
                            cameraId.GetText());
            return;
        }

        VtValue viewMatrixVt  = camera->Get(HdShaderTokens->worldToViewMatrix);
        VtValue projMatrixVt  = camera->Get(HdShaderTokens->projectionMatrix);
        GfMatrix4d viewMatrix = viewMatrixVt.Get<GfMatrix4d>();
        const GfMatrix4d &projMatrix = projMatrixVt.Get<GfMatrix4d>();
        GfMatrix4d projectionMatrix = projMatrix * yflip;

        GfVec2i const &resolution = drawTarget->GetGlfDrawTarget()->GetSize();
        GfVec4d viewport(0, 0, resolution[0], resolution[1]);

        // Update Raster States
        renderPassState->SetOverrideColor(_overrideColor);
        renderPassState->SetWireframeColor(_wireframeColor);
        renderPassState->SetLightingEnabled(_enableLighting);
        renderPassState->SetAlphaThreshold(_alphaThreshold);
        renderPassState->SetTessLevel(_tessLevel);
        renderPassState->SetDrawingRange(_drawingRange);
        renderPassState->SetCullStyle(_cullStyle);

        HdSimpleLightingShaderSharedPtr &simpleLightingShader
            = _renderPasses[renderPassIdx].simpleLightingShader;
        GlfSimpleLightingContextRefPtr const& simpleLightingContext =
            simpleLightingShader->GetLightingContext();

        renderPassState->SetLightingShader(simpleLightingShader);

        renderPassState->SetCamera(viewMatrix, projectionMatrix, viewport);
        simpleLightingContext->SetCamera(viewMatrix, projectionMatrix);

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

        renderPassState->Sync();
        renderPass->Sync();
    }
}

void
HdxDrawTargetTask::_Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    // When the draw targets are not enabled we don't need to do any
    // draw target computation
    if (not _enableDrawTargets) {
        return;
    }

    // Apply polygon offset to whole pass.
    // XXX TODO: Move to an appropriate home
    glPushAttrib(GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT);
    if (!_depthBiasUseDefault) {
        if (_depthBiasEnable) {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(_depthBiasSlopeFactor, _depthBiasConstantFactor);
        } else {
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
    }
    // XXX: Move conversion to sync once header is made private
    // to the library
    glDepthFunc(HdConversions::GetGlDepthFunc(_depthFunc));

    // XXX:  Long-term ALpha to Coverage will be a render style on the
    // task.  However, as there isn't a fallback we current force it
    // enabled, unless a client chooses to manage the setting itself (aka usdImaging).
    GLboolean oldAlphaToCoverage = glIsEnabled(GL_SAMPLE_ALPHA_TO_COVERAGE);
    if (GetDelegate()->IsEnabled(HdxOptionTokens->taskSetAlphaToCoverage)) {
        if (not TfDebug::IsEnabled(HDX_DISABLE_ALPHA_TO_COVERAGE)) {
            glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        } else {
            glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        }
    }


    // XXX: Do we ever want to disable this?
    GLboolean oldPointSizeEnabled = glIsEnabled(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_PROGRAM_POINT_SIZE);

    // XXX: We "Known" Hydra is always using CCW fase winding
    // which we need to flip.  This is a hack for now, but belongs in Hydra's
    // PSO.
    glFrontFace(GL_CW);  // Restored by GL_POLYGON_BIT

    size_t numRenderPasses = _renderPasses.size();
    for (size_t renderPassIdx = 0;
         renderPassIdx < numRenderPasses;
         ++renderPassIdx) {

        HdDrawTargetRenderPass *renderPass =
            _renderPasses[renderPassIdx].pass.get();
        HdRenderPassStateSharedPtr renderPassState =
            _renderPasses[renderPassIdx].renderPassState;
        renderPass->Execute(renderPassState);
    }

    if (oldAlphaToCoverage) {
        glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    } else {
        glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    }

    if (!oldPointSizeEnabled)
    {
        glDisable(GL_PROGRAM_POINT_SIZE);
    }

    glPopAttrib();
}


// --------------------------------------------------------------------------- //
// VtValue Requirements
// --------------------------------------------------------------------------- //

std::ostream& operator<<(std::ostream& out, const HdxDrawTargetTaskParams& pv)
{
    out << "GlimRG Draw Target Task Params: (...) "
        << pv.enableDrawTargets;

    return out;
}

bool operator==(const HdxDrawTargetTaskParams& lhs, const HdxDrawTargetTaskParams& rhs)
{
    return (lhs.enableDrawTargets == rhs.enableDrawTargets);
}

bool operator!=(const HdxDrawTargetTaskParams& lhs, const HdxDrawTargetTaskParams& rhs)
{
    return not(lhs == rhs);
}


