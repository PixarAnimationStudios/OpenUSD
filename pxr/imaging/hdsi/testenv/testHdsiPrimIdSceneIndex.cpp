//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include <iostream>
#include <utility>

#include "pxr/imaging/hdsi/primIdSceneIndex.h"

#include "pxr/imaging/hd/primIdSchema.h"
#include "pxr/imaging/hd/sceneGlobalsSchema.h"

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/retainedSceneIndex.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/unitTestNullRenderDelegate.h"

#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Records whether the prim it to path table has been dirtied.
//
class _DirtyRecordingObserver : public HdSceneIndexObserver
{
public:
    void PrimsAdded(const HdSceneIndexBase &,
                    const AddedPrimEntries &) override {}
    void PrimsRemoved(const HdSceneIndexBase &,
                      const RemovedPrimEntries &) override {}
    void PrimsRenamed(const HdSceneIndexBase &,
                      const RenamedPrimEntries &) override {}
    void PrimsDirtied(const HdSceneIndexBase &,
                      const DirtiedPrimEntries &entries) override
    {
        for (const DirtiedPrimEntry &entry : entries) {
            if (entry.primPath == HdSceneGlobalsSchema::GetDefaultPrimPath() &&
                entry.dirtyLocators.Intersects(
                    HdSceneGlobalsSchema::GetPrimIdToPathLocator())) {
                primIdDirtied = true;
            }
        }
    }

    bool GetAndClearPrimIdDirtied() { return std::exchange(primIdDirtied, false); }

private:
    bool primIdDirtied = false;
};

// Checks invariants for the prim id scene index - assuming that the only
// imagable prim used in the test is mesh.
//
// Invariant is that every prim has a unique prim id if and only if it is
// imagable and that the correct inverse map is stored in the scene globals
// schema.
//
bool
_CheckInvariants(HdSceneIndexBaseRefPtr const &si)
{
    const HdPathVectorSchema primIds =
        HdSceneGlobalsSchema::GetFromSceneIndex(si).GetPrimIdToPath();

    size_t numPrimsWithId = 0;

    // Traverse all prims.
    for (const SdfPath &primPath : HdSceneIndexPrimView(si)) {
        const HdSceneIndexPrim prim = si->GetPrim(primPath);

        if (HdPrimIdDataSourceHandle const primIdDs =
                HdPrimIdSchema::GetFromParent(prim.dataSource).GetPrimId()) {
            // prim has a prim Id.
            ++numPrimsWithId;

            // Check that the prim id maps back to the prim.
            const HdPathDataSourceHandle ds = primIds.GetElement(
                primIdDs->GetTypedValue(0.0f));
            if (!ds || ds->GetTypedValue(0.0f) != primPath) {
                return false;
            }

            // Check that the prim is actually imageable.
            if (prim.primType != HdPrimTypeTokens->mesh) {
                return false;
            }
        } else {
            // prim has no prim Id. Check that it is not imageable.
            if (prim.primType == HdPrimTypeTokens->mesh) {
                return false;
            }
        }
    }

    // Check that the number of assigned prim ids is equal to
    // the number of prims with assigned ids.

    size_t numPrimIds = 0;
    for (size_t i = 0; i < primIds.GetNumElements(); ++i) {
        if (primIds.GetElement(i)) {
            ++numPrimIds;
        }
    }

    return numPrimsWithId == numPrimIds;
}

#define TEST(X) \
    std::cout << "  " << #X << std::endl; \
    if (!(X)) { \
        std::cout << "  FAILED: " << #X << std::endl; \
        return -1; \
    }

} // anonymous namespace

int
main(const int argc, const char * const * const argv)
{
    std::cout << "STARTING testHdsiPrimIdSceneIndex" << std::endl;

    // Build an populate an input scene.
    
    HdRetainedSceneIndexRefPtr const retained =
        HdRetainedSceneIndex::New();

    HdContainerDataSourceHandle const emptyContainer =
        HdRetainedContainerDataSource::New();

    retained->AddPrims({
        { SdfPath("/A"),       HdPrimTypeTokens->mesh,     emptyContainer },
        { SdfPath("/A/child"), HdPrimTypeTokens->mesh,     emptyContainer },
        { SdfPath("/B"),       HdPrimTypeTokens->material, emptyContainer },
        { SdfPath("/C"),       HdPrimTypeTokens->mesh,     emptyContainer },
        { SdfPath("/D"),       HdPrimTypeTokens->material, emptyContainer },
        { SdfPath("/E"),       HdPrimTypeTokens->mesh,     emptyContainer }
    });

    HdsiPrimIdSceneIndexRefPtr primIdSi = HdsiPrimIdSceneIndex::New(retained);

    _DirtyRecordingObserver observer;
    primIdSi->AddObserver(HdSceneIndexObserverPtr(&observer));

    // 1) Check correct population.
    
    TEST(_CheckInvariants(primIdSi));

    // 2) Adding prims/resyncing prim type.
    retained->AddPrims({
       // Imageable -> not imageable
        { SdfPath("/A"), HdPrimTypeTokens->material, emptyContainer },
        { SdfPath("/E"), HdPrimTypeTokens->material, emptyContainer },
        // Nonimageable -> imagable
        { SdfPath("/D"), HdPrimTypeTokens->mesh, emptyContainer },
        // New
        { SdfPath("/F"), HdPrimTypeTokens->mesh, emptyContainer },
        // New with child.
        { SdfPath("/G"), HdPrimTypeTokens->mesh, emptyContainer },
        { SdfPath("/G/child"), HdPrimTypeTokens->mesh, emptyContainer },
    });
    TEST(observer.GetAndClearPrimIdDirtied());
    TEST(_CheckInvariants(primIdSi));

    // 3) Removing imagable prim.
    retained->RemovePrims({ { SdfPath("/C") } });
    TEST(observer.GetAndClearPrimIdDirtied());
    TEST(_CheckInvariants(primIdSi));

    // 4) Removing imageable prim with child.
    retained->RemovePrims({ { SdfPath("/G") } });
    TEST(observer.GetAndClearPrimIdDirtied());
    TEST(_CheckInvariants(primIdSi));

    retained->RemovePrims({ { SdfPath("/") } });
    TEST(observer.GetAndClearPrimIdDirtied());
    TEST(_CheckInvariants(primIdSi));

    primIdSi->RemoveObserver(HdSceneIndexObserverPtr(&observer));

    std::cout << "DONE testHdsiPrimIdSceneIndex" << std::endl;
    return 0;
}
