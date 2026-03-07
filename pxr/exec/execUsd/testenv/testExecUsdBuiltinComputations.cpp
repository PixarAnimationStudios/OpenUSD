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

#include "pxr/base/plug/notice.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/exec/exec/computationBuilders.h"
#include "pxr/exec/exec/registerSchema.h"
#include "pxr/exec/exec/systemDiagnostics.h"
#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/readIteratorRange.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/namespaceEditor.h"
#include "pxr/usd/usd/prim.h"

#include <iostream>
#include <optional>

PXR_NAMESPACE_USING_DIRECTIVE;

TF_DECLARE_WEAK_PTRS(UsdStage);

#define ASSERT_EQ(expr, expected)                                              \
    [&] {                                                                      \
        std::cout << std::flush;                                               \
        std::cout << std::flush;                                               \
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
    (computeAttributePath)
    (computeConnectionPaths)
    (computeParentPrimPath)
    (computePrimPath)
    (computeRelationshipPath)
    (inputName)
);

static std::string
_PathAsString(const VdfContext &ctx)
{
    VdfReadIteratorRange<SdfPath> range(ctx, _tokens->inputName);
    SdfPathVector paths(range.begin(), range.end());
    return paths.size() == 1 ? TfStringify(paths[0]) : TfStringify(paths);
}

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(TestExecUsdBuiltinComputationsCustomSchema)
{
    self.PrimComputation(_tokens->computePrimPath)
        .Callback(&_PathAsString)
        .Inputs(
            Computation<SdfPath>(ExecBuiltinComputations->computePath)
                .InputName(_tokens->inputName)
        );

    self.PrimComputation(_tokens->computeParentPrimPath)
        .Callback(&_PathAsString)
        .Inputs(
            NamespaceAncestor<SdfPath>(ExecBuiltinComputations->computePath)
                .InputName(_tokens->inputName)
        );

    self.AttributeComputation(_tokens->attr, _tokens->computeAttributePath)
        .Callback(&_PathAsString)
        .Inputs(
            Computation<SdfPath>(ExecBuiltinComputations->computePath)
                .InputName(_tokens->inputName)
        );

    self.AttributeComputation(_tokens->attr, _tokens->computeConnectionPaths)
        .Callback(&_PathAsString)
        .Inputs(
            Connections<SdfPath>(ExecBuiltinComputations->computePath)
                .InputName(_tokens->inputName)
        );
}

class Fixture
{
public:
    ExecUsdSystem &NewSystemFromLayer(const char *const layerContents) {
        TF_AXIOM(!_system);

        const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
        layer->ImportFromString(layerContents);
        TF_AXIOM(layer);

        stage = UsdStage::Open(layer);
        TF_AXIOM(stage);
        namespaceEditor.emplace(stage);
        _system.emplace(stage);

        return *_system;
    }

    UsdStageRefPtr stage;
    std::optional<UsdNamespaceEditor> namespaceEditor;

private:
    std::optional<ExecUsdSystem> _system;
};

static void
TestComputePath(Fixture &fixture)
{
    ExecUsdSystem &system = fixture.NewSystemFromLayer(R"usd(#usda 1.0
        def Scope "Parent" {
            string customAttr
            def CustomSchema "Prim" {
                string attr.connect = [
                    </Parent.customAttr>, </Parent/Prim.customAttr>]
                string customAttr
            }
        }

        def Scope "NewParent" {
        }
    )usd");

    UsdPrim prim = fixture.stage->GetPrimAtPath(SdfPath("/Parent/Prim"));
    TF_AXIOM(prim);
    UsdAttribute attr = prim.GetAttribute(TfToken("attr"));
    TF_AXIOM(attr);
    UsdAttribute customAttr = prim.GetAttribute(TfToken("customAttr"));
    TF_AXIOM(customAttr);
    UsdAttribute parentCustomAttr =
        fixture.stage->GetAttributeAtPath(SdfPath("/Parent.customAttr"));
    TF_AXIOM(parentCustomAttr);

    ExecUsdRequest request = system.BuildRequest({
        {prim, _tokens->computePrimPath},
        {prim, _tokens->computeParentPrimPath},
        {attr, _tokens->computeAttributePath},
        {attr, _tokens->computeConnectionPaths},
    });

    const auto validateResults = [&]
    {
        ExecUsdCacheView view = system.Compute(request);
        VtValue v;
        int index = 0;

        v = view.Get(index++);
        TF_AXIOM(v.IsHolding<std::string>());
        ASSERT_EQ(v.Get<std::string>(), prim.GetPath().GetString());

        v = view.Get(index++);
        TF_AXIOM(v.IsHolding<std::string>());
        ASSERT_EQ(v.Get<std::string>(), prim.GetParent().GetPath().GetString());

        v = view.Get(index++);
        TF_AXIOM(v.IsHolding<std::string>());
        ASSERT_EQ(v.Get<std::string>(), attr.GetPath().GetString());

        // TODO: We should revisit the question of whether the correct result
        // here is the path of the connections themselves or the paths of the
        // targeted attributes, which is what we currently produce and verify
        // here.
        v = view.Get(index++);
        TF_AXIOM(v.IsHolding<std::string>());
        ASSERT_EQ(
            v.Get<std::string>(),
            TfStringify(SdfPathVector({
                parentCustomAttr.GetPath(), customAttr.GetPath()})));
    };

    validateResults();

    // Reparent the prim to a different ancestor. This is a resync on the
    // prim, but not on the original ancestor that provided the input value.
    fixture.namespaceEditor->ReparentPrim(
        prim,
        fixture.stage->GetPrimAtPath(SdfPath("/NewParent")));
    fixture.namespaceEditor->ApplyEdits();

    prim = fixture.stage->GetPrimAtPath(SdfPath("/NewParent/Prim"));
    TF_AXIOM(prim);
    attr = prim.GetAttribute(TfToken("attr"));
    TF_AXIOM(attr);
    customAttr = prim.GetAttribute(TfToken("customAttr"));
    TF_AXIOM(customAttr);

    // TODO: When we add special handling for namespace edits, instead of
    // treating them as resyncs, we won't need to re-build the request.
    request = system.BuildRequest({
        {prim, _tokens->computePrimPath},
        {prim, _tokens->computeParentPrimPath},
        {attr, _tokens->computeAttributePath},
        {attr, _tokens->computeConnectionPaths},
    });

    validateResults();
}

int main()
{
    // Load the custom schema.
    const PlugPluginPtrVector testPlugins =
        PlugRegistry::GetInstance().RegisterPlugins(TfAbsPath("resources"));
    ASSERT_EQ(testPlugins.size(), 1);
    ASSERT_EQ(testPlugins[0]->GetName(), "testExecUsdBuiltinComputations");

    const TfType customSchemaType =
        TfType::FindByName("TestExecUsdBuiltinComputationsCustomSchema");
    TF_AXIOM(!customSchemaType.IsUnknown());

    std::vector tests {
        TestComputePath,
    };
    for (const auto &test : tests) {
        Fixture fixture;
        test(fixture);
    }
}
