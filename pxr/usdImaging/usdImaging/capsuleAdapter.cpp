//
// Copyright 2016 Pixar
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
#include "pxr/usdImaging/usdImaging/capsuleAdapter.h"

#include "pxr/usdImaging/usdImaging/dataSourceImplicits-Impl.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/geomUtil/capsuleMeshGenerator.h"
#include "pxr/imaging/hd/capsuleSchema.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/usdGeom/capsule.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include "pxr/base/tf/type.h"

#include <cmath>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
using _PrimSource = UsdImagingDataSourceImplicitsPrim<UsdGeomCapsule, HdCapsuleSchema>;
}

TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingCapsuleAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingCapsuleAdapter::~UsdImagingCapsuleAdapter() 
{
}

TfTokenVector
UsdImagingCapsuleAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdImagingCapsuleAdapter::GetImagingSubprimType(
        UsdPrim const& prim,
        TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return HdPrimTypeTokens->capsule;
    }
    return TfToken();
}

HdContainerDataSourceHandle
UsdImagingCapsuleAdapter::GetImagingSubprimData(
        UsdPrim const& prim,
        TfToken const& subprim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (subprim.IsEmpty()) {
        return _PrimSource::New(
            prim.GetPath(),
            prim,
            stageGlobals);
    }
    return nullptr;
}

HdDataSourceLocatorSet
UsdImagingCapsuleAdapter::InvalidateImagingSubprim(
        UsdPrim const& prim,
        TfToken const& subprim,
        TfTokenVector const& properties,
        const UsdImagingPropertyInvalidationType invalidationType)
{
    if (subprim.IsEmpty()) {
        return _PrimSource::Invalidate(
            prim, subprim, properties, invalidationType);
    }
    
    return HdDataSourceLocatorSet();
}

bool
UsdImagingCapsuleAdapter::IsSupported(UsdImagingIndexProxy const* index) const
{
    return index->IsRprimTypeSupported(HdPrimTypeTokens->mesh);
}

SdfPath
UsdImagingCapsuleAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)

{
    return _AddRprim(HdPrimTypeTokens->mesh,
                     prim, index, GetMaterialUsdPath(prim), instancerContext);
}

void 
UsdImagingCapsuleAdapter::TrackVariability(UsdPrim const& prim,
                                          SdfPath const& cachePath,
                                          HdDirtyBits* timeVaryingBits,
                                          UsdImagingInstancerContext const* 
                                              instancerContext) const
{
    BaseAdapter::TrackVariability(
        prim, cachePath, timeVaryingBits, instancerContext);

    // Check DirtyPoints before doing variability checks, in case we can skip
    // any of them...
    if ((*timeVaryingBits & HdChangeTracker::DirtyPoints) == 0) {
        _IsVarying(prim, UsdGeomTokens->height,
                   HdChangeTracker::DirtyPoints,
                   UsdImagingTokens->usdVaryingPrimvar,
                   timeVaryingBits, /*inherited*/false);
    }
    if ((*timeVaryingBits & HdChangeTracker::DirtyPoints) == 0) {
        _IsVarying(prim, UsdGeomTokens->radius,
                   HdChangeTracker::DirtyPoints,
                   UsdImagingTokens->usdVaryingPrimvar,
                   timeVaryingBits, /*inherited*/false);
    }
    if ((*timeVaryingBits & HdChangeTracker::DirtyPoints) == 0) {
        _IsVarying(prim, UsdGeomTokens->axis,
                   HdChangeTracker::DirtyPoints,
                   UsdImagingTokens->usdVaryingPrimvar,
                   timeVaryingBits, /*inherited*/false);
    }
}

HdDirtyBits
UsdImagingCapsuleAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                                SdfPath const& cachePath,
                                                TfToken const& propertyName)
{
    if (propertyName == UsdGeomTokens->height ||
        propertyName == UsdGeomTokens->radius ||
        propertyName == UsdGeomTokens->axis) {
        return HdChangeTracker::DirtyPoints;
    }

    // Allow base class to handle change processing.
    return BaseAdapter::ProcessPropertyChange(prim, cachePath, propertyName);
}

/*virtual*/
VtValue
UsdImagingCapsuleAdapter::GetPoints(UsdPrim const& prim,
                                    UsdTimeCode time) const
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    UsdGeomCapsule capsule(prim);
    double height = 1.0;
    double radius = 0.5;
    TfToken axis = UsdGeomTokens->z;
    TF_VERIFY(capsule.GetHeightAttr().Get(&height, time));
    TF_VERIFY(capsule.GetRadiusAttr().Get(&radius, time));
    TF_VERIFY(capsule.GetAxisAttr().Get(&axis, time));

    // The capsule point generator computes points such that the "rings" of the
    // capsule lie on a plane parallel to the XY plane, with the Z-axis being
    // the "spine" of the capsule. These need to be transformed to the right
    // basis when a different spine axis is used.
    const GfMatrix4d basis = UsdImagingGprimAdapter::GetImplicitBasis(axis);

    const size_t numPoints =
        GeomUtilCapsuleMeshGenerator::ComputeNumPoints(numRadial, numCapAxial);

    VtVec3fArray points(numPoints);
        
    GeomUtilCapsuleMeshGenerator::GeneratePoints(
        points.begin(),
        numRadial,
        numCapAxial,
        radius,
        height,
        &basis
    );

    return VtValue(points);
}

/*virtual*/ 
VtValue
UsdImagingCapsuleAdapter::GetTopology(UsdPrim const& prim,
                                      SdfPath const& cachePath,
                                      UsdTimeCode time) const
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // All capsules share the same topology.
    static const HdMeshTopology topology =
        HdMeshTopology(GeomUtilCapsuleMeshGenerator::GenerateTopology(
                            numRadial, numCapAxial));

    return VtValue(topology);
}

PXR_NAMESPACE_CLOSE_SCOPE

