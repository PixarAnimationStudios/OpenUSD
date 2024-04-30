//
// Copyright 2021 Pixar
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
#include "hdPrman/cameraContext.h"

#include "hdPrman/camera.h"
#include "hdPrman/rixStrings.h"
#include "hdPrman/utils.h"

#include "Riley.h"
#include "RixShadingUtils.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/tf/envSetting.h"

#include <cmath>

PXR_NAMESPACE_OPEN_SCOPE

static const RtUString s_projectionNodeName("cam_projection");

HdPrman_CameraContext::HdPrman_CameraContext()
  : _policy(CameraUtilFit)
  , _disableDepthOfField(false)
  , _invalid(false)
{
}

void
HdPrman_CameraContext::MarkCameraInvalid(const SdfPath &path)
{
    // No need to invalidate if camera that is not the active camera
    // changed.
    if (path == _cameraPath) {
        _invalid = true;
    }
}

void
HdPrman_CameraContext::SetCameraPath(const SdfPath &path)
{
    if (_cameraPath != path) {
        _invalid = true;
        _cameraPath = path;
    }
}

void
HdPrman_CameraContext::SetFraming(const CameraUtilFraming &framing)
{
    if (_framing != framing) {
        _framing = framing;
        _invalid = true;
    }
}

void
HdPrman_CameraContext::SetWindowPolicy(
    const CameraUtilConformWindowPolicy policy)
{
    if (_policy != policy) {
        _policy = policy;
        _invalid = true;
    }
}

void
HdPrman_CameraContext::SetDisableDepthOfField(bool disableDepthOfField)
{
    if (_disableDepthOfField != disableDepthOfField) {
        _disableDepthOfField = disableDepthOfField;
        _invalid = true;
    }
}

bool
HdPrman_CameraContext::IsInvalid() const
{
    return _invalid;
}

///////////////////////////////////////////////////////////////////////////////
//
// Screen window space: imagine a plane at in front of the camera (and parallel
// to the camera) with coordinates such that the square [-1,1]^2 spans a pyramid
// with angle being the (horizontal) FOV. This is the screen window space and is
// used to parametrize the rays from the camera.
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

    if (cam->GetFocalLength() == 0.0f || cam->GetHorizontalAperture() == 0.0f) {
        return filmbackPlane;
    }

    // Note that for perspective projection and with no horizontal aperture,
    // our screen widndow's x-coordinate are in [-1, 1].
    // Divide by appropriate factor to get to this.
    return filmbackPlane / double(0.5 * cam->GetHorizontalAperture());
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
    const GfVec2i &renderBufferSize)
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
            displayWindow.GetMin()[1] - renderBufferSize[1]));
        
    const GfVec2d screenWindowSize(
        screenWindowWidthPerPixel * renderBufferSize[0],
        screenWindowHeightPerPixel * renderBufferSize[1]);
    
    return GfRange2d(screenWindowMin, screenWindowMin + screenWindowSize);
}

static
double
_SafeDiv(const double a, const double b)
{
    if (b == 0) {
        TF_CODING_ERROR(
            "Invalid display window in render pass state for hdPrman");
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

// Convert a window into the format expected by RenderMan
// (xmin, xmax, ymin, ymax).
static
GfVec4f
_ToVec4f(const GfRange2d &window)
{
    return { float(window.GetMin()[0]), float(window.GetMax()[0]),
             float(window.GetMin()[1]), float(window.GetMax()[1]) };
}

// Get respective projection shader name for projection.
static
const RtUString&
_ComputeProjectionShader(const HdCamera::Projection projection)
{
    static const RtUString us_PxrCamera("PxrCamera");
    static const RtUString us_PxrOrthographic("PxrOrthographic");

    switch (projection) {
    case HdCamera::Perspective:
        return us_PxrCamera;
    case HdCamera::Orthographic:
        return us_PxrOrthographic;
    }

    // Make compiler happy.
    return us_PxrCamera;
}

// Compute parameters for the camera riley::ShadingNode for perspective camera
RtParamList
_ComputePerspectiveNodeParams(
    const HdPrmanCamera * const camera, bool disableDepthOfField)
{
    RtParamList result;

    static const RtUString us_lensType("lensType");
    // lensType values in PxrProjection.
    constexpr int lensTypeLensWarp = 2;

    // Pick a PxrProjection lens type that supports depth of field
    // and lens distortion.
    result.SetInteger(us_lensType, lensTypeLensWarp);

    // FOV settings.
    const float focalLength = camera->GetFocalLength();
    if (focalLength > 0) {
        result.SetFloat(RixStr.k_focalLength, focalLength);
        const float r = camera->GetHorizontalAperture() / focalLength;
        const float fov = 2.0f * GfRadiansToDegrees(std::atan(0.5f * r));
        result.SetFloat(RixStr.k_fov, fov);
    } else {
        // If focal length is bogus, don't set it.
        // Fallback to sane FOV.
        result.SetFloat(RixStr.k_fov, 90.0f);
    }

    // Depth of field settings.
    const float focusDistance = camera->GetFocusDistance();
    if (focusDistance > 0.0f) {
        result.SetFloat(RixStr.k_focalDistance, focusDistance);
    } else {
        // If value is bogus, set to sane value.
        result.SetFloat(RixStr.k_focalDistance, 1000.0f);
    }

    const float fStop = camera->GetFStop();
    if (disableDepthOfField || fStop <= 0.0f || focusDistance <= 0.0f) {
        // If depth of field is disabled or the values are bogus, 
        // disable depth of field by setting f-Stop to infinity, 
        // and a sane value for focalDistance. 
        result.SetFloat(RixStr.k_fStop, RI_INFINITY);
    } else {
        result.SetFloat(RixStr.k_fStop, fStop);
    }

    // Not setting fov frame begin/end - thus we do not support motion blur
    // due to changing FOV.

    // Some of these names might need to change when switching to PxrCamera.
    static const RtUString us_radial1("radial1");
    static const RtUString us_radial2("radial2");
    static const RtUString us_distortionCtr("distortionCtr");
    static const RtUString us_lensSqueeze("lensSqueeze");
    static const RtUString us_lensAsymmetryX("lensAsymmetryX");
    static const RtUString us_lensAsymmetryY("lensAsymmetryY");
    static const RtUString us_lensScale("lensScale");

    result.SetFloat(
        us_radial1,
        camera->GetLensDistortionK1());
    result.SetFloat(
        us_radial2,
        camera->GetLensDistortionK2());
    result.SetFloatArray(
        us_distortionCtr,
        camera->GetLensDistortionCenter().data(),
        2);
    result.SetFloat(
        us_lensSqueeze,
        camera->GetLensDistortionAnaSq());
    result.SetFloat(
        us_lensAsymmetryX,
        camera->GetLensDistortionAsym()[0]);
    result.SetFloat(
        us_lensAsymmetryY,
        camera->GetLensDistortionAsym()[1]);
    result.SetFloat(
        us_lensScale,
        camera->GetLensDistortionScale());

    return result;
}

// Compute parameters for the camera riley::ShadingNode for orthographic camera
RtParamList
_ComputeOrthographicNodeParams(const HdPrmanCamera * const camera)
{
    return {};
}

// Compute parameters for the camera riley::ShadingNode
static
RtParamList
_ComputeNodeParams(const HdPrmanCamera * const camera, bool disableDepthOfField)
{
    switch(camera->GetProjection()) {
    case HdCamera::Perspective:
        return _ComputePerspectiveNodeParams(camera, disableDepthOfField);
    case HdCamera::Orthographic:
        return _ComputeOrthographicNodeParams(camera);
    }

    // Make compiler happy
    return _ComputePerspectiveNodeParams(camera, disableDepthOfField);
}

// Compute params given to Riley::ModifyCamera
RtParamList
HdPrman_CameraContext::_ComputeCameraParams(
    const GfRange2d &screenWindow,
    const HdCamera * const camera) const
{
    RtParamList result;

    // Following parameters are currently set on the Riley camera:
    // 'nearClip' (float): near clipping distance
    // 'farClip' (float): near clipping distance
    // 'shutterOpenTime' (float): beginning of normalized shutter interval
    // 'shutterCloseTime' (float): end of normalized shutter interval

    // Parameters that are not handled (and use their defaults):
    // 'focusregion' (float):
    // 'dofaspect' (float): dof aspect ratio
    // 'apertureNSides' (int):
    // 'apertureAngle' (float):
    // 'apertureRoundness' (float):
    // 'apertureDensity' (float):

    // Parameter that is handled during Riley camera creation:
    // Rix::k_shutteropening (float[8] [c1 c2 d1 d2 e1 e2 f1 f2): additional
    // control points

    // Do not use clipping range if scene delegate did not provide one.
    // Note that we do a sanity check slightly stronger than
    // GfRange1f::IsEmpty() in that we do not allow the range to contain
    // only exactly one point.
    const GfRange1f &clippingRange = camera->GetClippingRange();
    if (clippingRange.GetMin() < clippingRange.GetMax()) {
        result.SetFloat(RixStr.k_nearClip, clippingRange.GetMin());
        result.SetFloat(RixStr.k_farClip, clippingRange.GetMax());
    }

    const HdPrmanCamera * const hdPrmanCamera =
        dynamic_cast<const HdPrmanCamera * const>(camera);
    const HdPrmanCamera::ShutterCurve &shutterCurve
        = hdPrmanCamera->GetShutterCurve();
        
    result.SetFloat(RixStr.k_shutterOpenTime, shutterCurve.shutterOpenTime);
    result.SetFloat(RixStr.k_shutterCloseTime, shutterCurve.shutterCloseTime);
    result.SetFloatArray(
        RixStr.k_shutteropening,
        shutterCurve.shutteropening.data(),
        shutterCurve.shutteropening.size());

    result.SetFloat(RixStr.k_apertureAngle,
                    hdPrmanCamera->GetApertureAngle());
    result.SetFloat(RixStr.k_apertureDensity,
                    hdPrmanCamera->GetApertureDensity());
    result.SetInteger(RixStr.k_apertureNSides,
                      hdPrmanCamera->GetApertureNSides());
    result.SetFloat(RixStr.k_apertureRoundness,
                    hdPrmanCamera->GetApertureRoundness());
                    

    const GfVec4f s = _ToVec4f(screenWindow);
    result.SetFloatArray(RixStr.k_Ri_ScreenWindow, s.data(), 4);

    return result;
}

// Convert Hydra time sampled matrices to renderman matrices.
// Optionally flip z-direction.
static
TfSmallVector<RtMatrix4x4, HDPRMAN_MAX_TIME_SAMPLES>
_ToRtMatrices(
    const HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES> &samples,
    const bool flipZ = false)
{
    using _RtMatrices = TfSmallVector<RtMatrix4x4, HDPRMAN_MAX_TIME_SAMPLES>;
    _RtMatrices matrices(samples.count);

    static const GfMatrix4d flipZMatrix(GfVec4d(1.0, 1.0, -1.0, 1.0));
    
    for (size_t i = 0; i < samples.count; ++i) {
        matrices[i] = HdPrman_Utils::GfMatrixToRtMatrix(
            flipZ
                ? flipZMatrix * samples.values[i]
                : samples.values[i]);
    }

    return matrices;
}

GfRange2d
HdPrman_CameraContext::_ComputeConformedScreenWindow(
    const HdCamera * const camera) const
{
    return
        CameraUtilConformedWindow(
            _GetScreenWindow(camera),
            _policy,
            _GetDisplayWindowAspect(_framing));
}

void
HdPrman_CameraContext::UpdateRileyCameraAndClipPlanes(
    riley::Riley * const riley,
    const HdRenderIndex * const renderIndex)
{
    const HdPrmanCamera * const camera =
        GetCamera(renderIndex);
    if (!camera) {
        // Bail if no camera.
        return;
    }

    const GfRange2d conformedScreenWindow =
        _ComputeConformedScreenWindow(camera);

    _UpdateRileyCamera(
        riley,
        conformedScreenWindow,
        camera);
    _UpdateClipPlanes(
        riley,
        camera);
}

void
HdPrman_CameraContext::UpdateRileyCameraAndClipPlanesInteractive(
    riley::Riley * const riley,
    const HdRenderIndex * const renderIndex,
    const GfVec2i &renderBufferSize)
{
    const HdPrmanCamera * const camera =
        GetCamera(renderIndex);
    if (!camera) {
        // Bail if no camera.
        return;
    }

    // The screen window we would need to use if we were targeting
    // the display window.
    const GfRange2d conformedScreenWindow =
        _ComputeConformedScreenWindow(camera);

    // But instead, we target the rect of pixels in the render
    // buffer baking the AOVs, so we need to convert the
    // screen window.
    _UpdateRileyCamera(
        riley,
        _ConvertScreenWindowForDisplayWindowToRenderBuffer(
            conformedScreenWindow,
            _framing.displayWindow,
            renderBufferSize),
        camera);
    _UpdateClipPlanes(
        riley,
        camera);
}

void
HdPrman_CameraContext::_UpdateRileyCamera(
    riley::Riley * const riley,
    const GfRange2d &screenWindow,
    const HdPrmanCamera * const camera)
{
    // The riley camera should have been created before we get here.
    if (!TF_VERIFY(_cameraId != riley::CameraId::InvalidId())) {
        return;
    }

    const riley::ShadingNode node = riley::ShadingNode {
        riley::ShadingNode::Type::k_Projection,
        _ComputeProjectionShader(camera->GetProjection()),
        s_projectionNodeName,
        _ComputeNodeParams(camera, _disableDepthOfField)
    };

    const RtParamList params = _ComputeCameraParams(screenWindow, camera);

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

    using _HdTimeSamples =
        HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES>;
    using _RtMatrices =
        TfSmallVector<RtMatrix4x4, HDPRMAN_MAX_TIME_SAMPLES>;    

    // Use time sampled transforms authored on the scene camera.
    const _HdTimeSamples &sampleXforms = camera->GetTimeSampleXforms();

    // Riley camera xform is "move the camera", aka viewToWorld.
    // Convert right-handed Y-up camera space (USD, Hydra) to
    // left-handed Y-up (Prman) coordinates.  This just amounts to
    // flipping the Z axis.
    const _RtMatrices rtMatrices =
        _ToRtMatrices(sampleXforms, /* flipZ = */ true);

    const riley::Transform transform{
        unsigned(sampleXforms.count),
        rtMatrices.data(),
        sampleXforms.times.data() };

    // Commit camera.
    riley->ModifyCamera(
        _cameraId, 
        &node,
        &transform,
        &params);
}

// Hydra expresses clipping planes as a plane equation
// in the camera object space.
// Riley API expresses clipping planes in terms of a
// time-sampled transform, a normal, and a point.
static
bool
_ToClipPlaneParams(const GfVec4d &plane, RtParamList * const params)
{
    static const RtUString us_planeNormal("planeNormal");
    static const RtUString us_planeOrigin("planeOrigin");
    
    const GfVec3f direction(plane[0], plane[1], plane[2]);
    const float directionLength = direction.GetLength();
    if (directionLength == 0.0f) {
        return false;
    }
    // Riley API expects a unit-length normal.
    const GfVec3f norm = direction / directionLength;
    params->SetNormal(us_planeNormal,
                      RtNormal3(norm[0], norm[1], norm[2]));
    // Determine the distance along the normal
    // to the plane.
    const float distance = -plane[3] / directionLength;
    // The origin can be any point on the plane.
    const RtPoint3 origin(norm[0] * distance,
                          norm[1] * distance,
                          norm[2] * distance);
    params->SetPoint(us_planeOrigin, origin);

    return true;
}

void
HdPrman_CameraContext::_UpdateClipPlanes(
    riley::Riley * const riley,
    const HdPrmanCamera * const camera)
{
    _DeleteClipPlanes(riley);

    // Create clipping planes
    const std::vector<GfVec4d> &clipPlanes = camera->GetClipPlanes();
    if (clipPlanes.empty()) {
        return;
    }

    using _HdTimeSamples =
        HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES>;
    using _RtMatrices =
        TfSmallVector<RtMatrix4x4, HDPRMAN_MAX_TIME_SAMPLES>;

    // Use time sampled transforms authored on the scene camera.
    const _HdTimeSamples &sampleXforms = camera->GetTimeSampleXforms();
    const _RtMatrices rtMatrices = _ToRtMatrices(sampleXforms);

    const riley::Transform transform {
        unsigned(sampleXforms.count),
        rtMatrices.data(),
        sampleXforms.times.data() };

    for (const GfVec4d &plane: clipPlanes) {
        RtParamList params;
        if (_ToClipPlaneParams(plane, &params)) {
            _clipPlaneIds.push_back(
                riley->CreateClippingPlane(transform, params));
        }
    }
}

void
HdPrman_CameraContext::_DeleteClipPlanes(
    riley::Riley * const riley)
{
    for (riley::ClippingPlaneId const& id: _clipPlaneIds) {
        riley->DeleteClippingPlane(id);
    }
    _clipPlaneIds.clear();
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
_DivRoundDown(const float a, const int b)
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

// Compute how the dataWindow sets in a window with upper left corner
// at camWindowMin and size camWindowSize.
static
GfVec4f
_ComputeCropWindow(
    const GfRect2i &dataWindow,
    const GfVec2f &camWindowMin,
    const GfVec2i &camWindowSize)
{
    return GfVec4f(
        _DivRoundDown(dataWindow.GetMinX() - camWindowMin[0]       ,
                      camWindowSize[0]),
        _DivRoundDown(dataWindow.GetMaxX() - camWindowMin[0] + 1.0f,
                      camWindowSize[0]),
        _DivRoundDown(dataWindow.GetMinY() - camWindowMin[1]       ,
                      camWindowSize[1]),
        _DivRoundDown(dataWindow.GetMaxY() - camWindowMin[1] + 1.0f,
                      camWindowSize[1]));
}

GfVec2i
HdPrman_CameraContext::GetResolutionFromDisplayWindow() const
{
    const GfVec2f size = _framing.displayWindow.GetSize();

    return GfVec2i(std::ceil(size[0]), std::ceil(size[1]));
}

void
HdPrman_CameraContext::SetRileyOptions(
    RtParamList * const options) const
{
    const GfVec2i res = GetResolutionFromDisplayWindow();

    // Compute how the data window sits in the display window.
    const GfVec4f cropWindow =
        _ComputeCropWindow(
            _framing.dataWindow,
            _framing.displayWindow.GetMin(),
            res);

    options->SetFloatArray(
        RixStr.k_Ri_CropWindow,
        cropWindow.data(), 4);

    options->SetIntegerArray(
        RixStr.k_Ri_FormatResolution,
        res.data(), 2);

    options->SetFloat(
        RixStr.k_Ri_FormatPixelAspectRatio,
        _framing.pixelAspectRatio);
}    

void
HdPrman_CameraContext::SetRileyOptionsInteractive(
    RtParamList * const options,
    const GfVec2i &renderBufferSize) const
{
    // Compute how the data window sits in the rect of the render
    // buffer baking the AOVs.

    const GfVec4f cropWindow =
        _ComputeCropWindow(
            _framing.dataWindow,
            GfVec2f(0.0f),
            renderBufferSize);

    options->SetFloatArray(
        RixStr.k_Ri_CropWindow,
        cropWindow.data(), 4);
    
    options->SetFloat(
        RixStr.k_Ri_FormatPixelAspectRatio,
        _framing.pixelAspectRatio);
}

void
HdPrman_CameraContext::MarkValid()
{
    _invalid = false;
}

void
HdPrman_CameraContext::CreateRileyCamera(
    riley::Riley * const riley,
    const RtUString &cameraName)
{
    _cameraName = cameraName;

    RtParamList nodeParams;
    nodeParams.SetFloat(RixStr.k_fov, 60.0f);

    // Projection
    const riley::ShadingNode node = riley::ShadingNode {
        riley::ShadingNode::Type::k_Projection,
        _ComputeProjectionShader(HdCamera::Perspective),
        s_projectionNodeName,
        nodeParams
    };

    // Camera params
    RtParamList params;

    // Transform
    float const zerotime[] = { 0.0f };
    RtMatrix4x4 matrix[] = {RixConstants::k_IdentityMatrix};
    matrix[0].Translate(0.f, 0.f, -10.0f);
    const riley::Transform transform = { 1, matrix, zerotime };
        
    _cameraId = riley->CreateCamera(
        riley::UserId(
            stats::AddDataLocation(_cameraName.CStr()).GetValue()),
        _cameraName,
        node,
        transform,
        params);

    // Dicing Camera
    // XXX This should be moved out if/when we support multiple camera contexts.
    riley->SetDefaultDicingCamera(_cameraId);
}

void
HdPrman_CameraContext::DeleteRileyCameraAndClipPlanes(
    riley::Riley * const riley)
{
    if (_cameraId != riley::CameraId::InvalidId()) {
        riley->DeleteCamera(_cameraId);
        _cameraId = riley::CameraId::InvalidId();
    }

    _DeleteClipPlanes(riley);
}

const HdPrmanCamera *
HdPrman_CameraContext::GetCamera(
    const HdRenderIndex * const renderIndex) const
{
    return
        static_cast<const HdPrmanCamera*>(
            renderIndex->GetSprim(
                HdPrimTypeTokens->camera,
                _cameraPath));
}

const CameraUtilFraming &
HdPrman_CameraContext::GetFraming() const
{
    return _framing;
}

/* static */
RtUString
HdPrman_CameraContext::GetDefaultReferenceCameraName()
{
    const static RtUString name("main_cam");
    return name;
}

PXR_NAMESPACE_CLOSE_SCOPE
