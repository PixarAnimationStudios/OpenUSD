//
// Copyright (c) 2022-2024, NVIDIA CORPORATION.
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

#include "pxr/usdImaging/plugin/hdUsdWriter/camera.h"
#include "pxr/usdImaging/plugin/hdUsdWriter/utils.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usdGeom/xformable.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace
{
VtArray<GfVec4f> _VectorVec4dToVtArrayVec4f(const std::vector<GfVec4d>& vec)
{
    return VtArray<GfVec4f>(std::cbegin(vec), std::cend(vec));
}
GfVec2f _Range1fToVec2f(const GfRange1f &range)
{
    return GfVec2f(range.GetMin(), range.GetMax());
}
}

HdUsdWriterCamera::HdUsdWriterCamera(SdfPath const& id) : HdSprim(id)
{
}

HdDirtyBits HdUsdWriterCamera::GetInitialDirtyBitsMask() const
{
    return HdCamera::AllDirty;
}

void HdUsdWriterCamera::Sync(HdSceneDelegate* sceneDelegate, HdRenderParam* renderParam, HdDirtyBits* dirtyBits)
{
    TF_UNUSED(renderParam);

    if (!TF_VERIFY(sceneDelegate != nullptr))
    {
        return;
    }

    SdfPath const& id = GetId();

    // Change tracking
    HdDirtyBits bits = *dirtyBits;

    if (bits & HdCamera::DirtyParams)
    {
        _focalLength = HdUsdWriterGetCameraParamValue<float>(sceneDelegate, id, HdCameraTokens->focalLength);
        _focusDistance = HdUsdWriterGetCameraParamValue<float>(sceneDelegate, id, HdCameraTokens->focusDistance);
        _fStop = HdUsdWriterGetCameraParamValue<float>(sceneDelegate, id, HdCameraTokens->fStop);
        _horizontalAperture = HdUsdWriterGetCameraParamValue<float>(sceneDelegate, id, HdCameraTokens->horizontalAperture);
        _horizontalApertureOffset = HdUsdWriterGetCameraParamValue<float>(sceneDelegate, id, HdCameraTokens->horizontalApertureOffset);
        _verticalAperture = HdUsdWriterGetCameraParamValue<float>(sceneDelegate, id, HdCameraTokens->verticalAperture);
        _verticalApertureOffset = HdUsdWriterGetCameraParamValue<float>(sceneDelegate, id, HdCameraTokens->verticalApertureOffset);
        _exposure = HdUsdWriterGetCameraParamValue<float>(sceneDelegate, id, HdCameraTokens->exposure);
        _shutterOpen = HdUsdWriterGetCameraParamValue<double>(sceneDelegate, id, HdCameraTokens->shutterOpen);
        _shutterClose = HdUsdWriterGetCameraParamValue<double>(sceneDelegate, id, HdCameraTokens->shutterClose);
        _projection = HdUsdWriterGetCameraParamValue<HdCamera::Projection>(sceneDelegate, id, HdCameraTokens->projection);
        _stereoRole = HdUsdWriterGetCameraParamValue<TfToken>(sceneDelegate, id, UsdGeomTokens->stereoRole);
        _clippingRange = HdUsdWriterGetCameraParamValue<GfRange1f>(sceneDelegate, id, HdCameraTokens->clippingRange);
        _focusOn = HdUsdWriterGetCameraParamValue<bool>(sceneDelegate, id, HdCameraTokens->focusOn);
        _dofAspect = HdUsdWriterGetCameraParamValue<float>(sceneDelegate, id, HdCameraTokens->dofAspect);
        _splitDiopterCount = HdUsdWriterGetCameraParamValue<int>(sceneDelegate, id, HdCameraTokens->splitDiopterCount);
        _splitDiopterAngle = HdUsdWriterGetCameraParamValue<float>(sceneDelegate, id, HdCameraTokens->splitDiopterAngle);
        _splitDiopterOffset1 = HdUsdWriterGetCameraParamValue<float>(sceneDelegate, id, HdCameraTokens->splitDiopterOffset1);
        _splitDiopterWidth1 = HdUsdWriterGetCameraParamValue<float>(sceneDelegate, id, HdCameraTokens->splitDiopterWidth1);
        _splitDiopterFocusDistance1 = HdUsdWriterGetCameraParamValue<float>(sceneDelegate, id, HdCameraTokens->splitDiopterFocusDistance1);
        _splitDiopterOffset2 = HdUsdWriterGetCameraParamValue<float>(sceneDelegate, id, HdCameraTokens->splitDiopterOffset2);
        _splitDiopterWidth2 = HdUsdWriterGetCameraParamValue<float>(sceneDelegate, id, HdCameraTokens->splitDiopterWidth2);
        _splitDiopterFocusDistance2 = HdUsdWriterGetCameraParamValue<float>(sceneDelegate, id, HdCameraTokens->splitDiopterFocusDistance2);
        _lensDistortionType = HdUsdWriterGetCameraParamValue<TfToken>(sceneDelegate, id, HdCameraTokens->lensDistortionType);
        _lensDistortionK1 = HdUsdWriterGetCameraParamValue<float>(sceneDelegate, id, HdCameraTokens->lensDistortionK1);
        _lensDistortionK2 = HdUsdWriterGetCameraParamValue<float>(sceneDelegate, id, HdCameraTokens->lensDistortionK2);
        _lensDistortionCenter = HdUsdWriterGetCameraParamValue<float>(sceneDelegate, id, HdCameraTokens->lensDistortionCenter);
        _lensDistortionAnaSq = HdUsdWriterGetCameraParamValue<float>(sceneDelegate, id, HdCameraTokens->lensDistortionAnaSq);
        _lensDistortionAsym = HdUsdWriterGetCameraParamValue<float>(sceneDelegate, id, HdCameraTokens->lensDistortionAsym);
        _lensDistortionScale = HdUsdWriterGetCameraParamValue<float>(sceneDelegate, id, HdCameraTokens->lensDistortionScale);
        _lensDistortionIor = HdUsdWriterGetCameraParamValue<float>(sceneDelegate, id, HdCameraTokens->lensDistortionIor);

        // Visibility and Transforms on Sprims are part of DirtyParams
        _visible = sceneDelegate->GetVisible(id);
        _transform = sceneDelegate->GetTransform(id);
    }

    // Still need to handle DirtyVisibility and DirtyTransform independently of DirtyParams
    if (*dirtyBits & HdChangeTracker::DirtyVisibility)
    {
        _visible = sceneDelegate->GetVisible(id);
    }
    if (bits & (HdCamera::DirtyTransform))
    {
        _transform = sceneDelegate->GetTransform(id);
    }

    if (bits & HdCamera::DirtyClipPlanes)
    {
        _clipPlanes = HdUsdWriterGetCameraParamValue<std::vector<GfVec4d>>(sceneDelegate, id, HdCameraTokens->clipPlanes);
    }

    if (bits & HdCamera::DirtyWindowPolicy)
    {
        _windowPolicy = HdUsdWriterGetCameraParamValue<CameraUtilConformWindowPolicy>(sceneDelegate, id, HdCameraTokens->windowPolicy);
    }

    // Clean all dirty bits.
    *dirtyBits = HdChangeTracker::Clean;
}

void HdUsdWriterCamera::SerializeToUsd(const UsdStagePtr &stage)
{
    auto camera = UsdGeomCamera::Define(stage, GetId());
    auto prim = camera.GetPrim();
    HdUsdWriterPopOptional(_transform,
        [&](const auto& transform)
        {
            HdUsdWriterSetTransformOp(UsdGeomXformable(prim), transform);
        });
    HdUsdWriterSetVisible(_visible, prim);

    HdUsdWriterPopOptional(_clipPlanes,
        [&](const auto& clipPlanes)
        {
            // For being able to roundtrip UsdGeomCamera.
            // Serialized as Float4[] 'clippingPlanes'.
            const auto& attr = prim.CreateAttribute(UsdGeomTokens->clippingPlanes, SdfValueTypeNames->Float4Array,
                    false /*custom*/, SdfVariabilityVarying);
            HdUsdWriterSetOrWarn(attr, _VectorVec4dToVtArrayVec4f(*_clipPlanes));
        });
    HdUsdWriterPopOptional(_focalLength, [&](const auto& focalLength)
        {
            const auto& attr = camera.CreateFocalLengthAttr();
            HdUsdWriterSetOrWarn(attr, float(focalLength / GfCamera::FOCAL_LENGTH_UNIT));
        });
    HdUsdWriterPopOptional(
        _focusDistance, [&](const auto& focusDistance)
        {
            const auto& attr = camera.CreateFocusDistanceAttr();
            HdUsdWriterSetOrWarn(attr, focusDistance);
        });
    HdUsdWriterPopOptional(_fStop, [&](const auto& fStop)
        {
            const auto& attr = camera.CreateFStopAttr();
            HdUsdWriterSetOrWarn(attr, fStop);
        });
    HdUsdWriterPopOptional(_horizontalAperture,
        [&](const auto& horizontalAperture)
        {
            const auto& attr = camera.CreateHorizontalApertureAttr();
            HdUsdWriterSetOrWarn(attr, float(horizontalAperture / (float)GfCamera::APERTURE_UNIT));
        });
    HdUsdWriterPopOptional(_horizontalApertureOffset,
        [&](const auto& horizontalApertureOffset)
        {
            const auto& attr = camera.CreateHorizontalApertureOffsetAttr();
            HdUsdWriterSetOrWarn(attr, float(horizontalApertureOffset / (float)GfCamera::APERTURE_UNIT));
        });
    HdUsdWriterPopOptional(
        _verticalAperture, [&](const auto& verticalAperture)
        {
            const auto& attr = camera.CreateVerticalApertureAttr();
            HdUsdWriterSetOrWarn(attr, float(verticalAperture / (float)GfCamera::APERTURE_UNIT));
        });
    HdUsdWriterPopOptional(_verticalApertureOffset,
        [&](const auto& verticalApertureOffset)
        {
            const auto& attr = camera.CreateVerticalApertureOffsetAttr();
            HdUsdWriterSetOrWarn(attr, float(verticalApertureOffset / (float)GfCamera::APERTURE_UNIT));
        });
    HdUsdWriterPopOptional(_exposure, [&](const auto& exposure)
        {
            const auto& attr = camera.CreateExposureAttr();
            HdUsdWriterSetOrWarn(attr, exposure);
        });
    HdUsdWriterPopOptional(
        _clippingRange, [&](const auto& clippingRange)
        {
            const auto& attr = camera.CreateClippingRangeAttr();
            HdUsdWriterSetOrWarn(attr, _Range1fToVec2f(clippingRange));
        });
    HdUsdWriterPopOptional(_shutterOpen, [&](const auto& shutterOpen)
        {
            const auto& attr = camera.CreateShutterOpenAttr();
            HdUsdWriterSetOrWarn(attr, shutterOpen);
        });
    HdUsdWriterPopOptional(
        _shutterClose, [&](const auto& shutterClose)
        {
            const auto& attr = camera.CreateShutterCloseAttr();
            HdUsdWriterSetOrWarn(attr, shutterClose);
        });
    HdUsdWriterPopOptional(_stereoRole, [&](const auto& stereoRole)
        {
            const auto& attr = camera.CreateStereoRoleAttr();
            HdUsdWriterSetOrWarn(attr, stereoRole);
        });
    HdUsdWriterPopOptional(_projection,
        [&](const auto& projection)
        {
            const auto& attr = camera.CreateProjectionAttr();
            HdUsdWriterSetOrWarn(attr, projection == HdCamera::Projection::Perspective ?
                UsdGeomTokens->perspective :
                UsdGeomTokens->orthographic);
        });
    HdUsdWriterPopOptional(_windowPolicy,
        [&](const auto& windowPolicy)
        {
            const auto& attr = prim.CreateAttribute(HdCameraTokens->windowPolicy, SdfValueTypeNames->Int,
                true /*custom*/, SdfVariabilityVarying);
            HdUsdWriterSetOrWarn(attr, int(windowPolicy));
        });
    HdUsdWriterPopOptional(_focusOn,
        [&](const auto& focusOn)
        {
            const auto& attr = prim.CreateAttribute(HdCameraTokens->focusOn, SdfValueTypeNames->Bool,
                true /*custom*/, SdfVariabilityVarying);
            HdUsdWriterSetOrWarn(attr, focusOn);
        });
    HdUsdWriterPopOptional(_dofAspect,
        [&](const auto& dofAspect)
        {
            const auto& attr = prim.CreateAttribute(HdCameraTokens->dofAspect, SdfValueTypeNames->Float,
                true /*custom*/, SdfVariabilityVarying);
            HdUsdWriterSetOrWarn(attr, dofAspect);
        });
    HdUsdWriterPopOptional(_splitDiopterCount,
        [&](const auto& splitDiopterCount)
        {
            const auto& attr = prim.CreateAttribute(HdCameraTokens->splitDiopterCount, SdfValueTypeNames->Int,
                true /*custom*/, SdfVariabilityVarying);
            HdUsdWriterSetOrWarn(attr, splitDiopterCount);
        });
    HdUsdWriterPopOptional(_splitDiopterAngle,
        [&](const auto& splitDiopterAngle)
        {
            const auto& attr = prim.CreateAttribute(HdCameraTokens->splitDiopterAngle, SdfValueTypeNames->Float,
                true /*custom*/, SdfVariabilityVarying);
            HdUsdWriterSetOrWarn(attr, splitDiopterAngle);
        });
    HdUsdWriterPopOptional(_splitDiopterOffset1,
        [&](const auto& splitDiopterOffset1)
        {
            const auto& attr = prim.CreateAttribute(HdCameraTokens->splitDiopterOffset1, SdfValueTypeNames->Float,
                true /*custom*/, SdfVariabilityVarying);
            HdUsdWriterSetOrWarn(attr, splitDiopterOffset1);
        });
    HdUsdWriterPopOptional(_splitDiopterWidth1,
        [&](const auto& splitDiopterWidth1)
        {
            const auto& attr = prim.CreateAttribute(HdCameraTokens->splitDiopterWidth1, SdfValueTypeNames->Float,
                true /*custom*/, SdfVariabilityVarying);
            HdUsdWriterSetOrWarn(attr, splitDiopterWidth1);
        });
    HdUsdWriterPopOptional(_splitDiopterFocusDistance1,
        [&](const auto& splitDiopterFocusDistance1)
        {
            const auto& attr = prim.CreateAttribute(HdCameraTokens->splitDiopterFocusDistance1, SdfValueTypeNames->Float,
                true /*custom*/, SdfVariabilityVarying);
            HdUsdWriterSetOrWarn(attr, splitDiopterFocusDistance1);
        });
        HdUsdWriterPopOptional(_splitDiopterOffset1,
        [&](const auto& splitDiopterOffset1)
        {
            const auto& attr = prim.CreateAttribute(HdCameraTokens->splitDiopterOffset1, SdfValueTypeNames->Float,
                true /*custom*/, SdfVariabilityVarying);
            HdUsdWriterSetOrWarn(attr, splitDiopterOffset1);
        });
    HdUsdWriterPopOptional(_splitDiopterWidth2,
        [&](const auto& splitDiopterWidth2)
        {
            const auto& attr = prim.CreateAttribute(HdCameraTokens->splitDiopterWidth2, SdfValueTypeNames->Float,
                true /*custom*/, SdfVariabilityVarying);
            HdUsdWriterSetOrWarn(attr, splitDiopterWidth2);
        });
    HdUsdWriterPopOptional(_splitDiopterFocusDistance2,
        [&](const auto& splitDiopterFocusDistance2)
        {
            const auto& attr = prim.CreateAttribute(HdCameraTokens->splitDiopterFocusDistance2, SdfValueTypeNames->Float,
                true /*custom*/, SdfVariabilityVarying);
            HdUsdWriterSetOrWarn(attr, splitDiopterFocusDistance2);
        });
    HdUsdWriterPopOptional(_lensDistortionType,
        [&](const auto& lensDistortionType)
        {
            const auto& attr = prim.CreateAttribute(HdCameraTokens->lensDistortionType, SdfValueTypeNames->Token,
                true /*custom*/, SdfVariabilityVarying);
            HdUsdWriterSetOrWarn(attr, lensDistortionType);
        });
    HdUsdWriterPopOptional(_lensDistortionK1,
        [&](const auto& lensDistortionK1)
        {
            const auto& attr = prim.CreateAttribute(HdCameraTokens->lensDistortionK1, SdfValueTypeNames->Float,
                true /*custom*/, SdfVariabilityVarying);
            HdUsdWriterSetOrWarn(attr, lensDistortionK1);
        });
    HdUsdWriterPopOptional(_lensDistortionK2,
        [&](const auto& lensDistortionK2)
        {
            const auto& attr = prim.CreateAttribute(HdCameraTokens->lensDistortionK2, SdfValueTypeNames->Float,
                true /*custom*/, SdfVariabilityVarying);
            HdUsdWriterSetOrWarn(attr, lensDistortionK2);
        });
    HdUsdWriterPopOptional(_lensDistortionCenter,
        [&](const auto& lensDistortionCenter)
        {
            const auto& attr = prim.CreateAttribute(HdCameraTokens->lensDistortionCenter, SdfValueTypeNames->Float,
                true /*custom*/, SdfVariabilityVarying);
            HdUsdWriterSetOrWarn(attr, lensDistortionCenter);
        });
    HdUsdWriterPopOptional(_lensDistortionAnaSq,
        [&](const auto& lensDistortionAnaSq)
        {
            const auto& attr = prim.CreateAttribute(HdCameraTokens->lensDistortionAnaSq, SdfValueTypeNames->Float,
                true /*custom*/, SdfVariabilityVarying);
            HdUsdWriterSetOrWarn(attr, lensDistortionAnaSq);
        });
    HdUsdWriterPopOptional(_lensDistortionAsym,
        [&](const auto& lensDistortionAsym)
        {
            const auto& attr = prim.CreateAttribute(HdCameraTokens->lensDistortionAsym, SdfValueTypeNames->Float,
                true /*custom*/, SdfVariabilityVarying);
            HdUsdWriterSetOrWarn(attr, lensDistortionAsym);
        });
    HdUsdWriterPopOptional(_lensDistortionScale,
        [&](const auto& lensDistortionScale)
        {
            const auto& attr = prim.CreateAttribute(HdCameraTokens->lensDistortionScale, SdfValueTypeNames->Float,
                true /*custom*/, SdfVariabilityVarying);
            HdUsdWriterSetOrWarn(attr, lensDistortionScale);
        });
    HdUsdWriterPopOptional(_lensDistortionIor,
        [&](const auto& lensDistortionIor)
        {
            const auto& attr = prim.CreateAttribute(HdCameraTokens->lensDistortionIor, SdfValueTypeNames->Float,
                true /*custom*/, SdfVariabilityVarying);
            HdUsdWriterSetOrWarn(attr, lensDistortionIor);
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
