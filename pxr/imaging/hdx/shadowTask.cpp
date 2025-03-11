//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/shadowTask.h"
#include "pxr/imaging/hdx/debugCodes.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hdx/package.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/imaging/hdSt/hioConversions.h"
#include "pxr/imaging/hdSt/light.h"
#include "pxr/imaging/hdSt/renderPass.h"
#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/renderPassState.h"
#include "pxr/imaging/hdSt/simpleLightingShader.h"
#include "pxr/imaging/hdSt/textureUtils.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/glf/simpleLightingContext.h"
#include "pxr/imaging/glf/diagnostic.h"

#include "pxr/imaging/hio/image.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/scoped.h"

PXR_NAMESPACE_OPEN_SCOPE

static bool
_HasDrawItems(HdRenderPassSharedPtr pass,
              const TfTokenVector &renderTags)
{
    HdSt_RenderPass *hdStRenderPass = static_cast<HdSt_RenderPass*>(pass.get());
    return hdStRenderPass && hdStRenderPass->HasDrawItems(renderTags);
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

    GlfSimpleLightVector const glfLights = lightingContext->GetLights();
    GlfSimpleShadowArrayRefPtr const shadows = lightingContext->GetShadows();
    const size_t numShadowMaps = shadows->GetNumShadowMapPasses();

    if (numShadowMaps > 0) {
        // Make sure we have the right number of shadow render passes.
        // Because we would like to render only prims with the 
        // "defaultMaterialTag" or "masked" material tag, we need to make two 
        // collections and thus two render passes for what would be the same 
        // shadow map pass. Thus we must make a distinction between the number 
        // of render passes and the number of shadow maps indicated by the 
        // shadow array.
        static const TfToken shadowMaterialTags[2] = 
            { HdStMaterialTagTokens->defaultMaterialTag, 
              HdStMaterialTagTokens->masked };
        _passes.resize( TfArraySize(shadowMaterialTags) * numShadowMaps);

        // Mostly we can populate the renderpasses from shadow info, but the 
        // lights contain the shadow collection; so we need to loop through the 
        // lights assigning collections to their shadows.
        for (size_t lightId = 0; lightId < glfLights.size(); ++lightId) {

            if (!glfLights[lightId].HasShadow()) {
                continue;
            }

            // Shadows are supported on for SimpleLights and DistantLights
            HdStLight* light = static_cast<HdStLight*>(renderIndex.GetSprim(
                    HdPrimTypeTokens->simpleLight, glfLights[lightId].GetID()));
            if (!light) {
                light = static_cast<HdStLight*>(renderIndex.GetSprim(
                    HdPrimTypeTokens->distantLight, glfLights[lightId].GetID()));
            }

            TF_VERIFY(light);

            // Extract the collection from the HD light
            VtValue vtShadowCollection =
                light->Get(HdLightTokens->shadowCollection);
            const HdRprimCollection &col =
                vtShadowCollection.IsHolding<HdRprimCollection>()
                    ? vtShadowCollection.Get<HdRprimCollection>()
                    : HdRprimCollection();

            // Only want opaque or masked prims to appear in shadow pass, so 
            // make two copies of the shadow collection with appropriate 
            // material tags
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
            for (int shadowId = shadowStart; shadowId <= shadowEnd; ++shadowId){
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

                //                
                // The pipeline state below merits explanation.
                // Hardcoded state:
                // 1. Depth clamping is enabled, which disables clipping for the
                //    clip-space Z coordinate. So, objects between the shadow
                //    camera and the near plane, and those behind the far plane
                //    may not be clipped.
                // 2, The depth range is set to [0.0, 0.99999] and not [0,1].
                //    This is done to clamps the depth of objects behind the far
                //    plane of the shadow frustum to 1. Note that the hardware
                //    always clamps depth values to [0,1], even when using float
                //    depth buffer formats. See ARB_depth_buffer_float.
                // 
                // Configurable state:
                // a. Depth function: This goes hand-in-hand with the shadow
                //    sampler's compare function, the depth range and the clear
                //    value used. All of these are currently hardcoded!
                //    XXX The simple lighting shader hardcodes the compare to
                //        LEQUAL and the clear value to 1.0.
                //        See HdStSimpleLightingShader::AllocateTextureHandles.
                //
                // b. Slope-scale bias:
                //    This offsets the fragment's interpolated depth using the
                //    slope and constant factors.
                //    XXX Do positive values push away or towards the eye?
                //     
                renderPassState->SetDepthFunc(_params.depthFunc);
                renderPassState->SetDepthBiasUseDefault(
                    !_params.depthBiasEnable);
                renderPassState->SetDepthBiasEnabled(_params.depthBiasEnable);
                renderPassState->SetDepthBias(_params.depthBiasConstantFactor,
                                              _params.depthBiasSlopeFactor);
                renderPassState->SetEnableDepthClamp(true);
                renderPassState->SetDepthRange(GfVec2f(0, 0.99999));

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

        // Get AOV bindings created by simple lighting shader.
        HdRenderPassAovBindingVector shadowAovBindings;
        VtValue lightingShader = (*ctx)[HdxTokens->lightingShader];
        if (lightingShader.IsHolding<HdStLightingShaderSharedPtr>()) {
            if (HdStSimpleLightingShaderSharedPtr simpleLightingShader =
                std::dynamic_pointer_cast<HdStSimpleLightingShader>(
                    lightingShader.UncheckedGet<HdStLightingShaderSharedPtr>())) {
                shadowAovBindings =
                    simpleLightingShader->GetShadowAovBindings();
            }
        }

        for (size_t passId = 0; passId < _passes.size(); passId++) {
            // Make sure each pass got created. Light shadow indices are 
            // supposed to be compact (see simpleLightTask.cpp).
            if (!TF_VERIFY(_passes[passId])) {
                continue;
            }

            // Because we create two render passes for each shadow map, we must 
            // convert the index
            size_t shadowMapId = passId % numShadowMaps;

            GfVec2i shadowMapRes = shadows->GetShadowMapSize(shadowMapId);

            // Set camera framing based on the shadow map's, which is computed 
            // in HdxSimpleLightTask.
            _renderPassStates[passId]->SetCameraFramingState( 
                shadows->GetViewMatrix(shadowMapId), 
                shadows->GetProjectionMatrix(shadowMapId),
                GfVec4d(0,0,shadowMapRes[0], shadowMapRes[1]),
                HdRenderPassState::ClipPlanesVector());
            
            // Set AOV bindings.
            if (shadowMapId < shadowAovBindings.size()) {
                if (passId == shadowMapId) {
                    _renderPassStates[passId]->SetAovBindings(
                        { shadowAovBindings[shadowMapId] });
                } else {
                    // Don't want "masked" render passes to clear.
                    HdRenderPassAovBinding aovBindingCopy = 
                        shadowAovBindings[shadowMapId];
                    aovBindingCopy.clearValue = VtValue();
                    _renderPassStates[passId]->SetAovBindings(
                        { aovBindingCopy });
                }
            }

            _passes[passId]->Sync();
        }
    }

    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxShadowTask::Prepare(HdTaskContext* ctx,
                       HdRenderIndex* renderIndex)
{
    GlfSimpleLightingContextRefPtr lightingContext;
    if (!_GetTaskContextData(ctx,
            HdxTokens->lightingContext, &lightingContext)) {
        return;
    }

    GlfSimpleShadowArrayRefPtr const shadows = lightingContext->GetShadows();
    if (shadows->GetNumShadowMapPasses() == 0) {
        // Bail if we are not generating shadow maps. We don't want to call
        // Prepare on outdated AOV bindings.
        return;
    }

    HdResourceRegistrySharedPtr resourceRegistry =
        renderIndex->GetResourceRegistry();

    for(size_t passId = 0; passId < _passes.size(); passId++) {
        _renderPassStates[passId]->Prepare(resourceRegistry);
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

    // Get AOV bindings.
    HdRenderPassAovBindingVector shadowAovBindings;
    VtValue lightingShader = (*ctx)[HdxTokens->lightingShader];
    if (lightingShader.IsHolding<HdStLightingShaderSharedPtr>()) {
        if (HdStSimpleLightingShaderSharedPtr simpleLightingShader =
            std::dynamic_pointer_cast<HdStSimpleLightingShader>(
                lightingShader.UncheckedGet<HdStLightingShaderSharedPtr>())) {
            shadowAovBindings = simpleLightingShader->GetShadowAovBindings();
        }
    }

    // Though we no longer use GlfSimpleShadowArray's raw GL code to capture 
    // shadows in Hdx, Presto expects the textures in GlfSimpleShadowArray to 
    // contain the shadows captured here. We fulfill this by setting 
    // GlfSimpleShadowArray's shadow textures to the textures backing the 
    // shadow render buffers.
    std::vector<uint32_t> textureIds;
    for (size_t shadowId = 0; shadowId < numShadowMaps; shadowId++) {
        if (shadowId < shadowAovBindings.size()) {
            HdRenderBuffer const * renderBuffer = 
                shadowAovBindings[shadowId].renderBuffer;
            VtValue aov = renderBuffer->GetResource(false);
            if (aov.IsHolding<HgiTextureHandle>()) {
                HgiTextureHandle texture = aov.UncheckedGet<HgiTextureHandle>();
                if (texture) {
                    textureIds.push_back((uint32_t)texture->GetRawResource());
                }
            }
        }
    }
    shadows->SetTextures(textureIds);

    for (size_t shadowId = 0; shadowId < numShadowMaps; shadowId++) {

        // Make sure each pass got created. Light shadow indices are supposed
        // to be compact (see simpleLightTask.cpp).
        if (!TF_VERIFY(_passes[shadowId]) || 
            !TF_VERIFY(_passes[shadowId + numShadowMaps])) {
            continue;
        }

        // Render the actual geometry in the "defaultMaterialTag" collection.
        // Always execute this render pass because it clears the AOVs.
        _passes[shadowId]->Execute(
            _renderPassStates[shadowId],
            GetRenderTags());

        if (_HasDrawItems(_passes[shadowId + numShadowMaps], GetRenderTags())) {
            // Render the actual geometry in the "masked" materialTag collection
            _passes[shadowId + numShadowMaps]->Execute(
               _renderPassStates[shadowId + numShadowMaps],
               GetRenderTags());
        }
    }

    if (TfDebug::IsEnabled(HDX_DEBUG_DUMP_SHADOW_TEXTURES)) {
        for (size_t shadowId = 0; shadowId < numShadowMaps; shadowId++) {
            HdRenderBuffer * renderBuffer = 
                shadowAovBindings[shadowId].renderBuffer;
            HioImage::StorageSpec storage;
            storage.width = renderBuffer->GetWidth();
            storage.height = renderBuffer->GetHeight();
            storage.format = 
                HdStHioConversions::GetHioFormat(renderBuffer->GetFormat());
            storage.flipped = true;
            storage.data = renderBuffer->Map();
            TfScoped<> scopedUnmap([renderBuffer](){ renderBuffer->Unmap(); });

            const std::string filename = ArchNormPath(
                TfStringPrintf("%s/HdxShadowTask.%zu.png",
                    ArchGetTmpDir(),
                    shadowId));

            if (storage.format == HioFormatInvalid) {
                TfDebug::Helper().Msg(
                    "Hgi texture has format not corresponding to an "
                    "HioFormat: %s\n", filename.c_str());
                continue;
            }

            if (!storage.data) {
                TfDebug::Helper().Msg(
                    "No data for texture: %s\n", filename.c_str());
                continue;
            }
                        
            HioImageSharedPtr const image = HioImage::OpenForWriting(filename);
            if (!image) {
                TfDebug::Helper().Msg(
                    "Failed to open image for writing: %s\n", filename.c_str());
                continue;
            }

            if (image->Write(storage)) {
                TfDebug::Helper().Msg(
                    "Wrote shadow texture: %s\n", filename.c_str());
            } else {
                TfDebug::Helper().Msg(
                    "Failed to write shadow texture: %s\n", filename.c_str());
            }
        }
    }
}

const TfTokenVector &
HdxShadowTask::GetRenderTags() const
{
    return _renderTags;
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
        extendedState->SetUseSceneMaterials(params.enableSceneMaterials);
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

