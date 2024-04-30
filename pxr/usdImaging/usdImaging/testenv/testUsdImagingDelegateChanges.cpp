//
// Copyright 2024 Pixar
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

#include "pxr/usdImaging/usdImaging/unitTestHelper.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/dirtyList.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/selection.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/unitTestNullRenderDelegate.h"

#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/collectionAPI.h"
#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/scope.h"
#include "pxr/usd/usdGeom/sphere.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/usd/usdShade/coordSysAPI.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/shader.h"

#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/gf/frustum.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

void
PrimResyncTest()
{
    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << "PrimResync Test\n";
    std::cout << "--------------------------------------------------------------------------------\n";

    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    SdfLayerRefPtr sessionLayer = SdfLayer::CreateAnonymous(".usda");
    SdfLayerRefPtr rootLayer = SdfLayer::CreateAnonymous(".usda");
    UsdStageRefPtr stage = UsdStage::Open(rootLayer, sessionLayer);

    int dirtyBits;
    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> renderIndex(
        HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    TF_AXIOM(renderIndex);
    std::unique_ptr<UsdImagingDelegate> delegate(
                          new UsdImagingDelegate(renderIndex.get(),
                                                  SdfPath::AbsoluteRootPath()));
    HdChangeTracker& tracker = renderIndex->GetChangeTracker();

    // Populate the empty stage
    delegate->Populate(stage->GetPseudoRoot());

    UsdGeomXform xf1 = UsdGeomXform::Define(stage, SdfPath("/Xf1"));
    UsdGeomXform xf2 = UsdGeomXform::Define(stage, SdfPath("/Xf1/Xf2"));
    UsdGeomMesh mesh1 = UsdGeomMesh::Define(stage, SdfPath("/Xf1/Xf2/Mesh1"));
    UsdGeomMesh mesh2 = UsdGeomMesh::Define(stage, SdfPath("/Xf1/Xf2/Mesh2"));
    UsdGeomCube cube = UsdGeomCube::Define(stage, SdfPath("/Xf1/Xf2/Cube"));
    UsdGeomCube cube2 = UsdGeomCube::Define(stage, SdfPath("/Xf1/Xf2/Cube2"));

    // Set time to force a sync and process changes.
    // This will generate warnings because points are not authored.
    std::cerr << "\nBEGIN EXPECTED WARNINGS\n"
              << "--------------------------------------------------------------------------------\n";
    delegate->SetTime(0);
    std::cerr << "--------------------------------------------------------------------------------\n"
              << "END WARNINGS\n\n";

    // We expect the prims to be fully dirty, as they were just inserted.
    dirtyBits = tracker.GetRprimDirtyBits(mesh1.GetPath());
    TF_AXIOM(dirtyBits & HdChangeTracker::AllDirty);
    tracker.MarkRprimClean(mesh1.GetPath());

    dirtyBits = tracker.GetRprimDirtyBits(mesh2.GetPath());
    TF_AXIOM(dirtyBits & HdChangeTracker::AllDirty);
    tracker.MarkRprimClean(mesh2.GetPath());

    dirtyBits = tracker.GetRprimDirtyBits(cube.GetPath());
    TF_AXIOM(dirtyBits & HdChangeTracker::AllDirty);
    tracker.MarkRprimClean(cube.GetPath());

    dirtyBits = tracker.GetRprimDirtyBits(cube2.GetPath());
    TF_AXIOM(dirtyBits & HdChangeTracker::AllDirty);
    tracker.MarkRprimClean(cube2.GetPath());

    VtVec3fArray points1(3);
    points1[0] = GfVec3f(1,0,0);
    points1[1] = GfVec3f(0,2,0);
    points1[2] = GfVec3f(0,0,3);

    mesh1.GetPointsAttr().Set(points1);
    delegate->SetTime(0);

    VtVec3fArray points2(3);
    points2[0] = GfVec3f(4,0,0);
    points2[1] = GfVec3f(0,5,0);
    points2[2] = GfVec3f(0,0,6);
    mesh2.GetPointsAttr().Set(points2);

    cube.GetSizeAttr().Set(1.0);
    UsdGeomXformOp cube2XformOp = cube2.AddTransformOp();
    cube2XformOp.Set(GfMatrix4d(1), UsdTimeCode::Default());

    // Process changes. 
    delegate->SetTime(0);

    // NOTE TO FUTURE DEBUGGERS: The first time an attribute gets set, it will
    // trigger a resync, because createing a new PropertySpec is "significant",
    // so now the dirtyBits below are all expected to be AllDirty (-1).

    // Expect dirty points for meshes
    dirtyBits = tracker.GetRprimDirtyBits(mesh1.GetPath());
    TF_AXIOM(dirtyBits & HdChangeTracker::DirtyPoints);
    dirtyBits = tracker.GetRprimDirtyBits(mesh2.GetPath());
    TF_AXIOM(dirtyBits & HdChangeTracker::DirtyPoints);

    // Changing the size should invalidate the points.
    dirtyBits = tracker.GetRprimDirtyBits(cube.GetPath());
    TF_AXIOM(dirtyBits & HdChangeTracker::DirtyPoints);

    // Changing the matrix should also invalidate the transform
    dirtyBits = tracker.GetRprimDirtyBits(cube2.GetPath());
    TF_AXIOM(dirtyBits & HdChangeTracker::DirtyTransform);

    // Ensure values are populated
    delegate->SyncAll(true);

    // Make sure the values are good
    VtValue value;
    value = delegate->Get(mesh1.GetPath(), UsdGeomTokens->points);
    TF_AXIOM(value.Get<VtVec3fArray>() == points1);
    value = delegate->Get(mesh2.GetPath(), UsdGeomTokens->points);
    TF_AXIOM(value.Get<VtVec3fArray>() == points2);

    // Mark everything as clean.
    tracker.MarkRprimClean(mesh1.GetPath());
    tracker.MarkRprimClean(mesh2.GetPath());
    tracker.MarkRprimClean(cube.GetPath());
    tracker.MarkRprimClean(cube2.GetPath());
    // Process changes. 
    delegate->SetTime(0);

    // We do not expect them to be dirty now, since the points are not actually
    // varying.
    dirtyBits = tracker.GetRprimDirtyBits(mesh1.GetPath());
    TF_AXIOM(!(dirtyBits & HdChangeTracker::DirtyPoints));
    dirtyBits = tracker.GetRprimDirtyBits(mesh2.GetPath());
    TF_AXIOM(!(dirtyBits & HdChangeTracker::DirtyPoints));
    dirtyBits = tracker.GetRprimDirtyBits(cube.GetPath());
    TF_AXIOM(!(dirtyBits & HdChangeTracker::DirtyTransform));
    dirtyBits = tracker.GetRprimDirtyBits(cube2.GetPath());
    TF_AXIOM(!(dirtyBits & HdChangeTracker::DirtyTransform));

    // Set the edit target to the session layer to ensure changes authored
    // in a stronger layer are picked up as expected.
    stage->SetEditTarget(sessionLayer);

    // Animate cube size.
    cube.GetSizeAttr().Set(2.0, 1.0);
    cube.GetSizeAttr().Set(3.0, 2.0);

    // Animate cube Transform.
    cube2XformOp.Set(GfMatrix4d(2), 1.0);
    cube2XformOp.Set(GfMatrix4d(3), 2.0);

    // Animate the points for mesh2.
    points2[0] = GfVec3f(7,0,0);
    points2[1] = GfVec3f(0,8,0);
    points2[2] = GfVec3f(0,0,9);
    mesh2.GetPointsAttr().Set(points2, 1.0);
    points2[0] = GfVec3f(-7,0,0);
    points2[1] = GfVec3f(0,-8,0);
    points2[2] = GfVec3f(0,0,-9);
    mesh2.GetPointsAttr().Set(points2, 2.0);

    // Update, clean, update to cycle time
    delegate->SetTime(1);
    tracker.MarkRprimClean(mesh1.GetPath());
    tracker.MarkRprimClean(mesh2.GetPath());
    tracker.MarkRprimClean(cube.GetPath());
    tracker.MarkRprimClean(cube2.GetPath());
    delegate->SetTime(2);

    // Now expect:
    //      dirtyBits(mesh1) == Clean
    //      dirtyBits(mesh2) == DirtyPoints
    //      dirtyBits(cube)  == DirtyPoints
    //      dirtyBits(cube2)  == DirtyTransform

    // Mesh1 should still be clean, but mesh2 should be marked as dirty.
    dirtyBits = tracker.GetRprimDirtyBits(mesh1.GetPath());
    TF_AXIOM(!(dirtyBits & HdChangeTracker::DirtyPoints));
    
    // Should be dirtyPoints:
    dirtyBits = tracker.GetRprimDirtyBits(mesh2.GetPath());
    TF_AXIOM(dirtyBits & HdChangeTracker::DirtyPoints);

    // Should be dirtyPoints:
    dirtyBits = tracker.GetRprimDirtyBits(cube.GetPath());
    TF_AXIOM((dirtyBits & HdChangeTracker::DirtyPoints));
    TF_AXIOM(!(dirtyBits & HdChangeTracker::DirtyTransform));

    // Should be dirtyTransform:
    dirtyBits = tracker.GetRprimDirtyBits(cube2.GetPath());
    TF_AXIOM(!(dirtyBits & HdChangeTracker::DirtyPoints));
    TF_AXIOM(dirtyBits & HdChangeTracker::DirtyTransform);

    // Ensure values are populated
    delegate->SyncAll(true);

    // Verify mesh2.points
    value = delegate->Get(mesh2.GetPath(), UsdGeomTokens->points);
    TF_AXIOM(value.Get<VtVec3fArray>() == points2);
}

void
VisibilityTest()
{
    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << "Visibility Test\n";
    std::cout << "--------------------------------------------------------------------------------\n";
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    UsdStageRefPtr stage = UsdStage::CreateInMemory();

    int dirtyBits;
    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> renderIndex(
        HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    TF_AXIOM(renderIndex);
    std::unique_ptr<UsdImagingDelegate> delegate(
                          new UsdImagingDelegate(renderIndex.get(),
                                                  SdfPath::AbsoluteRootPath()));
    HdChangeTracker& tracker = renderIndex->GetChangeTracker();

    // Populate the empty stage
    delegate->Populate(stage->GetPseudoRoot());

    UsdGeomXform xf1 = UsdGeomXform::Define(stage, SdfPath("/Xf1"));

    UsdGeomXform xf2 = UsdGeomXform::Define(stage, SdfPath("/Xf1/Xf2"));
    UsdGeomCube cube1 = UsdGeomCube::Define(stage, SdfPath("/Xf1/Xf2/Cube1"));

    UsdGeomXform xf3 = UsdGeomXform::Define(stage, SdfPath("/Xf1/Xf3"));
    UsdGeomCube cube2 = UsdGeomCube::Define(stage, SdfPath("/Xf1/Xf3/Cube2"));

    // Set time to force a sync.
    delegate->SetTime(0);

    // Expect visibility to be dirty upon creation.
    dirtyBits = tracker.GetRprimDirtyBits(cube1.GetPath());
    TF_AXIOM(dirtyBits & HdChangeTracker::DirtyVisibility);
    tracker.MarkRprimClean(cube1.GetPath());
    dirtyBits = tracker.GetRprimDirtyBits(cube2.GetPath());
    TF_AXIOM(dirtyBits & HdChangeTracker::DirtyVisibility);
    tracker.MarkRprimClean(cube2.GetPath());

    // Process changes. 
    delegate->SetTime(0);

    // NOTE TO FUTURE DEBUGGERS: The first time an attribute gets set, it will
    // trigger a resync, because createing a new PropertySpec is "significant",
    // so now the dirtyBits below are all expected to be AllDirty (-1).
     
    cube1.GetVisibilityAttr().Set(UsdGeomTokens->invisible, 1.0);
    cube2.GetVisibilityAttr().Set(UsdGeomTokens->invisible, 1.0);

    // Notices get sent upon setting the value, however they accumulate in the
    // delegate until SetTime is called, so we expect no dirtiness yet.
    dirtyBits = tracker.GetRprimDirtyBits(cube1.GetPath());
    TF_AXIOM(!(dirtyBits & HdChangeTracker::DirtyVisibility));
    dirtyBits = tracker.GetRprimDirtyBits(cube2.GetPath());
    TF_AXIOM(!(dirtyBits & HdChangeTracker::DirtyVisibility));

    // Process changes.
    delegate->SetTime(0);

    // Expect dirty visibility.
    dirtyBits = tracker.GetRprimDirtyBits(cube1.GetPath());
    TF_AXIOM(dirtyBits & HdChangeTracker::DirtyVisibility);
    dirtyBits = tracker.GetRprimDirtyBits(cube2.GetPath());
    TF_AXIOM(dirtyBits & HdChangeTracker::DirtyVisibility);

    // Make sure the values are good
    TF_AXIOM(!delegate->GetVisible(cube1.GetPath()));
    TF_AXIOM(!delegate->GetVisible(cube2.GetPath()));

    // Mark everything as clean.
    tracker.MarkRprimClean(cube1.GetPath());
    tracker.MarkRprimClean(cube2.GetPath());

    // Setting the time should flag them as dirty again
    delegate->SetTime(1.0);

    // We do not expect them to be dirty now, since the vis is not actually
    // varying.
    dirtyBits = tracker.GetRprimDirtyBits(cube1.GetPath());
    TF_AXIOM(!(dirtyBits & HdChangeTracker::DirtyVisibility));
    dirtyBits = tracker.GetRprimDirtyBits(cube2.GetPath());
    TF_AXIOM(!(dirtyBits & HdChangeTracker::DirtyVisibility));

    // Animate cube size.
    cube1.GetVisibilityAttr().Set(UsdGeomTokens->inherited, 1.0);
    cube2.GetVisibilityAttr().Set(UsdGeomTokens->inherited, 1.0);
    
    // Process Changes.
    delegate->SetTime(1.0);

    dirtyBits = tracker.GetRprimDirtyBits(cube1.GetPath());
    TF_AXIOM(dirtyBits & HdChangeTracker::DirtyVisibility);
    dirtyBits = tracker.GetRprimDirtyBits(cube2.GetPath());
    TF_AXIOM(dirtyBits & HdChangeTracker::DirtyVisibility);

    // Make sure the values are good
    TF_AXIOM(delegate->GetVisible(cube1.GetPath()));
    TF_AXIOM(delegate->GetVisible(cube2.GetPath()));
}

void
PrimExpiredTest(TfErrorMark* mark)
{
    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << "PrimExpired Test\n";
    std::cout << "--------------------------------------------------------------------------------\n";

    int dirtyBits = 0;
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomMesh mesh1 = UsdGeomMesh::Define(stage, SdfPath("/Mesh1"));

    VtVec3fArray points(3);
    points[0] = GfVec3f(1,0,0);
    points[1] = GfVec3f(0,2,0);
    points[2] = GfVec3f(0,0,3);
    mesh1.GetPointsAttr().Set(points);

    // Populate the stage
    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> renderIndex(
        HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    TF_AXIOM(renderIndex);
    std::unique_ptr<UsdImagingDelegate> delegate(
                          new UsdImagingDelegate(renderIndex.get(),
                                                  SdfPath::AbsoluteRootPath()));
    HdChangeTracker& tracker = renderIndex->GetChangeTracker();
    delegate->Populate(mesh1.GetPrim());

    dirtyBits = tracker.GetRprimDirtyBits(mesh1.GetPath());
    TF_AXIOM(dirtyBits & HdChangeTracker::AllDirty);

    // Delete the root prim.
    SdfLayerHandle layer = stage->GetRootLayer();
    SdfPrimSpecHandle root = layer->GetPrimAtPath(SdfPath::AbsoluteRootPath());
    SdfPrimSpecHandle prim = layer->GetPrimAtPath(mesh1.GetPath());
    root->RemoveNameChild(prim);

    // Process changes, killing the root prim; should not crash. 
    delegate->SetTime(0);

    // Recreate the prim.
    mesh1 = UsdGeomMesh::Define(stage, SdfPath("/Mesh1"));
    mesh1.GetPointsAttr().Set(points);

#if 0 // Unfortunately, mentor doesn't let us do this :(

    // Though the new prim was created, that change should not yet have been
    // processed.
    {
        TF_AXIOM(mark->IsClean());
        std::cerr << "\nBEGIN EXPECTED ERROR\n"
                  << "--------------------------------------------------------------------------------\n";
        tracker.GetRprimDirtyBits(mesh1.GetPath());
        std::cerr << "--------------------------------------------------------------------------------\n"
                  << "END ERROR\n\n";
        TF_AXIOM(!mark->IsClean());
        mark->Clear();
    }
#endif 

    // Process the change that restored the prim, then expect all normal API to
    // resume functioning.
    delegate->SetTime(0);
    TF_AXIOM(delegate->GetVisible(mesh1.GetPath()));
    dirtyBits = tracker.GetRprimDirtyBits(mesh1.GetPath());
    TF_AXIOM(dirtyBits & HdChangeTracker::AllDirty);
}

static void
PrimAndCollectionExpiredTest()
{
    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << "PrimAndCollectionExpiredTest\n";
    std::cout << "--------------------------------------------------------------------------------\n";

    // Define stage with a sphere and a collection that includes that sphere.
    UsdStageRefPtr stage = UsdStage::CreateInMemory();

    UsdGeomXform world = UsdGeomXform::Define(stage, SdfPath("/World"));
    UsdGeomSphere sphere = 
        UsdGeomSphere::Define(stage, SdfPath("/World/sphere"));

    UsdCollectionAPI collection = 
        UsdCollectionAPI::Apply(world.GetPrim(), TfToken("spheres"));
    collection.IncludePath(sphere.GetPath());

    // Create and populate delegate from the stage.
    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> renderIndex(
        HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    TF_AXIOM(renderIndex);
    std::unique_ptr<UsdImagingDelegate> delegate(
                          new UsdImagingDelegate(renderIndex.get(),
                                                  SdfPath::AbsoluteRootPath()));

    delegate->Populate(stage->GetPseudoRoot());
    delegate->SetTime(0);
    delegate->SyncAll(true);
    
    // Remove sphere and collection.
    stage->RemovePrim(sphere.GetPath());
    world.GetPrim().RemoveProperty(collection.GetIncludesRel().GetName());

    // Resync
    delegate->SetTime(0);
    delegate->SyncAll(true);
}

void
PrimHierarchyResyncTest()
{
    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << "PrimHierarchyResync Test\n";
    std::cout << "--------------------------------------------------------------------------------\n";
    
    // We want to test that a UsdImagingDelegate populated at a particular
    // prim does not respond to changes to prims outside that hierarchy.

    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    // Set up a test stage with two cubes in different branches of namespace.
    UsdStageRefPtr stage = UsdStage::CreateInMemory();

    UsdGeomXform xf1 = UsdGeomXform::Define(stage, SdfPath("/Xf1"));
    UsdGeomXform xf2 = UsdGeomXform::Define(stage, SdfPath("/Xf1/Xf2"));
    UsdGeomXform xf3 = UsdGeomXform::Define(stage, SdfPath("/Xf1/Xf3"));

    UsdGeomCube cube1 = UsdGeomCube::Define(stage, SdfPath("/Xf1/Xf2/Cube1"));
    UsdGeomCube cube2 = UsdGeomCube::Define(stage, SdfPath("/Xf1/Xf3/Cube2"));

    // Create and populate an imaging delegate for one of the cubes.
    // Verify that only it is marked dirty; the delegate should not care
    // about cube2.
    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> renderIndex(
        HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    TF_AXIOM(renderIndex);
    std::unique_ptr<UsdImagingDelegate> delegate(
                          new UsdImagingDelegate(renderIndex.get(),
                                                  SdfPath::AbsoluteRootPath()));
    HdChangeTracker& tracker = renderIndex->GetChangeTracker();

    delegate->Populate(cube1.GetPrim());
    delegate->SetTime(0);

    HdDirtyList dirtyList(delegate->GetRenderIndex());
    // Note: We don't call HdDirtyList::UpdateRenderTagsAndReprSelectors
    // here. The empty set of render tags effectively includes all Rprims.
    SdfPathVector dirtyPrims = dirtyList.GetDirtyRprims();
    TF_AXIOM(dirtyPrims.size() == 1);
    TF_AXIOM(dirtyPrims.front() == SdfPath("/Xf1/Xf2/Cube1"));
    tracker.MarkRprimClean(cube1.GetPath());
    tracker.ResetVaryingState();

    dirtyPrims = dirtyList.GetDirtyRprims();
    TF_AXIOM(dirtyPrims.empty());
    
    // Set the first time sample on the cubes. This authors new property
    // specs for the size attribute, causing resyncs. The imaging delegate
    // only cares about cube1, so it's still the only thing that should
    // be marked dirty.
    cube1.GetSizeAttr().Set(1.0, 1.0);
    cube2.GetSizeAttr().Set(1.0, 2.0);
    delegate->SetTime(1);

    dirtyPrims = dirtyList.GetDirtyRprims();
    TF_AXIOM(dirtyPrims.size() == 1);
    TF_AXIOM(dirtyPrims.front() == SdfPath("/Xf1/Xf2/Cube1"));
}

void
SparsePrimResyncTest()
{
    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << "SparsePrimResyncTest Test\n";
    std::cout << "--------------------------------------------------------------------------------\n";

    // Test that scene description changes to metadata that doesn't affect
    // imaging does not cause unnecessary resyncs.

    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    SdfLayerHandle sessionLayer = stage->GetSessionLayer();

    UsdGeomXform xf1 = UsdGeomXform::Define(stage, SdfPath("/Xf1"));
    UsdGeomXform xf2 = UsdGeomXform::Define(stage, SdfPath("/Xf1/Xf2"));
    UsdGeomCube cube1 = UsdGeomCube::Define(stage, SdfPath("/Xf1/Xf2/Cube1"));

    // Create and populate an imaging delegate for one of the cubes.
    // Verify that only it is marked dirty; the delegate should not care
    // about cube2.
    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> renderIndex(
        HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    TF_AXIOM(renderIndex);
    std::unique_ptr<UsdImagingDelegate> delegate(
                          new UsdImagingDelegate(renderIndex.get(),
                                                  SdfPath::AbsoluteRootPath()));
    HdChangeTracker& tracker = renderIndex->GetChangeTracker();

    delegate->Populate(stage->GetPseudoRoot());
    delegate->SetTime(0);
    delegate->SyncAll(true);

    // We expect the prims to be clean
    tracker.MarkRprimClean(cube1.GetPath());
    int dirtyBits = tracker.GetRprimDirtyBits(cube1.GetPath());
    TF_AXIOM(dirtyBits == HdChangeTracker::Clean);

    // Author an inert prim spec over cube1. cube1 should remain clean
    // since this change does not affect imaging.
    SdfCreatePrimInLayer(sessionLayer, cube1.GetPath());

    delegate->SetTime(0);
    delegate->SyncAll(true);
    
    dirtyBits = tracker.GetRprimDirtyBits(cube1.GetPath());
    TF_AXIOM(dirtyBits == HdChangeTracker::Clean);

    // Author some metadata on cube1 unrelated to imaging. cube1 should
    // remain clean.
    cube1.GetPrim().SetDocumentation("test docstring");

    delegate->SetTime(0);
    delegate->SyncAll(true);
    
    dirtyBits = tracker.GetRprimDirtyBits(cube1.GetPath());
    TF_AXIOM(dirtyBits == HdChangeTracker::Clean);

    // Author metadata on cube1 that Usd should consider significant
    // and cause a resync.
    UsdModelAPI(cube1.GetPrim()).SetKind(TfToken("test"));

    delegate->SetTime(0);
    delegate->SyncAll(true);
    
    dirtyBits = tracker.GetRprimDirtyBits(cube1.GetPath());
    TF_AXIOM(dirtyBits & HdChangeTracker::AllDirty);
}

void
InstancePrimResyncTest()
{
    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << "InstancePrimResyncTest Test\n";
    std::cout << "--------------------------------------------------------------------------------\n";
    
    UsdStageRefPtr stage = UsdStage::CreateInMemory();

    UsdGeomXform instXf = UsdGeomXform::Define(stage, SdfPath("/Instance"));
    UsdGeomCube instCube = UsdGeomCube::Define(stage, SdfPath("/Instance/cube"));

    UsdGeomXform root = UsdGeomXform::Define(stage, SdfPath("/Models"));
    UsdPrim instances[2];
    for (size_t i = 0; i < 2; ++i) {
        instances[i] = stage->DefinePrim(
            SdfPath(TfStringPrintf("/Models/cube_%zu", i)));
        TF_AXIOM(instances[i]);

        instances[i].GetReferences().AddInternalReference(SdfPath("/Instance"));
        instances[i].SetInstanceable(true);
    }

    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> renderIndex(
        HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    TF_AXIOM(renderIndex);
    std::unique_ptr<UsdImagingDelegate> delegate(
                          new UsdImagingDelegate(renderIndex.get(),
                                                  SdfPath::AbsoluteRootPath()));
    delegate->Populate(stage->GetPseudoRoot());
    delegate->SetTime(0);

    stage->SetEditTarget(stage->GetSessionLayer());

    // Creating the vis attribute should cause a prim resync.
    UsdAttribute visAttr = 
        UsdGeomImageable::Get(stage, SdfPath("/Models/cube_1"))
        .CreateVisibilityAttr();
    visAttr.Set(UsdGeomTokens->invisible);
    delegate->SetTime(0);

    // This should cause just a property change, not a prim resync.
    visAttr.Set(UsdGeomTokens->inherited);
    delegate->SetTime(0);
}

template <typename T>
static VtArray<T>
_BuildArray(T values[], int numValues)
{
    VtArray<T> result(numValues);
    std::copy(values, values+numValues, result.begin());
    return result;
}

static void
GeomSubsetResyncTest()
{
    std::cout << "-------------------------------------------------------\n";
    std::cout << "GeomSubsetResyncTest\n";
    std::cout << "-------------------------------------------------------\n";

    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    int dirtyBits;

    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> renderIndex(
        HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    TF_AXIOM(renderIndex);
    std::unique_ptr<UsdImagingDelegate> delegate(
                          new UsdImagingDelegate(renderIndex.get(),
                                                  SdfPath::AbsoluteRootPath()));
    HdChangeTracker& tracker = renderIndex->GetChangeTracker();

    // Populate with an empty stage
    delegate->Populate(stage->GetPseudoRoot());
    delegate->SetTime(0);
    delegate->SyncAll(true);

    // Now add a mesh
    UsdGeomMesh cube = UsdGeomMesh::Define(stage, SdfPath("/cube"));

    // Resync
    delegate->SetTime(0);
    delegate->SyncAll(true);

    HdGeomSubsets subsets;

    // Verify topology: initially empty
    subsets = delegate->GetMeshTopology(cube.GetPath()).GetGeomSubsets();
    TF_AXIOM(subsets.size() == 0);

    // Reset dirty bits so we can confirm dirtying on subsequent changes
    tracker.MarkRprimClean(cube.GetPath());
    dirtyBits = tracker.GetRprimDirtyBits(cube.GetPath());
    TF_AXIOM(!(dirtyBits & HdChangeTracker::DirtyTopology));

    // Add a subset
    UsdGeomSubset subset = UsdGeomSubset::CreateGeomSubset(
        cube, TfToken("subset_1"), UsdGeomTokens->face, VtIntArray(), 
        UsdShadeTokens->materialBind);

    // Resync
    delegate->SetTime(0);
    delegate->SyncAll(true);

    // Reset dirty bits so we can confirm dirtying on subsequent changes
    tracker.MarkRprimClean(cube.GetPath());
    dirtyBits = tracker.GetRprimDirtyBits(cube.GetPath());
    TF_AXIOM(!(dirtyBits & HdChangeTracker::DirtyTopology));

    // Verify topology: single subset, no indices
    subsets = delegate->GetMeshTopology(cube.GetPath()).GetGeomSubsets();
    TF_AXIOM(subsets.size() == 1);
    TF_AXIOM(subsets[0].id == SdfPath("/cube/subset_1"));
    TF_AXIOM(subsets[0].materialId == SdfPath());
    TF_AXIOM(subsets[0].type == HdGeomSubset::TypeFaceSet);
    TF_AXIOM(subsets[0].indices.size() == 0);

    // Modify subset indices
    int subset_1_indices[] = {1, 2, 3, 4};
    VtIntArray subset_1_indices_array = _BuildArray(subset_1_indices, 4);
    subset.GetIndicesAttr().Set(subset_1_indices_array);

    // Resync
    delegate->SetTime(0);
    delegate->SyncAll(true);

    // Change tracker should see dirty topology
    dirtyBits = tracker.GetRprimDirtyBits(cube.GetPath());
    TF_AXIOM(dirtyBits & HdChangeTracker::DirtyTopology);

    // Reset dirty bits so we can confirm dirtying on subsequent changes
    tracker.MarkRprimClean(cube.GetPath());
    dirtyBits = tracker.GetRprimDirtyBits(cube.GetPath());
    TF_AXIOM(!(dirtyBits & HdChangeTracker::DirtyTopology));

    // Verify topology: single subset, with expected indices
    subsets = delegate->GetMeshTopology(cube.GetPath()).GetGeomSubsets();
    TF_AXIOM(subsets.size() == 1);
    TF_AXIOM(subsets[0].indices.size() == 4);
    TF_AXIOM(subsets[0].indices == subset_1_indices_array);

    // Remove the subset
    stage->RemovePrim(subset.GetPath());

    // Resync
    delegate->SetTime(0);
    delegate->SyncAll(true);

    // Change tracker should see dirty topology
    dirtyBits = tracker.GetRprimDirtyBits(cube.GetPath());
    TF_AXIOM(dirtyBits & HdChangeTracker::DirtyTopology);

    // Reset dirty bits so we can confirm dirtying on subsequent changes
    tracker.MarkRprimClean(cube.GetPath());
    dirtyBits = tracker.GetRprimDirtyBits(cube.GetPath());
    TF_AXIOM(!(dirtyBits & HdChangeTracker::DirtyTopology));

    // Verify topology: no subsets
    subsets = delegate->GetMeshTopology(cube.GetPath()).GetGeomSubsets();
    TF_AXIOM(subsets.size() == 0);
}

////////////////////////////////////////////////////////////////////////

static void
MaterialRebindTest()
{
    std::cout << "-------------------------------------------------------\n";
    std::cout << "MaterialRebindTest\n";
    std::cout << "-------------------------------------------------------\n";

    UsdStageRefPtr stage = UsdStage::CreateInMemory();

    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> renderIndex(
        HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    TF_AXIOM(renderIndex);
    std::unique_ptr<UsdImagingDelegate> delegate(
                          new UsdImagingDelegate(renderIndex.get(),
                                                  SdfPath::AbsoluteRootPath()));
    HdChangeTracker& tracker = renderIndex->GetChangeTracker();

    // Populate with an empty stage
    delegate->Populate(stage->GetPseudoRoot());
    delegate->SetTime(0);
    delegate->SyncAll(true);

    // Add a scope with one child scope and two child cubes
    UsdGeomScope scope = UsdGeomScope::Define(stage, SdfPath("/scope"));
    UsdGeomScope childScope =
    UsdGeomScope::Define(stage, SdfPath("/scope/child"));
    UsdGeomMesh cube1 = UsdGeomMesh::Define(stage, SdfPath("/scope/cube1"));
    UsdGeomMesh cube2 = UsdGeomMesh::Define(stage, SdfPath("/scope/cube2"));
    UsdShadeMaterialBindingAPI scopeBindingAPI = 
        UsdShadeMaterialBindingAPI::Apply(scope.GetPrim());
    UsdShadeMaterialBindingAPI cube1BindingAPI(cube1);
    UsdShadeMaterialBindingAPI cube2BindingAPI =
        UsdShadeMaterialBindingAPI::Apply(cube2.GetPrim());

    // Add test materials
    UsdShadeMaterial material1 =
        UsdShadeMaterial::Define(stage, SdfPath("/material1"));
    UsdShadeOutput materialOut = material1.CreateSurfaceOutput();
    UsdShadeMaterial material2 =
        UsdShadeMaterial::Define(stage, SdfPath("/material2"));
    UsdShadeOutput materialOut2 = material2.CreateSurfaceOutput();
    UsdShadeMaterial material3 =
        UsdShadeMaterial::Define(stage, SdfPath("/material3"));
    UsdShadeOutput materialOut3 = material3.CreateSurfaceOutput();

    // Sync
    delegate->SetTime(0);
    delegate->SyncAll(true);
    tracker.MarkRprimClean(cube1.GetPath());
    tracker.MarkRprimClean(cube2.GetPath());
    TF_AXIOM(!(tracker.GetRprimDirtyBits(cube1.GetPath())
                & HdChangeTracker::DirtyMaterialId));
    TF_AXIOM(!(tracker.GetRprimDirtyBits(cube2.GetPath())
                & HdChangeTracker::DirtyMaterialId));

    // Set binding on parent scope.
    // Expect the scope binding to inherit to apply to both child cubes.
    scopeBindingAPI.Bind(material1);
    delegate->SetTime(0);
    TF_AXIOM(tracker.GetRprimDirtyBits(cube1.GetPath())
              & HdChangeTracker::DirtyMaterialId);
    TF_AXIOM(tracker.GetRprimDirtyBits(cube2.GetPath())
              & HdChangeTracker::DirtyMaterialId);
    delegate->SyncAll(true);
    tracker.MarkRprimClean(cube1.GetPath());
    tracker.MarkRprimClean(cube2.GetPath());

    // Now set a new binding directly on cube2.
    // This should set DirtyMaterialId on cube2, but not cube1.
    cube2BindingAPI.Bind(material2);
    delegate->SetTime(0);
    TF_AXIOM(!(tracker.GetRprimDirtyBits(cube1.GetPath())
              & HdChangeTracker::DirtyMaterialId));
    TF_AXIOM(tracker.GetRprimDirtyBits(cube2.GetPath())
              & HdChangeTracker::DirtyMaterialId);
    delegate->SyncAll(true);
    tracker.MarkRprimClean(cube1.GetPath());
    tracker.MarkRprimClean(cube2.GetPath());

    // Next, set up a collection-based binding on the child scope.
    // We place the collection here to confirm the ability of
    // a collection to refer to other prims outside the subtree where
    // the collection lives.
    // This should set DirtyMaterialId on both cubes,
    // since the collection-based binding overrides the direct bindings.
    UsdCollectionAPI childCollection = UsdCollectionAPI::Apply(
        childScope.GetPrim(), TfToken("collection"));
    childCollection.IncludePath(cube1.GetPath());
    scopeBindingAPI.Bind(childCollection, material3);
    delegate->SetTime(0);
    TF_AXIOM(tracker.GetRprimDirtyBits(cube1.GetPath())
               & HdChangeTracker::DirtyMaterialId);
    // XXX Note that currently, cube2 will *also* get DirtyMaterial,
    // due to conservative over-invalidation.  If we tighten that in
    // the future, we should be able to verify that cube2 is NOT
    // dirty at this point, i.e.:
    //
    // TF_AXIOM(!(tracker.GetRprimDirtyBits(cube2.GetPath())
    //             & HdChangeTracker::DirtyMaterialId));
    delegate->SyncAll(true);
    tracker.MarkRprimClean(cube1.GetPath());
    tracker.MarkRprimClean(cube2.GetPath());

    // Now modify the collection to include cube2.
    childCollection.IncludePath(cube2.GetPath());
    delegate->SetTime(0);
    TF_AXIOM(tracker.GetRprimDirtyBits(cube2.GetPath())
              & HdChangeTracker::DirtyMaterialId);
}

////////////////////////////////////////////////////////////////////////
//
static void
CoordSysMultiApplyTest()
{
    std::cout << "-------------------------------------------------------\n";
    std::cout << "CoordSysMultiApplyTest\n";
    std::cout << "-------------------------------------------------------\n";

    UsdStageRefPtr stage = UsdStage::CreateInMemory();

    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex>
        renderIndex(HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    TF_AXIOM(renderIndex);
    TF_AXIOM(renderIndex->IsSprimTypeSupported(HdPrimTypeTokens->coordSys));
    std::unique_ptr<UsdImagingDelegate> delegate(
        new UsdImagingDelegate(renderIndex.get(),
                               SdfPath::AbsoluteRootPath()));

    // Populate with an empty stage
    delegate->Populate(stage->GetPseudoRoot());
    delegate->SetTime(0);
    delegate->SyncAll(true);

    UsdGeomXform model = UsdGeomXform::Define(stage, SdfPath("/model"));
    UsdGeomSphere sphere =
        UsdGeomSphere::Define(stage, SdfPath("/model/sphere"));
    UsdGeomCube cube =
        UsdGeomCube::Define(stage, SdfPath("/model/cube"));
    UsdShadeCoordSysAPI::Apply(model.GetPrim(), TfToken("modelSpace"));
    UsdShadeCoordSysAPI::Apply(model.GetPrim(), TfToken("customSpace"));
    UsdShadeCoordSysAPI::Apply(model.GetPrim(), TfToken("missingSpace"));
    UsdShadeCoordSysAPI::Apply(sphere.GetPrim(), TfToken("sphereSpace"));

    UsdShadeCoordSysAPI coordSysAPI;
    coordSysAPI = UsdShadeCoordSysAPI(model.GetPrim(), TfToken("modelSpace"));
    coordSysAPI.CreateBindingRel();
    coordSysAPI = UsdShadeCoordSysAPI(model.GetPrim(), TfToken("customSpace"));
    coordSysAPI.CreateBindingRel();
    coordSysAPI = UsdShadeCoordSysAPI(model.GetPrim(), TfToken("missingSpace"));
    coordSysAPI.CreateBindingRel();
    coordSysAPI = UsdShadeCoordSysAPI(sphere.GetPrim(), TfToken("sphereSpace"));
    coordSysAPI.CreateBindingRel();

    SdfPath modelSpace("/model.coordSys:modelSpace:binding");
    SdfPath customSpace("/model.coordSys:customSpace:binding");
    SdfPath missingSpace("/model.coordSys:missingSpace:binding");
    SdfPath sphereSpace("/model/sphere.coordSys:sphereSpace:binding");

    delegate->ApplyPendingUpdates();
    delegate->SetTime(0);
    delegate->SyncAll(true);

    // No sprims yet
    TF_AXIOM(!renderIndex->GetSprim(HdPrimTypeTokens->coordSys, modelSpace));
    TF_AXIOM(!renderIndex->GetSprim(HdPrimTypeTokens->coordSys, sphereSpace));
    TF_AXIOM(!renderIndex->GetSprim(HdPrimTypeTokens->coordSys, missingSpace));
    TF_AXIOM(!renderIndex->GetSprim(HdPrimTypeTokens->coordSys, customSpace));

    // No bindings either
    TF_AXIOM(!delegate->GetCoordSysBindings(sphere.GetPath()));
    TF_AXIOM(!delegate->GetCoordSysBindings(cube.GetPath()));

    // Bind coordinate systems
    UsdShadeCoordSysAPI::Apply(model.GetPrim(), TfToken("modelSpace")). 
            Bind(SdfPath("/model"));
    UsdShadeCoordSysAPI::Apply(model.GetPrim(), TfToken("customSpace")). 
            Bind(SdfPath("/model/cube"));
    UsdShadeCoordSysAPI::Apply(model.GetPrim(), TfToken("missingSpace")). 
            Bind(SdfPath("/model/missing"));
    UsdShadeCoordSysAPI::Apply(sphere.GetPrim(), TfToken("sphereSpace")). 
            Bind(SdfPath("/model/sphere"));

    delegate->ApplyPendingUpdates();
    delegate->SetTime(0);
    delegate->SyncAll(true);

    // Sprims should now exist, and have expected names
    TF_AXIOM(renderIndex->GetSprim(HdPrimTypeTokens->coordSys, sphereSpace));
    TF_AXIOM(renderIndex->GetSprim(HdPrimTypeTokens->coordSys, customSpace));
    TF_AXIOM(
        dynamic_cast<const HdCoordSys*>(
            renderIndex->GetSprim(HdPrimTypeTokens->coordSys, sphereSpace))
          ->GetName() == TfToken("sphereSpace"));
    TF_AXIOM(
        dynamic_cast<const HdCoordSys*>(
            renderIndex->GetSprim(HdPrimTypeTokens->coordSys, customSpace))
          ->GetName() == TfToken("customSpace"));

    // Missing coordSys should be disregarded (but produces a warning)
    TF_AXIOM(!renderIndex->GetSprim(HdPrimTypeTokens->coordSys, missingSpace));

    // Sprim initial xforms are identity
    TF_AXIOM(delegate->GetTransform(customSpace) == GfMatrix4d(1.0));
    TF_AXIOM(delegate->GetTransform(sphereSpace) == GfMatrix4d(1.0));

    // Cube sees: modelSphere, customSpace
    TF_AXIOM(delegate->GetCoordSysBindings(cube.GetPath())->size() == 2);
    // Sphere sees: modelSphere, customSpace, sphereSpace
    TF_AXIOM(delegate->GetCoordSysBindings(sphere.GetPath())->size() == 3);

    // Set transform values
    model.AddTranslateOp().Set(GfVec3d(1, 0, 0));
    sphere.AddTranslateOp().Set(GfVec3d(0, 1, 0));
    cube.AddTranslateOp().Set(GfVec3d(0, 0, 1));

    delegate->ApplyPendingUpdates();
    delegate->SetTime(0);
    delegate->SyncAll(true);

    // Sprim xforms should now reflect inherited xforms
    GfMatrix4d xf;
    xf.SetTranslate(GfVec3d(1, 1, 0));
    TF_AXIOM(delegate->GetTransform(sphereSpace) == xf);
    xf.SetTranslate(GfVec3d(1, 0, 1));
    TF_AXIOM(delegate->GetTransform(customSpace) == xf);

    // Remove bindings
    UsdShadeCoordSysAPI::Apply(model.GetPrim(), TfToken("modelSpace")).
        ClearBinding(true);
    UsdShadeCoordSysAPI::Apply(model.GetPrim(), TfToken("customSpace")).
        ClearBinding(true);
    UsdShadeCoordSysAPI::Apply(model.GetPrim(), TfToken("missingSpace")).
        ClearBinding(true);
    UsdShadeCoordSysAPI::Apply(sphere.GetPrim(), TfToken("sphereSpace")).
        ClearBinding(true);

    delegate->ApplyPendingUpdates();
    delegate->SetTime(0);
    delegate->SyncAll(true);

    // Sprims should be gone
    TF_AXIOM(!renderIndex->GetSprim(HdPrimTypeTokens->coordSys, modelSpace));
    TF_AXIOM(!renderIndex->GetSprim(HdPrimTypeTokens->coordSys, sphereSpace));
    TF_AXIOM(!renderIndex->GetSprim(HdPrimTypeTokens->coordSys, customSpace));
}

static void
CoordSysTestDeprecated()
{
    std::cout << "-------------------------------------------------------\n";
    std::cout << "CoordSysTestDeprecated\n";
    std::cout << "-------------------------------------------------------\n";

    UsdStageRefPtr stage = UsdStage::CreateInMemory();

    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex>
        renderIndex(HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    TF_AXIOM(renderIndex);
    TF_AXIOM(renderIndex->IsSprimTypeSupported(HdPrimTypeTokens->coordSys));
    std::unique_ptr<UsdImagingDelegate> delegate(
        new UsdImagingDelegate(renderIndex.get(),
                               SdfPath::AbsoluteRootPath()));

    // Populate with an empty stage
    delegate->Populate(stage->GetPseudoRoot());
    delegate->SetTime(0);
    delegate->SyncAll(true);

    UsdGeomXform model = UsdGeomXform::Define(stage, SdfPath("/model"));
    UsdGeomSphere sphere =
        UsdGeomSphere::Define(stage, SdfPath("/model/sphere"));
    UsdGeomCube cube =
        UsdGeomCube::Define(stage, SdfPath("/model/cube"));
    // In order to conform to UsdShadeCoordSysAPI constructor as its a
    // multi-apply API schema now!
    UsdShadeCoordSysAPI modelCoordAPI(model.GetPrim(), TfToken("modelSpace"));
    UsdShadeCoordSysAPI sphereCoordAPI(sphere.GetPrim(), TfToken("sphereSpace"));
    SdfPath modelSpace("/model.coordSys:modelSpace");
    SdfPath customSpace("/model.coordSys:customSpace");
    SdfPath missingSpace("/model.coordSys:missingSpace");
    SdfPath sphereSpace("/model/sphere.coordSys:sphereSpace");

    delegate->ApplyPendingUpdates();
    delegate->SetTime(0);
    delegate->SyncAll(true);

    // No sprims yet
    TF_AXIOM(!renderIndex->GetSprim(HdPrimTypeTokens->coordSys, modelSpace));
    TF_AXIOM(!renderIndex->GetSprim(HdPrimTypeTokens->coordSys, sphereSpace));
    TF_AXIOM(!renderIndex->GetSprim(HdPrimTypeTokens->coordSys, missingSpace));
    TF_AXIOM(!renderIndex->GetSprim(HdPrimTypeTokens->coordSys, customSpace));

    // No bindings either
    TF_AXIOM(!delegate->GetCoordSysBindings(sphere.GetPath()));
    TF_AXIOM(!delegate->GetCoordSysBindings(cube.GetPath()));

    // Bind coordinate systems
    modelCoordAPI.Bind(TfToken("modelSpace"), SdfPath("/model"));
    modelCoordAPI.Bind(TfToken("customSpace"), SdfPath("/model/cube"));
    modelCoordAPI.Bind(TfToken("missingSpace"), SdfPath("/model/missing"));
    sphereCoordAPI.Bind(TfToken("sphereSpace"), SdfPath("/model/sphere"));

    delegate->ApplyPendingUpdates();
    delegate->SetTime(0);
    delegate->SyncAll(true);

    // Sprims should now exist, and have expected names
    TF_AXIOM(renderIndex->GetSprim(HdPrimTypeTokens->coordSys, sphereSpace));
    TF_AXIOM(renderIndex->GetSprim(HdPrimTypeTokens->coordSys, customSpace));
    TF_AXIOM(
        dynamic_cast<const HdCoordSys*>(
            renderIndex->GetSprim(HdPrimTypeTokens->coordSys, sphereSpace))
          ->GetName() == TfToken("sphereSpace"));
    TF_AXIOM(
        dynamic_cast<const HdCoordSys*>(
            renderIndex->GetSprim(HdPrimTypeTokens->coordSys, customSpace))
          ->GetName() == TfToken("customSpace"));

    // Missing coordSys should be disregarded (but produces a warning)
    TF_AXIOM(!renderIndex->GetSprim(HdPrimTypeTokens->coordSys, missingSpace));

    // Sprim initial xforms are identity
    TF_AXIOM(delegate->GetTransform(customSpace) == GfMatrix4d(1.0));
    TF_AXIOM(delegate->GetTransform(sphereSpace) == GfMatrix4d(1.0));

    // Cube sees: modelSphere, customSpace
    TF_AXIOM(delegate->GetCoordSysBindings(cube.GetPath())->size() == 2);
    // Sphere sees: modelSphere, customSpace, sphereSpace
    TF_AXIOM(delegate->GetCoordSysBindings(sphere.GetPath())->size() == 3);

    // Set transform values
    model.AddTranslateOp().Set(GfVec3d(1, 0, 0));
    sphere.AddTranslateOp().Set(GfVec3d(0, 1, 0));
    cube.AddTranslateOp().Set(GfVec3d(0, 0, 1));

    delegate->ApplyPendingUpdates();
    delegate->SetTime(0);
    delegate->SyncAll(true);

    // Sprim xforms should now reflect inherited xforms
    GfMatrix4d xf;
    xf.SetTranslate(GfVec3d(1, 1, 0));
    TF_AXIOM(delegate->GetTransform(sphereSpace) == xf);
    xf.SetTranslate(GfVec3d(1, 0, 1));
    TF_AXIOM(delegate->GetTransform(customSpace) == xf);

    // Remove bindings
    modelCoordAPI.ClearBinding(TfToken("modelSpace"), true);
    modelCoordAPI.ClearBinding(TfToken("customSpace"), true);
    modelCoordAPI.ClearBinding(TfToken("missingSpace"), true);
    sphereCoordAPI.ClearBinding(TfToken("sphereSpace"), true);

    delegate->ApplyPendingUpdates();
    delegate->SetTime(0);
    delegate->SyncAll(true);

    // Sprims should be gone
    TF_AXIOM(!renderIndex->GetSprim(HdPrimTypeTokens->coordSys, modelSpace));
    TF_AXIOM(!renderIndex->GetSprim(HdPrimTypeTokens->coordSys, sphereSpace));
    TF_AXIOM(!renderIndex->GetSprim(HdPrimTypeTokens->coordSys, customSpace));
}

// This is a specific regression test for a bug that would occur when a CoordSys
// is bound to an Xform prim with a renderable (Rprim) descendants and a scene 
// description change occurs that requires the both the Xform and its 
// descendants to be resynced. Prior to the fix of this bug the CoordSys would
// be resynced but none of its descendant Rprims would be dirtied.
// 
// The cause was an optimization in the UsdImagingDelegate's change processing 
// that assumes that all hydra prims are leaf prims in the USD scene (with some 
// very specific exceptions). This optimization would cause the resync of the 
// Xform prim to assume the CoordSys was a leaf prim and stop looking for 
// descendants that need to be resynced, thus the Rprims not being updated.
//
// This test verifies that the condition that would fail before, indeed no 
// longer fails.
static void
CoordSysInHierarchyTest()
{
    // Open the stage that's setup to repro the bug.
    const std::string usdPath = "coordSysRegression/root.usda";
    UsdStageRefPtr stage = UsdStage::Open(usdPath);
    TF_AXIOM(stage);

    // Bring up Hydra
    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex>
        renderIndex(HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    TF_AXIOM(renderIndex->IsSprimTypeSupported(HdPrimTypeTokens->coordSys));
    std::unique_ptr<UsdImagingDelegate> delegate(
        new UsdImagingDelegate(renderIndex.get(),
                               SdfPath::AbsoluteRootPath()));
    HdChangeTracker& tracker = renderIndex->GetChangeTracker();

    // Simple helpers for clarity on dirty tracking, as the tracker has no
    // IsSprimDirty like it does for Rprims.
    auto isRprimDirty = [&tracker](const SdfPath &path) {
         return tracker.IsRprimDirty(path); 
    };
    auto isSprimDirty = [&tracker](const SdfPath &path) {
         return (tracker.GetSprimDirtyBits(path) != HdChangeTracker::Clean); 
    };

    // Populate our stage
    delegate->Populate(stage->GetPseudoRoot());
    delegate->SetTime(0);
    delegate->SyncAll(true);

    // Verify we have a coord sys bindings that create a coord sys depending on 
    // the root Model prim and the Geom child prim and the geom descendant 
    // CubeChild prim.
    SdfPath modelPath("/Model");
    SdfPath modelSpacePath("/Model.coordSys:ModelSpace");
    SdfPath geomPath("/Model/Geom");
    SdfPath geomSpacePath("/Model.coordSys:GeomSpace");
    SdfPath cubeChildPath("/Model/Geom/Cube/CubeChild");
    SdfPath cubeChildSpacePath("/Model.coordSys:CubeChildSpace");

    // XXX: This test is only retrofitted to compile with updated
    // UsdShadeCoordSysAPI API but still uses the deprecated workflow.
    // When RenderIndex code is updated as part of HYD-2754, this test must be
    // cleaned, including its test usda file.
    UsdPrim modelPrim = stage->GetPrimAtPath(modelPath);
    UsdShadeCoordSysAPI::Apply(modelPrim, TfToken("ModelSpace"));

    // consume this prim change to not dirty subsequent changes
    delegate->ApplyPendingUpdates();
    delegate->SetTime(0);
    delegate->SyncAll(true);

    UsdShadeCoordSysAPI csysAPI(stage->GetPrimAtPath(modelPath), 
            TfToken("ModelSpace"));
    TF_AXIOM(csysAPI);
    TF_AXIOM(csysAPI.GetLocalBindings().size() == 3);
    {
        UsdShadeCoordSysAPI::Binding csysBinding = csysAPI.GetLocalBindings()[0];
        TF_AXIOM(csysBinding.bindingRelPath == cubeChildSpacePath);
        TF_AXIOM(csysBinding.coordSysPrimPath == cubeChildPath);
    }
    {
        UsdShadeCoordSysAPI::Binding csysBinding = csysAPI.GetLocalBindings()[1];
        TF_AXIOM(csysBinding.bindingRelPath == geomSpacePath);
        TF_AXIOM(csysBinding.coordSysPrimPath == geomPath);
    }
    {
        UsdShadeCoordSysAPI::Binding csysBinding = csysAPI.GetLocalBindings()[2];
        TF_AXIOM(csysBinding.bindingRelPath == modelSpacePath);
        TF_AXIOM(csysBinding.coordSysPrimPath == modelPath);
    }

    // Verify all CoordSys Sprim exists in the render index and mark them clean.
    TF_AXIOM(renderIndex->GetSprim(HdPrimTypeTokens->coordSys, 
                                    modelSpacePath));
    tracker.MarkSprimClean(modelSpacePath);
    TF_AXIOM(renderIndex->GetSprim(HdPrimTypeTokens->coordSys, 
                                    geomSpacePath));
    tracker.MarkSprimClean(geomSpacePath);
    TF_AXIOM(renderIndex->GetSprim(HdPrimTypeTokens->coordSys, 
                                    cubeChildSpacePath));
    tracker.MarkSprimClean(cubeChildSpacePath);

    // Get our cube prim under the /Model/Geom, mark its Rprim clean as we'll be 
    // changing its points.
    SdfPath modelCubePath("/Model/Geom/Cube");
    UsdPrim cube = stage->GetPrimAtPath(modelCubePath);
    TF_AXIOM(cube);
    tracker.MarkRprimClean(modelCubePath);

    // Get cube's points attribute and get its current value so we can compare
    // it later.
    UsdAttribute ptsAttr = cube.GetAttribute(TfToken("points"));
    VtVec3fArray origPts;
    ptsAttr.Get(&origPts, 0);

    // On the pts layer, change the cube's points.
    SdfLayerHandle ptsLayer = stage->GetRootLayer();
    TF_AXIOM(ptsLayer);
    stage->SetEditTarget(UsdEditTarget(ptsLayer));
    ptsAttr.Set(VtVec3fArray(
        {GfVec3f(-1.5, -0.5, 0.5), 
         GfVec3f(-0.5, -0.5, 0.5), 
         GfVec3f(-1.5, 0.5, 0.5), 
         GfVec3f(-0.5, 0.5, 0.5), 
         GfVec3f(-1.5, 0.5, -0.5), 
         GfVec3f(-0.5, 0.5, -0.5), 
         GfVec3f(-1.5, -0.5, -0.5), 
         GfVec3f(-0.5, -0.5, -0.5)}));

    // Verify the points actually changed on the prim.
    VtVec3fArray pts;
    ptsAttr.Get(&pts, 0);
    TF_AXIOM(pts != origPts);

    // Verify that updating the delegate dirties the cube's Rprim because of
    // the points change. None coord sys Sprims are not dirtied as result of
    // this property change.
    TF_AXIOM(!isSprimDirty(modelSpacePath));
    TF_AXIOM(!isSprimDirty(geomSpacePath));
    TF_AXIOM(!isRprimDirty(modelCubePath));
    TF_AXIOM(!isSprimDirty(cubeChildSpacePath));
    delegate->ApplyPendingUpdates();
    delegate->SetTime(0);
    delegate->SyncAll(true);
    TF_AXIOM(!isSprimDirty(modelSpacePath));
    TF_AXIOM(!isSprimDirty(geomSpacePath));
    TF_AXIOM(isRprimDirty(modelCubePath));
    TF_AXIOM(!isSprimDirty(cubeChildSpacePath));

    // Mark the cube's Rprim clean for the next change.
    tracker.MarkRprimClean(modelCubePath);

    // Now directly on the pts layer we remove the name children of /Model. 
    // This includes the points we just authored authored as well as the 
    // specs for /Model/Geom and /Model/Geom/Cube.
    ptsLayer->GetPrimAtPath(modelPath)->SetNameChildren({});

    // Verify the points actually changed back to the original points on the 
    // prim.
    ptsAttr.Get(&pts, 0);
    TF_AXIOM(pts == origPts);

    // Updating the delegate will trigger a refresh of /Model/Geom. Verify that
    // this dirties the geom space CoordSys Sprim that depends on /Model/Geom 
    // as well as the CoordSys Sprim that depends on /Model/Geom/Cube/CubeChild.
    // All descendant CoordSys dependencies are resynced regardless of if they
    // are below "pruning" Rprims.
    // Also verify that Rprim for the child cube is also marked dirty. The 
    // ancestor model space that depends /Model is still clean.
    TF_AXIOM(!isSprimDirty(modelSpacePath));
    TF_AXIOM(!isSprimDirty(geomSpacePath));
    TF_AXIOM(!isRprimDirty(modelCubePath));
    TF_AXIOM(!isSprimDirty(cubeChildSpacePath));
    delegate->ApplyPendingUpdates();
    delegate->SetTime(0);
    delegate->SyncAll(true);
    TF_AXIOM(!isSprimDirty(modelSpacePath));
    TF_AXIOM(isSprimDirty(geomSpacePath));
    TF_AXIOM(isRprimDirty(modelCubePath));
    TF_AXIOM(isSprimDirty(cubeChildSpacePath));
}

////////////////////////////////////////////////////////////////////////

static void
NestedInstancerCrashTest()
{
    std::cout << "-------------------------------------------------------\n";
    std::cout << "NestedInstancerCrashTest\n";
    std::cout << "-------------------------------------------------------\n";

    const std::string usdPath = "case1/first.usda";
    UsdStageRefPtr stage = UsdStage::Open(usdPath);
    TF_AXIOM(stage);
    
    // Bring up Hydra
    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex>
        renderIndex(HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    std::unique_ptr<UsdImagingDelegate> delegate(
        new UsdImagingDelegate(renderIndex.get(),
                               SdfPath::AbsoluteRootPath()));
    delegate->Populate(stage->GetPseudoRoot());
    delegate->SetTime(0);
    delegate->SyncAll(true);

    // Make layer edit
    const std::string layerPath = "case1/second.usda";
    SdfLayerRefPtr secondLayer = SdfLayer::FindOrOpen(layerPath);
    stage->GetRootLayer()->TransferContent(secondLayer);

    // Resync Hydra -- should not crash
    delegate->ApplyPendingUpdates();
    delegate->SetTime(0);
    delegate->SyncAll(true);
}

static void
InstanceTransformTest()
{
    std::cout << "-------------------------------------------------------\n";
    std::cout << "InstanceTransformTest\n";
    std::cout << "-------------------------------------------------------\n";

    // Case 1...
    {
    const std::string usdPath = "instance_changes1.usda";
    UsdStageRefPtr stage = UsdStage::Open(usdPath);
    TF_AXIOM(stage);
    
    // Bring up Hydra
    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex>
        renderIndex(HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    std::unique_ptr<UsdImagingDelegate> delegate(
        new UsdImagingDelegate(renderIndex.get(),
                               SdfPath::AbsoluteRootPath()));
    HdChangeTracker& tracker = renderIndex->GetChangeTracker();

    delegate->Populate(stage->GetPseudoRoot());
    delegate->SetTime(0);
    delegate->SyncAll(true);
    tracker.MarkRprimClean(SdfPath("/geo_1.proto_cube_id0"));

    // Set /geo/cube transform to translate(0,4,0)
    UsdGeomXformable xf = UsdGeomXformable(stage->GetPrimAtPath(SdfPath("/geo/cube")));
    xf.SetXformOpOrder(std::vector<UsdGeomXformOp>());
    UsdGeomXformOp xfOp = xf.AddTransformOp();
    xfOp.Set(GfMatrix4d(GfMatrix3d(1), GfVec3d(0,4,0)), UsdTimeCode::Default());

    // Process changes.
    delegate->SetTime(0);

    // Verify that /geo_1.cube_id0 has DirtyTransform
    HdDirtyBits dirtyBits = tracker.GetRprimDirtyBits(SdfPath("/geo_1.proto_cube_id0"));
    TF_AXIOM(dirtyBits & HdChangeTracker::DirtyTransform);
    }

    // Case 2...
    {
    const std::string usdPath = "instance_changes2.usda";
    UsdStageRefPtr stage = UsdStage::Open(usdPath);
    TF_AXIOM(stage);
    
    // Bring up Hydra
    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex>
        renderIndex(HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    std::unique_ptr<UsdImagingDelegate> delegate(
        new UsdImagingDelegate(renderIndex.get(),
                               SdfPath::AbsoluteRootPath()));
    HdChangeTracker& tracker = renderIndex->GetChangeTracker();

    delegate->Populate(stage->GetPseudoRoot());
    delegate->SetTime(0);
    delegate->SyncAll(true);

    UsdPrim instancePrim =
        stage->GetPrimAtPath(SdfPath("/Root/InstanceParent1"));
    TF_AXIOM(instancePrim.IsInstance());

    SdfPath prototype = instancePrim.GetPrototype().GetPath();
    SdfPath prototypeBoxes1 = prototype.AppendChild(TfToken("Boxes1"));
    
    TF_AXIOM(renderIndex->HasInstancer(prototypeBoxes1));
    tracker.MarkInstancerClean(prototypeBoxes1);
    
    // Set /Root/InstanceParent1 transform to translate(1,2,3)
    UsdGeomXformable xf = UsdGeomXformable(instancePrim);
    xf.SetXformOpOrder(std::vector<UsdGeomXformOp>());
    UsdGeomXformOp xfOp = xf.AddTransformOp();
    xfOp.Set(GfMatrix4d(GfMatrix3d(1), GfVec3d(1,2,3)), UsdTimeCode::Default());

    // Process changes.
    delegate->SetTime(0);

    // Verify that /Root/InstanceParent1 has DirtyPrimvar
    TF_AXIOM(renderIndex->HasInstancer(prototypeBoxes1));
    HdDirtyBits dirtyBits = tracker.GetInstancerDirtyBits(prototypeBoxes1);
    TF_AXIOM(dirtyBits & HdChangeTracker::DirtyPrimvar);
    }
}

static void
InheritedPrimvarsTest()
{
    std::cout << "-------------------------------------------------------\n";
    std::cout << "InheritedPrimvarsTest\n";
    std::cout << "-------------------------------------------------------\n";

    const std::string usdPath = "inherited_primvars.usda";
    UsdStageRefPtr stage = UsdStage::Open(usdPath);
    TF_AXIOM(stage);
    
    // Bring up Hydra
    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex>
        renderIndex(HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    std::unique_ptr<UsdImagingDelegate> delegate(
        new UsdImagingDelegate(renderIndex.get(),
                               SdfPath::AbsoluteRootPath()));
    HdChangeTracker& tracker = renderIndex->GetChangeTracker();

    delegate->Populate(stage->GetPseudoRoot());
    delegate->SetTime(0);
    delegate->SyncAll(true);
    tracker.MarkRprimClean(SdfPath("/instancer1/Instance0/mesh_0"));

    // Set /instancer1/Instance0.primvars:displayColor = (1,0,1)
    UsdPrim i0 = stage->GetPrimAtPath(SdfPath("/instancer1/Instance0"));
    UsdGeomPrimvar pv = UsdGeomPrimvarsAPI(i0).GetPrimvar(TfToken("displayColor"));
    VtVec3fArray values(1, GfVec3f(1,0,1));
    pv.Set(values);

    // Process changes.
    delegate->SetTime(0);

    // Verify that /instancer/Instance0/mesh_0 has DirtyPrimvar
    HdDirtyBits dirtyBits = tracker.GetRprimDirtyBits(SdfPath("/instancer1/Instance0/mesh_0"));
    TF_AXIOM(dirtyBits & HdChangeTracker::DirtyPrimvar);
}

static void
ReactivatingInstancedPrimTest()
{
    std::cout << "-------------------------------------------------------\n";
    std::cout << "ReactivatingInstancedPrimTest\n";
    std::cout << "-------------------------------------------------------\n";

    const std::string usdPath = "instance_changes2.usda";
    UsdStageRefPtr stage = UsdStage::Open(usdPath);
    TF_AXIOM(stage);
    
    // Bring up Hydra
    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex>
        renderIndex(HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    std::unique_ptr<UsdImagingDelegate> delegate(
        new UsdImagingDelegate(renderIndex.get(),
                               SdfPath::AbsoluteRootPath()));
    HdChangeTracker& tracker = renderIndex->GetChangeTracker();

    delegate->Populate(stage->GetPseudoRoot());
    delegate->SetTime(0);
    delegate->SyncAll(true);

    // Verify # of prims
    size_t nPrims = renderIndex->GetRprimSubtree(SdfPath::AbsoluteRootPath()).size();
    TF_AXIOM(nPrims == 2);

    // Deactivate/Reactivate at the root level
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/Root"));
    prim.SetActive(false);
    delegate->SetTime(0);

    // We should have depopulated everything.
    nPrims = renderIndex->GetRprimSubtree(SdfPath::AbsoluteRootPath()).size();
    TF_AXIOM(nPrims == 0);

    prim.SetActive(true);
    delegate->SetTime(0);

    // Verify # of prims (make sure we didn't populate prims in the proto root,
    // except through the instance adapter).
    nPrims = renderIndex->GetRprimSubtree(SdfPath::AbsoluteRootPath()).size();
    TF_AXIOM(nPrims == 2);

    // Deactivate/Reactivate the original reference prototype
    UsdPrim prim2 = stage->GetPrimAtPath(SdfPath("/inner/cube1"));
    prim2.SetActive(false);
    delegate->SetTime(0);

    // We should be down to 1 rprim.
    nPrims = renderIndex->GetRprimSubtree(SdfPath::AbsoluteRootPath()).size();
    TF_AXIOM(nPrims == 1);

    prim2.SetActive(true);
    delegate->SetTime(0);

    // Back up to baseline.
    nPrims = renderIndex->GetRprimSubtree(SdfPath::AbsoluteRootPath()).size();
    TF_AXIOM(nPrims == 2);

    const UsdPrim instancePrim =
        stage->GetPrimAtPath(SdfPath("/Root/InstanceParent1"));
    TF_AXIOM(instancePrim.IsInstance());

    const SdfPath instancerPath = 
        instancePrim.GetPrototype().GetPath().AppendChild(TfToken("Boxes1"));

    tracker.MarkInstancerClean(instancerPath);

    // De-activate/re-activate one of the instances and check that the
    // instance count is marked dirty and updated appropriately.
    UsdPrim prim3 = stage->GetPrimAtPath(SdfPath("/Root/InstanceParent2"));
    prim3.SetActive(false);
    delegate->SetTime(0);
    delegate->SyncAll(true);

    // Verify # of instances.
    // Everything in the scene is native instanced together, so just grab the
    // first rprim path.
    SdfPath protoPath = renderIndex->GetRprimSubtree(SdfPath("/"))[0];
    TF_AXIOM(tracker.GetInstancerDirtyBits(instancerPath) & HdChangeTracker::DirtyInstanceIndex);
    TF_AXIOM(delegate->GetInstanceIndices(instancerPath, protoPath).size() == 2);

    tracker.MarkInstancerClean(instancerPath);
    prim3.SetActive(true);
    delegate->SetTime(0);
    delegate->SyncAll(true);

    // Verify # of instances.
    TF_AXIOM(tracker.GetInstancerDirtyBits(instancerPath) & HdChangeTracker::DirtyInstanceIndex);
    TF_AXIOM(delegate->GetInstanceIndices(instancerPath, protoPath).size() == 4);
}

static void
BoundMaterialTest()
{
    std::cout << "-------------------------------------------------------\n";
    std::cout << "BoundMaterialTest\n";
    std::cout << "-------------------------------------------------------\n";

    const std::string usdPath = "boundMaterial.usda";
    UsdStageRefPtr stage = UsdStage::Open(usdPath);
    TF_AXIOM(stage);
    
    // Bring up Hydra
    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex>
        renderIndex(HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    std::unique_ptr<UsdImagingDelegate> delegate(
        new UsdImagingDelegate(renderIndex.get(),
                               SdfPath::AbsoluteRootPath()));
    HdChangeTracker& tracker = renderIndex->GetChangeTracker();

    delegate->Populate(stage->GetPseudoRoot());
    delegate->SetTime(0);
    delegate->SyncAll(true);
    tracker.MarkRprimClean(SdfPath("/World/Sphere"));
    tracker.MarkSprimClean(SdfPath("/World/Material"));

    // De-activate/re-activate the material.
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/World/Material"));
    prim.SetActive(false);
    delegate->SetTime(0);

    // Check DirtyMaterialId on the gprim, and check that /World/Material is
    // de-populated.
    HdDirtyBits dirtyBits = tracker.GetRprimDirtyBits(SdfPath("/World/Sphere"));
    TF_AXIOM(dirtyBits & HdChangeTracker::DirtyMaterialId);
    TF_AXIOM(renderIndex->GetSprim(HdPrimTypeTokens->material,
                                    SdfPath("/World/Material")) == nullptr);

    delegate->SyncAll(true);
    tracker.MarkRprimClean(SdfPath("/World/Sphere"));

    prim.SetActive(true);
    delegate->SetTime(0);

    // Check DirtyMaterialId on the gprim, and check that /World/Material is
    // re-populated.
    dirtyBits = tracker.GetRprimDirtyBits(SdfPath("/World/Sphere"));
    TF_AXIOM(dirtyBits & HdChangeTracker::DirtyMaterialId);
    TF_AXIOM(renderIndex->GetSprim(HdPrimTypeTokens->material,
                                    SdfPath("/World/Material")) != nullptr);
}

static void
ShaderResyncTest()
{
    std::cout << "-------------------------------------------------------\n";
    std::cout << "ShaderResyncTest\n";
    std::cout << "-------------------------------------------------------\n";

    const std::string usdPath = "shaderResync.usda";
    UsdStageRefPtr stage = UsdStage::Open(usdPath);
    TF_AXIOM(stage);

    // Bring up Hydra
    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex>
        renderIndex(HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    std::unique_ptr<UsdImagingDelegate> delegate(
        new UsdImagingDelegate(renderIndex.get(),
                               SdfPath::AbsoluteRootPath()));

    delegate->Populate(stage->GetPseudoRoot());
    delegate->SetTime(0);
    delegate->SyncAll(true);

    // Verify the material exists.
    TF_AXIOM(renderIndex->GetSprim(HdPrimTypeTokens->material,
                                    SdfPath("/World/Material")) != nullptr);

    // Resync the shader.
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/World/Material/PbrPreview"));
    prim.GetVariantSet("color").SetVariantSelection("green");
    delegate->SetTime(0);

    // Verify the material exists.
    TF_AXIOM(renderIndex->GetSprim(HdPrimTypeTokens->material,
                                    SdfPath("/World/Material")) != nullptr);
}

static void
InstancerMultipleEditTest()
{
    std::cout << "-------------------------------------------------------\n";
    std::cout << "InstancerMultipleEditTest\n";
    std::cout << "-------------------------------------------------------\n";

    const std::string usdPath = "instance_changes1.usda";
    UsdStageRefPtr stage = UsdStage::Open(usdPath);
    TF_AXIOM(stage);

    // Bring up Hydra
    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex>
        renderIndex(HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    std::unique_ptr<UsdImagingDelegate> delegate(
        new UsdImagingDelegate(renderIndex.get(),
                               SdfPath::AbsoluteRootPath()));

    delegate->Populate(stage->GetPseudoRoot());
    delegate->SetTime(0);
    delegate->SyncAll(true);

    UsdGeomXformable xf(stage->GetPrimAtPath(SdfPath("/geo_2")));
    UsdGeomXformOp t = xf.AddTranslateOp();
    t.Set(GfVec3d(1,2,3));
    UsdPrim prim2 = stage->GetPrimAtPath(SdfPath("/geo_1"));
    prim2.SetInstanceable(false);

    // Process changes.
    delegate->SetTime(0);
    delegate->SyncAll(true);
}

static void
DelegateDependencyMapTest()
{
    // The USD->Hydra dependency map (UsdImagingDelegate::__dependencyInfo)is an
    // implementation detail of UsdImagingDelegate that tracks the Hydra prims
    // inserted/affected by a USD prim.
    //
    // It can be a hotspot in certain workflows (e.g., editing such that several
    // resync notices are generated) because it isn't a thread safe map, and
    // insertion/deletion is performed serially as of this writing.
    // Attepts to parallelize it via an additional cache have resulted
    // in bugs. This test case exercies one of those scenarios.

    std::cout << "-------------------------------------------------------\n";
    std::cout << "DelegateDependencyMapTest\n";
    std::cout << "-------------------------------------------------------\n";

    // Create a stage, add a few prims and call Populate.
    // This should insert a Hydra prim corresponding to the Cube prim, and thus
    // an entry in the dependency map.
    //
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> renderIndex(
        HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    TF_AXIOM(renderIndex);
    std::unique_ptr<UsdImagingDelegate> delegate(
                          new UsdImagingDelegate(renderIndex.get(),
                                                  SdfPath::AbsoluteRootPath()));
    UsdGeomXform xf1 = UsdGeomXform::Define(stage, SdfPath("/Xf1"));
    UsdGeomCube cube1 = UsdGeomCube::Define(stage, SdfPath("/Xf1/Cube1"));
    
    delegate->Populate(stage->GetPseudoRoot());

    // Verify entry by selecting the cube1 prim.
    {
        HdSelectionSharedPtr selection = std::make_shared<HdSelection>();
        delegate->PopulateSelection(
            HdSelection::HighlightModeSelect,
            SdfPath("/Xf1/Cube1"),
            /*instanceId*/0,
            selection);

        TF_VERIFY(!selection->IsEmpty(),
            "HdSelection is empty (should have one entry for /Xf1/Cube1).\n");
    }

    // Add a new prim. This will trigger a resync notice for the subtree that
    // the prim is inserted at.
    UsdGeomCube cube2 = UsdGeomCube::Define(stage, SdfPath("/Xf1/Cube2"));

    // Process the resync notice by calling ApplyPendingUpdates
    // (Note: PopulateSelection, below, calls ApplyPendingUpdates as well, so
    //        this isn't strictly necessary).
    delegate->ApplyPendingUpdates();

    // Verify entry by selecting the cube2 prim.
    {
        HdSelectionSharedPtr selection = std::make_shared<HdSelection>();
        delegate->PopulateSelection(
            HdSelection::HighlightModeSelect,
            SdfPath("/Xf1/Cube2"),
            /*instanceId*/0,
            selection);

        TF_VERIFY(!selection->IsEmpty(),
            "HdSelection is empty (should have one entry for /Xf1/Cube2).\n");
    }
}

int main()
{
    TfErrorMark mark;

    PrimResyncTest();
    PrimHierarchyResyncTest();
    VisibilityTest();
    PrimExpiredTest(&mark);
    PrimAndCollectionExpiredTest();
    InstancePrimResyncTest();
    GeomSubsetResyncTest();
    SparsePrimResyncTest();
    MaterialRebindTest();
    CoordSysMultiApplyTest();
    CoordSysTestDeprecated();
    CoordSysInHierarchyTest();
    NestedInstancerCrashTest();
    InstanceTransformTest();
    InheritedPrimvarsTest();
    ReactivatingInstancedPrimTest();
    BoundMaterialTest();
    ShaderResyncTest();
    InstancerMultipleEditTest();
    DelegateDependencyMapTest();

    if (TF_AXIOM(mark.IsClean()))
        std::cout << "OK" << std::endl;
    else
        std::cout << "FAILED" << std::endl;
}

