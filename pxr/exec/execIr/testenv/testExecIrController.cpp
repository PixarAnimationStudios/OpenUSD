//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/execIr/computations.h"
#include "pxr/exec/execIr/controllerBuilder.h"
#include "pxr/exec/execIr/types.h"

#include "pxr/exec/exec/builtinComputations.h"
#include "pxr/exec/exec/computationBuilders.h"
#include "pxr/exec/exec/registerSchema.h"
#include "pxr/exec/execUsd/cacheView.h"
#include "pxr/exec/execUsd/request.h"
#include "pxr/exec/execUsd/system.h"
#include "pxr/exec/execUsd/valueKey.h"
#include "pxr/exec/vdf/context.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include <algorithm>
#include <iostream>
#include <utility>

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

struct _EnsureNoErrors {
    ~_EnsureNoErrors() {
        TF_VERIFY(mark.IsClean());
    }

    TfErrorMark mark;
};

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (input)
    (invertibleInput)
    (nonInvertibleInput)
    (output)
);

// AddOneController
//
// A controller with one double-valued input and one double-valued output, where
// output = input + 1

static ExecIrResult _AddOne_ForwardCompute(const VdfContext &);
static ExecIrResult _AddOne_InverseCompute(const VdfContext &);

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(TestExecIrControllerAddOneController)
{
    ExecIrControllerBuilder builder(
        self, &_AddOne_ForwardCompute, &_AddOne_InverseCompute);

    builder.InvertibleInputAttribute<double>(_tokens->input);
    builder.InvertibleOutputAttribute<double>(_tokens->output);
}

// The forward compute function.
//
// The context provides input attribute values. The function is responsible for
// computing output values, returning the values in a map from invertible input
// name to VtValue.
//
static ExecIrResult
_AddOne_ForwardCompute(const VdfContext &ctx)
{
    const double input = ctx.GetInputValue<double>(_tokens->input);

    // Create a map to store the results.
    ExecIrResult result;

    // Compute and store the output value.
    result[_tokens->output] = input + 1.0;

    return result;
}

// The inverse compute function.
//
// The context provides desired values for all invertible outputs. The
// function is responsible for computing the invertible input values that
// satisfy the desired output values, returning the values in a map from
// invertible input name to VtValue.
//
static ExecIrResult
_AddOne_InverseCompute(const VdfContext &ctx)
{
    // Create a map to store the results
    ExecIrResult result;

    const double output = ctx.GetInputValue<double>(_tokens->output);

    // Compute and store the input value
    result[_tokens->input] = output - 1.0;

    return result;
}

static void
Test_BasicForwardCompute()
{
    _EnsureNoErrors mark;

    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(
        R"usda(
        #usda 1.0

        def AddOneController "PlusOne" {
            double input = 10.0
        }
        )usda");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    const UsdPrim prim = usdStage->GetPrimAtPath(SdfPath("/PlusOne"));
    TF_AXIOM(prim);
    const UsdAttribute output = prim.GetAttribute(_tokens->output);
    TF_AXIOM(output);

    ExecUsdSystem execSystem(usdStage);
    const ExecUsdRequest request = execSystem.BuildRequest({
        ExecUsdValueKey{output, ExecBuiltinComputations->computeValue}
    });
    TF_AXIOM(request.IsValid());

    {
        ExecUsdCacheView cache = execSystem.Compute(request);
        const VtValue value = cache.Get(0);
        TF_AXIOM(!value.IsEmpty());

        ASSERT_EQ(value.Get<double>(), 11.0);
    }

    // Now set the input and compute again.
    {
        const UsdAttribute input = prim.GetAttribute(_tokens->input);
        TF_AXIOM(input);
        input.Set(2.0);

        ExecUsdCacheView cache = execSystem.Compute(request);
        const VtValue value = cache.Get(0);
        TF_AXIOM(!value.IsEmpty());
        ASSERT_EQ(value.Get<double>(), 3.0);
    }
}

static void
Test_BasicInverseCompute()
{
    _EnsureNoErrors mark;

    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(
        R"usda(
        #usda 1.0

        def AddOneController "PlusOne" {
        }
        )usda");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    const UsdPrim prim = usdStage->GetPrimAtPath(SdfPath("/PlusOne"));
    TF_AXIOM(prim);
    const UsdAttribute output = prim.GetAttribute(_tokens->output);
    const UsdAttribute input = prim.GetAttribute(_tokens->input);
    TF_AXIOM(output && input);

    ExecUsdSystem execSystem(usdStage);
    const ExecUsdRequest request = execSystem.BuildRequest({
        ExecUsdValueKey{input, ExecIrComputations->computeDesiredValue}
    });
    TF_AXIOM(request.IsValid());

    // Compute the input value that produces a given desired output value.
    {
        ExecUsdValueOverrideVector overrides {
            {{output, ExecIrComputations->explicitDesiredValue},
             VtValue(10.0)}
        };
        ExecUsdCacheView cache = execSystem.ComputeWithOverrides(
            request, std::move(overrides));

        ASSERT_EQ(cache.Get(0).Get<double>(), 9.0);
    }

    // Compute again with a different desired output value.
    {
        ExecUsdValueOverrideVector overrides {
            {{output, ExecIrComputations->explicitDesiredValue},
             VtValue(3.0)}
        };
        ExecUsdCacheView cache = execSystem.ComputeWithOverrides(
            request, std::move(overrides));

        ASSERT_EQ(cache.Get(0).Get<double>(), 2.0);
    }
}

// AddController
//
// This controller has one invertible input attribute, one non-invertible input
// attribute, and one invertible output attribute, all with value type
// double. The forward computation produces an output value that is the sum of
// the two input values.
//
// Note that this is a "spanning" controller, a controller with a forward
// compute function that can produce every value in the domain of its invertible
// output attributes. I.e., every inversion operation through a spanning
// controller produces invertible input values that satisfy the desired output
// values.

static ExecIrResult _Add_ForwardCompute(const VdfContext &);
static ExecIrResult _Add_InverseCompute(const VdfContext &);

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(TestExecIrControllerAddController)
{
    ExecIrControllerBuilder builder(
        self, &_Add_ForwardCompute, &_Add_InverseCompute);

    builder.InvertibleInputAttribute<double>(_tokens->invertibleInput);
    builder.NonInvertibleInputAttribute<double>(_tokens->nonInvertibleInput);
    builder.InvertibleOutputAttribute<double>(_tokens->output);
}

static ExecIrResult
_Add_ForwardCompute(const VdfContext &ctx)
{
    const double invertibleInput =
        ctx.GetInputValue<double>(_tokens->invertibleInput);
    const double nonInvertibleInput =
        ctx.GetInputValue<double>(_tokens->nonInvertibleInput);

    ExecIrResult result;

    // The output value is the sum of the input values.
    result[_tokens->output] = invertibleInput + nonInvertibleInput;

    return result;
}

static ExecIrResult
_Add_InverseCompute(const VdfContext &ctx)
{
    // Create a map to store the results
    ExecIrResult result;

    const double output =
        ctx.GetInputValue<double>(_tokens->output);
    const double nonInvertibleInput =
        ctx.GetInputValue<double>(_tokens->nonInvertibleInput);

    // The inverse is the output value minus the non-invertible input value.
    result[_tokens->invertibleInput] = output - nonInvertibleInput;

    return result;
}

// Test inversion operations through multiple controllers, where an invertible
// output is connected to an invertible input.
//
// In this case, we specify a single desired output value and compute a single
// desired input value, and we invoke the inverses of two controllers to do so.
static void
Test_InvertThroughMultiple()
{
    _EnsureNoErrors mark;

    // Create a scene with two spanning controllers, one of which has an
    // invertible input that is connected to the output of the other.
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(
        R"usda(
        #usda 1.0

        def AddController "C1" {
            double invertibleInput = 1
            double nonInvertibleInput = 2
        }

        def AddController "C2" {
            double invertibleInput.connect = </C1.output>
            double nonInvertibleInput = 3
        }
        )usda");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    const UsdPrim C1 = usdStage->GetPrimAtPath(SdfPath("/C1"));
    const UsdPrim C2 = usdStage->GetPrimAtPath(SdfPath("/C2"));
    TF_AXIOM(C1 && C2);

    const UsdAttribute input = C1.GetAttribute(_tokens->invertibleInput);
    const UsdAttribute output = C2.GetAttribute(_tokens->output);
    TF_AXIOM(input && output);

    ExecUsdSystem execSystem(usdStage);
    const ExecUsdRequest outputRequest = execSystem.BuildRequest({
        ExecUsdValueKey{output}
    });
    TF_AXIOM(outputRequest.IsValid());

    // Compute the forward values for both outputs produced by the authored
    // scene.
    {
        ExecUsdCacheView cache = execSystem.Compute(outputRequest);

        ASSERT_EQ(cache.Get(0).Get<double>(), 6.0);
    }

    // To test if controller inversion works correctly, it's important to choose
    // a desired output value that doesn't match those currently produced by the
    // authored scene.
    const double desiredOutputValue = 20.0;

    const double expectedInvertedInputValue = 15.0;

    // Compute the inverse value for the input that satisfies the given desired
    // output value.
    {
        const ExecUsdRequest desiredValueRequest = execSystem.BuildRequest({
            ExecUsdValueKey{
                input, ExecIrComputations->computeDesiredValue}
        });
        TF_AXIOM(desiredValueRequest.IsValid());

        ExecUsdValueOverrideVector overrides {
            {{output, ExecIrComputations->explicitDesiredValue},
             VtValue(desiredOutputValue)}
        };
        ExecUsdCacheView cache = execSystem.ComputeWithOverrides(
            desiredValueRequest, std::move(overrides));

        ASSERT_EQ(cache.Get(0).Get<double>(), expectedInvertedInputValue);
    }

    // Author the inverted value computed above and verify that it satisfies
    // the goal.
    {
        input.Set(expectedInvertedInputValue);

        ExecUsdCacheView cache = execSystem.Compute(outputRequest);

        ASSERT_EQ(cache.Get(0).Get<double>(), desiredOutputValue);
    }
}

// Test an inversion operation when conflicting goals are specified.
//
// This happens when we specify desired values for more than one invertible
// output that affect the inverted value for a requested invertible input. This
// is an error condition. (Note that we emit an error in this situation,
// regardless of whether the values of the conflicting goals might be consistent
// with a computed inverted value.)
static void
Test_ConflictingGoals()
{
    // Create a scene with three controllers, two (C2 and C3) with invertible
    // inputs that have connections to the output of C1
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(
        R"usda(
        #usda 1.0

        def AddController "C1" {
            double invertibleInput = 1
            double nonInvertibleInput = 2
        }

        def AddController "C2" {
            double invertibleInput.connect = </C1.output>
            double nonInvertibleInput = 1
        }

        def AddController "C3" {
            double invertibleInput.connect = </C1.output>
            double nonInvertibleInput = 2
        }
        )usda");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    const UsdAttribute input =
        usdStage->GetAttributeAtPath(SdfPath("/C1.invertibleInput"));
    const UsdAttribute output1 =
        usdStage->GetAttributeAtPath(SdfPath("/C2.output"));
    const UsdAttribute output2 =
        usdStage->GetAttributeAtPath(SdfPath("/C3.output"));
    TF_AXIOM(input && output1 && output2);

    ExecUsdSystem execSystem(usdStage);

    // Compute the forward values for both outputs produced by the authored
    // scene.
    {
        _EnsureNoErrors mark;

        const ExecUsdRequest request = execSystem.BuildRequest({
                ExecUsdValueKey{output1},
                ExecUsdValueKey{output2}
            });
        TF_AXIOM(request.IsValid());

        ExecUsdCacheView cache = execSystem.Compute(request);

        ASSERT_EQ(cache.Get(0).Get<double>(), 4.0);
        ASSERT_EQ(cache.Get(1).Get<double>(), 5.0);
    }

    // Compute the inverse with a goal specified for one of the invertible
    // outputs.
    {
        _EnsureNoErrors mark;

        const ExecUsdRequest request = execSystem.BuildRequest({
            ExecUsdValueKey{
                input, ExecIrComputations->computeDesiredValue}
            });
        TF_AXIOM(request.IsValid());

        const double desiredOutputValue = 10.0;

        ExecUsdValueOverrideVector overrides {
            {{output1, ExecIrComputations->explicitDesiredValue},
             VtValue(desiredOutputValue)},
        };

        ExecUsdCacheView cache = execSystem.ComputeWithOverrides(
            request, std::move(overrides));

        ASSERT_EQ(cache.Get(0).Get<double>(), 7.0);
    }

    // Attempt to compute the inverse values with goals specified for both
    // invertible outputs. These goals are conflicting because they both
    // influence the result for the requested invertible input.
    {
        TfErrorMark mark;

        const ExecUsdRequest request = execSystem.BuildRequest({
            ExecUsdValueKey{
                input, ExecIrComputations->computeDesiredValue}
            });
        TF_AXIOM(request.IsValid());

        const double desiredOutputValue1 = 10.0;
        const double desiredOutputValue2 = 20.0;

        ExecUsdValueOverrideVector overrides {
            {{output1, ExecIrComputations->explicitDesiredValue},
             VtValue(desiredOutputValue1)},
            {{output2, ExecIrComputations->explicitDesiredValue},
             VtValue(desiredOutputValue2)}
        };

        // We expect no errors up to this point.
        TF_AXIOM(mark.IsClean());

        ExecUsdCacheView cache = execSystem.ComputeWithOverrides(
            request, std::move(overrides));

        // We expect exactly one error, due to the presence of conflicting
        // inversion goals.
        ASSERT_EQ(std::distance(mark.begin(), mark.end()), 1);

        // The computed result is an empty value.
        TF_AXIOM(cache.Get(0).IsEmpty());
    }

    // Attempt to invert again, this time specifying desired values for C1's
    // output and C2's output. This is slightly different from the case above
    // because before, the conflicting desired values both flow over incoming
    // connections, but the confict is between an explicitly specified desired
    // value and one computed desired value via an incoming connection.
    {
        TfErrorMark mark;

        const UsdAttribute outputC1 =
            usdStage->GetAttributeAtPath(SdfPath("/C1.output"));
        TF_AXIOM(outputC1);

        const ExecUsdRequest request = execSystem.BuildRequest({
            ExecUsdValueKey{
                input, ExecIrComputations->computeDesiredValue}
            });
        TF_AXIOM(request.IsValid());

        const double desiredOutputValue1 = 10.0;
        const double desiredOutputValue2 = 20.0;

        ExecUsdValueOverrideVector overrides {
            {{output1, ExecIrComputations->explicitDesiredValue},
             VtValue(desiredOutputValue1)},
            {{outputC1, ExecIrComputations->explicitDesiredValue},
             VtValue(desiredOutputValue2)}
        };

        // We expect no errors up to this point.
        TF_AXIOM(mark.IsClean());

        ExecUsdCacheView cache = execSystem.ComputeWithOverrides(
            request, std::move(overrides));

        // We expect exactly one error, due to the presence of conflicting
        // inversion goals.
        ASSERT_EQ(std::distance(mark.begin(), mark.end()), 1);

        // The computed result is an empty value.
        TF_AXIOM(cache.Get(0).IsEmpty());
    }
}

// Test inversion operations involving two controllers, where a non-invertible
// inut is connected to an invertible output.
//
// In this case, we specify a desired output value for each controller and
// compute desired input values for each, and the controller inverses are
// invoked simultanously.
static void
Test_DependentControllers()
{
    _EnsureNoErrors mark;

    // Create a scene with two spanning controllers, one of which has a
    // non-invertible input that is connected to the output of the other.
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(
        R"usda(
        #usda 1.0

        def AddController "C1" {
            double invertibleInput = 1
            double nonInvertibleInput = 2
        }

        def AddController "C2" {
            double invertibleInput = 3
            double nonInvertibleInput.connect = </C1.output>
        }
        )usda");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    const UsdPrim C1 = usdStage->GetPrimAtPath(SdfPath("/C1"));
    const UsdPrim C2 = usdStage->GetPrimAtPath(SdfPath("/C2"));
    TF_AXIOM(C1 && C2);

    const UsdAttribute input1 = C1.GetAttribute(_tokens->invertibleInput);
    const UsdAttribute input2 = C2.GetAttribute(_tokens->invertibleInput);
    const UsdAttribute output1 = C1.GetAttribute(_tokens->output);
    const UsdAttribute output2 = C2.GetAttribute(_tokens->output);
    TF_AXIOM(input1 && input2 && output1 && output2);

    ExecUsdSystem execSystem(usdStage);
    const ExecUsdRequest outputRequest = execSystem.BuildRequest({
        ExecUsdValueKey{output1},
        ExecUsdValueKey{output2}
    });
    TF_AXIOM(outputRequest.IsValid());

    // Compute the forward values for both outputs produced by the authored
    // scene.
    {
        ExecUsdCacheView cache = execSystem.Compute(outputRequest);

        ASSERT_EQ(cache.Get(0).Get<double>(), 3.0);
        ASSERT_EQ(cache.Get(1).Get<double>(), 6.0);
    }

    // To test if controller inversion works correctly, it's important to choose
    // desired output values that don't match those currently produced by the
    // authored scene.
    const double desiredOutput1Value = 20.0;
    const double desiredOutput2Value = 30.0;

    const double expectedInvertedInput1Value = 18.0;
    const double expectedInvertedInput2Value = 10.0;

    // Compute inverse values for both inputs that satisfy the given desired
    // output values.
    {
        const ExecUsdRequest desiredValueRequest = execSystem.BuildRequest({
            ExecUsdValueKey{
                input1, ExecIrComputations->computeDesiredValue},
            ExecUsdValueKey{
                input2, ExecIrComputations->computeDesiredValue}
        });
        TF_AXIOM(desiredValueRequest.IsValid());

        ExecUsdValueOverrideVector overrides {
            {{output1, ExecIrComputations->explicitDesiredValue},
             VtValue(desiredOutput1Value)},
            {{output2, ExecIrComputations->explicitDesiredValue},
             VtValue(desiredOutput2Value)}
        };
        ExecUsdCacheView cache = execSystem.ComputeWithOverrides(
            desiredValueRequest, std::move(overrides));

        ASSERT_EQ(cache.Get(0).Get<double>(), expectedInvertedInput1Value);
        ASSERT_EQ(cache.Get(1).Get<double>(), expectedInvertedInput2Value);
    }

    // Author the inverted values computed above and verify that they satisfy
    // the goals.
    {
        input1.Set(expectedInvertedInput1Value);
        input2.Set(expectedInvertedInput2Value);

        ExecUsdCacheView cache = execSystem.Compute(outputRequest);

        ASSERT_EQ(cache.Get(0).Get<double>(), desiredOutput1Value);
        ASSERT_EQ(cache.Get(1).Get<double>(), desiredOutput2Value);
    }
}

// AddWithClampController
//
// This controller has one invertible input attribute, one non-invertible input
// attribute, and one invertible output attribute, all with value type
// double. The forward computation produces an output value that is the sum of
// the two input values, clamped to a value of 10.0.
//
// Note that this is a "non-spanning" controller, a controller with a forward
// compute function that can't produce all values in the domain of its
// invertible outputs. I.e., for such a controller, an inversion computation may
// produce input values that don't actually acheive the desired output values.

static ExecIrResult _AddWithClamp_ForwardCompute(const VdfContext &);
static ExecIrResult _AddWithClamp_InverseCompute(const VdfContext &);

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(TestExecIrControllerAddWithClampController)
{
    ExecIrControllerBuilder builder(
        self, &_AddWithClamp_ForwardCompute, &_AddWithClamp_InverseCompute);

    builder.InvertibleInputAttribute<double>(_tokens->invertibleInput);
    builder.NonInvertibleInputAttribute<double>(_tokens->nonInvertibleInput);
    builder.InvertibleOutputAttribute<double>(_tokens->output);
}

static ExecIrResult
_AddWithClamp_ForwardCompute(const VdfContext &ctx)
{
    const double invertibleInput =
        ctx.GetInputValue<double>(_tokens->invertibleInput);
    const double nonInvertibleInput =
        ctx.GetInputValue<double>(_tokens->nonInvertibleInput);

    ExecIrResult result;

    // The output value is the clamped sum of the input values.
    result[_tokens->output] =
        std::min(invertibleInput + nonInvertibleInput, 10.0);

    return result;
}

static ExecIrResult
_AddWithClamp_InverseCompute(const VdfContext &ctx)
{
    // Create a map to store the results
    ExecIrResult result;

    const double output =
        ctx.GetInputValue<double>(_tokens->output);
    const double nonInvertibleInput =
        ctx.GetInputValue<double>(_tokens->nonInvertibleInput);

    // The inverse is the output value minus the non-invertible input value,
    // which is the best we can do, but can't achieve every goal, since the
    // clamping means this controller doesn't fully span the space of solutions.
    result[_tokens->invertibleInput] =
        std::min(output, 10.0) - nonInvertibleInput;

    return result;
}

// Test inversion operations involving two controllers, where a non-invertible
// input is connected to an invertible output--and one of the controllers is
// non-spanning.
//
// This case demonstrates that solves involveing non-spanning controllers work
// correctly, since that requires "inverted forward values," where the inverse
// for the dependent controller sees forward values from the controller it
// depends on, evaluated with the desired output values that are provided as
// overrides for the inverse compute operation.
static void
Test_NonSpanningController()
{
    _EnsureNoErrors mark;

    // Create a scene with a spanning controller with a non-invertible input
    // that is connected to the output of a non-spanning controller.
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(
        R"usda(
        #usda 1.0

        def AddWithClampController "C1" {
            double invertibleInput = 1
            double nonInvertibleInput = 2
        }

        def AddController "C2" {
            double invertibleInput = 3
            double nonInvertibleInput.connect = </C1.output>
        }
        )usda");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    const UsdPrim C1 = usdStage->GetPrimAtPath(SdfPath("/C1"));
    const UsdPrim C2 = usdStage->GetPrimAtPath(SdfPath("/C2"));
    TF_AXIOM(C1 && C2);
    const UsdAttribute input1 = C1.GetAttribute(_tokens->invertibleInput);
    const UsdAttribute input2 = C2.GetAttribute(_tokens->invertibleInput);
    TF_AXIOM(input1 && input2);
    const UsdAttribute output1 = C1.GetAttribute(_tokens->output);
    const UsdAttribute output2 = C2.GetAttribute(_tokens->output);
    TF_AXIOM(output1 && output2);

    ExecUsdSystem execSystem(usdStage);
    const ExecUsdRequest outputRequest = execSystem.BuildRequest({
        ExecUsdValueKey{output1},
        ExecUsdValueKey{output2}
    });
    TF_AXIOM(outputRequest.IsValid());

    // Compute the forward values for both outputs produced by the authored
    // scene.
    {
        ExecUsdCacheView cache = execSystem.Compute(outputRequest);

        ASSERT_EQ(cache.Get(0).Get<double>(), 3.0);
        ASSERT_EQ(cache.Get(1).Get<double>(), 6.0);
    }

    const double expectedInvertedInput1Value = 8.0;
    const double expectedInvertedInput2Value = 20.0;

    // Compute inverse values for both inputs that satisfy the given desired
    // output values.
    {
        const ExecUsdRequest desiredValueRequest = execSystem.BuildRequest({
            ExecUsdValueKey{
                input1, ExecIrComputations->computeDesiredValue},
            ExecUsdValueKey{
                input2, ExecIrComputations->computeDesiredValue}
        });
        TF_AXIOM(desiredValueRequest.IsValid());

        // To test if inversion works correctly in this case, it's important to
        // choose desired output values that can't be satisfied by the
        // non-spanning controller and that don't match those currently produced
        // by the authored scene.
        ExecUsdValueOverrideVector overrides {
            {{output1, ExecIrComputations->explicitDesiredValue},
             VtValue(20.0)},
            {{output2, ExecIrComputations->explicitDesiredValue},
             VtValue(30.0)}
        };
        ExecUsdCacheView cache = execSystem.ComputeWithOverrides(
            desiredValueRequest, std::move(overrides));

        ASSERT_EQ(cache.Get(0).Get<double>(), expectedInvertedInput1Value);
        ASSERT_EQ(cache.Get(1).Get<double>(), expectedInvertedInput2Value);
    }

    // Author the inverted values computed above.
    {
        input1.Set(expectedInvertedInput1Value);
        input2.Set(expectedInvertedInput2Value);

        ExecUsdCacheView cache = execSystem.Compute(outputRequest);

        // As expected, C1 doesn't satisfy the goal given these values, because
        // it is non-spanning; instead we get the maxium value the non-spanning
        // controller will produce. But C2 is able to produce the desired value
        // because 1) it is a spanning controller and 2) its inverse is able to
        // see C1's best-effort solution, via inverted forward values.
        ASSERT_EQ(cache.Get(0).Get<double>(), 10.0);
        ASSERT_EQ(cache.Get(1).Get<double>(), 30.0);
    }
}

int main(int argc, char **argv)
{
    // Load the custom schema.
    const PlugPluginPtrVector testPlugins =
        PlugRegistry::GetInstance().RegisterPlugins(TfAbsPath("resources"));
    ASSERT_EQ(testPlugins.size(), 1);
    ASSERT_EQ(testPlugins[0]->GetName(), "testExecIrController");

    Test_BasicForwardCompute();
    Test_BasicInverseCompute();
    Test_InvertThroughMultiple();
    Test_ConflictingGoals();
    Test_DependentControllers();
    Test_NonSpanningController();
}
