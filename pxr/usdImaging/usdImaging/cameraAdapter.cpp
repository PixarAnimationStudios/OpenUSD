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

#include "pxr/base/gf/frustum.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingCameraAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingCameraAdapter::~UsdImagingCameraAdapter() 
{
}

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
        HdCamera::DirtyViewMatrix,
        UsdImagingTokens->usdVaryingXform,
        timeVaryingBits);

    // Properties that affect the projection matrix.
    // To get a small speed boost, if an attribute sets a dirty bit we skip
    // all subsequent checks for that dirty bit.
    _IsVarying(prim,
        UsdGeomTokens->horizontalAperture,
        HdCamera::DirtyProjMatrix,
        HdCameraTokens->projectionMatrix,
        timeVaryingBits,
        false);

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

VtValue
UsdImagingCameraAdapter::Get(UsdPrim const& prim,
                             SdfPath const& cachePath,
                             TfToken const& key,
                             UsdTimeCode time) const
{
    // Create a GfCamera object to help populate the value cache entries
    // pulled on by HdCamera during Sync.
    UsdGeomCamera cam(prim);
    if (!TF_VERIFY(cam)) {
        return VtValue();
    }

    GfCamera gfCam = cam.GetCamera(time);
    
    if (key == HdCameraTokens->worldToViewMatrix || 
        key == HdCameraTokens->projectionMatrix || 
        key == HdCameraTokens->clipPlanes ) {
        
        GfFrustum frustum = gfCam.GetFrustum();
        if (key == HdCameraTokens->worldToViewMatrix) {
            return VtValue(frustum.ComputeViewMatrix());
            
        } else if (key == HdCameraTokens->projectionMatrix) {
            return VtValue(frustum.ComputeProjectionMatrix());

        } else if (key == HdCameraTokens->clipPlanes) {
            std::vector<GfVec4f> const& fClipPlanes = gfCam.GetClippingPlanes();
            
            // Convert to use double (HdCamera & HdRenderPassState use doubles)
            std::vector<GfVec4d> dClipPlanes;
            if (!fClipPlanes.empty()) {
                dClipPlanes.reserve(fClipPlanes.size());
                for (GfVec4d const& fPlane : fClipPlanes) {
                    dClipPlanes.emplace_back(fPlane);
                }
            }
            return VtValue(dClipPlanes);
        }

    } else if (key == HdCameraTokens->horizontalAperture) {
        // The USD schema specifies several camera parameters in tenths of a
        // world unit (e.g., focalLength = 50mm)
        // Hydra's camera expects these parameters to be expressed in world
        // units. (e.g., if cm is the world unit, focalLength = 5cm)
        return VtValue(gfCam.GetHorizontalAperture() * 
            (float)GfCamera::APERTURE_UNIT);
    
    } else if (key == HdCameraTokens->verticalAperture) {
        return VtValue(gfCam.GetVerticalAperture() * 
            (float)GfCamera::APERTURE_UNIT);

    } else if (key == HdCameraTokens->horizontalApertureOffset) {
        return VtValue(gfCam.GetHorizontalApertureOffset() * 
            (float)GfCamera::APERTURE_UNIT);
        
    } else if (key == HdCameraTokens->verticalApertureOffset) {
        return VtValue(gfCam.GetVerticalApertureOffset() * 
            (float)GfCamera::APERTURE_UNIT);

    } else if (key == HdCameraTokens->focalLength) {
        return VtValue(gfCam.GetFocalLength() * 
            (float)GfCamera::FOCAL_LENGTH_UNIT);

    } else if (key == HdCameraTokens->clippingRange) {
        return VtValue(gfCam.GetClippingRange()); // in world units

    } else if (key == HdCameraTokens->fStop) {
        return VtValue(gfCam.GetFStop()); // lens aperture (conversion n/a)

    } else if (key == HdCameraTokens->focusDistance) {
        return VtValue(gfCam.GetFocusDistance()); // in world units

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
        return HdCamera::DirtyViewMatrix;

    else if (propertyName == UsdGeomTokens->horizontalAperture ||
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
