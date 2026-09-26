//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/stopwatch.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/pxrCLI11/CLI11.h"
#include "pxr/base/tf/stringUtils.h"

#include <atomic>
#include <ctime>
#include <cstdlib>
#include <mutex>
#include <random>
#include <thread>


using std::string;
using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE
using namespace pxr_CLI;

static unsigned int randomSeed;
static size_t numThreads;
static size_t msecsToRun = 2000;

// rand() is not thread-safe.  FreeBSD's rand() does not lock, and concurrent
// calls corrupt its state so that it returns long runs of even numbers, which
// make _MakeRandomPath() recurse until the stack overflows.  Use a generator
// per thread instead.
static int
_Rand()
{
    static std::atomic<unsigned int> nextSeed(0);
    thread_local std::mt19937 generator(randomSeed + nextSeed++);
    return static_cast<int>(generator() >> 1);
}

TF_MAKE_STATIC_DATA(vector<TfToken>, nameTokens)
{
    nameTokens->push_back(TfToken("A"));
    nameTokens->push_back(TfToken("B"));
    nameTokens->push_back(TfToken("C"));

    // Create a large number of candidates to try to exercise paths
    // over the SD_PATH_BINARY_SEARCH_THRESHOLD.
    for (int i=0; i < 64; ++i) {
        std::string s = TfStringPrintf("x_%i", i);
        nameTokens->push_back(TfToken(s));
    }
}

static TfToken
_GetRandomNameToken()
{
    return (*nameTokens)[_Rand() % nameTokens->size()];
}

static SdfPath
_MakeRandomPrimPath()
{
    static const size_t maxDepth = 2;
    SdfPath ret = SdfPath::AbsoluteRootPath();
    for (size_t i = 0, depth = _Rand() % maxDepth; i <= depth; ++i)
        ret = ret.AppendChild(_GetRandomNameToken());
    return ret;
}

static SdfPath
_MakeRandomPrimOrPropertyPath()
{
    SdfPath ret = _MakeRandomPrimPath();
    return _Rand() & 1 ? ret : ret.AppendProperty(_GetRandomNameToken());
}

static SdfPath
_MakeRandomPath(SdfPath const &path = SdfPath::AbsoluteRootPath())
{
    SdfPath ret = path;

    // Absolute root -> prim path.
    if (path == SdfPath::AbsoluteRootPath())
        ret = _MakeRandomPrimPath();

    // Extend a PrimPath.
    if (ret.IsPrimPath() && (_Rand() & 1)) {
        ret = ret.AppendVariantSelection(_GetRandomNameToken().GetString(),
                                         _GetRandomNameToken().GetString());
    }

    // Extend a PrimPath or a PrimVariantSelectionPath.
    if ((ret.IsPrimPath() || ret.IsPrimVariantSelectionPath())) {
        return (_Rand() & 1) ? ret :
            _MakeRandomPath(ret.AppendProperty(_GetRandomNameToken()));
    }

    // Extend a PrimPropertyPath
    if (ret.IsPrimPropertyPath()) {
        // options: target path, mapper path, expression path, or leave alone.
        switch (_Rand() & 3) {
        case 0:
            return _MakeRandomPath(
                ret.AppendTarget(_MakeRandomPrimOrPropertyPath()));
        case 1:
            return _MakeRandomPath(
                ret.AppendMapper(_MakeRandomPrimOrPropertyPath()));
        case 2:
            return _MakeRandomPath(ret.AppendExpression());
        case 3:
            return ret;
        };
    }

    // Extend a TargetPath
    if (ret.IsTargetPath()) {
        return (_Rand() & 1) ? ret :
            _MakeRandomPath(
            ret.AppendRelationalAttribute(_GetRandomNameToken()));
    }

    // Extend a MapperPath
    if (ret.IsMapperPath()) {
        return (_Rand() & 1) ? ret :
            _MakeRandomPath(ret.AppendMapperArg(_GetRandomNameToken()));
    }

    // Extend a RelationalAttributePath
    if (ret.IsRelationalAttributePath()) {
        return (_Rand() & 1) ? ret :
            _MakeRandomPath(ret.AppendTarget(_MakeRandomPrimOrPropertyPath()));
    }

    return ret;
}

TF_MAKE_STATIC_DATA(SdfPathVector, pathCache)
{
    static const size_t pathCacheSize = 32;
    *pathCache = SdfPathVector(pathCacheSize);
    for (size_t i = 0; i != pathCache->size(); ++i) 
        (*pathCache)[i] = _MakeRandomPath();
}

static TfStaticData<std::mutex> pathCacheMutex;

static void _PutPath(SdfPath const &path)
{
    std::lock_guard<std::mutex> lock(*pathCacheMutex);
    size_t index = _Rand() % pathCache->size();
    (*pathCache)[index] = path;
}

static SdfPath _GetPath()
{
    std::lock_guard<std::mutex> lock(*pathCacheMutex);
    size_t index = _Rand() % pathCache->size();
    return (*pathCache)[index];
}

static std::atomic_int nIters(0);

static TfStopwatch _DoPathOperations()
{
    TfStopwatch sw;

    while (static_cast<size_t>(sw.GetMilliseconds()) < msecsToRun) {
        sw.Start();
        SdfPath p = (_Rand() & 1) ? _GetPath() : SdfPath::AbsoluteRootPath();
        // If the path is not very extensible, trim it back to the prim path.
        if (p.IsExpressionPath() || p.IsMapperArgPath() || p.IsMapperPath())
            p = p.GetPrimPath();

        SdfPath randomP = _MakeRandomPath(p);
        _PutPath(randomP);

        sw.Stop();
        ++nIters;
    }

    return sw;
}

int main(int argc, char const **argv)
{
    // Set up arguments and their defaults
    CLI::App app("Tests SdfPath threading", "testSdfPathThreading");
    app.add_option("--seed", randomSeed, "Random seed")
        ->default_val(time(NULL));
    app.add_option("--numThreads", numThreads, "Number of threads to use")
        ->default_val(std::thread::hardware_concurrency());
    app.add_option("--msec", msecsToRun, "Milliseconds to run")
        ->default_val(2000);

    CLI11_PARSE(app, argc, argv);

    // Initialize. 
    srand(randomSeed);
    printf("Using random seed: %d\n", randomSeed);
    printf("Using %zu threads\n", numThreads);

    // Run.
    TfStopwatch sw;
    sw.Start();

    std::vector<std::thread> workers;
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back(_DoPathOperations);
    }

    std::for_each(workers.begin(), workers.end(), 
                  [](std::thread& t) { t.join(); });
    
    sw.Stop();

    // Report.
    printf("Ran %d SdfPath operations on %zu thread%s in %.3f sec "
           "(%.3f ops/sec)\n", (int)nIters, numThreads,
           numThreads > 1 ? "s" : "", sw.GetSeconds(),
           double((int)nIters) / sw.GetSeconds());
    return 0;
}

