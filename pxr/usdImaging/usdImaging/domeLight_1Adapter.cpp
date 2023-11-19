//
// Copyright 2017 Pixar
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
#include "pxr/usdImaging/usdImaging/domeLight_1Adapter.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/light.h"
#include "pxr/imaging/hd/lightSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/usdLux/tokens.h"
#include "pxr/usd/usdLux/domeLight_1.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingDomeLight_1Adapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

namespace {

// Return a matrix that will align the given dome light with its "poleAxis".
GfMatrix4d
_GetDomeOffset(const UsdPrim& prim)
{
    GfMatrix4d offset(1.0);
    UsdLuxDomeLight_1 domeLight = UsdLuxDomeLight_1(prim);
    if (domeLight) {
        VtValue poleAxis;
        domeLight.GetPoleAxisAttr().Get(&poleAxis);
        static const GfRotation zupRot(GfVec3d(1.0, 0.0, 0.0), 90.0);
        if (poleAxis == UsdLuxTokens->scene) {
            TfToken stageUpAxis;
            if (prim.GetStage()->GetMetadata(
                    UsdGeomTokens->upAxis, &stageUpAxis)) {
                if (stageUpAxis == UsdGeomTokens->z) {
                    offset.SetRotate(zupRot);
                }
            }
        } else if (poleAxis == UsdLuxTokens->Z) {
            offset.SetRotate(zupRot);
        }
    }
    return offset;
}

// An HdTypedSampledDataSource that determines the dome offset matrix for its
// stored UsdPrim at the time its value is requested.
class _LazyDomeOffsetDataSource : public HdTypedSampledDataSource<GfMatrix4d>
{
public:
    HD_DECLARE_DATASOURCE(_LazyDomeOffsetDataSource);

    _LazyDomeOffsetDataSource(const UsdPrim& prim)
    : _prim(prim)
    {
    }

    VtValue GetValue(HdSampledDataSource::Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

    GfMatrix4d GetTypedValue(HdSampledDataSource::Time shutterOffset) override
    {
        return _GetDomeOffset(_prim);
    }

    bool GetContributingSampleTimesForInterval(
        HdSampledDataSource::Time startTime,
        HdSampledDataSource::Time endTime,
        std::vector<HdSampledDataSource::Time>* outSampleTimes) override
    {
        return false;
    }

private:
    UsdPrim _prim;
};

} // namespace anonymous

UsdImagingDomeLight_1Adapter::~UsdImagingDomeLight_1Adapter() 
{
}

TfTokenVector
UsdImagingDomeLight_1Adapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdImagingDomeLight_1Adapter::GetImagingSubprimType(
    UsdPrim const& prim, TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return HdPrimTypeTokens->domeLight;
    }

    return TfToken();
}

HdContainerDataSourceHandle
UsdImagingDomeLight_1Adapter::GetImagingSubprimData(
    UsdPrim const& prim,
    TfToken const& subprim,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    // Hydra 2.0 code path: Overlay the domeOffset onto the light adapter's
    // result.
    return HdOverlayContainerDataSource::New(
        HdRetainedContainerDataSource::New(
            HdLightSchema::GetSchemaToken(),
            HdRetainedContainerDataSource::New(
                HdLightTokens->domeOffset,
                _LazyDomeOffsetDataSource::New(prim))),
        UsdImagingLightAdapter::GetImagingSubprimData(
            prim, subprim, stageGlobals));
}

HdDataSourceLocatorSet
UsdImagingDomeLight_1Adapter::InvalidateImagingSubprim(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfTokenVector const& properties,
    UsdImagingPropertyInvalidationType invalidationType)
{
    HdDataSourceLocatorSet result =
        UsdImagingLightAdapter::InvalidateImagingSubprim(
            prim, subprim, properties, invalidationType);

    for (const TfToken &propertyName : properties) {
        if (propertyName == UsdLuxTokens->poleAxis) {
            result.insert(HdLightSchema::GetDefaultLocator());
        }
    }

    return result;
}

VtValue
UsdImagingDomeLight_1Adapter::GetLightParamValue(
    const UsdPrim& prim,
    const SdfPath& cachePath,
    const TfToken& paramName,
    UsdTimeCode time) const
{
    // Hydra 1.0 code path: Return the domeOffset explicitly, if requested.
    if (paramName == HdLightTokens->domeOffset) {
        return VtValue(_GetDomeOffset(prim));
    }
    return UsdImagingLightAdapter::GetLightParamValue(
        prim, cachePath, paramName, time);
}

bool
UsdImagingDomeLight_1Adapter::IsSupported(UsdImagingIndexProxy const* index) const
{
    return UsdImagingLightAdapter::IsEnabledSceneLights() &&
           index->IsSprimTypeSupported(HdPrimTypeTokens->domeLight);
}

SdfPath
UsdImagingDomeLight_1Adapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    return _AddSprim(HdPrimTypeTokens->domeLight, prim, index, instancerContext);
}

void
UsdImagingDomeLight_1Adapter::_RemovePrim(SdfPath const& cachePath,
                                          UsdImagingIndexProxy* index)
{
    _RemoveSprim(HdPrimTypeTokens->domeLight, cachePath, index);
}


PXR_NAMESPACE_CLOSE_SCOPE
