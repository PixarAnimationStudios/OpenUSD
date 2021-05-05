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
#include "pxr/usdImaging/usdImaging/cameraAdapter.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/usd/usdGeom/camera.h"

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/camera.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdImagingCameraAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingCameraAdapter::~UsdImagingCameraAdapter() = default;

bool
UsdImagingCameraAdapter::IsSupported(UsdImagingIndexProxy const* index) const
{
    return index->IsSprimTypeSupported(HdPrimTypeTokens->camera);
}

SdfPath
UsdImagingCameraAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    if (!TF_VERIFY(prim.IsA<UsdGeomCamera>())) {
        return SdfPath();
    }

    index->InsertSprim(HdPrimTypeTokens->camera, prim.GetPath(), prim);
    HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);

    return prim.GetPath();
}

void 
UsdImagingCameraAdapter::TrackVariability(UsdPrim const& prim,
                                          SdfPath const& cachePath,
                                          HdDirtyBits* timeVaryingBits,
                                          UsdImagingInstancerContext const* 
                                              instancerContext) const
{
    UsdGeomCamera cam(prim);
    if (!TF_VERIFY(cam)) {
        return;
    }

    // Discover time-varying transforms.
    _IsTransformVarying(prim,
        HdCamera::DirtyTransform,
        UsdImagingTokens->usdVaryingXform,
        timeVaryingBits);

    // Properties that affect the projection matrix.
    // To get a small speed boost, if an attribute sets a dirty bit we skip
    // all subsequent checks for that dirty bit.
    _IsVarying(prim,
        UsdGeomTokens->projection,
        HdCamera::DirtyProjMatrix,
        HdCameraTokens->projection,
        timeVaryingBits,
        false);

    if ((*timeVaryingBits & HdCamera::DirtyProjMatrix) == 0) {
        _IsVarying(prim,
            UsdGeomTokens->horizontalAperture,
            HdCamera::DirtyProjMatrix,
            HdCameraTokens->projectionMatrix,
            timeVaryingBits,
            false);
    }
        
    if ((*timeVaryingBits & HdCamera::DirtyProjMatrix) == 0) {
        _IsVarying(prim,
            UsdGeomTokens->verticalAperture,
            HdCamera::DirtyProjMatrix,
            HdCameraTokens->projectionMatrix,
            timeVaryingBits,
            false);
    }
    if ((*timeVaryingBits & HdCamera::DirtyProjMatrix) == 0) {
        _IsVarying(prim,
            UsdGeomTokens->horizontalApertureOffset,
            HdCamera::DirtyProjMatrix,
            HdCameraTokens->projectionMatrix,
            timeVaryingBits,
            false);
    }
    if ((*timeVaryingBits & HdCamera::DirtyProjMatrix) == 0) {
        _IsVarying(prim,
            UsdGeomTokens->verticalApertureOffset,
            HdCamera::DirtyProjMatrix,
            HdCameraTokens->projectionMatrix,
            timeVaryingBits,
            false);
    }
    if ((*timeVaryingBits & HdCamera::DirtyProjMatrix) == 0) {
        _IsVarying(prim,
            UsdGeomTokens->clippingRange,
            HdCamera::DirtyProjMatrix,
            HdCameraTokens->projectionMatrix,
            timeVaryingBits,
            false);
    }
    if ((*timeVaryingBits & HdCamera::DirtyProjMatrix) == 0) {
        _IsVarying(prim,
            UsdGeomTokens->focalLength,
            HdCamera::DirtyProjMatrix,
            HdCameraTokens->projectionMatrix,
            timeVaryingBits,
            false);
    }

    _IsVarying(prim,
        UsdGeomTokens->clippingPlanes,
        HdCamera::DirtyClipPlanes,
        HdCameraTokens->clipPlanes,
        timeVaryingBits,
        false);

    // If any of the physical params that affect the projection matrix are time
    // varying, we can flag the DirtyParams bit as varying and avoid querying
    // variability of the remaining params.
    if (*timeVaryingBits & HdCamera::DirtyProjMatrix) {
        *timeVaryingBits |= HdCamera::DirtyParams;
    } else {
        _IsVarying(prim,
            UsdGeomTokens->fStop,
            HdCamera::DirtyParams,
            HdCameraTokens->fStop,
            timeVaryingBits,
            false);
        if ((*timeVaryingBits & HdCamera::DirtyParams) == 0) {
            _IsVarying(prim,
                UsdGeomTokens->focusDistance,
                HdCamera::DirtyParams,
                HdCameraTokens->focusDistance,
                timeVaryingBits,
                false);
        }
        if ((*timeVaryingBits & HdCamera::DirtyParams) == 0) {
            _IsVarying(prim,
                UsdGeomTokens->shutterOpen,
                HdCamera::DirtyParams,
                HdCameraTokens->shutterOpen,
                timeVaryingBits,
                false);
        }
        if ((*timeVaryingBits & HdCamera::DirtyParams) == 0) {
            _IsVarying(prim,
                UsdGeomTokens->shutterClose,
                HdCamera::DirtyParams,
                HdCameraTokens->shutterClose,
                timeVaryingBits,
                false);
        }
        if ((*timeVaryingBits & HdCamera::DirtyParams) == 0) {
            _IsVarying(prim,
                UsdGeomTokens->exposure,
                HdCamera::DirtyParams,
                HdCameraTokens->exposure,
                timeVaryingBits,
                false);
        }
    }
}

void 
UsdImagingCameraAdapter::UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext) const
{
}

static
HdCamera::Projection
_ToProjection(const TfToken &token)
{
    if (token == UsdGeomTokens->orthographic) {
        return HdCamera::Orthographic;
    }

    if (token != UsdGeomTokens->perspective) {
        TF_WARN("Unknown projection type %s", token.GetText());
    }

    return HdCamera::Perspective;
}

static
std::vector<GfVec4d>
_ToGfVec4dVector(const VtArray<GfVec4f> &vec)
{
    std::vector<GfVec4d> result;
    result.reserve(vec.size());
    result.assign(vec.cbegin(), vec.cend());
    return result;
}

VtValue
UsdImagingCameraAdapter::Get(UsdPrim const& prim,
                             SdfPath const& cachePath,
                             TfToken const& key,
                             UsdTimeCode time,
                             VtIntArray *outIndices) const
{
    UsdGeomCamera cam(prim);
    if (!TF_VERIFY(cam)) {
        return VtValue();
    }

    if (key == HdCameraTokens->projection) {
        TfToken v;
        cam.GetProjectionAttr().Get(&v, time);
        return VtValue(_ToProjection(v));
    } else if (key == HdCameraTokens->horizontalAperture) {
        // The USD schema specifies several camera parameters in tenths of a
        // world unit (e.g., focalLength = 50mm)
        // Hydra's camera expects these parameters to be expressed in world
        // units. (e.g., if cm is the world unit, focalLength = 5cm)
        float v;
        cam.GetHorizontalApertureAttr().Get(&v, time);
        return VtValue(v * float(GfCamera::APERTURE_UNIT));
    } else if (key == HdCameraTokens->verticalAperture) { 
        float v;
        cam.GetVerticalApertureAttr().Get(&v, time);
        return VtValue(v * float(GfCamera::APERTURE_UNIT));
    } else if (key == HdCameraTokens->horizontalApertureOffset) {
        float v;
        cam.GetHorizontalApertureOffsetAttr().Get(&v, time);
        return VtValue(v * float(GfCamera::APERTURE_UNIT));
    } else if (key == HdCameraTokens->verticalApertureOffset) {
        float v;
        cam.GetVerticalApertureOffsetAttr().Get(&v, time);
        return VtValue(v * float(GfCamera::APERTURE_UNIT));
    } else if (key == HdCameraTokens->focalLength) {
        float v;
        cam.GetFocalLengthAttr().Get(&v, time);
        return VtValue(v * float(GfCamera::FOCAL_LENGTH_UNIT));
    } else if (key == HdCameraTokens->clippingRange) {
        GfVec2f v;
        cam.GetClippingRangeAttr().Get(&v, time);
        return VtValue(GfRange1f(v[0], v[1]));
    } else if (key == HdCameraTokens->clipPlanes) {
        VtArray<GfVec4f> v;
        cam.GetClippingPlanesAttr().Get(&v, time);
        return VtValue(_ToGfVec4dVector(v));
    } else if (key == HdCameraTokens->fStop) {
        VtValue v;
        cam.GetFStopAttr().Get(&v, time);
        return v;
    } else if (key == HdCameraTokens->focusDistance) {
        VtValue v;
        cam.GetFocusDistanceAttr().Get(&v, time);
        return v;
    } else if (key == HdCameraTokens->shutterOpen) {
        VtValue vShutterOpen;
        cam.GetShutterOpenAttr().Get(&vShutterOpen, time); // conversion n/a
        return vShutterOpen;
    } else if (key == HdCameraTokens->shutterClose) {
        VtValue vShutterClose;
        cam.GetShutterCloseAttr().Get(&vShutterClose, time); // conversion n/a
        return vShutterClose;
    } else if (key == HdCameraTokens->exposure) {
        VtValue v;
        cam.GetExposureAttr().Get(&v, time); // conversion n/a
        return v;
    }

    VtValue v;
    prim.GetAttribute(key).Get(&v, time);
    return v;
}

HdDirtyBits
UsdImagingCameraAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                      SdfPath const& cachePath, 
                                      TfToken const& propertyName)
{
    if (UsdGeomXformable::IsTransformationAffectedByAttrNamed(propertyName))
        return HdCamera::DirtyTransform;

    else if (propertyName == UsdGeomTokens->projection ||
             propertyName == UsdGeomTokens->horizontalAperture ||
             propertyName == UsdGeomTokens->verticalAperture ||
             propertyName == UsdGeomTokens->horizontalApertureOffset ||
             propertyName == UsdGeomTokens->verticalApertureOffset ||
             propertyName == UsdGeomTokens->clippingRange ||
             propertyName == UsdGeomTokens->focalLength) {
        return HdCamera::DirtyProjMatrix |
               HdCamera::DirtyParams;
    }

    else if (propertyName == UsdGeomTokens->clippingPlanes)
        return HdCamera::DirtyClipPlanes;

    else if (propertyName == UsdGeomTokens->fStop ||
             propertyName == UsdGeomTokens->focusDistance ||
             propertyName == UsdGeomTokens->shutterOpen ||
             propertyName == UsdGeomTokens->shutterClose ||
             propertyName == UsdGeomTokens->exposure)
        return HdCamera::DirtyParams;

    // XXX: There's no catch-all dirty bit for weird camera params.
    return HdChangeTracker::AllDirty;
}

void
UsdImagingCameraAdapter::MarkDirty(UsdPrim const& prim,
                                  SdfPath const& cachePath,
                                  HdDirtyBits dirty,
                                  UsdImagingIndexProxy* index)
{
    index->MarkSprimDirty(cachePath, dirty);
}

void
UsdImagingCameraAdapter::MarkTransformDirty(UsdPrim const& prim,
                                           SdfPath const& cachePath,
                                           UsdImagingIndexProxy* index)
{
    index->MarkSprimDirty(cachePath, HdCamera::DirtyViewMatrix);
}

void
UsdImagingCameraAdapter::MarkWindowPolicyDirty(UsdPrim const& prim,
                                               SdfPath const& cachePath,
                                               UsdImagingIndexProxy* index)
{
    // Since windowPolicy isn't authored in the schema, we require an explicit
    // way to propagate application window policy to the Hydra camera.
    index->MarkSprimDirty(cachePath, HdCamera::DirtyWindowPolicy);
}

void
UsdImagingCameraAdapter::_RemovePrim(SdfPath const& cachePath,
                                     UsdImagingIndexProxy* index)
{
    index->RemoveSprim(HdPrimTypeTokens->camera, cachePath);
}

PXR_NAMESPACE_CLOSE_SCOPE
