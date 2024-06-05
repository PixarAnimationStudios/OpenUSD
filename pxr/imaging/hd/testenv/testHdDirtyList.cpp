//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/imaging/hd/unitTestHelper.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/dirtyList.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/unitTestNullRenderPass.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static void
_VerifyDirtyListSize(HdDirtyList *dl, size_t count)
{
    if (!TF_VERIFY(dl)) {
        return;
    }
    SdfPathVector const &dirtyRprimIds = dl->GetDirtyRprims();
    TF_VERIFY(dirtyRprimIds.size() == count, "expected %zu, found %zu",
              count, dirtyRprimIds.size());
}

static void
_VerifyCounter(HdPerfLog *perfLog, TfToken const &name, size_t count)
{
    size_t value = size_t(perfLog->GetCounter(name));
    TF_VERIFY(value == count, "expected %zu, found %zu",
              count, value);
}

static HdReprSelector surface(HdReprTokens->refined);
static HdReprSelector wireOnSurf(HdReprTokens->wireOnSurf);
static HdReprSelector wireOnSurfWithPoints(
    HdReprTokens->wireOnSurf, HdReprTokens->disabled, HdReprTokens->points);

static
bool
BasicTest()
{
    Hd_TestDriver driver;
    HdUnitTestDelegate &delegate = driver.GetDelegate();
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();

    // Add 3 prims (2 geometry, 1 guide)
    size_t numGeometryPrims = 0;
    size_t numGuidePrims = 0;
    {
        GfMatrix4f identity;
        identity.SetIdentity();
        delegate.AddCube(SdfPath("/cube1"), identity);
        numGeometryPrims++;
        delegate.AddCube(SdfPath("/cube2"), identity);
        numGeometryPrims++;
        delegate.AddCube(SdfPath("/cube3"), identity, /*guide =*/true);
        numGuidePrims++;
    }

    HdDirtyList dl(delegate.GetRenderIndex());
    // The dirty list wouldn't have any tracked render tags or reprs

    // 1. Empty render tags is currently treated as an all-pass filter.
    //    So, all the rprims should be in the dirty list.
    {
        std::cout << "1. Empty render tags\n";
        perfLog.ResetCounters();

        dl.UpdateRenderTagsAndReprSelectors(
            TfTokenVector(/*empty render tags*/),
            HdReprSelectorVector({surface}));

        _VerifyDirtyListSize(&dl, numGeometryPrims + numGuidePrims);
        _VerifyCounter(&perfLog, HdPerfTokens->dirtyListsRebuilt, 1);
    }

    // 2. Toggle the repr. This should grow the tracked repr set and rebuild
    //    the dirty list to initialize the repr for all the rprims. On switching
    //    back, the dirty list will be rebuilt to just the varying rprims.
    {
        std::cout << "2. Toggle repr\n";
        perfLog.ResetCounters();

        dl.UpdateRenderTagsAndReprSelectors(
            TfTokenVector(/*empty render tags*/),
            HdReprSelectorVector({wireOnSurf}));
        _VerifyDirtyListSize(&dl, numGeometryPrims + numGuidePrims);

        dl.UpdateRenderTagsAndReprSelectors(
            TfTokenVector(/*empty render tags*/),
            HdReprSelectorVector({surface}));

        _VerifyDirtyListSize(&dl, 0);
        _VerifyCounter(&perfLog, HdPerfTokens->dirtyListsRebuilt, 2);
    }

    // 3. Update the render tags.
    {
        std::cout << "3. Update render tags\n";
        perfLog.ResetCounters();

        // empty -> geometry : This will apply just the 'geometry' tag filter
        // when rebuilding the dirty list.
        dl.UpdateRenderTagsAndReprSelectors(
            TfTokenVector({HdRenderTagTokens->geometry}),
            HdReprSelectorVector({surface}));
        _VerifyDirtyListSize(&dl, numGeometryPrims);

        // geometry -> guide : The tracked render tag set is grown since the
        // no rprims have been added/removed, and the repr opinion of rprims
        // hasn't changed. The dirty list will be rebuilt to include both
        // geometry and guide prims.
        dl.UpdateRenderTagsAndReprSelectors(
            TfTokenVector({HdRenderTagTokens->guide}),
            HdReprSelectorVector({surface}));

        _VerifyDirtyListSize(&dl, numGeometryPrims + numGuidePrims);
        _VerifyCounter(&perfLog, HdPerfTokens->dirtyListsRebuilt, 2);

        // guide -> geometry : Dirty list will be rebuilt to just the varying
        // ones (which is none).
        dl.UpdateRenderTagsAndReprSelectors(
            TfTokenVector({HdRenderTagTokens->geometry}),
            HdReprSelectorVector({surface}));
        _VerifyDirtyListSize(&dl, 0);
        _VerifyCounter(&perfLog, HdPerfTokens->dirtyListsRebuilt, 3);
    }

    // 4. Add an rprim. This should reset the active repr set and rebuild the
    //    dirty list
    {
        std::cout << "4. Add an rprim\n";
        perfLog.ResetCounters();

        delegate.AddCube(SdfPath("/cube4"), GfMatrix4f());
        numGeometryPrims++;

        _VerifyDirtyListSize(&dl, numGeometryPrims + numGuidePrims);
        _VerifyCounter(&perfLog, HdPerfTokens->dirtyListsRebuilt, 1);
    }

    // 5. Varying tests. Update a few rprims. This will reduce the dirty list
    //    from all rprims to just the varying ones.
    {
        std::cout << "5. Varying test\n";
        perfLog.ResetCounters();
        
        HdChangeTracker &tracker = delegate.GetRenderIndex().GetChangeTracker();
        // Since we don't invoke HdRenderIndex::SyncAll, simulate the render
        // delegate sync'ing the rprim and clearing its dirty bits.
        tracker.MarkRprimClean(SdfPath("/cube1"));
        tracker.MarkRprimClean(SdfPath("/cube3"));

        // Make edits.
        tracker.MarkRprimDirty(SdfPath("/cube1"), HdChangeTracker::DirtyPrimvar);
        tracker.MarkRprimDirty(SdfPath("/cube3"), HdChangeTracker::DirtyPoints);

        _VerifyDirtyListSize(&dl, 2);
        _VerifyCounter(&perfLog, HdPerfTokens->dirtyListsRebuilt, 1);

        // Querying the dirty ids again when nothing has changed should return
        // an empty list.
        _VerifyDirtyListSize(&dl, 0);
    }

    return true;
}

int main()
{
    TfErrorMark mark;
    bool success = BasicTest();

    TF_VERIFY(mark.IsClean());

    if (success && mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
