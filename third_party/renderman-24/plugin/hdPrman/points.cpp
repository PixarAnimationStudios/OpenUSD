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
#include "hdPrman/points.h"

#include "hdPrman/renderParam.h"
#include "hdPrman/instancer.h"
#include "hdPrman/material.h"
#include "hdPrman/rixStrings.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

#include "Riley.h"
#include "RiTypesHelper.h"
#include "RixShadingUtils.h"
#include "RixPredefinedStrings.hpp"

PXR_NAMESPACE_OPEN_SCOPE

HdPrman_Points::HdPrman_Points(SdfPath const& id)
    : BASE(id)
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
        HdPrman_ConvertPointsPrimvarForPoints(sceneDelegate, id, primvars);

    *primType = RixStr.k_Ri_Points;

    HdPrman_ConvertPrimvars(sceneDelegate, id, primvars, 1,
                            npoints, npoints, npoints);
    return primvars;
}

PXR_NAMESPACE_CLOSE_SCOPE
