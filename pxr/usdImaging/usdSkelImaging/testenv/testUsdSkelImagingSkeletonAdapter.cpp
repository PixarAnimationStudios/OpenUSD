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
#include "pxr/usd/usd/editContext.h"
#include "pxr/usd/usdSkel/animation.h"
#include "pxr/usd/usdSkel/bindingAPI.h"
#include "pxr/usd/usdSkel/root.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static void
TestSkelAnimUpdateCrash()
{
    std::cout << "-------------------------------------------------------\n";
    std::cout << "TestSkelAnimUpdate\n";
    std::cout << "-------------------------------------------------------\n";

    const std::string usdPath = "model.usda";
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
    SdfPath animation_new_path("/Animation");
    UsdSkelAnimation animation_new = UsdSkelAnimation::Define(stage, animation_new_path);
    UsdPrim animation_new_prim = animation_new.GetPrim();
    TF_AXIOM(animation_new_prim);

    // Update skeleton binding
    UsdPrim skeleton_prim = stage->GetPrimAtPath(SdfPath("/Root/Skeleton"));
    UsdSkelBindingAPI skeleton_bindingAPI = UsdSkelBindingAPI(skeleton_prim);
    skeleton_bindingAPI.GetAnimationSourceRel().SetTargets({animation_new_path});
    delegate->ApplyPendingUpdates();
    delegate->SyncAll(true);

    // Remove animation and update skelroot's visibility
    stage->RemovePrim(animation_new_path);
    skeleton_bindingAPI.GetAnimationSourceRel().ClearTargets(false); 
    UsdPrim skel_root_prim = stage->GetPrimAtPath(SdfPath("/Root"));
    UsdSkelRoot skel_root = UsdSkelRoot(skel_root_prim);
    skel_root.GetVisibilityAttr().Set(UsdGeomTokens->inherited);

    // Test crash due to resync by updating visibility in skelroot.
    // NOTE: This is a test for a crash that happened in the
    //       UsdSkelImagingSkeletonAdapter::_IsAffectedByTimeVaryingSkelAnim() method.
    delegate->ApplyPendingUpdates();
    delegate->SyncAll(true);
}

int main()
{
    TestSkelAnimUpdateCrash();
}
