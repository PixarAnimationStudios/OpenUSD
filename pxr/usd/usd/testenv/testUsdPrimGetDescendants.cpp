//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

static void
TestRangeEqualityOperators()
{
    const std::string layerFile = "test.usda";
    UsdStageRefPtr stage = UsdStage::Open(layerFile, UsdStage::LoadNone);
    TF_AXIOM(stage->GetPseudoRoot().GetAllDescendants());
    TF_AXIOM(!stage->GetPseudoRoot().GetAllDescendants().empty());

    // Test UsdPrimSubtreeRange equality operator
    TF_AXIOM(UsdPrimSubtreeRange() == UsdPrimSubtreeRange());
    TF_AXIOM(stage->GetPseudoRoot().GetAllDescendants() ==
             stage->GetPseudoRoot().GetAllDescendants());

    // Test UsdPrimSubtreeRange templated equality operator
    TF_AXIOM(UsdPrimSubtreeRange() == std::vector<UsdPrim>());
    TF_AXIOM(std::vector<UsdPrim>() == UsdPrimSubtreeRange());
    {
        const auto allDescendants = stage->GetPseudoRoot().GetAllDescendants();
        std::vector<UsdPrim> allPrims(
            std::begin(allDescendants),
            std::end(allDescendants)
        );
        TF_AXIOM(!allPrims.empty());
        TF_AXIOM(stage->GetPseudoRoot().GetAllDescendants() == allPrims);
        TF_AXIOM(allPrims == stage->GetPseudoRoot().GetAllDescendants());
    }

    // Test UsdPrimSubtreeRange inequality operator
    TF_AXIOM(stage->GetPseudoRoot().GetAllDescendants() !=
             stage->GetPseudoRoot().GetFilteredDescendants(UsdPrimIsModel));

    // Test UsdPrimSubtreeRange templated inequality operator
    TF_AXIOM(stage->GetPseudoRoot().GetAllDescendants() !=
             std::vector<UsdPrim>());
    TF_AXIOM(std::vector<UsdPrim>() !=
             stage->GetPseudoRoot().GetAllDescendants());
    {
        const auto allDescendants = stage->GetPseudoRoot().GetAllDescendants();
        std::vector<UsdPrim> allDescendantsPlusOne(
            std::begin(allDescendants),
            std::end(allDescendants)
        );
        allDescendantsPlusOne.push_back(UsdPrim());
        TF_AXIOM(allDescendantsPlusOne.size() > 1);
        TF_AXIOM(stage->GetPseudoRoot().GetAllDescendants() !=
                 allDescendantsPlusOne);
        TF_AXIOM(allDescendantsPlusOne !=
                 stage->GetPseudoRoot().GetAllDescendants());
    }
    {
        const auto allDescendants = stage->GetPseudoRoot().GetAllDescendants();
        std::vector<UsdPrim> allDescendantsMinusOne(
            std::begin(allDescendants),
            std::end(allDescendants)
        );
        TF_AXIOM(!allDescendantsMinusOne.empty());
        allDescendantsMinusOne.pop_back();
        TF_AXIOM(stage->GetPseudoRoot().GetAllDescendants() !=
                 allDescendantsMinusOne);
        TF_AXIOM(allDescendantsMinusOne !=
                 stage->GetPseudoRoot().GetAllDescendants());
    }
    {
        const auto allDescendants = stage->GetPseudoRoot().GetAllDescendants();
        std::vector<UsdPrim> allDescendantsBackReplaced(
            std::begin(allDescendants),
            std::end(allDescendants)
        );
        TF_AXIOM(!allDescendantsBackReplaced.empty());
        TF_AXIOM(allDescendantsBackReplaced.back() != UsdPrim());
        allDescendantsBackReplaced.back() = UsdPrim();
        TF_AXIOM(stage->GetPseudoRoot().GetAllDescendants() !=
                 allDescendantsBackReplaced);
        TF_AXIOM(allDescendantsBackReplaced !=
                 stage->GetPseudoRoot().GetAllDescendants());
    }
}

static void
TestGetDescendants()
{
    std::string layerFile = "test.usda";
    UsdStageRefPtr stage = UsdStage::Open(layerFile, UsdStage::LoadNone);
    if (!stage) {
        TF_FATAL_ERROR("Failed to load stage for @%s@", layerFile.c_str());
    }

    UsdPrim 
        root = stage->GetPrimAtPath(SdfPath("/")),
        globalClass = stage->GetPrimAtPath(SdfPath("/GlobalClass")),
        abstractSubscope =
            stage->GetPrimAtPath(SdfPath("/GlobalClass/AbstractSubscope")),
        abstractOver = 
            stage->GetPrimAtPath(SdfPath("/GlobalClass/AbstractOver")),
        pureOver = stage->GetPrimAtPath(SdfPath("/PureOver")),
        undefSubscope = 
            stage->GetPrimAtPath(SdfPath("/PureOver/UndefinedSubscope")),
        group = stage->GetPrimAtPath(SdfPath("/Group")),
        modelChild = stage->GetPrimAtPath(SdfPath("/Group/ModelChild")),
        localChild = stage->GetPrimAtPath(SdfPath("/Group/LocalChild")),
        undefModelChild = 
            stage->GetPrimAtPath(SdfPath("/Group/UndefinedModelChild")),
        deactivatedScope = 
            stage->GetPrimAtPath(SdfPath("/Group/DeactivatedScope")),
        deactivatedModel = 
            stage->GetPrimAtPath(SdfPath("/Group/DeactivatedModel")),
        deactivatedOver = 
            stage->GetPrimAtPath(SdfPath("/Group/DeactivatedOver")),
        propertyOrder = stage->GetPrimAtPath(SdfPath("/PropertyOrder"));

    // Check filtered descendant access.
    TF_AXIOM((
        root.GetAllDescendants() ==
        std::vector<UsdPrim>{
            globalClass, abstractSubscope, abstractOver,
            pureOver, undefSubscope,
            group, modelChild, localChild, undefModelChild, 
            deactivatedScope, deactivatedModel, deactivatedOver,
            propertyOrder}));

    // Manually construct the "normal" view.
    TF_AXIOM((
        root.GetFilteredDescendants(
            UsdPrimIsActive && UsdPrimIsLoaded &&
            UsdPrimIsDefined && !UsdPrimIsAbstract) ==
        std::vector<UsdPrim>{propertyOrder}));

    // Only abstract prims.
    TF_AXIOM((
        root.GetFilteredDescendants(UsdPrimIsAbstract) ==
        std::vector<UsdPrim>{globalClass, abstractSubscope, abstractOver}));

    // Abstract & defined prims
    TF_AXIOM((
        root.GetFilteredDescendants(UsdPrimIsAbstract && UsdPrimIsDefined) ==
        std::vector<UsdPrim>{globalClass, abstractSubscope}));

    // Abstract | unloaded prims
    TF_AXIOM((
        root.GetFilteredDescendants(UsdPrimIsAbstract || !UsdPrimIsLoaded) ==
        std::vector<UsdPrim>{
            globalClass, abstractSubscope, abstractOver, 
            group, modelChild, localChild, undefModelChild, 
            deactivatedScope, deactivatedModel, deactivatedOver}));

    // Models only.
    TF_AXIOM((
        root.GetFilteredDescendants(UsdPrimIsModel) ==
        std::vector<UsdPrim>{group, modelChild, deactivatedModel}));

    // Non-models only.
    TF_AXIOM((
        root.GetFilteredDescendants(!UsdPrimIsModel) ==
        std::vector<UsdPrim>{
            globalClass, abstractSubscope, abstractOver, 
            pureOver, undefSubscope, propertyOrder}));

    // Models or undefined.
    TF_AXIOM((
        root.GetFilteredDescendants(UsdPrimIsModel || !UsdPrimIsDefined) ==
        std::vector<UsdPrim>{
            pureOver, undefSubscope, group, modelChild, undefModelChild,
            deactivatedModel, deactivatedOver}));
}

static void
TestGetDescendantsAsInstanceProxies()
{
    std::string layerFile = "nested/root.usda";
    UsdStageRefPtr stage = UsdStage::Open(layerFile);
    if (!stage) {
        TF_FATAL_ERROR("Failed to load stage for @%s@", layerFile.c_str());
    }

    auto Prim = [stage](const std::string& primPath) {
        return stage->GetPrimAtPath(SdfPath(primPath));
    };

    UsdPrim root = stage->GetPseudoRoot();

    TF_AXIOM((
        Prim("/")
            .GetFilteredDescendants(UsdTraverseInstanceProxies()) == 
        std::vector<UsdPrim>{
            Prim("/World"),
            Prim("/World/sets"),
            Prim("/World/sets/Set_1"),
            Prim("/World/sets/Set_1/Prop_1"),
            Prim("/World/sets/Set_1/Prop_1/geom"),
            Prim("/World/sets/Set_1/Prop_1/anim"),
            Prim("/World/sets/Set_1/Prop_2"),
            Prim("/World/sets/Set_1/Prop_2/geom"),
            Prim("/World/sets/Set_1/Prop_2/anim"),
            Prim("/World/sets/Set_2"),
            Prim("/World/sets/Set_2/Prop_1"),
            Prim("/World/sets/Set_2/Prop_1/geom"),
            Prim("/World/sets/Set_2/Prop_1/anim"),
            Prim("/World/sets/Set_2/Prop_2"),
            Prim("/World/sets/Set_2/Prop_2/geom"),
            Prim("/World/sets/Set_2/Prop_2/anim")}));

    TF_AXIOM((
        Prim("/World/sets/Set_1")
            .GetFilteredDescendants(UsdTraverseInstanceProxies()) == 
        std::vector<UsdPrim>{
            Prim("/World/sets/Set_1/Prop_1"),
            Prim("/World/sets/Set_1/Prop_1/geom"),
            Prim("/World/sets/Set_1/Prop_1/anim"),
            Prim("/World/sets/Set_1/Prop_2"),
            Prim("/World/sets/Set_1/Prop_2/geom"),
            Prim("/World/sets/Set_1/Prop_2/anim")}));

    TF_AXIOM((
        Prim("/World/sets/Set_1/Prop_1")
            .GetFilteredDescendants(UsdTraverseInstanceProxies()) == 
        std::vector<UsdPrim>{
            Prim("/World/sets/Set_1/Prop_1/geom"),
            Prim("/World/sets/Set_1/Prop_1/anim")}));

    TF_AXIOM((
        Prim("/World/sets/Set_1/Prop_2")
            .GetFilteredDescendants(UsdTraverseInstanceProxies()) == 
        std::vector<UsdPrim>{
            Prim("/World/sets/Set_1/Prop_2/geom"),
            Prim("/World/sets/Set_1/Prop_2/anim")}));

    TF_AXIOM((
        Prim("/World/sets/Set_2")
            .GetFilteredDescendants(UsdTraverseInstanceProxies()) == 
        std::vector<UsdPrim>{
            Prim("/World/sets/Set_2/Prop_1"),
            Prim("/World/sets/Set_2/Prop_1/geom"),
            Prim("/World/sets/Set_2/Prop_1/anim"),
            Prim("/World/sets/Set_2/Prop_2"),
            Prim("/World/sets/Set_2/Prop_2/geom"),
            Prim("/World/sets/Set_2/Prop_2/anim")}));

    TF_AXIOM((
        Prim("/World/sets/Set_1/Prop_1")
            .GetFilteredDescendants(UsdTraverseInstanceProxies()) == 
        std::vector<UsdPrim>{
            Prim("/World/sets/Set_1/Prop_1/geom"),
            Prim("/World/sets/Set_1/Prop_1/anim")}));

    TF_AXIOM((
        Prim("/World/sets/Set_1/Prop_2")
            .GetFilteredDescendants(UsdTraverseInstanceProxies()) == 
        std::vector<UsdPrim>{
            Prim("/World/sets/Set_1/Prop_2/geom"),
            Prim("/World/sets/Set_1/Prop_2/anim")}));

    TF_AXIOM((
        Prim("/World/sets/Set_2/Prop_1")
            .GetFilteredDescendants(UsdTraverseInstanceProxies()) == 
        std::vector<UsdPrim>{
            Prim("/World/sets/Set_2/Prop_1/geom"),
            Prim("/World/sets/Set_2/Prop_1/anim")}));

    TF_AXIOM((
        Prim("/World/sets/Set_2/Prop_2")
            .GetFilteredDescendants(UsdTraverseInstanceProxies()) == 
        std::vector<UsdPrim>{
            Prim("/World/sets/Set_2/Prop_2/geom"),
            Prim("/World/sets/Set_2/Prop_2/anim")}));

    // On instance proxies, UsdTraverseInstanceProxies is not required.
    TF_AXIOM((
        Prim("/World/sets/Set_1/Prop_1").GetDescendants() == 
        std::vector<UsdPrim>{
            Prim("/World/sets/Set_1/Prop_1/geom"),
            Prim("/World/sets/Set_1/Prop_1/anim")}));

    TF_AXIOM((
        Prim("/World/sets/Set_1/Prop_2").GetDescendants() == 
        std::vector<UsdPrim>{
            Prim("/World/sets/Set_1/Prop_2/geom"),
            Prim("/World/sets/Set_1/Prop_2/anim")}));

    TF_AXIOM((
        Prim("/World/sets/Set_2/Prop_1").GetDescendants() == 
        std::vector<UsdPrim>{
            Prim("/World/sets/Set_2/Prop_1/geom"),
            Prim("/World/sets/Set_2/Prop_1/anim")}));

    TF_AXIOM((
        Prim("/World/sets/Set_2/Prop_2").GetDescendants() == 
        std::vector<UsdPrim>{
            Prim("/World/sets/Set_2/Prop_2/geom"),
            Prim("/World/sets/Set_2/Prop_2/anim")}));
}

int 
main(int argc, char** argv)
{
    TestRangeEqualityOperators();
    TestGetDescendants();
    TestGetDescendantsAsInstanceProxies();

    std::cout << "OK\n";
    return EXIT_SUCCESS;
}

