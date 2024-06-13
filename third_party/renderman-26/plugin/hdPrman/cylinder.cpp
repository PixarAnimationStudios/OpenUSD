//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/cylinder.h"

#include "hdPrman/renderParam.h"
#include "hdPrman/instancer.h"
#include "hdPrman/material.h"
#include "hdPrman/rixStrings.h"
#include "pxr/imaging/hd/cylinderSchema.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

#include "Riley.h"
#include "RiTypesHelper.h"
#include "RixShadingUtils.h"
#include "RixPredefinedStrings.hpp"

PXR_NAMESPACE_OPEN_SCOPE

HdPrman_Cylinder::HdPrman_Cylinder(SdfPath const& id)
    : BASE(id)
{
}

HdDirtyBits
HdPrman_Cylinder::GetInitialDirtyBitsMask() const
{
    constexpr int mask = HdChangeTracker::Clean
        | HdChangeTracker::DirtyTransform
        | HdChangeTracker::DirtyVisibility
        | HdChangeTracker::DirtyPrimvar
        | HdChangeTracker::DirtyMaterialId
        | HdChangeTracker::DirtyInstancer
        ;

    return (HdDirtyBits)mask;
}

TfTokenVector const &
HdPrman_Cylinder::GetBuiltinPrimvarNames() const
{
    static TfTokenVector result{
        HdCylinderSchemaTokens->height,
        HdCylinderSchemaTokens->radius };
    return result;
}

RtPrimVarList
HdPrman_Cylinder::_ConvertGeometry(HdPrman_RenderParam *renderParam,
                                   HdSceneDelegate *sceneDelegate,
                                   const SdfPath &id,
                                   RtUString *primType,
                                   std::vector<HdGeomSubset> *geomSubsets)
{
    RtPrimVarList primvars;

    *primType = RixStr.k_Ri_Cylinder;

    const float radius =
        sceneDelegate->Get(id, HdCylinderSchemaTokens->radius)
            .GetWithDefault<double>(0.0);
    const float height =
        sceneDelegate->Get(id, HdCylinderSchemaTokens->height)
            .GetWithDefault<double>(0.0);

    primvars.SetFloat(RixStr.k_Ri_radius, radius);
    primvars.SetFloat(RixStr.k_Ri_zmin, -0.5f * height);
    primvars.SetFloat(RixStr.k_Ri_zmax,  0.5f * height);

    HdPrman_ConvertPrimvars(sceneDelegate, id, primvars, 1, 0, 0, 0);
    return primvars;
}

PXR_NAMESPACE_CLOSE_SCOPE
