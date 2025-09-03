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
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/stage.h"

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

    (convertibleReturnTypeComputation)
    (functionPointerComputation)
    (returnTypeLambdaComputation)
    (voidFunctionPointerComputation)
    (voidLambdaComputation)
);

static double
_CallbackFunction(const VdfContext &) {
    return 3.0;
}

static void
_CallbackFunctionVoidReturn(const VdfContext &ctx) {
    ctx.SetOutput<double>(4.0);
}

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(
    TestExecUsdCallbacksCustomSchema)
{
    //
    // Test different kinds of callback functions.
    //

    /// Callback: a lambda that returns the deduced result type
    self.PrimComputation(_tokens->returnTypeLambdaComputation)
        .Callback(+[](const VdfContext &) {
            return 1;
        });

    // Callback: a lambda that returns void
    self.PrimComputation(
        _tokens->voidLambdaComputation)
        .Callback<float>(+[](const VdfContext &ctx) {
            ctx.SetOutput(2.0f);
        });

    // Callback: a pointer to a function where the return type is the deduced
    // computation result type
    self.PrimComputation(
        _tokens->functionPointerComputation)
        .Callback(_CallbackFunction);

    // Callback: a pointer to a function that returns void
    self.PrimComputation(
        _tokens->voidFunctionPointerComputation)
        .Callback<double>(_CallbackFunctionVoidReturn);

    // Callback: a lambda that returns a type that is convertible to the
    // computation result type
    self.PrimComputation(
        _tokens->convertibleReturnTypeComputation)
        .Callback<std::string>(+[](const VdfContext &ctx) {
            return "string result value";
        });
}

// Test different kinds of callback functions to make sure they compile
// correctly and produce the expected values.
// 
static void
TestCallbackFunctions()
{
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(R"usd(#usda 1.0
        def CustomSchema "Prim" (
            doc = "prim documentation"
        ) {
            int attr (doc = "attribute documentation")
        }
    )usd");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    ExecUsdSystem execSystem(usdStage);

    UsdPrim prim = usdStage->GetPrimAtPath(SdfPath("/Prim"));
    TF_AXIOM(prim.IsValid());
    UsdAttribute attr = usdStage->GetAttributeAtPath(SdfPath("/Prim.attr"));
    TF_AXIOM(attr.IsValid());

    std::vector<ExecUsdValueKey> valueKeys {
        {prim, _tokens->returnTypeLambdaComputation},
        {prim, _tokens->voidLambdaComputation},
        {prim, _tokens->functionPointerComputation},
        {prim, _tokens->voidFunctionPointerComputation},
        {prim, _tokens->convertibleReturnTypeComputation},
    };

    ExecUsdRequest request = execSystem.BuildRequest(std::move(valueKeys));
    TF_AXIOM(request.IsValid());

    execSystem.PrepareRequest(request);
    TF_AXIOM(request.IsValid());

    {
        ExecUsdCacheView view = execSystem.Compute(request);
        VtValue v;
        int index = 0;

        v = view.Get(index++);
        TF_AXIOM(v.IsHolding<int>());
        ASSERT_EQ(v.Get<int>(), 1);

        v = view.Get(index++);
        TF_AXIOM(v.IsHolding<float>());
        ASSERT_EQ(v.Get<float>(), 2.0);

        v = view.Get(index++);
        TF_AXIOM(v.IsHolding<double>());
        ASSERT_EQ(v.Get<double>(), 3.0);

        v = view.Get(index++);
        TF_AXIOM(v.IsHolding<double>());
        ASSERT_EQ(v.Get<double>(), 4.0);

        v = view.Get(index++);
        TF_AXIOM(v.IsHolding<std::string>());
        ASSERT_EQ(v.Get<std::string>(), "string result value");
    }
}

int main()
{
    // Load test custom schemas.
    const PlugPluginPtrVector testPlugins = PlugRegistry::GetInstance()
        .RegisterPlugins(TfAbsPath("resources"));
    ASSERT_EQ(testPlugins.size(), 1);
    ASSERT_EQ(testPlugins[0]->GetName(), "testExecUsdCallbacks");

    TestCallbackFunctions();

    return 0;
}
