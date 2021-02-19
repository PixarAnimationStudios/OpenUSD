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

#include "pxr/imaging/hdx/simpleLightTask.h"

#include "pxr/imaging/hdx/shadowMatrixComputation.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/hdSt/light.h"
#include "pxr/imaging/hdSt/simpleLightingShader.h"

#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/primGather.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/glf/simpleLightingContext.h"

PXR_NAMESPACE_OPEN_SCOPE


// -------------------------------------------------------------------------- //

HdxSimpleLightTask::HdxSimpleLightTask(
    HdSceneDelegate* delegate, 
    SdfPath const& id)
  : HdTask(id) 
  , _cameraId()
  , _lightIds()
  , _lightIncludePaths()
  , _lightExcludePaths()
  , _numLights(0)
  , _lightingShader(std::make_shared<HdStSimpleLightingShader>())
  , _enableShadows(false)
  , _viewport(0.0f, 0.0f, 0.0f, 0.0f)
  , _overrideWindowPolicy{false, CameraUtilFit}
  , _material()
  , _sceneAmbient()
  , _glfSimpleLights()
{
}

HdxSimpleLightTask::~HdxSimpleLightTask() = default;

std::vector<GfMatrix4d>
HdxSimpleLightTask::_ComputeShadowMatrices(
    const HdCamera * const camera,
    HdxShadowMatrixComputationSharedPtr const &computation) const
{
    const CameraUtilConformWindowPolicy camPolicy = camera->GetWindowPolicy();

    if (_framing.IsValid()) {
        CameraUtilConformWindowPolicy const policy =
            _overrideWindowPolicy.first
                ? _overrideWindowPolicy.second
                : camPolicy;
        return computation->Compute(_framing, policy);
    } else {
        return computation->Compute(_viewport, camPolicy);
    }
}

void
HdxSimpleLightTask::Sync(HdSceneDelegate* delegate,
                         HdTaskContext* ctx,
                         HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();

    // Store the lighting context in the task context 
    // so later on other tasks can use this information 
    // draw shadows or other purposes
    (*ctx)[HdxTokens->lightingShader] =
        std::dynamic_pointer_cast<HdStLightingShader>(_lightingShader);


    HdRenderIndex &renderIndex = delegate->GetRenderIndex();

    if ((*dirtyBits) & HdChangeTracker::DirtyParams) {
        HdxSimpleLightTaskParams params;
        if (!_GetTaskParams(delegate, &params)) {
            return;
        }

        _lightIncludePaths = params.lightIncludePaths;
        _lightExcludePaths = params.lightExcludePaths;
        _cameraId = params.cameraPath;
        _enableShadows = params.enableShadows;
        _viewport = params.viewport;
        _framing = params.framing;
        _overrideWindowPolicy = params.overrideWindowPolicy;
        // XXX: compatibility hack for passing some unit tests until we have
        //      more formal material plumbing.
        _material = params.material;
        _sceneAmbient = params.sceneAmbient;
    }

    const HdCamera *camera = static_cast<const HdCamera *>(
        renderIndex.GetSprim(HdPrimTypeTokens->camera, _cameraId));
    if (!TF_VERIFY(camera)) {
        return;
    }

    // The lighting shader owns the lighting context, which in turn owns the
    // shadow array.
    GlfSimpleLightingContextRefPtr const& lightingContext = 
                                    _lightingShader->GetLightingContext(); 
    if (!TF_VERIFY(lightingContext)) {
        return;
    }
    GlfSimpleShadowArrayRefPtr const& shadows =lightingContext->GetShadows();
    if (!TF_VERIFY(shadows)) {
        return;
    }
    bool const useBindlessShadowMaps =
        GlfSimpleShadowArray::GetBindlessShadowMapsEnabled();

    // Place lighting context in task context
    (*ctx)[HdxTokens->lightingContext] = lightingContext;

    GfMatrix4d const& viewMatrix = camera->GetViewMatrix();
    GfMatrix4d const& viewInverseMatrix = camera->GetViewInverseMatrix();
    GfMatrix4d const& projectionMatrix = camera->GetProjectionMatrix();
    // Extract the camera window policy to adjust the frustum correctly for
    // lights that have shadows.

    // Unique identifier for lights with shadows
    int shadowIndex = -1;

    // Extract all light paths for each type of light
    static const TfTokenVector lightTypes = 
        {HdPrimTypeTokens->domeLight,
            HdPrimTypeTokens->simpleLight,
            HdPrimTypeTokens->sphereLight,
            HdPrimTypeTokens->rectLight};
    _lightIds.clear();
    _numLights = _AppendLightsOfType(renderIndex, lightTypes,
                        _lightIncludePaths,
                        _lightExcludePaths,
                        &_lightIds);

    // We rebuild the lights array every time, but avoid reallocating
    // the array every frame as this was showing up as a significant portion
    // of the time in this function.
    //
    // clear() is guaranteed by spec to not change the capacity of the
    // vector.
    _glfSimpleLights.clear();
    if (_numLights != _glfSimpleLights.capacity()) {
        // We don't just want to reserve here as we want to try and
        // recover memory if the number of lights shrinks.

        _glfSimpleLights.shrink_to_fit();
        _glfSimpleLights.reserve(_numLights);
    }

    std::vector<GfVec2i> shadowMapResolutions;
    shadowMapResolutions.reserve(_numLights);

    TF_FOR_ALL (lightPerTypeIt, _lightIds) {
        TF_FOR_ALL (lightPathIt, lightPerTypeIt->second) {

            HdStLight const *light = static_cast<const HdStLight *>(
                    renderIndex.GetSprim(lightPerTypeIt->first, *lightPathIt));
            if (!TF_VERIFY(light)) {
                _glfSimpleLights.push_back(GlfSimpleLight());
                continue;
            }

            // Take a copy of the simple light into our temporary array and
            // update it with viewer-dependant values.
            const VtValue vtLightParams = light->Get(HdLightTokens->params);
            GlfSimpleLight glfl = 
                vtLightParams.GetWithDefault<GlfSimpleLight>(GlfSimpleLight());

            // Skip lights with zero intensity
            if (!glfl.HasIntensity()) {
                continue;
            }

            // XXX: Pass id of light to Glf simple light, so that
            // glim can get access back to the light prim.
            glfl.SetID(light->GetId());

            // If the light is in camera space we need to transform
            // the position and spot direction to world space for
            // HdStSimpleLightingShader.
            if (glfl.IsCameraSpaceLight()) {
                GfVec4f lightPos = glfl.GetPosition();
                glfl.SetPosition(lightPos * viewInverseMatrix);
                GfVec3f lightDir = glfl.GetSpotDirection();
                glfl.SetSpotDirection(viewInverseMatrix.TransformDir(lightDir));

                // Since the light position has been transformed to world space,
                // record that it's no longer a camera-space light for any
                // downstream consumers of the lighting context.
                glfl.SetIsCameraSpaceLight(false);
            }

            const VtValue vLightShadowParams = 
                light->Get(HdLightTokens->shadowParams);
            const HdxShadowParams lightShadowParams = 
                vLightShadowParams.GetWithDefault<HdxShadowParams>
                    (HdxShadowParams());

            // If shadows are disabled from the rendergraph then
            // we treat this light as if it had the shadow disabled
            // doing so we guarantee that shadowIndex will be -1
            // which will not create memory for the shadow maps 
            if (!_enableShadows || !lightShadowParams.enabled) {
                glfl.SetHasShadow(false);
            }

            // Setup the rest of the light parameters necessary 
            // to calculate shadows.
            if (glfl.HasShadow()) {
                if (!TF_VERIFY(lightShadowParams.shadowMatrix)) {
                    glfl.SetHasShadow(false);
                    continue;
                }

                const std::vector<GfMatrix4d> shadowMatrices =
                    _ComputeShadowMatrices(
                        camera, lightShadowParams.shadowMatrix);

                if (shadowMatrices.empty()) {
                    glfl.SetHasShadow(false);
                    continue;
                }

                glfl.SetShadowIndexStart(shadowIndex + 1);
                glfl.SetShadowIndexEnd(shadowIndex + shadowMatrices.size());
                shadowIndex += shadowMatrices.size();

                glfl.SetShadowMatrices(shadowMatrices);
                glfl.SetShadowBias(lightShadowParams.bias);
                glfl.SetShadowBlur(lightShadowParams.blur);
                glfl.SetShadowResolution(lightShadowParams.resolution);

                for (size_t i = 0; i < shadowMatrices.size(); ++i) {
                    shadowMapResolutions.push_back(
                        GfVec2i(lightShadowParams.resolution));
                }
            }
            _glfSimpleLights.push_back(std::move(glfl));
        }
    }

    lightingContext->SetUseLighting(_numLights > 0);
    lightingContext->SetLights(_glfSimpleLights);
    lightingContext->SetCamera(viewMatrix, projectionMatrix);
    // XXX: compatibility hack for passing some unit tests until we have
    //      more formal material plumbing.
    lightingContext->SetMaterial(_material);
    lightingContext->SetSceneAmbient(_sceneAmbient);

    // If there are shadows then we need to create and setup 
    // the shadow array needed in the lighting context in 
    // order to receive shadows
    // These calls will re-allocate internal buffers if they change.

    if (useBindlessShadowMaps) {
        shadows->SetShadowMapResolutions(shadowMapResolutions);
    } else {
        // Bindful shadow maps use a texture array, and hence are limited to
        // a single resolution. Use the maximum authored resolution.
        int maxRes = 0;
        for (GfVec2i const& res : shadowMapResolutions) {
            maxRes = std::max(maxRes, res[0]);
        }
        shadows->SetSize(GfVec2i(maxRes, maxRes));
        shadows->SetNumLayers(shadowIndex + 1);
    }

    if (shadowIndex > -1) {
        for (size_t lightId = 0; lightId < _numLights; ++lightId) {
            if (!_glfSimpleLights[lightId].HasShadow()) {
                continue;
            }
            // Complete the shadow setup for this light
            int shadowStart = _glfSimpleLights[lightId].GetShadowIndexStart();
            int shadowEnd = _glfSimpleLights[lightId].GetShadowIndexEnd();
            std::vector<GfMatrix4d> shadowMatrices =
                _glfSimpleLights[lightId].GetShadowMatrices();

            for (int shadowId = shadowStart; shadowId <= shadowEnd; ++shadowId) {
                shadows->SetViewMatrix(shadowId,
                    _glfSimpleLights[lightId].GetTransform());
                shadows->SetProjectionMatrix(shadowId,
                    shadowMatrices[shadowId - shadowStart]);
            }
        }
    }

    _lightingShader->AllocateTextureHandles(delegate);

    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxSimpleLightTask::Prepare(HdTaskContext* ctx,
                            HdRenderIndex* renderIndex)
{
}

void
HdxSimpleLightTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
}

size_t
HdxSimpleLightTask::_AppendLightsOfType(HdRenderIndex &renderIndex,
                   std::vector<TfToken> const &lightTypes,
                   SdfPathVector const &lightIncludePaths,
                   SdfPathVector const &lightExcludePaths,
                   std::map<TfToken, SdfPathVector> *lights)
{
    size_t count = 0;
    TF_FOR_ALL(it, lightTypes) {
        if (renderIndex.IsSprimTypeSupported(*it)) {
            // XXX: This is inefficient, need to be optimized
            SdfPathVector sprimPaths = renderIndex.GetSprimSubtree(*it,
                SdfPath::AbsoluteRootPath());

            SdfPathVector lightsLocal;
            HdPrimGather gather;
            gather.Filter(sprimPaths, lightIncludePaths, lightExcludePaths,
                          &lightsLocal);
            (*lights)[*it] = lightsLocal;
            count += lightsLocal.size();
        }
    }
    return count;
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
        && lhs.lightIncludePaths == rhs.lightIncludePaths
        && lhs.lightExcludePaths == rhs.lightExcludePaths
        && lhs.material == rhs.material
        && lhs.sceneAmbient == rhs.sceneAmbient
        && lhs.enableShadows == rhs.enableShadows
       ;
}

bool operator!=(const HdxSimpleLightTaskParams& lhs, 
                const HdxSimpleLightTaskParams& rhs) 
{
    return !(lhs == rhs);
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
       && lhs.resolution == rhs.resolution
       && lhs.bias == rhs.bias
       && lhs.blur == rhs.blur
       && lhs.enabled == rhs.enabled;
}

bool
operator!=(const HdxShadowParams& lhs, const HdxShadowParams& rhs)
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE

