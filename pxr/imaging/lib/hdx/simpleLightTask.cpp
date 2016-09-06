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
#include "pxr/imaging/hdx/simpleLightTask.h"

#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hdx/simpleLightingShader.h"
#include "pxr/imaging/hdx/shadowMatrixComputation.h"

#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/light.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/imaging/glf/simpleLight.h"

#include "pxr/base/gf/frustum.h"

#include <boost/bind.hpp>

static const GfVec2i _defaultShadowRes = GfVec2i(1024, 1024);

// -------------------------------------------------------------------------- //

HdxSimpleLightTask::HdxSimpleLightTask(HdSceneDelegate* delegate, SdfPath const& id)
    : HdSceneTask(delegate, id) 
    , _camera()
    , _lights()
    , _lightingShader(new HdxSimpleLightingShader())
    , _collectionVersion(0)
    , _enableShadows(false)
    , _viewport(0.0f, 0.0f, 0.0f, 0.0f)
    , _material()
    , _sceneAmbient()
    , _shadows()
    , _glfSimpleLights()
{
    _shadows = TfCreateRefPtr(new GlfSimpleShadowArray(_defaultShadowRes, 0));
}

void
HdxSimpleLightTask::_Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();
}

void
HdxSimpleLightTask::_Sync(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();

    // Store the lighting context in the task context 
    // so later on other tasks can use this information 
    // draw shadows or other purposes
    (*ctx)[HdTokens->lightingShader] =
        boost::dynamic_pointer_cast<HdLightingShader>(_lightingShader);

    _TaskDirtyState dirtyState;

    _GetTaskDirtyState(HdTokens->geometry, &dirtyState);

    // Check if the collection version has changed, if so, it means
    // that we should extract the lights again from the render index.
    const bool collectionChanged = (_collectionVersion != dirtyState.collectionVersion);


    if ((dirtyState.bits & HdChangeTracker::DirtyParams) ||
        collectionChanged) {

        _collectionVersion = dirtyState.collectionVersion;

        HdxSimpleLightTaskParams params;
        if (not _GetSceneDelegateValue(HdTokens->params, &params)) {
            return;
        }

        HdSceneDelegate* delegate = GetDelegate();
        _camera = delegate->GetRenderIndex().GetCamera(params.cameraPath);
        _lights = _ComputeIncludedLights(delegate->GetRenderIndex().GetLights(),
                                         params.lightIncludePaths,
                                         params.lightExcludePaths);
        _enableShadows = params.enableShadows;

        // XXX: compatibility hack for passing some unit tests until we have
        //      more formal material plumbing.
        _material = params.material;
        _sceneAmbient = params.sceneAmbient;

        _viewport = params.viewport;
    }

    if (not TF_VERIFY(_camera)) {
        return;
    }

    GlfSimpleLightingContextRefPtr const& lightingContext = 
                                    _lightingShader->GetLightingContext(); 
    if (not TF_VERIFY(lightingContext)) {
        return;
    }

    // Place lighting context in task context
    (*ctx)[HdTokens->lightingContext] = lightingContext;

    VtValue modelViewMatrix = _camera->Get(HdShaderTokens->worldToViewMatrix);
    TF_VERIFY(modelViewMatrix.IsHolding<GfMatrix4d>());
    VtValue projectionMatrix = _camera->Get(HdShaderTokens->projectionMatrix);
    TF_VERIFY(projectionMatrix.IsHolding<GfMatrix4d>());
    GfMatrix4d invCamXform = modelViewMatrix.Get<GfMatrix4d>().GetInverse();

    // Unique identifier for lights with shadows
    int shadowIndex = -1;

    // Value used to extract the maximum resolution from all shadow maps 
    // because we need to create an array of shadow maps with the same resolution
    int maxShadowRes = 0;

    // We rebuild the lights array every time, but avoid reallocating
    // the array every frame as this was showing up as a significant portion
    // of the time in this function.
    //
    // clear() is guaranteed by spec to not change the capacity of the
    // vector.
    size_t numLights = _lights.size();

    _glfSimpleLights.clear();
    if (numLights != _glfSimpleLights.capacity()) {
        // We don't just want to reserve here as we want to try and
        // recover memory if the number of lights shrinks.

        _glfSimpleLights.shrink_to_fit();
        _glfSimpleLights.reserve(numLights);
    }

    for (size_t lightId = 0; lightId < numLights; ++lightId) {
        // Take a copy of the simple light into our temporary array and
        // update it with viewer-dependant values.
        _glfSimpleLights.push_back(_lights[lightId]->GetParams());

        // Get a reference to the light, so we can patch it.
        GlfSimpleLight &glfl = _glfSimpleLights.back();

        // XXX: Pass id of light to Glf simple light, so that
        // glim can get access back to the light prim
        glfl.SetID(_lights[lightId]->GetID());

        // If the light is in camera space we need to transform
        // the position and spot direction to the right
        // space
        if (glfl.IsCameraSpaceLight()) {
            const GfMatrix4d &lightXform = _lights[lightId]->GetTransform();

            GfVec4f lightPos(lightXform.GetRow(2));
            lightPos[3] = 0.0f;
            GfVec3d lightDir(-lightPos[2]);
            glfl.SetPosition(lightPos * invCamXform);
            glfl.SetSpotDirection(GfVec3f(invCamXform.TransformDir(lightDir)));
        }
         
        VtValue vLightShadowParams =  _lights[lightId]->GetShadowParams();
        TF_VERIFY(vLightShadowParams.IsHolding<HdxShadowParams>());
        HdxShadowParams lightShadowParams = 
            vLightShadowParams.GetWithDefault<HdxShadowParams>(HdxShadowParams());

        // If shadows are disabled from the rendergraph then
        // we treat this light as if it had the shadow disabled
        // doing so we guarantee that shadowIndex will be -1
        // which will not create memory for the shadow maps 
        if (not _enableShadows or not lightShadowParams.enabled) {
            glfl.SetHasShadow(false);
        } 

        // Setup the rest of the light parameters necessary 
        // to calculate shadows
        if (glfl.HasShadow()) {
            // Extract the window policy to adjust the frustum correctly
            VtValue windowPolicy = _camera->Get(HdTokens->windowPolicy);
            if (not TF_VERIFY(windowPolicy.IsHolding<CameraUtilConformWindowPolicy>())) {
                return;
            }

            CameraUtilConformWindowPolicy policy =
                windowPolicy.Get<CameraUtilConformWindowPolicy>();

            GfMatrix4d shadowMatrix = lightShadowParams.shadowMatrix->Compute(_viewport, policy);

            glfl.SetShadowIndex(++shadowIndex);
            glfl.SetShadowMatrix(shadowMatrix);
            glfl.SetShadowBias(lightShadowParams.bias);
            glfl.SetShadowBlur(lightShadowParams.blur);
            glfl.SetShadowResolution(lightShadowParams.resolution);
            maxShadowRes = GfMax(maxShadowRes, glfl.GetShadowResolution());
        }
    }

    lightingContext->SetUseLighting(numLights > 0);
    lightingContext->SetLights(_glfSimpleLights);
    lightingContext->SetCamera(modelViewMatrix.Get<GfMatrix4d>(),
                               projectionMatrix.Get<GfMatrix4d>());

    // XXX: compatibility hack for passing some unit tests until we have
    //      more formal material plumbing.
    lightingContext->SetMaterial(_material);
    lightingContext->SetSceneAmbient(_sceneAmbient);

    // If there are shadows then we need to create and setup 
    // the shadow array needed in the lighting context in 
    // order to receive shadows
    // These calls will re-allocate internal buffers if they change.
    _shadows->SetSize(GfVec2i(maxShadowRes, maxShadowRes));
    _shadows->SetNumLayers(shadowIndex + 1);


    if (shadowIndex > -1) {
        for (size_t lightId = 0; lightId < numLights; ++lightId) {
            if (not _glfSimpleLights[lightId].HasShadow()) {
                continue;
            }

            VtValue vLightShadowParams =  _lights[lightId]->GetShadowParams();
            TF_VERIFY(vLightShadowParams.IsHolding<HdxShadowParams>());
            HdxShadowParams lightShadowParams = 
                vLightShadowParams.GetWithDefault<HdxShadowParams>(
                                                             HdxShadowParams());

            // Complete the shadow setup for this light
            int shadowId = _glfSimpleLights[lightId].GetShadowIndex();

            _shadows->SetViewMatrix(shadowId, GfMatrix4d(1));
            _shadows->SetProjectionMatrix(shadowId,
                _glfSimpleLights[lightId].GetShadowMatrix());
        }
    }
    lightingContext->SetShadows(_shadows);
}

HdLightSharedPtrVector
_ComputeIncludedLights(
    HdLightSharedPtrVector const & allLights,
    SdfPathVector const & includedPaths,
    SdfPathVector const & excludedPaths)
{
    HD_TRACE_FUNCTION();

    HdLightSharedPtrVector result;

    // In practice, the include and exclude containers will either
    // be empty or have just one element. So, the complexity of this method
    // should be dominated by the number of lights, which should also
    // be small. The TRACE_FUNCTION above will help catch when this is
    // no longer the case.
    TF_FOR_ALL(lightIt, allLights) {
        bool included = false;
        TF_FOR_ALL(includePathIt, includedPaths) {
            if ((*lightIt)->GetID().HasPrefix(*includePathIt)) {
                included = true;
                break;
            }
        }
        if (not included) {
            continue;
        }
        TF_FOR_ALL(excludePathIt, excludedPaths) {
            if ((*lightIt)->GetID().HasPrefix(*excludePathIt)) {
                included = false;
                break;
            }
        }
        if (included) {
            result.push_back(*lightIt);
        }
    }

    return result;
}

// -------------------------------------------------------------------------- //
// VtValue requirements
// -------------------------------------------------------------------------- //

std::ostream& operator<<(std::ostream& out, const HdxSimpleLightTaskParams& pv)
{
    out << pv.cameraPath << " " 
        << pv.enableShadows << " ";
    TF_FOR_ALL(it, pv.lightIncludePaths) {
        out << *it;
    }
    TF_FOR_ALL(it, pv.lightExcludePaths) {
        out << *it;
    }
    return out;
}

bool operator==(const HdxSimpleLightTaskParams& lhs, 
                const HdxSimpleLightTaskParams& rhs) 
{
    return lhs.cameraPath == rhs.cameraPath
        and lhs.lightIncludePaths == rhs.lightIncludePaths
        and lhs.lightExcludePaths == rhs.lightExcludePaths
        and lhs.material == rhs.material
        and lhs.sceneAmbient == rhs.sceneAmbient
        and lhs.enableShadows == rhs.enableShadows
       ;
}

bool operator!=(const HdxSimpleLightTaskParams& lhs, 
                const HdxSimpleLightTaskParams& rhs) 
{
    return not(lhs == rhs);
}

// -------------------------------------------------------------------------- //
// More vt requirements
// -------------------------------------------------------------------------- //

std::ostream&
operator<<(std::ostream& out, const HdxShadowParams& pv)
{
    out << pv.shadowMatrix << " "
        << pv.resolution << " "
        << pv.bias << " "
        << pv.blur << " "
        << pv.enabled;
    return out;
}

bool
operator==(const HdxShadowParams& lhs, const HdxShadowParams& rhs)
{
    return lhs.shadowMatrix == rhs.shadowMatrix
       and lhs.resolution == rhs.resolution
       and lhs.bias == rhs.bias
       and lhs.blur == rhs.blur
       and lhs.enabled == rhs.enabled;
}

bool
operator!=(const HdxShadowParams& lhs, const HdxShadowParams& rhs)
{
    return not (lhs == rhs);
}
