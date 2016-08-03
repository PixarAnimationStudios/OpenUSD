#include "pxr/usdImaging/usdImaging/unitTestHelper.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/dirtyList.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/treeIterator.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/xform.h"

#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/gf/frustum.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>

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
    UsdImagingDelegate delegate;
    HdChangeTracker& tracker = delegate.GetRenderIndex().GetChangeTracker();

    // Populate the empty stage
    delegate.Populate(stage->GetPseudoRoot());

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
    delegate.SetTime(0);
    std::cerr << "--------------------------------------------------------------------------------\n"
              << "END WARNINGS\n\n";

    // We expect the prims to be fully dirty, as they were just inserted.
    dirtyBits = tracker.GetRprimDirtyBits(mesh1.GetPath());
    TF_VERIFY(dirtyBits & HdChangeTracker::AllDirty);
    tracker.MarkRprimClean(mesh1.GetPath());

    dirtyBits = tracker.GetRprimDirtyBits(mesh2.GetPath());
    TF_VERIFY(dirtyBits & HdChangeTracker::AllDirty);
    tracker.MarkRprimClean(mesh2.GetPath());

    dirtyBits = tracker.GetRprimDirtyBits(cube.GetPath());
    TF_VERIFY(dirtyBits & HdChangeTracker::AllDirty);
    tracker.MarkRprimClean(cube.GetPath());

    dirtyBits = tracker.GetRprimDirtyBits(cube2.GetPath());
    TF_VERIFY(dirtyBits & HdChangeTracker::AllDirty);
    tracker.MarkRprimClean(cube2.GetPath());

    VtVec3fArray points1(3);
    points1[0] = GfVec3f(1,0,0);
    points1[1] = GfVec3f(0,2,0);
    points1[2] = GfVec3f(0,0,3);

    mesh1.GetPointsAttr().Set(points1);
    delegate.SetTime(0);

    VtVec3fArray points2(3);
    points2[0] = GfVec3f(4,0,0);
    points2[1] = GfVec3f(0,5,0);
    points2[2] = GfVec3f(0,0,6);
    mesh2.GetPointsAttr().Set(points2);

    cube.GetSizeAttr().Set(1.0);
    UsdGeomXformOp cube2XformOp = cube2.AddTransformOp();
    cube2XformOp.Set(GfMatrix4d(1), UsdTimeCode::Default());

    // Process changes. 
    delegate.SetTime(0);

    // NOTE TO FUTURE DEBUGGERS: The first time an attribute gets set, it will
    // trigger a resync, because createing a new PropertySpec is "significant",
    // so now the dirtyBits below are all expected to be AllDirty (-1).

    // Expect dirty points for meshes
    dirtyBits = tracker.GetRprimDirtyBits(mesh1.GetPath());
    TF_VERIFY(dirtyBits & HdChangeTracker::DirtyPoints);
    dirtyBits = tracker.GetRprimDirtyBits(mesh2.GetPath());
    TF_VERIFY(dirtyBits & HdChangeTracker::DirtyPoints);

    // Changing the size should invalidate the transform, not the points.
    dirtyBits = tracker.GetRprimDirtyBits(cube.GetPath());
    TF_VERIFY(dirtyBits & HdChangeTracker::DirtyTransform);

    // Changing the matrix should also invalidate the transform
    dirtyBits = tracker.GetRprimDirtyBits(cube2.GetPath());
    TF_VERIFY(dirtyBits & HdChangeTracker::DirtyTransform);

    // Make sure the values are good
    VtValue value;
    value = delegate.Get(mesh1.GetPath(), UsdGeomTokens->points);
    TF_VERIFY(value.Get<VtVec3fArray>() == points1);
    value = delegate.Get(mesh2.GetPath(), UsdGeomTokens->points);
    TF_VERIFY(value.Get<VtVec3fArray>() == points2);

    // Mark everything as clean.
    tracker.MarkRprimClean(mesh1.GetPath());
    tracker.MarkRprimClean(mesh2.GetPath());
    tracker.MarkRprimClean(cube.GetPath());
    tracker.MarkRprimClean(cube2.GetPath());
    // Process changes. 
    delegate.SetTime(0);

    // We do not expect them to be dirty now, since the points are not actually
    // varying.
    dirtyBits = tracker.GetRprimDirtyBits(mesh1.GetPath());
    TF_VERIFY(not (dirtyBits & HdChangeTracker::DirtyPoints));
    dirtyBits = tracker.GetRprimDirtyBits(mesh2.GetPath());
    TF_VERIFY(not (dirtyBits & HdChangeTracker::DirtyPoints));
    dirtyBits = tracker.GetRprimDirtyBits(cube.GetPath());
    TF_VERIFY(not (dirtyBits & HdChangeTracker::DirtyTransform));
    dirtyBits = tracker.GetRprimDirtyBits(cube2.GetPath());
    TF_VERIFY(not (dirtyBits & HdChangeTracker::DirtyTransform));

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
    delegate.SetTime(1);
    tracker.MarkRprimClean(mesh1.GetPath());
    tracker.MarkRprimClean(mesh2.GetPath());
    tracker.MarkRprimClean(cube.GetPath());
    tracker.MarkRprimClean(cube2.GetPath());
    delegate.SetTime(2);

    // Now expect:
    //      dirtyBits(mesh1) == Clean
    //      dirtyBits(mesh2) == DirtyPoints
    //      dirtyBits(cube)  == DirtyTransform
    //      dirtyBits(cube2)  == DirtyTransform

    // Mesh1 should still be clean, but mesh2 should be marked as dirty.
    dirtyBits = tracker.GetRprimDirtyBits(mesh1.GetPath());
    TF_VERIFY(not (dirtyBits & HdChangeTracker::DirtyPoints));
    
    // Should be dirtyPoints:
    dirtyBits = tracker.GetRprimDirtyBits(mesh2.GetPath());
    TF_VERIFY(dirtyBits & HdChangeTracker::DirtyPoints);

    // Should be dirtyTransform:
    dirtyBits = tracker.GetRprimDirtyBits(cube.GetPath());
    TF_VERIFY(not(dirtyBits & HdChangeTracker::DirtyPoints));
    TF_VERIFY(dirtyBits & HdChangeTracker::DirtyTransform);

    // Should be dirtyTransform:
    dirtyBits = tracker.GetRprimDirtyBits(cube2.GetPath());
    TF_VERIFY(not(dirtyBits & HdChangeTracker::DirtyPoints));
    TF_VERIFY(dirtyBits & HdChangeTracker::DirtyTransform);

    // Verify cube2.transform. The final transform is computed from the
    // cube's size and its transform.
    value = delegate.Get(cube2.GetPath(), HdTokens->transform);
    TF_VERIFY(value.Get<GfMatrix4d>() == GfMatrix4d(3));
    
    value = delegate.Get(cube2.GetPath(), UsdGeomTokens->size);
    TF_VERIFY(value.Get<double>() == 2.0);

    TF_VERIFY(delegate.GetTransform(cube2.GetPath()) == 
              GfMatrix4d(GfVec4d(2.0, 2.0, 2.0, 1.0)) * GfMatrix4d(3.0));

    // Verify mesh2.points
    value = delegate.Get(mesh2.GetPath(), UsdGeomTokens->points);
    TF_VERIFY(value.Get<VtVec3fArray>() == points2);
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
    UsdImagingDelegate delegate;
    HdChangeTracker& tracker = delegate.GetRenderIndex().GetChangeTracker();

    // Populate the empty stage
    delegate.Populate(stage->GetPseudoRoot());

    UsdGeomXform xf1 = UsdGeomXform::Define(stage, SdfPath("/Xf1"));

    UsdGeomXform xf2 = UsdGeomXform::Define(stage, SdfPath("/Xf1/Xf2"));
    UsdGeomCube cube1 = UsdGeomCube::Define(stage, SdfPath("/Xf1/Xf2/Cube1"));

    UsdGeomXform xf3 = UsdGeomXform::Define(stage, SdfPath("/Xf1/Xf3"));
    UsdGeomCube cube2 = UsdGeomCube::Define(stage, SdfPath("/Xf1/Xf3/Cube2"));

    // Set time to force a sync.
    delegate.SetTime(0);

    // Expect visibility to be dirty upon creation.
    dirtyBits = tracker.GetRprimDirtyBits(cube1.GetPath());
    TF_VERIFY(dirtyBits & HdChangeTracker::DirtyVisibility);
    tracker.MarkRprimClean(cube1.GetPath());
    dirtyBits = tracker.GetRprimDirtyBits(cube2.GetPath());
    TF_VERIFY(dirtyBits & HdChangeTracker::DirtyVisibility);
    tracker.MarkRprimClean(cube2.GetPath());

    // Process changes. 
    delegate.SetTime(0);

    // NOTE TO FUTURE DEBUGGERS: The first time an attribute gets set, it will
    // trigger a resync, because createing a new PropertySpec is "significant",
    // so now the dirtyBits below are all expected to be AllDirty (-1).
     
    cube1.GetVisibilityAttr().Set(UsdGeomTokens->invisible, 1.0);
    cube2.GetVisibilityAttr().Set(UsdGeomTokens->invisible, 1.0);

    // Notices get sent upon setting the value, however they accumulate in the
    // delegate until SetTime is called, so we expect no dirtiness yet.
    dirtyBits = tracker.GetRprimDirtyBits(cube1.GetPath());
    TF_VERIFY(not(dirtyBits & HdChangeTracker::DirtyVisibility));
    dirtyBits = tracker.GetRprimDirtyBits(cube2.GetPath());
    TF_VERIFY(not(dirtyBits & HdChangeTracker::DirtyVisibility));

    // Process changes.
    delegate.SetTime(0);

    // Expect dirty visibility.
    dirtyBits = tracker.GetRprimDirtyBits(cube1.GetPath());
    TF_VERIFY(dirtyBits & HdChangeTracker::DirtyVisibility);
    dirtyBits = tracker.GetRprimDirtyBits(cube2.GetPath());
    TF_VERIFY(dirtyBits & HdChangeTracker::DirtyVisibility);

    // Make sure the values are good
    TF_VERIFY(not delegate.GetVisible(cube1.GetPath()));
    TF_VERIFY(not delegate.GetVisible(cube2.GetPath()));

    // Mark everything as clean.
    tracker.MarkRprimClean(cube1.GetPath());
    tracker.MarkRprimClean(cube2.GetPath());

    // Setting the time should flag them as dirty again
    delegate.SetTime(1.0);

    // We do not expect them to be dirty now, since the vis is not actually
    // varying.
    dirtyBits = tracker.GetRprimDirtyBits(cube1.GetPath());
    TF_VERIFY(not(dirtyBits & HdChangeTracker::DirtyVisibility));
    dirtyBits = tracker.GetRprimDirtyBits(cube2.GetPath());
    TF_VERIFY(not(dirtyBits & HdChangeTracker::DirtyVisibility));

    // Animate cube size.
    cube1.GetVisibilityAttr().Set(UsdGeomTokens->inherited, 1.0);
    cube2.GetVisibilityAttr().Set(UsdGeomTokens->inherited, 1.0);
    
    // Process Changes.
    delegate.SetTime(1.0);

    dirtyBits = tracker.GetRprimDirtyBits(cube1.GetPath());
    TF_VERIFY(dirtyBits & HdChangeTracker::DirtyVisibility);
    dirtyBits = tracker.GetRprimDirtyBits(cube2.GetPath());
    TF_VERIFY(dirtyBits & HdChangeTracker::DirtyVisibility);

    // Make sure the values are good
    TF_VERIFY(delegate.GetVisible(cube1.GetPath()));
    TF_VERIFY(delegate.GetVisible(cube2.GetPath()));
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
    UsdImagingDelegate delegate;
    HdChangeTracker& tracker = delegate.GetRenderIndex().GetChangeTracker();
    delegate.Populate(mesh1.GetPrim());

    dirtyBits = tracker.GetRprimDirtyBits(mesh1.GetPath());
    TF_VERIFY(dirtyBits & HdChangeTracker::AllDirty);

    // Delete the root prim.
    SdfLayerHandle layer = stage->GetRootLayer();
    SdfPrimSpecHandle root = layer->GetPrimAtPath(SdfPath::AbsoluteRootPath());
    SdfPrimSpecHandle prim = layer->GetPrimAtPath(mesh1.GetPath());
    root->RemoveNameChild(prim);

    // Process changes, killing the root prim; should not crash. 
    delegate.SetTime(0);

    // Recreate the prim.
    mesh1 = UsdGeomMesh::Define(stage, SdfPath("/Mesh1"));
    mesh1.GetPointsAttr().Set(points);

#if 0 // Unfortunately, mentor doesn't let us do this :(

    // Though the new prim was created, that change should not yet have been
    // processed.
    {
        TF_VERIFY(mark->IsClean());
        std::cerr << "\nBEGIN EXPECTED ERROR\n"
                  << "--------------------------------------------------------------------------------\n";
        tracker.GetRprimDirtyBits(mesh1.GetPath());
        std::cerr << "--------------------------------------------------------------------------------\n"
                  << "END ERROR\n\n";
        TF_VERIFY(not mark->IsClean());
        mark->Clear();
    }
#endif 

    // Process the change that restored the prim, then expect all normal API to
    // resume functioning.
    delegate.SetTime(0);
    TF_VERIFY(delegate.GetVisible(mesh1.GetPath()));
    dirtyBits = tracker.GetRprimDirtyBits(mesh1.GetPath());
    TF_VERIFY(dirtyBits & HdChangeTracker::AllDirty);
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
    UsdImagingDelegate delegate;
    HdChangeTracker& tracker = delegate.GetRenderIndex().GetChangeTracker();
    delegate.Populate(cube1.GetPrim());
    delegate.SetTime(0);

    HdRprimCollection collection(HdTokens->geometry, HdTokens->hull);
    HdDirtyListSharedPtr dirtyList(
        new HdDirtyList(collection, delegate.GetRenderIndex()));

    SdfPathVector dirtyPrims = dirtyList->GetDirtyRprims();
    TF_VERIFY(dirtyPrims.size() == 1);
    TF_VERIFY(dirtyPrims.front() == SdfPath("/Xf1/Xf2/Cube1"));
    tracker.MarkRprimClean(cube1.GetPath());
    tracker.ResetVaryingState();

    dirtyPrims = dirtyList->GetDirtyRprims();
    TF_VERIFY(dirtyPrims.empty());
    
    // Set the first time sample on the cubes. This authors new property
    // specs for the size attribute, causing resyncs. The imaging delegate
    // only cares about cube1, so it's still the only thing that should
    // be marked dirty.
    cube1.GetSizeAttr().Set(1.0, 1.0);
    cube2.GetSizeAttr().Set(1.0, 2.0);
    delegate.SetTime(1);

    dirtyPrims = dirtyList->GetDirtyRprims();
    TF_VERIFY(dirtyPrims.size() == 1);
    TF_VERIFY(dirtyPrims.front() == SdfPath("/Xf1/Xf2/Cube1"));
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
        TF_VERIFY(instances[i]);

        instances[i].GetReferences().AddInternal(SdfPath("/Instance"));
        instances[i].SetInstanceable(true);
    }

    UsdImagingDelegate delegate;
    delegate.Populate(stage->GetPseudoRoot());
    delegate.SetTime(0);

    stage->SetEditTarget(stage->GetSessionLayer());

    // Creating the vis attribute should cause a prim resync.
    UsdAttribute visAttr = 
        UsdGeomImageable::Get(stage, SdfPath("/Models/cube_1"))
        .CreateVisibilityAttr();
    visAttr.Set(UsdGeomTokens->invisible);
    delegate.SetTime(0);

    // This should cause just a property change, not a prim resync.
    visAttr.Set(UsdGeomTokens->inherited);
    delegate.SetTime(0);
}

int main()
{
    TfErrorMark mark;

    PrimResyncTest();
    PrimHierarchyResyncTest();
    VisibilityTest();
    PrimExpiredTest(&mark);
    InstancePrimResyncTest();

    if (TF_VERIFY(mark.IsClean()))
        std::cout << "OK" << std::endl;
    else
        std::cout << "FAILED" << std::endl;
}

