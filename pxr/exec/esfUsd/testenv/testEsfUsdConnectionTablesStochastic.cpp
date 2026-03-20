//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/esfUsd/stageData.h"

#include "pxr/base/arch/timing.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/pxrCLI11/CLI11.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/base/trace/aggregateNode.h"
#include "pxr/base/trace/collector.h"
#include "pxr/base/trace/reporter.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/threadLimits.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/notice.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE;
using namespace pxr_CLI;

#define ASSERT_EQ(expr, expected)                                       \
    [&] {                                                               \
        std::cout << std::flush;                                        \
        std::cerr << std::flush;                                        \
        auto&& expr_ = expr;                                            \
        if (expr_ != expected) {                                        \
            TF_FATAL_ERROR(                                             \
                "Expected " TF_PP_STRINGIZE(expr) " == '%s'; got '%s'", \
                TfStringify(expected).c_str(),                          \
                TfStringify(expr_).c_str());                            \
        }                                                               \
    }()

static std::set<SdfPath>
_MakePathSet(const SdfPathVector &paths) {
    return std::set<SdfPath>(paths.begin(), paths.end());
}

static void
_AddPrimHierarchy(
    const SdfPrimSpecHandle &parentPrimSpec,
    const unsigned branchingFactor,
    const unsigned treeDepth,
    SdfPathVector *const attrPaths)
{
    if (treeDepth == 0) {
        return;
    }

    // Create all the necessary tokens for child prim names. The vector is
    // static so these are only generated once.
    static std::vector<TfToken> childNames;
    for (unsigned index = childNames.size(); index < branchingFactor; ++index) {
        childNames.emplace_back(TfStringPrintf("Prim%u", index));
    }

    static TfToken attrName("attr");

    for (unsigned i = 0; i < branchingFactor; ++i) {
        const SdfPrimSpecHandle childPrimSpec = SdfPrimSpec::New(
            parentPrimSpec,
            childNames[i].GetString(),
            SdfSpecifierDef,
            "Scope");
        TF_AXIOM(childPrimSpec);

        const SdfAttributeSpecHandle attrSpec = SdfAttributeSpec::New(
            childPrimSpec,
            attrName.GetString(),
            SdfGetValueTypeNameForValue(VtValue(0.0)));
        TF_AXIOM(attrSpec);
        attrPaths->push_back(attrSpec->GetPath());
            
        // Add the child's descendants.
        _AddPrimHierarchy(
            childPrimSpec, branchingFactor, treeDepth - 1, attrPaths);
    }
}

static void
_AddPrimHierarchy(
    const SdfLayerHandle &layer,
    const char *rootPrimName,
    const unsigned branchingFactor,
    const unsigned treeDepth,
    SdfPathVector *attrPaths)
{
    // Define the prim at the root of the hierarchy.
    const SdfPrimSpecHandle rootPrimSpec =
        SdfPrimSpec::New(layer, rootPrimName, SdfSpecifierDef, "Scope");
    TF_AXIOM(rootPrimSpec);
    
    // Define the descendant prims.
    _AddPrimHierarchy(rootPrimSpec, branchingFactor, treeDepth, attrPaths);
}

static void
_ValidateConnections(
    const UsdStageConstPtr &stage)
{
    TRACE_FUNCTION();

    // Compute the actual incoming connections by traversing all active, loaded,
    // defined, non-abstract prims, gathering their attributes, and building up
    // a map of incoming connections for each targeted attribute.
    std::unordered_map<SdfPath, std::set<SdfPath>, TfHash> actualIncoming;

    for (const UsdPrim &prim : stage->GetPseudoRoot().GetDescendants()) {
        for (const UsdAttribute &attr : prim.GetAttributes()) {
            SdfPathVector targetPaths;
            if (!attr.GetConnections(&targetPaths)) {
                continue;
            }
            const SdfPath ownerPath = attr.GetPath();

            ASSERT_EQ(
                EsfUsdStageData::GetOutgoingConnections(stage, ownerPath),
                targetPaths);

            for (const SdfPath &targetPath : targetPaths) {
                actualIncoming[targetPath].insert(ownerPath);
            }
        }
    }

    for (const auto &entry : actualIncoming) {
        const SdfPath &path = entry.first;
        const std::set<SdfPath> &actual = entry.second;
        ASSERT_EQ(
            _MakePathSet(EsfUsdStageData::GetIncomingConnections(stage, path)),
            actual);
    }
}

static void
_MutateScene(
    const unsigned randomSeed,
    const unsigned numIterations,
    const UsdStageRefPtr &stage,
    const SdfPathVector &attrPaths)
{
    std::mt19937 rng(randomSeed);
    std::uniform_int_distribution<size_t> randomPath(0, attrPaths.size()-1);
    std::uniform_int_distribution<size_t> randomOp(0, 3);

    for (size_t j=0; j<numIterations; ++j) {
        const size_t op = randomOp(rng);

        if (op == 0) {
            // If we find an attribute at the owner path, add a connection.
            const size_t ownerI = randomPath(rng);
            const size_t targetI = randomPath(rng);

            UsdAttribute owner = stage->GetAttributeAtPath(attrPaths[ownerI]);
            if (owner) {
                owner.AddConnection(attrPaths[targetI]);
            }
        } else if (op == 1) {
            // If we find an attribute at the owner path and it has connections,
            // remove a random connection.
            const size_t ownerI = randomPath(rng);

            UsdAttribute owner = stage->GetAttributeAtPath(attrPaths[ownerI]);
            if (owner) {
                SdfPathVector connections;
                owner.GetConnections(&connections);

                std::uniform_int_distribution<size_t> randomConnection(
                    0, connections.size()-1);
                const size_t connectionI = randomConnection(rng);

                if (!connections.empty()) {
                    owner.RemoveConnection(connections[connectionI]);
                }
            }
        } else if (op == 2) {
            // If we find an attribute at the path, remove its spec. Otherwise,
            // if there's an owning prim, create the attribute spec.
            const size_t attrI = randomPath(rng);
            const SdfPath &attrPath = attrPaths[attrI];

            const SdfLayerHandle layer = stage->GetRootLayer();
            const SdfPrimSpecHandle prim =
                layer->GetPrimAtPath(attrPath.GetPrimPath());
            TF_AXIOM(prim);

            if (const SdfAttributeSpecHandle attr =
                layer->GetAttributeAtPath(attrPath)) {
                prim->RemoveProperty(attr);
            } else {
                const SdfAttributeSpecHandle attrSpec = SdfAttributeSpec::New(
                    prim,
                    attrPath.GetName(),
                    SdfGetValueTypeNameForValue(VtValue(0.0)));
                TF_AXIOM(attrSpec);
            }
        } else if (op == 3) {
            // If we find a prim at the path, toggle its activation state.
            const size_t primI = randomPath(rng);

            UsdPrim prim = stage->GetPrimAtPath(attrPaths[primI].GetPrimPath());
            if (prim) {
                prim.SetActive(!prim.IsActive());
            }
        } else {
            TF_FATAL_ERROR("Unexpected op %zu", op);
        }
    }
}

namespace {

// RAII class for collecting and reporting trace information.
class _PerformanceTracker
{
public:
    _PerformanceTracker(bool outputAsSpy, bool outputAsTrace)
        : _outputAsSpy(outputAsSpy)
        , _outputAsTrace(outputAsTrace) 
    {
        TraceCollector::GetInstance().SetEnabled(true);
    }

    ~_PerformanceTracker() {
        TraceCollector::GetInstance().SetEnabled(false);
        const TraceReporterPtr reporter = TraceReporter::GetGlobalReporter();
        reporter->UpdateTraceTrees();
        _WriteStats(*reporter);
        if (_outputAsSpy) {
            _WriteSpy(*reporter);
        }
        if (_outputAsTrace) {
            _WriteTrace(*reporter);
        }
    }

private:

    // Gets a trace node child of \p rootNode.
    static TraceAggregateNodePtr _GetTraceNode(
        const TraceAggregateNodePtr &rootNode,
        const std::string &childName,
        const bool isOptional = false) {

        // Get the child node by exact name.
        TraceAggregateNodePtr childNode = rootNode->GetChild(childName);

        // If not found, the child must be optional.
        if (!childNode && !isOptional) {
            TF_FATAL_ERROR(
                "Expected trace node '%s' not found", childName.c_str());
        }

        return childNode;
    }

    // Writes a node time in seconds to the stats file.
    static void _WriteStat(
        const TraceAggregateNodePtr &node,
        const char *const profileName,
        std::ofstream *const statsFile) {
        
        const double timeInSeconds = node
            ? ArchTicksToSeconds(node->GetInclusiveTime())
            : 0.0;

        // Always write the metric to stdout.
        std::cout << profileName << ": " << timeInSeconds << '\n';

        // Write to the stats file, if provided.
        if (statsFile) {
            *statsFile << '{'
                << "'profile':'" << profileName << "',"
                << "'metric':'time',"
                << "'value':" << timeInSeconds << ','
                << "'samples':1"
                << "}\n";
        }
    }

    static void _WriteStat(
        const TraceAggregateNodePtr &node,
        std::ofstream *const statsFile) {
        _WriteStat(node, node->GetKey().GetText(), statsFile);
    }

    static void _WriteStats(TraceReporter &reporter) {
        std::cout << "Writing to perfstats.raw\n";
        std::ofstream statsFile("perfstats.raw");
        TF_AXIOM(statsFile);

        const TraceAggregateNodePtr rootNode = reporter.GetAggregateTreeRoot();
        TF_AXIOM(rootNode);
        const TraceAggregateNodePtr mainThreadNode =
            rootNode->GetChild("Main Thread");
        TF_AXIOM(mainThreadNode);

        _WriteStat(_GetTraceNode(
                       mainThreadNode, "populate_stage_time"), &statsFile);
        _WriteStat(_GetTraceNode(
                       mainThreadNode, "register_stage_data_time"), &statsFile);
        _WriteStat(_GetTraceNode(
                       mainThreadNode, "mutate_scene_time"), &statsFile);
        _WriteStat(_GetTraceNode(
                       mainThreadNode, "validate_connections_time"), &statsFile);
        _WriteStat(_GetTraceNode(
                       mainThreadNode, "deactivate_roots_time"), &statsFile);
        _WriteStat(_GetTraceNode(
                       mainThreadNode, "free_stage_data_time"), &statsFile);
        _WriteStat(_GetTraceNode(
                       mainThreadNode, "free_stages_time"), &statsFile);
    }

    static void _WriteTrace(TraceReporter &reporter) {
        std::cout << "Writing to test.trace.\n";
        std::ofstream traceFile("test.trace");
        TF_AXIOM(traceFile);
        reporter.Report(traceFile);
    }

    static void _WriteSpy(TraceReporter &reporter) {
        std::cout << "Writing to test.spy.\n";
        std::ofstream spyFile("test.spy");
        TF_AXIOM(spyFile);
        reporter.SerializeProcessedCollections(spyFile);
    }

private:
    bool _outputAsSpy = false;
    bool _outputAsTrace = false;
};

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

        const size_t memInBytes = TfMallocTag::GetTotalBytes();

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
    WritePerfstats()
    {
        // If we're not measuring memory, return early.
        if (!TfMallocTag::IsInitialized()) {
            return;
        }

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
            "{'profile':'%s','metric':'malloc_size','value':%s,'samples':1}\n";
        file << TfStringPrintf(
            metricTemplate, tag.c_str(), memInMiBString.c_str());
    }

private:

    // Vector of (tag, memory in bytes) for each stat collected.
    std::vector<std::pair<std::string, size_t>> _stats;
};

class Test : public TfWeakBase
{
public:
    Test(unsigned numStages, unsigned numMutations,
         unsigned branchingFactor, unsigned treeDepth)
        : _numStages(numStages)
        , _numMutations(numMutations)
        , _branchingFactor(branchingFactor)
        , _treeDepth(treeDepth)
    {}

    // Runs the test.
    void Run() {
        std::vector<UsdStageRefPtr> stages;
        stages.reserve(_numStages);
        std::vector<SdfPathVector> attrPaths(_numStages);

        std::cout << "Populating stages.\n";
        for (unsigned i = 0; i<_numStages; ++i) {
            TRACE_SCOPE("populate_stage_time");
            const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
            _PopulateLayer(layer, &attrPaths[i]);
            UsdStageRefPtr stage = UsdStage::Open(layer);
            TF_AXIOM(stage);

            TfNotice::Register(
                TfCreateWeakPtr(this),
                &Test::_DidObjectsChanged,
                UsdStageConstPtr(stage));

            stages.push_back(std::move(stage));
        }
        _memMetrics.RecordMetric("mem_stages_populated");

        std::vector<unsigned> indices(_numStages);
        std::iota(indices.begin(), indices.end(), 0);
        std::vector<std::shared_ptr<EsfUsdStageData>> stageData(_numStages);

        std::cout << "Registering stage data.\n";
        {
            TRACE_SCOPE("register_stage_data_time");
            WorkParallelForEach(
                indices.begin(), indices.end(),
                [&stages, &stageData]
                (const unsigned i)
            {
                stageData[i] = EsfUsdStageData::RegisterStage(stages[i]);
            });
        }
        _memMetrics.RecordMetric("mem_stage_data_populated");

        std::cout << "Mutating the scene.\n";
        {
            TRACE_SCOPE("mutate_scene_time");
            WorkParallelForEach(
                indices.begin(), indices.end(),
                [&stages, &attrPaths, numMutations=_numMutations]
                (const unsigned i)
            {
                _MutateScene(i, numMutations, stages[i], attrPaths[i]);
            });
        }
        _memMetrics.RecordMetric("mem_scene_mutated");

        std::cout << "Validate connections.\n";
        {
            TRACE_SCOPE("validate_connections_time");
            for (const auto &stage : stages) {
                _ValidateConnections(stage);
            }
        }

        std::cout << "Deactivate root prims.\n";
        {
            TRACE_SCOPE("deactivate_roots_time");
            WorkParallelForEach(
                stages.begin(), stages.end(),
                [](const UsdStageConstPtr &stage)
            {
                const UsdPrim pseudoRoot = stage->GetPseudoRoot();
                for (const UsdPrim &root : pseudoRoot.GetChildren()) {
                    root.SetActive(false);
                }
            });
        }
        _memMetrics.RecordMetric("mem_roots_deactivated");

        std::cout << "Free stage data.\n";
        {
            TRACE_SCOPE("free_stage_data_time");
            stageData.clear();
        }
        _memMetrics.RecordMetric("mem_freed_stage_data");

        std::cout << "Free stages.\n";
        {
            TRACE_SCOPE("free_stages_time");
            stages.clear();
        }

        _memMetrics.RecordMetric("mem_at_end");
        _memMetrics.WritePerfstats();

        std::cout << "Test complete.\n";
    }

private:
    void _PopulateLayer(const SdfLayerHandle &layer, SdfPathVector *attrPaths) {
        _AddPrimHierarchy(
            layer, "Root", _branchingFactor, _treeDepth, attrPaths);
    }

    void _DidObjectsChanged(
        const UsdNotice::ObjectsChanged &objectsChanged)
    {
        EsfUsdStageData &stageData =
            EsfUsdStageData::GetStageData(objectsChanged.GetStage());

        EsfUsdStageData::ChangedPathSet changedTargetPaths;

        for (const SdfPath &path : objectsChanged.GetResyncedPaths()) {
            stageData.UpdateForResync(path, &changedTargetPaths);
        }

        for (const SdfPath &path : objectsChanged.GetChangedInfoOnlyPaths()) {
            const TfTokenVector changedFields =
                objectsChanged.GetChangedFields(path);
            if (std::find(
                    changedFields.begin(), changedFields.end(),
                    SdfFieldKeys->ConnectionPaths) != changedFields.end()) {
                stageData.UpdateForChangedAttributeConnections(
                    path, &changedTargetPaths);
            }
        }
    }

private:
    unsigned _numStages = 0;
    unsigned _numMutations = 0;
    unsigned _branchingFactor = 0;
    unsigned _treeDepth = 0;
    _MemoryMetrics _memMetrics;
};

} // anonymous namespace

int
main(int argc, char **argv)
{
    unsigned numThreads = WorkGetConcurrencyLimit();
    unsigned numStages = 1;
    unsigned branchingFactor = 5;
    unsigned treeDepth = 5;
    unsigned numMutations = std::numeric_limits<unsigned>::max();
    bool measureMemory = false;
    bool outputAsSpy = false;
    bool outputAsTrace = false;

    CLI::App app(
        "Performance and threading test of code that builds and updates\n"
        "connection tables.\n");
    app.add_option(
        "--numThreads", numThreads,
        "The number of threads to use");
    app.add_option(
        "--numStages", numStages,
        "The number of stages to use");
    app.add_option(
        "--numMutations", numMutations,
        "The number of times the scene is modified");
    app.add_option(
        "-b,--branchingFactor", branchingFactor,
        "The branching factor of the test scene namespace");
    app.add_option(
        "-d,--treeDepth", treeDepth,
        "The tree depth of the test scene namespace");
    app.add_flag(
        "--memory", measureMemory,
        "Measure memory. Writes data to\n"
        "per-metric .mallocTag files.");
    app.add_flag(
        "--spy", outputAsSpy,
        "Write trace data to 'test.spy'");
    app.add_flag(
        "--trace", outputAsTrace,
        "Write trace data to 'test.trace'");

    CLI11_PARSE(app, argc, argv);

    if (measureMemory && (outputAsSpy || outputAsTrace)) {
        std::cerr << "--memory can't be used with --spy or --trace\n";
        exit(1);
    }

    // If numMutations isn't set, perform ten mutations for every prim in the
    // tree.
    if (numMutations == std::numeric_limits<unsigned>::max()) {
        numMutations = 10 *
            (std::pow(branchingFactor, treeDepth+1) - 1)/(branchingFactor - 1);
    }

    WorkSetConcurrencyLimit(numThreads);
    std::cout << "Running with " << numThreads << " threads.\n";
    {
        std::unique_ptr<_PerformanceTracker> tracker(
            outputAsSpy || outputAsTrace
            ? new _PerformanceTracker(outputAsSpy, outputAsTrace)
            : static_cast<_PerformanceTracker*>(nullptr));

        if (measureMemory) {
            std::string errorMessage;
            const bool initialized = TfMallocTag::Initialize(&errorMessage);
            TF_VERIFY(
                initialized,
                "Failed to initialize TfMallocTag: %s", errorMessage.c_str());
        }

        Test(numStages, numMutations, branchingFactor, treeDepth).Run();
    }
}
