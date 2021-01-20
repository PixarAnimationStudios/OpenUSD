//
// Copyright 2020 Pixar
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

#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include <iostream>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

// Apply changes such that an instance not represening the sourcePrimIndex for a
// prototype is changed, and significant change is pseudoRoot (by adding a dummy
// sublayer to it)

PXR_NAMESPACE_OPEN_SCOPE
const PcpPrimIndex &
Usd_PrimGetSourcePrimIndex(const UsdPrim& prim)
{
    return prim._GetSourcePrimIndex();
}
PXR_NAMESPACE_CLOSE_SCOPE


/// Test to verify prototypes are appropriately changed when "/" is changed
void
TestInstancing_1() 
{
    std::string rootLayer = "./rootLayer.usda";

    // determine what instance to unset
    UsdStageRefPtr stage = UsdStage::Open(rootLayer);
    const UsdPrim& prototypePrim = 
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

    const PcpPrimIndex* origPrimIndexForSource =
        &(stage->GetPrimAtPath(sourcePrimIndexPath).GetPrimIndex());

    SdfLayerRefPtr anonymousLayer = SdfLayer::CreateAnonymous(".usda");
    SdfPrimSpecHandle dummpPrim = 
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

    const PcpPrimIndex* newPrimIndexForSource =
        &(stage->GetPrimAtPath(sourcePrimIndexPath).GetPrimIndex());

    // Prototype's sourcePrimIndexPath is unchanged, and primIndex for this
    // prototype's source index should have been recomputed, since "/" change
    // would have triggered a pcpIndexRecompute for everything
    TF_VERIFY((prototypePrim.GetPath() == newPrototypePrim.GetPath()) &&
               origPrimIndexForSource != newPrimIndexForSource);
}


/// Test to verify prototype for an instance is updated if its correspoding
/// sourcePrim is updated because of parent being recomposed.
void
TestInstancing_2() 
{
    std::string rootLayer = "./secondRoot.usda";

    // determine which instance to update
    UsdStageRefPtr stage = UsdStage::Open(rootLayer);
    const UsdPrim& prototypePrim =
        stage->GetPrimAtPath(SdfPath("/Ref1/instance1")).GetPrototype();

    const SdfPath sourcePrimIndexPath =
        Usd_PrimGetSourcePrimIndex(prototypePrim).GetRootNode().GetPath();

    const SdfPath& instancePathToUnset =
        (sourcePrimIndexPath.GetName() == "instance1") ?
        SdfPath("/Ref1/instance2") :
        SdfPath("/Ref1/instance1");

    const PcpPrimIndex* origPrimIndexForSource =
        &(stage->GetPrimAtPath(sourcePrimIndexPath).GetPrimIndex());

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

    const UsdPrim& newPrototypePrim = 
        stage->GetPrimAtPath(sourcePrimIndexPath).GetPrototype();

    const PcpPrimIndex* newPrimIndexForSource =
        &(stage->GetPrimAtPath(sourcePrimIndexPath).GetPrimIndex());

    // Prototype's sourcePrimIndexPath is unchanged, and primIndex for this
    // prototype's source index should have been recomputed, since "/Ref1"
    // change would have triggered a pcpIndexRecompute because of the added
    // reference
    TF_VERIFY((prototypePrim.GetPath() == newPrototypePrim.GetPath()) &&
               origPrimIndexForSource != newPrimIndexForSource);
}

/// Test to verify prototype is not updated when sourcePrim is corresponding to
/// this is not updated, but parent of other instances is changed.
void
TestInstancing_3()
{
    std::string rootLayer = "./thirdRoot.usda";

    // determine which instance to update
    UsdStageRefPtr stage = UsdStage::Open(rootLayer);
    const UsdPrim& prototypePrim =
        stage->GetPrimAtPath(SdfPath("/Ref1/instance1")).GetPrototype();

    const SdfPath& sourcePrimIndexPath =
        Usd_PrimGetSourcePrimIndex(prototypePrim).GetRootNode().GetPath();

    SdfPath parentPathToRecompose = SdfPath("/Ref2");
    if (sourcePrimIndexPath.HasPrefix(parentPathToRecompose)) {
        parentPathToRecompose = SdfPath("/Ref1");
    }

    const PcpPrimIndex* origPrimIndexForSource =
        &(stage->GetPrimAtPath(sourcePrimIndexPath).GetPrimIndex());

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

    const UsdPrim& newPrototypePrim = 
        stage->GetPrimAtPath(sourcePrimIndexPath).GetPrototype();

    const PcpPrimIndex* newPrimIndexForSource =
        &(stage->GetPrimAtPath(sourcePrimIndexPath).GetPrimIndex());

    // Prototype's sourcePrimIndexPath is unchanged, and primIndex for this
    // prototype's source index should be same since we triggered a change 
    // to the prim not containing our prototype's sourcePrim
    TF_VERIFY((prototypePrim.GetPath() == newPrototypePrim.GetPath()) &&
               origPrimIndexForSource == newPrimIndexForSource);
}

int main() 
{
    TestInstancing_1();
    TestInstancing_2();
    TestInstancing_3();
}
