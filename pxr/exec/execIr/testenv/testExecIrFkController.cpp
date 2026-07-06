//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/execIr/computations.h"
#include "pxr/exec/execIr/tokens.h"
#include "pxr/exec/execIr/types.h"

#include "pxr/exec/exec/builtinComputations.h"
#include "pxr/exec/execUsd/cacheView.h"
#include "pxr/exec/execUsd/request.h"
#include "pxr/exec/execUsd/system.h"
#include "pxr/exec/execUsd/valueKey.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include <iostream>
#include <utility>

PXR_NAMESPACE_USING_DIRECTIVE

#define ASSERT_CLOSE(expr, expected)                                           \
    [&] {                                                                      \
        auto&& expr_ = expr;                                                   \
        if (!GfIsClose(expr_, expected, 1e-6)) {                               \
            std::cout << std::flush;                                           \
            std::cerr << std::flush;                                           \
            TF_FATAL_ERROR(                                                    \
                "Expected " TF_PP_STRINGIZE(expr) " == '%s'; got '%s'",        \
                TfStringify(expected).c_str(),                                 \
                TfStringify(expr_).c_str());                                   \
        }                                                                      \
    }()

struct _EnsureNoErrors {
    ~_EnsureNoErrors() {
        TF_VERIFY(mark.IsClean());
    }

    TfErrorMark mark;
};

static void
Test_IrForwardCompute()
{
    _EnsureNoErrors mark;

    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(
        R"usda(
        #usda 1.0

        def Scope "Root" {
            def IrFkController "FkController" {
                double in:rx =    90.0
                double in:ry =   -90.0
                double in:rz =    90.0
                double in:tx =     1.0
                double in:ty =     2.0
                double in:tz =     3.0
            }
        }
        )usda");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    const UsdPrim prim = usdStage->GetPrimAtPath(SdfPath("/Root/FkController"));
    TF_AXIOM(prim);
    const UsdAttribute outSpace =
        prim.GetAttribute(ExecIrTokens->outSpace);
    TF_AXIOM(outSpace);

    ExecUsdSystem execSystem(usdStage);
    const ExecUsdRequest request = execSystem.BuildRequest({
        ExecUsdValueKey{outSpace, ExecBuiltinComputations->computeValue}
    });
    TF_AXIOM(request.IsValid());

    // Compute forward to get the output value produced by the authored scene.
    {
        ExecUsdCacheView cache = execSystem.Compute(request);
        const VtValue value = cache.Get(0);
        TF_AXIOM(value.IsHolding<GfMatrix4d>());
        ASSERT_CLOSE(
            value.Get<GfMatrix4d>(),
            GfMatrix4d(0,  0, 1, 0,
                       0, -1, 0, 0,
                       1,  0, 0, 0,
                       1,  2, 3, 1));
    }

    // Now set the parent space and compute again.
    {
        const UsdAttribute parentSpace =
            prim.GetAttribute(ExecIrTokens->parentInSpace);
        TF_AXIOM(parentSpace);
        parentSpace.Set(GfMatrix4d(1, 0, 0, 0,
                                   0, 1, 0, 0,
                                   0, 0, 1, 0,
                                   1, 1, 1, 1));

        ExecUsdCacheView cache = execSystem.Compute(request);
        const VtValue value = cache.Get(0);
        TF_AXIOM(value.IsHolding<GfMatrix4d>());
        ASSERT_CLOSE(
            value.Get<GfMatrix4d>(),
            GfMatrix4d(0,  0, 1, 0,
                       0, -1, 0, 0,
                       1,  0, 0, 0,
                       2,  3, 4, 1));
    }
}

// Tests inverse and forward computation by:
// - Solving for input avar values with the given desired values as goals for
//   the output spaces and confirming we get the expected input values.
// - Authoring the resulting input values onto the input avars and computing
//   the output spaces and confirming that we get the desired values that were
//   used for the initial inversion.
static
void _VerifyInverseAndForwardResults(
    const std::vector<UsdAttribute> &inputAvars,
    const std::vector<double> &expectedInputValues,
    const std::vector<UsdAttribute> &outputSpaces,
    const std::vector<GfMatrix4d> &desiredValues,
    ExecUsdSystem *execSystem)
{
    TF_AXIOM(inputAvars.size() == expectedInputValues.size());
    TF_AXIOM(outputSpaces.size() == desiredValues.size());

    // Invert, specifying the desired values as goals, and confirm we get the
    // expected input values.
    {
        std::vector<ExecUsdValueKey> valueKeys;
        for (const UsdAttribute &attr : inputAvars) {
            valueKeys.emplace_back(
                attr, ExecIrComputations->computeDesiredValue);
        }
        const ExecUsdRequest request =
            execSystem->BuildRequest(std::move(valueKeys));
        TF_AXIOM(request.IsValid());

        ExecUsdValueOverrideVector overrides;
        for (size_t i=0; i<desiredValues.size(); ++i) {
            overrides.push_back(
                {{outputSpaces[i],
                  ExecIrComputations->explicitDesiredValue},
                 VtValue(desiredValues[i])});
        };
        ExecUsdCacheView cache =
            execSystem->ComputeWithOverrides(request, std::move(overrides));

        for (unsigned int i=0; i<expectedInputValues.size(); ++i) {
            const VtValue value = cache.Get(i);
            TF_AXIOM(value.IsHolding<double>());
            ASSERT_CLOSE(value.Get<double>(), expectedInputValues[i]);
        }
    }

    // Author the expected input values and do a forward compute to confirm we
    // get the desired output values.
    {
        for (unsigned int i=0; i<expectedInputValues.size(); ++i) {
            inputAvars[i].Set(expectedInputValues[i]);
        }

        std::vector<ExecUsdValueKey> valueKeys;
        for (const UsdAttribute &attr : outputSpaces) {
            valueKeys.emplace_back(attr, ExecBuiltinComputations->computeValue);
        }
        const ExecUsdRequest request =
            execSystem->BuildRequest(std::move(valueKeys));
        TF_AXIOM(request.IsValid());

        ExecUsdCacheView cache = execSystem->Compute(request);

        for (size_t i=0; i<desiredValues.size(); ++i) {
            const VtValue value = cache.Get(i);
            TF_AXIOM(value.IsHolding<GfMatrix4d>());
            ASSERT_CLOSE(value.Get<GfMatrix4d>(), desiredValues[i]);
        }
    }
}

static void
Test_IrInverseCompute()
{
    _EnsureNoErrors mark;

    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(
        R"usda(#usda 1.0

        def Scope "Root" {
            def IrFkController "FkController" {
            }
        }
        )usda");

    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    const UsdPrim prim = usdStage->GetPrimAtPath(SdfPath("/Root/FkController"));
    TF_AXIOM(prim);

    const UsdAttribute outSpace =
        prim.GetAttribute(ExecIrTokens->outSpace);
    TF_AXIOM(outSpace);

    const std::vector<UsdAttribute> inputAttributes = {
        prim.GetAttribute(ExecIrTokens->inRx),
        prim.GetAttribute(ExecIrTokens->inRy),
        prim.GetAttribute(ExecIrTokens->inRz),
        prim.GetAttribute(ExecIrTokens->inRspin),
        prim.GetAttribute(ExecIrTokens->inTx),
        prim.GetAttribute(ExecIrTokens->inTy),
        prim.GetAttribute(ExecIrTokens->inTz),
    };
    for (const UsdAttribute &attr : inputAttributes) {
        TF_AXIOM(attr);
    }

    ExecUsdSystem execSystem(usdStage);

    std::vector<ExecUsdValueKey> valueKeys;
    for (const UsdAttribute &attr : inputAttributes) {
        valueKeys.emplace_back(
            attr, ExecIrComputations->computeDesiredValue);
    }
    const ExecUsdRequest request = execSystem.BuildRequest(std::move(valueKeys));
    TF_AXIOM(request.IsValid());

    // Perform an inverse compute, to get the values of the invertible inputs
    // that produce the desired value for the output space.
    {
        const GfMatrix4d desiredOutSpaceValue(0,  0, 1, 0,
                                              0, -1, 0, 0,
                                              1,  0, 0, 0,
                                              1,  2, 3, 1);

        // Expected input values in the same order as the value keys in the
        // request.
        const std::vector<double> expectedInputValues{
            90.0, -90.0, 90.0, 0.0,    1.0, 2.0, 3.0,
        };

        _VerifyInverseAndForwardResults(
            inputAttributes, expectedInputValues,
            {outSpace}, {desiredOutSpaceValue},
            &execSystem);
    }

    // Invert with a different desired value.
    {
        const GfMatrix4d desiredOutSpaceValue( 0,  0, -1, 0,
                                               0,  1,  0, 0,
                                               1,  0,  0, 0,
                                              10, 20, 30, 1);

        // Expected input values in the same order as the value keys in the
        // request.
        const std::vector<double> expectedInputValues{
            0.0, 90.0, 0.0, 0.0,    10.0, 20.0, 30.0,
        };

        _VerifyInverseAndForwardResults(
            inputAttributes, expectedInputValues,
            {outSpace}, {desiredOutSpaceValue},
            &execSystem);
    }

    // Now set the parent space and compute the inverse again.
    {
        const UsdAttribute parentSpace =
            prim.GetAttribute(ExecIrTokens->parentInSpace);
        TF_AXIOM(parentSpace);
        parentSpace.Set(GfMatrix4d(1, 0, 0, 0,
                                   0, 1, 0, 0,
                                   0, 0, 1, 0,
                                   1, 1, 1, 1));

        const GfMatrix4d desiredOutSpaceValue( 0,  0, -1, 0,
                                               0,  1,  0, 0,
                                               1,  0,  0, 0,
                                              10, 20, 30, 1);

        // Expected input values in the same order as the value keys in the
        // request.
        const std::vector<double> expectedInputValues{
            0.0, 90.0, 0.0, 0.0,    9.0, 19.0, 29.0,
        };

        _VerifyInverseAndForwardResults(
            inputAttributes, expectedInputValues,
            {outSpace}, {desiredOutSpaceValue},
            &execSystem);
    }
}

// Test posing with default and parent spaces, using just translations, so that
// the results are easy to verify by hand.
static void
Test_IrSpacesTranslates()
{
    _EnsureNoErrors mark;

    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(
        R"usda(#usda 1.0

        def Scope "Root" {
            def IrFkController "FkController" {
                double in:tx = 5.0
                matrix4d in:defaultSpace = ((1,  0, 0, 0),
                                            (0,  1, 0, 0),
                                            (0,  0, 1, 0),
                                            (0, 10, 0, 1))
            }
        }
        )usda");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);


    const UsdPrim prim = usdStage->GetPrimAtPath(SdfPath("/Root/FkController"));
    TF_AXIOM(prim);
    const UsdAttribute outSpace =
        prim.GetAttribute(ExecIrTokens->outSpace);
    TF_AXIOM(outSpace);

    ExecUsdSystem execSystem(usdStage);
    const ExecUsdRequest request = execSystem.BuildRequest({
        ExecUsdValueKey{outSpace, ExecBuiltinComputations->computeValue}
    });
    TF_AXIOM(request.IsValid());

    // Compute forward to get the output value produced by the authored scene.
    {
        ExecUsdCacheView cache = execSystem.Compute(request);
        const VtValue value = cache.Get(0);
        TF_AXIOM(value.IsHolding<GfMatrix4d>());
        ASSERT_CLOSE(
            value.Get<GfMatrix4d>(),
            GfMatrix4d(1,  0, 0, 0,
                       0,  1, 0, 0,
                       0,  0, 1, 0,
                       5, 10, 0, 1));
    }

    // Now set the parent space and compute again.
    {
        const UsdAttribute parentSpace =
            prim.GetAttribute(ExecIrTokens->parentInSpace);
        TF_AXIOM(parentSpace);
        parentSpace.Set(GfMatrix4d(1, 0,  0, 0,
                                   0, 1,  0, 0,
                                   0, 0,  1, 0,
                                   0, 0, 20, 1));

        ExecUsdCacheView cache = execSystem.Compute(request);
        const VtValue value = cache.Get(0);
        TF_AXIOM(!value.IsEmpty());
        ASSERT_CLOSE(
            value.Get<GfMatrix4d>(),
            GfMatrix4d(1,  0,  0, 0,
                       0,  1,  0, 0,
                       0,  0,  1, 0,
                       5, 10, 20, 1));
    }


    const std::vector<UsdAttribute> inputAttributes = {
        prim.GetAttribute(ExecIrTokens->inRx),
        prim.GetAttribute(ExecIrTokens->inRy),
        prim.GetAttribute(ExecIrTokens->inRz),
        prim.GetAttribute(ExecIrTokens->inRspin),
        prim.GetAttribute(ExecIrTokens->inTx),
        prim.GetAttribute(ExecIrTokens->inTy),
        prim.GetAttribute(ExecIrTokens->inTz),
    };
    for (const UsdAttribute &attr : inputAttributes) {
        TF_AXIOM(attr);
    }

    {
        const GfMatrix4d desiredOutSpaceValue( 1,  0,  0, 0,
                                               0,  1,  0, 0,
                                               0,  0,  1, 0,
                                              11, 22, 33, 1);

        // Expected input values in the same order as the value keys in the
        // request.
        const std::vector<double> expectedInputValues{
            0.0, 0.0, 0.0, 0.0,    11.0, 12.0, 13.0,
        };

        _VerifyInverseAndForwardResults(
            inputAttributes, expectedInputValues,
            {outSpace}, {desiredOutSpaceValue},
            &execSystem);
    }
}

// Test posing with default and parent spaces, using just rotations, so that the
// results are easy to verify by hand.
static void
Test_IrSpacesRotates()
{
    _EnsureNoErrors mark;

    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(
        R"usda(#usda 1.0

        def IrFkController "FkController" {
        }
        )usda");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    // Matrices representing rotations about the X axis by 30, 45, and 90
    // degrees.
    static const GfMatrix4d rotateX30(GfRotation({1, 0, 0}, 30), {0, 0, 0});
    static const GfMatrix4d rotateX45(GfRotation({1, 0, 0}, 45), {0, 0, 0});
    static const GfMatrix4d rotateX90(GfRotation({1, 0, 0}, 90), {0, 0, 0});

    const UsdPrim prim = usdStage->GetPrimAtPath(SdfPath("/FkController"));
    const UsdAttribute outSpace =
        prim.GetAttribute(ExecIrTokens->outSpace);
    const UsdAttribute rx = prim.GetAttribute(ExecIrTokens->inRx);
    const UsdAttribute defaultSpace =
        prim.GetAttribute(ExecIrTokens->inDefaultSpace);
    const UsdAttribute parentSpace =
        prim.GetAttribute(ExecIrTokens->parentInSpace);
    TF_AXIOM(prim && outSpace && rx && defaultSpace && parentSpace);

    ExecUsdSystem execSystem(usdStage);
    const ExecUsdRequest request = execSystem.BuildRequest({
        ExecUsdValueKey{outSpace, ExecBuiltinComputations->computeValue}
    });

    // Set default space to a 45 degree rotation about X and pose by the same
    // rotation, resulting in a 90 degree rotation about X.
    {
        defaultSpace.Set(rotateX45);
        rx.Set(45.0);

        ExecUsdCacheView cache = execSystem.Compute(request);
        const VtValue value = cache.Get(0);
        TF_AXIOM(value.IsHolding<GfMatrix4d>());
        ASSERT_CLOSE(value.Get<GfMatrix4d>(), rotateX90);
    }

    // Set default space and parent space to 30 degree rotations about X and
    // pose by the same rotation, resulting in a 90 degree rotation about X.
    {
        defaultSpace.Set(rotateX30);
        parentSpace.Set(rotateX30);
        rx.Set(30.0);

        ExecUsdCacheView cache = execSystem.Compute(request);
        const VtValue value = cache.Get(0);
        TF_AXIOM(!value.IsEmpty());
        ASSERT_CLOSE(value.Get<GfMatrix4d>(), GfMatrix4d(rotateX90));
    }

    // Set default and parent space to 45 degree rotations about X and invert to
    // find the pose that make the out:space be the identity matrix, which is a
    // rotation about X of -90 degrees.
    {
        const std::vector<UsdAttribute> inputAttributes = {
            prim.GetAttribute(ExecIrTokens->inRx),
            prim.GetAttribute(ExecIrTokens->inRy),
            prim.GetAttribute(ExecIrTokens->inRz),
            prim.GetAttribute(ExecIrTokens->inRspin),
            prim.GetAttribute(ExecIrTokens->inTx),
            prim.GetAttribute(ExecIrTokens->inTy),
            prim.GetAttribute(ExecIrTokens->inTz),
        };
        for (const UsdAttribute &attr : inputAttributes) {
            TF_AXIOM(attr);
        }

        defaultSpace.Set(rotateX45);
        parentSpace.Set(rotateX45);

        const GfMatrix4d desiredOutSpaceValue(1);
        const std::vector<double> expectedInputValues{
            -90.0, 0.0, 0.0, 0.0,    0.0, 0.0, 0.0,
        };

        _VerifyInverseAndForwardResults(
            inputAttributes, expectedInputValues,
            {outSpace}, {desiredOutSpaceValue},
            &execSystem);
    }
}

static void
Test_DependentFkControllers()
{
    _EnsureNoErrors mark;

    // Create a scene with two fkControllers, where the child controller's
    // parent space is connected to the output of the parent controller.
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(
        R"usda(#usda 1.0

        def Scope "Root" {
            def IrFkController "Parent" {
            }

            def IrFkController "Child" {
                matrix4d parentIn:space.connect = </Root/Parent.out:space>
            }
        }
        )usda");

    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    const UsdPrim parent = usdStage->GetPrimAtPath(SdfPath("/Root/Parent"));
    const UsdPrim child = usdStage->GetPrimAtPath(SdfPath("/Root/Child"));
    TF_AXIOM(parent && child);

    const UsdAttribute parentOutSpace =
        parent.GetAttribute(ExecIrTokens->outSpace);
    const UsdAttribute childOutSpace =
        child.GetAttribute(ExecIrTokens->outSpace);
    TF_AXIOM(parentOutSpace && childOutSpace);

    const std::vector<UsdAttribute> inputAttributes = {
        parent.GetAttribute(ExecIrTokens->inRx),
        parent.GetAttribute(ExecIrTokens->inRy),
        parent.GetAttribute(ExecIrTokens->inRz),
        parent.GetAttribute(ExecIrTokens->inRspin),
        parent.GetAttribute(ExecIrTokens->inTx),
        parent.GetAttribute(ExecIrTokens->inTy),
        parent.GetAttribute(ExecIrTokens->inTz),

        child.GetAttribute(ExecIrTokens->inRx),
        child.GetAttribute(ExecIrTokens->inRy),
        child.GetAttribute(ExecIrTokens->inRz),
        child.GetAttribute(ExecIrTokens->inRspin),
        child.GetAttribute(ExecIrTokens->inTx),
        child.GetAttribute(ExecIrTokens->inTy),
        child.GetAttribute(ExecIrTokens->inTz),
    };
    for (const UsdAttribute &attr : inputAttributes) {
        TF_AXIOM(attr);
    }

    ExecUsdSystem execSystem(usdStage);

    const GfMatrix4d desiredParentOutSpaceValue(0,  0, 1, 0,
                                                0, -1, 0, 0,
                                                1,  0, 0, 0,
                                                0,  0, 0, 1);
    GfMatrix4d desiredChildOutSpaceValue = desiredParentOutSpaceValue;
    desiredChildOutSpaceValue.SetTranslateOnly({1, 2, 3});

    // Expected input values in the same order as the value keys in the request.
    const std::vector<double> expectedInputValues{
        90.0, -90.0, 90.0, 0.0,    0.0,  0.0, 0.0,
        0.0,   0.0,   0.0, 0.0,    3.0, -2.0, 1.0,
    };

    _VerifyInverseAndForwardResults(
        inputAttributes, expectedInputValues,
        {parentOutSpace, childOutSpace},
        {desiredParentOutSpaceValue, desiredChildOutSpaceValue},
        &execSystem);
}

int main(int argc, char **argv)
{
    Test_IrForwardCompute();
    Test_IrInverseCompute();
    Test_IrSpacesTranslates();
    Test_IrSpacesRotates();
    Test_DependentFkControllers();
}
