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
#include "pxr/base/tf/stackTrace.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/xformCache.h"
#include "pxr/usd/usdGeom/scope.h"

PXR_NAMESPACE_USING_DIRECTIVE

// --------------------------------------------------------------------- //
// This test operates on /RootPrim
// and /RootPrim/Foo
// --------------------------------------------------------------------- //
SdfPath rootPrimPath ("/RootPrim");
SdfPath scopePrimPath("/RootPrim/Scope");
SdfPath fooPath      ("/RootPrim/Scope/Foo");
SdfPath fooBarPath   ("/RootPrim/Scope/Foo/Bar");
SdfPath fooBarBazPath("/RootPrim/Scope/Foo/Bar/Baz");
SdfPath barPath      ("/RootPrim/Scope/Bar");

GfMatrix4d GetXform() {
    GfMatrix4d xform(1);
    xform.SetTranslate(GfVec3d(10.0, 20.0, 30.0));
    return xform;
}

GfMatrix4d const IDENTITY(1);

UsdStageRefPtr 
CreateTestData(double timeShift = 0) {
    // --------------------------------------------------------------------- //
    // Author scene and compose the Stage
    // --------------------------------------------------------------------- //
    UsdStageRefPtr stage = UsdStage::CreateInMemory();

    TF_VERIFY(UsdGeomScope::Define(stage, scopePrimPath),
              "Failed to create prim at %s",
              scopePrimPath.GetText());

    TF_VERIFY(UsdGeomXform::Define(stage, rootPrimPath),
              "Failed to create prim at %s",
              rootPrimPath.GetText());

    TF_VERIFY(UsdGeomXform::Define(stage, fooPath),
              "Failed to create prim at %s",
              fooPath.GetText());

    TF_VERIFY(UsdGeomXform::Define(stage, fooBarPath),
              "Failed to create prim at %s",
              fooBarPath.GetText());

    TF_VERIFY(UsdGeomXform::Define(stage, fooBarBazPath),
              "Failed to create prim at %s",
              fooBarBazPath.GetText());

    TF_VERIFY(UsdGeomXform::Define(stage, barPath),
              "Failed to create prim at %s",
              barPath.GetText());


    UsdPrim root(stage->GetPrimAtPath(rootPrimPath));
    TF_VERIFY(root,
              "Failed to get Prim from %s",
              rootPrimPath.GetText());
    
    UsdPrim foo(stage->GetPrimAtPath(fooPath));
    TF_VERIFY(foo,
              "Failed to get Prim from %s",
              fooPath.GetText());

    UsdPrim fooBar(stage->GetPrimAtPath(fooBarPath));
    TF_VERIFY(fooBar,
              "Failed to get Prim from %s",
              fooBarPath.GetText());

    UsdPrim fooBarBaz(stage->GetPrimAtPath(fooBarBazPath));
    TF_VERIFY(fooBarBaz,
              "Failed to get Prim from %s",
              fooBarBazPath.GetText());

    UsdPrim bar(stage->GetPrimAtPath(barPath));
    TF_VERIFY(bar,
              "Failed to get Prim from %s",
              barPath.GetText());

    // --------------------------------------------------------------------- //
    // Setup some test data
    // --------------------------------------------------------------------- //
    GfMatrix4d xform = GetXform();

    UsdGeomXform rootSchema(root);
    UsdGeomXformOp xformOp = rootSchema.AddTransformOp();
    xformOp.Set(xform);
    xformOp.Set(xform*xform, 1+timeShift);
    xformOp.Set(xform*xform*xform, 2+timeShift);

    UsdGeomXform fooSchema(foo);
    UsdGeomXformOp fooXformOp = fooSchema.AddTransformOp();
    fooXformOp.Set(xform);
    fooXformOp.Set(xform*xform, 1+timeShift);
    fooXformOp.Set(xform*xform*xform, 2+timeShift);

    UsdGeomXform fooBarSchema(fooBar);
    UsdGeomXformOp fooBarXformOp = fooBarSchema.AddTransformOp();
    fooBarSchema.SetResetXformStack(true);
    fooBarXformOp.Set(xform);
    fooBarXformOp.Set(xform*xform, 1+timeShift);
    fooBarXformOp.Set(xform*xform*xform, 2+timeShift);

    UsdGeomXform fooBarBazSchema(fooBarBaz);
    UsdGeomXformOp fooBarBazXformOp = fooBarBazSchema.AddTransformOp();
    fooBarBazXformOp.Set(xform);
    fooBarBazXformOp.Set(xform*xform, 1+timeShift);
    fooBarBazXformOp.Set(xform*xform*xform, 2+timeShift);

    UsdGeomXform barSchema(bar);
    UsdGeomXformOp barXformOp = barSchema.AddTransformOp();
    barXformOp.Set(xform);
    barXformOp.Set(xform*xform, 1+timeShift);
    barXformOp.Set(xform*xform*xform, 2+timeShift);

    return stage;
}

void VerifyTransforms(UsdStageRefPtr const& stage, 
                      UsdGeomXformCache& xfCache, 
                      GfMatrix4d xform)
{
    GfMatrix4d ctm;

    UsdPrim root(stage->GetPrimAtPath(rootPrimPath));
    UsdPrim foo(stage->GetPrimAtPath(fooPath));
    UsdPrim fooBar(stage->GetPrimAtPath(fooBarPath));
    UsdPrim fooBarBaz(stage->GetPrimAtPath(fooBarBazPath));
    UsdPrim bar(stage->GetPrimAtPath(barPath));
    
    // --------------------------------------------------------------------- //
    // Test GetLocalToWorldTransform, TIME = DEFAULT
    // --------------------------------------------------------------------- //
 
    // '/' cannot have transformations, so should get IDENTITY
    ctm = xfCache.GetLocalToWorldTransform(stage->GetPseudoRoot());
    TF_VERIFY(ctm == IDENTITY,
              "LocalToWorldTransform value for %s is incorrect.",
              stage->GetPseudoRoot().GetPath().GetText());

    TF_VERIFY(!xfCache.TransformMightBeTimeVarying(stage->GetPseudoRoot()));
    TF_VERIFY(!xfCache.GetResetXformStack(stage->GetPseudoRoot()));

    // Should return xform
    ctm = xfCache.GetLocalToWorldTransform(root);
    TF_VERIFY(ctm == xform,
              "LocalToWorldTransform value for %s is incorrect.",
              root.GetPath().GetText());
    TF_VERIFY(xfCache.TransformMightBeTimeVarying(root));
    TF_VERIFY(!xfCache.GetResetXformStack(root));

    // Should return xform * xform
    ctm = xfCache.GetLocalToWorldTransform(foo);
    TF_VERIFY(ctm == (xform * xform),
              "LocalToWorldTransform value for %s is incorrect.",
              foo.GetPath().GetText());
    TF_VERIFY(xfCache.TransformMightBeTimeVarying(foo));
    TF_VERIFY(!xfCache.GetResetXformStack(foo));

    // Should return xform * xform * xform
    ctm = xfCache.GetLocalToWorldTransform(fooBar);
    TF_VERIFY(ctm == xform,
              "LocalToWorldTransform value for %s is incorrect.",
              fooBar.GetPath().GetText());
    TF_VERIFY(xfCache.TransformMightBeTimeVarying(fooBar));
    TF_VERIFY(xfCache.GetResetXformStack(fooBar));
    
    // Should return xform * xform * xform * xform
    ctm = xfCache.GetLocalToWorldTransform(fooBarBaz);
    TF_VERIFY(ctm == (xform * xform),
              "LocalToWorldTransform value for %s is incorrect.",
              fooBarBaz.GetPath().GetText());
    TF_VERIFY(xfCache.TransformMightBeTimeVarying(fooBarBaz));
    TF_VERIFY(!xfCache.GetResetXformStack(fooBarBaz));

    // Should return xform * xform
    ctm = xfCache.GetLocalToWorldTransform(bar);
    TF_VERIFY(ctm == (xform * xform),
              "LocalToWorldTransform value for %s is incorrect.",
              bar.GetPath().GetText());
    TF_VERIFY(xfCache.TransformMightBeTimeVarying(bar));
    TF_VERIFY(!xfCache.GetResetXformStack(bar));

    // --------------------------------------------------------------------- //
    // Test GetParentToWorldTransform
    // --------------------------------------------------------------------- //
    
    // '/' cannot have transformations, so should get IDENTITY
    ctm = xfCache.GetParentToWorldTransform(stage->GetPseudoRoot());
    TF_VERIFY(ctm == IDENTITY,
              "ParentToWorldTransform value for %s is incorrect.",
              stage->GetPseudoRoot().GetPath().GetText());

    // Should return IDENTITY
    ctm = xfCache.GetParentToWorldTransform(root);
    TF_VERIFY(ctm == IDENTITY,
              "ParentToWorldTransform value for %s is incorrect.",
              root.GetPath().GetText());
    
    // Should return xform
    ctm = xfCache.GetParentToWorldTransform(foo);
    TF_VERIFY(ctm == xform,
              "ParentToWorldTransform value for %s is incorrect.",
              foo.GetPath().GetText());

    // Should return xform * xform
    ctm = xfCache.GetParentToWorldTransform(fooBar);
    TF_VERIFY(ctm == (xform * xform),
              "ParentToWorldTransform value for %s is incorrect.",
              fooBar.GetPath().GetText());

    // Should return xform * xform * xform
    ctm = xfCache.GetParentToWorldTransform(fooBarBaz);
    TF_VERIFY(ctm == (xform),
              "ParentToWorldTransform value for %s is incorrect.",
              fooBarBaz.GetPath().GetText());

    // Should return xform
    ctm = xfCache.GetParentToWorldTransform(bar);
    TF_VERIFY(ctm == xform,
              "ParentToWorldTransform value for %s is incorrect.",
              bar.GetPath().GetText());

    // --------------------------------------------------------------------- //
    // Test ComputeRelativeTransform
    // --------------------------------------------------------------------- //

    bool resetXformStack = false;

    // /RootPrim relative to / : Should return xform.
    ctm = xfCache.ComputeRelativeTransform(root, stage->GetPseudoRoot(),
                                           &resetXformStack);
    TF_VERIFY(ctm == xform,
              "ComputeRelativeTransform value for (%s,%s) is incorrect.",
              root.GetPath().GetText(),
              stage->GetPseudoRoot().GetPath().GetText());

    // /RootPrim/Scope/Foo relative to /RootPrim. Should return xform.
    ctm = xfCache.ComputeRelativeTransform(foo, root, &resetXformStack);
    TF_VERIFY(ctm == (xform),
              "ComputeRelativeTransform value for (%s,%s) is incorrect.",
              foo.GetPath().GetText(), root.GetPath().GetText());

    // /RootPrim/Scope/Foo/Bar relative to /RootPrim.
    // fooBar resets the xform stack, so should return xform.
    ctm = xfCache.ComputeRelativeTransform(fooBar, root, &resetXformStack);
    TF_VERIFY(ctm == (xform), 
              "ComputeRelativeTransform value for (%s,%s) is incorrect.",
              fooBar.GetPath().GetText(), root.GetPath().GetText());

    // /RootPrim/Scope/Foo/Bar/Baz relative to /RootPrim/Foo/Bar.
    // fooBar resets the xform stack, so should return xform*xform.
    ctm = xfCache.ComputeRelativeTransform(fooBarBaz, root, &resetXformStack);
    TF_VERIFY(ctm == (xform * xform),
              "ComputeRelativeTransform value for (%s,%s) is incorrect.",
              fooBarBaz.GetPath().GetText(), root.GetPath().GetText());

    // /RootPrim/Scope/Bar relative to /RootPrim. Should return xform.
    ctm = xfCache.ComputeRelativeTransform(bar, root, &resetXformStack);
    TF_VERIFY(ctm == (xform),
              "ComputeRelativeTransform value for (%s,%s) is incorrect.",
              bar.GetPath().GetText(), root.GetPath().GetText());
}

void XformCacheTest(UsdStageRefPtr const& stage)
{
    UsdGeomXformCache xfCache;
    GfMatrix4d xform = GetXform();

    std::cout << "----------------------------------------------------------\n";
    std::cout << "Verify at time implicitly = UsdTimeCode::Default()\n";
    std::cout << "----------------------------------------------------------\n";
    VerifyTransforms(stage, xfCache, xform);

    std::cout << "----------------------------------------------------------\n";
    std::cout << "Verify at time = 1.0 (xform*xform), via SetTime(1.0)\n";
    std::cout << "----------------------------------------------------------\n";
    xfCache.SetTime(1.0);
    xform = GetXform()*GetXform();
    VerifyTransforms(stage, xfCache, xform);
    
    std::cout << "----------------------------------------------------------\n";
    std::cout << "Verify at time = 2.0 (xform*xform*xform), via ctor(2.0)\n";
    std::cout << "----------------------------------------------------------\n";
    xfCache = UsdGeomXformCache(2.0);
    xform = GetXform()*GetXform()*GetXform();
    VerifyTransforms(stage, xfCache, xform);

    std::cout << "----------------------------------------------------------\n";
    std::cout << "Verify at after XformCache::Clear(), time=2.0\n";
    std::cout << "----------------------------------------------------------\n";
    xfCache.Clear();
    VerifyTransforms(stage, xfCache, xform);

    std::cout << "----------------------------------------------------------\n";
    std::cout << "Verify at time explicitly = UsdTimeCode::Default (xform)\n";
    std::cout << "----------------------------------------------------------\n";
    xfCache.SetTime(UsdTimeCode::Default());
    xform = GetXform();
    VerifyTransforms(stage, xfCache, xform);

    std::cout << "----------------------------------------------------------\n";
    std::cout << "Verify mixed stages\n";
    std::cout << "----------------------------------------------------------\n";
    xfCache.SetTime(2.0);
    std::cout << "Verify default stage(xform*xform*xform)...\n";
    xform = GetXform()*GetXform()*GetXform();
    VerifyTransforms(stage, xfCache, xform);

    std::cout << "Verify alternate stage(xform*xform)...\n";
    // Create an alternate version of the main stage, with time shifted by 1.
    UsdStageRefPtr altStage = CreateTestData(1);
    // Time shift results in one lest xform multiplication at time = 2.0.
    xform = GetXform()*GetXform();
    VerifyTransforms(altStage, xfCache, xform);

    // Verify our original results are still sane.
    std::cout << "Verify default stage(xform*xform*xform)...\n";
    xform = GetXform()*GetXform()*GetXform();
    VerifyTransforms(stage, xfCache, xform);
}

int main()
{
    UsdStageRefPtr stage = CreateTestData();
    XformCacheTest(stage);
}

