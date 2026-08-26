//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/camera.h"

#include "hdPrman/cameraContext.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/rixStrings.h"
#include "hdPrman/utils.h"

#include "pxr/imaging/cameraUtil/conformWindow.h"
#include "pxr/imaging/cameraUtil/framing.h"
#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/timeSampleArray.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hf/perfLog.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdr/declare.h"
#include "pxr/usd/sdr/registry.h"
#if PXR_VERSION <= 2411
#include "pxr/usd/sdr/shaderProperty.h"
#endif

#include "pxr/base/gf/math.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/range1f.h"
#include "pxr/base/gf/range2d.h"
#include "pxr/base/gf/range2f.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/smallVector.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/dictionary.h"

#include "pxr/pxr.h"

#include <ri.h>
#include <Riley.h>
#include <RileyIds.h>
#include <RiTypesHelper.h>
#include <RixShadingUtils.h>
#include <stats/Roz.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

#if HD_API_VERSION < 52
TF_DEFINE_PRIVATE_TOKENS(
    _lensDistortionTokens,
    ((k1,     "lensDistortion:k1"))
    ((k2,     "lensDistortion:k2"))
    ((center, "lensDistortion:center"))
    ((anaSq,  "lensDistortion:anaSq"))
    ((asym,   "lensDistortion:asym"))
    ((scale,  "lensDistortion:scale"))
);
#endif

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((shutterOpenTime,    "ri:shutterOpenTime"))
    ((shutterCloseTime,   "ri:shutterCloseTime"))
    ((shutteropening,     "ri:shutteropening"))
    ((dofAspect,          "ri:dofaspect"))
    ((apertureAngle,      "ri:apertureAngle"))
    ((apertureDensity,    "ri:apertureDensity"))
    ((apertureNSides,     "ri:apertureNSides"))
    ((apertureRoundness,  "ri:apertureRoundness"))
    ((projection,         "ri:projection"))
    ((projection_dofMult, "ri:projection:dofMult"))
    (OSL)
    (resource)
    (RmanCpp)
);

TF_DEFINE_PRIVATE_TOKENS(
    _tokensLegacy,
    ((orthowidth,                     "ri:camera:orthowidth"))
    ((window,                         "ri:camera:window"))
    ((dofAspect,                      "ri:camera:dofaspect"))
    ((apertureNSides,                 "ri:camera:aperturensides"))
    ((apertureAngle,                  "ri:camera:apertureangle"))
    ((apertureRoundness,              "ri:camera:apertureroundness"))
    ((apertureDensity,                "ri:camera:aperturedensity"))
    ((shutteropening1,                "ri:camera:shutteropening1"))
    ((shutteropening2,                "ri:camera:shutteropening2"))
    ((shutterOpenTime,                "ri:camera:shutterOpenTime"))
    ((shutterCloseTime,               "ri:camera:shutterCloseTime"))
);

namespace {

const RtUString us_PxrPerspective("PxrPerspective");
const RtUString us_PxrCamera("PxrCamera");
const RtUString us_PxrOrthographic("PxrOrthographic");

template <class T>
const T*
_GetDictItem(const VtDictionary& dict, const TfToken& key)
{
    const VtValue* v = TfMapLookupPtr(dict, key.GetString());
    return v && v->IsHolding<T>() ? &v->UncheckedGet<T>() : nullptr;
}

std::optional<std::array<float, 8>>
_ToOptionalFloat8(const VtValue &value)
{
    if (!value.IsHolding<VtArray<float>>()) {
        return std::nullopt;
    }
    const VtArray<float> array = value.UncheckedGet<VtArray<float>>();
    if (array.size() != 8) {
        return std::nullopt;
    }
    std::array<float, 8> result;
    for (size_t i = 0; i < 8; i++) {
        result[i] = array[i];
    }
    return result;
}

riley::ShadingNode
_CreateNode(
    HdSceneDelegate *sceneDelegate,
    const SdfPath& riProjectionPath)
{
    riley::ShadingNode node;

    VtValue resourceValue =
        sceneDelegate->Get(riProjectionPath, _tokens->resource);
    if (!resourceValue.IsHolding<HdMaterialNode2>()) {
        node.type = riley::ShadingNode::Type::k_Invalid;
        return node;
    }
    auto resource = resourceValue.UncheckedGet<HdMaterialNode2>();

    SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
    SdrShaderNodeConstPtr sdrEntry =
        sdrRegistry.GetShaderNodeByIdentifier(
            resource.nodeTypeId, { _tokens->OSL, _tokens->RmanCpp, });

    if (!sdrEntry) {
        node.type = riley::ShadingNode::Type::k_Invalid;
        return node;
    }
    std::string shaderPath = sdrEntry->GetImplementationName();
    if (shaderPath.empty()) {
        TF_WARN("Shader '%s' did not provide a valid implementation path.",
                sdrEntry->GetName().c_str());
        node.type = riley::ShadingNode::Type::k_Invalid;
        return node;
    }
    node.name = RtUString(shaderPath.c_str());

    for (const auto &param : resource.parameters) {
        const SdrShaderProperty* prop = sdrEntry->GetShaderInput(param.first);
        if (!prop) {
            TF_WARN("Unknown shaderProperty '%s' for the '%s' "
                    "shader at '%s', ignoring.\n",
                    param.first.GetText(),
                    resource.nodeTypeId.GetText(),
                    riProjectionPath.GetText());
            continue;
        }
        HdPrman_Utils::SetParamFromVtValue(
            RtUString(prop->GetImplementationName().c_str()),
            param.second, prop->GetType(), &node.params);
    }

    node.type = riley::ShadingNode::Type::k_Projection;
    node.handle = RtUString(riProjectionPath.GetText());
    return node;
}

GfVec4f
_RangeToVec4(const GfRange2d& window)
{
    return { float(window.GetMin()[0]), float(window.GetMax()[0]),
             float(window.GetMin()[1]), float(window.GetMax()[1]) };
}

double
_SafeDiv(const double a, const double b)
{
    if (b == 0.) {
        TF_CODING_ERROR("Divide by 0");
        return 1.;
    }
    return a / b;
}

double
_GetDisplayWindowAspect(const CameraUtilFraming& framing)
{
    const GfVec2f& size = framing.displayWindow.GetSize();
    return framing.pixelAspectRatio * _SafeDiv(size[0], size[1]);
}

GfRange2d
_ConvertScreenWindowForDisplayWindowToRenderBuffer(
    const GfRange2d& screenWindowForDisplayWindow,
    const GfRange2f& displayWindow,
    const GfVec2i& renderBufferSize)
{
    const double screenWindowWidthPerPixel =
        screenWindowForDisplayWindow.GetSize()[0] /
        displayWindow.GetSize()[0];

    const double screenWindowHeightPerPixel =
        screenWindowForDisplayWindow.GetSize()[1] /
        displayWindow.GetSize()[1];

    const GfVec2d screenWindowMin(
        screenWindowForDisplayWindow.GetMin()[0]
            - screenWindowWidthPerPixel * displayWindow.GetMin()[0],
        // Note that image space is y-Down and screen window
        // space is y-Up, so this is a bit tricky...
        screenWindowForDisplayWindow.GetMax()[1]
            + screenWindowHeightPerPixel * (
                displayWindow.GetMin()[1] - float(renderBufferSize[1])));

    const GfVec2d screenWindowSize(
        screenWindowWidthPerPixel * renderBufferSize[0],
        screenWindowHeightPerPixel * renderBufferSize[1]);

    return GfRange2d(screenWindowMin, screenWindowMin + screenWindowSize);
}

TfSmallVector<RtMatrix4x4, HDPRMAN_MAX_TIME_SAMPLES>
_ToRtMatrices(
    const HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES>& samples,
    const bool flipZ = false)
{
    using _RtMatrices = TfSmallVector<RtMatrix4x4, HDPRMAN_MAX_TIME_SAMPLES>;
    _RtMatrices matrices(samples.count);

    static const GfMatrix4d flipZMatrix(GfVec4d(1., 1., -1., 1.));

    for (size_t i = 0; i < samples.count; ++i) {
        matrices[i] = HdPrman_Utils::GfMatrixToRtMatrix(
            flipZ
              ? flipZMatrix * samples.values[i]
              : samples.values[i]);
    }

    return matrices;
}

RtNormal3
_Vec3ToRtNormal(const GfVec3d& vec)
{
    return { float(vec[0]), float(vec[1]), float(vec[2]) };
}

RtPoint3
_Vec3ToRtPoint(const GfVec3d& vec)
{
    return { float(vec[0]), float(vec[1]), float(vec[2]) };
}

bool
_ToClipPlaneParams(const GfVec4d& plane, RtParamList* const params)
{
    static const RtUString us_planeNormal("planeNormal");
    static const RtUString us_planeOrigin("planeOrigin");

    const GfVec3d direction(plane[0], plane[1], plane[2]);
    const double directionLength = direction.GetLength();
    if (directionLength == 0.) {
        return false;
    }
    const GfVec3d norm = direction / directionLength;
    params->SetNormal(us_planeNormal, _Vec3ToRtNormal(norm));

    const double distance = -plane[3] / directionLength;
    // The origin can be any point on the plane.
    const RtPoint3 origin = _Vec3ToRtPoint(norm * distance);
    params->SetPoint(us_planeOrigin, origin);

    return true;
}

}  // anonymous namespace

/* static */
const RtUString&
HdPrmanCamera::ComputeProjectionShader(
    const HdCamera::Projection projection,
    const RtUString& projectionOverride)
{
    // Use projection override if it is not default perspective,
    // otherwise defer to camera to decide on orthographic vs perspective.
    // PxrPerspective here matches default of _projection in HdPrman_RenderPass,
    // which matches default in Solaris render settings.
    if (!projectionOverride.Empty() &&
        projectionOverride != us_PxrPerspective) {
        return projectionOverride;
    }

    switch (projection) {
        case HdCamera::Orthographic:
            return us_PxrOrthographic;
        case HdCamera::Perspective:
        default:
            return us_PxrCamera;
    }
}

/* static */
const RtUString&
HdPrmanCamera::ProjectionNodeName()
{
    static const RtUString s_projectionNodeName("cam_projection");
    return s_projectionNodeName;
}

RtParamList
HdPrmanCamera::_ComputePerspectiveNodeParams(bool disableDepthOfField) const
{
    RtParamList result;

    static const RtUString us_lensType("lensType");
    constexpr int lensTypeLensWarp = 2;

    // Pick a PxrProjection lens type that supports depth of field
    // and lens distortion.
    result.SetInteger(us_lensType, lensTypeLensWarp);

    // FOV settings.
    const float focalLength = GetFocalLength();
    if (focalLength > 0) {
        result.SetFloat(RixStr.k_focalLength, float(focalLength));
        const float r = GetHorizontalAperture() / focalLength;
        const double fov = 2. * GfRadiansToDegrees(std::atan(0.5 * r));
        result.SetFloat(RixStr.k_fov, float(fov));
    } else {
        // If focal length is bogus, don't set it.
        // Fallback to sane FOV.
        // TODO: Why are we using 90° here and 60° below?
        result.SetFloat(RixStr.k_fov, 90.f);
    }

    // Depth of field settings.
    const float focusDistance = GetFocusDistance();
    if (focusDistance > 0.f) {
        result.SetFloat(RixStr.k_focalDistance, focusDistance);
    } else {
        // If value is bogus, set to sane value.
        result.SetFloat(RixStr.k_focalDistance, 1000.f);
    }

    const float fStop = GetFStop();
    if (disableDepthOfField || fStop <= 0.f || focusDistance <= 0.f) {
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
    static const RtUString us_dofMult("dofMult");

    result.SetFloat(us_radial1, GetLensDistortionK1());
    result.SetFloat(us_radial2, GetLensDistortionK2());
    result.SetFloatArray(us_distortionCtr, GetLensDistortionCenter().data(), 2);
    result.SetFloat(us_lensSqueeze, GetLensDistortionAnaSq());
    result.SetFloat(us_lensAsymmetryX, GetLensDistortionAsym()[0]);
    result.SetFloat(us_lensAsymmetryY, GetLensDistortionAsym()[1]);
    result.SetFloat(us_lensScale, GetLensDistortionScale());
    result.SetFloat(us_dofMult, GetDofMult());

    return result;
}

// Stub for symmetry
RtParamList
HdPrmanCamera::_ComputeOrthographicNodeParams() const
{
    return { };
}

RtParamList
HdPrmanCamera::_ComputeNodeParams(
    bool disableDepthOfField,
    const RtUString& projectionOverride) const
{
    if (!projectionOverride.Empty() &&
        (projectionOverride != us_PxrPerspective &&
         projectionOverride != us_PxrCamera)) {
        return { };
    }

    switch (GetProjection()) {
        case HdCamera::Orthographic:
            return _ComputeOrthographicNodeParams();
        case HdCamera::Perspective:
        default:
            return _ComputePerspectiveNodeParams(disableDepthOfField);
    }
}

// Compute params given to Riley::ModifyCamera.
RtParamList
HdPrmanCamera::_ComputeCameraParams(const GfRange2d & screenWindow) const
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
    const GfRange1f& clippingRange = GetClippingRange();
    if (clippingRange.GetMin() < clippingRange.GetMax()) {
        result.SetFloat(RixStr.k_nearClip, clippingRange.GetMin());
        result.SetFloat(RixStr.k_farClip, clippingRange.GetMax());
    }

    const ShutterCurve& shutterCurve = GetShutterCurve();
    if (shutterCurve.shutterOpenTime) {
        result.SetFloat(
            RixStr.k_shutterOpenTime, *shutterCurve.shutterOpenTime);
    }
    if (shutterCurve.shutterCloseTime) {
        result.SetFloat(
            RixStr.k_shutterCloseTime, *shutterCurve.shutterCloseTime);
    }
    if (shutterCurve.shutteropening) {
        result.SetFloatArray(
            RixStr.k_shutteropening,
            shutterCurve.shutteropening->data(),
            shutterCurve.shutteropening->size());
    }

    result.SetFloat(RixStr.k_dofaspect, GetDofAspect());
    result.SetFloat(RixStr.k_apertureAngle, GetApertureAngle());
    result.SetFloat(RixStr.k_apertureDensity, GetApertureDensity());
    result.SetInteger(RixStr.k_apertureNSides, GetApertureNSides());
    result.SetFloat(RixStr.k_apertureRoundness, GetApertureRoundness());

    const GfVec4f s = _RangeToVec4(screenWindow);
    result.SetFloatArray(RixStr.k_Ri_ScreenWindow, s.data(), 4);

    return result;
}

void
HdPrmanCamera::_CommitToRiley(
    riley::Riley* const riley,
    const GfRange2d& screenWindow,
    const bool disableDepthOfField,
    const RtUString& projectionNameOverride,
    const RtParamList& projectionParamsOverride)
{
    RtParamList params = _ComputeCameraParams(screenWindow);

    // Backward compatibility for older USD versions and for some camera
    // settings not supported by the studio hdprman. The camera params are split
    // into groups indicating how they are inherited:
    //  customParams: defer to those computed above (the newer way wins).
    //  customParamsOverride: params available with custom names in Solaris that
    //                should override any hardcoded values (eg. shutteropening).
    RtParamList customParams;
    RtParamList customParamsOverride;
    RtParamList customNodeParams;
    SetRileyCameraParams(customParams, customParamsOverride, customNodeParams);
    // If any duplicates, the ones in params win.
    params.Inherit(customParams);
    // If any duplicates, the ones in customParamsOverride win.
    params.Update(customParamsOverride);

    // Favor ri:projection over projectionNameOverride.
    riley::ShadingNode node = GetProjectionNode();

    if (node.type != riley::ShadingNode::Type::k_Invalid) {
        RtParamList cameraNodeParams =
            _ComputeNodeParams(disableDepthOfField, node.name);
        node.params.Inherit(cameraNodeParams);
    } else {
        node = {
            riley::ShadingNode::Type::k_Projection,
            HdPrmanCamera::ComputeProjectionShader(
                GetProjection(), projectionNameOverride),
            HdPrmanCamera::ProjectionNodeName(),
            _ComputeNodeParams(disableDepthOfField, projectionNameOverride) };
        if (!projectionNameOverride.Empty() &&
            (projectionNameOverride == us_PxrPerspective ||
             projectionNameOverride == us_PxrCamera)) {
            node.params.Inherit(customNodeParams);
        }
        node.params.Update(projectionParamsOverride);
    }

    // Coordinate system notes.
    //
    // # Hydra & USD are right-handed
    // - Camera space is always Y-up, looking along -Z.
    // - World space may be either Y-up or Z-up, based on stage metadata.
    //
    // # Prman is left-handed
    // - World is Y-up
    // - Camera looks along +Z.
    //
    // Riley camera xform is "move the camera", aka viewToWorld. Convert
    // right-handed Y-up camera space (USD, Hydra) to left-handed Y-up (Prman)
    // coordinates. This just amounts to flipping the Z axis.
    const HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES>& sampleXforms =
        GetTimeSampleXforms();
    const TfSmallVector<RtMatrix4x4, HDPRMAN_MAX_TIME_SAMPLES> rtMatrices =
        _ToRtMatrices(sampleXforms, /* flipZ = */ true);

    const riley::Transform transform{
        unsigned(sampleXforms.count),
        rtMatrices.data(),
        sampleXforms.times.data() };

    if (_rileyCameraId == riley::CameraId::InvalidId()) {
        _rileyCameraName = RtUString(GetId().GetText());
        _rileyCameraId = riley->CreateCamera(
            riley::UserId(stats::AddDataLocation(
                _rileyCameraName.CStr()).GetValue()),
            _rileyCameraName,
            node,
            transform,
            params);
    } else {
        riley->ModifyCamera(_rileyCameraId, &node, &transform, &params);
    }
}

void
HdPrmanCamera::_UpdateClipPlanes(riley::Riley* const riley)
{
    // Collect params for the non-degenerate scene clip planes. (ToClipPlaneParams
    // skips degenerate planes, so the riley clipping-plane list is not 1:1 with
    // GetClipPlanes().)
    const std::vector<GfVec4d>& clipPlanes = GetClipPlanes();
    std::vector<RtParamList> planeParams;
    planeParams.reserve(clipPlanes.size());
    for (const GfVec4d& plane : clipPlanes) {
        RtParamList params;
        if (_ToClipPlaneParams(plane, &params)) {
            planeParams.push_back(params);
        }
    }

    if (!planeParams.empty()) {
        const HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES>&
            sampleXforms = GetTimeSampleXforms();
        const TfSmallVector<RtMatrix4x4, HDPRMAN_MAX_TIME_SAMPLES> rtMatrices =
            _ToRtMatrices(sampleXforms);
        const riley::Transform transform {
            unsigned(sampleXforms.count),
            rtMatrices.data(),
            sampleXforms.times.data() };
        // Re-use existing Riley clipping plane objects; only create more if
        // number increased
        for (size_t i = 0; i < planeParams.size(); ++i) {
            if (i < _rileyClipPlaneIds.size()) {
                riley->ModifyClippingPlane(
                    _rileyClipPlaneIds[i], &transform, &planeParams[i]);
            } else {
                _rileyClipPlaneIds.push_back(
                    riley->CreateClippingPlane(transform, planeParams[i]));
            }
        }
    }

    // Delete any planes beyond the current count.
    for (size_t i = planeParams.size(); i < _rileyClipPlaneIds.size(); ++i) {
        riley->DeleteClippingPlane(_rileyClipPlaneIds[i]);
    }
    _rileyClipPlaneIds.resize(planeParams.size());
}

void
HdPrmanCamera::_DeleteClipPlanes(riley::Riley* const riley)
{
    for (const riley::ClippingPlaneId & id : _rileyClipPlaneIds) {
        riley->DeleteClippingPlane(id);
    }
    _rileyClipPlaneIds.clear();
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
GfRange2d
HdPrmanCamera::_GetScreenWindow() const
{
    static const double half = 0.5;

    const GfVec2d size(GetHorizontalAperture(), GetVerticalAperture());
    const GfVec2d offset(
        GetHorizontalApertureOffset(), GetVerticalApertureOffset());

    const GfRange2d filmbackPlane(-half * size + offset, +half * size + offset);

    if (GetProjection() == HdCamera::Orthographic) {
        return filmbackPlane;
    }

    if (GetFocalLength() == 0.f || size[0] == 0.f) {
        return filmbackPlane;
    }

    // Note that for perspective projection and with no horizontal aperture,
    // our screen widndow's x-coordinate are in [-1, 1].
    // Divide by appropriate factor to get to this.
    return filmbackPlane / (half * size[0]);
}

HdPrmanCamera::HdPrmanCamera(SdfPath const& id)
  : HdCamera(id)
#if HD_API_VERSION < 52
  , _lensDistortionK1(0.0f)
  , _lensDistortionK2(0.0f)
  , _lensDistortionCenter(0.0f)
  , _lensDistortionAnaSq(1.0f)
  , _lensDistortionAsym(0.0f)
  , _lensDistortionScale(1.0f)
#endif
  , _dofAspect(1.0f)
  , _apertureAngle(0.0f)
  , _apertureDensity(0.0f)
  , _apertureNSides(0)
  , _apertureRoundness(1.0f)
  , _dofMult(1.0f)
  , _rileyCameraId(riley::CameraId::InvalidId())
{
}

HdPrmanCamera::~HdPrmanCamera() = default;

void
HdPrmanCamera::Finalize(HdRenderParam* const renderParam)
{
    auto* const param = static_cast<HdPrman_RenderParam*>(renderParam);
    HdPrman_CameraContext& cameraContext = param->GetCameraContext();

    // de-list this camera from the camera name resolution table
    cameraContext.UnregisterCameraName(GetId());

    riley::Riley* const riley = param->AcquireRiley();
    if (!riley) {
        return;
    }

    bool wasActive = false;
    {
        std::lock_guard<std::mutex> lock(_rileyCameraMutex);
        if (_rileyCameraId == riley::CameraId::InvalidId()) {
            return;
        }
        wasActive = (cameraContext.GetActiveCameraId() == _rileyCameraId);
    }

    // If this is the active camera, switch to the fallback camera before delete
    if (wasActive) {
        cameraContext.ActivateFallbackCamera();
    }

    std::lock_guard<std::mutex> lock(_rileyCameraMutex);
    _DeleteClipPlanes(riley);
    riley->DeleteCamera(_rileyCameraId);
    _rileyCameraId = riley::CameraId::InvalidId();
}

riley::CameraId
HdPrmanCamera::GetRileyCameraId() const
{
    std::lock_guard<std::mutex> lock(_rileyCameraMutex);
    return _rileyCameraId;
}

RtUString
HdPrmanCamera::GetRileyCameraName() const
{
    std::lock_guard<std::mutex> lock(_rileyCameraMutex);
    return _rileyCameraName;
}

void
HdPrmanCamera::UpdateRileyCameraForActive(
    HdPrman_RenderParam* const param,
    const HdPrman_CameraContext::ActiveCameraOverlay& overlay)
{
    riley::Riley* const riley = param->AcquireRiley();
    if (!riley) {
        return;
    }
    HdPrman_CameraContext& cameraContext = param->GetCameraContext();

    std::lock_guard<std::mutex> lock(_rileyCameraMutex);
    _CommitToRiley(
        riley,
        _ConformScreenWindow(overlay),
        overlay.disableDepthOfField,
        overlay.projectionNameOverride,
        overlay.projectionParamsOverride);
    _UpdateClipPlanes(riley);
    cameraContext.RegisterCameraName(GetId(), _rileyCameraName);

}

void
HdPrmanCamera::RecommitRileyCameraForActive(
    HdPrman_RenderParam* const param,
    const HdPrman_CameraContext::ActiveCameraOverlay& overlay)
{
    riley::Riley* const riley = param->AcquireRiley();
    if (!riley) {
        return;
    }

    std::lock_guard<std::mutex> lock(_rileyCameraMutex);

    // Never create from here. _CommitToRiley creates the riley camera when the
    // id is still invalid, and the callers of this reach it at points where the
    // camera's transform has not been sampled yet -- creating there would
    // register the camera with zero motion samples. Generally, the caller
    // should check this before calling RecommitRileyCameraForActive() but this
    // backstop exists just in case.
    if (_rileyCameraId == riley::CameraId::InvalidId()) {
        return;
    }

    _CommitToRiley(
        riley,
        _ConformScreenWindow(overlay),
        overlay.disableDepthOfField,
        overlay.projectionNameOverride,
        overlay.projectionParamsOverride);
}

GfRange2d
HdPrmanCamera::_ConformScreenWindow(
    const HdPrman_CameraContext::ActiveCameraOverlay& overlay) const
{
    const GfRange2d conformedScreenWindow =
        CameraUtilConformedWindow(
            _GetScreenWindow(),
            overlay.windowPolicy,
            _GetDisplayWindowAspect(overlay.framing));

    return overlay.renderBufferSize
        ? _ConvertScreenWindowForDisplayWindowToRenderBuffer(
              conformedScreenWindow,
              overlay.framing.displayWindow,
              *overlay.renderBufferSize)
        : conformedScreenWindow;
}

void
HdPrmanCamera::UpdateRileyCameraForInactive(HdPrman_RenderParam* const param)
{
    riley::Riley* const riley = param->AcquireRiley();
    if (!riley) {
        return;
    }
    HdPrman_CameraContext& cameraContext = param->GetCameraContext();

    const GfRange2d screenWindow = _GetScreenWindow();

    std::lock_guard<std::mutex> lock(_rileyCameraMutex);
    _CommitToRiley(
        riley,
        screenWindow,
        /* disableDepthOfField */ false,
        /* projectionNameOverride */ RtUString(),
        /* projectionParamsOverride */ RtParamList());
    _DeleteClipPlanes(riley);
    cameraContext.RegisterCameraName(GetId(), _rileyCameraName);
}

/* virtual */
void
HdPrmanCamera::Sync(HdSceneDelegate *sceneDelegate,
                    HdRenderParam   *renderParam,
                    HdDirtyBits     *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(sceneDelegate)) {
        return;
    }

    auto* const param = static_cast<HdPrman_RenderParam*>(renderParam);
    HdPrman_CameraContext& cameraContext = param->GetCameraContext();

    SdfPath const &id = GetId();
    // Save state of dirtyBits before HdCamera::Sync clears them.
    const HdDirtyBits bits = *dirtyBits;

    if (bits & AllDirty) {
        cameraContext.MarkCameraInvalidIfActive(id);
    }

    // These are legacy tokens for solaris that aren't updated
    // by HdCamera::Sync
    if (bits & DirtyParams) {
        TfToken params[] = {
            _tokensLegacy->orthowidth,
            _tokensLegacy->window,
            _tokensLegacy->dofAspect,
            _tokensLegacy->apertureNSides,
            _tokensLegacy->apertureAngle,
            _tokensLegacy->apertureRoundness,
            _tokensLegacy->apertureDensity,
            _tokensLegacy->shutteropening1,
            _tokensLegacy->shutteropening2,
            _tokensLegacy->shutterOpenTime,
            _tokensLegacy->shutterCloseTime,
        };

        for (TfToken const& param : params) {
            VtValue val = sceneDelegate->GetCameraParamValue(id, param);
            if (!val.IsEmpty()) {
                _params[param] = val;
            }
        }
    }

    HdCamera::Sync(sceneDelegate, renderParam, dirtyBits);

    if (bits & DirtyParams) {
        const VtValue riProjectionValue =
            sceneDelegate->GetCameraParamValue(id, _tokens->projection);
        const SdfPath riProjectionPath =
            HdPrman_Utils::GetPathFromVtValue(riProjectionValue);
        if (!riProjectionPath.IsEmpty()) {
            _projectionNode = _CreateNode(sceneDelegate, riProjectionPath);
        } else {
            _projectionNode = {};
        }
    }

    if (bits & DirtyParams) {
#if HD_API_VERSION < 52
        _lensDistortionK1 =
            sceneDelegate
                ->GetCameraParamValue(id, _lensDistortionTokens->k1)
                .GetWithDefault<float>(0.0f);
        _lensDistortionK2 =
            sceneDelegate
                ->GetCameraParamValue(id, _lensDistortionTokens->k2)
                .GetWithDefault<float>(0.0f);
        _lensDistortionCenter =
            sceneDelegate
                ->GetCameraParamValue(id, _lensDistortionTokens->center)
                .GetWithDefault<GfVec2f>(GfVec2f(0.0f));
        _lensDistortionAnaSq =
            sceneDelegate
                ->GetCameraParamValue(id, _lensDistortionTokens->anaSq)
                .GetWithDefault<float>(1.0f);
        _lensDistortionAsym =
            sceneDelegate
                ->GetCameraParamValue(id, _lensDistortionTokens->asym)
                .GetWithDefault<GfVec2f>(GfVec2f(0.0f));
        _lensDistortionScale =
            sceneDelegate
                ->GetCameraParamValue(id, _lensDistortionTokens->scale)
                .GetWithDefault<float>(1.0f);
#endif

        const VtValue vShutterOpenTime =
            sceneDelegate->GetCameraParamValue(id, _tokens->shutterOpenTime);
        if (vShutterOpenTime.IsHolding<float>()) {
            _shutterCurve.shutterOpenTime =
                vShutterOpenTime.UncheckedGet<float>();
        } else {
            _shutterCurve.shutterOpenTime = std::nullopt;
        }
        const VtValue vShutterCloseTime =
            sceneDelegate->GetCameraParamValue(id, _tokens->shutterCloseTime);
        if (vShutterCloseTime.IsHolding<float>()) {
            _shutterCurve.shutterCloseTime =
                vShutterCloseTime.UncheckedGet<float>();
        } else {
            _shutterCurve.shutterCloseTime = std::nullopt;
        }
        const VtValue vShutteropening =
            sceneDelegate->GetCameraParamValue(id, _tokens->shutteropening);
        _shutterCurve.shutteropening = _ToOptionalFloat8(vShutteropening);

        _dofAspect =
            sceneDelegate->GetCameraParamValue(id, _tokens->dofAspect)
                         .GetWithDefault<float>(1.0f);
        _apertureAngle =
            sceneDelegate->GetCameraParamValue(id, _tokens->apertureAngle)
                         .GetWithDefault<float>(0.0f);
        _apertureDensity =
            sceneDelegate->GetCameraParamValue(id, _tokens->apertureDensity)
                         .GetWithDefault<float>(0.0f);
        _apertureNSides =
            sceneDelegate->GetCameraParamValue(id, _tokens->apertureNSides)
                         .GetWithDefault<int>(0);
        _apertureRoundness =
            sceneDelegate->GetCameraParamValue(id, _tokens->apertureRoundness)
                         .GetWithDefault<float>(1.0f);
        _dofMult =
            sceneDelegate->GetCameraParamValue(id, _tokens->projection_dofMult)
                         .GetWithDefault<float>(1.0f);
        if (id == cameraContext.GetActiveCameraPath()) {
            // Motion blur in Riley only works correctly if the
            // shutter interval is set before any rprims are synced
            // (and the transform of the riley camera is updated).
            //
            // See SetRileyShutterIntervalFromCameraContextCameraPath
            // for additional context.
            //
            param->SetRileyShutterIntervalFromCameraContextCameraPath(
                &sceneDelegate->GetRenderIndex());
        }
    }

    if (bits & DirtyTransform) {
        // Do SampleTranform last.
        //
        // This is because it needs the shutter interval which is computed above.
        //
        sceneDelegate->SampleTransform(
            id,
#if HD_API_VERSION >= 68
            param->GetShutterInterval()[0],
            param->GetShutterInterval()[1],
#endif
            &_sampleXforms);
    }

    if (id == cameraContext.GetActiveCameraPath()) {
        // Commit as the active camera and bind the default dicing camera now,
        // while we are still in the Sprim phase and therefore still ahead of
        // every geometry prototype. This must happen even when the framing is
        // not yet valid -- the framing only affects the screen window, which the
        // render pass corrects later, whereas the dicing camera binding cannot
        // be corrected after the prototypes are diced.
        cameraContext.CommitActiveCameraDuringSync(
            &sceneDelegate->GetRenderIndex(), this);
        return;
    }
    UpdateRileyCameraForInactive(param);

    // XXX: Should we flip the proj matrix (RHS vs LHS) as well here?

    // We don't need to clear the dirty bits since HdCamera::Sync always clears
    // all the dirty bits.
}

void HdPrmanCamera::setFov(RtParamList& projParams) const
{
    const float horizontalAperture = GetHorizontalAperture();
    const float verticalAperture = GetVerticalAperture();
    const float focalLength = GetFocalLength();

    float filmAspect = horizontalAperture / verticalAperture;
    float aperture = (filmAspect < 1) ? horizontalAperture : verticalAperture;

    float focal = focalLength;
    float fov_rad = 2.f * atan((0.5 * aperture) / focal);

    float fov_deg = fov_rad / M_PI * 180.0;
    projParams.SetFloat(RixStr.k_fov, fov_deg);
}

void HdPrmanCamera::setScreenWindow(RtParamList& camParams, bool isPerspective) const
{
    const float horizontalAperture = GetHorizontalAperture();
    const float horizontalApertureOffset = GetHorizontalApertureOffset();
    const float verticalAperture = GetVerticalAperture();
    const float verticalApertureOffset = GetVerticalApertureOffset();

    float const *orthowidth =
        _GetDictItem<float>(_params, _tokensLegacy->orthowidth);

    GfVec4f const *window =
        _GetDictItem<GfVec4f>(_params, _tokensLegacy->window);

    float screenWindow[4] = {0.f, 0.f, 0.f, 0.f};

    float filmAspect = horizontalAperture / verticalAperture;
    if (window) // user defined
    {
        float const* win = window->GetArray();
        screenWindow[0] = win[0];
        screenWindow[1] = win[1];
        screenWindow[2] = win[2];
        screenWindow[3] = win[3];
    }
    else if (!isPerspective) {
        float wOver2, vOver2;
        float owidth = (orthowidth) ? *orthowidth : 2.f;
        if (filmAspect < 1)
        {
            wOver2 = 0.5f * owidth;
            vOver2 = wOver2 / filmAspect;
        }
        else
        {
            vOver2 = 0.5f * owidth / filmAspect;
            wOver2 = vOver2 * filmAspect;
        }
        screenWindow[0] = -wOver2;
        screenWindow[1] = wOver2;
        screenWindow[2] = -vOver2;
        screenWindow[3] = vOver2;
    }
    else if (filmAspect < 1) {
        screenWindow[0] = -1.f;
        screenWindow[1] = 1.f;
        screenWindow[2] = -1.f/filmAspect;
        screenWindow[3] = 1.f/filmAspect;
    }
    else {
        screenWindow[0] = -filmAspect;
        screenWindow[1] = filmAspect;
        screenWindow[2] = -1.f;
        screenWindow[3] = 1.f;
    }

    // aperture offset has same units as aperture
    float hOffsetScale = (screenWindow[1] - screenWindow[0]) / horizontalAperture;
    screenWindow[0] += horizontalApertureOffset * hOffsetScale;
    screenWindow[1] += horizontalApertureOffset * hOffsetScale;

    // aperture offset has same units as aperture
    float vOffsetScale = (screenWindow[3] - screenWindow[2]) / verticalAperture;
    screenWindow[2] += verticalApertureOffset * vOffsetScale;
    screenWindow[3] += verticalApertureOffset * vOffsetScale;

    camParams.SetFloatArray(RixStr.k_Ri_ScreenWindow, screenWindow, 4);
}

// Some of this method has moved to
// cameraContext.cpp SetCameraAndCameraNodeParams
// where newer camera APIs are used.
// Leaving this here to still be called for backward compatibility
// and some features not supported by the studio's hdprman.
void
HdPrmanCamera::SetRileyCameraParams(RtParamList& camParams,
                                    RtParamList& camParamsOverride,
                                    RtParamList& projParams) const
{
    float const *dofAspect =
        _GetDictItem<float>(_params, _tokensLegacy->dofAspect);
    if (dofAspect) {
        camParamsOverride.SetFloat(RixStr.k_dofaspect, *dofAspect);
    }

    int const *apertureNSides =
        _GetDictItem<int>(_params, _tokensLegacy->apertureNSides);
    if (apertureNSides) {
        camParamsOverride.SetInteger(RixStr.k_apertureNSides, *apertureNSides);
    }

    float const *apertureAngle =
        _GetDictItem<float>(_params, _tokensLegacy->apertureAngle);
    if (apertureAngle) {
        camParamsOverride.SetFloat(RixStr.k_apertureAngle, *apertureAngle);
    }

    float const *apertureRoundness =
        _GetDictItem<float>(_params, _tokensLegacy->apertureRoundness);
    if (apertureRoundness) {
        camParamsOverride.SetFloat(RixStr.k_apertureRoundness, *apertureRoundness);
    }

    float const *apertureDensity =
        _GetDictItem<float>(_params, _tokensLegacy->apertureDensity);
    if (apertureDensity) {
        camParamsOverride.SetFloat(RixStr.k_apertureDensity, *apertureDensity);
    }

    float const *shutterOpenTime =
        _GetDictItem<float>(_params, _tokensLegacy->shutterOpenTime);
    if (shutterOpenTime) {
        camParamsOverride.SetFloat(RixStr.k_shutterOpenTime, *shutterOpenTime);
    }

    float const *shutterCloseTime =
        _GetDictItem<float>(_params, _tokensLegacy->shutterCloseTime);
    if (shutterCloseTime) {
        camParamsOverride.SetFloat(RixStr.k_shutterCloseTime, *shutterCloseTime);
    }

    GfVec4f const *shutteropening1 =
        _GetDictItem<GfVec4f>(_params, _tokensLegacy->shutteropening1);
    GfVec4f const *shutteropening2 =
        _GetDictItem<GfVec4f>(_params, _tokensLegacy->shutteropening2);
    if (shutteropening1 && shutteropening2) {
        float shutteropening[8];
        float const* so1 = shutteropening1->GetArray();
        float const* so2 = shutteropening2->GetArray();
        shutteropening[0] = so1[0];
        shutteropening[1] = so1[1];
        shutteropening[2] = so1[2];
        shutteropening[3] = so1[3];
        shutteropening[4] = so2[0];
        shutteropening[5] = so2[1];
        shutteropening[6] = so2[2];
        shutteropening[7] = so2[3];
        camParamsOverride.SetFloatArray(RixStr.k_shutteropening, shutteropening, 8);
    }

    // Following parameters are currently set on the Riley camera:
    // 'nearClip' (float): near clipping distance
    // 'farClip' (float): near clipping distance
    // 'shutterOpenTime' (float): beginning of normalized shutter interval
    // 'shutterCloseTime' (float): end of normalized shutter interval

    // Parameter that is handled during Riley camera creation:
    // Rix::k_shutteropening (float[8] [c1 c2 d1 d2 e1 e2 f1 f2): additional
    // control points
    // Do not use clipping range if scene delegate did not provide one.
    // Note that we do a sanity check slightly stronger than
    // GfRange1f::IsEmpty() in that we do not allow the range to contain
    // only exactly one point.

    GfMatrix4d const proj = ComputeProjectionMatrix();
    bool isPerspective = round(proj[3][3]) != 1 || proj == GfMatrix4d(1);
    if((TfMapLookupPtr(_params, _tokensLegacy->window) != nullptr)) {
        setScreenWindow(camParamsOverride, isPerspective);
    } else {
        setScreenWindow(camParams, isPerspective);
    }
    if (isPerspective)
    {
       setFov(projParams);
    }
}

static void
_ConvertCameraExposureForPxrPathTracer(
    HdPrmanRenderDelegate* /* renderDelegate */,
    const HdPrmanCamera* camera,
    std::string const& integratorName,
    RtParamList& integratorParams)
{
    if (integratorName == "PxrPathTracer") {
        float emissionMultiplier = 1.0f;
        integratorParams.GetFloat(RtUString("emissionMultiplier"), emissionMultiplier);
        emissionMultiplier *= pow(2.0f, camera->GetExposure());
        integratorParams.SetFloat(RtUString("emissionMultiplier"), emissionMultiplier);
    }
}

TF_REGISTRY_FUNCTION(HdPrman_RenderParam)
{
    HdPrman_RenderParam::RegisterIntegratorCallbackForCamera(
        _ConvertCameraExposureForPxrPathTracer);
}

PXR_NAMESPACE_CLOSE_SCOPE

