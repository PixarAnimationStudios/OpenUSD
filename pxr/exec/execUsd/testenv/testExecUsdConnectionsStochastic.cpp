//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/execUsd/cacheView.h"
#include "pxr/exec/execUsd/request.h"
#include "pxr/exec/execUsd/system.h"
#include "pxr/exec/execUsd/valueKey.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/pxrCLI11/CLI11.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/threadLimits.h"
#include "pxr/exec/exec/registerSchema.h"
#include "pxr/exec/exec/typeRegistry.h"
#include "pxr/exec/vdf/readIteratorRange.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usd/attribute.h"
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

TF_REGISTRY_FUNCTION(ExecTypeRegistry)
{
    ExecTypeRegistry::RegisterType(SdfPathVector{});
    ExecTypeRegistry::RegisterType(std::set<SdfPath>{});
}

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (attr)
    (computeConnections)
    (computeIncomingConnections)
);

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(
    TestExecUsdConnectionsStochasticCustomSchema)
{
    // Attribute computation that returns the attribute's outgoing connections,
    // as an SdfPathVector.
    self.AttributeComputation(
        _tokens->attr,
        _tokens->computeConnections)
        .Callback(+[](const VdfContext &ctx) {
            VdfReadIteratorRange<SdfPath> range(
                ctx, ExecBuiltinComputations->computePath);
            return SdfPathVector(range.begin(), range.end());
        })
        .Inputs(
            Connections<SdfPath>(
                ExecBuiltinComputations->computePath)
        );

    // Attribute computation that returns the incoming connections that target
    // the attribute, as a std::set<SdfPath>.
    self.AttributeComputation(
        _tokens->attr,
        _tokens->computeIncomingConnections)
        .Callback(+[](const VdfContext &ctx) {
            VdfReadIteratorRange<SdfPath> range(
                ctx, ExecBuiltinComputations->computePath);
            return std::set<SdfPath>(range.begin(), range.end());
        })
        .Inputs(
            IncomingConnections<SdfPath>(
                ExecBuiltinComputations->computePath)
        );
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

    for (unsigned i = 0; i < branchingFactor; ++i) {
        const SdfPrimSpecHandle childPrimSpec = SdfPrimSpec::New(
            parentPrimSpec,
            childNames[i].GetString(),
            SdfSpecifierDef,
            "CustomSchema");
        TF_AXIOM(childPrimSpec);

        attrPaths->push_back(
            childPrimSpec->GetPath().AppendProperty(_tokens->attr));
            
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

    ExecUsdSystem execSystem(stage);

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

            // Filter target paths: Computation providers must be loaded,
            // active, defined, and non-abstract.
            for (auto rit=targetPaths.rbegin(); rit!=targetPaths.rend();) {
                const auto it = std::prev(rit.base());
                if (!UsdPrimDefaultPredicate(
                        stage->GetPrimAtPath(it->GetPrimPath()))) {
                    rit = std::make_reverse_iterator(targetPaths.erase(it));
                } else {
                    ++rit;
                }
            }

            const ExecUsdRequest request = execSystem.BuildRequest({
                {attr, _tokens->computeConnections}});
            const ExecUsdCacheView view = execSystem.Compute(request);
            const VtValue v = view.Get(0);

            ASSERT_EQ(v.Get<SdfPathVector>(), targetPaths);

            const SdfPath ownerPath = attr.GetPath();
            for (const SdfPath &targetPath : targetPaths) {
                actualIncoming[targetPath].insert(ownerPath);
            }
        }
    }

    for (const auto &entry : actualIncoming) {
        const SdfPath &targetPath = entry.first;
        const std::set<SdfPath> &actual = entry.second;

        const UsdAttribute attr = stage->GetAttributeAtPath(targetPath);

        const ExecUsdRequest request = execSystem.BuildRequest({
            {attr, _tokens->computeIncomingConnections}});
        const ExecUsdCacheView view = execSystem.Compute(request);
        const VtValue v = view.Get(0);

        ASSERT_EQ(v.Get<std::set<SdfPath>>(), actual);
    }
}

static void
_MutateScene(
    const unsigned numIterations,
    const UsdStageRefPtr &stage,
    const SdfPathVector &attrPaths)
{
    std::mt19937 rng(0);
    std::uniform_int_distribution<size_t> randomPath(0, attrPaths.size()-1);
    std::uniform_int_distribution<size_t> randomOp(0, 2);

    for (size_t j=0; j<numIterations; ++j) {
        const size_t op = randomOp(rng);

        if (op == 0) {
            // If we find an attribute at the owner path, add a connection.
            const size_t ownerI = randomPath(rng);
            const size_t targetI = randomPath(rng);

            if (UsdAttribute owner =
                    stage->GetAttributeAtPath(attrPaths[ownerI])) {
                owner.AddConnection(attrPaths[targetI]);
            }
        } else if (op == 1) {
            // If we find an attribute at the owner path and it has connections,
            // remove a random connection.
            const size_t ownerI = randomPath(rng);

            if (UsdAttribute owner =
                    stage->GetAttributeAtPath(attrPaths[ownerI])) {
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
            // If we find a prim at the path, toggle its activation state.
            const size_t primI = randomPath(rng);

            if (UsdPrim prim =
                    stage->GetPrimAtPath(attrPaths[primI].GetPrimPath())) {
                prim.SetActive(!prim.IsActive());
            }
        } else {
            TF_FATAL_ERROR("Unexpected op %zu", op);
        }
    }
}

namespace {

class Test
{
public:
    Test(const unsigned numMutations,
         const unsigned branchingFactor,
         const unsigned treeDepth)
        : _numMutations(numMutations)
        , _branchingFactor(branchingFactor)
        , _treeDepth(treeDepth)
    {}

    // Runs the test.
    void Run() {
        UsdStageRefPtr stage;
        SdfPathVector attrPaths;

        const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
        _PopulateLayer(layer, &attrPaths);
        stage = UsdStage::Open(layer);
        TF_AXIOM(stage);

        _MutateScene(_numMutations, stage, attrPaths);

        _ValidateConnections(stage);
    }

private:
    void _PopulateLayer(const SdfLayerHandle &layer, SdfPathVector *attrPaths) {
        _AddPrimHierarchy(
            layer, "Root", _branchingFactor, _treeDepth, attrPaths);
    }

private:
    unsigned _numMutations = 0;
    unsigned _branchingFactor = 0;
    unsigned _treeDepth = 0;
};

} // anonymous namespace

int
main(int argc, char **argv)
{
    unsigned numThreads = WorkGetConcurrencyLimit();
    unsigned branchingFactor = 5;
    unsigned treeDepth = 5;
    unsigned numMutations = std::numeric_limits<unsigned>::max();

    CLI::App app(
        "Performance and threading test of code that builds and updates\n"
        "connection tables.\n");
    app.add_option(
        "--numThreads", numThreads,
        "The number of threads to use");
    app.add_option(
        "--numMutations", numMutations,
        "The number of times the scene is modified");
    app.add_option(
        "-b,--branchingFactor", branchingFactor,
        "The branching factor of the initial scene graph");
    app.add_option(
        "-d,--treeDepth", treeDepth,
        "The tree depth of the test scene namespace");

    CLI11_PARSE(app, argc, argv);

    // If numMutations isn't set, perform ten mutations for every prim in the
    // tree.
    if (numMutations == std::numeric_limits<unsigned>::max()) {
        numMutations = 10 *
            (std::pow(branchingFactor, treeDepth+1) - 1)/(branchingFactor - 1);
    }

    // Load test custom schemas.
    const PlugPluginPtrVector testPlugins =
        PlugRegistry::GetInstance().RegisterPlugins(TfAbsPath("resources"));
    ASSERT_EQ(testPlugins.size(), 1);
    ASSERT_EQ(testPlugins[0]->GetName(), "testExecUsdConnectionsStochastic");

    WorkSetConcurrencyLimit(numThreads);
    std::cout << "Running with " << numThreads << " threads.\n";
    {
        Test(numMutations, branchingFactor, treeDepth).Run();
    }
}
