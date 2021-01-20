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

// The crop window for RenderMan.
//
// Computed from data window and render buffer size.
//
// Recall from the RenderMan API:
// Only the pixels within the crop window are rendered. Has no
// affect on how pixels in the image map into the filmback plane.
// The crop window is relative to the render buffer size, e.g.,
// the crop window of (0,0,1,1) corresponds to the entire render
// buffer. The coordinates of the crop window are y-down.
// Format is (xmin, xmax, ymin, ymax).
//
// The limits for the integer locations corresponding to the above crop
// window are:
//
//   rxmin = clamp(ceil( renderbufferwidth*xmin    ), 0, renderbufferwidth - 1)
//   rxmax = clamp(ceil( renderbufferwidth*xmax - 1), 0, renderbufferwidth - 1)
//   similar for y
//
static
float
_DivRoundDown(const int a, const int b)
{
    // Note that if the division (performed here)
    //    float(a) / b
    // rounds up, then the result (by RenderMan) of
    //    ceil(b * (float(a) / b))
    // might be a+1 instead of a.
    //
    // We add a slight negative bias to a to avoid this (we could also
    // set the floating point rounding mode but: how to do this in a
    // portable way - and on x86 switching the rounding is slow).

    return GfClamp((a - 0.0078125f) / b, 0.0f, 1.0f);
}

static
GfVec4f
_GetCropWindow(
    HdRenderPassStateSharedPtr const& renderPassState,
    const int32_t width, const int32_t height)
{
    const CameraUtilFraming &framing = renderPassState->GetFraming();
    if (!framing.IsValid()) {
        return GfVec4f(0,1,0,1);
    }

    const GfRect2i &w = framing.dataWindow;
    return GfVec4f(
        _DivRoundDown(w.GetMinX(), width),
        _DivRoundDown(w.GetMaxX() + 1, width),
        _DivRoundDown(w.GetMinY(), height),
        _DivRoundDown(w.GetMaxY() + 1, height));
}

///////////////////////////////////////////////////////////////////////////////
//
// Screen window space: imagine a plane at unit distance (*) in front
// of the camera (and parallel to the camera). Coordinates with
// respect to screen window space are measured in this plane with the
// y-Axis pointing up. Such coordinates parameterize rays from the
// camera.
// (*) This is a simplification achieved by fixing RenderMan's FOV to be
// 90 degrees.
//
// Image space: coordinates of the pixels in the rendered image with the top
// left pixel having coordinate (0,0), i.e., y-down.
// The display window from the camera framing is in image space as well
// as the width and height of the render buffer.
//
// We want to map the screen window space to the image space such that the
// conformed camera frustum from the scene delegate maps to the display window
// of the CameraUtilFraming. This is achieved by the following code.
//
//
// Compute screen window for given camera.
//
static
GfRange2d
_GetScreenWindow(const HdCamera * const cam)
{
    const GfVec2d size(
        cam->GetHorizontalAperture(),       cam->GetVerticalAperture());
    const GfVec2d offset(
        cam->GetHorizontalApertureOffset(), cam->GetVerticalApertureOffset());
        
    const GfRange2d filmbackPlane(-0.5 * size + offset, +0.5 * size + offset);

    if (cam->GetProjection() == HdCamera::Orthographic) {
        return filmbackPlane;
    }

    if (cam->GetFocalLength() == 0.0f) {
        return filmbackPlane;
    }

    return filmbackPlane / double(cam->GetFocalLength());
}

static
double
_SafeDiv(const double a, const double b)
{
    if (b == 0) {
        TF_CODING_ERROR(
            "Invalid display window in render pass state for hdxPrman");
        return 1.0;
    }
    return a / b;
}

// Compute the aspect ratio of the display window taking the
// pixel aspect ratio into account.
static
double
_GetDisplayWindowAspect(const CameraUtilFraming &framing)
{
    const GfVec2f &size = framing.displayWindow.GetSize();
    return framing.pixelAspectRatio * _SafeDiv(size[0], size[1]);
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

// Compute the screen window we need to give to RenderMan. This screen
// window is mapped to the entire render buffer (in image space) by
// RenderMan.
//
// The input is the screenWindowForDisplayWindow: the screen window
// corresponding to the camera from the scene delegate conformed to match
// the aspect ratio of the display window.
//
// Together with the displayWindow, this input establishes how screen
// window space is mapped to image space. We know need to take the
// render buffer rect in image space and convert it to screen window
// space.
// 
static
GfRange2d
_ConvertScreenWindowForDisplayWindowToRenderBuffer(
    const GfRange2d &screenWindowForDisplayWindow,
    const GfRange2f &displayWindow,
    const int32_t renderBufferWidth, const int32_t renderBufferHeight)
{
    // Scaling factors to go from image space to screen window space.
    const double screenWindowWidthPerPixel =
        screenWindowForDisplayWindow.GetSize()[0] /
        displayWindow.GetSize()[0];
        
    const double screenWindowHeightPerPixel =
        screenWindowForDisplayWindow.GetSize()[1] /
        displayWindow.GetSize()[1];

    // Assuming an affine mapping between screen window space
    // and image space, compute what (0,0) corresponds to in
    // screen window space.
    const GfVec2d screenWindowMin(
        screenWindowForDisplayWindow.GetMin()[0]
        - screenWindowWidthPerPixel * displayWindow.GetMin()[0],
        // Note that image space is y-Down and screen window
        // space is y-Up, so this is a bit tricky...
        screenWindowForDisplayWindow.GetMax()[1]
        + screenWindowHeightPerPixel * (
            displayWindow.GetMin()[1] - renderBufferHeight));
        
    const GfVec2d screenWindowSize(
        screenWindowWidthPerPixel * renderBufferWidth,
        screenWindowHeightPerPixel * renderBufferHeight);
    
    return GfRange2d(screenWindowMin, screenWindowMin + screenWindowSize);
}

// Convert a window into the format expected by RenderMan
// (xmin, xmax, ymin, ymax).
static
GfVec4f
_ToVec4f(const GfRange2d &window)
{
    return { float(window.GetMin()[0]), float(window.GetMax()[0]),
             float(window.GetMin()[1]), float(window.GetMax()[1]) };
}

// Compute the screen window we need to give to RenderMan.
// 
// See above comments. This also conforms the camera frustum using
// the window policy specified by the application or the HdCamera.
//
static
GfVec4f
_ComputeScreenWindow(
    HdRenderPassStateSharedPtr const& renderPassState,
    const int32_t renderBufferWidth, const int32_t renderBufferHeight)
{
    const CameraUtilFraming &framing = renderPassState->GetFraming();

    // Screen window from camera.
    const GfRange2d screenWindowForCamera =
        _GetScreenWindow(renderPassState->GetCamera());

    // Conform to match display window's aspect ratio.
    const GfRange2d screenWindowForDisplayWindow =
        CameraUtilConformedWindow(
            screenWindowForCamera,
            renderPassState->GetWindowPolicy(),
            _GetDisplayWindowAspect(framing));
    
    // Compute screen window we need to send to RenderMan.
    const GfRange2d screenWindowForRenderBuffer =
        _ConvertScreenWindowForDisplayWindowToRenderBuffer(
            screenWindowForDisplayWindow,
            framing.displayWindow,
            renderBufferWidth, renderBufferHeight);
    
    return _ToVec4f(screenWindowForRenderBuffer);
}

void
HdxPrman_RenderPass::_Execute(HdRenderPassStateSharedPtr const& renderPassState,
                              TfTokenVector const &/*renderTags*/)
{
    HD_TRACE_FUNCTION();
    
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
    const int currentSceneVersion = _interactiveContext->sceneVersion.load();
    if (currentSceneVersion != _lastRenderedVersion) {
        needStartRender = true;
        _lastRenderedVersion = currentSceneVersion;
    }

    // Creates displays if needed
    HdRenderPassAovBindingVector const &aovBindings =
        renderPassState->GetAovBindings();
    if(_interactiveContext->CreateDisplays(aovBindings))
    {
        needStartRender = true;
    }

    // Enable/disable the fallback light when the scene provides no lights.
    _interactiveContext->SetFallbackLightsEnabled(
        _interactiveContext->sceneLightCount == 0);

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

    // XXX: Need to cast away constness to process updated camera params since
    // the Hydra camera doesn't update the Riley camera directly.
    HdPrmanCamera * const hdCam =
        const_cast<HdPrmanCamera *>(
            dynamic_cast<HdPrmanCamera const *>(renderPassState->GetCamera()));
    const bool camParamsChanged =
        hdCam && hdCam->GetAndResetHasParamsChanged();

    // Check if any camera update needed
    // TODO: This should be part of a Camera sprim; then we wouldn't
    // need to sync anything here.  Note that we'll need to solve
    // thread coordination for sprim sync/finalize first.
    const bool resolutionChanged =
        _interactiveContext->resolution[0] != renderBufferWidth ||
        _interactiveContext->resolution[1] != renderBufferHeight;

    const GfMatrix4d proj =
        renderPassState->GetProjectionMatrix();
    const GfMatrix4d viewToWorldMatrix =
        renderPassState->GetWorldToViewMatrix().GetInverse();
    const CameraUtilFraming &framing =
        renderPassState->GetFraming();

    if ( camParamsChanged ||
         resolutionChanged ||
         proj != _lastProj ||
         viewToWorldMatrix != _lastViewToWorldMatrix ||
         framing != _lastFraming) {

        _lastProj = proj;
        _lastViewToWorldMatrix = viewToWorldMatrix;
        _lastFraming = framing;

        _interactiveContext->StopRender();

        const GfVec4f cropWindow =
            _GetCropWindow(
                renderPassState, renderBufferWidth, renderBufferHeight);
        const bool cropWindowChanged = cropWindow != _lastCropWindow;

        if (resolutionChanged || cropWindowChanged) {
            if (resolutionChanged) {
                _interactiveContext->resolution[0] = renderBufferWidth;
                _interactiveContext->resolution[1] = renderBufferHeight;
                _interactiveContext->_options.SetIntegerArray(
                    RixStr.k_Ri_FormatResolution,
                    _interactiveContext->resolution, 2);
            }
            if (cropWindowChanged) {
                _lastCropWindow = cropWindow;
                _interactiveContext->_options.SetFloatArray(
                    RixStr.k_Ri_CropWindow,
                    cropWindow.data(), 4);
            }
            _interactiveContext->riley->SetOptions(
                _interactiveContext->_options);
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

        const bool isPerspective =
            round(proj[3][3]) != 1 || proj == GfMatrix4d(1);
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

        if (hdCam && renderPassState->GetFraming().IsValid()) {
            const GfVec4f screenWindow =
                _ComputeScreenWindow(
                    renderPassState,
                    renderBufferWidth, renderBufferHeight);

            if (hdCam->GetProjection() == HdCamera::Perspective) {
                // TODO: For lens distortion to be correct, we might
                // need to set a different FOV and adjust the screenwindow
                // accordingly.
                // For now, lens distortion parameters are not passed through
                // hdPrman anyway.
                //
                cameraNode.params.SetFloat(
                    RixStr.k_fov, 90.0f);
            }
            camParams.SetFloatArray(
                RixStr.k_Ri_ScreenWindow, screenWindow.data(), 4);
        } else {
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

        needStartRender = true;
    }

    // Likewise the render settings.
    HdRenderDelegate *renderDelegate = GetRenderIndex()->GetRenderDelegate();
    int currentSettingsVersion = renderDelegate->GetRenderSettingsVersion();
    if (_lastSettingsVersion != currentSettingsVersion || camParamsChanged) {
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
            _interactiveContext->SetIntegratorParamsFromCamera(
                (HdPrmanRenderDelegate*)renderDelegate, hdCam,
                _integrator, integratorParams);

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

    // Blit from the framebuffer to the currently selected AOVs.
    // Lock the framebuffer when reading so we don't overlap
    // with RenderMan's resize/writing.
    _interactiveContext->framebuffer.mutex.lock();

    for(size_t aov = 0; aov < aovBindings.size(); ++aov) {
        if(!TF_VERIFY(aovBindings[aov].renderBuffer)) {
            continue;
        }
        HdxPrmanRenderBuffer *rb = static_cast<HdxPrmanRenderBuffer*>(
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
