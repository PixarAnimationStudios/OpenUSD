//
// Copyright 2021 Pixar
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

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/dirtyList.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/unitTestHelper.h"
#include "pxr/imaging/hd/unitTestNullRenderPass.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

#define _VERIFY_PERF_COUNT(token, count) \
            TF_VERIFY(perfLog.GetCounter(token) == count, \
                    "expected %d found %.0f", \
                    count,\
                    perfLog.GetCounter(token));

#define _VERIFY_DIRTY_SIZE(pass, count) \
        { \
            HdDirtyListSharedPtr dirtyList = pass->GetDirtyList(); \
            SdfPathVector const& dirtyPaths = \
                       dirtyList->GetDirtyRprims(); \
            TF_VERIFY(dirtyPaths.size() == count, \
                        "expected %d found %zu", \
                        count,\
                        dirtyPaths.size());\
       }

void
DirtyListTest()
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyLists, 0);

    Hd_TestDriver driver;
    HdUnitTestDelegate &delegate = driver.GetDelegate();
    HdRenderIndex& index = delegate.GetRenderIndex();

    HdChangeTracker &changeTracker = index.GetChangeTracker();

    SdfPath id("/prim");

    HdRprimCollection col(HdTokens->geometry, 
        HdReprSelector(HdReprTokens->hull));
    HdRenderPassSharedPtr renderPass0 = 
        HdRenderPassSharedPtr(new Hd_UnitTestNullRenderPass(&index, col));

    // creating renderPass allocates 1 dirtyList in the changeTracker.
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyLists, 1);

    // renderPass0
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyLists, 1);

    // no dirty prims at this point
    _VERIFY_DIRTY_SIZE(renderPass0, 0);

    // make dirty
    delegate.AddMesh(id);
    changeTracker.MarkRprimDirty(id, HdChangeTracker::DirtyVisibility);

    // 1 dirty prim
    _VERIFY_DIRTY_SIZE(renderPass0, 1);

    // clean
    changeTracker.ResetVaryingState();
    changeTracker.MarkRprimClean(id, HdChangeTracker::Clean);

    // 0 dirty prim
    _VERIFY_DIRTY_SIZE(renderPass0, 0);

    // hull repr doesn't care about Normals. 
    changeTracker.ResetVaryingState();
    changeTracker.MarkRprimDirty(id, HdChangeTracker::DirtyNormals);

    // however, the dirtylist always includes Varying prims even though
    // they are assumed clean for the repr
    // XXX: we'd like to fix this inefficiency.
    _VERIFY_DIRTY_SIZE(renderPass0, 1);

    // more render passes
    HdRprimCollection collection(HdTokens->geometry, 
        HdReprSelector(HdReprTokens->hull));
    HdRenderPassSharedPtr renderPass1 =
        HdRenderPassSharedPtr(new Hd_UnitTestNullRenderPass(&index, collection));
    HdRenderPassSharedPtr renderPass2 =
        HdRenderPassSharedPtr(new Hd_UnitTestNullRenderPass(&index, collection));

    // make dirty
    changeTracker.ResetVaryingState();
    changeTracker.MarkRprimDirty(id, HdChangeTracker::DirtyVisibility);

    // new render pass. returns 1 dirty prim
    _VERIFY_DIRTY_SIZE(renderPass1, 1);

    // renderPass0:Visibility, renderPass1:Hull, renderPass2:Hull
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyLists, 3);

    // new render pass. returns 1 dirty prim
    _VERIFY_DIRTY_SIZE(renderPass2, 1);

    changeTracker.ResetVaryingState();
    changeTracker.MarkRprimDirty(id, HdChangeTracker::DirtyTopology);
    _VERIFY_DIRTY_SIZE(renderPass0, 1);
    _VERIFY_DIRTY_SIZE(renderPass1, 1);
    _VERIFY_DIRTY_SIZE(renderPass2, 1);

    // clean all.
    changeTracker.ResetVaryingState();
    changeTracker.MarkRprimClean(id, HdChangeTracker::Clean);

    renderPass0.reset();
    changeTracker.ResetVaryingState();

    // renderPass1:Hull, renderPass2:Hull
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyLists, 2);

    renderPass1.reset();

    // renderPass2:Hull
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyLists, 1);

    changeTracker.ResetVaryingState();
    changeTracker.MarkRprimDirty(id, HdChangeTracker::DirtyPrimvar);

    renderPass2.reset();

    // nothing. :)
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyLists, 0);
}

static void
DirtyListTest2()
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    Hd_TestDriver driver;
    GfMatrix4f identity;
    identity.SetIdentity();

    HdUnitTestDelegate &delegate = driver.GetDelegate();
    HdRenderPassSharedPtr const &geomPass = driver.GetRenderPass();
    HdRenderPassSharedPtr const &geomAndGuidePass = driver.GetRenderPass(true);

    _VERIFY_DIRTY_SIZE(geomPass, 0);
    _VERIFY_DIRTY_SIZE(geomAndGuidePass, 0);

    delegate.AddCube(SdfPath("/cube"), identity, /*guide=*/false);
    delegate.AddCube(SdfPath("/guideCube"), identity, /*guide=*/true);

    _VERIFY_DIRTY_SIZE(geomPass, 1);
    _VERIFY_DIRTY_SIZE(geomAndGuidePass, 2);

    // draw only cube.
    driver.Draw();
    // Even though guide cube is dirty.
    // geomAndGuidePass's dirty list will return clean as scene state hasn't
    // changed
    _VERIFY_DIRTY_SIZE(geomPass, 0);
    _VERIFY_DIRTY_SIZE(geomAndGuidePass, 0);

    // draw guide
    driver.Draw(/*withGuides=*/true);
    // everything clean.
    _VERIFY_DIRTY_SIZE(geomPass, 0);
    _VERIFY_DIRTY_SIZE(geomAndGuidePass, 0);
}

static void
DirtyListTest3()
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    Hd_TestDriver driver;
    HdUnitTestDelegate &delegate = driver.GetDelegate();
    GfMatrix4f identity(1);

    HdRenderPassSharedPtr const &geomPass = driver.GetRenderPass();
    HdRenderPassSharedPtr const &geomAndGuidePass = driver.GetRenderPass(true);

    _VERIFY_DIRTY_SIZE(geomPass, 0);
    _VERIFY_DIRTY_SIZE(geomAndGuidePass, 0);

    delegate.AddCube(SdfPath("/cube"), identity, /*guide=*/false);
    delegate.AddCube(SdfPath("/guideCube"), identity, /*guide=*/true);

    _VERIFY_DIRTY_SIZE(geomPass, 1);
    _VERIFY_DIRTY_SIZE(geomAndGuidePass, 2);

    // These changes should be tracked and cause no prims to be updated during
    // the following Draw() calls.
    delegate.HideRprim(SdfPath("/cube"));
    delegate.HideRprim(SdfPath("/guideCube"));
    
    // draw nothing.
    driver.Draw();
    driver.Draw(/*guides*/true);

    // Verify that our dirty lists are now empty.
    _VERIFY_DIRTY_SIZE(geomPass, 0);
    _VERIFY_DIRTY_SIZE(geomAndGuidePass, 0);

    // This should trigger an update in the DirtyList to recompute its included
    // prims.
    delegate.UnhideRprim(SdfPath("/cube"));
    delegate.UnhideRprim(SdfPath("/guideCube"));

    _VERIFY_DIRTY_SIZE(geomPass, 1);
    _VERIFY_DIRTY_SIZE(geomAndGuidePass, 2);

    // draw only cube.
    driver.Draw();
    _VERIFY_DIRTY_SIZE(geomPass, 0);
    // Even though guide cube is dirty.
    // geomAndGuidePass's dirty list will return clean as scene state hasn't
    // changed
    _VERIFY_DIRTY_SIZE(geomAndGuidePass, 0);

    // Swapping the collection (geomPass creates a new dirtyList)
    geomPass->SetRprimCollection(geomAndGuidePass->GetRprimCollection());

    // /cube and /guideCube is added into the dirty list.
    // note that /cube ic clean, but new dirty list contains all due to InitRepr
    _VERIFY_DIRTY_SIZE(geomPass, 2);

    // Sanity check, this pass should be unaffected.
    _VERIFY_DIRTY_SIZE(geomAndGuidePass, 2);

    // Trigger a collection change :  /cube=clean, /guideCube=dirty
    driver.GetDelegate().UnhideRprim(SdfPath("/cube"));

    // 'Unhide' is a collection change. all dirty list will be refreshed
    // to include all items in the collection.
    _VERIFY_DIRTY_SIZE(geomPass, 2);          // /cube, /guideCube
    _VERIFY_DIRTY_SIZE(geomAndGuidePass, 2);  // /cube, /guideCube

}

static void
DirtyListTest4()
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    Hd_TestDriver driver;
    GfMatrix4f identity;
    identity.SetIdentity();

    HdUnitTestDelegate &delegate = driver.GetDelegate();
    HdRenderPassSharedPtr const &geomPass = driver.GetRenderPass();
    HdRenderPassSharedPtr const &geomAndGuidePass = driver.GetRenderPass(true);

    HdRprimCollection col = geomPass->GetRprimCollection();
    SdfPathVector rootPaths;
    rootPaths.push_back(SdfPath("/cube"));
    col.SetRootPaths(rootPaths);
    geomPass->SetRprimCollection(col);

    _VERIFY_DIRTY_SIZE(geomPass, 0);
    _VERIFY_DIRTY_SIZE(geomAndGuidePass, 0);

    delegate.AddCube(SdfPath("/cube"), identity, /*guide=*/false);
    delegate.AddCube(SdfPath("/guideCube"), identity, /*guide=*/true);

    // Expect GetSize() == 2 in both cases here because dirty lists start off
    // with all prims present, its only after drawing that the list is reduced.
    _VERIFY_DIRTY_SIZE(geomPass, 1);
    _VERIFY_DIRTY_SIZE(geomAndGuidePass, 2);

    // These changes should be tracked and cause no prims to be updated during
    // the following Draw() calls.
    delegate.HideRprim(SdfPath("/cube"));
    delegate.HideRprim(SdfPath("/guideCube"));
    
    // draw nothing.
    driver.Draw();
    driver.Draw(/*guides*/true);

    // Verify that our dirty lists are now empty.
    _VERIFY_DIRTY_SIZE(geomPass, 0);
    _VERIFY_DIRTY_SIZE(geomAndGuidePass, 0);

    // This should trigger an update in the DirtyList to recompute its included
    // prims.
    delegate.UnhideRprim(SdfPath("/cube"));
    delegate.UnhideRprim(SdfPath("/guideCube"));

    _VERIFY_DIRTY_SIZE(geomPass, 1);
    _VERIFY_DIRTY_SIZE(geomAndGuidePass, 2);

    // draw only cube.
    driver.Draw();

    rootPaths.clear();
    rootPaths.push_back(SdfPath("/guideCube"));
    col.SetRootPaths(rootPaths);
    geomPass->SetRprimCollection(col);

    driver.Draw();

    _VERIFY_DIRTY_SIZE(geomPass, 0);
    // Even though guide cube is dirty.
    // geomAndGuidePass's dirty list will return clean as scene state hasn't
    // changed
    _VERIFY_DIRTY_SIZE(geomAndGuidePass, 0);

    // switch collection, create a new dirtyList
    geomPass->SetRprimCollection(geomAndGuidePass->GetRprimCollection());
    _VERIFY_DIRTY_SIZE(geomPass, 2);  // cube:clean guideCube:dirty

    // Sanity check, this pass should be unaffected.
    _VERIFY_DIRTY_SIZE(geomAndGuidePass, 2);  // cube:partially-clean, guideCube:dirty

    // Trigger a dirty change
    // XXX: revisit this test
    delegate.UnhideRprim(SdfPath("/cube"));
    _VERIFY_DIRTY_SIZE(geomPass, 2);          // cube:clean guideCube:dirty
    _VERIFY_DIRTY_SIZE(geomAndGuidePass, 2);  // cube:clean guideCube:dirty
}

static void
DirtyListTest5()
{
    // This test specifically tests stable-state behavior.
    
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    Hd_TestDriver driver;
    GfMatrix4f identity(1);
    int dirtyBits = HdChangeTracker::DirtyVisibility;

    HdUnitTestDelegate &delegate = driver.GetDelegate();
    HdRenderIndex &renderIndex = delegate.GetRenderIndex();
    HdChangeTracker &tracker = renderIndex.GetChangeTracker();

    HdRenderPassSharedPtr const &geomPass = driver.GetRenderPass();

    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 0);
    _VERIFY_DIRTY_SIZE(geomPass, 0);
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 1);

    delegate.AddCube(SdfPath("/cube"), identity, /*guide=*/false);
    delegate.AddCube(SdfPath("/cube2"), identity, /*guide=*/false);
    delegate.AddCube(SdfPath("/cube3"), identity, /*guide=*/false);

    _VERIFY_DIRTY_SIZE(geomPass, 3);
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 2);

    // Make sure the prims are marked clean, like sync would, so we can enter
    // the varying state
    tracker.MarkRprimClean(SdfPath("/cube"), HdChangeTracker::Clean);
    tracker.MarkRprimClean(SdfPath("/cube2"), HdChangeTracker::Clean);
    tracker.MarkRprimClean(SdfPath("/cube3"), HdChangeTracker::Clean);


    // ---------------------------------------------------------------------- //
    // Setup a stable-state dirty set of /cube and /cube2
    // ---------------------------------------------------------------------- //
    delegate.MarkRprimDirty(SdfPath("/cube"), dirtyBits);
    delegate.MarkRprimDirty(SdfPath("/cube2"), dirtyBits);
    // dirtylist becomes stable-set containing 2 prims,
    // since we cleared the initialization list.

    _VERIFY_DIRTY_SIZE(geomPass, 2);
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 3);

    // Two mark dirties of the same prim is seen as a redundant sets
    // A clean needs to be issue, so that Hydra considers the next dirties
    // to be a new frame.
    tracker.MarkRprimClean(SdfPath("/cube"), HdChangeTracker::Clean);
    tracker.MarkRprimClean(SdfPath("/cube2"), HdChangeTracker::Clean);

    // Mark dirty again, to trigger a rebuild during Draw().
    delegate.MarkRprimDirty(SdfPath("/cube"), dirtyBits);
    delegate.MarkRprimDirty(SdfPath("/cube2"), dirtyBits);
    driver.Draw();                                                // << REBUILD

    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 3);
    _VERIFY_DIRTY_SIZE(geomPass, 0);
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 3);


    // Marking dirty should no longer trigger a rebuild, expect stable state.
    delegate.MarkRprimDirty(SdfPath("/cube"), dirtyBits);
    delegate.MarkRprimDirty(SdfPath("/cube2"), dirtyBits);
    _VERIFY_DIRTY_SIZE(geomPass, 2);
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 3);
    tracker.MarkRprimClean(SdfPath("/cube"), HdChangeTracker::Clean);
    tracker.MarkRprimClean(SdfPath("/cube2"), HdChangeTracker::Clean);
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 3);
    _VERIFY_DIRTY_SIZE(geomPass, 0);
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 3);

    delegate.MarkRprimDirty(SdfPath("/cube"), dirtyBits);
    delegate.MarkRprimDirty(SdfPath("/cube2"), dirtyBits);
    _VERIFY_DIRTY_SIZE(geomPass, 2);
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 3);
    tracker.MarkRprimClean(SdfPath("/cube"), HdChangeTracker::Clean);
    tracker.MarkRprimClean(SdfPath("/cube2"), HdChangeTracker::Clean);
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 3);
    _VERIFY_DIRTY_SIZE(geomPass, 0);
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 3);

    // ---------------------------------------------------------------------- //
    // Setup a stable-state dirty set of /cube3
    // ---------------------------------------------------------------------- //
    tracker.ResetVaryingState();

    delegate.MarkRprimDirty(SdfPath("/cube3"), dirtyBits);
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 3);
    _VERIFY_DIRTY_SIZE(geomPass, 1);
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 4);
    tracker.MarkRprimClean(SdfPath("/cube3"), HdChangeTracker::Clean);
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 4);
    _VERIFY_DIRTY_SIZE(geomPass, 0);
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 4);

    delegate.MarkRprimDirty(SdfPath("/cube3"), dirtyBits);
    _VERIFY_DIRTY_SIZE(geomPass, 1);
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 4);
    tracker.MarkRprimClean(SdfPath("/cube3"), HdChangeTracker::Clean);
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 4);
    _VERIFY_DIRTY_SIZE(geomPass, 0);
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 4);

    delegate.MarkRprimDirty(SdfPath("/cube3"), dirtyBits);
    _VERIFY_DIRTY_SIZE(geomPass, 1);
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 4);
    tracker.MarkRprimClean(SdfPath("/cube3"), HdChangeTracker::Clean);
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 4);
    _VERIFY_DIRTY_SIZE(geomPass, 0);
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 4);
}

static void
DirtyListTest6()
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    Hd_TestDriver driver;
    GfMatrix4f identity(1);

    HdUnitTestDelegate &delegate = driver.GetDelegate();
    HdRenderIndex &renderIndex = delegate.GetRenderIndex();

    HdRprimCollection col;

    col = HdRprimCollection(HdTokens->geometry, 
        HdReprSelector(HdReprTokens->hull), SdfPath("/a"));
    HdRenderPassSharedPtr const &passA = 
        HdRenderPassSharedPtr(new Hd_UnitTestNullRenderPass(&renderIndex, col));

    col = HdRprimCollection(HdTokens->geometry, 
        HdReprSelector(HdReprTokens->hull), SdfPath("/b"));
    HdRenderPassSharedPtr const &passB = 
        HdRenderPassSharedPtr(new Hd_UnitTestNullRenderPass(&renderIndex, col));

    col = HdRprimCollection(HdTokens->geometry, 
        HdReprSelector(HdReprTokens->hull), SdfPath("/c"));
    HdRenderPassSharedPtr const &passC = 
        HdRenderPassSharedPtr(new Hd_UnitTestNullRenderPass(&renderIndex, col));

    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 0);
    _VERIFY_DIRTY_SIZE(passA, 0);
    _VERIFY_DIRTY_SIZE(passB, 0);
    _VERIFY_DIRTY_SIZE(passC, 0);

    delegate.AddCube(SdfPath("/c/cube5"), identity, /*guide=*/false);
    delegate.AddCube(SdfPath("/b/cube3"), identity, /*guide=*/false);
    delegate.AddCube(SdfPath("/b/cube6"), identity, /*guide=*/false);
    delegate.AddCube(SdfPath("/b/cube7"), identity, /*guide=*/false);
    delegate.AddCube(SdfPath("/a/cube1"), identity, /*guide=*/false);
    delegate.AddCube(SdfPath("/b/cube4"), identity, /*guide=*/false);
    delegate.AddCube(SdfPath("/c/cube4"), identity, /*guide=*/false);
    delegate.AddCube(SdfPath("/c/cube8"), identity, /*guide=*/false);
    delegate.AddCube(SdfPath("/a/cube2"), identity, /*guide=*/false);

    // The dirty lists now include all prims in the scene
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 3);
    _VERIFY_DIRTY_SIZE(passA, 9);
    _VERIFY_DIRTY_SIZE(passB, 9);
    _VERIFY_DIRTY_SIZE(passC, 9);
    driver.Draw();

    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 7);

    // The draw above is the first draw.
    // The task list used for the draw doesn't contain any of the render passes
    // instead the render pass from the unit test delegate.
    // As it is the first time that test delegate is used.  It set a new
    // collection, the render tags are different, so the scene state is
    // incremented.
    //
    // Therefore, when we run pass A's dirty list again.  It sees something has
    // changed and so it enters the varying state.
    _VERIFY_DIRTY_SIZE(passA, 9);
    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 8);
}

static void
DirtyListTest7()
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    Hd_TestDriver driver;
    GfMatrix4f identity(1);

    HdUnitTestDelegate &delegate = driver.GetDelegate();
    HdRenderIndex &renderIndex = delegate.GetRenderIndex();

    HdRprimCollection col;

    col = HdRprimCollection(HdTokens->geometry, 
        HdReprSelector(HdReprTokens->hull));
    HdRenderPassSharedPtr const &passA =
        HdRenderPassSharedPtr(new Hd_UnitTestNullRenderPass(&renderIndex, col));

    col = HdRprimCollection(HdTokens->geometry, 
        HdReprSelector(HdReprTokens->smoothHull));
    HdRenderPassSharedPtr const &passB =
        HdRenderPassSharedPtr(new Hd_UnitTestNullRenderPass(&renderIndex, col));

    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 0);

    SdfPath id("/cube");
    delegate.AddCube(id, identity, /*guide=*/false);

    HdDirtyBits dirtyBits
        = renderIndex.GetChangeTracker().GetRprimDirtyBits(id);

    // Make sure that we initialize the dirty bits correctly.
    TF_VERIFY(dirtyBits == renderIndex.GetRprim(id)->GetInitialDirtyBitsMask());

    // Draw flat shaded hull
    driver.Draw(passA);
    dirtyBits = renderIndex.GetChangeTracker().GetRprimDirtyBits(id);

    TF_VERIFY(!HdChangeTracker::IsExtentDirty(dirtyBits, id));
    TF_VERIFY(!HdChangeTracker::IsTopologyDirty(dirtyBits, id));
    TF_VERIFY(!HdChangeTracker::IsDoubleSidedDirty(dirtyBits, id));
    TF_VERIFY(!HdChangeTracker::IsTransformDirty(dirtyBits, id));
    TF_VERIFY(!HdChangeTracker::IsVisibilityDirty(dirtyBits, id));
    TF_VERIFY(!HdChangeTracker::IsPrimIdDirty(dirtyBits, id));
    TF_VERIFY(!HdChangeTracker::IsPrimvarDirty(dirtyBits, id, HdTokens->points));
    TF_VERIFY(!HdChangeTracker::IsPrimvarDirty(dirtyBits, id, HdTokens->normals));

    // Draw smooth shaded hull (cleans normals)
    driver.Draw(passB);
    dirtyBits = renderIndex.GetChangeTracker().GetRprimDirtyBits(id);

    TF_VERIFY(!HdChangeTracker::IsPrimvarDirty(dirtyBits, id, HdTokens->normals));
}

static void
DirtyListTest8()
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    Hd_TestDriver driver;
    GfMatrix4f identity(1);

    HdUnitTestDelegate &delegate = driver.GetDelegate();
    HdRenderIndex &renderIndex = delegate.GetRenderIndex();
    HdChangeTracker &tracker = renderIndex.GetChangeTracker();

    HdRprimCollection col;

    col = HdRprimCollection(HdTokens->geometry, 
        HdReprSelector(HdReprTokens->hull));
    HdRenderPassSharedPtr const &pass =
        HdRenderPassSharedPtr(new Hd_UnitTestNullRenderPass(&renderIndex, col));

    _VERIFY_PERF_COUNT(HdPerfTokens->dirtyListsRebuilt, 0);

    SdfPathVector ids;
    for (int i = 0; i < 100; ++i) {
        SdfPath id("/cube"+TfIntToString(i));
        delegate.AddCube(id, identity, /*guide=*/false);
        ids.push_back(id);
    }
    _VERIFY_DIRTY_SIZE(pass, 100);

    // clean (initial)
    for (size_t i = 0; i < ids.size(); ++i) {
        tracker.MarkRprimClean(ids[i], HdChangeTracker::Clean);
    }

    _VERIFY_DIRTY_SIZE(pass, 0);

    // mark half dirty
    for (int i = 0; i < 50; ++i) {
        delegate.MarkRprimDirty(ids[i], HdChangeTracker::DirtyTransform);
    }

    // 50 varying prims
    _VERIFY_DIRTY_SIZE(pass, 50);

    // clean all prims
    for (size_t i = 0; i < ids.size(); ++i) {
        tracker.MarkRprimClean(ids[i], HdChangeTracker::Clean);
    }

    // mark 30 dirty again
    for (int i = 0; i < 30; ++i) {
        delegate.MarkRprimDirty(ids[i], HdChangeTracker::DirtyTransform);
    }

    // 50 varying prims
    _VERIFY_DIRTY_SIZE(pass, 50);

    // clean all prims
    for (size_t i = 0; i < ids.size(); ++i) {
        tracker.MarkRprimClean(ids[i], HdChangeTracker::Clean);
    }

    _VERIFY_DIRTY_SIZE(pass, 0);

    // mark 2 dirty
    for (int i = 0; i < 2; ++i) {
        delegate.MarkRprimDirty(ids[i], HdChangeTracker::DirtyTransform);
    }

    // still 50 prims
    _VERIFY_DIRTY_SIZE(pass, 50);

    // Mark one more dirty.  As varying the dirty list, reset the
    // dirty list, so Sync won't operate.
    delegate.MarkRprimDirty(ids[2], HdChangeTracker::DirtyTransform);

    driver.Draw(pass);  // should reset varying state, since only < 10% prims are varying

    _VERIFY_DIRTY_SIZE(pass, 0);

    // mark 2 dirty
    for (int i = 0; i < 2; ++i) {
        delegate.MarkRprimDirty(ids[i], HdChangeTracker::DirtyTransform);
    }

    // shrink dirty list
    _VERIFY_DIRTY_SIZE(pass, 2);

    driver.Draw(pass);

    _VERIFY_DIRTY_SIZE(pass, 0);

}

int main()
{
    TfErrorMark mark;

    DirtyListTest();
    DirtyListTest2();
    DirtyListTest3();
    DirtyListTest4();
    DirtyListTest5();
    DirtyListTest6();
    DirtyListTest7();
    DirtyListTest8();

    TF_VERIFY(mark.IsClean());

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

