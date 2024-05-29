//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include "pxr/base/tf/pySafePython.h"
#endif // PXR_PYTHON_SUPPORT_ENABLED

#include "pxr/pxr.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/variantSets.h"

#include "pxr/base/tf/stopwatch.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/pxrCLI11/CLI11.h"
#include "pxr/base/work/dispatcher.h"
#include "pxr/base/work/withScopedParallelism.h"

#include <random>
#include <thread>

using std::string;
using std::vector;
PXR_NAMESPACE_USING_DIRECTIVE
using namespace pxr_CLI;

static size_t numThreads;
static size_t msecsToRun;
static bool runForever = false;

TF_MAKE_STATIC_DATA(vector<string>, _testPaths)
{
    _testPaths->push_back("a/test.usda");
    _testPaths->push_back("b/test.usda");
    _testPaths->push_back("c/test.usda");
    _testPaths->push_back("d/test.usda");
    _testPaths->push_back("e/test.usda");
}

typedef struct {
    bool didLoad;
    std::string digest;
} _Result;

static std::atomic_int _nIters(0);
static std::vector< std::pair<std::string, _Result> > _testCases;

inline static void
_Add(std::string *result, const std::string &s)
{
    (*result) += s;
    // XXX You can add a diagnostic print statement here.
}

static void
_DumpResults(const UsdStageRefPtr &stage,
             const SdfPath & path, std::string *result)
{
    if (UsdPrim prim = stage->Load(path)) {
        _Add(result, TfStringPrintf("%s: %s\n", path.GetString().c_str(),
                                    prim.GetTypeName().GetText()));
                                    
        // XXX TODO: We should include prim metadata.

        // Variants
        if (path != SdfPath::AbsoluteRootPath()) {
            vector<string> vsetNames;
            prim.GetVariantSets().GetNames(&vsetNames);
            for (const auto& vsetName : vsetNames) {
                _Add(result, TfStringPrintf("\tVariantSet: %s\n",
                                            vsetName.c_str()));
                UsdVariantSet vset = prim.GetVariantSets()[vsetName];
                vector<string> varNames = vset.GetVariantNames();
                for (const auto& varName : varNames) {
                    _Add(result, TfStringPrintf("\t\tvariant: %s\n",
                                                varName.c_str()));
                }
                _Add(result, TfStringPrintf(
                         "\\tselection: %s\n",
                         vset.GetVariantSelection().c_str()));
            }
        }

        // Properties
        const TfTokenVector propNames = prim.GetPropertyNames();
        for (const auto& propName : propNames) {
            _Add(result, TfStringPrintf("\tproperty: %s\n",
                                        propName.GetText()));
            // XXX TODO: We should include property values, metadata, etc.
        }

        // Children
        for (const auto& child : prim.GetChildren()) {
            _DumpResults(stage, child.GetPath(), result);
        }
    }
}

static _Result
_ComputeResult(const std::string & inputAssetPath)
{
    _Result result;

    UsdStageRefPtr stage = UsdStage::Open(inputAssetPath);
    result.didLoad = bool(stage);
    if (stage) {
        _DumpResults(stage, SdfPath::AbsoluteRootPath(), &result.digest);
    }
    return result;
}

static void
_AddTestCase(const std::string &assetPath)
{
    _Result result = _ComputeResult(assetPath);
    _testCases.push_back( std::make_pair(assetPath, result) );
    printf("Added test case:\n  path  : %s\n  digest: (%zu bytes)\n",
        assetPath.c_str(), result.digest.size());
}

static void
_WorkTask(size_t msecsToRun, bool runForever)
{
    TfStopwatch sw;
    int count = 0;

    // Use a local random number generator to minimize synchronization
    // between threads, as would happen with using libc's random().
    const std::thread::id threadId = std::this_thread::get_id();
    std::mt19937 mt(std::hash<std::thread::id>()(threadId));
    std::uniform_int_distribution<unsigned int> dist(0, _testCases.size()-1);

    while (runForever || static_cast<size_t>(sw.GetMilliseconds()) < msecsToRun) {
        sw.Start();
        const int i = dist(mt);

        const _Result & expected = _testCases[i].second;
        const _Result & actual   = _ComputeResult(_testCases[i].first);
        TF_VERIFY(actual.didLoad == expected.didLoad);
        TF_VERIFY(actual.digest  == expected.digest);

        ++count;
        sw.Stop();
    }
    
    _nIters += count;
    printf("  Thread %s done; %d stages composed.\n",
           TfStringify(threadId).c_str(), count);
}

int main(int argc, char const **argv)
{
    // Set up arguments and their defaults
    CLI::App app("Tests USD threading", "testUsdStageThreading");
    app.add_flag("--forever", runForever, "Run Forever");
    app.add_option("--numThreads", numThreads, "Number of threads to use")
        ->default_val(std::thread::hardware_concurrency());
    app.add_option("--msec", msecsToRun, "Milliseconds to run")
        ->default_val(10*1000);

    CLI11_PARSE(app, argc, argv);

    // Initialize. 
    printf("Using %zu threads\n", numThreads);

    // Pull on the schema registry to create any schema layers so we can get a
    // baseline of # of loaded layers.
    printf("pulling schema registry\n");
    UsdSchemaRegistry::GetInstance();
    size_t baselineNumLayers = SdfLayer::GetLoadedLayers().size();
    printf("done\n");
    
    printf("==================================================\n");
    printf("SETUP PHASE (MAIN THREAD ONLY)\n");
    for (const auto& assetPath : *_testPaths) {
        _AddTestCase(assetPath);
    }

    // Verify that all layers we loaded at startup have been dropped.
    // (Leaked layers could mask bugs.)
    TF_VERIFY(SdfLayer::GetLoadedLayers().size() == baselineNumLayers,
              "Expected no additional layers in memory, got %zu",
              SdfLayer::GetLoadedLayers().size() - baselineNumLayers);

    // Verify that at least one test case loaded.  If not, that's probably a
    // bug in the test setup.
    bool loadedAny = false;
    for (const auto &p: _testCases) {
        if (p.second.didLoad) {
            loadedAny = true;
            break;
        }
    }
    TF_VERIFY(loadedAny, "Expected at least one asset to load successfully.");

    // Run.
    printf("==================================================\n");
    printf("BEGIN THREADED TESTING\n");
    TfStopwatch sw;
    sw.Start();

    WorkWithScopedDispatcher([&](WorkDispatcher &wd) {
        auto localMsecsToRun = msecsToRun;
        auto localRunForever = runForever;
        for (size_t i = 0; i < numThreads; ++i) {
            wd.Run([localMsecsToRun, localRunForever]() {
                _WorkTask(localMsecsToRun, localRunForever);
            });
        }
    });

    sw.Stop();

    // Verify that we did not leak any layers along the way.
    TF_VERIFY(SdfLayer::GetLoadedLayers().size() == baselineNumLayers,
              "Expected no additional layers in memory, got %zu",
              SdfLayer::GetLoadedLayers().size() - baselineNumLayers);

    // Report.
    printf("Ran %d operations total, paritioned over %zu thread%s in %.3f sec "
           "(%.3f ops/sec)\n", (int)_nIters, numThreads,
           numThreads > 1 ? "s" : "", sw.GetSeconds(),
           double((int)_nIters) / sw.GetSeconds());

#ifdef PXR_PYTHON_SUPPORT_ENABLED
    TF_AXIOM(!Py_IsInitialized());
#endif // PXR_PYTHON_SUPPORT_ENABLED

    return 0;
}

