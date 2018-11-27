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

#include "pxr/imaging/hdx/shadowTask.h"
#include "pxr/imaging/hdx/simpleLightTask.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hdx/package.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/primGather.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/sceneDelegate.h"


#include "pxr/imaging/hdSt/glConversions.h"
#include "pxr/imaging/hdSt/glslfxShader.h"
#include "pxr/imaging/hdSt/light.h"
#include "pxr/imaging/hdSt/lightingShader.h"
#include "pxr/imaging/hdSt/package.h"
#include "pxr/imaging/hdSt/renderPass.h"
#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/renderPassState.h"

#include "pxr/imaging/glf/simpleLightingContext.h"
#include "pxr/imaging/glf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

HdStShaderCodeSharedPtr HdxShadowTask::_overrideShader;

HdxShadowTask::HdxShadowTask(HdSceneDelegate* delegate, SdfPath const& id)
    : HdSceneTask(delegate, id)
    , _passes()
    , _renderPassStates()
    , _params()
{
}

HdxShadowTask::~HdxShadowTask()
{
}

void
HdxShadowTask::Sync(HdSceneDelegate* delegate,
                    HdTaskContext* ctx,
                    HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    GLF_GROUP_FUNCTION();

    // Extract the lighting context information from the task context
    GlfSimpleLightingContextRefPtr lightingContext;
    if (!_GetTaskContextData(ctx, 
            HdxTokens->lightingContext, 
            &lightingContext)) {
        return;
    }

    GlfSimpleLightVector const glfLights = lightingContext->GetLights();
    GlfSimpleShadowArrayRefPtr const shadows = lightingContext->GetShadows();
    HdRenderIndex &renderIndex = delegate->GetRenderIndex();

    const bool dirtyParams = (*dirtyBits) & HdChangeTracker::DirtyParams;
    if (dirtyParams) {
        // Extract the new shadow task params from exec
        if (!_GetTaskParams(delegate, &_params)) {
            return;
        }
    }

    if (!renderIndex.IsSprimTypeSupported(HdPrimTypeTokens->simpleLight)) {
        return;
    }

    // Iterate through all lights and for those that have shadows enabled
    // and ensure we have enough passes to render the shadows.
    size_t passCount = 0;
    for (size_t lightId = 0; lightId < glfLights.size(); lightId++) {
        const HdStLight* light = static_cast<const HdStLight*>(
            renderIndex.GetSprim(HdPrimTypeTokens->simpleLight,
                                 glfLights[lightId].GetID()));

        if (!glfLights[lightId].HasShadow()) {
            continue;
        }

        // It is possible the light is nullptr for area lights converted to 
        // simple lights, however they should not have shadows enabled.
        TF_VERIFY(light);

        // Extract the collection from the HD light
        VtValue vtShadowCollection =
            light->Get(HdLightTokens->shadowCollection);
        const HdRprimCollection &col =
            vtShadowCollection.IsHolding<HdRprimCollection>() ?
            vtShadowCollection.Get<HdRprimCollection>() : HdRprimCollection();

        // Creates or reuses a pass with the right geometry that will be
        // used during Execute phase to draw the shadow maps.
        if (passCount < _passes.size()) {
            // Note here that we may want to sort the passes by collection
            // to invalidate fewer passes if the collections match already.
            // SetRprimCollection checks for identity changes on the collection
            // and no-ops in that case.
            _passes[passCount]->SetRprimCollection(col);
        } else {
            // Create a new pass if we didn't have enough already,
            HdRenderPassSharedPtr p = boost::make_shared<HdSt_RenderPass>
                (&renderIndex, col);
            _passes.push_back(p);
        }
        passCount++;
    }
    
    // Shrink down to fit to conserve resources
    // We may want hysteresis here if we find the count goes up and down
    // frequently.
    if (_passes.size() > passCount) {
        _passes.resize(passCount);
    }
    
    // Shrink down to fit to conserve resources
    if (_renderPassStates.size() > _passes.size()) {
        _renderPassStates.resize(_passes.size());
    } 
    
    // Ensure all passes have the right params set.
    if (dirtyParams) {
        TF_FOR_ALL(it, _renderPassStates) {
            _UpdateDirtyParams(*it, _params);
        }
    }
    
    // Add new states if the number of passes has grown
    if (_renderPassStates.size() < _passes.size()) {
        for (size_t passId = _renderPassStates.size();
             passId < _passes.size(); passId++) {
            HdStRenderPassShaderSharedPtr renderPassShadowShader
                (new HdStRenderPassShader(HdxPackageRenderPassShadowShader()));
            HdRenderPassStateSharedPtr renderPassState
                (new HdStRenderPassState(renderPassShadowShader));

            // This state is invariant of parameter changes so set it
            // once.
            renderPassState->SetLightingEnabled(false);
            // XXX : This can be removed when Hydra has support for 
            //       transparent objects.
            //       We use an epsilon offset from 1.0 to allow for 
            //       calculation during primvar interpolation which
            //       doesn't fully saturate back to 1.0.
            const float TRANSPARENT_ALPHA_THRESHOLD = (1.0f - 1e-6f);
            renderPassState->SetAlphaThreshold(TRANSPARENT_ALPHA_THRESHOLD);
            
            // A new state is treated as dirty and needs the params set.
            _UpdateDirtyParams(renderPassState, _params);

            _renderPassStates.push_back(renderPassState);
        }
    }

    
    // This should always be true.
    TF_VERIFY(_passes.size() == shadows->GetNumLayers());

    // But if it is not then we still have to make sure we don't
    // buffer overrun here.
    const size_t shadowCount = 
        std::min(shadows->GetNumLayers(), _passes.size());
    for(size_t passId = 0; passId < shadowCount; passId++) {
        // Move the camera to the correct position to take the shadow map
        _renderPassStates[passId]->SetCamera( 
            shadows->GetViewMatrix(passId), 
            shadows->GetProjectionMatrix(passId),
            GfVec4d(0,0,shadows->GetSize()[0],shadows->GetSize()[1]));

        _renderPassStates[passId]->Sync(
            renderIndex.GetResourceRegistry());
        _passes[passId]->Sync();
    }

    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxShadowTask::Execute(HdTaskContext* ctx)
{
    static const TfTokenVector SHADOW_RENDER_TAGS =
    {
        HdTokens->geometry,
        HdxRenderTagsTokens->interactiveOnlyGeom
    };

    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    GLF_GROUP_FUNCTION();

    // Extract the lighting context information from the task context
    GlfSimpleLightingContextRefPtr lightingContext;
    if (!_GetTaskContextData(ctx,
            HdxTokens->lightingContext, &lightingContext)) {
        return;
    }

    if (_params.depthBiasEnable) {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(_params.depthBiasSlopeFactor,
            _params.depthBiasConstantFactor);
    } else {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    // XXX: Move conversion to sync time once Task header becomes private.
    glDepthFunc(HdStGLConversions::GetGlDepthFunc(_params.depthFunc));
    glEnable(GL_PROGRAM_POINT_SIZE);

    // Generate the actual shadow maps
    GlfSimpleShadowArrayRefPtr const shadows = lightingContext->GetShadows();
    // This ensures we don't segfault if the shadows and passes are out of sync.
    // The TF_VERIFY is in Sync for making sure they match but we handle
    // failure gracefully here.
    const size_t shadowCount =
        std::min(shadows->GetNumLayers(), _passes.size());
    for(size_t shadowId = 0; shadowId < shadowCount; shadowId++) {

        // Bind the framebuffer that will store shadowId shadow map
        shadows->BeginCapture(shadowId, true);

        // Render the actual geometry in the collection
        _passes[shadowId]->Execute(
            _renderPassStates[shadowId],
            SHADOW_RENDER_TAGS);

        // Unbind the buffer and move on to the next shadow map
        shadows->EndCapture(shadowId);
    }

    // restore GL states to default
    glDisable(GL_PROGRAM_POINT_SIZE);
    glDisable(GL_POLYGON_OFFSET_FILL);
}

void
HdxShadowTask::_CreateOverrideShader()
{
    static std::mutex shaderCreateLock;

    if (!_overrideShader) {
        std::lock_guard<std::mutex> lock(shaderCreateLock);
        if (!_overrideShader) {
            _overrideShader = HdStShaderCodeSharedPtr(new HdStGLSLFXShader(
                GlfGLSLFXSharedPtr(new GlfGLSLFX(
                    HdStPackageFallbackSurfaceShader()))));
        }
    }
}

void
HdxShadowTask::_SetHdStRenderPassState(HdxShadowTaskParams const &params,
    HdStRenderPassState *renderPassState)
{
    if (params.enableSceneMaterials) {
        renderPassState->SetOverrideShader(HdStShaderCodeSharedPtr());
    } else {
        if (!_overrideShader) {
            _CreateOverrideShader();
        }
        renderPassState->SetOverrideShader(_overrideShader);
    }
}

void
HdxShadowTask::_UpdateDirtyParams(HdRenderPassStateSharedPtr &renderPassState,
    HdxShadowTaskParams const &params)
{
    renderPassState->SetOverrideColor(params.overrideColor);
    renderPassState->SetWireframeColor(params.wireframeColor);
    renderPassState->SetCullStyle(HdInvertCullStyle(params.cullStyle));

    if (HdStRenderPassState* extendedState =
            dynamic_cast<HdStRenderPassState*>(renderPassState.get())) {
        _SetHdStRenderPassState(params, extendedState);
    }
}

// ---------------------------------------------------------------------------//
// VtValue Requirements
// ---------------------------------------------------------------------------//

std::ostream& operator<<(std::ostream& out, const HdxShadowTaskParams& pv)
{
    out << "ShadowTask Params: (...) "
        << pv.overrideColor << " " 
        << pv.wireframeColor << " " 
        << pv.enableLighting << " "
        << pv.enableIdRender << " "
        << pv.enableSceneMaterials << " "
        << pv.alphaThreshold << " "
        << pv.depthBiasEnable << " "
        << pv.depthBiasConstantFactor << " "
        << pv.depthBiasSlopeFactor << " "
        << pv.depthFunc << " "
        << pv.cullStyle << " "
        << pv.camera << " "
        << pv.viewport << " "
        ;
        TF_FOR_ALL(it, pv.lightIncludePaths) {
            out << *it;
        }
        TF_FOR_ALL(it, pv.lightExcludePaths) {
            out << *it;
        }
    return out;
}

bool operator==(const HdxShadowTaskParams& lhs, const HdxShadowTaskParams& rhs) 
{
    return  lhs.overrideColor == rhs.overrideColor                      && 
            lhs.wireframeColor == rhs.wireframeColor                    && 
            lhs.enableLighting == rhs.enableLighting                    &&
            lhs.enableIdRender == rhs.enableIdRender                    &&
            lhs.enableSceneMaterials == rhs.enableSceneMaterials        &&
            lhs.alphaThreshold == rhs.alphaThreshold                    &&
            lhs.depthBiasEnable == rhs.depthBiasEnable                  && 
            lhs.depthBiasConstantFactor == rhs.depthBiasConstantFactor  && 
            lhs.depthBiasSlopeFactor == rhs.depthBiasSlopeFactor        && 
            lhs.depthFunc == rhs.depthFunc                              && 
            lhs.cullStyle == rhs.cullStyle                              && 
            lhs.camera == rhs.camera                                    && 
            lhs.viewport == rhs.viewport                                && 
            lhs.lightIncludePaths == rhs.lightIncludePaths              && 
            lhs.lightExcludePaths == rhs.lightExcludePaths
            ;
}

bool operator!=(const HdxShadowTaskParams& lhs, const HdxShadowTaskParams& rhs) 
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE

