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

    (attr)
    (computeConstant)
    (computePrimComputation)
    (computeStringValue)
    (computeSiblingAttrComputation)
    (computeSiblingAttrValue)
    (computeViaRelTargets)
    (consumesDispatchedComputation)
    (dispatchedComputation)
    (dispatchingRel)
    (otherAttr)
    (rel)
);

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(
    TestExecUsdAttributeComputationsCustomSchema)
{
    // An attribute computation that returns a constant string.
    self.AttributeComputation(
        _tokens->attr,
        _tokens->computeConstant)
        .Callback<std::string>(+[](const VdfContext &ctx) {
            return "attribute computation result";
        });

    // An attribute computation with the same name and result type, registered
    // on a different attribute.
    self.AttributeComputation(
        _tokens->otherAttr,
        _tokens->computeConstant)
        .Callback(+[](const VdfContext &ctx) -> std::string {
            return "sibling attribute computation result";
        });

    // A prim computation with the same name and result type.
    self.PrimComputation(
        _tokens->computeConstant)
        .Callback(+[](const VdfContext &ctx) -> std::string {
            return "prim computation result";
        });

    // An attribute computation that computes the value of the owning attribute,
    // which must be string-valued.
    self.AttributeComputation(
        _tokens->attr,
        _tokens->computeStringValue)
        .Callback<std::string>(+[](const VdfContext &ctx) {
            const std::string *const valuePtr =
                ctx.GetInputValuePtr<std::string>(
                    ExecBuiltinComputations->computeValue);
            return valuePtr ? *valuePtr : "(no value)";
        })
        .Inputs(
            Computation<std::string>(ExecBuiltinComputations->computeValue)
        );

    // An attribute computation that computes the value of the prim computation.
    self.AttributeComputation(
        _tokens->attr,
        _tokens->computePrimComputation)
        .Callback(+[](const VdfContext &ctx) -> std::string {
            const std::string *const valuePtr =
                ctx.GetInputValuePtr<std::string>(_tokens->computeConstant);
            return valuePtr ? *valuePtr : "(no value)";
        })
        .Inputs(
            Prim().Computation<std::string>(_tokens->computeConstant)
        );

    // An attribute computation that computes the value of a string-valued
    // sibling attribute.
    self.AttributeComputation(
        _tokens->attr,
        _tokens->computeSiblingAttrValue)
        .Callback(+[](const VdfContext &ctx) -> std::string {
            const std::string *const valuePtr =
                ctx.GetInputValuePtr<std::string>(_tokens->otherAttr);
            return valuePtr ? *valuePtr : "(no value)";
        })
        .Inputs(
            Prim().AttributeValue<std::string>(_tokens->otherAttr)
        );

    // An attribute computation that requests the 'computeConstant' computation
    // on the sibling attribute 'otherAttr'.
    self.AttributeComputation(
        _tokens->attr,
        _tokens->computeSiblingAttrComputation)
        .Callback(+[](const VdfContext &ctx) -> std::string {
            const std::string *const valuePtr =
                ctx.GetInputValuePtr<std::string>(_tokens->computeConstant);
            return valuePtr ? *valuePtr : "(no value)";
        })
        .Inputs(
            Prim().Attribute(_tokens->otherAttr)
                .Computation<std::string>(_tokens->computeConstant)
        );

    // An attribute computation that requests the 'computeConstant' computation
    // on all objects targeted by the relationship 'rel'.
    self.AttributeComputation(
        _tokens->attr,
        _tokens->computeViaRelTargets)
        .Callback(+[](const VdfContext &ctx) -> std::string {
            std::string result;
            for (VdfReadIterator<std::string> it(ctx, _tokens->computeConstant);
                 !it.IsAtEnd(); ++it) {
                if (!result.empty()) {
                    result += " ";
                }
                result += "'" + *it + "'";
            }
            return result.empty() ? "(no value)" : result;
        })
        .Inputs(
            Prim().Relationship(_tokens->rel)
                .TargetedObjects<std::string>(_tokens->computeConstant)
        );
}

static void
TestAttributeComputations()
{
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(R"usd(#usda 1.0
        def CustomSchema "Prim" {
            string attr = "my attribute value"
            string otherAttr = "sibling attribute value"
            rel rel = [</Prim.attr>, </Prim.otherAttr>, </Prim>]
        }
    )usd");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    ExecUsdSystem execSystem(usdStage);

    UsdAttribute attr =
        usdStage->GetAttributeAtPath(SdfPath("/Prim.attr"));
    TF_AXIOM(attr.IsValid());

    std::vector<ExecUsdValueKey> valueKeys {
        {attr, _tokens->computeConstant},
        {attr, _tokens->computeStringValue},
        {attr, _tokens->computePrimComputation},
        {attr, _tokens->computeSiblingAttrValue},
        {attr, _tokens->computeSiblingAttrComputation},
        {attr, _tokens->computeViaRelTargets},
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
        TF_AXIOM(v.IsHolding<std::string>());
        ASSERT_EQ(v.Get<std::string>(), "attribute computation result");

        v = view.Get(index++);
        TF_AXIOM(v.IsHolding<std::string>());
        ASSERT_EQ(v.Get<std::string>(), "my attribute value");

        v = view.Get(index++);
        TF_AXIOM(v.IsHolding<std::string>());
        ASSERT_EQ(v.Get<std::string>(), "prim computation result");

        v = view.Get(index++);
        TF_AXIOM(v.IsHolding<std::string>());
        ASSERT_EQ(v.Get<std::string>(), "sibling attribute value");

        v = view.Get(index++);
        TF_AXIOM(v.IsHolding<std::string>());
        ASSERT_EQ(v.Get<std::string>(), "sibling attribute computation result");

        v = view.Get(index++);
        TF_AXIOM(v.IsHolding<std::string>());
        ASSERT_EQ(
            v.Get<std::string>(),
            "'attribute computation result' "
            "'sibling attribute computation result' "
            "'prim computation result'");
    }
}

// Test dispatched attribute computations.

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(
    TestExecUsdAttributeComputationsDispatchingSchema)
{
    self.DispatchedAttributeComputation(
        _tokens->dispatchedComputation)
        .Callback(+[](const VdfContext &) -> std::string {
            return "dispatched attribute computation result";
        });

    // Register a prim computation that requests the above dispatched
    // computation via a relationship.
    self.PrimComputation(_tokens->consumesDispatchedComputation)
        .Callback(+[](const VdfContext &ctx) {
            const std::string *const valuePtr =
                ctx.GetInputValuePtr<std::string>(
                    _tokens->dispatchedComputation);
            return valuePtr ? *valuePtr : "(no value)";
        })
        .Inputs(
            Relationship(_tokens->dispatchingRel)
                .TargetedObjects<std::string>(_tokens->dispatchedComputation)
                .FallsBackToDispatched()
        );
}

static void
TestDispatchedAttributeComputations()
{
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(R"usd(#usda 1.0
        def Scope "DispatchingPrim" (apiSchemas = ["DispatchingSchema"]) {
            add rel dispatchingRel = </DispatchedOnPrim.attr>
        }
        def Scope "DispatchedOnPrim" {
            string attr
        }
    )usd");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    ExecUsdSystem execSystem(usdStage);

    UsdPrim dispatchingPrim =
        usdStage->GetPrimAtPath(SdfPath("/DispatchingPrim"));
    TF_AXIOM(dispatchingPrim.IsValid());

    std::vector<ExecUsdValueKey> valueKeys {
        {dispatchingPrim,
         TfToken("consumesDispatchedComputation")},
    };

    ExecUsdRequest request = execSystem.BuildRequest(std::move(valueKeys));
    TF_AXIOM(request.IsValid());

    execSystem.PrepareRequest(request);
    TF_AXIOM(request.IsValid());

    {
        ExecUsdCacheView view = execSystem.Compute(request);

        VtValue v = view.Get(0);
        TF_AXIOM(v.IsHolding<std::string>());
        ASSERT_EQ(v.Get<std::string>(),
                  "dispatched attribute computation result");
    }
}

int main()
{
    // Load test custom schemas.
    const PlugPluginPtrVector testPlugins = PlugRegistry::GetInstance()
        .RegisterPlugins(TfAbsPath("resources"));
    ASSERT_EQ(testPlugins.size(), 1);
    ASSERT_EQ(testPlugins[0]->GetName(), "testExecUsdAttributeComputations");

    TestAttributeComputations();
    TestDispatchedAttributeComputations();

    return 0;
}
