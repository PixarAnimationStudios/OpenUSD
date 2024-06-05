//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/attribute.h"

#include "pxr/base/work/dispatcher.h"

#include "pxr/base/tf/staticData.h"

#include <tbb/concurrent_vector.h>

#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include "pxr/base/tf/pySafePython.h"
#endif // PXR_PYTHON_SUPPORT_ENABLED

#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE
using std::string;
using std::vector;

static const size_t numSiblings = 4;
static const size_t depth = 4;
static const size_t numAttrs = 2;

static void
Check(const UsdPrim &parent, char const *prefix, size_t depth)
{
    UsdStagePtr stage = parent.GetStage();
    for (size_t i = 0; i != numSiblings; ++i) {
        UsdPrim p = stage->GetPrimAtPath(
            parent.GetPath().AppendChild(
                TfToken(TfStringPrintf("%s_%zu", prefix, i))));
        TF_AXIOM(p);
        for (size_t j = 0; j != numAttrs; ++j) {
            UsdAttribute a = p.GetAttribute(
                TfToken(TfStringPrintf("%s_%zu", prefix, j)));
            TF_AXIOM(a);
            int value;
            TF_AXIOM(a.Get(&value) && value == (int)j);
        }
        if (depth)
            Check(p, prefix, depth-1);
    }
}

static void
Author(const UsdPrim &parent, char const *prefix, size_t depth)
{
    UsdStagePtr stage = parent.GetStage();
    for (size_t i = 0; i != numSiblings; ++i) {
        UsdPrim p = stage->DefinePrim(
            parent.GetPath().AppendChild(
                TfToken(TfStringPrintf("%s_%zu", prefix, i))));
        for (size_t j = 0; j != numAttrs; ++j) {
            UsdAttribute a = p.CreateAttribute(
                TfToken(TfStringPrintf("%s_%zu", prefix, j)),
                SdfValueTypeNames->Int);
            a.Set((int)j);
        }
        if (depth)
            Author(p, prefix, depth-1);
    }
}

static TfStaticData<tbb::concurrent_vector<UsdStageRefPtr> > allStages;

static void
AddStage(const UsdStageRefPtr &stage)
{
    allStages->push_back(stage);
}

static vector<UsdStageRefPtr>
GetAllStages()
{
    return vector<UsdStageRefPtr>(allStages->begin(), allStages->end());
}

static void
CreateStage()
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();

    // Create prims and properties.
    UsdPrim pseudoRoot = stage->GetPseudoRoot();
    Author(pseudoRoot, "prim", depth);

    AddStage(stage);
    printf("Stage done.\n");
}

static void
CheckAllStages()
{
    for (const auto& stage : GetAllStages()) {
        // Check that the structure is what we expect.
        UsdPrim pseudoRoot = stage->GetPseudoRoot();
        Check(pseudoRoot, "prim", depth);
    }
}

static void
TestParallelAuthoring()
{
    static const size_t numJobs = 24;

    // Launch jobs.
    printf("Launching %zu jobs.\n", numJobs);
    WorkDispatcher dispatcher;
    for (size_t i = 0; i != numJobs; ++i)
        dispatcher.Run(&CreateStage);

    // Wait for jobs.
    printf("Waiting for jobs to complete.\n");
    dispatcher.Wait();

    // Check results.
    printf("Checking results.\n");
    CheckAllStages();

    printf("Done.\n");
}

int
main(int, char const *[])
{
    TestParallelAuthoring();

#ifdef PXR_PYTHON_SUPPORT_ENABLED
    TF_AXIOM(!Py_IsInitialized());
#endif // PXR_PYTHON_SUPPORT_ENABLED
    
    return 0;
}

