//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/motionAPI.h"
#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/modelAPI.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

void
TestHasAPI()
{
    auto stage = UsdStage::CreateInMemory("TestPrimQueries.usd");
    auto path = SdfPath("/p");
    auto prim = stage->DefinePrim(path);
    
    // Valid cases
    TF_AXIOM(!prim.HasAPI<UsdGeomMotionAPI>());
    TF_AXIOM(UsdGeomMotionAPI::CanApply(prim));
    TF_AXIOM(prim.CanApplyAPI<UsdGeomMotionAPI>());
    UsdGeomMotionAPI::Apply(prim);
    TF_AXIOM(prim.HasAPI<UsdGeomMotionAPI>());
    prim.RemoveAPI<UsdGeomMotionAPI>();
    TF_AXIOM(!prim.HasAPI<UsdGeomMotionAPI>());
    prim.ApplyAPI<UsdGeomMotionAPI>();
    TF_AXIOM(prim.HasAPI<UsdGeomMotionAPI>());
    
    TF_AXIOM(!prim.HasAPI<UsdGeomModelAPI>());
    TF_AXIOM(UsdGeomModelAPI::CanApply(prim));
    TF_AXIOM(prim.CanApplyAPI<UsdGeomModelAPI>());
    UsdGeomModelAPI::Apply(prim);
    TF_AXIOM(prim.HasAPI<UsdGeomModelAPI>());
    prim.RemoveAPI<UsdGeomModelAPI>();
    TF_AXIOM(!prim.HasAPI<UsdGeomModelAPI>());
    prim.ApplyAPI<UsdGeomModelAPI>();
    TF_AXIOM(prim.HasAPI<UsdGeomModelAPI>());

    // The following cases won't compile, uncomment them to confirm
    // TF_AXIOM(prim.HasAPI<UsdGeomImageable>()); // can't be typed
    // TF_AXIOM(prim.HasAPI<UsdGeomXform>());     // can't be concrete
    // TF_AXIOM(!prim.HasAPI<UsdModelAPI>());     // can't be non-applied API schema
    // 
    // // must be derived from UsdAPISchemaBase
    // TF_AXIOM(prim.CanApplyAPI<UsdGeomXform>());   
    // TF_AXIOM(prim.ApplyAPI<UsdGeomXform>());   
    // TF_AXIOM(prim.RemoveAPI<UsdGeomXform>());  
    // 
    // // must be multiple apply for instance name
    // TF_AXIOM(!prim.HasAPI<UsdGeomMotionAPI>(TfToken("instance")));
    // TF_AXIOM(prim.CanApplyAPI<UsdGeomModelAPI>(TfToken("instance")));
    // TF_AXIOM(prim.ApplyAPI<UsdGeomModelAPI>(TfToken("instance")));   
    // TF_AXIOM(prim.RemoveAPI<UsdGeomModelAPI>(TfToken("instance")));
}
 
int main()
{
    TestHasAPI();
}
