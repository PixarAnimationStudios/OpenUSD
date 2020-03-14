//
// Copyright 2019 Pixar
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
#include "renderPass.h"
#include "context.h"
#include "renderBuffer.h"

#include "hdPrman/camera.h"
#include "hdPrman/renderDelegate.h"
#include "hdPrman/rixStrings.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/envSetting.h"

#include "Riley.h"
#include "RixShadingUtils.h"
#include "RixPredefinedStrings.hpp"

PXR_NAMESPACE_OPEN_SCOPE

static bool _enableQuickIntegrate = TfGetenvBool(
    "HDX_PRMAN_ENABLE_QUICKINTEGRATE", false);

HdxPrman_RenderPass::HdxPrman_RenderPass(HdRenderIndex *index,
                                     HdRprimCollection const &collection,
                                     std::shared_ptr<HdPrman_Context> context)
    : HdRenderPass(index, collection)
    , _converged(false)
    , _context(context)
    , _lastRenderedVersion(0)
    , _lastSettingsVersion(0)
    , _integrator(HdPrmanIntegratorTokens->PxrPathTracer)
    , _quickIntegrator(HdPrmanIntegratorTokens->PxrDirectLighting)
    , _quickIntegrateTime(200.f/1000.f)
    , _quickIntegrate(false)
    , _isPrimaryIntegrator(false)
{
    // Check if this is an interactive context.
    _interactiveContext =
        std::dynamic_pointer_cast<HdxPrman_InteractiveContext>(context);

    _quickIntegrateTime = _enableQuickIntegrate ? 200.f/1000.f : 0.f;
}

HdxPrman_RenderPass::~HdxPrman_RenderPass()
{
}

bool
HdxPrman_RenderPass::IsConverged() const
{
    if (!_interactiveContext) {
        return true;
    }
    return _converged;
}

// Return the seconds between now and then.
static double
_DiffTimeToNow(std::chrono::steady_clock::time_point const& then)
{
    std::chrono::duration<double> diff;
    diff = std::chrono::duration_cast<std::chrono::duration<double>>
                                (std::chrono::steady_clock::now()-then);
    return(diff.count());
}

void
HdxPrman_RenderPass::_Execute(HdRenderPassStateSharedPtr const& renderPassState,
                            TfTokenVector const &renderTags)
{
    static const RtUString us_PxrPerspective("PxrPerspective");
    static const RtUString us_PxrOrthographic("PxrOrthographic");
    static const RtUString us_PathTracer("PathTracer");
    static const RtUString us_main_cam_projection("main_cam_projection");

    if (!_interactiveContext) {
        // If this is not an interactive context, don't use Hydra to drive
        // rendering and presentation of the framebuffer.  Instead, assume
        // we are just using Hydra to sync the scene contents to Riley.
        return;
    }
    if (_interactiveContext->renderThread.IsPauseRequested()) {
        // No more updates if pause is pending
        return;
    }

    riley::Riley *riley = _interactiveContext->riley;

    bool needStartRender = false;
    int currentSceneVersion = _interactiveContext->sceneVersion.load();
    if (currentSceneVersion != _lastRenderedVersion) {
        needStartRender = true;
        _lastRenderedVersion = currentSceneVersion;
    }

    // Enable/disable the fallback light when the scene provides no lights.
    _interactiveContext->SetFallbackLightsEnabled(
        _interactiveContext->sceneLightCount == 0);

    // Account for resolution edits
    GfVec4f vp = renderPassState->GetViewport();
    if (_interactiveContext->resolution[0] != vp[2] ||
        _interactiveContext->resolution[1] != vp[3]) {

        _interactiveContext->resolution[0] = vp[2];
        _interactiveContext->resolution[1] = vp[3];

        _interactiveContext->StopRender();

        _interactiveContext->_options.SetIntegerArray(
            RixStr.k_Ri_FormatResolution, _interactiveContext->resolution, 2);
        _interactiveContext->riley->SetOptions(_interactiveContext->_options);
        needStartRender = true;
    }

    // Check if any camera update needed
    // TODO: This should be part of a Camera sprim; then we wouldn't
    // need to sync anything here.  Note that we'll need to solve
    // thread coordination for sprim sync/finalize first.
    GfMatrix4d proj = renderPassState->GetProjectionMatrix();
    GfMatrix4d viewToWorldMatrix =
        renderPassState->GetWorldToViewMatrix().GetInverse();

    // XXX: Need to cast away constness to process updated camera params since
    // the Hydra camera doesn't update the Riley camera directly.
    HdPrmanCamera *hdCam =
        const_cast<HdPrmanCamera *>(
            dynamic_cast<HdPrmanCamera const *>(renderPassState->GetCamera()));
    bool camParamsChanged = hdCam? hdCam->GetAndResetHasParamsChanged() : false;
    if (proj != _lastProj ||
        viewToWorldMatrix != _lastViewToWorldMatrix ||
        camParamsChanged) {

        _lastProj = proj;
        _lastViewToWorldMatrix = viewToWorldMatrix;

        _interactiveContext->StopRender();

        // Coordinate system notes.
        //
        // # Hydra & USD are right-handed
        // - Camera space is always Y-up, looking along -Z.
        // - World space may be either Y-up or Z-up, based on stage metadata.
        // - Individual prims may be marked to be left-handed, which
        //   does not affect spatial coordinates, it only flips the
        //   winding order of polygons.
        //
        // # Prman is left-handed
        // - World is Y-up
        // - Camera looks along +Z.

        bool isPerspective = round(proj[3][3]) != 1 || proj == GfMatrix4d(1);
        riley::ShadingNode cameraNode = riley::ShadingNode {
            riley::ShadingNode::k_Projection,
            isPerspective ? us_PxrPerspective : us_PxrOrthographic,
            us_main_cam_projection,
            RtParamList()
        };

        // Set riley camera and projection shader params from the Hydra camera,
        // if available.
        RtParamList camParams;
        if (hdCam) {
            hdCam->SetRileyCameraParams(camParams, cameraNode.params);
        }

        // XXX Normally we would update RenderMan option 'ScreenWindow' to
        // account for an orthographic camera,
        //     options.SetFloatArray(RixStr.k_Ri_ScreenWindow, window, 4);
        // But we cannot update this option in Renderman once it is running.
        // We apply the orthographic-width to the viewMatrix scale instead.
        // Inverse computation of GfFrustum::ComputeProjectionMatrix()
        GfMatrix4d viewToWorldCorrectionMatrix(1.0);
        if (!isPerspective) {
            double left   = -(1 + proj[3][0]) / proj[0][0];
            double right  =  (1 - proj[3][0]) / proj[0][0];
            double bottom = -(1 - proj[3][1]) / proj[1][1];
            double top    =  (1 + proj[3][1]) / proj[1][1];
            double w = (right-left) / 2;
            double h = (top-bottom) / 2;
            GfMatrix4d scaleMatrix;
            scaleMatrix.SetScale(GfVec3d(w,h,1));
            viewToWorldCorrectionMatrix = scaleMatrix;
        } else {
            // Extract vertical FOV from hydra projection matrix after
            // accounting for the crop window.
            const float fov_rad = atan(1.0f / proj[1][1])*2;
            const float fov_deg = fov_rad / M_PI * 180.0;
            cameraNode.params.SetFloat(RixStr.k_fov, fov_deg);
        }

        // Riley camera xform is "move the camera", aka viewToWorld.
        // Convert right-handed Y-up camera space (USD, Hydra) to
        // left-handed Y-up (Prman) coordinates.  This just amounts to
        // flipping the Z axis.
        GfMatrix4d flipZ(1.0);
        flipZ[2][2] = -1.0;
        viewToWorldCorrectionMatrix = flipZ * viewToWorldCorrectionMatrix;

        riley::Transform xform;
        if (hdCam) {
            // Use time sampled transforms authored on the scene camera.
            HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES> const& 
                xforms = hdCam->GetTimeSampleXforms();

            TfSmallVector<RtMatrix4x4, HDPRMAN_MAX_TIME_SAMPLES> 
                xf_rt_values(xforms.count);
            
            for (size_t i=0; i < xforms.count; ++i) {
                xf_rt_values[i] = HdPrman_GfMatrixToRtMatrix(
                    viewToWorldCorrectionMatrix * xforms.values[i]);
            }

            xform = { unsigned(xforms.count), xf_rt_values.data(),
                      xforms.times.data() };
        } else {
            // Use the framing state as a single time sample.
            float const zerotime = 0.0f;
            RtMatrix4x4 matrix = HdPrman_GfMatrixToRtMatrix(
                viewToWorldCorrectionMatrix * viewToWorldMatrix);

            xform = {1, &matrix, &zerotime};
        }

        // Commit new camera.

        riley->ModifyCamera(
            _interactiveContext->cameraId, 
            &cameraNode,
            &xform, 
            &camParams);

        // Update the framebuffer Z scaling
        _interactiveContext->framebuffer.proj = proj;

        needStartRender = true;
    }

    // Likewise the render settings.
    HdRenderDelegate *renderDelegate = GetRenderIndex()->GetRenderDelegate();
    int currentSettingsVersion = renderDelegate->GetRenderSettingsVersion();
    if (_lastSettingsVersion != currentSettingsVersion) {
        _interactiveContext->StopRender();

        _integrator = renderDelegate->GetRenderSetting<std::string>(
            HdPrmanRenderSettingsTokens->integratorName,
            HdPrmanIntegratorTokens->PxrPathTracer.GetString());
        _isPrimaryIntegrator = _integrator ==
                HdPrmanIntegratorTokens->PxrPathTracer.GetString() ||
            _integrator ==
                HdPrmanIntegratorTokens->PbsPathTracer.GetString();
        if (_enableQuickIntegrate)
        {
            _quickIntegrator = renderDelegate->GetRenderSetting<std::string>(
                HdPrmanRenderSettingsTokens->interactiveIntegrator,
                HdPrmanIntegratorTokens->PxrDirectLighting.GetString());

            _quickIntegrateTime = renderDelegate->GetRenderSetting<int>(
                HdPrmanRenderSettingsTokens->interactiveIntegratorTimeout,
                200) / 1000.f;
        }
        else
        {
            _quickIntegrateTime = 0.0f;

            RtParamList integratorParams;
            _interactiveContext->SetIntegratorParamsFromRenderSettings(
                (HdPrmanRenderDelegate*)renderDelegate,
                _integrator,
                 integratorParams);             
            riley::ShadingNode integratorNode {
                riley::ShadingNode::k_Integrator,
                RtUString(_integrator.c_str()),
                RtUString(_integrator.c_str()),
                integratorParams
            };
           riley->ModifyIntegrator(_interactiveContext->integratorId,
                                   &integratorNode);
        }

        // Update convergence criteria.
        VtValue vtMaxSamples = renderDelegate->GetRenderSetting(
            HdRenderSettingsTokens->convergedSamplesPerPixel).Cast<int>();
        int maxSamples = TF_VERIFY(!vtMaxSamples.IsEmpty()) ?
            vtMaxSamples.UncheckedGet<int>() : 1024;
        _interactiveContext->_options.SetInteger(RixStr.k_hider_maxsamples,
                                                 maxSamples);

        VtValue vtPixelVariance = renderDelegate->GetRenderSetting(
            HdRenderSettingsTokens->convergedVariance).Cast<float>();
        float pixelVariance = TF_VERIFY(!vtPixelVariance.IsEmpty()) ?
            vtPixelVariance.UncheckedGet<float>() : 0.001f;
        _interactiveContext->_options.SetFloat(RixStr.k_Ri_PixelVariance,
                                               pixelVariance);

        // Set Options from RenderSettings schema
        _interactiveContext->SetOptionsFromRenderSettings(
            static_cast<HdPrmanRenderDelegate*>(renderDelegate),
             _interactiveContext->_options);
        
        _interactiveContext->riley->SetOptions(_interactiveContext->_options);
        _lastSettingsVersion = currentSettingsVersion;

        needStartRender = true;

        // Setup quick integrator and save ids of it and main
        if (_enableQuickIntegrate)
        {
            riley::ShadingNode integratorNode {
                riley::ShadingNode::k_Integrator,
                RtUString(_quickIntegrator.c_str()),
                us_PathTracer,
                RtParamList()
            };
            integratorNode.params.SetInteger(
                RtUString("numLightSamples"), 1);
            integratorNode.params.SetInteger(
                RtUString("numBxdfSamples"), 1);
            _quickIntegratorId = riley->CreateIntegrator(integratorNode);
        }
	_mainIntegratorId = _interactiveContext->integratorId;
    }

    // NOTE:
    //
    // _quickIntegrate enables hdxPrman to go into a mode
    // where it will switch to PxrDirectLighting
    // integrator for a couple of interations
    // and then switch back to PxrPathTracer/PbsPathTracer
    // The thinking is that we want to use PxrDirectLighting for quick
    // camera tumbles. To enable this mode, the 
    // HDX_PRMAN_ENABLE_QUICKINTEGRATE (bool) env var must be set.

    // If we're rendering but we're still in the quick integrate window,
    // check and see if we need to switch to the main integrator yet.
    if (_quickIntegrate &&
        (!needStartRender) &&
        _interactiveContext->renderThread.IsRendering() &&
        _DiffTimeToNow(_frameStart) > _quickIntegrateTime) {

        _interactiveContext->StopRender();
        _interactiveContext->SetIntegrator(_mainIntegratorId);
        _interactiveContext->StartRender();

        _quickIntegrate = false;
    }
    // Start (or restart) concurrent rendering.
    if (needStartRender) {
        if (_quickIntegrateTime > 0 && _isPrimaryIntegrator) {
            if (!_quickIntegrate) {
                // Start the frame with interactive integrator to give faster
                // time-to-first-buckets.
                _interactiveContext->SetIntegrator(_quickIntegratorId);
                _quickIntegrate = true;
            }
        } else if (_quickIntegrateTime <= 0 || _quickIntegrate) {
            // Disable quick integrate
            _interactiveContext->SetIntegrator(_mainIntegratorId);
            _quickIntegrate = false;
        }
        _interactiveContext->StartRender();
        _frameStart = std::chrono::steady_clock::now();
    }

    _converged = !_interactiveContext->renderThread.IsRendering();

    HdRenderPassAovBindingVector aovBindings =
        renderPassState->GetAovBindings();


    // Blit from the framebuffer to the currently selected AOVs.
    // Lock the framebuffer when reading so we don't overlap
    // with RenderMan's resize/writing.
    _interactiveContext->framebuffer.mutex.lock();
    for (size_t aov = 0; aov < aovBindings.size(); ++aov) {
        if(!TF_VERIFY(aovBindings[aov].renderBuffer)) {
            continue;
        }
        HdxPrmanRenderBuffer *rb = static_cast<HdxPrmanRenderBuffer*>(
            aovBindings[aov].renderBuffer);

        // Forward convergence state to the render buffers...
        rb->SetConverged(_converged);

        if (aovBindings[aov].aovName == HdAovTokens->color) {
            rb->Blit(HdFormatFloat32Vec4,
                _interactiveContext->framebuffer.w,
                _interactiveContext->framebuffer.h,
                reinterpret_cast<uint8_t*>(
                   _interactiveContext->framebuffer.color.data()));
        } else if (aovBindings[aov].aovName == HdAovTokens->depth) {
            rb->Blit(HdFormatFloat32,
                _interactiveContext->framebuffer.w,
                _interactiveContext->framebuffer.h,
                reinterpret_cast<uint8_t*>(
                    _interactiveContext->framebuffer.depth.data()));
        } else if (aovBindings[aov].aovName == HdAovTokens->primId) {
            rb->Blit(HdFormatInt32,
                _interactiveContext->framebuffer.w,
                _interactiveContext->framebuffer.h,
                reinterpret_cast<uint8_t*>(
                    _interactiveContext->framebuffer.primId.data()));
        } else if (aovBindings[aov].aovName == HdAovTokens->instanceId) {
            rb->Blit(HdFormatInt32,
                _interactiveContext->framebuffer.w,
                _interactiveContext->framebuffer.h,
                reinterpret_cast<uint8_t*>(
                    _interactiveContext->framebuffer.instanceId.data()));
        } else if (aovBindings[aov].aovName == HdAovTokens->elementId) {
            rb->Blit(HdFormatInt32,
                _interactiveContext->framebuffer.w,
                _interactiveContext->framebuffer.h,
                reinterpret_cast<uint8_t*>(
                    _interactiveContext->framebuffer.elementId.data()));
        }
    }
    _interactiveContext->framebuffer.mutex.unlock();
}

PXR_NAMESPACE_CLOSE_SCOPE
