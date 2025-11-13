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
#include "pxr/base/tf/preprocessorUtilsLite.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/exec/exec/registerSchema.h"
#include "pxr/exec/exec/typeRegistry.h"
#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/readIterator.h"
#include "pxr/usd/sdf/layer.h"
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

TF_REGISTRY_FUNCTION(ExecTypeRegistry)
{
    ExecTypeRegistry::RegisterType(std::vector<GfVec3f>{});
}

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (computeDoubleAttr)
    (computeStringAttr)
    (computeArrayAttr)
    (doubleAttr)
    (stringAttr)
    (arrayAttr)
);

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(
    TestExecUsdAttributeValueInputCustomSchema)
{
    self.PrimComputation(_tokens->computeDoubleAttr)
        .Callback<double>(+[](const VdfContext &ctx) {
            return ctx.GetInputValue<double>(_tokens->doubleAttr);
        })
        .Inputs(
            AttributeValue<double>(_tokens->doubleAttr)
        );

    self.PrimComputation(_tokens->computeStringAttr)
        .Callback<std::string>(+[](const VdfContext &ctx) {
            return ctx.GetInputValue<std::string>(_tokens->stringAttr);
        })
        .Inputs(
            AttributeValue<std::string>(_tokens->stringAttr)
        );

    self.PrimComputation(_tokens->computeArrayAttr)
        .Callback<std::vector<GfVec3f>>(+[](const VdfContext &ctx) {
            VdfReadIterator<GfVec3f> rIt(ctx, _tokens->arrayAttr);

            std::vector<GfVec3f> result;
            result.reserve(rIt.ComputeSize());
            for (size_t i = 0; !rIt.IsAtEnd(); ++rIt, ++i) {
                result.push_back(*rIt);
            }

            return result;
        })
        .Inputs(
            AttributeValue<GfVec3f>(_tokens->arrayAttr)
        );
}

// Test the attribute inputs yield the expected values.
static void
TestAttributeValueInput()
{
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(R"usd(#usda 1.0
        def CustomSchema "Prim" {
            double doubleAttr = 1.0
            string stringAttr = "a string"
            Vec3f[] arrayAttr = [(0, 0, 0), (0, 1, 0), (1, 1, 1)]
        }
    )usd");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    ExecUsdSystem execSystem(usdStage);

    UsdPrim prim = usdStage->GetPrimAtPath(SdfPath("/Prim"));
    TF_AXIOM(prim.IsValid());

    ExecUsdRequest request = execSystem.BuildRequest({
        {prim, _tokens->computeDoubleAttr},
        {prim, _tokens->computeStringAttr},
        {prim, _tokens->computeArrayAttr},
    });

    TF_AXIOM(request.IsValid());

    ExecUsdCacheView view = execSystem.Compute(request);
    VtValue v;

    v = view.Get(0);
    TF_AXIOM(v.IsHolding<double>());
    ASSERT_EQ(v.Get<double>(), 1.0);

    v = view.Get(1);
    TF_AXIOM(v.IsHolding<std::string>());
    ASSERT_EQ(v.Get<std::string>(), "a string");

    v = view.Get(2);
    TF_AXIOM(v.IsHolding<std::vector<GfVec3f>>());
    const std::vector<GfVec3f> expected{
        GfVec3f(0, 0, 0),
        GfVec3f(0, 1, 0),
        GfVec3f(1, 1, 1)};
    const std::vector<GfVec3f> &computed = v.Get<std::vector<GfVec3f>>();
    ASSERT_EQ(computed.size(), expected.size());
    for (size_t i = 0; i < computed.size(); ++i) {
        ASSERT_EQ(computed[i], expected[i]);
    }
}

int main()
{
    // Load test custom schemas.
    const PlugPluginPtrVector testPlugins = PlugRegistry::GetInstance()
        .RegisterPlugins(TfAbsPath("resources"));
    ASSERT_EQ(testPlugins.size(), 1);
    ASSERT_EQ(testPlugins[0]->GetName(), "testExecUsdAttributeValueInput");

    TestAttributeValueInput();

    return 0;
}
