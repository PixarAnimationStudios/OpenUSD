//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/sphere.h"

#include "hdPrman/renderParam.h"
#include "hdPrman/instancer.h"
#include "hdPrman/material.h"
#include "hdPrman/rixStrings.h"
#include "pxr/imaging/hd/sphereSchema.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

#include "Riley.h"
#include "RiTypesHelper.h"
#include "RixShadingUtils.h"
#include "RixPredefinedStrings.hpp"

PXR_NAMESPACE_OPEN_SCOPE

HdPrman_Sphere::HdPrman_Sphere(SdfPath const& id)
    : BASE(id)
{
}

HdDirtyBits
HdPrman_Sphere::GetInitialDirtyBitsMask() const
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
HdPrman_Sphere::GetBuiltinPrimvarNames() const
{
    static TfTokenVector result{
        HdSphereSchemaTokens->radius };
    return result;
}

RtPrimVarList
HdPrman_Sphere::_ConvertGeometry(HdPrman_RenderParam *renderParam,
                                   HdSceneDelegate *sceneDelegate,
                                   const SdfPath &id,
                                   RtUString *primType,
                                   std::vector<HdGeomSubset> *geomSubsets)
{
    RtPrimVarList primvars;

    *primType = RixStr.k_Ri_Sphere;

    const float radius =
        sceneDelegate->Get(id, HdSphereSchemaTokens->radius)
            .GetWithDefault<double>(0.0);

    primvars.SetFloat(RixStr.k_Ri_radius, radius);

    HdPrman_ConvertPrimvars(sceneDelegate, id, primvars, 1, 0, 0, 0);
    return primvars;
}

PXR_NAMESPACE_CLOSE_SCOPE
