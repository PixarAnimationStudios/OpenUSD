//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/execUsd/cacheView.h"
#include "pxr/exec/execUsd/system.h"
#include "pxr/exec/execUsd/request.h"
#include "pxr/exec/execUsd/valueKey.h"

#include "pxr/base/plug/notice.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/tf/callContext.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/exec/ef/timeInterval.h"
#include "pxr/exec/exec/computationBuilders.h"
#include "pxr/exec/exec/registerSchema.h"
#include "pxr/exec/exec/validationError.h"
#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/readIteratorRange.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/stage.h"

#include <algorithm>
#include <numeric>
#include <sstream>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE;

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    
    (cyclicComputation)
    (cyclicComputationPairA)
    (cyclicComputationPairB)
    (customRel)
    (cyclicRelComputation)
    (cyclicAncestorComputation)
    (ancestor)
);

static void
ConfigureTestPlugin()
{
    const PlugPluginPtrVector testPlugins = PlugRegistry::GetInstance()
        .RegisterPlugins(TfAbsPath("resources"));

    TF_AXIOM(testPlugins.size() == 1);
    TF_AXIOM(testPlugins[0]->GetName() == "testExecUsdCycleDetection");
}

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(TestExecUsdCycleDetectionCustomSchema)
{
    // This computation consumes itself as input.
    self.PrimComputation(_tokens->cyclicComputation)
        .Callback<int>(+[](const VdfContext &ctx) {
            return ctx.GetInputValue<int>(_tokens->cyclicComputation);
        })
        .Inputs(Computation<int>(_tokens->cyclicComputation));

    // The following 2 computations consume each other as input.
    self.PrimComputation(_tokens->cyclicComputationPairA)
        .Callback<int>(+[](const VdfContext &ctx) {
            return ctx.GetInputValue<int>(_tokens->cyclicComputationPairB);
        })
        .Inputs(Computation<int>(_tokens->cyclicComputationPairB));

    self.PrimComputation(_tokens->cyclicComputationPairB)
        .Callback<int>(+[](const VdfContext &ctx) {
            return ctx.GetInputValue<int>(_tokens->cyclicComputationPairA);
        })
        .Inputs(Computation<int>(_tokens->cyclicComputationPairA));

    // This computation sources its input by invoking the same computation on
    // the relationship targeted objects. If there is a cycle of relationship
    // targets, then compiling this computation will result in a cycle.
    self.PrimComputation(_tokens->cyclicRelComputation)
        .Callback<int>(+[](const VdfContext &ctx) {
            const VdfReadIteratorRange<int> range(
                ctx, _tokens->cyclicRelComputation);
            return std::accumulate(range.begin(), range.end(), 1);
        })
        .Inputs(
            Relationship(_tokens->customRel)
                .TargetedObjects<int>(_tokens->cyclicRelComputation));

    // This computation sources inputs by invoking the same computation on its
    // namespace ancestor, and the target of a relationship. If that
    // relationship targets a descendant object, there will be a cycle.
    self.PrimComputation(_tokens->cyclicAncestorComputation)
        .Callback<int>(+[](const VdfContext &ctx) {
            const int fallback = 0;
            const int sum =
                *ctx.GetInputValuePtr<int>(_tokens->ancestor, &fallback) +
                *ctx.GetInputValuePtr<int>(_tokens->customRel, &fallback);
            return sum;
        })
        .Inputs(
            NamespaceAncestor<int>(_tokens->cyclicAncestorComputation)
                .InputName(_tokens->ancestor),
            Relationship(_tokens->customRel)
                .TargetedObjects<int>(_tokens->cyclicAncestorComputation)
                .InputName(_tokens->customRel));
}

class Fixture
{
public:
    ExecUsdSystem &NewSystemFromLayer(const char *const layerContents) {
        TF_AXIOM(!_system);

        const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
        layer->ImportFromString(layerContents);
        TF_AXIOM(layer);

        _stage = UsdStage::Open(layer);
        TF_AXIOM(_stage);
        _system.emplace(_stage);

        return *_system;
    }

    ExecUsdRequest BuildRequest(
        std::vector<ExecUsdValueKey> &&valueKeys) {
        return _system->BuildRequest(
            std::move(valueKeys),
            [this](
                const ExecRequestIndexSet &invalidIndices,
                const EfTimeInterval &invalidInterval) {
                invalidRequestIndices.insert(
                    invalidIndices.begin(), invalidIndices.end());
                invalidTimeInterval |= invalidInterval;
            });
    }

    UsdRelationship GetRelationshipAtPath(const char *const pathStr) const {
        return _stage->GetRelationshipAtPath(SdfPath(pathStr));
    }

    UsdPrim GetPrimAtPath(const char *const pathStr) const {
        return _stage->GetPrimAtPath(SdfPath(pathStr));
    }

    UsdAttribute GetAttributeAtPath(const char *const pathStr) const {
        return _stage->GetAttributeAtPath(SdfPath(pathStr));
    }

    void AddRelationshipTarget(
        const char *const relPathStr,
        const char *const targetPathStr) const {
        UsdRelationship rel =
            _stage->GetRelationshipAtPath(SdfPath(relPathStr));
        TF_AXIOM(rel);
        rel.AddTarget(SdfPath(targetPathStr));
    }

public:
    ExecRequestIndexSet invalidRequestIndices;
    EfTimeInterval invalidTimeInterval;

private:
    UsdStageRefPtr _stage;
    std::optional<ExecUsdSystem> _system;
};

// An RAII helper that checks for expected validation errors.
class ExpectedValidationErrors
{
public:
    ExpectedValidationErrors(
        const TfCallContext &callContext,
        std::vector<ExecValidationErrorType> expectedErrorTypes)
        : _callContext(callContext)
        , _expectedErrorTypes(std::move(expectedErrorTypes))
    {}

    ~ExpectedValidationErrors() {
        std::string foundExpectedErrors;
        std::string missingExpectedErrors;
        std::string unexpectedErrors;

        for (const TfError &error : _errorMark) {
            const auto it = std::find(
                _expectedErrorTypes.begin(),
                _expectedErrorTypes.end(),
                error.GetErrorCode());
            if (it != _expectedErrorTypes.end()) {
                // This error was expected. Strike it from the list of expected
                // errors.
                _expectedErrorTypes.erase(it);
                foundExpectedErrors += "- " + _GetErrorString(error) + '\n';
                continue;
            }

            // The error is not expected.
            unexpectedErrors += "- " + _GetErrorString(error) + '\n';
        }

        // These expected errors were never found.
        for (const auto &expectedErrorType : _expectedErrorTypes) {
            missingExpectedErrors +=
                "- " + TfEnum::GetDisplayName(expectedErrorType) + '\n';
        }
        _errorMark.Clear();

        // Build a final error message.
        std::string message = TfStringPrintf(
            "In %s at %s:%zu:\n",
            _callContext.GetPrettyFunction(),
            _callContext.GetFile(),
            _callContext.GetLine());
        if (!foundExpectedErrors.empty()) {
            message += "The following expected errors were found:\n";
            message += foundExpectedErrors;
        }
        if (!missingExpectedErrors.empty()) {
            message += "The following expected errors were not found:\n";
            message += missingExpectedErrors;
        }
        if (!unexpectedErrors.empty()) {
            message += "The following errors were unexpected:\n";
            message += unexpectedErrors;
        }

        // We require that all expected errors were found, and that there were
        // no unexpected errors.
        if (!missingExpectedErrors.empty() || !unexpectedErrors.empty()) {
            TF_FATAL_ERROR(message);
        }
    }

private:
    static std::string _GetErrorString(const TfError &error) {
        return TfStringPrintf("[%s] %s (in function '%s' at %s:%zu)",
            error.GetErrorCodeAsString().c_str(),
            error.GetCommentary().c_str(),
            error.GetSourceFunction().c_str(),
            error.GetSourceFileName().c_str(),
            error.GetSourceLineNumber());
    }

    TfErrorMark _errorMark;
    TfCallContext _callContext;
    std::vector<ExecValidationErrorType> _expectedErrorTypes;
};

#define EXPECT_VALIDATION_ERRORS(...)                                          \
    const ExpectedValidationErrors _expectedErrors(                            \
        TF_CALL_CONTEXT, {__VA_ARGS__});

static void
Test_CycleDetectionRequiresRecompilation()
{
    Fixture fixture;
    ExecUsdSystem &system = fixture.NewSystemFromLayer(R"usd(#usda 1.0
        def CustomSchema "Prim" {
        }
    )usd");

    ExecUsdRequest request = fixture.BuildRequest({
        {fixture.GetPrimAtPath("/Prim"), _tokens->cyclicComputation}
    });

    {
        // Compiling the request should detect a cycle.
        EXPECT_VALIDATION_ERRORS(ExecValidationErrorType::DataDependencyCycle);
        system.PrepareRequest(request);
    }
    {
        // Even though we just compiled the request, the network requires
        // recompilation since the previous round was interrupted. This should
        // find a cycle again.
        EXPECT_VALIDATION_ERRORS(ExecValidationErrorType::DataDependencyCycle);
        system.PrepareRequest(request);
    }
    {
        // Computing the request will compile the request again, since the
        // previous round was interrupted due to a cycle.
        EXPECT_VALIDATION_ERRORS(ExecValidationErrorType::DataDependencyCycle);
        const ExecUsdCacheView cacheView = system.Compute(request);
        TF_AXIOM(cacheView.Get(0).IsEmpty());
    }
}

// Test that we detect a cycle when a computation sources itself as an input.
static void
Test_CyclicComputation()
{
    Fixture fixture;
    ExecUsdSystem &system = fixture.NewSystemFromLayer(R"usd(#usda 1.0
        def CustomSchema "Prim" {
        }
    )usd");

    ExecUsdRequest request = fixture.BuildRequest({
        {fixture.GetPrimAtPath("/Prim"), _tokens->cyclicComputation}
    });

    // Compiling the request should detect a cycle.
    EXPECT_VALIDATION_ERRORS(ExecValidationErrorType::DataDependencyCycle);

    // Should extract an empty value because the leaf node was not compiled.
    const ExecUsdCacheView cacheView = system.Compute(request);
    TF_AXIOM(cacheView.Get(0).IsEmpty());
}

// Test that we detect a cycle when a pair of computations source eachother as
// inputs.
//
static void
Test_CyclicComputationPair()
{
    Fixture fixture;
    ExecUsdSystem &system = fixture.NewSystemFromLayer(R"usd(#usda 1.0
        def CustomSchema "Prim" {
        }
    )usd");

    ExecUsdRequest request = fixture.BuildRequest({
        {fixture.GetPrimAtPath("/Prim"), _tokens->cyclicComputationPairA}
    });

    // Compiling the request should detect a cycle.
    EXPECT_VALIDATION_ERRORS(ExecValidationErrorType::DataDependencyCycle);

    // Should extract an empty value because the leaf node was not compiled.
    const ExecUsdCacheView cacheView = system.Compute(request);
    TF_AXIOM(cacheView.Get(0).IsEmpty());
}

// Test that we detect a cycle when a computation sources its input from a
// relationship target, which is part of a cycle of relationship targets.
//
static void
Test_CyclicRelationshipComputation()
{
    Fixture fixture;
    ExecUsdSystem &system = fixture.NewSystemFromLayer(R"usd(#usda 1.0
        def CustomSchema "Prim1" {
            add rel customRel = [</Prim2>]
        }
        def CustomSchema "Prim2" {
            add rel customRel = [</Prim3>]
        }
        def CustomSchema "Prim3" {
            add rel customRel = [</Prim1>]
        }
    )usd");

    ExecUsdRequest request = fixture.BuildRequest({
        {fixture.GetPrimAtPath("/Prim2"), _tokens->cyclicRelComputation}
    });

    // Compiling the request should detect a cycle.
    EXPECT_VALIDATION_ERRORS(ExecValidationErrorType::DataDependencyCycle);

    // Should extract an empty value because the leaf node was not compiled.
    const ExecUsdCacheView cacheView = system.Compute(request);
    TF_AXIOM(cacheView.Get(0).IsEmpty());
}

// Test that we detect a cycle when a computation sources its input from a
// relationship target, which is part of a cycle of relationship targets. This
// specifically tests the case when the relationship cycle is large, and also
// tests the case where multiple leaf tasks depend on the same cycle.
//
static void
Test_LargeCyclicRelationshipComputation()
{
    const int SIZE = 500;
    std::ostringstream layer;
    layer << "#usda 1.0\n";

    // Prim[i].customRel targets Prim[i+1].
    for (int i = 0; i < SIZE; ++i) {
        layer << "def CustomSchema \"Prim" << i << "\" {\n"
              << "    add rel customRel = [</Prim" << (i + 1) << ">]\n"
              << "}\n";
    }

    // Prim[SIZE].customRel targets Prim[0].
    layer << "def CustomSchema \"Prim" << SIZE << "\" {\n"
          << "    add rel customRel = [</Prim0>]\n"
          << "}\n";

    Fixture fixture;
    ExecUsdSystem &system = fixture.NewSystemFromLayer(layer.str().c_str());

    ExecUsdRequest request = fixture.BuildRequest({
        {fixture.GetPrimAtPath("/Prim0"), _tokens->cyclicRelComputation},
        {fixture.GetPrimAtPath("/Prim100"), _tokens->cyclicRelComputation},
        {fixture.GetPrimAtPath("/Prim200"), _tokens->cyclicRelComputation},
        {fixture.GetPrimAtPath("/Prim300"), _tokens->cyclicRelComputation},
        {fixture.GetPrimAtPath("/Prim400"), _tokens->cyclicRelComputation},
    });

    // Compiling the request should detect a cycle.
    EXPECT_VALIDATION_ERRORS(ExecValidationErrorType::DataDependencyCycle);
    
    // Unable to compile the leaf node. Should extract a empty VtValues.
    const ExecUsdCacheView cacheView = system.Compute(request);
    for (int i = 0; i < 5; ++i) {
        TF_AXIOM(cacheView.Get(i).IsEmpty());
    }
}

// Test that we detect a cycle when a computation sources its input from its
// ancestor, but one of those ancestors sources its input from a descendant.
//
static void
Test_CyclicAncestorComputation()
{
    Fixture fixture;
    ExecUsdSystem &system = fixture.NewSystemFromLayer(R"usd(#usda 1.0
        def CustomSchema "A" {
            add rel customRel = [</A/B/C>]

            def CustomSchema "B" {

                def CustomSchema "C" {
                }
            }
        }
    )usd");

    ExecUsdRequest request = fixture.BuildRequest({
        {fixture.GetPrimAtPath("/A"), _tokens->cyclicAncestorComputation}
    });

    // Compiling the request should detect a cycle.
    EXPECT_VALIDATION_ERRORS(ExecValidationErrorType::DataDependencyCycle);

    // Should extract an empty value because the leaf node was not compiled.
    const ExecUsdCacheView cacheView = system.Compute(request);
    TF_AXIOM(cacheView.Get(0).IsEmpty());
}

// Test that we detect a cycle when a previously cycle-free scene introduces
// a cycle by authoring a new relationship target.
//
static void
Test_CyclicRelationshipComputationAfterRecompile()
{
    Fixture fixture;
    ExecUsdSystem &system = fixture.NewSystemFromLayer(R"usd(#usda 1.0
        def CustomSchema "A" {
            add rel customRel = [</B>]
        }
        def CustomSchema "B" {
            add rel customRel = [</C>]
        }
        def CustomSchema "C" {
            rel customRel
        }
    )usd");

    ExecUsdRequest request = fixture.BuildRequest({
        {fixture.GetPrimAtPath("/A"), _tokens->cyclicRelComputation}
    });

    {
        // There are no cycles in the network. This should compile successfully.
        EXPECT_VALIDATION_ERRORS();
        system.PrepareRequest(request);
    }

    // Compute values. The result is well-defined.
    const ExecUsdCacheView cacheView = system.Compute(request);
    TF_AXIOM(cacheView.Get(0).IsHolding<int>());
    TF_AXIOM(cacheView.Get(0).Get<int>() == 3);

    // Make a change such that </C> [cyclicRelComputation] now depends on
    // </A> [cyclicRelComputaiton]. This would introduce a cycle.
    fixture.AddRelationshipTarget("/C.customRel", "/A");

    {
        EXPECT_VALIDATION_ERRORS(ExecValidationErrorType::DataDependencyCycle);

        // The new VdfConnection from </A> [cyclicRelComputation] to
        // </C> [cyclicRelComputation] will not be created because it introduces
        // a cycle. The network will not be modified, so the computed value
        // should not change.
        const ExecUsdCacheView cacheView = system.Compute(request);
        TF_AXIOM(cacheView.Get(0).IsHolding<int>());
        TF_AXIOM(cacheView.Get(0).Get<int>() == 3);
    }
}

// Test that we detect a cycle when a previously cycle-free scene recompiles
// two separate inputs that end up depending on each other, creating a
// "figure 8" cycle.
//
static void
Test_CyclicRelationshipComputationFigure8()
{
    Fixture fixture;
    ExecUsdSystem &system = fixture.NewSystemFromLayer(R"usd(#usda 1.0
        def CustomSchema "A1" {
            add rel customRel = [</A2>]
        }
        def CustomSchema "A2" {
            rel customRel
        }
        def CustomSchema "B1" {
            add rel customRel = [</B2>]
        }
        def CustomSchema "B2" {
            rel customRel
        }
    )usd");

    ExecUsdRequest request = fixture.BuildRequest({
        {fixture.GetPrimAtPath("/A1"), _tokens->cyclicRelComputation},
        {fixture.GetPrimAtPath("/B1"), _tokens->cyclicRelComputation}
    });

    // This should compile the following cycle-free network:
    //
    //      [ A2 cyclicRelComputation ]         [ B2 cyclicRelComputation ]
    //                   V                                  V              
    //      [ A1 cyclicRelComputaiton ]         [ B1 cyclicRelComputation ]
    //                   V                                  V              
    //             [ Leaf Node ]                      [ Leaf Node ]        
    //
    {
        EXPECT_VALIDATION_ERRORS();
        system.PrepareRequest(request);
    }

    // The computed values are well-defined.
    const ExecUsdCacheView cacheView = system.Compute(request);
    TF_AXIOM(cacheView.Get(0).IsHolding<int>());
    TF_AXIOM(cacheView.Get(1).IsHolding<int>());
    TF_AXIOM(cacheView.Get(0).Get<int>() == 2);
    TF_AXIOM(cacheView.Get(1).Get<int>() == 2);

    // Make scene changes that would introduce a cycle in a "figure 8" pattern:
    //
    //                   +----------------+  +---------------+               
    //                   V                |   |              V              
    //      [ A2 cyclicRelComputation ]   |   |  [ B2 cyclicRelComputation ]
    //                   V                |   |              V              
    //      [ A1 cyclicRelComputaiton ]   |   |  [ B1 cyclicRelComputation ]
    //                   V         |      |   |     |        V              
    //             [ Leaf Node ]   +----- | --+     |  [ Leaf Node ]        
    //                                    +---------+                       
    //
    fixture.AddRelationshipTarget("/A2.customRel", "/B1");
    fixture.AddRelationshipTarget("/B2.customRel", "/A1");

    {
        EXPECT_VALIDATION_ERRORS(ExecValidationErrorType::DataDependencyCycle);

        // The new connections will not be made because together they would
        // introduce a cycle. As a result, the computed values are the same.
        const ExecUsdCacheView cacheView = system.Compute(request);
        TF_AXIOM(cacheView.Get(0).IsHolding<int>());
        TF_AXIOM(cacheView.Get(1).IsHolding<int>());
        TF_AXIOM(cacheView.Get(0).Get<int>() == 2);
        TF_AXIOM(cacheView.Get(1).Get<int>() == 2);
    }
}

// Test that we send value key invalidation for indicies that depend on cycles.
// When compilation is interrupted due to cycle detection, we cannot tell which
// scene changes would break those cycles. We must conservatively invalidate
// those value keys on every scene change.
// 
static void
Test_CycleDetectionRequestInvalidation()
{
    Fixture fixture;
    ExecUsdSystem &system = fixture.NewSystemFromLayer(R"usd(#usda 1.0
        def CustomSchema "A" {
            add rel customRel = </B>
        }
        def CustomSchema "B" {
            rel customRel
        }
        def "Other" {
            int customAttr = 42
            rel customRel
            def "Target1" {}
            def "Target2" {}
            def "Target3" {}
        }
    )usd");

    ExecUsdRequest request = fixture.BuildRequest({
        // [Key 0] This value key always depends on a cycle.
        {fixture.GetPrimAtPath("/A"), _tokens->cyclicComputation},

        // [Key 1] This value key initially does not depend on a cycle. Then,
        // adding a relationship target from /B.customRel -> /A creates a cycle.
        {fixture.GetPrimAtPath("/A"), _tokens->cyclicRelComputation},

        // [Key 2] The value key will never depend on a cycle.
        ExecUsdValueKey{fixture.GetAttributeAtPath("/Other.customAttr")}
    });

    {
        // On first compute, we find a cycle due to Key 0. The leaf node for key
        // 0 could not be compiled. Keys 1 and 2 are well-defined.
        EXPECT_VALIDATION_ERRORS(ExecValidationErrorType::DataDependencyCycle);
        const ExecUsdCacheView cacheView = system.Compute(request);
        TF_AXIOM(cacheView.Get(0).IsEmpty());
        TF_AXIOM(cacheView.Get(1).Get<int>() == 2);
        TF_AXIOM(cacheView.Get(2).Get<int>() == 42);
    }

    // Keys that depend on a cycle should be invalidated for all scene changes,
    // because exec cannot know if the network objects that *would have* been
    // compiled actually depend on a given scene change.
    fixture.invalidRequestIndices.clear();
    fixture.AddRelationshipTarget("/Other.customRel", "/Other/Target1");
    TF_AXIOM(fixture.invalidRequestIndices.size() == 1);
    TF_AXIOM(fixture.invalidRequestIndices.contains(0));

    // Once invalidated, the indices are not notified again (until the next call
    // to Compute).
    fixture.invalidRequestIndices.clear();
    fixture.AddRelationshipTarget("/Other.customRel", "/Other/Target2");
    TF_AXIOM(fixture.invalidRequestIndices.empty());

    // Make another scene change that causes key 1 to depend on a cycle. This
    // should invalidate key 1.
    fixture.invalidRequestIndices.clear();
    fixture.AddRelationshipTarget("/B.customRel", "/A");
    TF_AXIOM(fixture.invalidRequestIndices.size() == 1);
    TF_AXIOM(fixture.invalidRequestIndices.contains(1));

    {
        // Recomputing the request should again find cycles. The leaf node for
        // key 0 still cannot be created.
        EXPECT_VALIDATION_ERRORS(ExecValidationErrorType::DataDependencyCycle);
        const ExecUsdCacheView cacheView = system.Compute(request);

        // Key 0 still doesn't have a leaf node. We extract an empty VtValue.
        TF_AXIOM(cacheView.Get(0).IsEmpty());

        // Key 1 did have a leaf node, so we're still able to extract a value,
        // even if it is an incorrect value.
        TF_AXIOM(cacheView.Get(1).Get<int>() == 2);

        // Key 2 still has a well-defined value.
        TF_AXIOM(cacheView.Get(2).Get<int>() == 42);
    }

    // Now that keys 0 and 1 both depend on cycles, they should both be
    // invalidated on any scene change.
    fixture.invalidRequestIndices.clear();
    fixture.AddRelationshipTarget("/Other.customRel", "/Other/Target3");
    TF_AXIOM(fixture.invalidRequestIndices.size() == 2);
    TF_AXIOM(fixture.invalidRequestIndices.contains(0));
    TF_AXIOM(fixture.invalidRequestIndices.contains(1));
}

int main()
{
    ConfigureTestPlugin();

    Test_CycleDetectionRequiresRecompilation();
    Test_CyclicComputation();
    Test_CyclicComputationPair();
    Test_CyclicRelationshipComputation();
    Test_LargeCyclicRelationshipComputation();
    Test_CyclicAncestorComputation();
    Test_CyclicRelationshipComputationAfterRecompile();
    Test_CyclicRelationshipComputationFigure8();
    Test_CycleDetectionRequestInvalidation();
}
