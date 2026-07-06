//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/execIr/computations.h"
#include "pxr/exec/execIr/tokens.h"

#include "pxr/exec/exec/builtinComputations.h"
#include "pxr/exec/execUsd/cacheView.h"
#include "pxr/exec/execUsd/request.h"
#include "pxr/exec/execUsd/system.h"
#include "pxr/exec/execUsd/valueKey.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/ts/spline.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/timeCode.h"

#include <iostream>
#include <utility>

PXR_NAMESPACE_USING_DIRECTIVE

// This scene defines a "waddler," a rig with two JointScopes and a
// SwitchController that allows one or the other JointScope to act as the
// parent. I.e., when the switch attribute is set to 'rig1', Joint1 acts as the
// parent and Joint1's avars control the posing and Joint2 inherits
// transformation from Joint1. When the switch attribute is set to 'rig2', the
// roles are reversed, and Joint2's avars drive the overall pose. And
// importantly, rotate avars pivot about the origin of whichever JointScope is
// currently acting as the parent, allowing the rig to "waddle," as it
// alternately pivots about one end or the other.
static const std::string waddlerRigLayerContents = R"usda(#usda 1.0
    def Scope "Rig" {
        def Scope "Anim" {
            token switch = "rig1"

            def IrJointScope "Joint1" {
                matrix4d posed:space.connect = [
                    </Rig/Control/Switch1.out:space>
                ]

                def IrJointScope "Joint2" {
                    double rest:tz = 10.0
                    matrix4d posed:space.connect = [
                        </Rig/Control/Switch2.out:space>
                    ]
                }
            }
        }

        def Scope "Control" {
            def IrSwitchController "Switch1" {
                token switch.connect = </Rig/Anim.switch>
                matrix4d rig1:space.connect = </Rig/Control/Rig1/FK1.out:space>
                matrix4d rig2:space.connect = </Rig/Control/Rig2/FK1.out:space>
            }

            def IrSwitchController "Switch2" {
                token switch.connect = </Rig/Anim.switch>
                matrix4d rig1:space.connect = </Rig/Control/Rig1/FK2.out:space>
                matrix4d rig2:space.connect = </Rig/Control/Rig2/FK2.out:space>
            }

            def Scope "Rig1" {
                def IrFkController "FK1" {
                    double in:tx.connect = </Rig/Anim/Joint1.avars:tx>
                    double in:ty.connect = </Rig/Anim/Joint1.avars:ty>
                    double in:tz.connect = </Rig/Anim/Joint1.avars:tz>
                    double in:rx.connect = </Rig/Anim/Joint1.avars:rx>
                    double in:ry.connect = </Rig/Anim/Joint1.avars:ry>
                    double in:rz.connect = </Rig/Anim/Joint1.avars:rz>
                    double in:rspin.connect = </Rig/Anim/Joint1.avars:rspin>
                    token in:rotationOrder.connect = [
                        </Rig/Anim/Joint1.avars:rotationOrder>
                    ]
                    matrix4d in:defaultSpace.connect = [
                        </Rig/Anim/Joint1.avars:defaultSpace>
                    ]
                }

                def IrFkController "FK2" {
                    double in:tx.connect = </Rig/Anim/Joint1/Joint2.avars:tx>
                    double in:ty.connect = </Rig/Anim/Joint1/Joint2.avars:ty>
                    double in:tz.connect = </Rig/Anim/Joint1/Joint2.avars:tz>
                    double in:rx.connect = </Rig/Anim/Joint1/Joint2.avars:rx>
                    double in:ry.connect = </Rig/Anim/Joint1/Joint2.avars:ry>
                    double in:rz.connect = </Rig/Anim/Joint1/Joint2.avars:rz>
                    double in:rspin.connect = [
                        </Rig/Anim/Joint1/Joint2.avars:rspin>
                    ]
                    token in:rotationOrder.connect = [
                        </Rig/Anim/Joint1/Joint2.avars:rotationOrder>
                    ]
                    matrix4d in:defaultSpace.connect = [
                        </Rig/Anim/Joint1/Joint2.avars:defaultSpace>
                    ]

                    matrix4d parentIn:defaultSpace.connect = [
                        </Rig/Control/Rig1/FK1.out:defaultSpace>
                    ]
                    matrix4d parentIn:space.connect = [
                        </Rig/Control/Rig1/FK1.out:space>
                    ]
                }
            }

            def Scope "Rig2" {
                def IrFkController "FK1" {
                    double in:tx.connect = </Rig/Anim/Joint1.avars:tx>
                    double in:ty.connect = </Rig/Anim/Joint1.avars:ty>
                    double in:tz.connect = </Rig/Anim/Joint1.avars:tz>
                    double in:rx.connect = </Rig/Anim/Joint1.avars:rx>
                    double in:ry.connect = </Rig/Anim/Joint1.avars:ry>
                    double in:rz.connect = </Rig/Anim/Joint1.avars:rz>
                    double in:rspin.connect = </Rig/Anim/Joint1.avars:rspin>
                    token in:rotationOrder.connect = [
                        </Rig/Anim/Joint1.avars:rotationOrder>
                    ]
                    matrix4d in:defaultSpace.connect = [
                        </Rig/Anim/Joint1.avars:defaultSpace>
                    ]

                    matrix4d parentIn:defaultSpace.connect = [
                        </Rig/Control/Rig2/FK2.out:defaultSpace>
                    ]
                    matrix4d parentIn:space.connect = [
                        </Rig/Control/Rig2/FK2.out:space>
                    ]
                }

                def IrFkController "FK2" {
                    double in:tx.connect = </Rig/Anim/Joint1/Joint2.avars:tx>
                    double in:ty.connect = </Rig/Anim/Joint1/Joint2.avars:ty>
                    double in:tz.connect = </Rig/Anim/Joint1/Joint2.avars:tz>
                    double in:rx.connect = </Rig/Anim/Joint1/Joint2.avars:rx>
                    double in:ry.connect = </Rig/Anim/Joint1/Joint2.avars:ry>
                    double in:rz.connect = </Rig/Anim/Joint1/Joint2.avars:rz>
                    double in:rspin.connect = [
                        </Rig/Anim/Joint1/Joint2.avars:rspin>
                    ]
                    token in:rotationOrder.connect = [
                        </Rig/Anim/Joint1/Joint2.avars:rotationOrder>
                    ]
                    matrix4d in:defaultSpace.connect = [
                        </Rig/Anim/Joint1/Joint2.avars:defaultSpace>
                    ]
                }
            }
        }
    }
    )usda";

#define ASSERT_CLOSE(expr, expected)                                           \
    [&] {                                                                      \
        auto&& expr_ = expr;                                                   \
        if (!GfIsClose(expr_, expected, 1e-6)) {                               \
            std::cout << std::flush;                                           \
            std::cerr << std::flush;                                           \
            TF_FATAL_ERROR(                                                    \
                "Expected " TF_PP_STRINGIZE(expr) " =~ '%s'; got '%s'",        \
                TfStringify(expected).c_str(),                                 \
                TfStringify(expr_).c_str());                                   \
        }                                                                      \
    }()

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

struct _EnsureNoErrors {
    ~_EnsureNoErrors() {
        TF_VERIFY(mark.IsClean());
    }

    TfErrorMark mark;
};

// Tests inverse and forward computation through a switch controller by:
// - Solving for input avar values with a switch avar override that selects a
//   specified rig and the given desired values as goals for the output spaces,
//   to confirm we get the expected input values.
// - Authoring the switch avar value and authoring the resulting input values
//   onto the input avars and computing the output spaces, to confirm that we
//   get the desired values that were used as goals for the initial inversion.
static
void _VerifySwitchInverseAndForwardResults(
    const UsdAttribute &switchAttribute,
    const TfToken &switchValue,
    const std::vector<UsdPrim> &jointScopes,
    const std::vector<double> &expectedInputValues,
    const std::vector<GfMatrix4d> &desiredValues,
    ExecUsdSystem *execSystem)
{
    // Verify that the switch value isn't what the switch is currently set to,
    // to make sure the switch value override has an effect.
    VtValue value;
    TF_AXIOM(switchAttribute.Get(&value));
    TF_AXIOM(value != switchValue);

    std::vector<UsdAttribute> inputAvars;
    std::vector<UsdAttribute> outputSpaces;
    for (const UsdPrim &prim : jointScopes) {
        inputAvars.push_back(
            prim.GetAttribute(ExecIrTokens->avarsRx));
        inputAvars.push_back(
            prim.GetAttribute(ExecIrTokens->avarsRy));
        inputAvars.push_back(
            prim.GetAttribute(ExecIrTokens->avarsRz));
        inputAvars.push_back(
            prim.GetAttribute(ExecIrTokens->avarsRspin));
        inputAvars.push_back(
            prim.GetAttribute(ExecIrTokens->avarsTx));
        inputAvars.push_back(
            prim.GetAttribute(ExecIrTokens->avarsTy));
        inputAvars.push_back(
            prim.GetAttribute(ExecIrTokens->avarsTz));

        outputSpaces.push_back(
            prim.GetAttribute(ExecIrTokens->posedSpace));
    }
    TF_AXIOM(inputAvars.size() == expectedInputValues.size());
    for (const UsdAttribute &attr : inputAvars) {
        TF_AXIOM(attr);
    }
    TF_AXIOM(outputSpaces.size() == desiredValues.size());
    for (const UsdAttribute &attr : outputSpaces) {
        TF_AXIOM(attr);
    }

    // Invert, specifying the desired values as goals, and confirm we get the
    // expected input values.
    {
        std::vector<ExecUsdValueKey> valueKeys;
        for (const UsdAttribute &avar : inputAvars) {
            valueKeys.emplace_back(
                avar, ExecIrComputations->computeDesiredValue);
        }
        TF_AXIOM(valueKeys.size() == expectedInputValues.size());
        const ExecUsdRequest request =
            execSystem->BuildRequest(std::move(valueKeys));
        TF_AXIOM(request.IsValid());

        ExecUsdValueOverrideVector overrides;
        overrides.push_back(
            {{switchAttribute,
              ExecBuiltinComputations->computeValue},
             VtValue(switchValue)});
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

    // Author the switch value and the expected input values and do a forward
    // compute to confirm we get the desired output values.
    switchAttribute.Set(switchValue);
    for (unsigned int i=0; i<expectedInputValues.size(); ++i) {
        inputAvars[i].Set(expectedInputValues[i]);
    }

    {
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
Test_WaddlerRigBasic()
{
    _EnsureNoErrors mark;

    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(waddlerRigLayerContents);
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    const UsdPrim joint1 =
        usdStage->GetPrimAtPath(SdfPath("/Rig/Anim/Joint1"));
    const UsdPrim joint2 =
        usdStage->GetPrimAtPath(SdfPath("/Rig/Anim/Joint1/Joint2"));
    TF_AXIOM(joint1 && joint2);

    const UsdAttribute joint1Space =
        joint1.GetAttribute(ExecIrTokens->posedSpace);
    const UsdAttribute joint2Space =
        joint2.GetAttribute(ExecIrTokens->posedSpace);
    TF_AXIOM(joint1Space && joint2Space);

    ExecUsdSystem execSystem(usdStage);
    const ExecUsdRequest outputRequest = execSystem.BuildRequest({
        ExecUsdValueKey{joint1Space},
        ExecUsdValueKey{joint2Space},
    });
    TF_AXIOM(outputRequest.IsValid());

    const UsdAttribute switchAttr =
        usdStage->GetAttributeAtPath(SdfPath("/Rig/Anim.switch"));
    TF_AXIOM(switchAttr);

    // Set the switch to Rig2 and test inverse computation with an override that
    // selects Rig1.
    switchAttr.Set(ExecIrTokens->rig2);

    {
        const GfMatrix4d desiredOutSpaceValue1(
            GfRotation({1, 0, 0}, 90),
            GfVec3d(0, 0, 0));
        GfMatrix4d desiredOutSpaceValue2(
            GfRotation({1, 0, 0}, 90),
            GfVec3d(10, 20, 30));

        const std::vector<double> expectedInputValues{
            90.0, 0.0, 0.0, 0.0,     0.0,  0.0,  0.0,
             0.0, 0.0, 0.0, 0.0,    10.0, 30.0, -30.0,
        };

        _VerifySwitchInverseAndForwardResults(
            switchAttr, ExecIrTokens->rig1,
            {joint1, joint2},
            expectedInputValues,
            {desiredOutSpaceValue1, desiredOutSpaceValue2},
            &execSystem);
    }

    // Verify the switch is now set to Rig1 and test inverse with an override
    // that selects Rig2.
    {
        VtValue value;
        TF_AXIOM(switchAttr.Get(&value));
        ASSERT_EQ(value, ExecIrTokens->rig1);

        const GfMatrix4d desiredOutSpaceValue1(
            GfRotation({0, 1, 0}, -90),
            GfVec3d(1, 2, 3));
        const GfMatrix4d desiredOutSpaceValue2(
            GfRotation({0, 1, 0}, -90),
            GfVec3d(0));

        const std::vector<double> expectedInputValues{
            0.0,   0.0, 0.0, 0.0,    3.0, 2.0, 9.0,
            0.0, -90.0, 0.0, 0.0,    0.0, 0.0, -10.0,
        };

        _VerifySwitchInverseAndForwardResults(
            switchAttr, ExecIrTokens->rig2,
            {joint1, joint2},
            expectedInputValues,
            {desiredOutSpaceValue1, desiredOutSpaceValue2},
            &execSystem);
    }
}

// Adds a knot to attr's spline with the given value at the given time, with
// curve interpolation.
//
static void
_SetSplineKnot(
    const UsdAttribute &attr,
    const UsdTimeCode &time,
    const VtValue &value)
{
    const TfType valueType = attr.GetTypeName().GetType();

    // If the attribute doesn't already have a spline, author an empty spline
    // onto it.
    if (!attr.HasSpline()) {
        attr.SetSpline(TsSpline(valueType));
    }
    TsSpline spline = attr.GetSpline();

    // Get or create a knot at the given time.
    TsKnot knot;
    if (!spline.GetKnot(time.GetValue(), &knot)) {
        knot = TsKnot(valueType);
        knot.SetTime(time.GetValue());
    }

    // Set the knots value and interpolation for the segment after the knot.
    knot.SetValue(value);
    knot.SetNextInterpolation(TsInterpCurve);

    // Set the knot on the spline and author the spline onto the attribute.
    spline.SetKnot(knot);
    attr.SetSpline(spline);
}

// Convenience template function that wraps the new knot value in a VtValue.
template <typename ValueType>
static void
_SetSplineKnot(
    const UsdAttribute &attr,
    const UsdTimeCode &time,
    const ValueType &value)
{
    _SetSplineKnot(attr, time, VtValue(value));
}

// Breaks down the given attribute's spline and copies all broken down values
// (which are _at_ the given time) to the corresponding pre-time, in preparation
// for authoring other values at the given time without disturbing animation in
// the interval before that time.
//
static void
_BreakdownPreTime(
    const UsdAttribute &attr,
    const UsdTimeCode &time)
{
    TsSpline spline = attr.GetSpline();
    if (spline.CanBreakdown(time.GetValue())) {
        spline.Breakdown(time.GetValue());
    }
    
    VtValue value;
    TsKnot knot;
    if (TF_VERIFY(spline.GetKnot(time.GetValue(), &knot))) {
        knot.GetValue(&value);
        knot.SetPreValue(value);
        if (!TF_VERIFY(spline.SetKnot(knot))) {
            return;
        }
    }

    attr.SetSpline(spline);
}

// Author a value to a switch attribute with compensation that authors input
// avar values such that the overall pose is the same before and after the
// change to the switch attribute value.
//
static void
_CompensateSwitch(
    const UsdTimeCode &time,
    const UsdAttribute &switchAttribute,
    const VtValue &switchValue,
    const UsdAttributeVector &inputAvars,
    const UsdAttributeVector &outputSpaces)
{
    ExecUsdSystem execSystem(switchAttribute.GetStage());
    execSystem.ChangeTime(time);

    // Breakdown all input avar values to the pre-time, in preparation for
    // setting new values at the current time as we compensate.
    for (const UsdAttribute &attr : inputAvars) {
        _BreakdownPreTime(attr, time);
    }

    // Compute in the forward direction to get the current values of the output
    // spaces.
    std::vector<GfMatrix4d> preSwitchSpaceValues;
    {
        std::vector<ExecUsdValueKey> valueKeys;
        for (const UsdAttribute &avar : outputSpaces) {
            valueKeys.emplace_back(avar);
        }
        const ExecUsdRequest request =
            execSystem.BuildRequest(std::move(valueKeys));
        ExecUsdCacheView cache = execSystem.Compute(request);

        preSwitchSpaceValues.reserve(outputSpaces.size());
        for (size_t i=0; i<outputSpaces.size(); ++i) {
            const VtValue value = cache.Get(i);
            TF_AXIOM(value.IsHolding<GfMatrix4d>());
            preSwitchSpaceValues.push_back(value.Get<GfMatrix4d>());
        }
    }

    // Invert to compute the input avar values that will keep the output spaces
    // the same when we author the new switch avar value.
    std::vector<double> inputAvarValues;
    {
        std::vector<ExecUsdValueKey> valueKeys;
        for (const UsdAttribute &avar : inputAvars) {
            valueKeys.emplace_back(
                avar, ExecIrComputations->computeDesiredValue);
        }
        const ExecUsdRequest request =
            execSystem.BuildRequest(std::move(valueKeys));

        ExecUsdValueOverrideVector overrides;
        overrides.reserve(outputSpaces.size() + 1);
        overrides.push_back(
            {{switchAttribute,
                 ExecBuiltinComputations->computeValue},
             switchValue});
        for (size_t i=0; i<outputSpaces.size(); ++i) {
            overrides.push_back(
                {{outputSpaces[i],
                     ExecIrComputations->explicitDesiredValue},
                 VtValue(preSwitchSpaceValues[i])});
        };

        ExecUsdCacheView cache =
            execSystem.ComputeWithOverrides(request, std::move(overrides));

        inputAvarValues.reserve(inputAvars.size());
        for (unsigned int i=0; i<inputAvars.size(); ++i) {
            const VtValue value = cache.Get(i);
            TF_AXIOM(value.IsHolding<double>());
            inputAvarValues.push_back(value.Get<double>());
        }
    }

    // Author the switch value and the input avar values that compensate for the
    // switch value change.
    switchAttribute.Set(switchValue, time);
    for (unsigned int i=0; i<inputAvarValues.size(); ++i) {
        _SetSplineKnot(inputAvars[i], time, inputAvarValues[i]);
    }

    // TEST: Compute in the forward direction to confirm we got the desired
    // values.
    {        
        std::vector<ExecUsdValueKey> valueKeys;
        for (const UsdAttribute &avar : outputSpaces) {
            valueKeys.emplace_back(avar);
        }
        const ExecUsdRequest request =
            execSystem.BuildRequest(std::move(valueKeys));
        ExecUsdCacheView cache = execSystem.Compute(request);

        for (size_t i=0; i<outputSpaces.size(); ++i) {
            const VtValue value = cache.Get(i);
            TF_AXIOM(value.IsHolding<GfMatrix4d>());
            ASSERT_CLOSE(value.Get<GfMatrix4d>(), preSwitchSpaceValues[i]);
        }
    }
}

static void
_VerifyAvarValues(
    const UsdTimeCode &time,
    const UsdAttributeVector &inputAvars,
    const std::vector<double> &expectedValues)
{
    ASSERT_EQ(inputAvars.size(), expectedValues.size());
    for (size_t i=0; i<inputAvars.size(); ++i) {
        double value;
        if (TF_VERIFY(inputAvars[i].Get(&value, time))) {
            ASSERT_CLOSE(value, expectedValues[i]);
        }
    }
}

static void
_VerifyComputedValue(
    const UsdTimeCode &time,
    const UsdAttribute &attr,
    const GfMatrix4d &expectedValue)
{
    ExecUsdSystem execSystem(attr.GetStage());
    execSystem.ChangeTime(time);
    const ExecUsdRequest request =
        execSystem.BuildRequest({ExecUsdValueKey{attr}});
    ExecUsdCacheView cache = execSystem.Compute(request);
    ASSERT_CLOSE(cache.Get(0).Get<GfMatrix4d>(), expectedValue);
}

static void
Test_WaddlerRigSwitchCompensation()
{
    _EnsureNoErrors mark;

    // Create a scene that references the waddler rig.
    const SdfLayerRefPtr rigLayer = SdfLayer::CreateAnonymous(".usda");
    rigLayer->ImportFromString(waddlerRigLayerContents);
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(
        R"usda(#usda 1.0
        def Scope "Root" (
            references = @)usda" + rigLayer->GetIdentifier() + R"usda(@</Rig>
            ) {
        }
        )usda");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    UsdAttribute switchAttr =
        usdStage->GetAttributeAtPath(SdfPath("/Root/Anim.switch"));
    TF_AXIOM(switchAttr);

    const UsdPrim joint1 =
        usdStage->GetPrimAtPath(SdfPath("/Root/Anim/Joint1"));
    const UsdPrim joint2 =
        usdStage->GetPrimAtPath(SdfPath("/Root/Anim/Joint1/Joint2"));
    TF_AXIOM(joint1 && joint2);

    UsdAttribute ry1 =
        joint1.GetAttribute(ExecIrTokens->avarsRy);
    UsdAttribute ry2 =
        joint2.GetAttribute(ExecIrTokens->avarsRy);
    TF_AXIOM(ry1 && ry2);

    const UsdAttribute joint1Space =
        joint1.GetAttribute(ExecIrTokens->posedSpace);
    const UsdAttribute joint2Space =
        joint2.GetAttribute(ExecIrTokens->posedSpace);
    TF_AXIOM(joint1Space && joint2Space);
    std::vector<UsdAttribute> outputSpaces{joint1Space, joint2Space};

    std::vector<UsdAttribute> inputAvars;
    for (const UsdPrim &prim : {joint1, joint2}) {
        inputAvars.push_back(
            prim.GetAttribute(ExecIrTokens->avarsTx));
        inputAvars.push_back(
            prim.GetAttribute(ExecIrTokens->avarsTy));
        inputAvars.push_back(
            prim.GetAttribute(ExecIrTokens->avarsTz));
        inputAvars.push_back(
            prim.GetAttribute(ExecIrTokens->avarsRx));
        inputAvars.push_back(
            prim.GetAttribute(ExecIrTokens->avarsRy));
        inputAvars.push_back(
            prim.GetAttribute(ExecIrTokens->avarsRz));
        inputAvars.push_back(
            prim.GetAttribute(ExecIrTokens->avarsRspin));
    }
    for (const UsdAttribute &attr : inputAvars) {
        TF_AXIOM(attr);
    }

    // Start the current time at 0.
    UsdTimeCode currentTime(0.0);

    // Initialize all avars with empty splines and set 0.0 knots for all input
    // avars at the initial time and set an initial time sample for the switch
    // attribute selecting 'rig1'.
    //
    // This starts the waddler off at the origin, pointing up along the Z axis.
    for (const UsdAttribute &attr : inputAvars) {
        _SetSplineKnot(attr, currentTime, 0.0);
    }
    switchAttr.Set(ExecIrTokens->rig1, currentTime);

    // Verify that the posed spaces for both joints start where we expect:
    // - They both have the same orientation, which is the identity.
    // - They are both at X=0
    // - Joint1 is at Z=0
    // - Joint2 is at Z=10
    _VerifyComputedValue(
        currentTime,
        joint1Space,
        GfMatrix4d(1, 0, 0, 0,
                   0, 1, 0, 0,
                   0, 0, 1, 0,
                   0, 0, 0, 1));
    _VerifyComputedValue(
        currentTime,
        joint2Space,
        GfMatrix4d(1, 0,  0, 0,
                   0, 1,  0, 0,
                   0, 0,  1, 0,
                   0, 0, 10, 1));

    // Set the current time to 10.0 and author a knot to Joint1.avars:ry that
    // rotates the waddler so it is laying on the ground.
    currentTime = UsdTimeCode(10.0);
    _SetSplineKnot(ry1, currentTime, 90.0);

    // Switch from rig1 to rig2, compensating into the input avars.
    _CompensateSwitch(
        currentTime,
        switchAttr, VtValue(ExecIrTokens->rig2),
        inputAvars, outputSpaces);

    // Verify the compensated input avar values:
    // - Joint2 now has the 90 degree rotation about Y because it is the parent.
    // - Joint2 is at X=10 because the rotation we applied moved it along the X
    //   axis.
    // - Joint2 is at Z=-10 because its rest position was at Z=10
    // - Joint1 is now zeroed out because it is now the child and inherits its
    //   transform from Joint2.
    _VerifyAvarValues(
        currentTime,
        inputAvars,
        { 0, 0,   0,    0, 0, 0, 0,
         10, 0, -10,    0, 90, 0, 0});

    // Set the current time to 30.0 and author a knot to Joint2.avars:ry that
    // rotates the waddler so it is laying on the ground, pointing in the
    // direction of the -X axis.
    currentTime = UsdTimeCode(30.0);
    _SetSplineKnot(ry2, currentTime, 270.0);

    // Switch from rig2 to rig1, compensating into the input avars.
    _CompensateSwitch(
        currentTime,
        switchAttr, VtValue(ExecIrTokens->rig1),
        inputAvars, outputSpaces);

    // Verify the compensated input avar values:
    // - Joint1 now has a -90 orientation about Y because it is now the parent
    //   and it is on the ground, pointing in the opposite direction from where
    //   it was at time=10.
    // - Joint1 is now at X=20 because the rotations we applied moved it along
    //   the X axis.
    // - Joint2 is now zeroed out because it is now the child and inherits its
    //   transform from Joint2.
    _VerifyAvarValues(
        currentTime,
        inputAvars,
        {20, 0, 0,    0, -90, 0, 0,
          0, 0, 0,    0,   0, 0, 0});

    // Set the current time to 50.0 and author a knot to Joint1.avars:ry that
    // rotates the waddler so it is laying on the ground, pointing in the
    // direction of the +X axis.
    currentTime = UsdTimeCode(50.0);
    _SetSplineKnot(ry1, currentTime, 90.0);

    // Switch from rig1 to rig2, compensating into the input avars.
    _CompensateSwitch(
        currentTime,
        switchAttr, VtValue(ExecIrTokens->rig2),
        inputAvars, outputSpaces);

    // Verify the compensated input avar values:
    // - Joint2 now has the 90 degree rotation about Y because it is the parent
    //   and it is on the ground, in the same orientation as it was at time=10.
    // - Joint2 is at X=30 because the rotations we applied moved it along the X
    //   axis.
    // - Joint2 is at Z=-10 because its rest position was at Z=10
    // - Joint1 is now zeroed out because it is now the child and inherits its
    //   transform from Joint2.
    _VerifyAvarValues(
        currentTime,
        inputAvars,
        { 0, 0,   0,    0,  0, 0, 0,
         30, 0, -10,    0, 90, 0, 0});

    // Set the current time to 60.0 and author a knot to Joint2.avars:ry that
    // rotates the waddler so it is back to pointing upward along the Z axis.
    currentTime = UsdTimeCode(60.0);
    _SetSplineKnot(ry2, currentTime, 180.0);

    // Verify that the posed spaces for both joints ended up where we expect:
    // - They both have the same orientation, which is a rotation by 180 about
    //   the Y axis, relative to the identity (where they started).
    // - They are both at X=30
    // - Joint1 is at Z=10
    _VerifyComputedValue(
        currentTime,
        joint1Space,
        GfMatrix4d(-1, 0,  0, 0,
                    0, 1,  0, 0,
                    0, 0, -1, 0,
                   30, 0, 10, 1));
    _VerifyComputedValue(
        currentTime,
        joint2Space,
        GfMatrix4d(-1, 0,  0, 0,
                    0, 1,  0, 0,
                    0, 0, -1, 0,
                   30, 0,  0, 1));
}

int main(int argc, char **argv)
{
    Test_WaddlerRigBasic();
    Test_WaddlerRigSwitchCompensation();
}
