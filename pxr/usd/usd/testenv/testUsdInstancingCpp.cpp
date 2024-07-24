//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include <iostream>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

PXR_NAMESPACE_OPEN_SCOPE
const PcpPrimIndex &
Usd_PrimGetSourcePrimIndex(const UsdPrim& prim)
{
    return prim._GetSourcePrimIndex();
}
PXR_NAMESPACE_CLOSE_SCOPE

// Verify that an instancing prototype is not replaced with a new prototype
// when one of its non-source instances is deinstanced at the same time that
// entire stage is recomposed.
void
TestInstancing_1() 
{
    std::string rootLayer = "./rootLayer.usda";

    // determine what instance to unset
    UsdStageRefPtr stage = UsdStage::Open(rootLayer);

    const UsdPrim prototypePrim = 
        stage->GetPrimAtPath(SdfPath("/instancer1/Instance0")).GetPrototype();

    const SdfPath sourcePrimIndexPath = 
        Usd_PrimGetSourcePrimIndex(prototypePrim).GetRootNode().GetPath();
    
    const SdfPath& instancePathToUnset = 
        (sourcePrimIndexPath.GetName() == "Instance0") ? 
        SdfPath("/instancer1/Instance1") : 
        SdfPath("/instancer1/Instance0");

    SdfLayerRefPtr rootLayerPtr = stage->GetRootLayer();
    SdfLayerRefPtr subLayerPtr = 
        SdfLayer::FindOrOpen(rootLayerPtr->GetSubLayerPaths()[0]);

    SdfPrimSpecHandle instancePrimToUnset = 
        subLayerPtr->GetPrimAtPath(instancePathToUnset);

    SdfLayerRefPtr anonymousLayer = SdfLayer::CreateAnonymous(".usda");
    SdfCreatePrimInLayer(anonymousLayer, SdfPath("/dummy"));
    {
        SdfChangeBlock block;

        // unset instance
        instancePrimToUnset->SetInstanceable(false);
        // make a dummy change to sublayers - to trigger a significant change of
        // "/" - makes sure prototype are rebuild, since all prim indexes are
        // invalid and new ones are generated as part of this "/" change.
        rootLayerPtr->SetSubLayerPaths({subLayerPtr->GetIdentifier(), 
                anonymousLayer->GetIdentifier()});
    }

    const UsdPrim& newPrototypePrim = 
        stage->GetPrimAtPath(sourcePrimIndexPath).GetPrototype();

    const SdfPath newSourcePrimIndexPath =
        Usd_PrimGetSourcePrimIndex(newPrototypePrim).GetRootNode().GetPath();

    // Verify that the prototype UsdPrim's path and the path of its underlying
    // source prim index have not changed.
    TF_VERIFY(
        prototypePrim.GetPath() == newPrototypePrim.GetPath(),
        "prototypePrim.GetPath() = <%s>, newPrototypePrim.GetPath() = <%s>",
        prototypePrim.GetPath().GetText(),
        newPrototypePrim.GetPath().GetText());

    TF_VERIFY(
        sourcePrimIndexPath == newSourcePrimIndexPath,
        "sourcePrimIndexPath = <%s>, newSourcePrimIndexPath = <%s>",
        sourcePrimIndexPath.GetText(), newSourcePrimIndexPath.GetText());
}

// Verify that an instancing prototype is not replaced with a new prototype
// when one of its non-source instances is deinstanced at the same time
// that a parent prim of all of the instances is recomposed.
void
TestInstancing_2() 
{
    std::string rootLayer = "./secondRoot.usda";

    // determine which instance to update
    UsdStageRefPtr stage = UsdStage::Open(rootLayer);

    const UsdPrim prototypePrim =
        stage->GetPrimAtPath(SdfPath("/Ref1/instance1")).GetPrototype();

    const SdfPath sourcePrimIndexPath =
        Usd_PrimGetSourcePrimIndex(prototypePrim).GetRootNode().GetPath();

    const SdfPath& instancePathToUnset =
        (sourcePrimIndexPath.GetName() == "instance1") ?
        SdfPath("/Ref1/instance2") :
        SdfPath("/Ref1/instance1");

    SdfPrimSpecHandle ref1PrimSpec =
        stage->GetRootLayer()->GetPrimAtPath(SdfPath("/Ref1"));
    SdfPrimSpecHandle instancePrimToUnset =
        stage->GetRootLayer()->GetPrimAtPath(instancePathToUnset);

    SdfPrimSpecHandle dummyPrim = 
        SdfCreatePrimInLayer(stage->GetRootLayer(), SdfPath("/dummy"));
    SdfReference dummyReference("", dummyPrim->GetPath());

    // Test if a significant change in "/Ref1" triggers a rebuild of the
    // prototype since prior prim indexes for the source instance would have
    // been changed
    {
        SdfChangeBlock block;
        // unset instance
        instancePrimToUnset->SetInstanceable(false);

        // Add a reference to /ref1 to trigger a /ref1 change at pcp level
        ref1PrimSpec->GetReferenceList().Add(dummyReference);
    }

    const UsdPrim newPrototypePrim = 
        stage->GetPrimAtPath(sourcePrimIndexPath).GetPrototype();

    const SdfPath newSourcePrimIndexPath =
        Usd_PrimGetSourcePrimIndex(newPrototypePrim).GetRootNode().GetPath();

    // Verify that the prototype UsdPrim's path and the path of its underlying
    // source prim index have not changed.
    TF_VERIFY(
        prototypePrim.GetPath() == newPrototypePrim.GetPath(),
        "prototypePrim.GetPath() = <%s>, newPrototypePrim.GetPath() = <%s>",
        prototypePrim.GetPath().GetText(),
        newPrototypePrim.GetPath().GetText());

    TF_VERIFY(
        sourcePrimIndexPath == newSourcePrimIndexPath,
        "sourcePrimIndexPath = <%s>, newSourcePrimIndexPath = <%s>",
        sourcePrimIndexPath.GetText(), newSourcePrimIndexPath.GetText());
}

// Verify that an instancing prototype is not replaced with a new prototype
// when one of its non-source instances is deinstanced at the same time
// that a parent prim of other instances is recomposed.
void
TestInstancing_3()
{
    std::string rootLayer = "./thirdRoot.usda";

    // determine which instance to update
    UsdStageRefPtr stage = UsdStage::Open(rootLayer);

    const UsdPrim prototypePrim =
        stage->GetPrimAtPath(SdfPath("/Ref1/instance1")).GetPrototype();

    const SdfPath sourcePrimIndexPath =
        Usd_PrimGetSourcePrimIndex(prototypePrim).GetRootNode().GetPath();

    SdfPath parentPathToRecompose = SdfPath("/Ref2");
    if (sourcePrimIndexPath.HasPrefix(parentPathToRecompose)) {
        parentPathToRecompose = SdfPath("/Ref1");
    }

    SdfPrimSpecHandle refPrimSpec =
        stage->GetRootLayer()->GetPrimAtPath(parentPathToRecompose);

    SdfPrimSpecHandle dummyPrim = 
        SdfCreatePrimInLayer(stage->GetRootLayer(), SdfPath("/dummy"));
    SdfReference dummyReference("", dummyPrim->GetPath());

    {
        SdfChangeBlock block;
        // Add a reference to the parentPathToRecompose prim so as to trigger a
        // change in that prim, which does not hold sourceIndex for our
        // prototype
        refPrimSpec->GetReferenceList().Add(dummyReference);
    }

    const UsdPrim newPrototypePrim = 
        stage->GetPrimAtPath(sourcePrimIndexPath).GetPrototype();

    const SdfPath newSourcePrimIndexPath =
        Usd_PrimGetSourcePrimIndex(newPrototypePrim).GetRootNode().GetPath();

    // Verify that the prototype UsdPrim's path and the path of its underlying
    // source prim index have not changed.
    TF_VERIFY(
        prototypePrim.GetPath() == newPrototypePrim.GetPath(),
        "prototypePrim.GetPath() = <%s>, newPrototypePrim.GetPath() = <%s>",
        prototypePrim.GetPath().GetText(),
        newPrototypePrim.GetPath().GetText());

    TF_VERIFY(
        sourcePrimIndexPath == newSourcePrimIndexPath,
        "sourcePrimIndexPath = <%s>, newSourcePrimIndexPath = <%s>",
        sourcePrimIndexPath.GetText(), newSourcePrimIndexPath.GetText());
}

int main() 
{
    TestInstancing_1();
    TestInstancing_2();
    TestInstancing_3();
}
