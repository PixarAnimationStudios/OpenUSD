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
#include "hdPrman/interactiveRenderPass.h"
#include "hdPrman/camera.h"
#include "hdPrman/interactiveContext.h"
#include "hdPrman/renderBuffer.h"
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
    "HD_PRMAN_ENABLE_QUICKINTEGRATE", false);

HdPrman_InteractiveRenderPass::HdPrman_InteractiveRenderPass(
    HdRenderIndex *index,
    HdRprimCollection const &collection,
    std::shared_ptr<HdPrman_Context> context)
: HdRenderPass(index, collection)
, _converged(false)
, _lastRenderedVersion(0)
, _lastSettingsVersion(0)
, _integrator(HdPrmanIntegratorTokens->PxrPathTracer)
, _quickIntegrator(HdPrmanIntegratorTokens->PxrDirectLighting)
, _quickIntegrateTime(200.f/1000.f)
, _quickIntegrate(false)
, _isPrimaryIntegrator(false)
{
    _interactiveContext =
        std::dynamic_pointer_cast<HdPrman_InteractiveContext>(context);

    TF_VERIFY(_interactiveContext);

    _quickIntegrateTime = _enableQuickIntegrate ? 200.f/1000.f : 0.f;
}

HdPrman_InteractiveRenderPass::~HdPrman_InteractiveRenderPass() = default;

bool
HdPrman_InteractiveRenderPass::IsConverged() const
{
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

static
const HdRenderBuffer *
_GetRenderBuffer(const HdRenderPassAovBinding& aov,
                 const HdRenderIndex * const renderIndex)
{
    if (aov.renderBuffer) {
        return aov.renderBuffer;
    }
    
    return
        dynamic_cast<HdRenderBuffer*>(
            renderIndex->GetBprim(
                HdPrimTypeTokens->renderBuffer,
                aov.renderBufferId));
}

static
bool
_GetRenderBufferSize(const HdRenderPassAovBindingVector &aovBindings,
                     const HdRenderIndex * const renderIndex,
                     int32_t * const width,
                     int32_t * const height)
{
    for (const HdRenderPassAovBinding &aovBinding : aovBindings) {
        if (const HdRenderBuffer * const renderBuffer =
                        _GetRenderBuffer(aovBinding, renderIndex)) {
            *width  = renderBuffer->GetWidth();
            *height = renderBuffer->GetHeight();
            return true;
        } else {
            TF_CODING_ERROR("No render buffer available for AOV "
                            "%s", aovBinding.aovName.GetText());
        }
    }

    return false;
}

void
HdPrman_InteractiveRenderPass::_Execute(
    HdRenderPassStateSharedPtr const& renderPassState,
    TfTokenVector const & renderTags)
{
    HD_TRACE_FUNCTION();
    
    static const RtUString us_PxrPerspective("PxrPerspective");
    static const RtUString us_PxrOrthographic("PxrOrthographic");
    static const RtUString us_PathTracer("PathTracer");
    static const RtUString us_main_cam_projection("main_cam_projection");
    static const RtUString us_planeNormal("planeNormal");
    static const RtUString us_planeOrigin("planeOrigin");

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

    // Creates displays if needed
    HdRenderPassAovBindingVector const &aovBindings =
        renderPassState->GetAovBindings();

    _interactiveContext->CreateDisplays(aovBindings);

    // Enable/disable the fallback light when the scene provides no lights.
    _interactiveContext->SetFallbackLightsEnabled(
        _interactiveContext->sceneLightCount == 0);

    // Likewise the render settings.
    HdPrmanRenderDelegate * const renderDelegate =
        static_cast<HdPrmanRenderDelegate*>(
            GetRenderIndex()->GetRenderDelegate());
    const int currentSettingsVersion =
        renderDelegate->GetRenderSettingsVersion();
    
    // XXX: Need to cast away constness to process updated camera params since
    // the Hydra camera doesn't update the Riley camera directly.
    HdPrmanCamera * const hdCam =
        const_cast<HdPrmanCamera *>(
            dynamic_cast<HdPrmanCamera const *>(renderPassState->GetCamera()));
    const CameraUtilFraming &framing = renderPassState->GetFraming();

    HdPrmanCameraContext &cameraContext =
        _interactiveContext->GetCameraContext();
    cameraContext.SetCamera(hdCam);
    cameraContext.SetFraming(framing);
    cameraContext.SetWindowPolicy(renderPassState->GetWindowPolicy());

    const bool camChanged = cameraContext.IsInvalid();
    cameraContext.MarkValid();

    if (_lastSettingsVersion != currentSettingsVersion || camChanged) {

        // AcquireRiley will stop rendering and increase sceneVersion
        // so that the render will be re-started below.
        riley::Riley * const riley = _interactiveContext->AcquireRiley();

        _integrator = renderDelegate->GetRenderSetting<std::string>(
            HdPrmanRenderSettingsTokens->integratorName,
            HdPrmanIntegratorTokens->PxrPathTracer.GetString());
        _isPrimaryIntegrator =
            _integrator ==
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

            riley::ShadingNode &integratorNode =
                _interactiveContext->GetActiveIntegratorShadingNode();

            _interactiveContext->SetIntegratorParamsFromRenderSettings(
                renderDelegate,
                _integrator,
                integratorNode.params);

            _interactiveContext->SetIntegratorParamsFromCamera(
                    renderDelegate, hdCam, _integrator, integratorNode.params);

            RtUString integrator(_integrator.c_str());
            integratorNode.handle = integratorNode.name = integrator;

            riley->ModifyIntegrator(
                _interactiveContext->GetActiveIntegratorId(), &integratorNode);
        }

        // Update convergence criteria.
        VtValue vtMaxSamples = renderDelegate->GetRenderSetting(
            HdRenderSettingsTokens->convergedSamplesPerPixel).Cast<int>();
        int maxSamples = TF_VERIFY(!vtMaxSamples.IsEmpty()) ?
            vtMaxSamples.UncheckedGet<int>() : 64; // RenderMan default
        _interactiveContext->GetOptions().SetInteger(RixStr.k_hider_maxsamples,
                                                     maxSamples);

        VtValue vtPixelVariance = renderDelegate->GetRenderSetting(
            HdRenderSettingsTokens->convergedVariance).Cast<float>();
        float pixelVariance = TF_VERIFY(!vtPixelVariance.IsEmpty()) ?
            vtPixelVariance.UncheckedGet<float>() : 0.001f;
        _interactiveContext->GetOptions().SetFloat(RixStr.k_Ri_PixelVariance,
                                                   pixelVariance);

        // Set Options from RenderSettings schema
        _interactiveContext->SetOptionsFromRenderSettings(
            renderDelegate,
            _interactiveContext->GetOptions());
        
        riley->SetOptions(
            _interactiveContext->_GetDeprecatedOptionsPrunedList());

        _lastSettingsVersion = currentSettingsVersion;

        // Setup quick integrator and save ids of it and main
        if (_enableQuickIntegrate)
        {
            riley::ShadingNode integratorNode {
                riley::ShadingNode::Type::k_Integrator,
                RtUString(_quickIntegrator.c_str()),
                us_PathTracer,
                RtParamList()
            };
            integratorNode.params.SetInteger(
                RtUString("numLightSamples"), 1);
            integratorNode.params.SetInteger(
                RtUString("numBxdfSamples"), 1);
            _quickIntegratorId =
                riley->CreateIntegrator(riley::UserId::DefaultId(),
                    integratorNode);
        }
        _mainIntegratorId = _interactiveContext->integratorId;
    }

    int32_t renderBufferWidth = 0;
    int32_t renderBufferHeight = 0;

    if (!_GetRenderBufferSize(aovBindings,
                              GetRenderIndex(),
                              &renderBufferWidth, &renderBufferHeight)) {
        // For legacy clients not using AOVs, take size of viewport.
        const GfVec4f vp = renderPassState->GetViewport();
        renderBufferWidth  = vp[2];
        renderBufferHeight = vp[3];
    }

    // Check if any camera update needed
    // TODO: This should be part of a Camera sprim; then we wouldn't
    // need to sync anything here.  Note that we'll need to solve
    // thread coordination for sprim sync/finalize first.
    const bool resolutionChanged =
        _interactiveContext->resolution[0] != renderBufferWidth ||
        _interactiveContext->resolution[1] != renderBufferHeight;

    const GfMatrix4d viewToWorldMatrix =
        renderPassState->GetWorldToViewMatrix().GetInverse();

    if (camChanged ||
        resolutionChanged) {

        // AcquireRiley will stop rendering and increase sceneVersion
        // so that the render will be re-started below.
        riley::Riley * const riley = _interactiveContext->AcquireRiley();

        if (resolutionChanged) {
            _interactiveContext->resolution[0] = renderBufferWidth;
            _interactiveContext->resolution[1] = renderBufferHeight;
            
            _interactiveContext->GetOptions().SetIntegerArray(
                RixStr.k_Ri_FormatResolution,
                _interactiveContext->resolution, 2);
            
            // There is currently only one render target per context
            if (_interactiveContext->renderViews.size() == 1) {
                riley::RenderViewId const renderViewId =
                    _interactiveContext->renderViews[0];
                
                auto it =
                    _interactiveContext->renderTargets.find(renderViewId);
                
                if (it != _interactiveContext->renderTargets.end()) {
                    riley::RenderTargetId const rtid = it->second;
                    const riley::Extent targetExtent = {
                        static_cast<uint32_t>(
                            _interactiveContext->resolution[0]),
                        static_cast<uint32_t>(
                            _interactiveContext->resolution[1]),
                        0};
                    riley->ModifyRenderTarget(
                        rtid, nullptr,
                        &targetExtent, nullptr, nullptr, nullptr);
                }
            }

            cameraContext.SetRileyOptions(
                &(_interactiveContext->GetOptions()),
                GfVec2i(renderBufferWidth, renderBufferHeight));
            
            riley->SetOptions(
                _interactiveContext->_GetDeprecatedOptionsPrunedList());
        }

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

        const GfMatrix4d proj =
            renderPassState->GetProjectionMatrix();

        const bool isPerspective =
            round(proj[3][3]) != 1 || proj == GfMatrix4d(1);
        riley::ShadingNode cameraNode = riley::ShadingNode {
            riley::ShadingNode::Type::k_Projection,
            isPerspective ? us_PxrPerspective : us_PxrOrthographic,
            us_main_cam_projection,
            RtParamList()
        };

        // Set riley camera and projection shader params from the Hydra camera,
        // if available.
        RtParamList camParams;
        cameraContext.SetCameraAndCameraNodeParams(
            &camParams,
            &cameraNode.params,
            GfVec2i(renderBufferWidth, renderBufferHeight));

        // XXX Normally we would update RenderMan option 'ScreenWindow' to
        // account for an orthographic camera,
        //     options.SetFloatArray(RixStr.k_Ri_ScreenWindow, window, 4);
        // But we cannot update this option in Renderman once it is running.
        // We apply the orthographic-width to the viewMatrix scale instead.
        // Inverse computation of GfFrustum::ComputeProjectionMatrix()
        GfMatrix4d viewToWorldCorrectionMatrix(1.0);

        if (! (hdCam && framing.IsValid())) {
            // Note that the above cameraContext.SetCameraAndCameraNodeParams
            // is not working when there is no valid camera and framing.

            // Implementing behaviors for old clients here.

            if (!isPerspective) {
                const double left   = -(1 + proj[3][0]) / proj[0][0];
                const double right  =  (1 - proj[3][0]) / proj[0][0];
                const double bottom = -(1 - proj[3][1]) / proj[1][1];
                const double top    =  (1 + proj[3][1]) / proj[1][1];
                const double w = (right-left) / 2;
                const double h = (top-bottom) / 2;
                viewToWorldCorrectionMatrix = GfMatrix4d(GfVec4d(w,h,1,1));
            } else {
                // Extract FOV from hydra projection matrix. More precisely,
                // use the smaller value among the horizontal and vertical FOV.
                //
                // This seems to match the resolution API which uses the smaller
                // value among width and height to match to the FOV.
                // 
                const float fov_rad =
                    atan(1.0f / std::max(proj[0][0], proj[1][1])) * 2;
                const float fov_deg =
                    fov_rad / M_PI * 180.0;
                cameraNode.params.SetFloat(RixStr.k_fov, fov_deg);
            }
        }

        // Riley camera xform is "move the camera", aka viewToWorld.
        // Convert right-handed Y-up camera space (USD, Hydra) to
        // left-handed Y-up (Prman) coordinates.  This just amounts to
        // flipping the Z axis.
        GfMatrix4d flipZ(1.0);
        flipZ[2][2] = -1.0;
        viewToWorldCorrectionMatrix = flipZ * viewToWorldCorrectionMatrix;

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

            riley::Transform xform = { unsigned(xforms.count),
                                       xf_rt_values.data(),
                                       xforms.times.data() };

            // Commit camera.
            riley->ModifyCamera(
                _interactiveContext->cameraId, 
                &cameraNode,
                &xform, 
                &camParams);


            // Clipping planes
            for (riley::ClippingPlaneId const& id: _clipPlanes) {
                riley->DeleteClippingPlane(id);
            }
            _clipPlanes.clear();
            HdRenderPassState::ClipPlanesVector hdClipPlanes = 
                renderPassState->GetClipPlanes();
            if (!hdClipPlanes.empty()) {
                // Convert camera's object xform.
                TfSmallVector<RtMatrix4x4, HDPRMAN_MAX_TIME_SAMPLES> 
                    xf_values(xforms.count);
                for (size_t i=0; i < xforms.count; ++i) {
                    xf_values[i] =
                        HdPrman_GfMatrixToRtMatrix(
                        xforms.values[i]);
                }
                riley::Transform camXform = { unsigned(xforms.count),
                                           xf_values.data(),
                                           xforms.times.data() };
                // Hydra expresses clipping planes as a plane equation
                // in the camera object space.
                // Riley API expresses clipping planes in terms of a
                // time-sampled transform, a normal, and a point.
                for (GfVec4d plane: hdClipPlanes) {
                    RtParamList params;
                    GfVec3f direction(plane[0], plane[1], plane[2]);
                    float directionLength = direction.GetLength();
                    if (directionLength == 0.0f) {
                        continue;
                    }
                    // Riley API expects a unit-length normal.
                    GfVec3f norm = direction / directionLength;
                    params.SetNormal(us_planeNormal,
                        RtNormal3(norm[0], norm[1], norm[2]));
                    // Determine the distance along the normal
                    // to the plane.
                    float distance = -plane[3] / directionLength;
                    // The origin can be any point on the plane.
                    RtPoint3 origin(norm[0] * distance,
                                    norm[1] * distance,
                                    norm[2] * distance);
                    params.SetPoint(us_planeOrigin, origin);
                    _clipPlanes.push_back(
                        riley->CreateClippingPlane(
                            camXform, params));
                }
            }
        } else {
            // Use the framing state as a single time sample.
            float const zerotime = 0.0f;
            RtMatrix4x4 matrix = HdPrman_GfMatrixToRtMatrix(
                viewToWorldCorrectionMatrix * viewToWorldMatrix);

            riley::Transform xform = {1, &matrix, &zerotime};

            // Commit camera.
            riley->ModifyCamera(
                _interactiveContext->cameraId, 
                &cameraNode,
                &xform, 
                &camParams);
        }

        // Update the framebuffer Z scaling
        _interactiveContext->framebuffer.proj = proj;
    }    
    
    // We need to capture the value of sceneVersion here after all
    // the above calls to AcquireRiley since AcquireRiley increases
    // the sceneVersion.
    const int currentSceneVersion =
        _interactiveContext->sceneVersion.load();
    const bool needsStartRender = 
        currentSceneVersion != _lastRenderedVersion;

    if (needsStartRender) {
        _lastRenderedVersion = currentSceneVersion;

        // NOTE:
        //
        // _quickIntegrate enables hdPrman to go into a mode
        // where it will switch to PxrDirectLighting
        // integrator for a couple of interations
        // and then switch back to PxrPathTracer/PbsPathTracer
        // The thinking is that we want to use PxrDirectLighting for quick
        // camera tumbles. To enable this mode, the 
        // HD_PRMAN_ENABLE_QUICKINTEGRATE (bool) env var must be set.
        
        // If we're rendering but we're still in the quick integrate window,
        // check and see if we need to switch to the main integrator yet.

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
    } else {
        if (_quickIntegrate &&
            _interactiveContext->renderThread.IsRendering() &&
            _DiffTimeToNow(_frameStart) > _quickIntegrateTime) {

            _interactiveContext->StopRender();
            _interactiveContext->SetIntegrator(_mainIntegratorId);
            _interactiveContext->StartRender();
            
            _quickIntegrate = false;
        }
    }

    _converged = !_interactiveContext->renderThread.IsRendering();

    // Blit from the framebuffer to the currently selected AOVs.
    // Lock the framebuffer when reading so we don't overlap
    // with RenderMan's resize/writing.
    _interactiveContext->framebuffer.mutex.lock();

    for(size_t aov = 0; aov < aovBindings.size(); ++aov) {
        if(!TF_VERIFY(aovBindings[aov].renderBuffer)) {
            continue;
        }
        HdPrmanRenderBuffer *rb = static_cast<HdPrmanRenderBuffer*>(
            aovBindings[aov].renderBuffer);

        // Forward convergence state to the render buffers...
        rb->SetConverged(_converged);
        rb->Blit(_interactiveContext->framebuffer.aovs[aov].format,
                 _interactiveContext->framebuffer.w,
                 _interactiveContext->framebuffer.h,
                 reinterpret_cast<uint8_t*>(
                     _interactiveContext->
                     framebuffer.aovs[aov].pixels.data()));
    }
    _interactiveContext->framebuffer.mutex.unlock();
}

PXR_NAMESPACE_CLOSE_SCOPE
