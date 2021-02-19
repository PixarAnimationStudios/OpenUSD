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
#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/hdx/shadowTask.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hdx/package.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/sceneDelegate.h"


#include "pxr/imaging/hdSt/glConversions.h"
#include "pxr/imaging/hdSt/glslfxShader.h"
#include "pxr/imaging/hdSt/light.h"
#include "pxr/imaging/hdSt/package.h"
#include "pxr/imaging/hdSt/renderPass.h"
#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/renderPassState.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/glf/simpleLightingContext.h"
#include "pxr/imaging/glf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

HdStShaderCodeSharedPtr HdxShadowTask::_overrideShader;

bool
_HasDrawItems(HdRenderPassSharedPtr pass)
{
    HdSt_RenderPass *hdStRenderPass = static_cast<HdSt_RenderPass*>(pass.get());
    return hdStRenderPass && hdStRenderPass->GetDrawItemCount() > 0;
}

HdxShadowTask::HdxShadowTask(HdSceneDelegate* delegate, SdfPath const& id)
    : HdTask(id)
    , _passes()
    , _renderPassStates()
    , _params()
    , _renderTags()
{
}

HdxShadowTask::~HdxShadowTask() = default;

void
HdxShadowTask::Sync(HdSceneDelegate* delegate,
                    HdTaskContext* ctx,
                    HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    GLF_GROUP_FUNCTION();

    HdRenderIndex &renderIndex = delegate->GetRenderIndex();
    if (!renderIndex.IsSprimTypeSupported(HdPrimTypeTokens->simpleLight)) {
        // Clean to prevent repeated calling.
        *dirtyBits = HdChangeTracker::Clean;
        return;
    }

    // Extract the lighting context information from the task context
    GlfSimpleLightingContextRefPtr lightingContext;
    if (!_GetTaskContextData(ctx, 
            HdxTokens->lightingContext, 
            &lightingContext)) {
        return;
    }

    GlfSimpleLightVector const glfLights = lightingContext->GetLights();
    GlfSimpleShadowArrayRefPtr const shadows = lightingContext->GetShadows();

    // Extract the new shadow task params from scene delegate
    const bool dirtyParams = (*dirtyBits) & HdChangeTracker::DirtyParams;
    if (dirtyParams) {
        if (!_GetTaskParams(delegate, &_params)) {
            return;
        }
    }

    // Update render tags from scene delegate
    if ((*dirtyBits) & HdChangeTracker::DirtyRenderTags) {
        _renderTags = _GetTaskRenderTags(delegate);
    }

    // Make sure we have the right number of shadow render passes.
    // Because we would like to render only prims with the "defaultMaterialTag"
    // or "masked" material tag, we need to make two collections and thus two
    // render passes for what would be the same shadow map pass.  
    // Thus we must make a distinction between the number of render passes and 
    // the number of shadow maps indicated by the shadow array.
    size_t numShadowMaps = shadows->GetNumShadowMapPasses();
    static const TfToken shadowMaterialTags[2] = 
        { HdStMaterialTagTokens->defaultMaterialTag, 
          HdStMaterialTagTokens->masked };
    _passes.resize( TfArraySize(shadowMaterialTags) * numShadowMaps);

    // Mostly we can populate the renderpasses from shadow info, but the lights
    // contain the shadow collection; so we need to loop through the lights
    // assigning collections to their shadows.
    for (size_t lightId = 0; lightId < glfLights.size(); ++lightId) {

        if (!glfLights[lightId].HasShadow()) {
            continue;
        }

        const HdStLight* light = static_cast<const HdStLight*>(
            renderIndex.GetSprim(HdPrimTypeTokens->simpleLight,
                                 glfLights[lightId].GetID()));

        // It is possible the light is nullptr for area lights converted to 
        // simple lights, however they should not have shadows enabled.
        TF_VERIFY(light);

        // Extract the collection from the HD light
        VtValue vtShadowCollection =
            light->Get(HdLightTokens->shadowCollection);
        const HdRprimCollection &col =
            vtShadowCollection.IsHolding<HdRprimCollection>() ?
            vtShadowCollection.Get<HdRprimCollection>() : HdRprimCollection();

        // Only want opaque or masked prims to appear in shadow pass, so make
        // two copies of the shadow collection with appropriate material tags
        HdRprimCollection newColDefault = col;
        newColDefault.SetMaterialTag(shadowMaterialTags[0]);
        HdRprimCollection newColMasked = col;
        newColMasked.SetMaterialTag(shadowMaterialTags[1]);

        int shadowStart = glfLights[lightId].GetShadowIndexStart();
        int shadowEnd = glfLights[lightId].GetShadowIndexEnd();

        // Note here that we may want to sort the passes by collection
        // to invalidate fewer passes if the collections match already.
        // SetRprimCollection checks for identity changes on the collection
        // and no-ops in that case.
        for (int shadowId = shadowStart; shadowId <= shadowEnd; ++shadowId) {
            // Remember, we have two render passes (one for each collection)
            // per shadow map. First the "defaultMaterialTag" passes.
            if (_passes[shadowId]) {
                _passes[shadowId]->SetRprimCollection(newColDefault);
            } else {
                _passes[shadowId] = std::make_shared<HdSt_RenderPass>
                    (&renderIndex, newColDefault);
            }

            // Then the "masked" materialTag passes
            if (_passes[shadowId + numShadowMaps]) {
                _passes[shadowId + numShadowMaps]->SetRprimCollection(
                    newColMasked);
            } else {
                _passes[shadowId + numShadowMaps] = 
                    std::make_shared<HdSt_RenderPass>
                    (&renderIndex, newColMasked);
            }
        }
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
            HdStRenderPassShaderSharedPtr renderPassShadowShader = 
                std::make_shared<HdStRenderPassShader>(
                    HdxPackageRenderPassShadowShader());
            HdStRenderPassStateSharedPtr renderPassState =
                std::make_shared<HdStRenderPassState>(
                    renderPassShadowShader);

            renderPassState->SetDepthFunc(_params.depthFunc);
            renderPassState->SetDepthBiasUseDefault(!_params.depthBiasEnable);
            renderPassState->SetDepthBiasEnabled(_params.depthBiasEnable);
            renderPassState->SetDepthBias(_params.depthBiasConstantFactor,
                                          _params.depthBiasSlopeFactor);

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

            _renderPassStates.push_back(std::move(renderPassState));
        }
    }

    for (size_t passId = 0; passId < _passes.size(); passId++) {

        // Make sure each pass got created. Light shadow indices are supposed
        // to be compact (see simpleLightTask.cpp).
        if (!TF_VERIFY(_passes[passId])) {
            continue;
        }

        // Because we create two render passes for each shadow map, we must 
        // convert the index
        size_t shadowMapId = passId % numShadowMaps;

        GfVec2i shadowMapRes = shadows->GetShadowMapSize(shadowMapId);

        // Set camera framing based on the shadow map's, which is computed in
        // HdxSimpleLightTask.
        _renderPassStates[passId]->SetCameraFramingState( 
            shadows->GetViewMatrix(shadowMapId), 
            shadows->GetProjectionMatrix(shadowMapId),
            GfVec4d(0,0,shadowMapRes[0], shadowMapRes[1]),
            HdRenderPassState::ClipPlanesVector());

        _passes[passId]->Sync();
    }

    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxShadowTask::Prepare(HdTaskContext* ctx,
                       HdRenderIndex* renderIndex)
{
    HdResourceRegistrySharedPtr resourceRegistry =
        renderIndex->GetResourceRegistry();

    for(size_t passId = 0; passId < _passes.size(); passId++) {
        _renderPassStates[passId]->Prepare(resourceRegistry);
        _passes[passId]->Prepare(GetRenderTags());
    }
}

void
HdxShadowTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    GLF_GROUP_FUNCTION();

    // Extract the lighting context information from the task context
    GlfSimpleLightingContextRefPtr lightingContext;
    if (!_GetTaskContextData(ctx,
            HdxTokens->lightingContext, &lightingContext)) {
        return;
    }

    // Generate the actual shadow maps
    GlfSimpleShadowArrayRefPtr const shadows = lightingContext->GetShadows();
    size_t numShadowMaps = shadows->GetNumShadowMapPasses();
    for (size_t shadowId = 0; shadowId < numShadowMaps; shadowId++) {

        // Make sure each pass got created. Light shadow indices are supposed
        // to be compact (see simpleLightTask.cpp).
        if (!TF_VERIFY(_passes[shadowId]) || 
            !TF_VERIFY(_passes[shadowId + numShadowMaps])) {
            continue;
        }

        // Bind the framebuffer that will store shadowId shadow map
        shadows->BeginCapture(shadowId, true);

        if (_HasDrawItems(_passes[shadowId])) {
            // Render the actual geometry in the "defaultMaterialTag" collection
            _passes[shadowId]->Execute(
                _renderPassStates[shadowId],
                GetRenderTags());
        }

        if (_HasDrawItems(_passes[shadowId + numShadowMaps])) {
            // Render the actual geometry in the "masked" materialTag collection
            _passes[shadowId + numShadowMaps]->Execute(
                _renderPassStates[shadowId + numShadowMaps],
                GetRenderTags());
        }
   
        // Unbind the buffer and move on to the next shadow map
        shadows->EndCapture(shadowId);
    }
}

const TfTokenVector &
HdxShadowTask::GetRenderTags() const
{
    return _renderTags;
}

void
HdxShadowTask::_CreateOverrideShader()
{
    static std::mutex shaderCreateLock;

    if (!_overrideShader) {
        std::lock_guard<std::mutex> lock(shaderCreateLock);
        if (!_overrideShader) {
            _overrideShader =
                std::make_shared<HdStGLSLFXShader>(
                    std::make_shared<HioGlslfx>(
                        HdStPackageFallbackSurfaceShader()));
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
HdxShadowTask::_UpdateDirtyParams(HdStRenderPassStateSharedPtr &renderPassState,
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
        ;
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
            lhs.cullStyle == rhs.cullStyle
            ;
}

bool operator!=(const HdxShadowTaskParams& lhs, const HdxShadowTaskParams& rhs) 
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE

