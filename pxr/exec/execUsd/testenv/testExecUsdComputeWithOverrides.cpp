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
#include <utility>

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

    (attr)
    (computeViaRelationship)
    (computeTestValue)
    (relationship)
    (unknownComputation)
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

    // This computation requests itself on a single target of the 'relationship'
    // relationship, then appends the value of the 'attr' attribute, if present,
    // or the owning prim's name. The appended strings are separated by a space.
    self.PrimComputation(_tokens->computeViaRelationship)
        .Inputs(
            Relationship(_tokens->relationship)
               .TargetedObjects<std::string>(_tokens->computeViaRelationship),
            AttributeValue<std::string>(_tokens->attr),
            Computation<SdfPath>(ExecBuiltinComputations->computePath))
        .Callback(+[](const VdfContext &ctx) -> std::string {
            const std::string *const attrValue =
                ctx.GetInputValuePtr<std::string>(_tokens->attr);
            const std::string &name =
                attrValue
                ? *attrValue
                : ctx.GetInputValue<SdfPath>(
                    ExecBuiltinComputations->computePath).GetName();
            const std::string *const relValue =
                ctx.GetInputValuePtr<std::string>(
                    _tokens->computeViaRelationship);
            return relValue ? *relValue + ' ' + name : name;
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

// Test that ComputeWithOverrides produces correct result when scene edits have
// happened that require invalidation to be processed before computation.
//
static void
TestInvalidation()
{
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(R"usd(#usda 1.0
        def Scope "Root" {
            def CustomSchema "Child1" {
                custom rel relationship = </Root/Child2>
            }

            def CustomSchema "Child2" {
                custom rel relationship = </Root/Child3>
                string attr = "Child2.attr"
            }

            def CustomSchema "Child3" {
            }
        }
    )usd");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    const UsdPrim child1 = usdStage->GetPrimAtPath(SdfPath("/Root/Child1"));
    const UsdPrim child3 = usdStage->GetPrimAtPath(SdfPath("/Root/Child3"));
    TF_AXIOM(child1 && child3);

    const UsdAttribute child2Attr =
        usdStage->GetAttributeAtPath(SdfPath("/Root/Child2.attr"));
    TF_AXIOM(child2Attr);

    ExecUsdSystem execSystem(usdStage);
    ExecUsdRequest request = execSystem.BuildRequest({
        {child1, _tokens->computeViaRelationship},
    });
    TF_AXIOM(request.IsValid());

    // Do a Compute, causing the network to be recompiled, and populating output
    // caches with computed values.
    {
        const ExecUsdCacheView view = execSystem.Compute(request);
        ASSERT_EQ(view.Get(0).Get<std::string>(),
                  "Child3 Child2.attr Child1");
    }

    // Author an attribute value that the following ComputeWithOverrides call
    // depends on.
    child2Attr.Set("AuthoredValue");

    // Call ComputeWithOverrides, providing an override that doesn't mask the
    // value of the attribute we just authored, so that the results allow us to
    // confirm that invalidation due to the above authoring is processed,
    // invalidating caches from the above call to Compute.
    {
        ExecUsdValueOverrideVector overrides {
            {{child3, _tokens->computeViaRelationship},
             VtValue("Child3Override")}
        };

        const ExecUsdCacheView view =
            execSystem.ComputeWithOverrides(request, std::move(overrides));
        ASSERT_EQ(view.Get(0).Get<std::string>(),
                  "Child3Override AuthoredValue Child1");
    }
}

// Test ComputeWithOverrides when uncompilation makes topological changes to the
// network that causes structural invalidation caches to become stale.
//
// This provides coverage for a bug that caused ComputeWithOverrides to produce
// incorrect results in this situation.
//
static void
TestTopologicalUpdate()
{
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(R"usd(#usda 1.0
        def Scope "Root" {
            def CustomSchema "Child1" {
                custom rel relationship = </Root/Child2>
            }

            def CustomSchema "Child2" {
                custom rel relationship = </Root/Child3>
            }

            def CustomSchema "Child3" {
            }
        }
    )usd");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    const UsdPrim child1 = usdStage->GetPrimAtPath(SdfPath("/Root/Child1"));
    const UsdPrim child2 = usdStage->GetPrimAtPath(SdfPath("/Root/Child2"));
    const UsdPrim child3 = usdStage->GetPrimAtPath(SdfPath("/Root/Child3"));
    TF_AXIOM(child1 && child2 && child3);

    ExecUsdSystem execSystem(usdStage);
    ExecUsdRequest request = execSystem.BuildRequest({
        {child1, _tokens->computeViaRelationship},
    });
    TF_AXIOM(request.IsValid());

    // Do an initial ComputeWithOverrides.
    // 
    // The executor used to do the evaluation will cache data that is used to
    // accelerate subsequent invalidation traversals.
    {
        ExecUsdValueOverrideVector overrides {
            {{child3, _tokens->computeViaRelationship},
             VtValue("Child3Override")}
        };

        const ExecUsdCacheView view =
            execSystem.ComputeWithOverrides(request, std::move(overrides));
        ASSERT_EQ(view.Get(0).Get<std::string>(),
                  "Child3Override Child2 Child1");
    }

    // Toggle 'active' on Child2 so we uncompile nodes that are visited in the
    // invalidation traversal done as part of the call to ComputeWithOverrides
    // above.
    child2.SetActive(false);
    child2.SetActive(true);

    // Do a Compute, causing the network to be recompiled, and populating output
    // caches with new computed values.
    {
        const ExecUsdCacheView view = execSystem.Compute(request);
        ASSERT_EQ(view.Get(0).Get<std::string>(),
                  "Child3 Child2 Child1");
    }

    // Call ComputeWithOverrides again.
    //
    // In order to produce the correct result, this must invalidate all outputs
    // that depend on the overridden output. If the invalidation caches used to
    // perform that invalidation traversal are stale, we can fail to visit all
    // outputs, returning results from the Compute call above.
    {
        ExecUsdValueOverrideVector overrides {
            {{child3, _tokens->computeViaRelationship},
             VtValue("Child3Override")}
        };

        const ExecUsdCacheView view =
            execSystem.ComputeWithOverrides(request, std::move(overrides));
        ASSERT_EQ(view.Get(0).Get<std::string>(),
                  "Child3Override Child2 Child1");
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
    TestInvalidation();
    TestTopologicalUpdate();

    return 0;
}
