//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/retainedSceneIndex.h"
#include "pxr/imaging/hdsi/computeSceneIndexDiff.h"

#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/token.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

//-----------------------------------------------------------------------------

static bool
_TestComputeSceneIndexDiffDelta()
{
    HdRetainedSceneIndexRefPtr siA = HdRetainedSceneIndex::New();
    siA->AddPrims({
        { SdfPath("/Prim"), TfToken("A"), nullptr },
        { SdfPath("/Unchanged"), TfToken("A"), nullptr },
        { SdfPath("/Removed"), TfToken("A"), nullptr },
    });

    HdRetainedSceneIndexRefPtr siB = HdRetainedSceneIndex::New();
    siB->AddPrims({
        { SdfPath("/Prim"), TfToken("B"), nullptr },
        { SdfPath("/Unchanged"), TfToken("A"), nullptr },
    });

    HdSceneIndexObserver::RemovedPrimEntries removedEntries;
    HdSceneIndexObserver::AddedPrimEntries addedEntries;
    HdSceneIndexObserver::RenamedPrimEntries renamedEntries;
    HdSceneIndexObserver::DirtiedPrimEntries dirtiedEntries;
    HdsiComputeSceneIndexDiffDelta(
        siA, siB, &removedEntries, &addedEntries, &renamedEntries,
        &dirtiedEntries);

    TF_AXIOM(
        addedEntries.size() == 1
        && addedEntries[0].primPath == SdfPath("/Prim"));

    TF_AXIOM(
        removedEntries.size() == 1
        && removedEntries[0].primPath == SdfPath("/Removed"));

    return true;
}

#define xstr(s) str(s)
#define str(s) #s
#define TEST(X)                                                                \
    std::cout << (++i) << ")" << str(X) << "..." << std::endl;                 \
    if (!X()) {                                                                \
        std::cout << "FAILED" << std::endl;                                    \
        return -1;                                                             \
    }                                                                          \
    else {                                                                     \
        std::cout << "... SUCCEEDED" << std::endl;                             \
    }

int
main(int argc, char** argv)
{
    std::cout << "STARTING testHdsiSwitchingSceneIndex" << std::endl;
    int i = 0;
    TEST(_TestComputeSceneIndexDiffDelta);
    return 0;
}
