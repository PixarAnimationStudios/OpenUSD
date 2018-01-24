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
#include "pxr/imaging/hdSt/glConversions.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/primGather.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/imaging/hdSt/light.h"
#include "pxr/imaging/hdSt/lightingShader.h"
#include "pxr/imaging/hdSt/renderPass.h"
#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/renderPassState.h"

#include "pxr/imaging/glf/simpleLightingContext.h"

PXR_NAMESPACE_OPEN_SCOPE


HdxShadowTask::HdxShadowTask(HdSceneDelegate* delegate, SdfPath const& id)
    : HdSceneTask(delegate, id)
    , _collectionVersion(0)
    , _depthBiasEnable(false)
    , _depthBiasConstantFactor(0.0f)
    , _depthBiasSlopeFactor(1.0f)
    , _depthFunc(HdCmpFuncLEqual)
{
}

void
HdxShadowTask::_Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Extract the lighting context information from the task context
    GlfSimpleLightingContextRefPtr lightingContext;
    if (!_GetTaskContextData(ctx, HdxTokens->lightingContext, &lightingContext)) {
        return;
    }

    if (_depthBiasEnable) {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(_depthBiasSlopeFactor, _depthBiasConstantFactor);
    } else {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    // XXX: Move conversion to sync time once Task header becomes private.
    glDepthFunc(HdStGLConversions::GetGlDepthFunc(_depthFunc));
    glEnable(GL_PROGRAM_POINT_SIZE);

    // Generate the actual shadow maps
    GlfSimpleShadowArrayRefPtr const shadows = lightingContext->GetShadows();
    for(size_t shadowId = 0; shadowId < shadows->GetNumLayers(); shadowId++) {

        // Bind the framebuffer that will store shadowId shadow map
        shadows->BeginCapture(shadowId, true);

        // Render the actual geometry in the collection
        _passes[shadowId]->Execute(_renderPassStates[shadowId], HdTokens->geometry);

        // Unbind the buffer and move on to the next shadow map
        shadows->EndCapture(shadowId);
    }

    // restore GL states to default
    glDisable(GL_PROGRAM_POINT_SIZE);
    glDisable(GL_POLYGON_OFFSET_FILL);
}

void
HdxShadowTask::_Sync(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Extract the lighting context information from the task context
    GlfSimpleLightingContextRefPtr lightingContext;
    if (!_GetTaskContextData(ctx, HdxTokens->lightingContext, &lightingContext)) {
        return;
    }
    GlfSimpleShadowArrayRefPtr const shadows = lightingContext->GetShadows();

    _TaskDirtyState dirtyState;

    _GetTaskDirtyState(HdTokens->geometry, &dirtyState);


    // Check if the collection version has changed, if so, it means
    // that we should extract the new collections
    // from the lights in the render index, and then recreate the passes
    // required to render the shadow maps
    const bool collectionChanged = (_collectionVersion != dirtyState.collectionVersion);

    if ((dirtyState.bits & HdChangeTracker::DirtyParams) ||
        collectionChanged) {
        _collectionVersion = dirtyState.collectionVersion;

        // Extract the new shadow task params from exec
        HdxShadowTaskParams params;
        if (!_GetSceneDelegateValue(HdTokens->params, &params)) {
            return;
        }

        _depthBiasEnable = params.depthBiasEnable;
        _depthBiasConstantFactor = params.depthBiasConstantFactor;
        _depthBiasSlopeFactor = params.depthBiasSlopeFactor;
        _depthFunc = params.depthFunc;

        // XXX TODO: What to do about complexity?

        // We can now use the light information now
        // and create a pass for each
        _passes.clear();
        _renderPassStates.clear();

        HdSceneDelegate* delegate = GetDelegate();
        HdRenderIndex &renderIndex = delegate->GetRenderIndex();
        
        // Extract the HD lights used to render the scene from the 
        // task context, we will use them to find out what
        // lights are dirty and if we need to update the
        // collection for shadows mapping
        
        // XXX: This is inefficient, need to be optimized
        SdfPathVector lightPaths;
        if (renderIndex.IsSprimTypeSupported(HdPrimTypeTokens->light)) {
            SdfPathVector sprimPaths = renderIndex.GetSprimSubtree(
                HdPrimTypeTokens->light, SdfPath::AbsoluteRootPath());

            HdPrimGather gather;

            gather.Filter(sprimPaths,
                          params.lightIncludePaths,
                          params.lightExcludePaths,
                          &lightPaths);
        }
        
        HdStLightPtrConstVector lights;
        TF_FOR_ALL (it, lightPaths) {
             const HdStLight *light = static_cast<const HdStLight *>(
                                         renderIndex.GetSprim(
                                                 HdPrimTypeTokens->light, *it));

             if (light != nullptr) {
                lights.push_back(light);
            }
        }
        
        GlfSimpleLightVector const glfLights = lightingContext->GetLights();
        
        if (!renderIndex.IsSprimTypeSupported(HdPrimTypeTokens->light) ||
            !TF_VERIFY(lights.size() == glfLights.size())) {
            return;
        }
        
        // Iterate through all lights and for those that have
        // shadows enabled we will extract the colection from 
        // the render index and create a pass that during execution
        // it will be used for generating each shadowmap
        for (size_t lightId = 0; lightId < glfLights.size(); lightId++) {
            
            if (!glfLights[lightId].HasShadow()) {
                continue;
            }
            
            // Extract the collection from the HD light
            VtValue vtShadowCollection =
                lights[lightId]->Get(HdStLightTokens->shadowCollection);
            const HdRprimCollection &col =
                vtShadowCollection.IsHolding<HdRprimCollection>() ?
                    vtShadowCollection.Get<HdRprimCollection>() : HdRprimCollection();

            // Creates a pass with the right geometry that will be
            // use during Execute phase to draw the maps
            HdRenderPassSharedPtr p = boost::make_shared<HdSt_RenderPass>
                (&delegate->GetRenderIndex(), col);

            HdStRenderPassShaderSharedPtr renderPassShadowShader
                (new HdStRenderPassShader(HdxPackageRenderPassShadowShader()));
            HdStRenderPassStateSharedPtr renderPassState
                (new HdStRenderPassState(renderPassShadowShader));

            // Update the rest of the renderpass state parameters for this pass
            renderPassState->SetOverrideColor(params.overrideColor);
            renderPassState->SetWireframeColor(params.wireframeColor);
            renderPassState->SetLightingEnabled(false);
            
            // XXX : This can be removed when Hydra has support for 
            //       transparent objects.
            //       We use an epsilon offset from 1.0 to allow for 
            //       calculation during primvar interpolation which
            //       doesn't fully saturate back to 1.0.
            const float TRANSPARENT_ALPHA_THRESHOLD = (1.0f - 1e-6f);
            renderPassState->SetAlphaThreshold(TRANSPARENT_ALPHA_THRESHOLD);
            renderPassState->SetTessLevel(params.tessLevel);
            renderPassState->SetDrawingRange(params.drawingRange);
            renderPassState->SetCullStyle(params.cullStyle);
            
            _passes.push_back(p);
            _renderPassStates.push_back(renderPassState);
        }
    }

    for(size_t passId = 0; passId < _passes.size() ; passId++) {
        // Move the camera to the correct position to take the shadow map
        _renderPassStates[passId]->SetCamera( 
            shadows->GetViewMatrix(passId), 
            shadows->GetProjectionMatrix(passId),
            GfVec4d(0,0,shadows->GetSize()[0],shadows->GetSize()[1]));

        _renderPassStates[passId]->Sync(
            GetDelegate()->GetRenderIndex().GetResourceRegistry());
        _passes[passId]->Sync();
    }
}

// --------------------------------------------------------------------------- //
// VtValue Requirements
// --------------------------------------------------------------------------- //

std::ostream& operator<<(std::ostream& out, const HdxShadowTaskParams& pv)
{
    out << "ShadowTask Params: (...) "
        << pv.overrideColor << " " 
        << pv.wireframeColor << " " 
        << pv.enableLighting << " "
        << pv.enableIdRender << " "
        << pv.alphaThreshold << " "
        << pv.tessLevel << " "
        << pv.drawingRange << " "
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
            lhs.alphaThreshold == rhs.alphaThreshold                    &&
            lhs.tessLevel == rhs.tessLevel                              && 
            lhs.drawingRange == rhs.drawingRange                        && 
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

