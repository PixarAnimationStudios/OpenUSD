//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/cameraAdapter.h"

#include "pxr/usdImaging/usdImaging/dataSourceCamera.h"
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

TfTokenVector
UsdImagingCameraAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdImagingCameraAdapter::GetImagingSubprimType(
        UsdPrim const& prim,
        TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return HdPrimTypeTokens->camera;
    }
    return TfToken();
}

HdContainerDataSourceHandle
UsdImagingCameraAdapter::GetImagingSubprimData(
        UsdPrim const& prim,
        TfToken const& subprim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (subprim.IsEmpty()) {
        return UsdImagingDataSourceCameraPrim::New(
            prim.GetPath(),
            prim,
            stageGlobals);
    }
    return nullptr;
}

HdDataSourceLocatorSet
UsdImagingCameraAdapter::InvalidateImagingSubprim(
        UsdPrim const& prim,
        TfToken const& subprim,
        TfTokenVector const& properties,
        const UsdImagingPropertyInvalidationType invalidationType)
{
    if (subprim.IsEmpty()) {
        return UsdImagingDataSourceCameraPrim::Invalidate(
            prim, subprim, properties, invalidationType);
    }

    return HdDataSourceLocatorSet();
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
                        HdCamera::DirtyTransform,
                        UsdImagingTokens->usdVaryingXform,
                        timeVaryingBits);

    _IsVarying(prim,
               UsdGeomTokens->clippingPlanes,
               HdCamera::DirtyClipPlanes,
               HdCameraTokens->clipPlanes,
               timeVaryingBits,
               false);

    // If any other camera attributes are time varying
    // we will assume all camera params are time-varying.
    const std::vector<UsdAttribute> &attrs = prim.GetAttributes();
    for (UsdAttribute const& attr : attrs) {
        // Don't double-count clipping-plane or transform attrs.
        if (attr.GetBaseName() == UsdGeomTokens->clippingPlanes) { continue; }
        if (UsdGeomXformable::IsTransformationAffectedByAttrNamed(
                attr.GetName())) {
            continue;
        }
        if (_IsVarying(prim,
                       attr.GetName(),
                       HdCamera::DirtyParams,
                       attr.GetName(),
                       timeVaryingBits,
                       false)) {
            return;
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
        // The raw exponential compensation attribute.
        // See "linearExposureScale" below for the computed linear multiplier.
        VtValue vExposureExponent;
        cam.GetExposureAttr().Get(&vExposureExponent, time); // conversion n/a
        return vExposureExponent;
    } else if (key == HdCameraTokens->exposureTime) {
        VtValue vExposureTime;
        cam.GetExposureTimeAttr().Get(&vExposureTime, time); // conversion n/a
        return vExposureTime;
    } else if (key == HdCameraTokens->exposureIso) {
        VtValue vExposureIso;
        cam.GetExposureIsoAttr().Get(&vExposureIso, time); // conversion n/a
        return vExposureIso;
    } else if (key == HdCameraTokens->exposureFStop) {
        VtValue vExposureFStop;
        cam.GetExposureFStopAttr().Get(&vExposureFStop, time); // conversion n/a
        return vExposureFStop;
    } else if (key == HdCameraTokens->exposureResponsivity) {
        VtValue vExposureResponsivity;
        cam.GetExposureResponsivityAttr().Get(&vExposureResponsivity, time); // conversion n/a
        return vExposureResponsivity;
    } else if (key == HdCameraTokens->linearExposureScale) {
        // The computed linear exposure multiplier.
        return VtValue(cam.ComputeLinearExposureScale(time));
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

    if (propertyName == UsdGeomTokens->clippingPlanes)

        return HdCamera::DirtyClipPlanes;

    if (propertyName == UsdGeomTokens->projection ||
        propertyName == UsdGeomTokens->horizontalAperture ||
        propertyName == UsdGeomTokens->verticalAperture ||
        propertyName == UsdGeomTokens->horizontalApertureOffset ||
        propertyName == UsdGeomTokens->verticalApertureOffset ||
        propertyName == UsdGeomTokens->focalLength ||
        propertyName == UsdGeomTokens->clippingRange ||
        propertyName == UsdGeomTokens->fStop ||
        propertyName == UsdGeomTokens->focusDistance ||
        propertyName == UsdGeomTokens->shutterOpen ||
        propertyName == UsdGeomTokens->shutterClose ||
        propertyName == UsdGeomTokens->exposure ||
        propertyName == UsdGeomTokens->exposureTime ||
        propertyName == UsdGeomTokens->exposureIso ||
        propertyName == UsdGeomTokens->exposureFStop ||
        propertyName == UsdGeomTokens->exposureResponsivity)
        
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
    index->MarkSprimDirty(cachePath, HdCamera::DirtyTransform);
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
