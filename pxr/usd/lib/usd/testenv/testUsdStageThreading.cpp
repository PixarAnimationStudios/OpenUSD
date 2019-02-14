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

#ifdef PXR_PYTHON_SUPPORT_ENABLED
#ifdef _DEBUG
  #undef _DEBUG
  #include <Python.h>
  #define _DEBUG
#else
  #include <Python.h>
#endif
#endif // PXR_PYTHON_SUPPORT_ENABLED

#include "pxr/pxr.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/variantSets.h"

#include "pxr/base/tf/stopwatch.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/work/arenaDispatcher.h"

#include <boost/random.hpp>
#include <boost/program_options.hpp>

#include <thread>

using std::string;
using std::vector;
PXR_NAMESPACE_USING_DIRECTIVE
namespace boost_po = boost::program_options;

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
    boost::mt19937 mt(std::hash<std::thread::id>()(threadId));
    boost::uniform_int<unsigned int> dist(0, _testCases.size()-1);
    boost::variate_generator< boost::mt19937, boost::uniform_int<unsigned int> >
        gen(mt, dist);

    while (runForever || static_cast<size_t>(sw.GetMilliseconds()) < msecsToRun) {
        sw.Start();
        const int i = gen();

        printf("  Thread %s running test case %d\n",
               TfStringify(threadId).c_str(), i);
                                    
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
    boost_po::options_description desc("Options");
    desc.add_options()
        ("forever",
         boost_po::bool_switch(&runForever),
         "Run forever")

        ("numThreads",
         boost_po::value<size_t>(&numThreads)->default_value(
             std::thread::hardware_concurrency()),
         "Number of threads to use")

        ("msec",
         boost_po::value<size_t>(&msecsToRun)->default_value(10*1000),
         "Milliseconds to run")
    ;

    boost_po::variables_map vm;                                                 
    try {                                                                       
        boost_po::store(boost_po::parse_command_line(argc, argv, desc), vm);    
        boost_po::notify(vm);                                                   
    } catch (const boost_po::error &e) {                                        
        fprintf(stderr, "%s\n", e.what());                                      
        fprintf(stderr, "%s\n", TfStringify(desc).c_str());                     
        exit(1);                                                                
    }


    // Initialize. 
    printf("Using %zu threads\n", numThreads);

    // Pull on the schema registry to create any schema layers so we can get a
    // baseline of # of loaded layers.
    UsdSchemaRegistry::GetSchematics();
    size_t baselineNumLayers = SdfLayer::GetLoadedLayers().size();

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

    WorkArenaDispatcher wd;
    auto localMsecsToRun = msecsToRun;
    auto localRunForever = runForever;
    for (size_t i = 0; i < numThreads; ++i) {
        wd.Run([localMsecsToRun, localRunForever]() {
                _WorkTask(localMsecsToRun, localRunForever);
            });
    }
    wd.Wait();

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

