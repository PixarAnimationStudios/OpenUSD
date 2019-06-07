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

    // Propeties that affect the projection matrix.
    // IMPORTANT: Calling _IsVarying will clear the specified bit if the given
    // attribute is _not_ varying.  Since we have multiple attributes that might
    // result in the bit being set, we need to be careful not to reset it.
    // Translation: only check _IsVarying for a given cause IFF the bit wasn't 
    // already set by a previous invocation.
    _IsVarying(prim,
        cam.GetHorizontalApertureAttr().GetBaseName(),
        HdCamera::DirtyProjMatrix,
        HdCameraTokens->projectionMatrix,
        timeVaryingBits,
        false);
    if ((*timeVaryingBits & HdCamera::DirtyProjMatrix) == 0) {
        _IsVarying(prim,
            cam.GetVerticalApertureAttr().GetBaseName(),
            HdCamera::DirtyProjMatrix,
            HdCameraTokens->projectionMatrix,
            timeVaryingBits,
            false);
    }
    if ((*timeVaryingBits & HdCamera::DirtyProjMatrix) == 0) {
        _IsVarying(prim,
            cam.GetHorizontalApertureOffsetAttr().GetBaseName(),
            HdCamera::DirtyProjMatrix,
            HdCameraTokens->projectionMatrix,
            timeVaryingBits,
            false);
    }
    if ((*timeVaryingBits & HdCamera::DirtyProjMatrix) == 0) {
        _IsVarying(prim,
            cam.GetVerticalApertureOffsetAttr().GetBaseName(),
            HdCamera::DirtyProjMatrix,
            HdCameraTokens->projectionMatrix,
            timeVaryingBits,
            false);
    }
    if ((*timeVaryingBits & HdCamera::DirtyProjMatrix) == 0) {
        _IsVarying(prim,
            cam.GetClippingRangeAttr().GetBaseName(),
            HdCamera::DirtyProjMatrix,
            HdCameraTokens->projectionMatrix,
            timeVaryingBits,
            false);
    }
    if ((*timeVaryingBits & HdCamera::DirtyProjMatrix) == 0) {
        _IsVarying(prim,
            cam.GetFocalLengthAttr().GetBaseName(),
            HdCamera::DirtyProjMatrix,
            HdCameraTokens->projectionMatrix,
            timeVaryingBits,
            false);
    }

    _IsVarying(prim,
        cam.GetClippingPlanesAttr().GetBaseName(),
        HdCamera::DirtyClipPlanes,
        HdCameraTokens->clipPlanes,
        timeVaryingBits,
        false);
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
    GfCamera gfCam = UsdGeomCamera(prim).GetCamera(time);
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
}

HdDirtyBits
UsdImagingCameraAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                      SdfPath const& cachePath, 
                                      TfToken const& propertyName)
{
    // Could be smarter, but there isn't much compute to save here.
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
