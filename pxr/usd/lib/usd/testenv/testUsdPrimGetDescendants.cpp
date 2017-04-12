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

#include "pxr/pxr.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

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
    TestGetDescendants();
    TestGetDescendantsAsInstanceProxies();

    std::cout << "OK\n";
    return EXIT_SUCCESS;
}

