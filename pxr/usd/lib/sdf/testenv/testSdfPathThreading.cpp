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
#include "pxr/pxr.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/stopwatch.h"
#include "pxr/base/tf/diagnostic.h"

#include <atomic>
#include <ctime>
#include <cstdlib>
#include <mutex>
#include <thread>

#include <boost/program_options.hpp>

using std::string;
using std::vector;
namespace boost_po = boost::program_options;

PXR_NAMESPACE_USING_DIRECTIVE

static unsigned int randomSeed;
static size_t numThreads;
static size_t msecsToRun = 2000;

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
    return (*nameTokens)[random() % nameTokens->size()];
}

static SdfPath
_MakeRandomPrimPath()
{
    static const size_t maxDepth = 2;
    SdfPath ret = SdfPath::AbsoluteRootPath();
    for (size_t i = 0, depth = random() % maxDepth; i <= depth; ++i)
        ret = ret.AppendChild(_GetRandomNameToken());
    return ret;
}

static SdfPath
_MakeRandomPrimOrPropertyPath()
{
    SdfPath ret = _MakeRandomPrimPath();
    return random() & 1 ? ret : ret.AppendProperty(_GetRandomNameToken());
}

static SdfPath
_MakeRandomPath(SdfPath const &path = SdfPath::AbsoluteRootPath())
{
    SdfPath ret = path;

    // Absolute root -> prim path.
    if (path == SdfPath::AbsoluteRootPath())
        ret = _MakeRandomPrimPath();

    // Extend a PrimPath.
    if (ret.IsPrimPath() && (random() & 1)) {
        ret = ret.AppendVariantSelection(_GetRandomNameToken().GetString(),
                                         _GetRandomNameToken().GetString());
    }

    // Extend a PrimPath or a PrimVariantSelectionPath.
    if ((ret.IsPrimPath() || ret.IsPrimVariantSelectionPath())) {
        return (random() & 1) ? ret :
            _MakeRandomPath(ret.AppendProperty(_GetRandomNameToken()));
    }

    // Extend a PrimPropertyPath
    if (ret.IsPrimPropertyPath()) {
        // options: target path, mapper path, expression path, or leave alone.
        switch (random() & 3) {
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
        return (random() & 1) ? ret :
            _MakeRandomPath(
            ret.AppendRelationalAttribute(_GetRandomNameToken()));
    }

    // Extend a MapperPath
    if (ret.IsMapperPath()) {
        return (random() & 1) ? ret :
            _MakeRandomPath(ret.AppendMapperArg(_GetRandomNameToken()));
    }

    // Extend a RelationalAttributePath
    if (ret.IsRelationalAttributePath()) {
        return (random() & 1) ? ret :
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
    size_t index = random() % pathCache->size();
    (*pathCache)[index] = path;
}

static SdfPath _GetPath()
{
    std::lock_guard<std::mutex> lock(*pathCacheMutex);
    size_t index = random() % pathCache->size();
    return (*pathCache)[index];
}

static std::atomic_int nIters(0);

static TfStopwatch _DoPathOperations()
{
    TfStopwatch sw;

    while (static_cast<size_t>(sw.GetMilliseconds()) < msecsToRun) {
        sw.Start();
        SdfPath p = (random() & 1) ? _GetPath() : SdfPath::AbsoluteRootPath();
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
    boost_po::options_description desc("Options");
    desc.add_options()

        ("seed",
        boost_po::value<unsigned int>(&randomSeed)->default_value(time(NULL)),
        "Random seed")

        ("numThreads", 
        boost_po::value<size_t>(&numThreads)->default_value(
            std::thread::hardware_concurrency()), 
        "Number of threads to use")

        ("msec", 
        boost_po::value<size_t>(&msecsToRun)->default_value(2000),
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
    srandom(randomSeed);
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

