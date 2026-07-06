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

#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/ts/knot.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/vt/value.h"
#include "pxr/exec/exec/builtinComputations.h"
#include "pxr/exec/exec/systemDiagnostics.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/stage.h"

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

// Verfies that the computed value of the given attribute matches the given
// expected value.
static void
_VerifyAttrValue(
    const UsdAttribute &attr,
    const double expectedValue,
    ExecUsdSystem *execSystem)
{
    const ExecUsdRequest request =
        execSystem->BuildRequest({ExecUsdValueKey{
            attr, ExecBuiltinComputations->computeResolvedValue}});

    ExecUsdCacheView cache = execSystem->Compute(request);
    const VtValue value = cache.Get(0);
    TF_AXIOM(value.IsHolding<double>());
    ASSERT_EQ(value.Get<double>(), expectedValue);
}

// Sets a spline on attr with a knot with the given value at the given time.
static void
_SetSplineKnot(
    const UsdTimeCode &time,
    const double value,
    UsdAttribute *const attr)
{
    static const TfType doubleType = TfType::Find<double>();
    TsSpline spline(doubleType);

    TsKnot knot = TsKnot(doubleType);
    knot.SetTime(time.GetValue());
    knot.SetValue(VtValue(value));
    spline.SetKnot(knot);

    attr->SetSpline(spline);
}

static void
Test_ValueInvalidation()
{
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(
        R"usda(#usda 1.0
        def Scope "Prim" {
            custom double attr = 0
        }
        )usda");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    UsdAttribute attr = usdStage->GetAttributeAtPath(SdfPath("/Prim.attr"));
    TF_AXIOM(attr);

    ExecUsdSystem execSystem(usdStage);
    UsdTimeCode time(0.0);
    execSystem.ChangeTime(time);

    _VerifyAttrValue(attr, 0.0, &execSystem);

    // Author the default value.
    attr.Set(1.0);
    _VerifyAttrValue(attr, 1.0, &execSystem);

    // Change the default value.
    attr.Set(2.0);
    _VerifyAttrValue(attr, 2.0, &execSystem);

    // Add a spline.
    _SetSplineKnot(time, 3.0, &attr);
    _VerifyAttrValue(attr, 3.0, &execSystem);

    // Change the value produced by the spline.
    _SetSplineKnot(time, 4.0, &attr);
    _VerifyAttrValue(attr, 4.0, &execSystem);

    // Author a time sample.
    attr.Set(5.0, time);
    _VerifyAttrValue(attr, 5.0, &execSystem);

    // Change the time sample value.
    attr.Set(6.0, time);
    _VerifyAttrValue(attr, 6.0, &execSystem);

    // InvalidateAll: The computed value should remain the same. (And we
    // shouldn't crash.)
    ExecSystem::Diagnostics(&execSystem).InvalidateAll();
    execSystem.ChangeTime(time);
    _VerifyAttrValue(attr, 6.0, &execSystem);

    // Set the time back to default.
    //
    // TODO: We only call InvalidateAll all here to work around a bug in exec
    // invalidation.
    ExecSystem::Diagnostics(&execSystem).InvalidateAll();
    execSystem.ChangeTime(UsdTimeCode::Default());
    _VerifyAttrValue(attr, 2.0, &execSystem);
}

int main()
{
    Test_ValueInvalidation();

    return 0;
}
