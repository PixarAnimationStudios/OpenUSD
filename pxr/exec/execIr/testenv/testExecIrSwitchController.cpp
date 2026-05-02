//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

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
    const std::vector<UsdPrim> &inputPrims,
    const std::vector<double> &expectedInputValues,
    const std::vector<UsdAttribute> &outputSpaces,
    const std::vector<GfMatrix4d> &desiredValues,
    ExecUsdSystem *execSystem)
{
    TF_AXIOM(outputSpaces.size() == desiredValues.size());

    // Verify that the switch value isn't what the switch is currently set to,
    // to make sure the switch value override has an effect.
    VtValue value;
    TF_AXIOM(switchAttribute.Get(&value));
    TF_AXIOM(value != switchValue);

    std::vector<UsdAttribute> inputAvars;
    for (const UsdPrim &prim : inputPrims) {
        inputAvars.push_back(prim.GetAttribute(ExecIrFkControllerTokens->inRx));
        inputAvars.push_back(prim.GetAttribute(ExecIrFkControllerTokens->inRy));
        inputAvars.push_back(prim.GetAttribute(ExecIrFkControllerTokens->inRz));
        inputAvars.push_back(
            prim.GetAttribute(ExecIrFkControllerTokens->inRspin));
        inputAvars.push_back(prim.GetAttribute(ExecIrFkControllerTokens->inTx));
        inputAvars.push_back(prim.GetAttribute(ExecIrFkControllerTokens->inTy));
        inputAvars.push_back(prim.GetAttribute(ExecIrFkControllerTokens->inTz));
    }
    TF_AXIOM(inputAvars.size() == expectedInputValues.size());

    // Invert, specifying the desired values as goals, and confirm we get the
    // expected input values.
    {
        std::vector<ExecUsdValueKey> valueKeys;
        for (const UsdAttribute &avar : inputAvars) {
            valueKeys.emplace_back(
                avar, ExecIrComputationTokens->computeDesiredValue);
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
                  ExecIrComputationTokens->explicitDesiredValue},
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
Test_BasicSwitch()
{
    _EnsureNoErrors mark;

    // Create a scene with two rigs, each of which contains two independent FK
    // controllers, and a swtich controller that can select one of the rigs as
    // the source for two computed output spaces.
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(
        R"usda(
        #usda 1.0

        def Scope "Root" {
            def Scope "Rig1" {
                def IrFkController "FK1" {
                    double in:tx = 1.0
                }
                def IrFkController "FK2" {
                    double in:ty = 2.0
                }
            }

            def Scope "Rig2" {
                def IrFkController "FK1" {
                    double in:tx = 3.0
                }
                def IrFkController "FK2" {
                    double in:ty = 4.0
                }
            }

            def IrSwitchController "Switch" {
                matrix4d rig1:joint1:space.connect = </Root/Rig1/FK1.out:space>
                matrix4d rig1:joint2:space.connect = </Root/Rig1/FK2.out:space>
                matrix4d rig2:joint1:space.connect = </Root/Rig2/FK1.out:space>
                matrix4d rig2:joint2:space.connect = </Root/Rig2/FK2.out:space>
            }
        }
        )usda");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    const UsdAttribute joint1Space =
        usdStage->GetAttributeAtPath(SdfPath("/Root/Switch.out:joint1:space"));
    const UsdAttribute joint2Space =
        usdStage->GetAttributeAtPath(SdfPath("/Root/Switch.out:joint2:space"));
    TF_AXIOM(joint1Space && joint2Space);

    ExecUsdSystem execSystem(usdStage);
    const ExecUsdRequest outputRequest = execSystem.BuildRequest({
        ExecUsdValueKey{joint1Space},
        ExecUsdValueKey{joint2Space},
    });
    TF_AXIOM(outputRequest.IsValid());

    const UsdAttribute switchAttr =
        usdStage->GetAttributeAtPath(SdfPath("/Root/Switch.switch"));
    TF_AXIOM(switchAttr);

    // With Rig1 active ('rig1' is the fallback value for the 'switch' avar),
    // compute in the forward direction and confirm that we get the values Rig1
    // computes as authored.
    {
        ExecUsdCacheView cache = execSystem.Compute(outputRequest);
        ASSERT_CLOSE(
            cache.Get(0).Get<GfMatrix4d>(),
            []{ GfMatrix4d mat; mat.SetTranslate({1, 0, 0}); return mat; }());
        ASSERT_CLOSE(
            cache.Get(1).Get<GfMatrix4d>(),
            []{ GfMatrix4d mat; mat.SetTranslate({0, 2, 0}); return mat; }());
    }

    // Set the switch controller so Rig2 is active and compute in the forward
    // direction and confirm that we get the values Rig2 computes as authored.
    switchAttr.Set(ExecIrSwitchControllerTokens->rig2);

    {
        ExecUsdCacheView cache = execSystem.Compute(outputRequest);
        ASSERT_CLOSE(
            cache.Get(0).Get<GfMatrix4d>(),
            []{ GfMatrix4d mat; mat.SetTranslate({3, 0, 0}); return mat; }());
        ASSERT_CLOSE(
            cache.Get(1).Get<GfMatrix4d>(),
            []{ GfMatrix4d mat; mat.SetTranslate({0, 4, 0}); return mat; }());
    }

    // Set the switch to Rig1 and test inverse computation with an override that
    // selects Rig2.
    switchAttr.Set(ExecIrSwitchControllerTokens->rig1);

    {
        const UsdPrim fk1 = usdStage->GetPrimAtPath(SdfPath("/Root/Rig2/FK1"));
        const UsdPrim fk2 = usdStage->GetPrimAtPath(SdfPath("/Root/Rig2/FK2"));
        TF_AXIOM(fk1 && fk2);

        const GfMatrix4d desiredOutSpaceValue1(
            GfRotation({1, 0, 0}, 90),
            GfVec3d(0));
        const GfMatrix4d desiredOutSpaceValue2(
            GfRotation({1, 0, 0}, 0),
            GfVec3d(10, 20, 30));

        const std::vector<double> expectedInputValues{
            90.0, 0.0, 0.0, 0.0,     0.0,  0.0,  0.0,
             0.0, 0.0, 0.0, 0.0,    10.0, 20.0, 30.0,
        };

        _VerifySwitchInverseAndForwardResults(
            switchAttr, ExecIrSwitchControllerTokens->rig2,
            {fk1, fk2},
            expectedInputValues,
            {joint1Space, joint2Space},
            {desiredOutSpaceValue1, desiredOutSpaceValue2},
            &execSystem);
    }

    // Verify the switch is now set to Rig2 and test inverse with an override
    // that selects Rig1.
    {
        VtValue value;
        TF_AXIOM(switchAttr.Get(&value));
        ASSERT_EQ(value, ExecIrSwitchControllerTokens->rig2);

        const UsdPrim fk1 = usdStage->GetPrimAtPath(SdfPath("/Root/Rig1/FK1"));
        const UsdPrim fk2 = usdStage->GetPrimAtPath(SdfPath("/Root/Rig1/FK2"));
        TF_AXIOM(fk1 && fk2);

        const GfMatrix4d desiredOutSpaceValue1(
            GfRotation({1, 0, 0}, 0),
            GfVec3d(1, 2, 3));
        const GfMatrix4d desiredOutSpaceValue2(
            GfRotation({0, 1, 0}, -90),
            GfVec3d(0));

        const std::vector<double> expectedInputValues{
            0.0,   0.0, 0.0, 0.0,    1.0, 2.0, 3.0,
            0.0, -90.0, 0.0, 0.0,    0.0, 0.0, 0.0,
        };

        _VerifySwitchInverseAndForwardResults(
            switchAttr, ExecIrSwitchControllerTokens->rig1,
            {fk1, fk2},
            expectedInputValues,
            {joint1Space, joint2Space},
            {desiredOutSpaceValue1, desiredOutSpaceValue2},
            &execSystem);
    }
}

int main(int argc, char **argv)
{
    Test_BasicSwitch();
}
