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
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/exec/exec/registerSchema.h"
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

namespace {

// Structure for keeping track of invalidation state as received from request
// callback invocations.
// 
struct _InvalidationState {
    // Map from invalid index to number of times invalidated.
    std::unordered_map<int, int> indices;

    // Number of times the callback has been invoked.
    int numInvoked = 0;

    // Reset the invalidation state.
    void Reset() {
        indices.clear();
        numInvoked = 0;
    }

    // The value invalidation callback invoked by the request.
    void ValueCalback(
        const ExecRequestIndexSet &invalidIndices,
        const EfTimeInterval &invalidInterval)
    {
        // Add all invalid indices to the map and increment the invalidation
        // count for each entry.
        for (const int i : invalidIndices) {
            auto [it, emplaced] = indices.emplace(i, 0);
            ++it->second;
        }

        // Increment the number of times the callback has been invoked.
        ++numInvoked;
    }
};

}

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (attr)
    (bogusMetadataKey)
    (computeBogusMetadata)
    (computeDocumentationMetadata)
    (computePrimDocumentationMetadata)
);

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(
    TestExecUsdMetadataInputCustomSchema)
{
    // A prim computation that computes the value of the 'documentation'
    // metadata.
    self.PrimComputation(
        _tokens->computeDocumentationMetadata)
        .Callback<std::string>(+[](const VdfContext &ctx) {
            const std::string *const valuePtr =
                ctx.GetInputValuePtr<std::string>(SdfFieldKeys->Documentation);
            return valuePtr ? *valuePtr : "(no value)";
        })
        .Inputs(
            Metadata<std::string>(SdfFieldKeys->Documentation)
        );

    // An attribute computation that computes the value of the 'documentation'
    // metadata on the attribute.
    self.AttributeComputation(
        _tokens->attr,
        _tokens->computeDocumentationMetadata)
        .Callback<std::string>(+[](const VdfContext &ctx) {
            const std::string *const valuePtr =
                ctx.GetInputValuePtr<std::string>(SdfFieldKeys->Documentation);
            return valuePtr ? *valuePtr : "(no value)";
        })
        .Inputs(
            Metadata<std::string>(SdfFieldKeys->Documentation)
        );

    // An attribute computation that computes the value of the 'documentation'
    // metadata on the owning prim.
    self.AttributeComputation(
        _tokens->attr,
        _tokens->computePrimDocumentationMetadata)
        .Callback<std::string>(+[](const VdfContext &ctx) {
            const std::string *const valuePtr =
                ctx.GetInputValuePtr<std::string>(SdfFieldKeys->Documentation);
            return valuePtr ? *valuePtr : "(no value)";
        })
        .Inputs(
            Prim().Metadata<std::string>(SdfFieldKeys->Documentation)
        );

    //
    // Error cases
    //

    // A prim computation that attempts to compute metadata using an invalid
    // metadata key.
    self.PrimComputation(
        _tokens->computeBogusMetadata)
        .Callback<std::string>(+[](const VdfContext &ctx) {
            const std::string *const valuePtr =
                ctx.GetInputValuePtr<std::string>(_tokens->bogusMetadataKey);
            return valuePtr ? *valuePtr : "(no value)";
        })
        .Inputs(
            Metadata<std::string>(_tokens->bogusMetadataKey)
        );

}

// Test the metadata computation input yield the expected values.
static void
TestMetadataBasic()
{
    _InvalidationState invalidation;

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
        {prim, _tokens->computeDocumentationMetadata},
        {attr, _tokens->computeDocumentationMetadata},
        {attr, _tokens->computePrimDocumentationMetadata},
    };

    ExecUsdRequest request = execSystem.BuildRequest(
        std::move(valueKeys),
        std::bind(
            &_InvalidationState::ValueCalback, &invalidation,
            std::placeholders::_1,
            std::placeholders::_2));
    TF_AXIOM(request.IsValid());

    execSystem.PrepareRequest(request);
    TF_AXIOM(request.IsValid());

    const auto TestValues =
        [&execSystem, &request, &invalidation, &prim, &attr]
        (int numInvalidations) {

        ExecUsdCacheView view = execSystem.Compute(request);
        ASSERT_EQ(invalidation.numInvoked, numInvalidations);
        VtValue v;
        int index = 0;

        v = view.Get(index++);
        TF_AXIOM(v.IsHolding<std::string>());
        std::string primDocValue;
        TF_AXIOM(prim.GetMetadata(SdfFieldKeys->Documentation, &primDocValue));
        TF_AXIOM(!primDocValue.empty());
        ASSERT_EQ(v.Get<std::string>(), primDocValue);

        v = view.Get(index++);
        TF_AXIOM(v.IsHolding<std::string>());
        std::string attrDocValue;
        TF_AXIOM(attr.GetMetadata(SdfFieldKeys->Documentation, &attrDocValue));
        TF_AXIOM(!attrDocValue.empty());
        ASSERT_EQ(v.Get<std::string>(), attrDocValue);

        v = view.Get(index++);
        TF_AXIOM(v.IsHolding<std::string>());
        ASSERT_EQ(v.Get<std::string>(), primDocValue);
    };

    TestValues(/* numInvalidations */ 0);
    invalidation.Reset();

    // Author new metadata values and re-compute.
    prim.SetMetadata(SdfFieldKeys->Documentation, "new prim doc");
    attr.SetMetadata(SdfFieldKeys->Documentation, "new attribute doc");
    TestValues(/* numInvalidations */ 2);
    invalidation.Reset();
}

// Test error cases involving metadata computation inputs.
static void
TestMetadataErrorCases()
{
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(R"usd(#usda 1.0
        def CustomSchema "Prim" (
            doc = "prim documentation"
        ) {
        }
    )usd");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    ExecUsdSystem execSystem(usdStage);

    UsdPrim prim = usdStage->GetPrimAtPath(SdfPath("/Prim"));
    TF_AXIOM(prim.IsValid());

    std::vector<ExecUsdValueKey> valueKeys {
        {prim, _tokens->computeBogusMetadata},
    };

    ExecUsdRequest request = execSystem.BuildRequest(std::move(valueKeys));
    TF_AXIOM(request.IsValid());

    {
        TfErrorMark mark;
        execSystem.PrepareRequest(request);
        TF_AXIOM(request.IsValid());
        TF_AXIOM(!mark.IsClean());
    }

    {
        ExecUsdCacheView view = execSystem.Compute(request);
        VtValue v;
        int index = 0;

        v = view.Get(index++);
        TF_AXIOM(v.IsHolding<std::string>());
        ASSERT_EQ(v.Get<std::string>(), "(no value)");
    }
}

int main()
{
    // Load test custom schemas.
    const PlugPluginPtrVector testPlugins = PlugRegistry::GetInstance()
        .RegisterPlugins(TfAbsPath("resources"));
    ASSERT_EQ(testPlugins.size(), 1);
    ASSERT_EQ(testPlugins[0]->GetName(), "testExecUsdMetadataInput");

    TestMetadataBasic();
    TestMetadataErrorCases();

    return 0;
}
