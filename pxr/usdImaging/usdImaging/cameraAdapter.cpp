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
    // Discover time-varying transforms.
    _IsTransformVarying(prim,
        HdCamera::DirtyViewMatrix,
        UsdImagingTokens->usdVaryingXform,
        timeVaryingBits);

    UsdGeomCamera cam(prim);
    if (!TF_VERIFY(prim)) {
        return;
    }

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
    }

    // In addition to variability tracking, populate the camera tokens for which
    // values are going to always be added to the value cache,
    // This lets us avoid redundant lookups in UpdateForTime for the
    // corresponding dirty bits.
    // This is populated purely to aid prim entry deletion.
    UsdImagingValueCache* valueCache = _GetValueCache();
    TfTokenVector& cameraParams = valueCache->GetCameraParamNames(cachePath);
    cameraParams = {HdCameraTokens->worldToViewMatrix,
                    HdCameraTokens->projectionMatrix,
                    HdCameraTokens->clipPlanes,
                    HdCameraTokens->windowPolicy};
}

void 
UsdImagingCameraAdapter::UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext) const
{
    UsdImagingValueCache* valueCache = _GetValueCache();

    // Note: UsdGeomCamera does not specify a windowPolicy; we handle dirtyness
    // propagation via the MarkWindowPolicy adapter API, and leave it to the
    // UsdImagingDelegate to return the policy.
    if (requestedBits == HdCamera::Clean || 
        requestedBits == HdCamera::DirtyWindowPolicy) {
        return;
    }
    // Create a GfCamera object to help populate the value cache entries
    // pulled on by HdCamera during Sync.
    UsdGeomCamera cam(prim);
    GfCamera gfCam = cam.GetCamera(time);
    GfFrustum frustum = gfCam.GetFrustum();
    
    if (requestedBits & HdCamera::DirtyViewMatrix) {
        valueCache->GetCameraParam(cachePath, HdCameraTokens->worldToViewMatrix)
            = frustum.ComputeViewMatrix();
    }
    if (requestedBits & HdCamera::DirtyProjMatrix) {
        valueCache->GetCameraParam(cachePath, HdCameraTokens->projectionMatrix)
            = frustum.ComputeProjectionMatrix();
    }
    if (requestedBits & HdCamera::DirtyClipPlanes) {
        std::vector<GfVec4f> const& fClipPlanes = gfCam.GetClippingPlanes();
        // Convert to use double (HdCamera and HdRenderPassState use doubles)
        std::vector<GfVec4d> dClipPlanes;
        if (!fClipPlanes.empty()) {
            dClipPlanes.reserve(fClipPlanes.size());
            for (GfVec4d const& fPlane : fClipPlanes) {
                dClipPlanes.emplace_back(fPlane);
            }
        }
        valueCache->GetCameraParam(cachePath, HdCameraTokens->clipPlanes)
            = dClipPlanes;
    }
    if (requestedBits & HdCamera::DirtyParams) {
        // The USD schema specifies several camera parameters in tenths of a
        // world unit (e.g., focalLength = 50mm)
        // Hydra's camera expects these parameters to be expressed in world
        // units. (e.g., if cm is the world unit, focalLength = 5cm)
        valueCache->GetCameraParam(cachePath,
            HdCameraTokens->horizontalAperture)
                = gfCam.GetHorizontalAperture() / 10.0f;
        
        valueCache->GetCameraParam(cachePath,
            HdCameraTokens->verticalAperture)
                = gfCam.GetVerticalAperture() / 10.0f;
        
        valueCache->GetCameraParam(cachePath,
            HdCameraTokens->horizontalApertureOffset)
                = gfCam.GetHorizontalApertureOffset() / 10.0f;
        
        valueCache->GetCameraParam(cachePath,
            HdCameraTokens->verticalApertureOffset)
                = gfCam.GetVerticalApertureOffset() / 10.0f;
        
        valueCache->GetCameraParam(cachePath, HdCameraTokens->focalLength)
            = gfCam.GetFocalLength() / 10.0f;
        
        valueCache->GetCameraParam(cachePath, HdCameraTokens->clippingRange)
            = gfCam.GetClippingRange(); // in world units
        
        valueCache->GetCameraParam(cachePath, HdCameraTokens->fStop)
            = gfCam.GetFStop(); // lens aperture (conversion n/a)
        
        valueCache->GetCameraParam(cachePath, HdCameraTokens->focusDistance)
            = gfCam.GetFocusDistance(); // in world units
        
        VtValue& vShutterOpen =
            valueCache->GetCameraParam(cachePath, HdCameraTokens->shutterOpen);
        cam.GetShutterOpenAttr().Get(&vShutterOpen, time); // conversion n/a
        
        VtValue& vShutterClose =
            valueCache->GetCameraParam(cachePath, HdCameraTokens->shutterClose);
        cam.GetShutterCloseAttr().Get(&vShutterClose, time); // conversion n/a

        // Add all the above params to the list of camera params for which
        // value cache entries have been populated. Test for one, and push
        // all of them if it doesn't exist.
        // This is populated purely to aid prim entry deletion.
        TfTokenVector& cameraParams = valueCache->GetCameraParamNames(cachePath);
        if (std::find(cameraParams.begin(), cameraParams.end(),
                      HdCameraTokens->horizontalAperture) == 
            cameraParams.end()) {

            TfTokenVector params = {
                HdCameraTokens->horizontalAperture,
                HdCameraTokens->verticalAperture,
                HdCameraTokens->horizontalApertureOffset,
                HdCameraTokens->verticalApertureOffset,
                HdCameraTokens->focalLength,
                HdCameraTokens->clippingRange,
                HdCameraTokens->fStop,
                HdCameraTokens->focusDistance,
                HdCameraTokens->shutterOpen,
                HdCameraTokens->shutterClose
            };

            std::copy(params.begin(), params.end(),
                      std::back_inserter(cameraParams));
        }
    }
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
             propertyName == UsdGeomTokens->shutterClose)
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
