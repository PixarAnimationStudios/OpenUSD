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
#include "pxr/imaging/hdx/light.h"
#include "pxr/imaging/hdx/simpleLightTask.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hd/lightingShader.h"

#include "pxr/imaging/glf/simpleLightingContext.h"

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
    HD_MALLOC_TAG_FUNCTION();

    // Extract the lighting context information from the task context
    GlfSimpleLightingContextRefPtr lightingContext;
    if (not _GetTaskContextData(ctx, HdxTokens->lightingContext, &lightingContext)) {
        return;
    }

    // Store the current viewport so we can reset after generating the shadows.
    // Every call to BeginCapture will run internally a glViewport so we need
    // to pop the state after creating the shadows
    //
    // Also store the polygon bit for polygon offset
    glPushAttrib(GL_VIEWPORT_BIT | GL_POLYGON_BIT);

    if (_depthBiasEnable) {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(_depthBiasSlopeFactor, _depthBiasConstantFactor);
    } else {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
    // XXX: Move conversion to sync time once Task header becomes private.
    glDepthFunc(HdConversions::GetGlDepthFunc(_depthFunc));

    // XXX: Do we ever want to disable this?
    GLboolean oldPointSizeEnabled = glIsEnabled(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_PROGRAM_POINT_SIZE);

    // Generate the actual shadow maps
    GlfSimpleShadowArrayRefPtr const shadows = lightingContext->GetShadows();
    for(size_t shadowId = 0; shadowId < shadows->GetNumLayers(); shadowId++) {

        // Bind the framebuffer that will store shadowId shadow map
        shadows->BeginCapture(shadowId, true);

        // Render the actual geometry in the collection
        _passes[shadowId]->Execute(_renderPassStates[shadowId]);

        // Unbind the buffer and move on to the next shadow map
        shadows->EndCapture(shadowId);
    }

    if (!oldPointSizeEnabled)
    {
        glDisable(GL_PROGRAM_POINT_SIZE);
    }

    glPopAttrib();

}

void
HdxShadowTask::_Sync(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    // Extract the lighting context information from the task context
    GlfSimpleLightingContextRefPtr lightingContext;
    if (not _GetTaskContextData(ctx, HdxTokens->lightingContext, &lightingContext)) {
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
        if (not _GetSceneDelegateValue(HdTokens->params, &params)) {
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
        
        // Extract the HD lights used to render the scene from the 
        // task context, we will use them to find out what
        // lights are dirty and if we need to update the
        // collection for shadows mapping
        
        // XXX: This is inefficient, need to be optimized
        SdfPathVector sprimPaths = delegate->GetRenderIndex().GetSprimSubtree(
            SdfPath::AbsoluteRootPath());
        SdfPathVector lightPaths =
            HdxSimpleLightTask::ComputeIncludedLights(
                sprimPaths,
                params.lightIncludePaths,
                params.lightExcludePaths);
        
        HdSprimSharedPtrVector lights;
        TF_FOR_ALL (it, lightPaths) {
            HdSprimSharedPtr const &sprim = delegate->GetRenderIndex().GetSprim(*it);
            // XXX: or we could say instead of downcast,
            //      sprim->Has(HdxLightInterface) ?
            if (boost::dynamic_pointer_cast<HdxLight>(sprim)) {
                lights.push_back(sprim);
            }
        }
        
        GlfSimpleLightVector const glfLights = lightingContext->GetLights();
        
        TF_VERIFY(lights.size() == glfLights.size());
        
        // Iterate through all lights and for those that have
        // shadows enabled we will extract the colection from 
        // the render index and create a pass that during execution
        // it will be used for generating each shadowmap
        for (size_t lightId = 0; lightId < glfLights.size(); lightId++) {
            
            if (not glfLights[lightId].HasShadow()) {
                continue;
            }
            
            // Extract the collection from the HD light
            VtValue vtShadowCollection =
                lights[lightId]->Get(HdxLightTokens->shadowCollection);
            const HdRprimCollection &col =
                vtShadowCollection.IsHolding<HdRprimCollection>() ?
                    vtShadowCollection.Get<HdRprimCollection>() : HdRprimCollection();

            // Creates a pass with the right geometry that will be
            // use during Execute phase to draw the maps
            HdRenderPassSharedPtr p = boost::make_shared<HdRenderPass>
                (&delegate->GetRenderIndex(), col);

            HdRenderPassStateSharedPtr renderPassState(new HdRenderPassState());

            // Update the rest of the renderpass state parameters for this pass
            renderPassState->SetOverrideColor(params.overrideColor);
            renderPassState->SetWireframeColor(params.wireframeColor);
            renderPassState->SetLightingEnabled(false);
            // XXX : This can be removed when Hydra has support for 
            //       transparent objects.
            renderPassState->SetAlphaThreshold(1.0 /* params.alphaThreshold */);
            renderPassState->SetTessLevel(params.tessLevel);
            renderPassState->SetDrawingRange(params.drawingRange);
                
            // Invert front and back faces for shadow
            // XXX: this should be taken care by shadow-repr.
            switch (params.cullStyle)
            {
                case HdCullStyleBack:
                    renderPassState->SetCullStyle(HdCullStyleFront);
                    break;
                    
                case HdCullStyleFront:
                    renderPassState->SetCullStyle(HdCullStyleBack);
                    break;

                case HdCullStyleBackUnlessDoubleSided:
                    renderPassState->SetCullStyle(HdCullStyleFrontUnlessDoubleSided);
                    break;

                case HdCullStyleFrontUnlessDoubleSided:
                    renderPassState->SetCullStyle(HdCullStyleBackUnlessDoubleSided);
                    break;
                    
                default:
                    renderPassState->SetCullStyle(params.cullStyle);
                    break;
            }
            
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

        _renderPassStates[passId]->Sync();
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
    return  lhs.overrideColor == rhs.overrideColor and
            lhs.wireframeColor == rhs.wireframeColor and
            lhs.enableLighting == rhs.enableLighting and
            lhs.enableIdRender == rhs.enableIdRender and
            lhs.alphaThreshold == rhs.alphaThreshold and
            lhs.tessLevel == rhs.tessLevel and
            lhs.drawingRange == rhs.drawingRange and
            lhs.depthBiasEnable == rhs.depthBiasEnable and
            lhs.depthBiasConstantFactor == rhs.depthBiasConstantFactor and
            lhs.depthBiasSlopeFactor == rhs.depthBiasSlopeFactor and
            lhs.depthFunc == rhs.depthFunc and
            lhs.cullStyle == rhs.cullStyle and
            lhs.camera == rhs.camera and
            lhs.viewport == rhs.viewport and
            lhs.lightIncludePaths == rhs.lightIncludePaths and
            lhs.lightExcludePaths == rhs.lightExcludePaths
            ;
}

bool operator!=(const HdxShadowTaskParams& lhs, const HdxShadowTaskParams& rhs) 
{
    return not(lhs == rhs);
}
