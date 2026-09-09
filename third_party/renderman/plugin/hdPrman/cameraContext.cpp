//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/cameraContext.h"

#include "hdPrman/camera.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/rixStrings.h"
#include "hdPrman/utils.h"

#include "pxr/imaging/cameraUtil/conformWindow.h"
#include "pxr/imaging/cameraUtil/framing.h"
#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/gf/rect2i.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticData.h"

#include "pxr/pxr.h"

#include <Riley.h>
#include <RileyIds.h>
#include <RiTypesHelper.h>
#include <RixShadingUtils.h>
#include <stats/Roz.h>

#include <cmath>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

static constexpr float f_fallbackFov = 60.f;

HdPrman_CameraContext::HdPrman_CameraContext(
    HdPrman_RenderParam* const renderParam)
  : _renderParam(renderParam)
  , _policy(CameraUtilFit)
  , _disableDepthOfField(false)
  , _invalid(false)
{
    // Seed the cached params hash from the (empty) initial value, so that
    // SetProjectionOverride's compare-and-set sees no change when it is first
    // called with an absent/empty override. Hashing a hard-coded 0 instead would
    // make that first call invalidate spuriously on every render.
    _projectionParamsOverrideHash = _projectionParamsOverride.Hash();
}

void
HdPrman_CameraContext::MarkCameraInvalidIfActive(const SdfPath &path)
{
    // No need to invalidate if camera that is not the active camera
    // changed. Compared against the requested path, so a camera nominated
    // before it exists in the render index still invalidates.
    if (path == _requestedActiveCameraPath) {
        _invalid = true;
    }
}

void
HdPrman_CameraContext::SetActiveCameraPath(const SdfPath &path)
{
    if (_requestedActiveCameraPath != path) {
        _invalid = true;
        _requestedActiveCameraPath = path;
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

HdPrman_CameraContext::ActiveCameraOverlay
HdPrman_CameraContext::GetActiveCameraOverlay(
    const std::optional<GfVec2i> renderBufferSize) const
{
    return ActiveCameraOverlay{
        _framing,
        _policy,
        _projectionNameOverride,
        _projectionParamsOverride,
        _disableDepthOfField,
        renderBufferSize };
}

void
HdPrman_CameraContext::UpdateActiveCamera(
    const HdRenderIndex* const renderIndex,
    const std::optional<GfVec2i> renderBufferSize)
{
    if (!_renderParam || !renderIndex) { return; }
    HdPrmanCamera* const camera =
        GetActiveCamera(renderIndex);

    const ActiveCameraOverlay overlay =
        GetActiveCameraOverlay(renderBufferSize);
    // Remember what we applied so ReapplyActiveCamera can replay it after a
    // Riley::SetOptions.
    _lastAppliedOverlay = overlay;

    if (camera) {
        camera->UpdateRileyCameraForActive(_renderParam, overlay);
        _UpdateActiveCameraIds(camera);
    } else {
        // Use the fallback camera instead. Note this resets only the resolved
        // identity; _requestedActiveCameraPath is left alone so a camera
        // nominated before its Sprim appeared is still recognized when it does.
        _ApplyFallbackOverlay(overlay);
        _activeCamera = _fallbackCamera;
    }

    _BindActiveCamera(renderIndex);
}

void
HdPrman_CameraContext::_BindActiveCamera(const HdRenderIndex* const renderIndex)
{
    if (!_renderParam) { return; }

    if (_activeCamera.id == _overlayCamera.id &&
        _activeCamera.path == _overlayCamera.path) {
        return;
    }

    _RevertPrevious(renderIndex, _activeCamera.path);
    if (riley::Riley* const riley = _renderParam->AcquireRiley()) {
        riley->SetDefaultDicingCamera(_activeCamera.id);
        _renderParam->GetRenderViewContext()
            .SetCameraId(_activeCamera.id, riley);
    }
    _overlayCamera = _activeCamera;
}

void
HdPrman_CameraContext::CommitActiveCameraDuringSync(
    const HdRenderIndex* const renderIndex,
    HdPrmanCamera* const camera)
{
    if (!_renderParam || !camera) { return; }

    // Riley requires, before any geometry prototype is created, that the active
    // camera already exist with its real transform (including motion samples)
    // and already be bound as the scene's default dicing camera -- dicing
    // happens at prototype creation and reads the dicing camera's projection and
    // transform, so re-binding afterwards cannot retroactively fix it.
    //
    // HdRenderIndex::SyncAll syncs all Sprims before any Rprim, so the active
    // camera's own Sync is the last point that reliably precedes all geometry.
    // That is why this runs here rather than from the render pass.
    const ActiveCameraOverlay overlay = GetActiveCameraOverlay();

    if (overlay.framing.IsValid()) {
        _lastAppliedOverlay = overlay;
        camera->UpdateRileyCameraForActive(_renderParam, overlay);
    } else {
        // The framing is not resolved until the render pass runs, and conforming
        // against an invalid framing would divide by a zero-sized display
        // window. Commit the camera's intrinsic screen window for now -- what
        // matters here is that the camera exists with the right transform and
        // projection; the render pass corrects the screen window via
        // ModifyCamera once the framing is known.
        camera->UpdateRileyCameraForInactive(_renderParam);
    }

    _UpdateActiveCameraIds(camera);
    _BindActiveCamera(renderIndex);
}

void
HdPrman_CameraContext::ReapplyActiveCamera(
    const HdRenderIndex* const renderIndex)
{
    if (!_renderParam) { return; }

    // Nothing has been committed yet, so there is nothing to re-assert. This is
    // the case for the very first SetOptions, issued from
    // HdPrman_RenderParam::Begin before _CreateInternalPrims has created even
    // the fallback camera.
    if (!_lastAppliedOverlay) { return; }

    // An empty active path means the fallback camera is active (both
    // ActivateFallbackCamera and UpdateActiveCamera's else-branch assign
    // _activeCamera = _fallbackCamera, whose path is never set).
    if (_activeCamera.path.IsEmpty()) {
        _ApplyFallbackOverlay(*_lastAppliedOverlay);
        return;
    }

    // Re-resolve the camera from the render index by path on every call rather
    // than caching the pointer: the camera Sprim can be dropped between two
    // SetOptions calls, and a stale pointer here would outlive it.
    if (!renderIndex) { return; }
    HdPrmanCamera* const camera = GetActiveCamera(renderIndex);
    if (!camera) { return; }

    // Only re-commit a camera that riley already knows about.
    // HdPrmanCamera::_CommitToRiley creates the riley camera when its id is
    // still invalid, and this hook is reached from paths where that has not
    // happened yet -- notably from within HdPrmanCamera::Sync itself, via
    // SetRileyShutterIntervalFromCameraContextCameraPath, which runs before the
    // DirtyTransform block has sampled any transform. Creating there would
    // register the camera with zero motion samples.
    if (_activeCamera.id == riley::CameraId::InvalidId()) { return; }

    // Deliberately narrower than UpdateActiveCamera: this republishes only the
    // camera's parameters. SetOptions does not disturb the default dicing
    // camera or the render view's camera binding, so the transition work done on
    // an identity change is not repeated, and RecommitRileyCameraForActive
    // leaves the clipping planes and the camera-name registry alone.
    camera->RecommitRileyCameraForActive(_renderParam, *_lastAppliedOverlay);
}

void
HdPrman_CameraContext::_UpdateActiveCameraIds(
    const HdPrmanCamera* const camera)
{
    _activeCamera.id = camera->GetRileyCameraId();
    _activeCamera.name = camera->GetRileyCameraName();
    _activeCamera.path = camera->GetId();
}

void
HdPrman_CameraContext::_RevertPrevious(
    const HdRenderIndex* const renderIndex,
    const SdfPath& newActivePath)
{
    if (_overlayCamera.id == riley::CameraId::InvalidId()) { return; }

    const SdfPath prevPath = _overlayCamera.path;
    if (prevPath.IsEmpty()) {
        _RevertFallback();
        return;
    }
    if (prevPath == newActivePath) { return; }

    auto* const prevCamera = static_cast<HdPrmanCamera*>(
        renderIndex->GetSprim(HdPrimTypeTokens->camera, prevPath));
    if (!prevCamera) {
        return;
    }

    if (!_renderParam) { return; }
    prevCamera->UpdateRileyCameraForInactive(_renderParam);
}

void
HdPrman_CameraContext::ActivateFallbackCamera()
{
    _activeCamera = _fallbackCamera;

    const ActiveCameraOverlay overlay = GetActiveCameraOverlay();
    _lastAppliedOverlay = overlay;
    _ApplyFallbackOverlay(overlay);

    _overlayCamera = _fallbackCamera;

    if (!_renderParam) { return; }

    if (riley::Riley* const riley = _renderParam->AcquireRiley()) {
        riley->SetDefaultDicingCamera(_fallbackCamera.id);
        _renderParam->GetRenderViewContext()
            .SetCameraId(_fallbackCamera.id, riley);
    }
}

void
HdPrman_CameraContext::_ApplyFallbackOverlay(
    const ActiveCameraOverlay& overlay)
{
    if (_fallbackCamera.id == riley::CameraId::InvalidId()) { return; }
    if (!_renderParam) { return; }
    riley::Riley* const riley = _renderParam->AcquireRiley();
    if (!riley) { return; }

    RtParamList nodeParams;
    nodeParams.SetFloat(RixStr.k_fov, f_fallbackFov);
    riley::ShadingNode node{
        riley::ShadingNode::Type::k_Projection,
        HdPrmanCamera::ComputeProjectionShader(
            HdCamera::Perspective, overlay.projectionNameOverride),
        HdPrmanCamera::ProjectionNodeName(),
        nodeParams };
    node.params.Update(overlay.projectionParamsOverride);
    riley->ModifyCamera(_fallbackCamera.id, &node, nullptr, nullptr);
}

void
HdPrman_CameraContext::_RevertFallback()
{
    if (_fallbackCamera.id == riley::CameraId::InvalidId()) { return; }
    if (!_renderParam) { return; }
    riley::Riley* const riley = _renderParam->AcquireRiley();
    if (!riley) { return; }

    static const RtUString us_PxrPerspective("PxrPerspective");
    RtParamList nodeParams;
    nodeParams.SetFloat(RixStr.k_fov, f_fallbackFov);
    const riley::ShadingNode node{
        riley::ShadingNode::Type::k_Projection,
        HdPrmanCamera::ComputeProjectionShader(
            HdCamera::Perspective, us_PxrPerspective),
        HdPrmanCamera::ProjectionNodeName(),
        nodeParams };
    riley->ModifyCamera(_fallbackCamera.id, &node, nullptr, nullptr);
}

void
HdPrman_CameraContext::RegisterCameraName(
    const SdfPath& cameraPath,
    const RtUString& rileyName)
{
    std::unique_lock<std::shared_mutex> lock(_cameraNameTableMutex);
    _cameraNameTable[cameraPath] = rileyName;
}

void
HdPrman_CameraContext::UnregisterCameraName(const SdfPath& cameraPath)
{
    std::unique_lock<std::shared_mutex> lock(_cameraNameTableMutex);
    _cameraNameTable.erase(cameraPath);
}

RtUString
HdPrman_CameraContext::ResolveCameraName(
    const std::string& authored) const
{
    if (authored.empty()) {
        return RtUString();
    }

    std::shared_lock<std::shared_mutex> lock(_cameraNameTableMutex);

    // (1) Exact Hydra path match. Each HdPrmanCamera registered its riley
    // camera name under its Hydra path, so the canonical name is that entry.
    if (SdfPath::IsValidPathString(authored)) {
        const auto it = _cameraNameTable.find(SdfPath(authored));
        if (it != _cameraNameTable.end()) {
            return it->second;
        }
    }

    // (2) Unique terminal-token ("name") match.
    const TfToken authoredToken(authored);
    const RtUString* tokenMatch = nullptr;
    bool tokenAmbiguous = false;
    for (const auto& entry : _cameraNameTable) {
        if (entry.first.GetNameToken() != authoredToken) {
            continue;
        }
        if (tokenMatch) {
            tokenAmbiguous = true;
            break;
        }
        tokenMatch = &entry.second;
    }
    if (tokenAmbiguous) {
        TF_WARN("'%s' matches more than one camera by name; ignoring.",
            authored.c_str());
        return RtUString();
    }
    if (tokenMatch) {
        return *tokenMatch;
    }

    TF_WARN("'%s' does not resolve to a registered camera; ignoring.",
        authored.c_str());
    return RtUString();
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

    return (a - 0.0078125f) / b;
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

// This can be removed once XPU handles under/overscan correctly.
GfVec2i
HdPrman_CameraContext::GetResolutionFromDataWindow() const
{
    return _framing.dataWindow.GetSize();
}

void
HdPrman_CameraContext::SetRileyOptions(
    RtParamList * const options) const
{
    const GfVec2i res = GetResolutionFromDisplayWindow();

    options->SetIntegerArray(
        RixStr.k_Ri_FormatResolution,
        res.data(), 2);

    // Compute how the data window sits in the display window.
    const GfVec4f cropWindow =
        _ComputeCropWindow(
            _framing.dataWindow,
            _framing.displayWindow.GetMin(),
            res);

    options->SetFloatArray(
        RixStr.k_Ri_CropWindow,
        cropWindow.data(), 4);

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

    options->SetIntegerArray(
        RixStr.k_Ri_FormatResolution,
        renderBufferSize.data(), 2);

    options->SetFloat(
        RixStr.k_Ri_FormatPixelAspectRatio,
        _framing.pixelAspectRatio);
}

void
HdPrman_CameraContext::SetProjectionOverride(const RtUString& projection,
                                            const RtParamList& projectionParams)
{
    // The projection override is part of ActiveCameraOverlay, so a change to it
    // MUST invalidate: the drive loops re-commit the active camera only while
    // the context is invalid (renderPass.cpp's camChanged latch,
    // renderSettings.cpp's IsInvalid() gate). Without this, a projection-only
    // edit was stashed and applied on some later frame that happened to
    // invalidate for another reason -- and never at all for a single-frame
    // offline render.
    //
    // Compare-and-set rather than invalidating unconditionally, to match the
    // other overlay setters. RtParamList (pxrcore::ParamList) has no
    // operator==, and its Hash() is non-const and re-sorts, so the params half
    // is compared via a cached hash taken from a local copy.
    RtParamList newParams = projectionParams;
    const uint32_t newParamsHash = newParams.Hash();
    if (_projectionNameOverride == projection &&
        _projectionParamsOverrideHash == newParamsHash) {
        return;
    }

    _invalid = true;
    _projectionNameOverride = projection;
    _projectionParamsOverride = projectionParams;
    _projectionParamsOverrideHash = newParamsHash;
}

void
HdPrman_CameraContext::MarkValid()
{
    _invalid = false;
}

void
HdPrman_CameraContext::CreateFallbackCamera()
{
    if (!_renderParam) { return; }
    riley::Riley* const riley = _renderParam->AcquireRiley();
    if (!riley) { return; }

    static const RtUString us_PxrPerspective("PxrPerspective");

    _fallbackCamera.name = GetFallbackCameraName();

    RtParamList nodeParams;
    nodeParams.SetFloat(RixStr.k_fov, f_fallbackFov);

    // Projection
    const riley::ShadingNode node = riley::ShadingNode {
        riley::ShadingNode::Type::k_Projection,
        HdPrmanCamera::ComputeProjectionShader(
            HdCamera::Perspective, us_PxrPerspective),
        HdPrmanCamera::ProjectionNodeName(),
        nodeParams };

    // Camera params
    RtParamList params;

    // Transform
    static const std::vector<float> zerotime { 0.f };
    static const std::vector<RtMatrix4x4> matrix { {
        1.f, 0.f,   0.f, 0.f,
        0.f, 1.f,   0.f, 0.f,
        0.f, 0.f,   1.f, 0.f,
        0.f, 0.f, -10.f, 1.f } };
    static const riley::Transform transform = {
        1, matrix.data(), zerotime.data() };

    _fallbackCamera.id = riley->CreateCamera(
        riley::UserId(
            stats::AddDataLocation(_fallbackCamera.name.CStr()).GetValue()),
        _fallbackCamera.name,
        node,
        transform,
        params);

    if (_requestedActiveCameraPath == SdfPath::EmptyPath()) {
        ActivateFallbackCamera();
    }

    // NOTE: The default dicing camera is now set (and re-issued on active-
    // camera change) by the render param / render pass against the active
    // camera; it is intentionally not bound here. See _CreateInternalPrims.
}

void
HdPrman_CameraContext::DeleteFallbackCamera()
{
    if (!_renderParam) { return; }
    riley::Riley* const riley = _renderParam->AcquireRiley();
    if (!riley) { return; }

    if (_fallbackCamera.id != riley::CameraId::InvalidId()) {
        riley->DeleteCamera(_fallbackCamera.id);
        _fallbackCamera.id = riley::CameraId::InvalidId();
    }
}

HdPrmanCamera*
HdPrman_CameraContext::GetActiveCamera(
    const HdRenderIndex * const renderIndex) const
{
    return static_cast<HdPrmanCamera*>(
        renderIndex->GetSprim(
            HdPrimTypeTokens->camera, _requestedActiveCameraPath));
}

const CameraUtilFraming &
HdPrman_CameraContext::GetFraming() const
{
    return _framing;
}

/* static */
RtUString
HdPrman_CameraContext::GetFallbackCameraName()
{
    static const RtUString name("__hdPrman_fallback_camera");
    return name;
}

PXR_NAMESPACE_CLOSE_SCOPE
