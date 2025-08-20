//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/execUsd/cacheView.h"
#include "pxr/exec/execUsd/request.h"
#include "pxr/exec/execUsd/system.h"
#include "pxr/exec/execUsd/valueKey.h"

#include "pxr/base/arch/timing.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/pxrCLI11/CLI11.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/trace/aggregateNode.h"
#include "pxr/base/trace/collector.h"
#include "pxr/base/trace/reporter.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/threadLimits.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/stage.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE
using namespace pxr_CLI;

// Creates a hierarchy of Xform prims.
static void
_CreateDescendantPrims(
    const SdfPrimSpecHandle root,
    const unsigned branchingFactor,
    const unsigned treeDepth,
    std::vector<SdfPath> *const leafPrims)
{
    // Traversal state vector: Each entry contains the parent prim spec and
    // the current traversal depth.
    std::vector<std::pair<SdfPrimSpecHandle, unsigned>>
        traversalState{{root, 1}};

    while (!traversalState.empty()) {
        auto [parent, currentDepth] = traversalState.back();
        traversalState.pop_back();

        ++currentDepth;
        if (currentDepth > treeDepth) {
            continue;
        }

        for (unsigned i=0; i<branchingFactor; ++i) {
            const SdfPrimSpecHandle primSpec =
                SdfPrimSpec::New(
                    parent,
                    TfStringPrintf("Prim%u", i),
                    SdfSpecifierDef, "Xform");
            TF_AXIOM(primSpec);

            const VtValue xformOpValue(VtStringArray({"xformOp:transform"}));
            const SdfAttributeSpecHandle xformOpAttr =
                SdfAttributeSpec::New(
                    primSpec,
                    "xformOpOrder",
                    SdfGetValueTypeNameForValue(xformOpValue),
                    SdfVariabilityUniform);
            xformOpAttr->SetDefaultValue(xformOpValue);

            GfMatrix4d transform{1};
            transform.SetTranslate(GfVec3d(1, 0, 0));
            const VtValue transformValue{transform};
            const SdfAttributeSpecHandle transformAttr =
                SdfAttributeSpec::New(
                    primSpec,
                    "xformOps:transform",
                    SdfGetValueTypeNameForValue(transformValue));
            transformAttr->SetDefaultValue(transformValue);

            if (currentDepth == treeDepth) {
                leafPrims->push_back(primSpec->GetPath());
            }

            traversalState.emplace_back(primSpec, currentDepth);
        }
    }
}

// Creates a stage and populates it with a hierarchy of Xform prims with the
// given branching factor and depth.
static UsdStageConstRefPtr
_CreateStage(
    const unsigned branchingFactor,
    const unsigned treeDepth,
    std::vector<SdfPath> *const leafPrims)
{
    TRACE_FUNCTION();

    std::cout << "Creating Xform tree with branching factor "
              << branchingFactor << " and tree depth "
              << treeDepth << "\n";

    const unsigned long numPrims =
        branchingFactor == 1
        ? treeDepth
        : (std::pow(branchingFactor, treeDepth) - 1) / (branchingFactor - 1);
    const unsigned long numLeafPrims = std::pow(branchingFactor, treeDepth-1);
    std::cout << "The tree will contain " << numPrims
              << " prims and " << numLeafPrims << " leaf prims.\n";

    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");

    const SdfPrimSpecHandle primSpec =
        SdfPrimSpec::New(layer, "Root", SdfSpecifierDef, "Xform");
    TF_AXIOM(primSpec);

    const VtValue xformOpValue(VtStringArray({"xformOp:transform"}));
    const SdfAttributeSpecHandle xformOpAttr =
        SdfAttributeSpec::New(
            primSpec,
            "xformOpOrder",
            SdfGetValueTypeNameForValue(xformOpValue),
            SdfVariabilityUniform);
    xformOpAttr->SetDefaultValue(xformOpValue);

    const VtValue transformValue(GfMatrix4d(1));
    const SdfAttributeSpecHandle transformAttr =
        SdfAttributeSpec::New(
            primSpec,
            "xformOps:transform",
            SdfGetValueTypeNameForValue(transformValue));
    transformAttr->SetDefaultValue(transformValue);

    _CreateDescendantPrims(
        primSpec, branchingFactor, treeDepth, leafPrims);

    // Make sure we ended up with the correct number of leaf nodes.
    TF_AXIOM(leafPrims->size() == numLeafPrims);

    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    return usdStage;
}

// Looks for the trace aggregate node with the given key among the children of
// the given parent node.
//
static TraceAggregateNodePtr
_FindTraceNode(
    const TraceAggregateNodePtr &parent,
    const std::string &key)
{
    for (const TraceAggregateNodePtr &child : parent->GetChildren()) {
        // We look for a key that ends with the search string, rather than
        // require an exact match to account for the fact that in pxr-namespaced
        // builds, trace function keys are generated from namespaced symbols.
        if (TfStringEndsWith(child->GetKey().GetString(), key)) {
            return child;
        }
    }

    return {};
}

static double
_GetInclusiveTimeInSeconds(const TraceAggregateNodePtr &node)
{
    return ArchTicksToSeconds(node->GetInclusiveTime());
}

// Given a parent trace node and a tag name, returns inclusive times for
// compilation, scheduling, cache values, and value extraction.
static std::tuple<double, double, double, double>
_GetExecTimes(
    const TraceAggregateNodePtr &parentNode,
    const std::string &tag)
{
    const TraceAggregateNodePtr tagNode = _FindTraceNode(parentNode, tag);
    TF_AXIOM(tagNode);

    const TraceAggregateNodePtr prepareNode =
        _FindTraceNode(tagNode, "ExecUsdSystem::PrepareRequest");
    TF_AXIOM(prepareNode);

    const TraceAggregateNodePtr compileNode =
        _FindTraceNode(prepareNode, "ExecUsd_RequestImpl::Compile");
    TF_AXIOM(compileNode);
    const double compileTime = _GetInclusiveTimeInSeconds(compileNode);

    const TraceAggregateNodePtr scheduleNode =
        _FindTraceNode(prepareNode, "VdfScheduler::Schedule");
    TF_AXIOM(scheduleNode);
    const double scheduleTime = _GetInclusiveTimeInSeconds(scheduleNode);

    const TraceAggregateNodePtr cacheValuesNode =
        _FindTraceNode(tagNode, "ExecUsdSystem::Compute");
    TF_AXIOM(cacheValuesNode);
    const double cacheValuesTime = _GetInclusiveTimeInSeconds(cacheValuesNode);

    const TraceAggregateNodePtr extractValuesNode =
        _FindTraceNode(tagNode, "Extract values");
    TF_AXIOM(extractValuesNode);
    const double extractValuesTime =
        _GetInclusiveTimeInSeconds(extractValuesNode);

    return {compileTime, scheduleTime, cacheValuesTime, extractValuesTime};
}

static void
_WritePerfstats(
    const TraceReporterPtr &globalReporter,
    bool recompile)
{
    std::ofstream statsFile("perfstats.raw");
    static const char *const metricTemplate =
        "{'profile':'%s','metric':'time','value':%f,'samples':1}\n";

    const TraceAggregateNodePtr root =
        globalReporter->GetAggregateTreeRoot();

    const TraceAggregateNodePtr mainThreadNode =
        _FindTraceNode(root, "Main Thread");
    TF_AXIOM(mainThreadNode);
    const double mainThreadTime = _GetInclusiveTimeInSeconds(mainThreadNode);

    auto [compileTime, scheduleTime, cacheValuesTime, extractValuesTime] =
        _GetExecTimes(mainThreadNode, "Initial exec");

    statsFile << TfStringPrintf(
        metricTemplate, "time", mainThreadTime);
    statsFile << TfStringPrintf(
        metricTemplate, "compile_time", compileTime);
    statsFile << TfStringPrintf(
        metricTemplate, "schedule_time", scheduleTime);
    statsFile << TfStringPrintf(
        metricTemplate, "cache_values_time", cacheValuesTime);
    statsFile << TfStringPrintf(
        metricTemplate, "extract_values_time", extractValuesTime);

    if (!recompile) {
        return;
    }

    // Scene edit 1

    const TraceAggregateNodePtr sceneEdit1Node =
        _FindTraceNode(mainThreadNode, "Scene edit 1");
    TF_AXIOM(sceneEdit1Node);
    const double sceneEdit1Time =
        _GetInclusiveTimeInSeconds(sceneEdit1Node);

    auto [recompile1Time, reschedule1Time,
          cacheValues1Time, extractValues1Time] =
        _GetExecTimes(mainThreadNode, "Post-scene edit 1");

    statsFile << TfStringPrintf(
        metricTemplate, "scene_edit_1_time", sceneEdit1Time);
    statsFile << TfStringPrintf(
        metricTemplate, "recompile_1_time", recompile1Time);
    statsFile << TfStringPrintf(
        metricTemplate, "reschedule_1_time", reschedule1Time);
    statsFile << TfStringPrintf(
        metricTemplate, "cache_values_1_time", cacheValues1Time);
    statsFile << TfStringPrintf(
        metricTemplate, "extract_values_1_time", extractValues1Time);

    // Scene edit 2

    const TraceAggregateNodePtr sceneEdit2Node =
        _FindTraceNode(mainThreadNode, "Scene edit 2");
    TF_AXIOM(sceneEdit2Node);
    const double sceneEdit2Time =
        _GetInclusiveTimeInSeconds(sceneEdit2Node);

    auto [recompile2Time, reschedule2Time,
          cacheValues2Time, extractValues2Time] =
        _GetExecTimes(mainThreadNode, "Post-scene edit 2");

    statsFile << TfStringPrintf(
        metricTemplate, "scene_edit_2_time", sceneEdit2Time);
    statsFile << TfStringPrintf(
        metricTemplate, "recompile_2_time", recompile2Time);
    statsFile << TfStringPrintf(
        metricTemplate, "reschedule_2_time", reschedule2Time);
    statsFile << TfStringPrintf(
        metricTemplate, "cache_values_2_time", cacheValues2Time);
    statsFile << TfStringPrintf(
        metricTemplate, "extract_values_2_time", extractValues2Time);
}

namespace {

// Class used to gather and report memory measurements.
class _MemoryMetrics {
public:
    void
    RecordMetric(const char *const tag)
    {
        // If we're not measuring memory, return early.
        if (!TfMallocTag::IsInitialized()) {
            return;
        }

        const size_t memInBytes = TfMallocTag::GetMaxTotalBytes();

        TfMallocTag::CallTree tree;
        const bool success = TfMallocTag::GetCallTree(&tree);
        if (!TF_VERIFY(success)) {
            return;
        }

        std::ofstream mallocTagFile(TfStringPrintf("%s.mallocTag", tag));
        tree.Report(mallocTagFile);

        _stats.emplace_back(tag, memInBytes);
    }

    void
    WritePerfstats(const bool recompile)
    {
        _stats.emplace_back(
            "mem_high_water_mark", TfMallocTag::GetMaxTotalBytes());

        std::ofstream statsFile("perfstats.raw");
        for (const auto &[tag, memInBytes] : _stats) {
            _DumpMemStat(tag, memInBytes, statsFile);
        }
    }

private:
    static double
    _BytesToMiB(size_t numBytes)
    {
        return static_cast<double>(numBytes) / (1024 * 1024);
    }

    static void
    _DumpMemStat(
        const std::string &tag,
        const size_t memInBytes,
        std::ofstream &file)
    {
        const std::string memInMiBString =
            TfStringPrintf("%f", _BytesToMiB(memInBytes));

        // Print the value to stdout, with a label.
        const std::string label = TfStringReplace(tag, "_", " ");
        std::cout << label << ": " << memInMiBString << " MiB\n";

        // Write the value to the perfstats file.
        static const char *const metricTemplate =
            "{'profile':'%s','metric':'memory','value':%s,'samples':1}\n";
        file << TfStringPrintf(
            metricTemplate, tag.c_str(), memInMiBString.c_str());
    }

private:

    // Vector of (tag, memory in bytes) for each stat collected.
    std::vector<std::pair<std::string, size_t>> _stats;
};

} // anonymous namespace

static void
TestExecGeomXformable_Perf(
    const unsigned branchingFactor,
    const unsigned treeDepth,
    const bool measureMemory,
    const bool recompile,
    const bool outputAsSpy)
{
    _MemoryMetrics memMetrics;

    if (measureMemory) {
        std::string errorMessage;
        const bool initialized = TfMallocTag::Initialize(&errorMessage);
        TF_VERIFY(
            initialized,
            "Failed to initialize TfMallocTag: %s", errorMessage.c_str());
    } else {
        TraceCollector::GetInstance().SetEnabled(true);
    }

    // Instantiate a hierarchy of Xform prims on a stage and get access to the
    // leaf prims.
    std::vector<SdfPath> leafPrims;
    const UsdStageConstRefPtr usdStage =
        _CreateStage(branchingFactor, treeDepth, &leafPrims);

    // Call IsValid on an attribute as a way to ensure that the
    // UsdSchemaRegistry has been populated before starting compilation.
    {
        TRACE_SCOPE("Preroll stage access");
        UsdPrim prim = usdStage->GetPrimAtPath(SdfPath("/Root"));
        UsdAttribute attribute =
            prim.GetAttribute(TfToken("xformOps:transform"));
        TF_AXIOM(attribute.IsValid());
    }

    TRACE_MARKER("Begin exec");
    memMetrics.RecordMetric("mem_at_start");

    ExecUsdSystem execSystem(usdStage);

    // Create value keys that compute the transforms for all leaf prims in
    // the namespace hierarchy.
    ExecUsdRequest request = [&]{
        TRACE_SCOPE("Build request 1");

        std::vector<ExecUsdValueKey> valueKeys;
        valueKeys.reserve(leafPrims.size());
        for (const SdfPath &path : leafPrims) {
            UsdPrim prim = usdStage->GetPrimAtPath(path);
            TF_AXIOM(prim.IsValid());
            valueKeys.emplace_back(
                prim, TfToken("computeLocalToWorldTransform"));
        }

        return execSystem.BuildRequest(std::move(valueKeys));
    }();
    TF_AXIOM(request.IsValid());

    {
        TRACE_SCOPE("Initial exec");

        execSystem.PrepareRequest(request);
        TF_AXIOM(request.IsValid());
        memMetrics.RecordMetric("mem_prepare_request_1");

        ExecUsdCacheView cache = execSystem.Compute(request);
        memMetrics.RecordMetric("mem_cache_values_1");

        {
            TRACE_SCOPE("Extract values");

            // The expected result translation, given that all transforms impart
            // a unit translation in X, except the root.
            GfVec3d expectedTranslation(treeDepth-1, 0, 0);

            for (size_t idx=0; idx<leafPrims.size(); ++idx) {
                VtValue value = cache.Get(idx);
                TF_AXIOM(!value.IsEmpty());
                const GfMatrix4d matrix = value.Get<GfMatrix4d>();
                TF_AXIOM(matrix.ExtractTranslation() == expectedTranslation);
            }
        }
    }

    if (recompile) {
        // The first scene edit modifies changes the type of one child of the
        // root prim from Xform to Scope. Currently, this recursively resyncs
        // all descendant prims.
        TRACE_MARKER("Scene edit 1");

        {
            TRACE_SCOPE("Scene edit 1");

            const SdfPrimSpecHandle rootChildSpec =
                usdStage->GetRootLayer()->GetPrimAtPath(SdfPath("/Root/Prim0"));
            TF_AXIOM(rootChildSpec);
            rootChildSpec->SetTypeName("Scope");
        }
        memMetrics.RecordMetric("mem_scene_edit_1");

        TRACE_MARKER("Re-exec 1");

        {
            TRACE_SCOPE("Post-scene edit 1");

            execSystem.PrepareRequest(request);
            TF_AXIOM(request.IsValid());
            memMetrics.RecordMetric("mem_prepare_request_2");

            ExecUsdCacheView cache = execSystem.Compute(request);
            memMetrics.RecordMetric("mem_cache_values_2");

            {
                TRACE_SCOPE("Extract values");

                for (size_t idx=0; idx<leafPrims.size(); ++idx) {
                    VtValue value = cache.Get(idx);
                    TF_AXIOM(!value.IsEmpty());
                }
            }
        }

        // The second scene edit changes the types for half of the leaf prims
        // from Xform to Scope. This invalidates value keys, so we re-build the
        // request for the leaf prims that remain unchanged.
        //
        // This is set up so that we end up with lots of isolated network that
        // needs to be uncompiled.
        //
        TRACE_MARKER("Scene edit 2");

        {
            TRACE_SCOPE("Scene edit 2");

            for (size_t i=leafPrims.size()/2; i<leafPrims.size(); ++i) {
                const SdfPrimSpecHandle leafPrimSpec =
                    usdStage->GetRootLayer()->GetPrimAtPath(leafPrims[i]);
                TF_AXIOM(leafPrimSpec);
                leafPrimSpec->SetTypeName("Scope");
            }
        }
        memMetrics.RecordMetric("mem_scene_edit_2");

        TRACE_MARKER("Re-exec 2");

        request = [&]{
            TRACE_SCOPE("Build request 2");

            std::vector<ExecUsdValueKey> valueKeys;
            valueKeys.reserve(leafPrims.size());
            for (size_t i=0; i<leafPrims.size()/2; ++i) {
                UsdPrim prim = usdStage->GetPrimAtPath(leafPrims[i]);
                TF_AXIOM(prim.IsValid());
                valueKeys.emplace_back(
                    prim, TfToken("computeLocalToWorldTransform"));
            }

            return execSystem.BuildRequest(std::move(valueKeys));
        }();
        TF_AXIOM(request.IsValid());

        {
            TRACE_SCOPE("Post-scene edit 2");

            execSystem.PrepareRequest(request);
            TF_AXIOM(request.IsValid());
            memMetrics.RecordMetric("mem_prepare_request_3");

            // TODO: Compute and extract values again.
            ExecUsdCacheView cache = execSystem.Compute(request);
            memMetrics.RecordMetric("mem_cache_values_3");

            {
                TRACE_SCOPE("Extract values");

                for (size_t idx=0; idx<leafPrims.size()/2; ++idx) {
                    VtValue value = cache.Get(idx);
                    TF_AXIOM(!value.IsEmpty());
                }
            }
        }
    }

    if (measureMemory) {
        memMetrics.RecordMetric("mem_at_end");
        memMetrics.WritePerfstats(recompile);
    } else {
        TraceCollector::GetInstance().SetEnabled(false);

        const TraceReporterPtr globalReporter =
            TraceReporter::GetGlobalReporter();
        globalReporter->UpdateTraceTrees();

        {
            if (outputAsSpy) {
                std::ofstream traceFile("test.spy");
                globalReporter->SerializeProcessedCollections(traceFile);
            } else {
                std::ofstream traceFile("test.trace");
                globalReporter->Report(traceFile);
            }
        }

        _WritePerfstats(globalReporter, recompile);
    }
}

int 
main(int argc, char **argv) 
{
    unsigned branchingFactor = 0;
    unsigned treeDepth = 0;
    unsigned numThreads = WorkGetConcurrencyLimit();
    bool measureMemory = false;
    bool outputAsSpy = false;
    bool recompile = false;

    // Set up arguments and their defaults
    CLI::App app(
        "Creates a transform hierarchy by building a regular tree of Xform "
        "prims where each prim has <branchingFactor> children with an overall "
        "tree depth of <treeDepth>.",
        "testExecGeomXformable_Perf");
    app.add_option(
        "--branchingFactor", branchingFactor,
        "Branching factor used to build the Xform tree")
        ->required(true);
    app.add_option(
        "--treeDepth", treeDepth,
        "The depth of the Xform tree to build.")
        ->required(true);
    app.add_option(
        "--numThreads", numThreads, "The number of threads to use.");
    app.add_flag(
        "--memory", measureMemory,
        "Measure memory, instead of time (the default).");
    app.add_flag(
        "--recompile", recompile,
        "Measure recompilation time in response to various scene edits.");
    app.add_flag(
        "--spy", outputAsSpy,
        "Report traces in .spy format.");

    CLI11_PARSE(app, argc, argv);

    std::cout << "Running with " << numThreads << " threads.\n";
    WorkSetConcurrencyLimit(numThreads);

    TestExecGeomXformable_Perf(
        branchingFactor, treeDepth, measureMemory, recompile, outputAsSpy);
}
