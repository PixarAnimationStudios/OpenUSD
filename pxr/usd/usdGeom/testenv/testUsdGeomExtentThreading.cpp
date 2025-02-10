//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/work/loops.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/boundable.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdGeom/subset.h"

#include <iostream>
#include <mutex>

PXR_NAMESPACE_USING_DIRECTIVE


const SdfPathSet listOfPathsWithMaterials = {
    SdfPath("/World/Plane"),
};

void ComputeMaterialBindings(PXR_NS::UsdPrim prim)
{
    for (auto p = prim; !p.IsPseudoRoot(); p = p.GetParent())
    {
        PXR_NS::UsdRelationship relationship = p.GetRelationship(PXR_NS::UsdShadeTokens->materialBinding);
        if(relationship)
        {
            PXR_NS::SdfPathVector targetPaths;
            relationship.GetForwardedTargets(&targetPaths);
            if (targetPaths.empty() && listOfPathsWithMaterials.count(prim.GetPrimPath()) > 0)
            {
                static std::mutex mtx;
                std::scoped_lock lock(mtx);
                std::cerr << "Buggy relationship: " << relationship.GetPath() << "\n";
                std::cerr << "Relationship returned empty forwarded targets on " << p.GetPrimPath() << "\n";
                std::cerr << "Relationship is " << (relationship.IsValid() ? "valid" : "not valid") << "\n";
                std::exit(1);
            }
        }
    }
}

void TraverseNode(PXR_NS::UsdPrim prim)
{
    if (prim.IsA<PXR_NS::UsdGeomMesh>())
    {
        ComputeMaterialBindings(prim);
    }

    if (prim.IsA<PXR_NS::UsdGeomBoundable>())
    {
        PXR_NS::UsdGeomBoundable points(prim);
        PXR_NS::VtVec3fArray extent;
        points.ComputeExtent(PXR_NS::UsdTimeCode::Default(), &extent);
    }

    std::vector<PXR_NS::UsdPrim> filteredChildren;
    auto children = prim.GetChildren();
    for (const auto& childPrim : children)
    {
        // Cull materials and their child shaders.
        if (!childPrim.IsA<PXR_NS::UsdShadeMaterial>() && !childPrim.IsA<PXR_NS::UsdGeomSubset>())
        {
            filteredChildren.push_back(childPrim);
        }
    }

    PXR_NS::WorkParallelForN(filteredChildren.size(), [&](size_t start, size_t end) {
        for (size_t i = start; i < end; ++i)
        {
            TraverseNode(filteredChildren[i]);
        }
    });
}

void BuildTreeFrom(PXR_NS::UsdStageRefPtr stage)
{
    TraverseNode(stage->GetPseudoRoot());
}

void TestThreading()
{
    PXR_NS::UsdStageRefPtr stage = PXR_NS::UsdStage::Open("mesh.usda");
    BuildTreeFrom(stage);
}

int main()
{
    TestThreading();
}