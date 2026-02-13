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
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/pxrCLI11/CLI11.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/trace/aggregateNode.h"
#include "pxr/base/trace/collector.h"
#include "pxr/base/trace/reporter.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/work/threadLimits.h"
#include "pxr/exec/exec/computationBuilders.h"
#include "pxr/exec/exec/registerSchema.h"
#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/readIteratorRange.h"
#include "pxr/usd/pcp/layerStackIdentifier.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/stage.h"

#include <iostream>
#include <memory>
#include <numeric>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE;
using namespace pxr_CLI;

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    
    (CustomSchema)
    (compute)
    (rel)
);

static void
_ConfigureTestPlugin()
{
    const PlugPluginPtrVector testPlugins = PlugRegistry::GetInstance()
        .RegisterPlugins(TfAbsPath("resources"));
    TF_AXIOM(testPlugins.size() == 1);
    TF_AXIOM(testPlugins[0]->GetName() == "testExecUsdRecompilation_Perf");
}

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(TestExecUsdRecompilation_PerfCustomSchema)
{
    // `compute` evalutes the number of authored targets for `rel` plus the
    // results of `compute` on each object targeted by `rel`.
    self.PrimComputation(_tokens->compute)
        .Callback<int>(+[](const VdfContext &ctx) {
            VdfReadIteratorRange<int> range(ctx, _tokens->compute);
            return std::accumulate(range.begin(), range.end(), 1);
        })
        .Inputs(
            Relationship(_tokens->rel).TargetedObjects<int>(_tokens->compute));
}

static void
_AddPrimHierarchy(
    const SdfPrimSpecHandle &parentPrimSpec,
    const SdfRelationshipSpecHandle &parentRelSpec,
    const unsigned branchingFactor,
    const unsigned treeDepth)
{
    if (treeDepth == 0) {
        return;
    }

    // Create all the necessary tokens for child prim names. The vector is
    // static so these are only generated once.
    static std::vector<TfToken> childNames;
    for (unsigned index = childNames.size(); index < branchingFactor; ++index) {
        const std::string childName = TfStringPrintf("Prim%u", index);
        childNames.emplace_back(childName);
    }

    // Get a proxy object that is used to add targets to the parent's
    // relationship.
    SdfTargetsProxy parentRelTargets = parentRelSpec->GetTargetPathList();

    for (unsigned i = 0; i < branchingFactor; ++i) {
        // Define the child and it's relationship.
        const SdfPrimSpecHandle childPrimSpec = SdfPrimSpec::New(
            parentPrimSpec,
            childNames[i].GetString(),
            SdfSpecifierDef,
            "CustomSchema");
        TF_AXIOM(childPrimSpec);
        
        const SdfRelationshipSpecHandle childRelSpec = SdfRelationshipSpec::New(
            childPrimSpec, "rel", /* custom */ false);
        TF_AXIOM(childRelSpec);

        parentRelTargets.Add(
            parentPrimSpec->GetPath().AppendChild(childNames[i]));

        // Add the children's descendants.
        _AddPrimHierarchy(
            childPrimSpec, childRelSpec, branchingFactor, treeDepth - 1);
    }
}

static void
_AddPrimHierarchy(
    const SdfLayerHandle &layer,
    const char *rootPrimName,
    const unsigned branchingFactor,
    const unsigned treeDepth)
{
    // Define the prim at the root of the hierarchy.
    const SdfPrimSpecHandle rootPrimSpec =
        SdfPrimSpec::New(layer, rootPrimName, SdfSpecifierDef, "CustomSchema");
    TF_AXIOM(rootPrimSpec);
    
    // Define the root prim's relationship.
    const SdfRelationshipSpecHandle rootRelSpec =
        SdfRelationshipSpec::New(rootPrimSpec, "rel", /* custom */ false);
    TF_AXIOM(rootRelSpec);
    
    // Define the descendant prims.
    _AddPrimHierarchy(rootPrimSpec, rootRelSpec, branchingFactor, treeDepth);
}

class Test
{
public:
    Test(unsigned branchingFactor, unsigned treeDepth)
        : _branchingFactor(branchingFactor)
        , _treeDepth(treeDepth)
    {}

    virtual ~Test() = default;

    // Runs the test. Each stage of the test can be customized by overriding
    // the protected virtual methods.
    void Run() {
        std::cout << "Populating stage.\n";
        UsdStageRefPtr stage;
        {
            TRACE_SCOPE("populate_stage_time");
            const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
            _PopulateLayer(layer);
            stage = UsdStage::Open(layer);
            TF_AXIOM(stage);
        }

        std::cout << "Building request.\n";
        ExecUsdSystem system(stage);
        std::optional<ExecUsdRequest> request;
        {
            TRACE_SCOPE("build_request_time");
            request = _BuildRequest(system, *stage);
        }

        std::cout << "Preparing the request.\n";
        {
            TRACE_SCOPE("prepare_request_time");
            system.PrepareRequest(*request);
        }

        std::cout << "Computing values.\n";
        std::optional<ExecUsdCacheView> cacheView;
        {
            TRACE_SCOPE("evaluate_time");
            cacheView.emplace(system.Compute(*request));
        }

        std::vector<int> computedValues;
        computedValues.resize(_numValueKeys);
        {
            TRACE_SCOPE("extract_time");
            _ExtractValues(*cacheView, &computedValues);
        }
        _PrintComputedValues(computedValues);

        std::cout << "Modifying the scene.\n";
        {
            TRACE_SCOPE("scene_edit_time");
            _EditStage(&*stage);
        }

        std::cout << "Updating the request.\n";
        {
            TRACE_SCOPE("rebuild_request_time");
            _RebuildRequest(system, &*request, *stage);
        }

        std::cout << "Re-preparing the request.\n";
        {
            TRACE_SCOPE("reprepare_request_time");
            system.PrepareRequest(*request);
        }

        std::cout << "Recomputing the values.\n";
        {
            TRACE_SCOPE("reevaluate_time");
            cacheView.emplace(system.Compute(*request));
        }
        computedValues.resize(_numValueKeys);
        {
            TRACE_SCOPE("reextract_time");
            _ExtractValues(*cacheView, &computedValues);
        }
        _PrintComputedValues(computedValues);

        std::cout << "Test complete.\n";
    }

protected:
    virtual void _PopulateLayer(const SdfLayerHandle &layer) {
        _AddPrimHierarchy(layer, "Root", _branchingFactor, _treeDepth);
    }

    // Builds an initial request that compiles the initial exec network. This
    // method must set the protected member '_numValueKeys' accordingly.
    virtual ExecUsdRequest _BuildRequest(
        ExecUsdSystem &system, 
        UsdStage &stage) {

        const UsdPrim rootPrim = stage.GetPrimAtPath(SdfPath("/Root"));
        TF_AXIOM(rootPrim);

        _numValueKeys = 1;
        return system.BuildRequest({
            {rootPrim, _tokens->compute}
        });
    }

    // Modifies the scene to set up the recompilation scenario.
    virtual void _EditStage(UsdStage *const stage) {}

    // Optionally rebuilds the request. This must update the protected member
    // '_numValueKeys' if necessary.
    virtual void _RebuildRequest(
        ExecUsdSystem &system,
        ExecUsdRequest *const request,
        UsdStage &stage) {}

private:
    // Extracts '_numValueKeys' into the \p computedValues vector. The vector
    // is pre-sized to fit all extracted values.
    void _ExtractValues(
        const ExecUsdCacheView &cacheView,
        std::vector<int> *const computedValues) const {
        for (unsigned i = 0; i < _numValueKeys; ++i) {
            const int requestIndex = static_cast<int>(i);
            (*computedValues)[i] = cacheView.Get(requestIndex).Get<int>();
        }
    }

    // Prints '_numValueKeys' values from the \p computedValues vector.
    void _PrintComputedValues(const std::vector<int> &computedValues) const {
        for (unsigned i = 0; i < _numValueKeys; ++i) {
            std::cout
                << "Computed value " << i << " = " << computedValues[i] << '\n';
        }
    }

protected:
    unsigned _branchingFactor = 0;
    unsigned _treeDepth = 0;
    unsigned _numValueKeys = 0;
};

// Adds a target to a relationship, then removes that target.
class Test_TouchRel : public Test
{
public:
    Test_TouchRel(
        unsigned branchingFactor,
        unsigned treeDepth,
        const std::string &relPathStr)
        : Test(branchingFactor, treeDepth)
        , _relPath(relPathStr)
    {}

private:
    void _EditStage(UsdStage *stage) override {
        UsdRelationship rel = stage->GetRelationshipAtPath(_relPath);
        TF_AXIOM(rel.IsValid());
        rel.AddTarget(SdfPath("/Root"));
        rel.RemoveTarget(SdfPath("/Root"));
    }

private:
    SdfPath _relPath;
};

// Adds a new target to a relationship.
class Test_AddTarget : public Test
{
public:
    Test_AddTarget(
        unsigned branchingFactor,
        unsigned treeDepth,
        const std::string &relPathStr,
        const std::string &targetPathStr)
        : Test(branchingFactor, treeDepth)
        , _relPath(relPathStr)
        , _targetPath(targetPathStr)
    {}

private:
    void _EditStage(UsdStage *stage) override {
        UsdRelationship rel = stage->GetRelationshipAtPath(_relPath);
        TF_AXIOM(rel.IsValid());
        TF_AXIOM(stage->GetPrimAtPath(_targetPath).IsValid());
        rel.AddTarget(_targetPath);
    }

private:
    SdfPath _relPath;
    SdfPath _targetPath;
};

// Adds a relationship target into a previously-uncompiled hierarchy of prims.
class Test_AddTargetToNewHierarchy : public Test
{
public:
    Test_AddTargetToNewHierarchy(
        unsigned branchingFactor,
        unsigned treeDepth,
        const std::string &relPathStr)
        : Test(branchingFactor, treeDepth)
        , _relPath(relPathStr)
    {}

protected:
    void _PopulateLayer(const SdfLayerHandle &layer) override {
        Test::_PopulateLayer(layer);
        _AddPrimHierarchy(layer, "NewRoot", _branchingFactor, _treeDepth);
    }

    void _EditStage(UsdStage *stage) override {
        UsdRelationship rel = stage->GetRelationshipAtPath(_relPath);
        TF_AXIOM(rel);
        rel.AddTarget(SdfPath("/NewRoot"));
    }

private:
    SdfPath _relPath;
};

// Same as Test_AddTargetToNewHierarchy, except one of the leaf prims has a
// relationship with a target back into the original hierarchy.
//
class Test_AddTargetToConnectedHierarchy : public Test_AddTargetToNewHierarchy
{
public:
    Test_AddTargetToConnectedHierarchy(
        unsigned branchingFactor,
        unsigned treeDepth,
        const std::string &relPathStr,
        const std::string &targetPathStr)
        : Test_AddTargetToNewHierarchy(branchingFactor, treeDepth, relPathStr)
        , _targetPath(targetPathStr)
    {}

private:
    void _EditStage(UsdStage *stage) override {
        Test_AddTargetToNewHierarchy::_EditStage(stage);

        // Make path to a relationship on a leaf prim in the new hierarchy.
        const TfToken primName("Prim1");
        SdfPath leafRelPath("/NewRoot");
        for (unsigned i = 0; i < _treeDepth; ++i) {
            leafRelPath = leafRelPath.AppendChild(primName);
        }
        leafRelPath = leafRelPath.AppendProperty(_tokens->rel);

        // Add the target.
        UsdRelationship leafRel = stage->GetRelationshipAtPath(leafRelPath);
        TF_AXIOM(leafRel.IsValid());
        TF_AXIOM(stage->GetPrimAtPath(_targetPath).IsValid());
        leafRel.AddTarget(_targetPath);
    }

private:
    SdfPath _targetPath;
};

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

    // Gets a trace node descenant of \p rootNode.
    //
    // Arguments following \p rootNode describe a path of descendants from the
    // root node. Each descendant is a string for the node name.
    // 
    // If prefixed by a '*', then the descendant matches the first child that
    // ends with the node name. This is useful to select nodes added by
    // TRACE_FUNCTION_SCOPE, because they may or may not be prefixed by the
    // pxr namespace.
    //
    // If ending with '?', then the requested descendant is optional. If an
    // optional descendant is not found, this function returns nullptr. If a
    // non-optional descendant is not found, this reports a TF_FATAL_ERROR.
    //
    template <class... Tail>
    static TraceAggregateNodePtr _GetTraceNode(
        const TraceAggregateNodePtr &rootNode,
        const std::string &childNameExpr,
        Tail &&...tail) {

        TF_AXIOM(childNameExpr.size() > 0);
        const bool isSuffix = childNameExpr.front() == '*';
        const bool isOptional = childNameExpr.back() == '?';
        const std::string childName = TfStringTrim(childNameExpr, "*?");

        TraceAggregateNodePtr childNode;
        if (isSuffix) {
            // Get the child node by finding the first child with the given
            // suffix.
            for (const TraceAggregateNodePtr &child : rootNode->GetChildren()) {
                if (TfStringEndsWith(child->GetKey().GetString(), childName)) {
                    childNode = child;
                    break;
                }
            }
        }
        else {
            // Get the child node by exact name.
            childNode = rootNode->GetChild(childName);
        }

        // If not found, the child must be optional.
        if (!childNode && !isOptional) {
            TF_FATAL_ERROR(
                "Expected trace node '%s' not found", childNameExpr.c_str());
        }
        
        // If optional and not found, return null now. Otherwise, find the
        // remaining children.
        if (isOptional && !childNode) {
            return nullptr;
        }
        return _GetTraceNode(childNode, std::forward<Tail>(tail)...);
    }

    static TraceAggregateNodePtr _GetTraceNode(
        const TraceAggregateNodePtr &rootNode) {
        return rootNode;
    }

    // Writes a node time in seconds to the stats file.
    static void _WriteStat(
        const TraceAggregateNodePtr &node,
        const char *const profileName,
        std::ofstream *const statsFile = nullptr) {
        
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
        std::ofstream *const statsFile = nullptr) {
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

        _WriteStat(_GetTraceNode(mainThreadNode, "populate_stage_time"));
        _WriteStat(_GetTraceNode(mainThreadNode, "build_request_time"));
        _WriteStat(_GetTraceNode(mainThreadNode,
                "prepare_request_time",
                "*ExecUsdSystem::PrepareRequest",
                "*ExecUsd_RequestImpl::Compile"),
            "compile_time", &statsFile);
        _WriteStat(_GetTraceNode(mainThreadNode,
                "prepare_request_time",
                "*ExecUsdSystem::PrepareRequest",
                "*VdfScheduler::Schedule"),
            "schedule_time", &statsFile);
        _WriteStat(_GetTraceNode(mainThreadNode, "evaluate_time"));
        _WriteStat(_GetTraceNode(mainThreadNode, "extract_time"));
        _WriteStat(_GetTraceNode(mainThreadNode, "scene_edit_time"));
        _WriteStat(_GetTraceNode(mainThreadNode, "rebuild_request_time"));
        
        // If there was no scene edit, then there is no node for
        // ExecUsd_RequestImpl::Compile.
        _WriteStat(_GetTraceNode(mainThreadNode,
                "reprepare_request_time",
                "*ExecUsdSystem::PrepareRequest",
                "*ExecUsd_RequestImpl::Compile?"),
            "recompile_time", &statsFile);

        // Some scene edits might not require scheduling, in which case
        // there is no node for VdfScheduler::Schedule.
        _WriteStat(_GetTraceNode(mainThreadNode,
                "reprepare_request_time",
                "*ExecUsdSystem::PrepareRequest",
                "*VdfScheduler::Schedule?"),
            "reschedule_time", &statsFile);

        _WriteStat(_GetTraceNode(mainThreadNode, "reevaluate_time"));
        _WriteStat(_GetTraceNode(mainThreadNode, "reextract_time"));
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

int
main(int argc, char **argv)
{
    unsigned numThreads = WorkGetConcurrencyLimit();
    unsigned branchingFactor = 5;
    unsigned treeDepth = 5;
    std::string relPath;
    std::string targetPath;
    std::string testName;
    bool outputAsSpy = false;
    bool outputAsTrace = false;

    CLI::App app(
        "Performance test that measures OpenExec recompilation time.\n"
        "\n"
        "This performance test creates a hierarchy of prims of depth "
        "--treeDepth and branching factor --branchingFactor. Each prim "
        "contains a single relationship that targets the prim's children.\n"
        "\n"
        "Each prim defines a computation that depends on the targets of its "
        "relationship. The test first creates a request that evaluates the "
        "computation on the root of the hierarchy, thus compiling a callback "
        "node for every prim in the hierarchy.\n"
        "\n"
        "Then, the test edits the scene according to the 'testName' parameter "
        "and recompiles the exec graph.\n",
        "TestExecUsdRecompilation_Perf");
    app.add_option(
        "testName", testName,
        "Choose one of:\n"
        "-  Default\n"
        "-  TouchRel (Requires --rel)\n"
        "-  AddTarget (Requires --rel, --target)\n"
        "-  AddTargetToNewHierarchy (Requires --rel)\n"
        "-  AddTargetToConnectedHierarchy (Requires --rel, --target):\n")
        ->required();
    app.add_option(
        "-j,--numThreads", numThreads,
        "The number of threads to use");
    app.add_option(
        "-b,--branchingFactor", branchingFactor,
        "The branching factor of the initial scene graph");
    app.add_option(
        "-d,--treeDepth", treeDepth,
        "The tree depth of the initial scene graph");
    app.add_option(
        "-r,--rel", relPath,
        "Path to relationship that will be edited");
    app.add_option(
        "-t,--target", targetPath,
        "Path to the object targeted by the relationship");
    app.add_flag(
        "--spy", outputAsSpy,
        "Write trace data to test.spy");
    app.add_flag(
        "--trace", outputAsTrace,
        "Write trace data to test.trace");

    CLI11_PARSE(app, argc, argv);

    std::unique_ptr<Test> test;
    if (testName == "Default") {
        test = std::make_unique<Test>(branchingFactor, treeDepth);
    }
    else if (testName == "TouchRel") {
        test = std::make_unique<Test_TouchRel>(
            branchingFactor, treeDepth, relPath);
    }
    else if (testName == "AddTarget") {
        test = std::make_unique<Test_AddTarget>(
            branchingFactor, treeDepth, relPath, targetPath);
    }
    else if (testName == "AddTargetToNewHierarchy") {
        test = std::make_unique<Test_AddTargetToNewHierarchy>(
            branchingFactor, treeDepth, relPath);
    }
    else if (testName == "AddTargetToConnectedHierarchy") {
        test = std::make_unique<Test_AddTargetToConnectedHierarchy>(
            branchingFactor, treeDepth, relPath, targetPath);
    }
    else {
        std::cout << "Invalid test name. See usage.\n";
        return 1;
    }

    _ConfigureTestPlugin();
    std::cout << "Running with " << numThreads << " threads.\n";
    {
        _PerformanceTracker performanceTracker(outputAsSpy, outputAsTrace);
        test->Run();
    }
}
