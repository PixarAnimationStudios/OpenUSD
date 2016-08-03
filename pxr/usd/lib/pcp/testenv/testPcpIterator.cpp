#include "pxr/usd/pcp/arc.h"
#include "pxr/usd/pcp/cache.h"
#include "pxr/usd/pcp/diagnostic.h"
#include "pxr/usd/pcp/errors.h"
#include "pxr/usd/pcp/iterator.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/primIndex.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/site.h"
#include "pxr/usd/sdf/siteUtils.h"

#include "pxr/base/tf/pyInterpreter.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include <fstream>
#include <iostream>
#include <mutex>
#include <boost/python.hpp>

static void
_ValidateAndPrintNode(
    std::ostream& out, const PcpNodeRef& node)
{
    TF_AXIOM(node);

    out << Pcp_FormatSite(node.GetSite())
        << "\t"
        << TfEnum::GetDisplayName(node.GetArcType()).c_str()
        ;
}

static void
_ValidateAndPrintPrimFromNode(
    std::ostream& out, 
    const SdfSite& sdSite,
    const PcpNodeRef& node)
{
    TF_AXIOM(SdfGetPrimAtPath(sdSite));
    TF_AXIOM(node);
    TF_AXIOM(node.GetSite().path == sdSite.path);
    TF_AXIOM(node.GetSite().layerStack->GetIdentifier().rootLayer == 
             sdSite.layer);

    out << Pcp_FormatSite(node.GetSite())
        << "\t"
        << TfEnum::GetDisplayName(node.GetArcType()).c_str()
        ;
}

static std::string
_GetRangeType(PcpRangeType type)
{
    return TfEnum::GetDisplayName(type);
}

static void
_IterateAndPrintPrimIndexNodes(
    std::ostream& out,
    PcpCache* cache, 
    const PcpPrimIndex& primIndex,
    PcpRangeType type = PcpRangeTypeAll)
{
    out << "Iterating over " << _GetRangeType(type) << " nodes for " 
        << TfStringPrintf("<%s>:", primIndex.GetRootNode().GetSite().path.GetText())
        << std::endl;

    TF_FOR_ALL(it, primIndex.GetNodeRange(type)) {
        out << " ";
        _ValidateAndPrintNode(out, *it);
        out << std::endl;
    }

    out << std::endl;
    out << "Reverse iterating over " << _GetRangeType(type) << " nodes for " 
        << TfStringPrintf("<%s>:", primIndex.GetRootNode().GetSite().path.GetText())
        << std::endl;

    TF_REVERSE_FOR_ALL(it, primIndex.GetNodeRange(type)) {
        out << " ";
        _ValidateAndPrintNode(out, *it);
        out << std::endl;
    }
}

static void 
_IterateAndPrintPrimIndexPrims(
    std::ostream& out,
    PcpCache* cache,
    const PcpPrimIndex& primIndex,
    PcpRangeType type = PcpRangeTypeAll)
{
    out << "Iterating over " << _GetRangeType(type) << " prim specs for " 
        << TfStringPrintf("<%s>:", primIndex.GetRootNode().GetSite().path.GetText())
        << std::endl;

    TF_FOR_ALL(it, primIndex.GetPrimRange(type)) {
        out << " ";
        _ValidateAndPrintPrimFromNode(out, *it, it.base().GetNode());
        out << std::endl;
    }

    out << std::endl;
    out << "Reverse iterating over " << _GetRangeType(type) << " prim specs for " 
        << TfStringPrintf("<%s>:", primIndex.GetRootNode().GetSite().path.GetText())
        << std::endl;

    TF_REVERSE_FOR_ALL(it, primIndex.GetPrimRange(type)) {
        out << " ";
        _ValidateAndPrintPrimFromNode(out, *it, it.base().GetNode());
        out << std::endl;
    }
}

static void
_IterateAndPrintPrimIndex(
    std::ostream& out,
    PcpCache* cache,
    const SdfPath& primPath,
    PcpRangeType type = PcpRangeTypeAll)
{
    PcpErrorVector errors;
    const PcpPrimIndex& primIndex = cache->ComputePrimIndex(primPath, &errors);
    PcpRaiseErrors(errors);

    _IterateAndPrintPrimIndexNodes(out, cache, primIndex, type);
    out << std::endl;
    _IterateAndPrintPrimIndexPrims(out, cache, primIndex, type);
}

static void
_IterateAndPrintPropertyIndex(
    std::ostream& out,
    PcpCache* cache,
    const SdfPath& propPath,
    bool directOnly)
{
    PcpErrorVector errors;
    const PcpPropertyIndex& propIndex = 
        cache->ComputePropertyIndex(propPath, &errors);
    PcpRaiseErrors(errors);

    out << "Iterating over " << (directOnly ? "local" : "all") 
        << " property specs for " << TfStringPrintf("<%s>:", propPath.GetText())
        << std::endl;

    TF_FOR_ALL(it, propIndex.GetPropertyRange(directOnly)) {
        out << " "
            << Pcp_FormatSite(PcpSite((*it)->GetLayer(), (*it)->GetPath()))
            << " from node ";
        _ValidateAndPrintNode(out, it.base().GetNode());
        out << std::endl;
    }

    out << std::endl;

    out << "Reverse iterating over " << (directOnly ? "local" : "all") 
        << " property specs for " << TfStringPrintf("<%s>:", propPath.GetText())
        << std::endl;

    TF_REVERSE_FOR_ALL(it, propIndex.GetPropertyRange(directOnly)) {
        out << " "
            << Pcp_FormatSite(PcpSite((*it)->GetLayer(), (*it)->GetPath()))
            << " from node ";
        _ValidateAndPrintNode(out, it.base().GetNode());
        out << std::endl;
    }
}

template <class IteratorType>
static void
_TestComparisonOperations(IteratorType first, IteratorType last)
{
    TF_AXIOM(first != last);

    IteratorType first2 = first, last2 = last;
    do {
        TF_AXIOM(first == first2);
        ++first;
        TF_AXIOM(first != first2);
        ++first2;
    }
    while (first != last and first2 != last2);
}

template <class IteratorType>
static void
_TestRandomAccessOperations(IteratorType first, IteratorType last)
{
    TF_AXIOM(first != last);

    size_t idx = 0;
    for (IteratorType it = first; it != last; ++it, ++idx) {
        TF_AXIOM(it - first == idx);
        TF_AXIOM(it - idx == first);
        TF_AXIOM(it == first + idx);
    }
}

static boost::shared_ptr<PcpCache>
_CreateCacheForRootLayer(const std::string& rootLayerPath)
{
    SdfLayerRefPtr rootLayer = SdfLayer::FindOrOpen(rootLayerPath);
    if (not rootLayer) {
        return boost::shared_ptr<PcpCache>();
    }

    const PcpLayerStackIdentifier layerStackID(
        rootLayer, SdfLayerRefPtr(), ArResolverContext());
    return boost::shared_ptr<PcpCache>(new PcpCache(layerStackID));
}

// Copied from testSdGetObjectAtPath.. and a ton of other places.
// See bug 54798.
static
std::string
_FindDataFile(const std::string& file)
{
    static std::once_flag importOnce;
    std::call_once(importOnce, [](){
        const std::string importFindDataFile = "from Mentor.Runtime import *";
        if (TfPyRunSimpleString(importFindDataFile) != 0) {
            TF_FATAL_ERROR("ERROR: Could not import FindDataFile");
        }
    });

    const std::string findDataFile =
        TfStringPrintf("FindDataFile(\'%s\')", file.c_str());

    using namespace boost::python;
    const object resultObj(TfPyRunString(findDataFile, Py_eval_input));
    const extract<std::string> dataFileObj(resultObj);

    if (not dataFileObj.check()) {
        TF_FATAL_ERROR("ERROR: Could not extract result of FindDataFile");
        return std::string();
    }

    return dataFileObj();
}

int 
main(int argc, char** argv)
{
    if (argc != 1 and argc != 3) {
        std::cerr << "usage: " << std::endl;
        std::cerr << argv[0] << std::endl;
        std::cerr << "\tRuns standard built-in tests" << std::endl;
        std::cerr << argv[0] << " root_layer prim_path" << std::endl;
        std::cerr << "\tPrints results of iteration over prim_path in scene "
                  << "with given root_layer" << std::endl;
        std::cerr << "\tex: " << argv[0] << " root.menva /Model" << std::endl;
        return EXIT_FAILURE;
    }

    // Handle case where user specifies root layer and prim path to iterate
    // over.
    if (argc == 3) {
        const std::string layerPath(argv[1]);
        const SdfPath primPath(argv[2]);

        boost::shared_ptr<PcpCache> cache = _CreateCacheForRootLayer(layerPath);
        if (not cache) {
            std::cerr << "Failed to load root layer " << layerPath << std::endl;
            return EXIT_FAILURE;
        }

        _IterateAndPrintPrimIndex(std::cout, cache.get(), primPath);
        return EXIT_SUCCESS;
    }

    // Otherwise, run the normal test suite.
    boost::shared_ptr<PcpCache> cache = _CreateCacheForRootLayer(
        _FindDataFile("testPcpIterator.testenv/root.sdf"));
    TF_AXIOM(cache);

    SdfPathSet includePayload;
    includePayload.insert(SdfPath("/Model"));
    cache->RequestPayloads(includePayload, SdfPathSet());

    std::cout << "Testing comparison operators..." << std::endl;
    {
        PcpErrorVector errors;
        const PcpPrimIndex& primIndex = 
            cache->ComputePrimIndex(SdfPath("/Model"), &errors);
        PcpRaiseErrors(errors);

        const PcpNodeRange nodeRange = primIndex.GetNodeRange();
        _TestComparisonOperations(nodeRange.first, nodeRange.second);

        const PcpPrimRange primRange = primIndex.GetPrimRange();
        _TestComparisonOperations(primRange.first, primRange.second);

        const PcpPropertyIndex& propIndex = 
            cache->ComputePropertyIndex(SdfPath("/Model.a"), &errors);
        PcpRaiseErrors(errors);

        const PcpPropertyRange propRange = propIndex.GetPropertyRange();
        _TestComparisonOperations(propRange.first, propRange.second);
    }

    std::cout << "Testing random access operations..." << std::endl;
    {
        PcpErrorVector errors;
        const PcpPrimIndex& primIndex = 
            cache->ComputePrimIndex(SdfPath("/Model"), &errors);
        PcpRaiseErrors(errors);

        const PcpNodeRange nodeRange = primIndex.GetNodeRange();
        _TestRandomAccessOperations(nodeRange.first, nodeRange.second);
        _TestRandomAccessOperations(
            PcpNodeReverseIterator(nodeRange.second),
            PcpNodeReverseIterator(nodeRange.first));

        const PcpPrimRange primRange = primIndex.GetPrimRange();
        _TestRandomAccessOperations(primRange.first, primRange.second);
        _TestRandomAccessOperations(
            PcpPrimReverseIterator(primRange.second),
            PcpPrimReverseIterator(primRange.first));

        const PcpPropertyIndex& propIndex = 
            cache->ComputePropertyIndex(SdfPath("/Model.a"), &errors);
        PcpRaiseErrors(errors);

        const PcpPropertyRange propRange = propIndex.GetPropertyRange();
        _TestRandomAccessOperations(propRange.first, propRange.second);
        _TestRandomAccessOperations(
            PcpPropertyReverseIterator(propRange.second),
            PcpPropertyReverseIterator(propRange.first));
    }

    std::cout << "Testing iteration (output to file)..." << std::endl;
    {
        std::ofstream outfile("iteration_results.txt");
        for (PcpRangeType type = PcpRangeTypeRoot;
             type != PcpRangeTypeInvalid;
             type = (PcpRangeType)(type + 1)) {

            _IterateAndPrintPrimIndex(
                outfile, cache.get(), SdfPath("/Model"), type);
            outfile << std::endl 
                    << "====================" << std::endl 
                    << std::endl;
        }

        _IterateAndPrintPropertyIndex(
            outfile, cache.get(), SdfPath("/Model.a"), /* directOnly */ true);

        outfile << std::endl 
                << "====================" << std::endl 
                << std::endl;

        _IterateAndPrintPropertyIndex(
            outfile, cache.get(), SdfPath("/Model.a"), /* directOnly */ false);
    }
    
    return EXIT_SUCCESS;
}
