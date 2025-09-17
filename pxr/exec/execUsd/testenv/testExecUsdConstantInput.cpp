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
#include "pxr/base/tf/preprocessorUtilsLite.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/exec/exec/registerSchema.h"
#include "pxr/exec/exec/systemDiagnostics.h"
#include "pxr/exec/exec/typeRegistry.h"
#include "pxr/exec/vdf/context.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/usd/stage.h"

#include <functional>
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

struct CustomType {
    int i;
    std::string s;

    friend
    bool operator==(const CustomType &a, const CustomType &b) {
        return a.i == b.i && a.s == b.s;
    }

    template <typename HashState>
    friend
    void TfHashAppend(HashState &h, const CustomType &s) {
        h.Append(s.i, s.s);
    }
};

TF_REGISTRY_FUNCTION(ExecTypeRegistry)
{
    ExecTypeRegistry::RegisterType(CustomType{});
}

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (computeConstantCustomType)
    (computeConstantDouble)
    (computeConstantString)
    (computeMultipleConstants)
    (constantCustomType)
    (constantDouble)
    (constantString)
);

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(
    TestExecUsdConstantInputCustomSchema)
{
    self.PrimComputation(_tokens->computeConstantDouble)
        .Callback<double>(+[](const VdfContext &ctx) {
            return ctx.GetInputValue<double>(_tokens->constantDouble);
        })
        .Inputs(
            Constant(1.0).InputName(_tokens->constantDouble)
        );

    self.PrimComputation(_tokens->computeConstantString)
        .Callback<std::string>(+[](const VdfContext &ctx) {
            return ctx.GetInputValue<std::string>(_tokens->constantString);
        })
        .Inputs(
            Constant("a string").InputName(_tokens->constantString)
        );

    self.PrimComputation(_tokens->computeConstantCustomType)
        .Callback<CustomType>(+[](const VdfContext &ctx) {
            return ctx.GetInputValue<CustomType>(_tokens->constantCustomType);
        })
        .Inputs(
            Constant(CustomType{-1, "a string"})
                .InputName(_tokens->constantCustomType)
        );

    self.PrimComputation(_tokens->computeMultipleConstants)
        .Callback<std::string>(+[](const VdfContext &ctx) {
            std::string result;

            result +=
                TfStringify(ctx.GetInputValue<double>(_tokens->constantDouble));
            result += "\n";
            
            result += ctx.GetInputValue<std::string>(_tokens->constantString);
            result += "\n";

            const CustomType &value =
                ctx.GetInputValue<CustomType>(_tokens->constantCustomType);
            result += TfStringPrintf("%d %s", value.i, value.s.c_str());

            return result;
        })
        .Inputs(
            Constant(1.0).InputName(_tokens->constantDouble),
            Constant("a string").InputName(_tokens->constantString),
            Constant(CustomType{-1, "a string"})
                .InputName(_tokens->constantCustomType)
        );
}

// Test the metadata computation input yield the expected values.
static void
TestConstantInput()
{
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(R"usd(#usda 1.0
        def CustomSchema "Prim" {
        }
    )usd");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    ExecUsdSystem execSystem(usdStage);

    UsdPrim prim = usdStage->GetPrimAtPath(SdfPath("/Prim"));
    TF_AXIOM(prim.IsValid());

    ExecUsdRequest request = execSystem.BuildRequest({
        {prim, _tokens->computeConstantDouble},
        {prim, _tokens->computeConstantString},
        {prim, _tokens->computeConstantCustomType},
        {prim, _tokens->computeMultipleConstants},
    });

    TF_AXIOM(request.IsValid());

    ExecUsdCacheView view = execSystem.Compute(request);
    VtValue v;
    int index = 0;

    ExecSystem::Diagnostics(&execSystem).GraphNetwork("testConstantInput.dot");

    v = view.Get(index++);
    TF_AXIOM(v.IsHolding<double>());
    ASSERT_EQ(v.Get<double>(), 1.0);

    v = view.Get(index++);
    TF_AXIOM(v.IsHolding<std::string>());
    ASSERT_EQ(v.Get<std::string>(), "a string");

    v = view.Get(index++);
    TF_AXIOM(v.IsHolding<CustomType>());
    ASSERT_EQ(v.Get<CustomType>().i, -1);
    ASSERT_EQ(v.Get<CustomType>().s, "a string");

    v = view.Get(index++);
    TF_AXIOM(v.IsHolding<std::string>());
    ASSERT_EQ(v.Get<std::string>(),
              "1\n"
              "a string\n"
              "-1 a string");
}

int main()
{
    // Load test custom schemas.
    const PlugPluginPtrVector testPlugins = PlugRegistry::GetInstance()
        .RegisterPlugins(TfAbsPath("resources"));
    ASSERT_EQ(testPlugins.size(), 1);
    ASSERT_EQ(testPlugins[0]->GetName(), "testExecUsdConstantInput");

    TestConstantInput();

    return 0;
}
