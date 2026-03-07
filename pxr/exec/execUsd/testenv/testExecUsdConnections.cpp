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
    (computeViaConnections)
    (computeConnectedConstants)
    (computeConstant)
);

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(
    TestExecUsdConnectionsCustomSchema)
{
    // An attribute computation that computes the values of the string-valued
    // attributes targeted by the attribute's connections.
    self.AttributeComputation(
        _tokens->attr,
        _tokens->computeViaConnections)
        .Callback(+[](const VdfContext &ctx) -> std::string {
            std::string result;
            for (VdfReadIterator<std::string> it(
                     ctx, ExecBuiltinComputations->computeValue);
                 !it.IsAtEnd(); ++it) {
                if (!result.empty()) {
                    result += " ";
                }
                result += "'" + *it + "'";
            }
            return result.empty() ? "(no value)" : result;
        })
        .Inputs(
            Connections<std::string>(ExecBuiltinComputations->computeValue)
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
}

static void
TestAttributeConnections()
{
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(R"usd(#usda 1.0
        def CustomSchema "Prim" {
            string attr = "attr value"
            string attr.connect = [</Prim.attr>, </Prim.attr2>]
            string attr3 = "attr3 value"
        }
    )usd");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    ExecUsdSystem execSystem(usdStage);

    UsdPrim prim = usdStage->GetPrimAtPath(SdfPath("/Prim"));
    TF_AXIOM(prim.IsValid());

    UsdAttribute attr = usdStage->GetAttributeAtPath(SdfPath("/Prim.attr"));
    TF_AXIOM(attr.IsValid());

    ExecUsdRequest request = execSystem.BuildRequest({
        {attr, _tokens->computeViaConnections}});
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

    // Add a connection to an existing attribute.
    attr.AddConnection(SdfPath("/Prim.attr3"));

    {
        ExecUsdCacheView view = execSystem.Compute(request);
        VtValue v = view.Get(0);
        TF_AXIOM(v.IsHolding<std::string>());
        ASSERT_EQ(
            v.Get<std::string>(),
            "'attr value' "
            "'attr3 value'");
    }

    // Create attr2, which is already targeted by an attribute connection.
    UsdAttribute attr2 = prim.CreateAttribute(
        TfToken("attr2"),
        SdfValueTypeNames->String,
        /* custom */ true);
    attr2.Set("attr2 value");

    {
        ExecUsdCacheView view = execSystem.Compute(request);
        VtValue v = view.Get(0);
        TF_AXIOM(v.IsHolding<std::string>());
        ASSERT_EQ(
            v.Get<std::string>(),
            "'attr value' "
            "'attr2 value' "
            "'attr3 value'");
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

    const UsdAttribute attr = usdStage->GetAttributeAtPath(SdfPath("/Prim.attr"));
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

    return 0;
}
