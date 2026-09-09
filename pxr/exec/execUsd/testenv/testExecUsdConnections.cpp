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
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/exec/exec/builtinComputations.h"
#include "pxr/exec/exec/registerSchema.h"
#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/readIterator.h"
#include "pxr/exec/vdf/readIteratorRange.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include <iostream>
#include <numeric>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

#define ASSERT_EQ(expr, expected)                                              \
    [&] {                                                                      \
        std::cout << std::flush;                                               \
        std::cerr << std::flush;                                               \
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

    (attr)
    (emptyAttr)
    (computeViaIncomingConnections)
    (computeConnectedConstants)
    (computeConstant)
);

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(
    TestExecUsdConnectionsCustomSchema)
{
    // A prim computation that computes the values of the string-valued
    // attributes that target the prim with attribute connections.
    self.PrimComputation(
        _tokens->computeViaIncomingConnections)
        .Callback(+[](const VdfContext &ctx) -> std::string {
            std::string result;
            for (VdfReadIterator<std::string> it(
                     ctx, ExecBuiltinComputations->computeResolvedValue);
                 !it.IsAtEnd(); ++it) {
                if (!result.empty()) {
                    result += " ";
                }
                result += "'" + *it + "'";
            }
            return result.empty() ? "(no value)" : result;
        })
        .Inputs(
            IncomingConnections<std::string>(
                ExecBuiltinComputations->computeResolvedValue)
        );

    // An attribute computation that always returns the constant value 1.
    self.AttributeComputation(_tokens->attr, _tokens->computeConstant)
        .Callback(+[](const VdfContext &) { return 1; });

    // An attribute computation that sums computeConstant on all targeted
    // objects.
    self.AttributeComputation(_tokens->attr, _tokens->computeConnectedConstants)
        .Callback(+[](const VdfContext &ctx) -> int {
            const VdfReadIteratorRange<int> range(
                ctx, _tokens->computeConstant);
            return std::accumulate(range.begin(), range.end(), 0);
        })
        .Inputs(
            Connections<int>(_tokens->computeConstant)
        );

    self.AttributeExpression(_tokens->emptyAttr)
        .Callback<std::string>([](const VdfContext &ctx) -> void {
            ctx.SetEmptyOutput();
        });
}

// Verifies we got the expected single invalid index and resets invalidIndices
// in preparation for more testing.
//
// \p indexSet should be the ExecRequestIndexSet that we get from a request
// invalidation callback.
// 
// \p i is the single index we expect to find in the set.
//
#define VERIFY_SINGLE_INVALID_INDEX(indexSet, i)                               \
    ASSERT_EQ(indexSet.size(), 1);                                             \
    ASSERT_EQ(indexSet.count(i), 1);                                           \
    indexSet.clear();

// Test that we get the expected results and the expected invalidation when
// attribute connections are edited.
//
static void
TestAttributeConnections()
{
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(R"usd(#usda 1.0
        def CustomSchema "Prim" {
            string attr = "attr value"

            // Note that 'attr2' does not exist
            string attr.connect = [</Prim.attr2>]

            string attr3 = "attr3 value"
            string emptyAttr = "empty"
            string[] array = ["array value"]
        }
    )usd");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    ExecUsdSystem execSystem(usdStage);

    UsdPrim prim = usdStage->GetPrimAtPath(SdfPath("/Prim"));
    TF_AXIOM(prim.IsValid());

    UsdAttribute attr = usdStage->GetAttributeAtPath(SdfPath("/Prim.attr"));
    TF_AXIOM(attr.IsValid());

    ExecRequestIndexSet invalidIndices;

    ExecUsdRequest request = execSystem.BuildRequest(
        {{attr, ExecBuiltinComputations->computeValue}},
        [&invalidIndices](const ExecRequestIndexSet &indices,
                          const class EfTimeInterval &) {
            invalidIndices = indices;
        });
    TF_AXIOM(request.IsValid());

    execSystem.PrepareRequest(request);
    TF_AXIOM(request.IsValid());
    // The connection owned by 'attr' targets 'attr2', which does not exist, so 
    // the computed result falls back to the resolved value of 'attr'. 
    {
        ExecUsdCacheView view = execSystem.Compute(request);
        VtValue v = view.Get(0);
        TF_AXIOM(v.IsHolding<std::string>());
        ASSERT_EQ(
            v.Get<std::string>(),
            "attr value");
    }

    // Add a connection to an existing attribute. The value of the connected 
    // attribute is ignored, and the computed result is the resolved value of 
    // the owning attribute 'attr'. 
    attr.AddConnection(SdfPath("/Prim.attr3"));
    VERIFY_SINGLE_INVALID_INDEX(invalidIndices, 0);
    {
        ExecUsdCacheView view = execSystem.Compute(request);
        VtValue v = view.Get(0);
        TF_AXIOM(v.IsHolding<std::string>());
        ASSERT_EQ(
            v.Get<std::string>(),
            "attr value");
    }

    // Remove the connection that targets attr2, s.t. the single remaining 
    // valid connection owned by 'attr' provides its computed value.
    attr.RemoveConnection(SdfPath("/Prim.attr2"));
    TF_AXIOM(request.IsValid());
    VERIFY_SINGLE_INVALID_INDEX(invalidIndices, 0);
    {
        ExecUsdCacheView view = execSystem.Compute(request);
        VtValue v = view.Get(0);
        TF_AXIOM(v.IsHolding<std::string>());
        ASSERT_EQ(
            v.Get<std::string>(),
            "attr3 value");
    }

    // Remove the existing connection and connect an attribute that produces an 
    // empty output. 
    attr.RemoveConnection(SdfPath("/Prim.attr3"));
    VERIFY_SINGLE_INVALID_INDEX(invalidIndices, 0);

    UsdAttribute emptyAttr =
        usdStage->GetAttributeAtPath(SdfPath("/Prim.emptyAttr"));
    TF_AXIOM(emptyAttr.IsValid());
    
    attr.AddConnection(SdfPath("/Prim.emptyAttr"));
    {
        ExecUsdCacheView view = execSystem.Compute(request);
        TF_AXIOM(view.Get(0).IsEmpty());
    }

    // Reverse the connection s.t. 'emptyAttr' targets 'attr'. The connected 
    // value is ignored since the computed value is provided by the expression 
    // registered for 'emptyAttr'.   
    attr.RemoveConnection(SdfPath("/Prim.emptyAttr"));
    TF_AXIOM(emptyAttr.IsValid());
    VERIFY_SINGLE_INVALID_INDEX(invalidIndices, 0);
    emptyAttr.AddConnection(SdfPath("/Prim.attr"));

    ExecUsdRequest emptyAttrRequest = execSystem.BuildRequest({
        {emptyAttr, ExecBuiltinComputations->computeValue}});
    TF_AXIOM(emptyAttrRequest.IsValid());

    execSystem.PrepareRequest(emptyAttrRequest);
    TF_AXIOM(emptyAttrRequest.IsValid());
    {
        ExecUsdCacheView view = execSystem.Compute(emptyAttrRequest);
        TF_AXIOM(view.Get(0).IsEmpty());
    }

    // Connect scalar-typed attribute 'attr' to an array-typed attribute 
    // 'array' that contains a single element. 
    UsdAttribute arrayAttr =
        usdStage->GetAttributeAtPath(SdfPath("/Prim.array"));
    TF_AXIOM(arrayAttr.IsValid());

    attr.AddConnection(SdfPath("/Prim.array"));
    // This is an unsupported case, so the computed value is the resolved value 
    // of 'attr'.
    {
        ExecUsdCacheView view = execSystem.Compute(request);
        VtValue v = view.Get(0);
        TF_AXIOM(v.IsHolding<std::string>());
        ASSERT_EQ(
            v.Get<std::string>(),
            "attr value");
    }
}

// Tests that Connections inputs omit input values from targeted objects if
// those objects don't provide the requested computation.
//
static void
TestConnectionsComputationNotFound()
{
    const TfErrorMark errorMark;

    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(R"usd(#usda 1.0
        def CustomSchema "Prim" {
            string attr = "Prim.attr"
            string attr.connect = [
                </Target1.attr>,
                </Target2.otherAttr>,
                </Target3.attr>
            ]
        }
        def CustomSchema "Target1" {
            # This attribute has the computeConstant computation.
            string attr = "Target1.attr"
        }
        def CustomSchema "Target2" {
            # This attribute does not have the computeConstant computation,
            # because the attribute has a different name.
            string otherAttr = "Target2.otherAttr"
        }
        def "Target3" {
            # This attribute does not have the computeConstant computation,
            # because the prim is not a CustomSchema.
            string attr = "Target3.attr"
        }
    )usd");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    ExecUsdSystem execSystem(usdStage);

    const UsdAttribute attr =
        usdStage->GetAttributeAtPath(SdfPath("/Prim.attr"));
    TF_AXIOM(attr.IsValid());

    const ExecUsdRequest request = execSystem.BuildRequest({
        {attr, _tokens->computeConnectedConstants}});
    TF_AXIOM(request.IsValid());

    execSystem.PrepareRequest(request);
    TF_AXIOM(request.IsValid());

    const ExecUsdCacheView view = execSystem.Compute(request);
    TF_AXIOM(view.Get(0).IsHolding<int>());
    ASSERT_EQ(view.Get(0).Get<int>(), 1);

    // There was previously a bug where composing the exec prim definition of
    // typeless prims (e.g. Target3) would emit a coding error. This error mark
    // verifies that no such coding errors were emitted.
    TF_AXIOM(errorMark.IsClean());
}

static void
TestIncomingConnections()
{
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(R"usd(#usda 1.0
        def CustomSchema "Prim" {
            string attr = "attr value"
            string attr.connect = [</Prim>]
            string attr3 = "attr3 value"
        }
    )usd");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    ExecUsdSystem execSystem(usdStage);

    UsdPrim prim = usdStage->GetPrimAtPath(SdfPath("/Prim"));
    TF_AXIOM(prim.IsValid());

    ExecUsdRequest request = execSystem.BuildRequest({
        {prim, _tokens->computeViaIncomingConnections}});
    TF_AXIOM(request.IsValid());

    execSystem.PrepareRequest(request);
    TF_AXIOM(request.IsValid());

    {
        ExecUsdCacheView view = execSystem.Compute(request);
        VtValue v = view.Get(0);
        TF_AXIOM(v.IsHolding<std::string>());
        ASSERT_EQ(
            v.Get<std::string>(),
            "'attr value'");
    }

    UsdAttribute attr3 = usdStage->GetAttributeAtPath(SdfPath("/Prim.attr3"));
    TF_AXIOM(attr3.IsValid());

    // Add another connection
    attr3.AddConnection(SdfPath("/Prim"));

    {
        ExecUsdCacheView view = execSystem.Compute(request);
        VtValue v = view.Get(0);
        TF_AXIOM(v.IsHolding<std::string>());
        ASSERT_EQ(
            v.Get<std::string>(),
            "'attr value' "
            "'attr3 value'");
    }
}

// Test that a request's schedule is properly invalidated when the leaf node 
// for one of the request's value keys becomes disconnected. 
//
// Note that the deletion of any non-leaf node in the schedule would invalidate 
// the schedule. This test carefully constructs a situation in which the leaf 
// node is disconnected, but the node feeding that leaf node remains in the 
// network. 
//
static void TestDisconnectedValueKeys()
{
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(R"usd(#usda 1.0
        def CustomSchema "Prim" {
            int attr = 1
            int attrTarget = 2
        }
    )usd");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    ExecUsdSystem execSystem(usdStage);

    UsdPrim prim = usdStage->GetPrimAtPath(SdfPath("/Prim"));
    TF_AXIOM(prim.IsValid());

    UsdAttribute attr = usdStage->GetAttributeAtPath(SdfPath("/Prim.attr"));
    TF_AXIOM(attr.IsValid());

    // Request the resolved value and computed value of the attribute provider. 
    // The compiled network contains a leaf node for each requested  
    // computation, both of which source their input from the resolved value of 
    // the provider, as the provider does not own any connections nor an 
    // expression. 
    ExecUsdRequest request = execSystem.BuildRequest({
        {attr, ExecBuiltinComputations->computeResolvedValue},
        {attr, ExecBuiltinComputations->computeValue}});
    TF_AXIOM(request.IsValid());

    execSystem.PrepareRequest(request);
    TF_AXIOM(request.IsValid());

    // The provider does not own any connections, so the computed result falls 
    // back to its resolved value. 
    {
        ExecUsdCacheView view = execSystem.Compute(request);
        VtValue v = view.Get(1);
        TF_AXIOM(v.IsHolding<int>());
        ASSERT_EQ(v.Get<int>(), 1);
    }

    // Add a connection s.t. the computed value of the provider is sourced 
    // across the connection. This will disconnect the leaf input representing  
    // 'computeValue' and reconnect it to the result of 
    // 'computeConnectedValue'. 
    //
    // Note that the node providing 'computeResolvedValue' is still feeding the 
    // leaf node for the value key at index 0. Although no nodes in the 
    // schedule were modified, the request schedule is still invalidated since 
    // a leaf node corresponding to a value key in the request was 
    // disconnected. 
    //
    attr.AddConnection(SdfPath("/Prim.attrTarget"));
    {
        ExecUsdCacheView view = execSystem.Compute(request);
        VtValue v = view.Get(1);
        TF_AXIOM(v.IsHolding<int>());
        ASSERT_EQ(v.Get<int>(), 2);
    }

    // Remove the connection and verify that the computed value is the resolved 
    // value of the owner. This will disconnect the leaf input representing 
    // 'computeValue' and reconnect it to the output that is providing 
    // 'computeResolvedValue'. 
    attr.RemoveConnection(SdfPath("/Prim.attrTarget"));
    {
        ExecUsdCacheView view = execSystem.Compute(request);
        VtValue v = view.Get(1);
        TF_AXIOM(v.IsHolding<int>());
        ASSERT_EQ(v.Get<int>(), 1);
    }
}

int main()
{
    // Load test custom schemas.
    const PlugPluginPtrVector testPlugins = PlugRegistry::GetInstance()
        .RegisterPlugins(TfAbsPath("resources"));
    ASSERT_EQ(testPlugins.size(), 1);
    ASSERT_EQ(
        testPlugins[0]->GetName(), "testExecUsdConnections");

    TestAttributeConnections();
    TestConnectionsComputationNotFound();
    TestIncomingConnections();
    TestDisconnectedValueKeys();

    return 0;
}
