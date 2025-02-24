//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImaging/unitTestHelper.h"

#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/unitTestNullRenderDelegate.h"

#include "pxr/usd/usd/stage.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static void
SwitchBoundMaterialTest()
{
    std::cout << "-------------------------------------------------------\n";
    std::cout << "SwitchBoundMaterialTest\n";
    std::cout << "-------------------------------------------------------\n";

    const std::string usdPath = "switchBoundMaterial/model.usda";
    UsdStageRefPtr stage = UsdStage::Open(usdPath);
    TF_AXIOM(stage);
    
    // Bring up Hydra
    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex>
        renderIndex(HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    auto delegate = std::make_unique<UsdImagingDelegate>(renderIndex.get(),
                               SdfPath::AbsoluteRootPath());
    delegate->Populate(stage->GetPseudoRoot());
    delegate->ApplyPendingUpdates();
    delegate->SyncAll(true);
    
    // Clean the dirty bits
    HdChangeTracker& tracker = renderIndex->GetChangeTracker();
    tracker.MarkRprimClean(SdfPath("/Root/Geometry/box1"));
    tracker.MarkRprimClean(SdfPath("/Root/Geometry/box2"));

    // Switch the material for box1
    auto box1Prim = stage->GetPrimAtPath(SdfPath("/Root/Geometry/box1"));
    TF_AXIOM(box1Prim);
    auto materialBinding = box1Prim.GetRelationship(UsdShadeTokens->materialBinding);
    TF_AXIOM(materialBinding);
    materialBinding.SetTargets({SdfPath("/Root/Looks/green")});

    delegate->ApplyPendingUpdates();
    delegate->SyncAll(true);
    
    // Check that the dirty bits are clean for box2
    auto dirtyBits = tracker.GetRprimDirtyBits(SdfPath("/Root/Geometry/box2"));
    TF_AXIOM(dirtyBits == HdChangeTracker::Clean);

    // Switch the material on the neck
    auto box2Prim = stage->GetPrimAtPath(SdfPath("/Root/Geometry/box2"));
    TF_AXIOM(box2Prim);
    materialBinding = box2Prim.GetRelationship(UsdShadeTokens->materialBinding);
    TF_AXIOM(materialBinding);
    materialBinding.SetTargets({SdfPath("/Root/Looks/green")});
    delegate->ApplyPendingUpdates();

    // Check that the dirty bits are set for box2
    // Note: There was a bug that switching the material on a skinned mesh
    // the second time would not cause the dirty bits to be set.
    dirtyBits = tracker.GetRprimDirtyBits(SdfPath("/Root/Geometry/box2"));
    TF_AXIOM(dirtyBits != HdChangeTracker::Clean);
}

int main()
{
    TfErrorMark mark;

    SwitchBoundMaterialTest();

    if (TF_AXIOM(mark.IsClean()))
        std::cout << "OK" << std::endl;
    else
        std::cout << "FAILED" << std::endl;
}

