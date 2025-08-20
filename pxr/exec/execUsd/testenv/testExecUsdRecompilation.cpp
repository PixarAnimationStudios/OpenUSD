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

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/exec/exec/builtinComputations.h"
#include "pxr/exec/exec/computationBuilders.h"
#include "pxr/exec/exec/registerSchema.h"
#include "pxr/exec/exec/systemDiagnostics.h"
#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/readIterator.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/stage.h"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE;

#define ASSERT_EQ(expr, expected)                                       \
    [&] {                                                               \
        auto&& expr_ = expr;                                            \
        if (expr_ != expected) {                                        \
            TF_FATAL_ERROR(                                             \
                "Expected " TF_PP_STRINGIZE(expr) " == '%s'; got '%s'", \
                TfStringify(expected).c_str(),                          \
                TfStringify(expr_).c_str());                            \
        }                                                               \
     }()

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    
    (computeOnNamespaceAncestor)
    (computeUsingCustomAttr)
    (computeUsingCustomRel)
    (computeUsingDuplicateInputNames)
    (customAttr)
    (customRel)
    (customRel2)
);

static void
ConfigureTestPlugin()
{
    const PlugPluginPtrVector testPlugins = PlugRegistry::GetInstance()
        .RegisterPlugins(TfAbsPath("resources"));

    TF_AXIOM(testPlugins.size() == 1);
    TF_AXIOM(testPlugins[0]->GetName() == "testExecUsdRecompilation");
}

static int
CommonComputationCallback(const VdfContext &ctx)
{
    return 42;
}

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(TestExecUsdRecompilationCustomSchema)
{
    // A computation that depends on customAttr only.
    self.PrimComputation(_tokens->computeUsingCustomAttr)
        .Callback(+[](const VdfContext &context) {
            const int *const valuePtr =
                context.GetInputValuePtr<int>(_tokens->customAttr);
            return valuePtr ? *valuePtr : -1;
        })
        .Inputs(
            AttributeValue<int>(_tokens->customAttr));

    // A computation that depends on the targets of customRel.
    self.PrimComputation(_tokens->computeUsingCustomRel)
        .Callback(+[](const VdfContext &context) {
            int result = 0;
            VdfReadIterator<int> it(
                context, ExecBuiltinComputations->computeValue);
            for (; !it.IsAtEnd(); ++it) {
                result += *it;
            }
            return result;
        })
        .Inputs(
            Relationship(_tokens->customRel)
            .TargetedObjects<int>(ExecBuiltinComputations->computeValue));

    // A computation that depends on the namespace ancestor.
    self.PrimComputation(_tokens->computeOnNamespaceAncestor)
        .Callback(CommonComputationCallback)
        .Inputs(
            NamespaceAncestor<int>(_tokens->computeOnNamespaceAncestor));

    // A computation that uses two inputs of the same name and type.
    self.PrimComputation(_tokens->computeUsingDuplicateInputNames)
        .Callback<std::string>(+[](const VdfContext &context) {
            std::string result;
            VdfReadIterator<std::string> it(context,
                ExecBuiltinComputations->computeValue);
            for (; !it.IsAtEnd(); ++it) {
                result += *it;
            }
            return result;
        })
        .Inputs(
            // Both inputs are named 'computeValue'
            Relationship(_tokens->customRel).TargetedObjects<std::string>(
                ExecBuiltinComputations->computeValue),
            Relationship(_tokens->customRel2).TargetedObjects<std::string>(
                ExecBuiltinComputations->computeValue)
        );
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
            std::move(valueKeys));
    }

    UsdStagePtr GetStage() const {
        return _stage;
    }

    UsdPrim GetPrimAtPath(const char *const pathStr) const {
        return _stage->GetPrimAtPath(SdfPath(pathStr));
    }

    UsdAttribute GetAttributeAtPath(const char *const pathStr) const {
        return _stage->GetAttributeAtPath(SdfPath(pathStr));
    }

    UsdRelationship GetRelationshipAtPath(const char *const pathStr) const {
        return _stage->GetRelationshipAtPath(SdfPath(pathStr));
    }

    void GraphNetwork(const char *const filename) {
        ExecSystem::Diagnostics diagnostics(&*_system);
        diagnostics.GraphNetwork(filename);
    }

private:
    UsdStageRefPtr _stage;
    std::optional<ExecUsdSystem> _system;
};

// Tests that we recompile a disconnected attribute input, when that attribute
// comes into existence.
//
static void
TestRecompileDisconnectedAttributeInput(Fixture &fixture)
{
    ExecUsdSystem &system = fixture.NewSystemFromLayer(R"usd(#usda 1.0
        def CustomSchema "Prim" {
        }
    )usd");

    // Compile a leaf node and callback node for `computeUsingCustomAttr`.
    // The callback node's input for `customAttr` is disconnected because the
    // attribute does not exist.
    ExecUsdRequest request = fixture.BuildRequest({
        {fixture.GetPrimAtPath("/Prim"), _tokens->computeUsingCustomAttr}
    });
    system.PrepareRequest(request);
    fixture.GraphNetwork("TestRecompileDisconnectedAttributeInput-1.dot");
    {
        ExecUsdCacheView view = system.Compute(request);
        VtValue v = view.Get(0);
        TF_AXIOM(!v.IsEmpty());
        TF_AXIOM(v.Get<int>() == -1);
    }

    // Create the attribute. The next round of compilation should compile and
    // connect the `customAttr` input of the callback node.
    UsdAttribute attr = fixture.GetPrimAtPath("/Prim").CreateAttribute(
        _tokens->customAttr, SdfValueTypeNames->Int);
    attr.Set(2);
    system.PrepareRequest(request);
    fixture.GraphNetwork("TestRecompileDisconnectedAttributeInput-2.dot");
    {
        ExecUsdCacheView view = system.Compute(request);
        VtValue v = view.Get(0);
        TF_AXIOM(!v.IsEmpty());
        TF_AXIOM(v.Get<int>() == 2);
    }

    // Delete the attribute. The next round of compilation should uncompile the
    // attribute input node--but it should *not* uncompile the time input node.
    const SdfLayerHandle layer = fixture.GetStage()->GetRootLayer();
    TF_AXIOM(layer);
    layer->ImportFromString(R"usd(#usda 1.0
        def CustomSchema "Prim" {
        }
    )usd");
    system.PrepareRequest(request);
    fixture.GraphNetwork("TestRecompileDisconnectedAttributeInput-3.dot");
    {
        ExecUsdCacheView view = system.Compute(request);
        VtValue v = view.Get(0);
        TF_AXIOM(!v.IsEmpty());
        TF_AXIOM(v.Get<int>() == -1);
    }
}

// Tests that inputs which depend on relationship targets are recompiled when
// the set of targets changes.
//
static void
TestRecompileChangedRelationshipTargets(Fixture &fixture)
{
    ExecUsdSystem &system = fixture.NewSystemFromLayer(R"usd(#usda 1.0
        def CustomSchema "Prim" {
            add rel customRel = [</Prim.forwardingRel>, </C.customAttr>]
            add rel forwardingRel
        }
        def Scope "A" {
            int customAttr = 1
        }
        def Scope "B" {
            int customAttr = 2
        }
        def Scope "C" {
        }
    )usd");

    ExecUsdRequest request = fixture.BuildRequest({
        {fixture.GetPrimAtPath("/Prim"), _tokens->computeUsingCustomRel}
    });
    system.PrepareRequest(request);
    fixture.GraphNetwork("TestRecompileChangedRelationshipTargets-1.dot");
    {
        ExecUsdCacheView view = system.Compute(request);
        VtValue v = view.Get(0);
        TF_AXIOM(!v.IsEmpty());
        TF_AXIOM(v.Get<int>() == 0);
    }

    // Create a second target.
    fixture.GetRelationshipAtPath("/Prim.customRel").AddTarget(
        SdfPath("/A.customAttr"));
    system.PrepareRequest(request);
    fixture.GraphNetwork("TestRecompileChangedRelationshipTargets-2.dot");
    {
        ExecUsdCacheView view = system.Compute(request);
        VtValue v = view.Get(0);
        TF_AXIOM(!v.IsEmpty());
        TF_AXIOM(v.Get<int>() == 1);
    }

    // Add a second target on the forwarding relationship.
    fixture.GetRelationshipAtPath("/Prim.forwardingRel").AddTarget(
        SdfPath("/B.customAttr"));
    system.PrepareRequest(request);
    fixture.GraphNetwork("TestRecompileChangedRelationshipTargets-3.dot");
    {
        ExecUsdCacheView view = system.Compute(request);
        VtValue v = view.Get(0);
        TF_AXIOM(!v.IsEmpty());
        TF_AXIOM(v.Get<int>() == 3);
    }

    // Create the missing 'customAttr' on prim C.
    UsdPrim primC = fixture.GetPrimAtPath("/C");
    TF_AXIOM(primC);
    UsdAttribute attr =
        primC.CreateAttribute(_tokens->customAttr, SdfValueTypeNames->Int);
    attr.Set(3);
    system.PrepareRequest(request);
    fixture.GraphNetwork("TestRecompileChangedRelationshipTargets-4.dot");
    {
        ExecUsdCacheView view = system.Compute(request);
        VtValue v = view.Get(0);
        TF_AXIOM(!v.IsEmpty());
        TF_AXIOM(v.Get<int>() == 6);
    }

    // Clear all targets.
    fixture.GetRelationshipAtPath("/Prim.customRel")
        .ClearTargets(/* removeSpec */ true);
    system.PrepareRequest(request);
    fixture.GraphNetwork("TestRecompileChangedRelationshipTargets-5.dot");
    {
        ExecUsdCacheView view = system.Compute(request);
        VtValue v = view.Get(0);
        TF_AXIOM(!v.IsEmpty());
        TF_AXIOM(v.Get<int>() == 0);
    }
}

// Tests that changes to objects that were previously targeted by a
// relationship (but are no longer targeted) do not cause uncompilation of
// inputs that depend on the new targets of that relationship.
//
static void
TestRecompileAfterChangingOldRelationshipTarget(Fixture &fixture)
{
    ExecUsdSystem &system = fixture.NewSystemFromLayer(R"usd(#usda 1.0
        def CustomSchema "Prim" {
            add rel customRel = [</X.attr>, </Y.attr>, </Z.attr>]
        }
        def Scope "X" {
            int attr = 1
        }
        def Scope "Y" {
            int attr = 2
        }
        def Scope "Z" {
            int attr = 3
        }
    )usd");

    ExecUsdRequest request = fixture.BuildRequest({
        {fixture.GetPrimAtPath("/Prim"), _tokens->computeUsingCustomRel}
    });

    // Compile the network.
    system.PrepareRequest(request);
    fixture.GraphNetwork(
        "TestRecompileAfterChangingOldRelationshipTarget-1.dot");
    {
        ExecUsdCacheView view = system.Compute(request);
        VtValue v = view.Get(0);
        TF_AXIOM(!v.IsEmpty());
        TF_AXIOM(v.Get<int>() == 6);
    }

    // Remove <X.attr> as a relationship target. This will disconnect all
    // VdfConnections to the callback node input.
    fixture.GetRelationshipAtPath("/Prim.customRel").RemoveTarget(
        SdfPath("/X.attr"));
    fixture.GraphNetwork(
        "TestRecompileAfterChangingOldRelationshipTarget-2.dot");

    // Re-compile the network.
    system.PrepareRequest(request);
    fixture.GraphNetwork(
        "TestRecompileAfterChangingOldRelationshipTarget-3.dot");
    {
        ExecUsdCacheView view = system.Compute(request);
        VtValue v = view.Get(0);
        TF_AXIOM(!v.IsEmpty());
        TF_AXIOM(v.Get<int>() == 5);
    }

    // Deactivate </X>. This should not affect the compiled network because
    // <X.attr>'s computeValue is no longer connected to the callback node.
    fixture.GetPrimAtPath("/X").SetActive(false);
    fixture.GraphNetwork(
        "TestRecompileAfterChangingOldRelationshipTarget-4.dot");
    {
        ExecUsdCacheView view = system.Compute(request);
        VtValue v = view.Get(0);
        TF_AXIOM(!v.IsEmpty());
        TF_AXIOM(v.Get<int>() == 5);
    }
}

// Tests that when we recompile a network, we recompile all inputs that
// require recompilation, even those that do not contribute to the request
// being compiled.
//
static void
TestRecompileMultipleRequests(Fixture &fixture)
{
    ExecUsdSystem &system = fixture.NewSystemFromLayer(R"usd(#usda 1.0
        def CustomSchema "Prim1" {
            int customAttr = 10
        }
        def CustomSchema "Prim2" {
            int customAttr = 20
        }
    )usd");

    UsdPrim prim1 = fixture.GetPrimAtPath("/Prim1");
    UsdPrim prim2 = fixture.GetPrimAtPath("/Prim2");

    // Make 2 requests.
    ExecUsdRequest request1 = fixture.BuildRequest({
        {prim1, _tokens->computeUsingCustomAttr}
    });
    ExecUsdRequest request2 = fixture.BuildRequest({
        {prim2, _tokens->computeUsingCustomAttr}
    });
    
    // Compile the requests.
    system.PrepareRequest(request1);
    system.PrepareRequest(request2);
    fixture.GraphNetwork("TestRecompileMultipleRequests-1.dot");

    // Remove the custom attributes. This will uncompile both attribute input
    // nodes.
    prim1.RemoveProperty(_tokens->customAttr);
    prim2.RemoveProperty(_tokens->customAttr);
    fixture.GraphNetwork("TestRecompileMultipleRequests-2.dot");

    // Re-add both attributes.
    prim1.CreateAttribute(_tokens->customAttr, SdfValueTypeNames->Int);
    prim2.CreateAttribute(_tokens->customAttr, SdfValueTypeNames->Int);

    // By preparing just one of the requests, all inputs should be recompiled,
    // even those that only contribute to the other request.
    system.PrepareRequest(request1);
    fixture.GraphNetwork("TestRecompileMultipleRequests-3.dot");
}

// Tests that when we recompile a network, we delete nodes and connections
// that become isolated during uncompilation and remain isolated after
// recompilation.
//
static void
TestRecompileDeletedPrim(Fixture &fixture)
{
    ExecUsdSystem &system = fixture.NewSystemFromLayer(R"usd(#usda 1.0
        def CustomSchema "Prim1" {
            def CustomSchema "Prim2" {
            }
        }
        def CustomSchema "Prim3" {
        }
    )usd");

    UsdPrim prim2 = fixture.GetPrimAtPath("/Prim1/Prim2");
    UsdPrim prim3 = fixture.GetPrimAtPath("/Prim3");

    // Make 2 requests.
    ExecUsdRequest request1 = fixture.BuildRequest({
        {prim2, _tokens->computeOnNamespaceAncestor}
    });
    ExecUsdRequest request2 = fixture.BuildRequest({
        {prim3, _tokens->computeOnNamespaceAncestor}
    });
    
    // Compile the requests.
    system.PrepareRequest(request1);
    system.PrepareRequest(request2);
    fixture.GraphNetwork("TestRecompileDeletedPrim-1.dot");

    // Remove Prim2
    const SdfLayerHandle layer = fixture.GetStage()->GetRootLayer();
    TF_AXIOM(layer);
    layer->ImportFromString(R"usd(#usda 1.0
        def CustomSchema "Prim1" {
        }
        def CustomSchema "Prim3" {
        }
    )usd");

    fixture.GraphNetwork("TestRecompileDeletedPrim-2.dot");

    // Prepare only the request that still has a value key with a valid
    // provider.
    system.PrepareRequest(request2);
    fixture.GraphNetwork("TestRecompileDeletedPrim-3.dot");
}

// Tests that when a prim is resynced (but not deleted), we can recompile
// value keys for that prim.
//
static void
TestRecompileResyncedPrim(Fixture &fixture)
{
    ExecUsdSystem &system = fixture.NewSystemFromLayer(R"usd(#usda 1.0
        def CustomSchema "Prim" {
            int customAttr = 1
        }
    )usd");

    UsdPrim prim = fixture.GetPrimAtPath("/Prim");

    // Request a computation on Prim.
    ExecUsdRequest request = fixture.BuildRequest({
        {prim, _tokens->computeUsingCustomAttr}
    });

    // Compile and evaluate the request.
    system.PrepareRequest(request);
    fixture.GraphNetwork("TestRecompileResyncedPrim-1.dot");
    {
        ExecUsdCacheView view = system.Compute(request);
        VtValue v = view.Get(0);
        TF_AXIOM(!v.IsEmpty());
        TF_AXIOM(v.Get<int>() == 1);
    }

    // Apply a schema to the prim. This produduces a resync event for the prim,
    // but the prim still exists.
    prim.AddAppliedSchema(TfToken("CustomAppliedSchema"));
    fixture.GraphNetwork("TestRecompileResyncedPrim-2.dot");
    
    // Compile a new request for the same value key. This should recompile the
    // leaf node because the prim still exists.
    system.PrepareRequest(request);
    fixture.GraphNetwork("TestRecompileResyncedPrim-3.dot");
    {
        ExecUsdCacheView view = system.Compute(request);
        VtValue v = view.Get(0);
        TF_AXIOM(!v.IsEmpty());
        TF_AXIOM(v.Get<int>() == 1);
    }
}

static void
TestRecompileDuplicateInputNames(Fixture &fixture)
{
    ExecUsdSystem &system = fixture.NewSystemFromLayer(R"usd(#usda 1.0
        def CustomSchema "Prim" {
            add rel customRel = [</Targets.a>]
            add rel customRel2 = [</Targets.x>]
        }
        def Scope "Targets" {
            custom string a = "a"
            custom string b = "b"
            custom string x = "x"
            custom string y = "y"
        }
    )usd");

    ExecUsdRequest request = fixture.BuildRequest({
        {fixture.GetPrimAtPath("/Prim"),
         _tokens->computeUsingDuplicateInputNames}
    });

    // Compile and compute the request.
    system.PrepareRequest(request);
    fixture.GraphNetwork("TestRecompileDuplicateInputNames-1.dot");
    {
        ExecUsdCacheView view = system.Compute(request);
        VtValue v = view.Get(0);
        TF_AXIOM(!v.IsEmpty());
        ASSERT_EQ(v.Get<std::string>(), "ax");
    }

    // Add a target to customRel. This affects the journal of the first input
    // registered as 'computeValue'. After recompilation, the computed value
    // should be greater to account for the new target.
    fixture.GetRelationshipAtPath("/Prim.customRel")
        .AddTarget(SdfPath("/Targets.b"), UsdListPositionBackOfAppendList);
    system.PrepareRequest(request);
    fixture.GraphNetwork("TestRecompileDuplicateInputNames-2.dot");
    {
        ExecUsdCacheView view = system.Compute(request);
        VtValue v = view.Get(0);
        TF_AXIOM(!v.IsEmpty());
        ASSERT_EQ(v.Get<std::string>(), "abx");
    }

    // Add a target to customRel2. This affects the journal of the second input
    // registered as 'computeValue'. After recompilation, the computed value
    // should be greater to account for the new target.
    fixture.GetRelationshipAtPath("/Prim.customRel2")
        .AddTarget(SdfPath("/Targets.y"), UsdListPositionBackOfAppendList);
    system.PrepareRequest(request);
    fixture.GraphNetwork("TestRecompileDuplicateInputNames-3.dot");
    {
        ExecUsdCacheView view = system.Compute(request);
        VtValue v = view.Get(0);
        TF_AXIOM(!v.IsEmpty());
        ASSERT_EQ(v.Get<std::string>(), "abxy");
    }
}

int main()
{
    ConfigureTestPlugin();

    std::vector tests {
        TestRecompileDisconnectedAttributeInput,
        TestRecompileMultipleRequests,
        TestRecompileChangedRelationshipTargets,
        TestRecompileAfterChangingOldRelationshipTarget,
        TestRecompileDeletedPrim,
        TestRecompileResyncedPrim,
        TestRecompileDuplicateInputNames,
    };
    for (const auto &test : tests) {
        Fixture fixture;
        test(fixture);
    }
}
