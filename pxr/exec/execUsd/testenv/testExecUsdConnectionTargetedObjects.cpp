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
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include <iostream>
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
);

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(
    TestExecUsdConnectionTargetedObjectsCustomSchema)
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
            ConnectionTargetedObjects<std::string>(
                ExecBuiltinComputations->computeValue)
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

int main()
{
    // Load test custom schemas.
    const PlugPluginPtrVector testPlugins = PlugRegistry::GetInstance()
        .RegisterPlugins(TfAbsPath("resources"));
    ASSERT_EQ(testPlugins.size(), 1);
    ASSERT_EQ(
        testPlugins[0]->GetName(), "testExecUsdConnectionTargetedObjects");

    TestAttributeConnections();

    return 0;
}
