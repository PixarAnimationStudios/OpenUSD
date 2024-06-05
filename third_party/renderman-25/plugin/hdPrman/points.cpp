//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/points.h"

#include "hdPrman/renderParam.h"
#include "hdPrman/material.h"
#include "hdPrman/rixStrings.h"

#include "RiTypesHelper.h"

PXR_NAMESPACE_OPEN_SCOPE

#if PXR_VERSION > 2011
HdPrman_Points::HdPrman_Points(SdfPath const& id)
    : BASE(id)
#else
HdPrman_Points::HdPrman_Points(SdfPath const& id,
                               SdfPath const& instancerId)
    : BASE(id, instancerId)
#endif
{
}

HdDirtyBits
HdPrman_Points::GetInitialDirtyBitsMask() const
{
    // The initial dirty bits control what data is available on the first
    // run through _PopulateRtPoints(), so it should list every data item
    // that _PopluateRtPoints requests.
    int mask = HdChangeTracker::Clean
        | HdChangeTracker::DirtyPoints
        | HdChangeTracker::DirtyTransform
        | HdChangeTracker::DirtyVisibility
        | HdChangeTracker::DirtyPrimvar
        | HdChangeTracker::DirtyNormals
        | HdChangeTracker::DirtyWidths
        | HdChangeTracker::DirtyMaterialId
        | HdChangeTracker::DirtyInstancer
        ;

    return (HdDirtyBits)mask;
}

RtPrimVarList
HdPrman_Points::_ConvertGeometry(HdPrman_RenderParam *renderParam,
                                  HdSceneDelegate *sceneDelegate,
                                  const SdfPath &id,
                                  RtUString *primType,
                                  std::vector<HdGeomSubset> *geomSubsets)
{
    RtPrimVarList primvars;

    const size_t npoints =
        HdPrman_ConvertPointsPrimvarForPoints(
            sceneDelegate, id, renderParam->GetShutterInterval(), primvars);

    *primType = RixStr.k_Ri_Points;

    HdPrman_ConvertPrimvars(sceneDelegate, id, primvars, 1,
                            npoints, npoints, npoints);

    return primvars;
}

PXR_NAMESPACE_CLOSE_SCOPE
