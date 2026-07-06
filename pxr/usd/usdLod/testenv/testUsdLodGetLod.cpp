//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/vec3f.h"

#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/relationship.h"

#include "pxr/usd/usdLod/distanceHeuristic.h"
#include "pxr/usd/usdLod/overrideAPI.h"
#include "pxr/usd/usdLod/rootAPI.h"
#include "pxr/usd/usdLod/screenSizeHeuristic.h"
#include "pxr/usd/usdLod/tokens.h"

#include "pxr/usd/usdGeom/tokens.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

// Magic values for convenient return from GetLODForPrim. These represent an
// error or an override that wants to show no LOD items or all LOD items.
constexpr float INVALID_LOD = -999.0;
constexpr float NO_LOD = -998.0;
constexpr float ALL_LOD = -997.0;


class GetLODTest {
public:
    GfFrustum frustum;
    GfMatrix4d transform = GfMatrix4d(1.0);
    UsdTimeCode time = UsdTimeCode::Default();

    bool testPassed = true;

public:
    float GetLODForPrim(const UsdPrim& prim,
                        const TfToken& targetDomain)
    {
        UsdLodRootAPI rootAPI(prim);
        if (!rootAPI) {
            return INVALID_LOD;
        }

        // ComputeLODOverride will walk up the hierarchy to search for an
        // override to inherit.
        float overrideIndex = INVALID_LOD;
        TfToken overrideMode =
            UsdLodOverrideAPI(prim).ComputeLODOverride(&overrideIndex);

        if (overrideMode != UsdLodTokens->noOverride) {
            if (overrideMode == UsdLodTokens->indexedLOD) {
                return overrideIndex;
            } else if (overrideMode == UsdLodTokens->noLOD) {
                return NO_LOD;
            } else if (overrideMode == UsdLodTokens->allLOD) {
                return ALL_LOD;
            } else {
                return INVALID_LOD;
            }
        }

        SdfPathVector targetPaths;
        UsdRelationship heuristicsRel = rootAPI.GetLodHeuristicsRel();
        heuristicsRel.GetForwardedTargets(&targetPaths);

        for (const auto& path : targetPaths) {
            const UsdPrim targetPrim = prim.GetStage()->GetPrimAtPath(path);
            UsdLodHeuristic heuristic(targetPrim);
            UsdAttribute lodDomainAttr = heuristic.GetLodDomainAttr();

            // We're only interested in targetDomain heuristics
            TfToken domain;
            if (!lodDomainAttr.Get(&domain, time) ||
                (domain != targetDomain))
            {
                // Not a targetDomain heuristic
                continue;
            }

            // Check for known heuristic types.
            if (auto distHeuristic = UsdLodDistanceHeuristic(heuristic)) {
                return distHeuristic.ComputeLOD(frustum.GetPosition(),
                                                transform,
                                                time);
            } else if (auto sizeHeuristic = UsdLodScreenSizeHeuristic(heuristic)) {
                return sizeHeuristic.ComputeLOD(frustum,
                                                transform,
                                                time);
            } else {
                // Unknown heuristic type!
                continue;
            }
        }

        UsdAttribute defaultIndex = rootAPI.GetLodDefaultIndexAttr();
        int index;
        if (defaultIndex.Get(&index, time)) {
            return float(index);
        }

        std::cerr << "Unable to find a LOD opinion." << std::endl;

        return INVALID_LOD;
    }

    bool CheckLOD(const UsdPrim& prim, float expected, const char* msg)
    {
        float lodIndex = GetLODForPrim(prim, UsdLodTokens->imaging);
        bool result = TF_VERIFY(lodIndex == expected,
                                "Failed %s: lod = %f, expected = %f",
                                msg, lodIndex, expected);

        if (result) {
            std::cout << "Passed "
                      << msg
                      << ": lod = "
                      << lodIndex
                      << std::endl;
        } else {
            testPassed = false;
        }

        return result;
    }

    bool RunTest()
    {
        testPassed = true;  // Modified by CheckLOD above.

        UsdStageRefPtr stage = UsdStage::CreateInMemory("foo.usda");
        UsdPrim world = stage->DefinePrim(SdfPath("/World"),
                                          UsdGeomTokens->Scope);
        UsdPrim rootPrim = stage->DefinePrim(SdfPath("/World/Root"),
                                             UsdGeomTokens->Scope);
        UsdLodRootAPI rootAPI = UsdLodRootAPI::Apply(rootPrim);

        UsdRelationship lodHeuristics = rootAPI.GetLodHeuristicsRel();

        // Set a default LOD and verify that it worked.
        rootAPI.GetLodDefaultIndexAttr().Set(2);
        CheckLOD(rootPrim, 2.0, "basic default test");

        // Define a heuristics scope and put something random in it.
        auto random = stage->DefinePrim(SdfPath("/World/Heuristics/random"),
                                        UsdGeomTokens->Scope);
        lodHeuristics.AddTarget(random.GetPath());

        // Still 2.0?
        CheckLOD(rootPrim, 2.0, "junk heuristic test");

        // Add real heuristics but using "audio" domain.
        auto dist = UsdLodDistanceHeuristic::Define(
            stage, SdfPath("/World/Heuristics/dist"));

        dist.GetLodDomainAttr().Set(UsdLodTokens->audio);
        dist.GetCenterAttr().Set(GfVec3f(0.0, 0.0, -5.0));
        dist.GetThresholdsAttr().Set(VtFloatArray({4.0, 10.0}));
        dist.GetBlendThresholdsAttr().Set(VtFloatArray({6.0, 12.0}));
        lodHeuristics.AddTarget(dist.GetPath());

        // Still 2.0?
        CheckLOD(rootPrim, 2.0, "audio distance heuristic");
        
        auto size = UsdLodScreenSizeHeuristic::Define(
            stage, SdfPath("/World/Heuristics/size"));

        size.GetLodDomainAttr().Set(UsdLodTokens->audio);
        GfRange3f extent(GfVec3f(0.0, 0.0, -4.0),
                         GfVec3f(2.0, 2.0, -2.0));
        size.GetExtentAttr().Set(VtArray({extent.GetMin(), extent.GetMax()}));
        size.GetThresholdsAttr().Set(VtFloatArray({.99, .30, .10}));
        size.GetBlendThresholdsAttr().Set(VtFloatArray({.75, .20,}));
        size.GetProjectionMethodAttr().Set(UsdLodTokens->projectedExtent);
        lodHeuristics.AddTarget(size.GetPath());

        // Still 2.0?
        CheckLOD(rootPrim, 2.0, "audio screen-size heuristic");

        // Swap the screen-size heuristic to the imaging domain
        size.GetLodDomainAttr().Set(UsdLodTokens->imaging);
        // Now we expect 1.5.
        CheckLOD(rootPrim, 1.5, "imaging screen-size heuristic");

        // Switch the frustum to orthographic. The screen size should now be 100%
        frustum.SetProjectionType(GfFrustum::Orthographic);
        CheckLOD(rootPrim, 0.0, "orthographic frustum");

        // Swap the distance heuristic to imaging as well. Our fake "renderer"
        // prefers the distance heuristic because it is the first targeted
        // imaging heuristic.
        dist.GetLodDomainAttr().Set(UsdLodTokens->imaging);
        // Now we expect 0.5 as it transitions from 0 to 1.
        CheckLOD(rootPrim, 0.5, "imaging distance heuristic");

        // Switch back to perspective so the return value is more verifiable.
        // (The value 0.0 is just too common to prove the code worked.)
        frustum.SetProjectionType(GfFrustum::Perspective);
        
        // Finally verify LODOverrideAPI
        auto overrideAPI = UsdLodOverrideAPI::Apply(world);
        overrideAPI.GetLodOverrideIndexAttr().Set(3.14159f);

        // Nothing mode has been authored so nothing should change.
        CheckLOD(rootPrim, 0.5, "empty override API");

        // Set the override mode to No LOD
        overrideAPI.GetLodOverrideModeAttr().Set(UsdLodTokens->noLOD);
        CheckLOD(rootPrim, NO_LOD, "override to noLOD");

        // Set the override mode to All LOD
        overrideAPI.GetLodOverrideModeAttr().Set(UsdLodTokens->allLOD);
        CheckLOD(rootPrim, ALL_LOD, "override to allLOD");

        // Set the override mode to Indexed
        overrideAPI.GetLodOverrideModeAttr().Set(UsdLodTokens->indexedLOD);
        CheckLOD(rootPrim, 3.14159, "override to indexed (3.14159)");

        // Set the override mode to inherited which does not override
        overrideAPI.GetLodOverrideModeAttr().Set(UsdLodTokens->inherited);
        CheckLOD(rootPrim, 0.5, "override to inherited");

        // Set it back to pi
        overrideAPI.GetLodOverrideModeAttr().Set(UsdLodTokens->indexedLOD);
        CheckLOD(rootPrim, 3.14159, "override to indexed again (3.14159)");

        // Remove the override
        world.RemoveAPI<UsdLodOverrideAPI>();
        CheckLOD(rootPrim, 0.5, "after remove override API");

        return testPassed;
    }

};

int main(int argc, char **argv)
{
    GetLODTest test;

    if (test.RunTest()) {
        std::cout << "Test Passed" << std::endl;
        return 0;
    }

    std::cout << "Test Failed" << std::endl;
    return 1;
}

