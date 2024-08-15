//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/simpleLightTask.h"

#include "pxr/imaging/hdx/shadowMatrixComputation.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/hdSt/light.h"
#include "pxr/imaging/hdSt/simpleLightingShader.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/primGather.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/glf/simpleLightingContext.h"

#include "pxr/base/gf/matrix4f.h"

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
  , _numLightIds(0)
  , _maxLights(16)
  , _sprimIndexVersion(0)
  , _settingsVersion(0)
  , _lightingShader(std::make_shared<HdStSimpleLightingShader>())
  , _enableShadows(false)
  , _viewport(0.0f, 0.0f, 0.0f, 0.0f)
  , _overrideWindowPolicy{false, CameraUtilFit}
  , _material()
  , _sceneAmbient()
  , _glfSimpleLights()
  , _lightingBar(nullptr)
  , _lightSourcesBar(nullptr)
  , _shadowsBar(nullptr)
  // Build all buffer sources the first time.
  , _rebuildLightingBufferSources(true)
  , _rebuildLightAndShadowBufferSources(true)
  , _rebuildMaterialBufferSources(true)
{
}

HdxSimpleLightTask::~HdxSimpleLightTask() = default;

std::vector<GfMatrix4d>
HdxSimpleLightTask::_ComputeShadowMatrices(
    const HdCamera * const camera,
    HdxShadowMatrixComputationSharedPtr const &computation) const
{
    if (!TF_VERIFY(computation)) {
        return std::vector<GfMatrix4d>();
    }

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
    HdChangeTracker &tracker = renderIndex.GetChangeTracker();
    HdRenderDelegate *renderDelegate = renderIndex.GetRenderDelegate();

    // Update params if needed.
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

        _rebuildMaterialBufferSources = true;
    }

    static const TfTokenVector lightTypes = 
        {HdPrimTypeTokens->domeLight,
         HdPrimTypeTokens->simpleLight,
         HdPrimTypeTokens->sphereLight,
         HdPrimTypeTokens->rectLight,
         HdPrimTypeTokens->diskLight,
         HdPrimTypeTokens->cylinderLight,
         HdPrimTypeTokens->distantLight};

    bool verifyNumLights = false;

    // Update _lightIds if the params or the set of render index sprims changed.
    if (((*dirtyBits) & HdChangeTracker::DirtyParams) ||
        (tracker.GetSprimIndexVersion() != _sprimIndexVersion)) {

        // Extract all light paths for each type of light
        _lightIds.clear();
        _numLightIds = _AppendLightsOfType(renderIndex, lightTypes,
                _lightIncludePaths,
                _lightExcludePaths,
                &_lightIds);

        _sprimIndexVersion = tracker.GetSprimIndexVersion();
        verifyNumLights = true;
    }

    // Update _maxLights if necessary.
    if (renderDelegate->GetRenderSettingsVersion() != _settingsVersion) {
        _maxLights = size_t(
            renderIndex.GetRenderDelegate()->GetRenderSetting<int>(
                HdStRenderSettingsTokens->maxLights, 16));
        _settingsVersion = renderDelegate->GetRenderSettingsVersion();
        verifyNumLights = true;
    }

    // Only emit the warning if the comparands changed.
    if (verifyNumLights && (_numLightIds > _maxLights)) {
        TF_WARN("Hydra Storm supports up to %zu lights, truncating the "
                "%zu found lights to this max.", _maxLights, _numLightIds);
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
    GlfSimpleShadowArrayRefPtr const& shadows = lightingContext->GetShadows();
    if (!TF_VERIFY(shadows)) {
        return;
    }

    // Place lighting context in task context
    (*ctx)[HdxTokens->lightingContext] = lightingContext;

    GfMatrix4d const  viewMatrix = camera->GetTransform().GetInverse();
    GfMatrix4d const& viewInverseMatrix = camera->GetTransform();
    GfMatrix4d const  projectionMatrix = camera->ComputeProjectionMatrix();
    // XXX: Extract the camera window policy to adjust the frustum correctly for
    // lights that have shadows.

    // Unique identifier for lights with shadows
    int shadowIndex = -1;

    // We rebuild the lights array every time, but avoid reallocating
    // the array every frame as this was showing up as a significant portion
    // of the time in this function.
    //
    // clear() is guaranteed by spec to not change the capacity of the
    // vector.

    size_t targetCapacity = std::min(_numLightIds, _maxLights);

    _glfSimpleLights.clear();
    if (targetCapacity != _glfSimpleLights.capacity()) {
        // We don't just want to reserve here as we want to try and
        // recover memory if the number of lights shrinks.

        _glfSimpleLights.shrink_to_fit();
        _glfSimpleLights.reserve(targetCapacity);
    }

    std::vector<GfVec2i> shadowMapResolutions;
    shadowMapResolutions.reserve(targetCapacity);

    // Loop over the lightTypes vector so we always add the built-in light  
    // types (dome and simple lights) first. This way if the scene has more  
    // lights than is supported, the built-in lights should still be included.
    for (TfToken const &lightType : lightTypes) {
        const auto lightPathsIt = _lightIds.find(lightType);
        if (lightPathsIt == _lightIds.end()) {
            continue;
        }
        for (SdfPath const &lightPath : lightPathsIt->second) {

            // Stop adding lights if we're at the light limit.
            if (_glfSimpleLights.size() >= _maxLights) {
                break;
            }

            HdStLight const *light = static_cast<const HdStLight *>(
                    renderIndex.GetSprim(lightType, lightPath));
            if (!TF_VERIFY(light)) {
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
                glfl.SetPosition(GfVec4f(lightPos * viewInverseMatrix));
                GfVec3f lightDir = glfl.GetSpotDirection();
                glfl.SetSpotDirection(
                    GfVec3f(viewInverseMatrix.TransformDir(lightDir)));

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
                const std::vector<GfMatrix4d> shadowMatrices =
                    _ComputeShadowMatrices(
                        camera, lightShadowParams.shadowMatrix);

                if (shadowMatrices.empty()) {
                    glfl.SetHasShadow(false);
                } else {
                    glfl.SetShadowIndexStart(shadowIndex + 1);
                    glfl.SetShadowIndexEnd(shadowIndex + shadowMatrices.size());
                    shadowIndex += shadowMatrices.size();

                    glfl.SetShadowMatrices(shadowMatrices);
                    glfl.SetShadowBias(lightShadowParams.bias);
                    glfl.SetShadowBlur(lightShadowParams.blur);
                    glfl.SetShadowResolution(lightShadowParams.resolution);
                }

                for (size_t i = 0; i < shadowMatrices.size(); ++i) {
                    shadowMapResolutions.push_back(
                        GfVec2i(lightShadowParams.resolution));
                }
            }
            _glfSimpleLights.push_back(std::move(glfl));
        }
        // Stop adding lights if we're at the light limit.
        if (_glfSimpleLights.size() >= _maxLights) {
            break;
        }
    }

    TF_VERIFY(_glfSimpleLights.size() <= _maxLights);

    const bool useLighting = !_glfSimpleLights.empty();
    if (useLighting != lightingContext->GetUseLighting()) {
        _rebuildLightingBufferSources = true;
    }

    if (_glfSimpleLights != lightingContext->GetLights()) {
        _rebuildLightAndShadowBufferSources = true;
    }

    lightingContext->SetUseLighting(useLighting);
    lightingContext->SetLights(_glfSimpleLights);
    lightingContext->SetCamera(viewMatrix, projectionMatrix);
    // XXX: compatibility hack for passing some unit tests until we have
    //      more formal material plumbing.
    lightingContext->SetMaterial(_material);
    lightingContext->SetSceneAmbient(_sceneAmbient);

    // If there are shadows then we need to create and setup 
    // the shadow array needed in the lighting context in 
    // order to receive shadows
    // This will re-allocate internal buffers if they change.
    if (lightingContext->GetUseShadows()) {
        shadows->SetShadowMapResolutions(shadowMapResolutions);

        if (shadowIndex > -1) {
            for (size_t lightId = 0; lightId < _glfSimpleLights.size();
                 ++lightId) {
                if (!_glfSimpleLights[lightId].HasShadow()) {
                    continue;
                }
                // Complete the shadow setup for this light
                const int shadowStart = 
                    _glfSimpleLights[lightId].GetShadowIndexStart();
                const int shadowEnd = 
                    _glfSimpleLights[lightId].GetShadowIndexEnd();
                const std::vector<GfMatrix4d> shadowMatrices =
                    _glfSimpleLights[lightId].GetShadowMatrices();

                for (int shadowId = shadowStart; shadowId <= shadowEnd; 
                     ++shadowId) {
                    shadows->SetViewMatrix(shadowId,
                        _glfSimpleLights[lightId].GetTransform().GetInverse());
                    shadows->SetProjectionMatrix(shadowId,
                        shadowMatrices[shadowId - shadowStart]);
                }
            }
        }
    } else {
        shadows->SetShadowMapResolutions( std::vector<GfVec2i>() );
    }

    _lightingShader->AllocateTextureHandles(renderIndex);

    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxSimpleLightTask::Prepare(HdTaskContext* ctx,
                            HdRenderIndex* renderIndex)
{
    HD_TRACE_FUNCTION();

    GlfSimpleLightingContextRefPtr const& lightingContext = 
        _lightingShader->GetLightingContext(); 
    if (!TF_VERIFY(lightingContext)) {
        return;
    }

    HdStResourceRegistrySharedPtr const& hdStResourceRegistry =
        std::dynamic_pointer_cast<HdStResourceRegistry>(
            renderIndex->GetResourceRegistry());

    if (!hdStResourceRegistry) {
        return;
    }

    // Allocate lighting BAR
    if (!_lightingBar)
    {
        HdBufferSpecVector bufferSpecs;

        bufferSpecs.emplace_back(
            HdxSimpleLightTaskTokens->useLighting,
            HdTupleType{HdTypeBool, 1});
        bufferSpecs.emplace_back(
            HdxSimpleLightTaskTokens->useColorMaterialDiffuse,
            HdTupleType{HdTypeBool, 1});

        _lightingBar = hdStResourceRegistry->AllocateUniformBufferArrayRange(
            HdxSimpleLightTaskTokens->lighting, 
            bufferSpecs, 
            HdBufferArrayUsageHintBitsUniform);

        _lightingShader->AddBufferBinding(
            HdStBindingRequest(HdStBinding::UBO, 
                               HdxSimpleLightTaskTokens->lightingContext,
                               _lightingBar, /*interleaved=*/true));
    }
  
    // Add lighting buffer sources
    if (_rebuildLightingBufferSources) {
        HdBufferSourceSharedPtrVector sources = {
            std::make_shared<HdVtBufferSource>(
                HdxSimpleLightTaskTokens->useLighting,
                VtValue(lightingContext->GetUseLighting())),
            std::make_shared<HdVtBufferSource>(
                HdxSimpleLightTaskTokens->useColorMaterialDiffuse,
                VtValue(lightingContext->GetUseColorMaterialDiffuse()))
        };
        
        hdStResourceRegistry->AddSources(_lightingBar, std::move(sources));
    }
    
    size_t const numLights = static_cast<size_t>(
        lightingContext->GetNumLightsUsed());
    size_t const numShadows = static_cast<size_t>(
        lightingContext->ComputeNumShadowsUsed());

    // Allocate light sources BAR
    if (!_lightSourcesBar) 
    {
        HdBufferSpecVector bufferSpecs;

        bufferSpecs.emplace_back(
            HdxSimpleLightTaskTokens->position,
            HdTupleType{HdTypeFloatVec4, 1});
        bufferSpecs.emplace_back(
            HdxSimpleLightTaskTokens->ambient,
            HdTupleType{HdTypeFloatVec4, 1});
        bufferSpecs.emplace_back(
            HdxSimpleLightTaskTokens->diffuse,
            HdTupleType{HdTypeFloatVec4, 1});
        bufferSpecs.emplace_back(
            HdxSimpleLightTaskTokens->specular,
            HdTupleType{HdTypeFloatVec4, 1});
        bufferSpecs.emplace_back(
            HdxSimpleLightTaskTokens->spotDirection,
            HdTupleType{HdTypeFloatVec3, 1});
        bufferSpecs.emplace_back(
            HdxSimpleLightTaskTokens->spotCutoff,
            HdTupleType{HdTypeFloat, 1});
        bufferSpecs.emplace_back(
            HdxSimpleLightTaskTokens->spotFalloff,
            HdTupleType{HdTypeFloat, 1});
        bufferSpecs.emplace_back(
            HdxSimpleLightTaskTokens->attenuation,
            HdTupleType{HdTypeFloatVec3, 1});
        bufferSpecs.emplace_back(
            HdxSimpleLightTaskTokens->worldToLightTransform,
            HdTupleType{HdTypeFloatMat4, 1});
        bufferSpecs.emplace_back(
            HdxSimpleLightTaskTokens->shadowIndexStart,
            HdTupleType{HdTypeInt32, 1});
        bufferSpecs.emplace_back(
            HdxSimpleLightTaskTokens->shadowIndexEnd,
            HdTupleType{HdTypeInt32, 1});
        bufferSpecs.emplace_back(
            HdxSimpleLightTaskTokens->hasShadow,
            HdTupleType{HdTypeBool, 1});
        bufferSpecs.emplace_back(
            HdxSimpleLightTaskTokens->isIndirectLight,
            HdTupleType{HdTypeBool, 1});

        _lightSourcesBar = hdStResourceRegistry->AllocateUniformBufferArrayRange(
            HdxSimpleLightTaskTokens->lighting,
            bufferSpecs,
            HdBufferArrayUsageHintBitsUniform);
    }

    _lightingShader->RemoveBufferBinding(HdxSimpleLightTaskTokens->lightSource);

    if (numLights != 0) {
        _lightingShader->AddBufferBinding(
            HdStBindingRequest(HdStBinding::UBO, 
                               HdxSimpleLightTaskTokens->lightSource,
                               _lightSourcesBar, /*interleaved=*/true, 
                               /*writable*/false, numLights,
                               /*concatenateNames*/true));
    }

    // Allocate shadows BAR if needed
    bool const useShadows = lightingContext->GetUseShadows();
    if (!_shadowsBar && useShadows) {
        HdBufferSpecVector bufferSpecs;

        bufferSpecs.emplace_back(
            HdxSimpleLightTaskTokens->worldToShadowMatrix,
            HdTupleType{HdTypeFloatMat4, 1});
        bufferSpecs.emplace_back(
            HdxSimpleLightTaskTokens->shadowToWorldMatrix,
            HdTupleType{HdTypeFloatMat4, 1});
        bufferSpecs.emplace_back(
            HdxSimpleLightTaskTokens->blur,
            HdTupleType{HdTypeFloat, 1});
        bufferSpecs.emplace_back(
            HdxSimpleLightTaskTokens->bias,
            HdTupleType{HdTypeFloat, 1});

        _shadowsBar = 
            hdStResourceRegistry->AllocateUniformBufferArrayRange(
                HdxSimpleLightTaskTokens->lighting, bufferSpecs, 
                HdBufferArrayUsageHintBitsUniform);
    }
    
    _lightingShader->RemoveBufferBinding(HdxSimpleLightTaskTokens->shadow);
    
    if (numShadows != 0) {
        _lightingShader->AddBufferBinding(
            HdStBindingRequest(HdStBinding::UBO, HdxSimpleLightTaskTokens->shadow,
                               _shadowsBar, /*interleaved=*/true, 
                               /*writable*/false, numShadows, 
                               /*concatenateNames*/true));
    }

    // Add light and shadow buffer sources
    if (_rebuildLightAndShadowBufferSources) {
        // Light sources
        VtVec4fArray position(numLights);
        VtVec4fArray ambient(numLights);
        VtVec4fArray diffuse(numLights);
        VtVec4fArray specular(numLights);
        VtVec3fArray spotDirection(numLights);
        VtFloatArray spotCutoff(numLights);
        VtFloatArray spotFalloff(numLights);
        VtVec3fArray attenuation(numLights);
        VtMatrix4fArray worldToLightTransform(numLights);
        VtIntArray shadowIndexStart(numLights);
        VtIntArray shadowIndexEnd(numLights);
        VtBoolArray hasShadow(numLights);
        VtBoolArray isIndirectLight(numLights);

        // Shadows
        VtMatrix4fArray worldToShadowMatrix(numShadows);
        VtMatrix4fArray shadowToWorldMatrix(numShadows);
        VtFloatArray blur(numShadows);
        VtFloatArray bias(numShadows);
        
        GlfSimpleLightVector const & lights = lightingContext->GetLights();
        GlfSimpleShadowArrayRefPtr const & shadows = 
            lightingContext->GetShadows();

        for (size_t i = 0; i < numLights; ++i) {
            position[i] = lights[i].GetPosition();
            ambient[i] = lights[i].GetAmbient();
            diffuse[i] = lights[i].GetDiffuse();
            specular[i] = lights[i].GetSpecular();
            spotDirection[i] = lights[i].GetSpotDirection();
            spotCutoff[i] = lights[i].GetSpotCutoff();
            spotFalloff[i] = lights[i].GetSpotFalloff();
            attenuation[i] = lights[i].GetAttenuation();
            worldToLightTransform[i] = 
                GfMatrix4f(lights[i].GetTransform().GetInverse());
            shadowIndexStart[i] = lights[i].GetShadowIndexStart();
            shadowIndexEnd[i] = lights[i].GetShadowIndexEnd();
            hasShadow[i] = lights[i].HasShadow();
            isIndirectLight[i] = lights[i].IsDomeLight();
            // Shadows
            if (hasShadow[i]) {
                for (int j = shadowIndexStart[i]; j <= shadowIndexEnd[i]; ++j) {
                    worldToShadowMatrix[j] = GfMatrix4f(
                        shadows->GetWorldToShadowMatrix(j));
                    shadowToWorldMatrix[j] = GfMatrix4f(
                        worldToShadowMatrix[j].GetInverse());
                    blur[j] = lights[i].GetShadowBlur();
                    bias[j] = lights[i].GetShadowBias();
                }
            }
        }
        
        HdBufferSourceSharedPtrVector sources = {
            std::make_shared<HdVtBufferSource>(
                HdxSimpleLightTaskTokens->position,
                VtValue(position)),
            std::make_shared<HdVtBufferSource>(
                HdxSimpleLightTaskTokens->ambient,
                VtValue(ambient)),
            std::make_shared<HdVtBufferSource>(
                HdxSimpleLightTaskTokens->diffuse,
                VtValue(diffuse)),
            std::make_shared<HdVtBufferSource>(
                HdxSimpleLightTaskTokens->specular,
                VtValue(specular)),
            std::make_shared<HdVtBufferSource>(
                HdxSimpleLightTaskTokens->spotDirection,
                VtValue(spotDirection)),
            std::make_shared<HdVtBufferSource>(
                HdxSimpleLightTaskTokens->spotCutoff,
                VtValue(spotCutoff)),
            std::make_shared<HdVtBufferSource>(
                HdxSimpleLightTaskTokens->spotFalloff,
                VtValue(spotFalloff)),
            std::make_shared<HdVtBufferSource>(
                HdxSimpleLightTaskTokens->attenuation,
                VtValue(attenuation)),
            std::make_shared<HdVtBufferSource>(
                HdxSimpleLightTaskTokens->worldToLightTransform,
                VtValue(worldToLightTransform)),
            std::make_shared<HdVtBufferSource>(
                HdxSimpleLightTaskTokens->shadowIndexStart,
                VtValue(shadowIndexStart)),
            std::make_shared<HdVtBufferSource>(
                HdxSimpleLightTaskTokens->shadowIndexEnd,
                VtValue(shadowIndexEnd)),
            std::make_shared<HdVtBufferSource>(
                HdxSimpleLightTaskTokens->hasShadow,
                VtValue(hasShadow)),
            std::make_shared<HdVtBufferSource>(
                HdxSimpleLightTaskTokens->isIndirectLight,
                VtValue(isIndirectLight)),
        };

        hdStResourceRegistry->AddSources(_lightSourcesBar, 
            std::move(sources));

        if (useShadows) {
            HdBufferSourceSharedPtrVector shadowSources = {
                std::make_shared<HdVtBufferSource>(
                    HdxSimpleLightTaskTokens->worldToShadowMatrix,
                    VtValue(worldToShadowMatrix)),
                std::make_shared<HdVtBufferSource>(
                    HdxSimpleLightTaskTokens->shadowToWorldMatrix,
                    VtValue(shadowToWorldMatrix)),
                std::make_shared<HdVtBufferSource>(
                    HdxSimpleLightTaskTokens->blur,
                    VtValue(blur)),
                std::make_shared<HdVtBufferSource>(
                    HdxSimpleLightTaskTokens->bias,
                    VtValue(bias)),
            };
            
            hdStResourceRegistry->AddSources(_shadowsBar, 
                std::move(shadowSources));
        }
    }

    // Allocate material BAR
    if (!_materialBar) 
    {
        HdBufferSpecVector bufferSpecs;

        bufferSpecs.emplace_back(
            HdxSimpleLightTaskTokens->ambient,
            HdTupleType{HdTypeFloatVec4, 1});
        bufferSpecs.emplace_back(
            HdxSimpleLightTaskTokens->diffuse,
            HdTupleType{HdTypeFloatVec4, 1});
        bufferSpecs.emplace_back(
            HdxSimpleLightTaskTokens->specular,
            HdTupleType{HdTypeFloatVec4, 1});
        bufferSpecs.emplace_back(
            HdxSimpleLightTaskTokens->emission,
            HdTupleType{HdTypeFloatVec4, 1});
        bufferSpecs.emplace_back(
            HdxSimpleLightTaskTokens->sceneColor,
            HdTupleType{HdTypeFloatVec4, 1});
        bufferSpecs.emplace_back(
            HdxSimpleLightTaskTokens->shininess,
            HdTupleType{HdTypeFloat, 1});

        // Allocate interleaved buffer
        _materialBar = hdStResourceRegistry->AllocateUniformBufferArrayRange(
            HdxSimpleLightTaskTokens->lighting, 
            bufferSpecs, 
            HdBufferArrayUsageHintBitsUniform);

        // Add buffer binding request
        _lightingShader->AddBufferBinding(
            HdStBindingRequest(HdStBinding::UBO, TfToken("material"),
                               _materialBar, /*interleaved=*/true, 
                               /*writable*/false, /*arraySize*/0, 
                               /*concatenateNames*/true));
    }
    
    // Add material buffer sources
    if (_rebuildMaterialBufferSources) {
        GlfSimpleMaterial const & material = lightingContext->GetMaterial();

        HdBufferSourceSharedPtrVector sources = {
            std::make_shared<HdVtBufferSource>(
                HdxSimpleLightTaskTokens->ambient,
                VtValue(material.GetAmbient())),
            std::make_shared<HdVtBufferSource>(
                HdxSimpleLightTaskTokens->diffuse,
                VtValue(material.GetDiffuse())),
            std::make_shared<HdVtBufferSource>(
                HdxSimpleLightTaskTokens->specular,
                VtValue(material.GetSpecular())),
            std::make_shared<HdVtBufferSource>(
                HdxSimpleLightTaskTokens->emission,
                VtValue(material.GetEmission())),
            std::make_shared<HdVtBufferSource>(
                HdxSimpleLightTaskTokens->sceneColor,
                VtValue(lightingContext->GetSceneAmbient())),
            std::make_shared<HdVtBufferSource>(
                HdxSimpleLightTaskTokens->shininess,
                VtValue(static_cast<float>(material.GetShininess())))
        };

        hdStResourceRegistry->AddSources(_materialBar, std::move(sources));
    }

    _rebuildLightingBufferSources = false;
    _rebuildLightAndShadowBufferSources = false;
    _rebuildMaterialBufferSources = false;
}

void
HdxSimpleLightTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
}

size_t
HdxSimpleLightTask::_AppendLightsOfType(HdRenderIndex &renderIndex,
                   TfTokenVector const &lightTypes,
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
            const size_t numLocalLights = lightsLocal.size();
            if (numLocalLights > 0) {
                (*lights)[*it] = lightsLocal;
                count += numLocalLights;
            }
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

