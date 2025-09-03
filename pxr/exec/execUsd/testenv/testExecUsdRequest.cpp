//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/execUsd/cacheView.h"
#include "pxr/exec/execUsd/request.h"
#include "pxr/exec/execUsd/system.h"
#include "pxr/exec/execUsd/valueKey.h"

#include "pxr/exec/ef/time.h"
#include "pxr/exec/exec/builtinComputations.h"
#include "pxr/exec/exec/registerSchema.h"
#include "pxr/exec/exec/typeRegistry.h"
#include "pxr/exec/vdf/context.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/work/loops.h"
#include "pxr/usd/usd/timeCode.h"

#include <atomic>

PXR_NAMESPACE_USING_DIRECTIVE

#define ASSERT_EQ(expr, expected)                                       \
    [&] {                                                               \
        auto&& expr_ = expr;                                            \
        if (expr_ != expected) {                                        \
            TF_FATAL_ERROR(                                             \
                "Expected " TF_PP_STRINGIZE(expr) " == '%s'; got '%s'", \
                TfStringify(expected).c_str(),                          \
                TfStringify(expr_).c_str());                            \
        }                                                               \
     }()

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (computeXf)
    (computeTimeVarying)
    (xf)
    );

TF_REGISTRY_FUNCTION(ExecTypeRegistry)
{
    ExecTypeRegistry::RegisterType(UsdTimeCode::Default());
}

static std::atomic<int> NumComputed{0};

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(TestExecUsdRequestComputedTransform)
{
    self.PrimComputation(_tokens->computeXf)
        .Callback(+[](const VdfContext &ctx) {
            const GfMatrix4d id(1);
            const GfMatrix4d &xf = *ctx.GetInputValuePtr<GfMatrix4d>(
                _tokens->xf, &id);
            const GfMatrix4d &parentXf = *ctx.GetInputValuePtr<GfMatrix4d>(
                _tokens->computeXf, &id);
            return xf * parentXf;
        })
        .Inputs(
            AttributeValue<GfMatrix4d>(_tokens->xf),
            NamespaceAncestor<GfMatrix4d>(_tokens->computeXf)
        );

    self.PrimComputation(_tokens->computeTimeVarying)
        .Callback(+[](const VdfContext &ctx) {
            ++NumComputed;
            const UsdTimeCode tc = ctx.GetInputValue<EfTime>(
                ExecBuiltinComputations->computeTime).GetTimeCode();
            return tc;
        })
        .Inputs(
            Stage().Computation<EfTime>(ExecBuiltinComputations->computeTime)
        );
}

static void
ConfigureTestPlugin()
{
    const PlugPluginPtrVector testPlugins = PlugRegistry::GetInstance()
        .RegisterPlugins(TfAbsPath("resources"));

    TF_AXIOM(testPlugins.size() == 1);
    TF_AXIOM(testPlugins[0]->GetName() == "testExecUsdRequest");
}

static UsdStageRefPtr
CreateTestStage()
{
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    const bool importedLayer = layer->ImportFromString(
        R"usda(#usda 1.0
        (
            defaultPrim = "Root"
        )
        def ComputedTransform "Root" (
            kind = "component"
        )
        {
            def ComputedTransform "A1"
            {
                matrix4d xf = ( (2, 0, 0, 0), (0, 2, 0, 0), (0, 0, 2, 0), (0, 0, 0, 1) )
                def ComputedTransform "B"
                {
                    matrix4d xf = ( (3, 0, 0, 0), (0, 3, 0, 0), (0, 0, 3, 0), (0, 0, 0, 1) )
                }
            }
            def ComputedTransform "A2"
            {
                matrix4d xf = ( (5, 0, 0, 0), (0, 5, 0, 0), (0, 0, 5, 0), (0, 0, 0, 1) )
            }
            def ComputedTransform "A3"
            {
                matrix4d xf = ( (7, 0, 0, 0), (0, 7, 0, 0), (0, 0, 7, 0), (0, 0, 0, 1) )
                def ComputedTransform "B"
                {
                    matrix4d xf = ( (3, 0, 0, 0), (0, 3, 0, 0), (0, 0, 3, 0), (0, 0, 0, 1) )
                }
            }
        }
        )usda");
    TF_AXIOM(importedLayer);

    UsdStageRefPtr stage = UsdStage::Open(layer);
    TF_AXIOM(stage);
    return stage;
}

static void
TestValueExtraction()
{
    UsdStageRefPtr stage = CreateTestStage();

    ExecUsdSystem system(stage);

    ExecUsdRequest request = system.BuildRequest({
        {stage->GetPrimAtPath(SdfPath("/Root")), _tokens->computeXf},
        {stage->GetPrimAtPath(SdfPath("/Root/A1")), _tokens->computeXf},
        {stage->GetPrimAtPath(SdfPath("/Root/A1/B")), _tokens->computeXf},
        {stage->GetPrimAtPath(SdfPath("/Root/A2")), _tokens->computeXf},
        {stage->GetPrimAtPath(SdfPath("/Root/A3/B")), _tokens->computeXf},
    });
    TF_AXIOM(request.IsValid());

    system.PrepareRequest(request);
    TF_AXIOM(request.IsValid());

    ExecUsdCacheView view = system.Compute(request);

    // Extract values concurrently and repeatedly from the same index.
    WorkParallelForN(
        12345,
        [view](int i, int n) {
            for (; i!=n; ++i) {
                VtValue v = view.Get(i%4);
                TF_AXIOM(!v.IsEmpty());
            }
        });

    // Assert that the request values are as expected.
    VtValue v = view.Get(0);
    TF_AXIOM(!v.IsEmpty());
    TF_AXIOM(v.IsHolding<GfMatrix4d>());
    ASSERT_EQ(v.Get<GfMatrix4d>(), GfMatrix4d(1.));

    v = view.Get(1);
    TF_AXIOM(!v.IsEmpty());
    TF_AXIOM(v.IsHolding<GfMatrix4d>());
    ASSERT_EQ(v.Get<GfMatrix4d>(), GfMatrix4d(1.).SetScale(2.));

    v = view.Get(2);
    TF_AXIOM(!v.IsEmpty());
    TF_AXIOM(v.IsHolding<GfMatrix4d>());
    ASSERT_EQ(v.Get<GfMatrix4d>(), GfMatrix4d(1.).SetScale(6.));

    v = view.Get(3);
    TF_AXIOM(!v.IsEmpty());
    TF_AXIOM(v.IsHolding<GfMatrix4d>());
    ASSERT_EQ(v.Get<GfMatrix4d>(), GfMatrix4d(1.).SetScale(5.));

    v = view.Get(4);
    TF_AXIOM(!v.IsEmpty());
    TF_AXIOM(v.IsHolding<GfMatrix4d>());
    ASSERT_EQ(v.Get<GfMatrix4d>(), GfMatrix4d(1.).SetScale(21.));
}

static void
TestTimeVaryingCache()
{
    // Initialize NumComputed. We don't expect it to be incremented until
    // values are computed.
    NumComputed.store(0);

    UsdStageRefPtr stage = CreateTestStage();

    ExecUsdSystem system(stage);

    ExecUsdRequest request =
    system.BuildRequest({
        {stage->GetPrimAtPath(SdfPath("/Root")), _tokens->computeTimeVarying}});
    TF_AXIOM(request.IsValid());

    TF_AXIOM(NumComputed == 0);

    // Compute for the first time, and verify that the callback is invoked and
    // returns the expected computed value.
    UsdTimeCode currentTime = UsdTimeCode::Default();
    VtValue v = system.Compute(request).Get(0);
    TF_AXIOM(v.IsHolding<UsdTimeCode>());
    ASSERT_EQ(v.Get<UsdTimeCode>(), currentTime);
    TF_AXIOM(NumComputed == 1);

    // Compute again. The result should still be cached, and the callback
    // should not be invoked.
    v = system.Compute(request).Get(0);
    TF_AXIOM(v.IsHolding<UsdTimeCode>());
    ASSERT_EQ(v.Get<UsdTimeCode>(), currentTime);
    TF_AXIOM(NumComputed == 1);

    // Change the time, and compute again. Verify that the callback is invoked
    // and returns the expected computed value.
    currentTime = UsdTimeCode(1.0);
    system.ChangeTime(currentTime);
    v = system.Compute(request).Get(0);
    TF_AXIOM(v.IsHolding<UsdTimeCode>());
    ASSERT_EQ(v.Get<UsdTimeCode>(), currentTime);
    TF_AXIOM(NumComputed == 2);

    // Change time to a previously visited time code, and compute. Verify that
    // the callback is not invoked, as the computed result should still be
    // cached.
    currentTime = UsdTimeCode::Default();
    system.ChangeTime(currentTime);
    v = system.Compute(request).Get(0);
    TF_AXIOM(v.IsHolding<UsdTimeCode>());
    ASSERT_EQ(v.Get<UsdTimeCode>(), currentTime);
    TF_AXIOM(NumComputed == 2);
}

int
main(int argc, char* argv[])
{
    ConfigureTestPlugin();

    TestValueExtraction();
    TestTimeVaryingCache();

    return 0;
}
