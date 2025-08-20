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
#include "pxr/base/tf/token.h"
#include "pxr/exec/exec/registerSchema.h"
#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/readIterator.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/stage.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

#define ASSERT_EQ(expr, expected)                                              \
    [&] {                                                                      \
        auto&& expr_ = expr;                                                   \
        if (expr_ != expected) {                                               \
            TF_FATAL_ERROR(                                                    \
                "Expected " TF_PP_STRINGIZE(expr) " == '%s'; got '%s'",        \
                TfStringify(expected).c_str(),                                 \
                TfStringify(expr_).c_str());                                   \
        }                                                                      \
    }()

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (consumesDispatchedComputation)
    (consumesDispatchedComputationOnAncestor)
    (consumesRecursiveDispatchedComputationOnAncestor)
    (recursiveDispatchedComputationOnAncestor)
    (dispatchedComputation)
    (dispatchingRel)
    (myAttr)
    (nonDispatchedInput)
    (nonDispatchingRel)
);

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(
    TestExecUsdDispatchedComputationsDispatchingSchema1)
{
    self.DispatchedPrimComputation(
        _tokens->dispatchedComputation)
        .Callback<int>(+[](const VdfContext &) { return 42; });

    // Register a prim computation that requests the above dispatched
    // computation via a relationship.
    self.PrimComputation(_tokens->consumesDispatchedComputation)
        .Callback<int>(+[](const VdfContext &ctx) {
            int result = 0;
            for (VdfReadIterator<int> it(ctx, _tokens->dispatchedComputation);
                 !it.IsAtEnd(); ++it) {
                result += *it;
            }
            for (VdfReadIterator<int> it(ctx, _tokens->nonDispatchedInput);
                 !it.IsAtEnd(); ++it) {
                result += *it;
            }
            return result;
        })
        .Inputs(

            // This input can find the dispatched computation.
            Relationship(_tokens->dispatchingRel)
                .TargetedObjects<int>(_tokens->dispatchedComputation)
                .FallsBackToDispatched(),

            // This input *can't* find the dispatched computation because it
            // doesn't request it.
            Relationship(_tokens->nonDispatchingRel)
                .TargetedObjects<int>(_tokens->dispatchedComputation)
                .InputName(_tokens->nonDispatchedInput)
        );

    // Register a prim computation that requests the above dispatched
    // computation on a namespace ancestor.
    self.PrimComputation(_tokens->consumesDispatchedComputationOnAncestor)
        .Callback<int>(+[](const VdfContext &ctx) {
            const int *const valuePtr =
                ctx.GetInputValuePtr<int>(_tokens->dispatchedComputation);
            return valuePtr ? *valuePtr : -1;
        })
        .Inputs(
            NamespaceAncestor<int>(_tokens->dispatchedComputation)
                .FallsBackToDispatched());

    // Register a recursive dispatched computation, i.e., a dispatched
    // computation that requests an input from another input of the same
    // dispatched computation.
    self.DispatchedPrimComputation(
        _tokens->recursiveDispatchedComputationOnAncestor)
        .Callback<int>(+[](const VdfContext &ctx) {
            const int *const attrValuePtr =
                ctx.GetInputValuePtr<int>(_tokens->myAttr);
            if (attrValuePtr) {
                return *attrValuePtr;
            }

            const int *const valuePtr =
                ctx.GetInputValuePtr<int>(
                    _tokens->recursiveDispatchedComputationOnAncestor);
            return valuePtr ? *valuePtr : -1;
        })
        .Inputs(
            AttributeValue<int>(_tokens->myAttr),
            NamespaceAncestor<int>(
                _tokens->recursiveDispatchedComputationOnAncestor)
                .FallsBackToDispatched());

    // Register a computation that consumes a recursive dispatched computation.
    self.PrimComputation(
        _tokens->consumesRecursiveDispatchedComputationOnAncestor)
        .Callback<int>(+[](const VdfContext &ctx) {
            const int *const valuePtr =
                ctx.GetInputValuePtr<int>(
                    _tokens->recursiveDispatchedComputationOnAncestor);
            return valuePtr ? *valuePtr : -1;
        })
        .Inputs(
            NamespaceAncestor<int>(
                _tokens->recursiveDispatchedComputationOnAncestor)
                .FallsBackToDispatched());
}

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(
    TestExecUsdDispatchedComputationsDispatchingSchema2)
{
    // Register a dispatched computation that only applies to prims with 
    // MatchingSchema applied.
    self.DispatchedPrimComputation(
        _tokens->dispatchedComputation,
        TfType::FindByName("TestExecUsdDispatchedComputationsMatchingingSchema"))
        .Callback<int>(+[](const VdfContext &) { return 11; });

    self.PrimComputation(_tokens->consumesDispatchedComputation)
        .Callback<int>(+[](const VdfContext &ctx) {
            const int *const valuePtr =
                ctx.GetInputValuePtr<int>(_tokens->dispatchedComputation);
            return valuePtr ? *valuePtr : -1;
        })
        .Inputs(
            Relationship(_tokens->dispatchingRel)
                .TargetedObjects<int>(_tokens->dispatchedComputation)
                .FallsBackToDispatched()
        );
}

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(
    TestExecUsdDispatchedComputationsNonDispatchingSchema)
{
    // Register a computation that requests a dispatched computation that it
    // will fail to find, since it does not dispatch the requested computation.
    //
    // Note that if a schema that *does* dispatch 'dispatchedComputation' is
    // applied to the same prim as this schema, then this computation *can* find
    // the dispatched computation.
    //
    self.PrimComputation(_tokens->consumesDispatchedComputation)
        .Callback<int>(+[](const VdfContext &ctx) {
            const int *const valuePtr =
                ctx.GetInputValuePtr<int>(_tokens->dispatchedComputation);
            return valuePtr ? *valuePtr + 100 : -100;
        })
        .Inputs(
            Relationship(_tokens->dispatchingRel)
                .TargetedObjects<int>(_tokens->dispatchedComputation)
                    .FallsBackToDispatched()
        );
}

// Tests the semantics of dispatched computations.
static void
TestDispatchedComputation()
{
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(R"usd(#usda 1.0
        def Scope "DispatchedOnPrim" {}

        # DispatchingSchema1 dispatches a computaiton, which is found via
        # dispatchingRel, but not via nonDispatchingRel.
        def Scope "DispatchingPrim1" (apiSchemas = ["DispatchingSchema1"]) {
            add rel dispatchingRel = </DispatchedOnPrim>
            add rel nonDispatchingRel = </DispatchedOnPrim>
        }

        # DispatchingSchema2 dispatches a computation of the same name as the
        # one dispatched by DispatchingSchema1, but there is no name collision,
        # due to dispatching semantics. Furthermore, this dispatched computation
        # only matches if the API schema MatchingSchema is applied to the target
        # prim.
        def Scope "DispatchingPrim2" (apiSchemas = ["DispatchingSchema2"]) {
            add rel dispatchingRel = </DispatchedOnPrim>
        }

        # The schema applied to this prim attempts to consume a dispatched
        # computation of the same name as the computations dispatched by the
        # dispatching schemas, but it can't find it because it doesn't dispatch
        # the computation.
        def Scope "NonDispatchingPrim" (apiSchemas = ["NonDispatchingSchema"]) {
            add rel dispatchingRel = </DispatchedOnPrim>
            def Scope "DispatchedOnPrim" {}
        }
    )usd");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    ExecUsdSystem execSystem(usdStage);

    UsdPrim dispatchedOnPrim =
        usdStage->GetPrimAtPath(SdfPath("/DispatchedOnPrim"));
    TF_AXIOM(dispatchedOnPrim.IsValid());

    UsdPrim dispatchingPrim1 =
        usdStage->GetPrimAtPath(SdfPath("/DispatchingPrim1"));
    TF_AXIOM(dispatchingPrim1.IsValid());

    UsdPrim dispatchingPrim2 =
        usdStage->GetPrimAtPath(SdfPath("/DispatchingPrim2"));
    TF_AXIOM(dispatchingPrim2.IsValid());

    UsdPrim nonDispatchingPrim =
        usdStage->GetPrimAtPath(SdfPath("/NonDispatchingPrim"));
    TF_AXIOM(nonDispatchingPrim.IsValid());

    std::vector<ExecUsdValueKey> valueKeys {
        {dispatchingPrim1,
         TfToken("consumesDispatchedComputation")},
        {dispatchingPrim2,
         TfToken("consumesDispatchedComputation")},
        {nonDispatchingPrim,
         TfToken("consumesDispatchedComputation")}
    };

    ExecUsdRequest request = execSystem.BuildRequest(std::move(valueKeys));
    TF_AXIOM(request.IsValid());

    execSystem.PrepareRequest(request);
    TF_AXIOM(request.IsValid());

    {
        ExecUsdCacheView view = execSystem.Compute(request);

        VtValue v = view.Get(0);
        TF_AXIOM(v.IsHolding<int>());
        ASSERT_EQ(v.Get<int>(), 42);

        v = execSystem.Compute(request).Get(1);
        TF_AXIOM(v.IsHolding<int>());
        ASSERT_EQ(v.Get<int>(), -1);

        v = view.Get(2);
        TF_AXIOM(v.IsHolding<int>());
        ASSERT_EQ(v.Get<int>(), -100);
    }

    // Now apply MatchingSchema to dispatchedOnPrim, so the computation
    // dispatched by DispatchingPrim2 will be found.
    dispatchedOnPrim.ApplyAPI(TfToken("MatchingSchema"));

    {
        ExecUsdCacheView view = execSystem.Compute(request);

        VtValue v = view.Get(1);
        TF_AXIOM(v.IsHolding<int>());
        ASSERT_EQ(v.Get<int>(), 11);
    }

    // Now apply a dispatching schema to NonDispatchingPrim, so that it will
    // be able to find the dispatched computation.
    //
    // Note that NonDispatchingSchema is stronger, so we end up computing the
    // 'consumesDispatchedComputation' defined by that schema, but finding the
    // 'dispatchedComputation' dispatched by DispatchingSchema1.
    //
    nonDispatchingPrim.ApplyAPI(TfToken("DispatchingSchema1"));

    execSystem.PrepareRequest(request);
    TF_AXIOM(request.IsValid());

    {
        ExecUsdCacheView view = execSystem.Compute(request);

        VtValue v = view.Get(2);
        TF_AXIOM(v.IsHolding<int>());
        ASSERT_EQ(v.Get<int>(), 142);
    }
}

// Tests dispatched computations consumed via namespace ancestor inputs.
static void
TestDispatchedComputationOnAncestor()
{
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(R"usd(#usda 1.0
        def Scope "Root" {
            def Scope "DispatchingPrim" (apiSchemas = ["DispatchingSchema1"]) {
            }
        }
    )usd");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    ExecUsdSystem execSystem(usdStage);

    const UsdPrim dispatchingPrim =
        usdStage->GetPrimAtPath(SdfPath("/Root/DispatchingPrim"));
    TF_AXIOM(dispatchingPrim.IsValid());

    std::vector<ExecUsdValueKey> valueKeys {
        {dispatchingPrim,
         TfToken("consumesDispatchedComputationOnAncestor")}
    };

    ExecUsdRequest request = execSystem.BuildRequest(std::move(valueKeys));
    TF_AXIOM(request.IsValid());

    execSystem.PrepareRequest(request);
    TF_AXIOM(request.IsValid());

    ExecUsdCacheView view = execSystem.Compute(request);

    VtValue v = view.Get(0);
    TF_AXIOM(v.IsHolding<int>());
    ASSERT_EQ(v.Get<int>(), 42);
}

// Tests that a computation can be dispatched recursively.
//
// I.e., here we verify that a dispatched computation can take input from a
// different instance of the same computation, dispatched onto a different prim.
//
static void
TestRecursiveDispatchedComputation()
{
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(R"usd(#usda 1.0
        def Scope "Root" {
            custom int myAttr = 10
            def Scope "Prim" {
                def Scope "DispatchingPrim" (
                    apiSchemas = ["DispatchingSchema1"]
                ) {
                }
            }
        }
    )usd");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    ExecUsdSystem execSystem(usdStage);

    const UsdPrim dispatchingPrim =
        usdStage->GetPrimAtPath(SdfPath("/Root/Prim/DispatchingPrim"));
    TF_AXIOM(dispatchingPrim.IsValid());

    std::vector<ExecUsdValueKey> valueKeys {
        {dispatchingPrim,
         TfToken("consumesRecursiveDispatchedComputationOnAncestor")}
    };

    ExecUsdRequest request = execSystem.BuildRequest(std::move(valueKeys));
    TF_AXIOM(request.IsValid());

    execSystem.PrepareRequest(request);
    TF_AXIOM(request.IsValid());

    ExecUsdCacheView view = execSystem.Compute(request);

    VtValue v = view.Get(0);
    TF_AXIOM(v.IsHolding<int>());
    ASSERT_EQ(v.Get<int>(), 10);
}

int main()
{
    // Load test custom schemas.
    const PlugPluginPtrVector testPlugins = PlugRegistry::GetInstance()
        .RegisterPlugins(TfAbsPath("resources"));
    ASSERT_EQ(testPlugins.size(), 1);
    ASSERT_EQ(testPlugins[0]->GetName(), "testExecUsdDispatchedComputations");

    TestDispatchedComputation();
    TestDispatchedComputationOnAncestor();
    TestRecursiveDispatchedComputation();

    return 0;
}
