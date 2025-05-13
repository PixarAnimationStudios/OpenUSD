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
TestSkinnedMeshInvalidation()
{
    std::cout << "-------------------------------------------------------\n";
    std::cout << "TestSkinnedMeshInvalidation\n";
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
    delegate->ApplyPendingUpdates();
    delegate->SyncAll(true);
    
    UsdPrim box_prim = stage->GetPrimAtPath(SdfPath("/Root/Skinning"));
    TF_AXIOM(box_prim);
    box_prim.SetActive(false);
    // Test crash due to access invalidated skinning mesh.
    // NOTE: This is a test for a crash that happened in the
    //       UsdSkelImagingSkeletonAdapter::Populate() method.
    delegate->ApplyPendingUpdates();
    delegate->SyncAll(true);
    box_prim.SetActive(true);
    delegate->ApplyPendingUpdates();
    delegate->SyncAll(true);
}

int main()
{
    TestSkinnedMeshInvalidation();
}
