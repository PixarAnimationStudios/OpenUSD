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
#include "pxr/base/tf/stringUtils.h"
#include "pxr/exec/exec/builtinComputations.h"
#include "pxr/exec/exec/computationBuilders.h"
#include "pxr/exec/exec/registerSchema.h"
#include "pxr/exec/vdf/context.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/stage.h"

#include <iostream>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

#define ASSERT_EQ(expr, expected)                                              \
    [&] {                                                                      \
        auto&& expr_ = expr;                                                   \
        if (expr_ != expected) {                                               \
            std::cout << std::flush;                                           \
            std::cerr << std::flush;                                           \
            TF_FATAL_ERROR(                                                    \
                "Expected " TF_PP_STRINGIZE(expr) " == '%s'; got '%s'",        \
                TfStringify(expected).c_str(),                                 \
                TfStringify(expr_).c_str());                                   \
        }                                                                      \
    }()

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (pluginExpressionAttr)
    (authoredValue)
    (connectedValue)
    (AppliedAPI)
);

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(
    TestExecUsdAttributeExpressionsCustomSchema)
{
    // The expression for attr prepends the authored value of attr with
    // "pluginExpr:".
    self.AttributeExpression(_tokens->pluginExpressionAttr)
        .Callback<std::string>(+[](const VdfContext &ctx) {
            return "pluginExpr:" + ctx.GetInputValue<std::string>(
                ExecBuiltinComputations->computeResolvedValue);
        })
        .Inputs(
            Computation<std::string>(
                ExecBuiltinComputations->computeResolvedValue));
}

static void
TestCustomSchemaAttributeExpression()
{
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(R"usd(#usda 1.0
        def CustomSchema "PrimWithPluginAttrExpr" {
            string pluginExpressionAttr = "PrimWithPluginAttrExpr.pluginExpressionAttr"
        }
        def "PrimWithoutAttrExpr" {
            string attr = "PrimWithoutAttrExpr.attr"
        }
    )usd");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    ExecUsdSystem execSystem(usdStage);

    const UsdAttribute attrWithPluginExpr =
        usdStage->GetAttributeAtPath(
            SdfPath("/PrimWithPluginAttrExpr.pluginExpressionAttr"));
    const UsdAttribute attrWithoutExpr =
        usdStage->GetAttributeAtPath(SdfPath("/PrimWithoutAttrExpr.attr"));
    TF_AXIOM(attrWithPluginExpr.IsValid());
    TF_AXIOM(attrWithoutExpr.IsValid());

    std::vector<ExecUsdValueKey> valueKeys {
        // Computed value of attribute with a plugin expression.
        {attrWithPluginExpr, ExecBuiltinComputations->computeValue},

        // Authored value of attribute with a plugin expression.
        {attrWithPluginExpr, ExecBuiltinComputations->computeResolvedValue},

        // Computed value of attribute without a registered expression.
        {attrWithoutExpr, ExecBuiltinComputations->computeValue},

        // Authored value of attribute without a registered expression.
        {attrWithoutExpr, ExecBuiltinComputations->computeResolvedValue}
    };

    ExecUsdRequest request = execSystem.BuildRequest(std::move(valueKeys));
    TF_AXIOM(request.IsValid());

    execSystem.PrepareRequest(request);
    TF_AXIOM(request.IsValid());

    {
        const ExecUsdCacheView view = execSystem.Compute(request);
        TF_AXIOM(view.Get(0).IsHolding<std::string>());
        TF_AXIOM(view.Get(1).IsHolding<std::string>());
        TF_AXIOM(view.Get(2).IsHolding<std::string>());
        TF_AXIOM(view.Get(3).IsHolding<std::string>());
        ASSERT_EQ(
            view.Get(0).Get<std::string>(),
            "pluginExpr:PrimWithPluginAttrExpr.pluginExpressionAttr");
        ASSERT_EQ(
            view.Get(1).Get<std::string>(),
            "PrimWithPluginAttrExpr.pluginExpressionAttr");
        ASSERT_EQ(view.Get(2).Get<std::string>(), "PrimWithoutAttrExpr.attr");
        ASSERT_EQ(view.Get(3).Get<std::string>(), "PrimWithoutAttrExpr.attr");
    }
}

int main()
{
    // Load test custom schemas.
    const PlugPluginPtrVector testPlugins = PlugRegistry::GetInstance()
        .RegisterPlugins(TfAbsPath("resources"));
    ASSERT_EQ(testPlugins.size(), 1);
    ASSERT_EQ(testPlugins[0]->GetName(), "testExecUsdAttributeExpressions");

    TestCustomSchemaAttributeExpression();

    return 0;
}
