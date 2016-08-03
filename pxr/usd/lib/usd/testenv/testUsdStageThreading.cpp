#include <Python.h>

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
namespace boost_po = boost::program_options;

static size_t numThreads;
static size_t msecsToRun;
static bool runForever = false;

TF_MAKE_STATIC_DATA(vector<string>, _testPaths)
{
#define ASSET(x)                                                        \
    "/usr/anim/menv30/testing/usd/City_set_crate/modarchitecture/"      \
        x "/usd/" x ".usd"
    _testPaths->push_back(ASSET("Bob"));
    _testPaths->push_back(ASSET("ChrisB"));
    _testPaths->push_back(ASSET("CityGround"));
    _testPaths->push_back(ASSET("Frank"));
    _testPaths->push_back(ASSET("Hank"));
    _testPaths->push_back(ASSET("Ivo"));
    _testPaths->push_back(ASSET("Jason"));
    _testPaths->push_back(ASSET("Jay"));
    _testPaths->push_back(ASSET("Kipply"));
    _testPaths->push_back(ASSET("Kyle"));
    _testPaths->push_back(ASSET("Liz"));
    _testPaths->push_back(ASSET("Lofty"));
    _testPaths->push_back(ASSET("Manny"));
    _testPaths->push_back(ASSET("Mark"));
    _testPaths->push_back(ASSET("Momo"));
    _testPaths->push_back(ASSET("Omar"));
    _testPaths->push_back(ASSET("Pauls"));
    _testPaths->push_back(ASSET("Ralf"));
    _testPaths->push_back(ASSET("Sal"));
    _testPaths->push_back(ASSET("Saschka"));
    _testPaths->push_back(ASSET("Subway"));
#undef ASSET
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
            TF_FOR_ALL(vsetName, vsetNames) {
                _Add(result, TfStringPrintf("\tVariantSet: %s\n",
                                            vsetName->c_str()));
                UsdVariantSet vset = prim.GetVariantSets()[*vsetName];
                vector<string> varNames = vset.GetVariantNames();
                TF_FOR_ALL(varName, varNames) {
                    _Add(result, TfStringPrintf("\t\tvariant: %s\n",
                                                varName->c_str()));
                }
                _Add(result, TfStringPrintf(
                         "\\tselection: %s\n",
                         vset.GetVariantSelection().c_str()));
            }
        }

        // Properties
        const TfTokenVector propNames = prim.GetPropertyNames();
        TF_FOR_ALL(propName, propNames) {
            _Add(result, TfStringPrintf("\tproperty: %s\n",
                                        propName->GetText()));
            // XXX TODO: We should include property values, metadata, etc.
        }

        // Children
        TF_FOR_ALL(child, prim.GetChildren()) {
            _DumpResults(stage, child->GetPath(), result);
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

    while (runForever or sw.GetMilliseconds() < msecsToRun) {
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
    TF_FOR_ALL(assetPath, *_testPaths) {
        _AddTestCase(*assetPath);
    }

    // Verify that all layers we loaded at startup have been dropped.
    // (Leaked layers could mask bugs.)
    TF_VERIFY(SdfLayer::GetLoadedLayers().size() == baselineNumLayers,
              "Expected no additional layers in memory, got %zu",
              SdfLayer::GetLoadedLayers().size() - baselineNumLayers);

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

    TF_AXIOM(not Py_IsInitialized());

    return 0;
}

