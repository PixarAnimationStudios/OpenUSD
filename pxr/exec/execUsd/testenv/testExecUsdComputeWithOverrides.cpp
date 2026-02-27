//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/execUsd/cacheView.h"
#include "pxr/exec/execUsd/request.h"
#include "pxr/exec/execUsd/system.h"
#include "pxr/exec/execUsd/valueKey.h"
#include "pxr/exec/execUsd/valueOverride.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/exec/exec/computationBuilders.h"
#include "pxr/exec/exec/registerSchema.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include <iostream>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE;

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

    (computeTestValue)
    (unknownComputation)
    (attr)
);

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(
    TestExecUsdComputeWithOverridesCustomSchema)
{
    // This computation recursively requests itself on the prim's namespace
    // ancestor, then appends the string value of the 'attr' attribute.
    self.PrimComputation(_tokens->computeTestValue)
        .Inputs(
            NamespaceAncestor<std::string>(_tokens->computeTestValue),
            AttributeValue<std::string>(_tokens->attr))
        .Callback(+[](const VdfContext &ctx) -> std::string {
            const static std::string emptyString;
            const std::string *const parentValue =
                ctx.GetInputValuePtr<std::string>(
                    _tokens->computeTestValue, &emptyString);
            const std::string *const attrValue =
                ctx.GetInputValuePtr<std::string>(_tokens->attr, &emptyString);
            return *parentValue + *attrValue;
        });
}

static void
TestComputeWithOverrides()
{
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(R"usd(#usda 1.0
        def CustomSchema "Root" {
            string attr = "(Root)"

            def CustomSchema "Child1" {
                string attr = "(Child1)"
            }

            def CustomSchema "Child2" {
                string attr = "(Child2)"
            }

            # This prim is not part of the exec request.
            def CustomSchema "Child3" {
                string attr = "(Child3)"
            }
        }
    )usd");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    ExecUsdSystem execSystem(usdStage);

    const UsdPrim root = usdStage->GetPrimAtPath(SdfPath("/Root"));
    const UsdAttribute rootAttr =
        usdStage->GetAttributeAtPath(SdfPath("/Root.attr"));
    const UsdPrim child1 = usdStage->GetPrimAtPath(SdfPath("/Root/Child1"));
    const UsdAttribute child1Attr =
        usdStage->GetAttributeAtPath(SdfPath("/Root/Child1.attr"));
    const UsdPrim child2 = usdStage->GetPrimAtPath(SdfPath("/Root/Child2"));
    const UsdPrim child3 = usdStage->GetPrimAtPath(SdfPath("/Root/Child3"));
    TF_AXIOM(root.IsValid());
    TF_AXIOM(rootAttr.IsValid());
    TF_AXIOM(child1.IsValid());
    TF_AXIOM(child1Attr.IsValid());
    TF_AXIOM(child2.IsValid());
    TF_AXIOM(child3.IsValid());

    ExecUsdRequest request = execSystem.BuildRequest({
        {child1, _tokens->computeTestValue},
        {child2, _tokens->computeTestValue}
    });
    TF_AXIOM(request.IsValid());

    execSystem.PrepareRequest(request);
    TF_AXIOM(request.IsValid());

    {
        // Compute the request without overrides.
        const ExecUsdCacheView view = execSystem.Compute(request);
        const VtValue v0 = view.Get(0);
        const VtValue v1 = view.Get(1);
        TF_AXIOM(v0.IsHolding<std::string>());
        TF_AXIOM(v1.IsHolding<std::string>());
        ASSERT_EQ(v0.Get<std::string>(), "(Root)(Child1)");
        ASSERT_EQ(v1.Get<std::string>(), "(Root)(Child2)");
    }
    {
        // Compute the request with an override for the Root's computation
        // result.
        ExecUsdValueOverrideVector overrides {
            {{root, _tokens->computeTestValue}, VtValue("(RootOverride)")}
        };
        const ExecUsdCacheView view =
            execSystem.ComputeWithOverrides(request, std::move(overrides));
        const VtValue v0 = view.Get(0);
        const VtValue v1 = view.Get(1);
        TF_AXIOM(v0.IsHolding<std::string>());
        TF_AXIOM(v1.IsHolding<std::string>());
        ASSERT_EQ(v0.Get<std::string>(), "(RootOverride)(Child1)");
        ASSERT_EQ(v1.Get<std::string>(), "(RootOverride)(Child2)");
    }
    {
        // Compute the request with an override for the Root's attr value.
        ExecUsdValueOverrideVector overrides {
            {ExecUsdValueKey{rootAttr}, VtValue("(RootAttrOverride)")}
        };
        const ExecUsdCacheView view =
            execSystem.ComputeWithOverrides(request, std::move(overrides));
        const VtValue v0 = view.Get(0);
        const VtValue v1 = view.Get(1);
        TF_AXIOM(v0.IsHolding<std::string>());
        TF_AXIOM(v1.IsHolding<std::string>());
        ASSERT_EQ(v0.Get<std::string>(), "(RootAttrOverride)(Child1)");
        ASSERT_EQ(v1.Get<std::string>(), "(RootAttrOverride)(Child2)");
    }
    {
        // Compute the request with an override for Child1's attr value.
        ExecUsdValueOverrideVector overrides {
            {ExecUsdValueKey{child1Attr}, VtValue("(Child1AttrOverride)")}
        };
        const ExecUsdCacheView view =
            execSystem.ComputeWithOverrides(request, std::move(overrides));
        const VtValue v0 = view.Get(0);
        const VtValue v1 = view.Get(1);
        TF_AXIOM(v0.IsHolding<std::string>());
        TF_AXIOM(v1.IsHolding<std::string>());
        ASSERT_EQ(v0.Get<std::string>(), "(Root)(Child1AttrOverride)");
        ASSERT_EQ(v1.Get<std::string>(), "(Root)(Child2)");
    }
    {
        // Compute the request with an override for Child1's computation result.
        ExecUsdValueOverrideVector overrides {
            {{child1, _tokens->computeTestValue}, VtValue("(Child1Override)")}
        };
        const ExecUsdCacheView view =
            execSystem.ComputeWithOverrides(request, std::move(overrides));
        const VtValue v0 = view.Get(0);
        const VtValue v1 = view.Get(1);
        TF_AXIOM(v0.IsHolding<std::string>());
        TF_AXIOM(v1.IsHolding<std::string>());
        // </Root/Child1> computeTestValue was overridden, so its result does
        // not have any contribution from the root prim.
        ASSERT_EQ(v0.Get<std::string>(), "(Child1Override)");
        ASSERT_EQ(v1.Get<std::string>(), "(Root)(Child2)");
    }
    {
        // Compute the request with an override for Child3's computation result.
        // Child3 is not part of the request, nor is it part of the compiled
        // network. Overriding this value should have no effect.
        ExecUsdValueOverrideVector overrides {
            {{child3, _tokens->computeTestValue}, VtValue("(Child3Override)")}
        };
        const ExecUsdCacheView view =
            execSystem.ComputeWithOverrides(request, std::move(overrides));
        const VtValue v0 = view.Get(0);
        const VtValue v1 = view.Get(1);
        TF_AXIOM(v0.IsHolding<std::string>());
        TF_AXIOM(v1.IsHolding<std::string>());
        ASSERT_EQ(v0.Get<std::string>(), "(Root)(Child1)");
        ASSERT_EQ(v1.Get<std::string>(), "(Root)(Child2)");
    }
    {
        // Compute the request with an override for a computation on Root that
        // is not defined. This should emit a coding error and not affect the
        // results.
        ExecUsdValueOverrideVector overrides {
            {{root, _tokens->unknownComputation}, VtValue("(RootOverride)")}
        };
        const TfErrorMark errorMark;
        const ExecUsdCacheView view =
            execSystem.ComputeWithOverrides(request, std::move(overrides));
        const VtValue v0 = view.Get(0);
        const VtValue v1 = view.Get(1);
        TF_AXIOM(v0.IsHolding<std::string>());
        TF_AXIOM(v1.IsHolding<std::string>());
        ASSERT_EQ(v0.Get<std::string>(), "(Root)(Child1)");
        ASSERT_EQ(v1.Get<std::string>(), "(Root)(Child2)");

        // Check for the expected coding error. It should be the only error.
        std::vector<const TfError *> errors;
        for (const TfError &error : errorMark) {
            errors.push_back(&error);
        }
        ASSERT_EQ(errors.size(), 1);
        ASSERT_EQ(errors[0]->GetErrorCode(), TF_DIAGNOSTIC_CODING_ERROR_TYPE);
        ASSERT_EQ(
            errors[0]->GetCommentary(),
            "Cannot override value for value key '/Root [unknownComputation]'"
            ", because the computation was not defined for the provider.");
    }
    {
        // Compute the request with an override for the Root's computation
        // result, but using an override value of the wrong type. This should
        // emit a coding error and not affect the results.
        ExecUsdValueOverrideVector overrides {
            {{root, _tokens->computeTestValue}, VtValue(42)}
        };
        const TfErrorMark errorMark;
        const ExecUsdCacheView view =
            execSystem.ComputeWithOverrides(request, std::move(overrides));
        const VtValue v0 = view.Get(0);
        const VtValue v1 = view.Get(1);
        TF_AXIOM(v0.IsHolding<std::string>());
        TF_AXIOM(v1.IsHolding<std::string>());
        ASSERT_EQ(v0.Get<std::string>(), "(Root)(Child1)");
        ASSERT_EQ(v1.Get<std::string>(), "(Root)(Child2)");

        // Check for the expected coding error. It should be the only error.
        std::vector<const TfError *> errors;
        for (const TfError &error : errorMark) {
            errors.push_back(&error);
        }
        ASSERT_EQ(errors.size(), 1);
        ASSERT_EQ(errors[0]->GetErrorCode(), TF_DIAGNOSTIC_CODING_ERROR_TYPE);
        ASSERT_EQ(
            errors[0]->GetCommentary(),
            "Expected override of value key '/Root [computeTestValue]' "
            "to have type 'string'; got 'int'");
    }
}

int main()
{
    // Load test custom schemas.
    const PlugPluginPtrVector testPlugins = PlugRegistry::GetInstance()
        .RegisterPlugins(TfAbsPath("resources"));
    ASSERT_EQ(testPlugins.size(), 1);
    ASSERT_EQ(testPlugins[0]->GetName(), "testExecUsdComputeWithOverrides");

    TestComputeWithOverrides();

    return 0;
}
