//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/errorMark.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/unitTestNullRenderDelegate.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/editContext.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdSkel/animation.h"
#include "pxr/usd/usdSkel/bindingAPI.h"
#include "pxr/usd/usdSkel/root.h"

#include "pxr/usdImaging/usdImaging/delegate.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static void
SwitchBoundMaterialTest()
{
    std::cout << "-------------------------------------------------------\n";
    std::cout << "SwitchBoundMaterialTest\n";
    std::cout << "-------------------------------------------------------\n";

    const std::string usdPath = "boundMaterial.usda";
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
    auto materialBinding =
        box1Prim.GetRelationship(UsdShadeTokens->materialBinding);
    TF_AXIOM(materialBinding);
    materialBinding.SetTargets({SdfPath("/Root/Looks/green")});

    delegate->ApplyPendingUpdates();
    delegate->SyncAll(true);
    
    // Check that the dirty bits are clean for box2
    auto dirtyBits = tracker.GetRprimDirtyBits(SdfPath("/Root/Geometry/box2"));
    TF_AXIOM(dirtyBits == HdChangeTracker::Clean);

    // Switch the material on box2
    auto box2Prim = stage->GetPrimAtPath(SdfPath("/Root/Geometry/box2"));
    TF_AXIOM(box2Prim);
    materialBinding = box2Prim.GetRelationship(UsdShadeTokens->materialBinding);
    TF_AXIOM(materialBinding);
    materialBinding.SetTargets({SdfPath("/Root/Looks/green")});
    delegate->ApplyPendingUpdates();

    // Check that the dirty bits are set for box2
    dirtyBits = tracker.GetRprimDirtyBits(SdfPath("/Root/Geometry/box2"));
    TF_AXIOM(dirtyBits != HdChangeTracker::Clean);
}

static void
SkelAnimUpdateTest()
{
    std::cout << "-------------------------------------------------------\n";
    std::cout << "SkelAnimUpdateTest\n";
    std::cout << "-------------------------------------------------------\n";

    const std::string usdPath = "animation.usda";
    UsdStageRefPtr stage = UsdStage::Open(usdPath);
    TF_AXIOM(stage);
    
    // Bring up Hydra
    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex>
        renderIndex(HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    auto delegate = std::make_unique<UsdImagingDelegate>(renderIndex.get(),
                               SdfPath::AbsoluteRootPath());
    delegate->Populate(stage->GetPseudoRoot());
    delegate->SetTime(0);
    delegate->SyncAll(true);
    
    UsdEditContext editContext(stage, stage->GetSessionLayer());
    SdfPath animationPath("/Animation");
    UsdSkelAnimation skelAnimation =
        UsdSkelAnimation::Define(stage, animationPath);
    UsdPrim animationPrim = skelAnimation.GetPrim();
    TF_AXIOM(animationPrim);

    // Update skeleton binding
    UsdPrim skeletonPrim = stage->GetPrimAtPath(SdfPath("/Root/Skeleton"));
    UsdSkelBindingAPI skeletonBindingAPI = UsdSkelBindingAPI(skeletonPrim);
    skeletonBindingAPI.GetAnimationSourceRel()
        .SetTargets({animationPath});
    delegate->ApplyPendingUpdates();
    delegate->SyncAll(true);

    // Remove animation and update skelRoot's visibility
    stage->RemovePrim(animationPath);
    skeletonBindingAPI.GetAnimationSourceRel().ClearTargets(false); 
    UsdPrim rootPrim = stage->GetPrimAtPath(SdfPath("/Root"));
    UsdSkelRoot skelRoot = UsdSkelRoot(rootPrim);
    skelRoot.GetVisibilityAttr().Set(UsdGeomTokens->inherited);

    // Expect errors because animation was removed
    TfErrorMark errorMark;

    delegate->ApplyPendingUpdates();
    delegate->SyncAll(true);

    size_t numErrors = 0;
    errorMark.GetBegin(&numErrors);
    TF_AXIOM(numErrors == 2);

    errorMark.Clear();
}

static void
SkinnedMeshInvalidationTest()
{
    std::cout << "-------------------------------------------------------\n";
    std::cout << "SkinnedMeshInvalidationTest\n";
    std::cout << "-------------------------------------------------------\n";

    const std::string usdPath = "skinning.usda";
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

    // Deactivate and reactivate prim
    UsdPrim skinningPrim = stage->GetPrimAtPath(SdfPath("/Root/Skinning"));
    TF_AXIOM(skinningPrim);

    skinningPrim.SetActive(false);
    delegate->ApplyPendingUpdates();
    delegate->SyncAll(true);

    skinningPrim.SetActive(true);
    delegate->ApplyPendingUpdates();
    delegate->SyncAll(true);
}

int main()
{
    TfErrorMark mark;

    SwitchBoundMaterialTest();
    SkelAnimUpdateTest();
    SkinnedMeshInvalidationTest();

    if (TF_AXIOM(mark.IsClean())) {
        std::cout << "OK" << std::endl;
    } else {
        std::cout << "FAILED" << std::endl;
    }
}

