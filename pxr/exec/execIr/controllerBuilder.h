//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_IR_CONTROLLER_BUILDER_H
#define PXR_EXEC_EXEC_IR_CONTROLLER_BUILDER_H

/// \file
///
/// Registration utilities for defining invertible controller computations,
/// which are the basis for invertible rigging.
///
/// Controllers define forward computations that take the values of input
/// attributes and produce values for output attributes. Controllers also define
/// inverse computations that take desired values for invertible output
/// attributes and produce values for invertible input attributs that are
/// necessary for the forward computation to produce the desired results.
///

#include "pxr/pxr.h"

#include "pxr/exec/execIr/computations.h"
#include "pxr/exec/execIr/types.h"

#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include "pxr/exec/exec/builtinComputations.h"
#include "pxr/exec/exec/computationBuilders.h"
#include "pxr/exec/vdf/executionTypeRegistry.h"
#include "pxr/exec/vdf/readIteratorRange.h"

PXR_NAMESPACE_OPEN_SCOPE

// Builder base class used to share common code.
//
class ExecIr_ControllerBuilderBase {
protected:
    EXECIR_API
    ExecIr_ControllerBuilderBase(
        ExecComputationBuilder &self);

    // The destructor is protected so that it's not possible to delete through
    // a base class pointer.
    //
    virtual ~ExecIr_ControllerBuilderBase();

    // Registers the 'explicitDesiredValue' and 'computedDesiredValue'
    // computations that are required to compute desired values, including
    // desired values that are expressed as overrides.
    //
    // TODO: These plugin computations won't be necessary when OpenExec provides
    // core inversion support.
    //
    template <typename ValueType>
    void
    _DesiredValueComputations(
        const TfToken &attributeName);

    // Sets the computation output to the value from the 'explicitDesiredValue'
    // input if it provides one; otherwise sets the output to the the
    // 'computeDesiredValue' input, if it has one; otherwise sets an empty
    // output value.
    //
    // If, among all considered inputs, more than one input value is provided,
    // an error is emitted.
    //
    // TODO: The compuations that use this callback won't be necessary when
    // OpenExec provides core inversion support.
    //
    template <typename ValueType>
    static void
    _GetExactlyOneDesiredValue(
        const VdfContext &ctx);

protected:
    ExecComputationBuilder &_self;
};

/// Builder class used to register invertible controller computations.
///
/// This class can only be used in the context of schema computation
/// registration. The constructor takes the `self` builder object that is
/// defined by the `EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA` macro. The
/// constructor also takes the callbacks that implement the forward and inverse
/// computations for the controller. The client uses member functions to
/// register controller attributes as inputs, outputs, switches, etc. (see the
/// documentation on the corresonding registration methods for details). These
/// registrations, in turn, generate the computation inputs for the callbacks
/// (as documented in the member function documentation), as well as other
/// computations that are required to implement invertible controllers within
/// OpenExec.
///
/// # Example
///
/// ```cpp
/// // A simple invertible controller where the forward compute takes an input
/// // value and produces an output value that is one greater than the input.
///
/// // Forward declare forward and inverse functions.
/// static ExecIrResult _ForwardCompute(const VdfContext &ctx);
/// static ExecIrResult _InverseCompute(const VdfContext &ctx);
///
/// EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(MyAddOneController)
/// {
///     auto builder = ExecIrControllerBuilder(
///         self, _ForwardCompute, _InverseCompute);
///
///     // Register one invertible input and one invertible output.
///     builder.InvertibleInputAttribute(_tokens->input);
///     builder.InvertibleOutputAttribute(_tokens->output);
/// }
///
/// // The forward compute callback function.
/// //
/// // The VdfContext provides values for all inputs. The function is
/// // responsible for computing all output values, returning the values in a
/// // map from output name to VtValue.
/// //
/// static ExecIrResult
/// _ForwardCompute(const VdfContext & ctx)
/// {
///     // Extract the input value.
///     const double input = ctx.GetInputValue<double>(_tokens->input);
///
///     // Create a map to store the results.
///     ExecIrResult result;
///
///     // Compute and store the output value.
///     result[_tokens->output] = input + 1.0;
///
///     return result;
/// }
///
/// // The inverse compute callback function.
/// //
/// // The context provides desired values for all invertible outputs. The
/// // function is responsible for computing the invertible input values that
/// // satisfy the desired output values, returning the values in a map from
/// // invertible input name to VtValue.
/// //
/// static ExecIrResult
/// _InverseCompute(const VdfContext & ctx)
/// {
///     // Extract the output value.
///     const double output = ctx.GetInputValue<double>(_tokens->output);
///
///     // Create a map to store the results
///     ExecIrResult result;
///
///     // Compute and store the input value
///     result[_tokens->input] = output - 1.0;
///
///     return result;
/// }
/// ```
///
class ExecIrControllerBuilder : ExecIr_ControllerBuilderBase {
public:

    /// The type for forward and inverse controller computation calbacks.
    using Callback = ExecIrResult(*)(const VdfContext &);

    /// Constructs a builder that is used to register computations that
    /// implement an invertible controller.
    ///
    /// \p self is the builder that is defined by
    /// `EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA`. \p forwardCallback and \p
    /// inverseCallback are the callbacks that define the forward and inverse
    /// computations that implement the controller to be registered by the
    /// constructed instance.
    ///
    EXECIR_API
    ExecIrControllerBuilder(
        ExecComputationBuilder &self,
        Callback forwardCallback,
        Callback inverseCallback);

    EXECIR_API
    ~ExecIrControllerBuilder() override;

    /// Registers an invertible input attribute.
    ///
    /// - Invertible input attributes provide input to the forward computation.
    /// - Invertible input values are produced by the inverse computation.
    ///
    template <typename ValueType>
    void
    InvertibleInputAttribute(
        const TfToken &attributeName);

    /// Registers a non-invertible input attribute.
    ///
    /// - All input attributes provide input to the forward computation.
    /// - Non-invertible input attributes also provide input to the inverse
    ///   computation.
    ///
    template <typename ValueType>
    void
    NonInvertibleInputAttribute(
        const TfToken &attributeName);

    /// Registers multiple invertible input attributes.
    ///
    template <typename ValueType>
    void
    InvertibleInputAttributes(
        const TfTokenVector &attributeNames) {
        for (const TfToken &attributeName : attributeNames) {
            InvertibleInputAttribute<ValueType>(attributeName);
        }
    }

    /// Registers an invertible output attribute.
    ///
    /// - Output attributes produce computed values that are the results of the
    ///   forward computation and provide input to the inverse computation.
    ///
    /// TODO: Non-invertible output attributes are not yet implemented.
    ///
    template <typename ValueType>
    void
    InvertibleOutputAttribute(
        const TfToken &attributeName);

    /// Registers multiple invertible output attributes.
    ///
    template <typename ValueType>
    void
    InvertibleOutputAttributes(
        const TfTokenVector &attributeNames) {
        for (const TfToken &attributeName : attributeNames) {
            InvertibleOutputAttribute<ValueType>(attributeName);
        }
    }

    /// Registers a switch attribute.
    /// 
    /// Switch attributes hold values that change the behavior of the forward
    /// and inverse computations.
    /// 
    /// - Switch attributes provide input to both the forward and the inverse
    ///   computation.
    /// 
    template <typename ValueType>
    void
    SwitchAttribute(
        const TfToken &attributeName);

    /// Registers a pair of input, output passthrough attributes.
    /// 
    /// - The passthrough input attribute provides input to both the forward and
    ///   inverse computations.
    /// - The passthrough output attribute produces the value of the input
    ///   attribute.
    /// 
    template <typename ValueType>
    void
    PassthroughAttributes(
        const TfToken &inAttributeName,
        const TfToken &outAttributeName);

private:
    
    // These computations are registered by the controller builder, but they
    // are not meant to be consumed by code outside the controller builder.
    struct _PrivateComputationsType {
        EXECIR_API
        _PrivateComputationsType();

        const TfToken computeInvertedForwardValue;
        const TfToken forwardCompute;
        const TfToken inverseCompute;
    };

    EXECIR_API
    static TfStaticData<_PrivateComputationsType> _privateComputations;

    ExecPrimComputationBuilder _forwardComputeReg;
    ExecPrimComputationBuilder _inverseComputeReg;
    TfTokenVector _invertibleOutputAttributeNames;
    Callback _inverseCallback;
};

template <typename ValueType>
void
ExecIrControllerBuilder::InvertibleInputAttribute(
    const TfToken &attributeName)
{
    using namespace exec_registration;

    // Invertible input attributes define 'computeDesiredValue' computations
    // that extract their values from the inverse computation.
    //
    // TODO: When we have core inversion support, this plugin computation will
    // be registered using a builtin computation token that the core will
    // provide, as a plugin point for plugins that provide inversion behavior.
    _self.AttributeComputation(
        attributeName, ExecIrComputations->computeDesiredValue)
        .Callback<ValueType>([attributeName](const VdfContext &ctx) {
            const ExecIrResult &resultMap = ctx.GetInputValue<ExecIrResult>(
                    _privateComputations->inverseCompute);

            // If the inverse computation computed a value for this attribute,
            // return it; otherwise, set an empty result.
            if (const auto it = resultMap.find(attributeName);
                it != resultMap.end()) {
                ctx.SetOutput(it->second.Get<ValueType>());
            }
            else {
                ctx.SetEmptyOutput();
            }
        })
        .Inputs(
            Prim().Computation<ExecIrResult>(
                _privateComputations->inverseCompute));

    // Invertible input attributes define 'computeInvertedForwardValue'
    // computations that get their values from the inverse computation, but only
    // if they are evaluated in a context where the inverse is being computed
    // (i.e., when the inverse directly or transitively receives desired value
    // overrides). Otherwise, this computation falls back to the computed value
    // of the input attribute.
    // 
    // This allows for correct computation of inverses for "non-spanning"
    // controllers. I.e., this is important in cases where controller inverses
    // can't always satisfy all desired output values, and when one controller
    // is dependent on another. In such situations, computing "inverted forward
    // values" allows for controller networks to produce "best try" solutions.
    _self.AttributeComputation(
        attributeName, _privateComputations->computeInvertedForwardValue)
        .Callback(+[](const VdfContext &ctx) -> ValueType {
            // If we have a computed desired value, that takes precedence.
            if (const ValueType *const desiredValue =
                ctx.GetInputValuePtr<ValueType>(
                    ExecIrComputations->computeDesiredValue)) {
                return *desiredValue;
            }

            // Otherwise, return the attribute's computed value.
            return ctx.GetInputValue<ValueType>(
                ExecBuiltinComputations->computeValue);
        })
        .Inputs(
            Computation<ValueType>(ExecIrComputations->computeDesiredValue),
            Computation<ValueType>(ExecBuiltinComputations->computeValue));

    // Invertible input attributes provide inputs to the forward computation via
    // the 'computeInvertedForwardValue' computation.
    _forwardComputeReg.Inputs(
        Attribute(attributeName)
            .Computation<ValueType>(
                _privateComputations->computeInvertedForwardValue)
            .InputName(attributeName));
}

template <typename ValueType>
void
ExecIrControllerBuilder::NonInvertibleInputAttribute(
    const TfToken &attributeName)
{
    using namespace exec_registration;

    // All input attributes (invertible or not) are inputs to the forward
    // computation.
    _forwardComputeReg.Inputs(AttributeValue<ValueType>(attributeName));

    // Non-invertible input attributes are inputs to the inverse computation.
    _inverseComputeReg.Inputs(AttributeValue<ValueType>(attributeName));
}

template <typename ValueType>
void
ExecIrControllerBuilder::InvertibleOutputAttribute(
    const TfToken &attributeName)
{
    using namespace exec_registration;

    _invertibleOutputAttributeNames.push_back(attributeName);

    // Invertible outputs support computing desired values, for inversion.
    _DesiredValueComputations<ValueType>(attributeName);

    // Invertible outputs provide inputs to the inverse computation, getting
    // their values from the attribute's 'computeDesiredValue' computation.
    _inverseComputeReg.Inputs(
        Attribute(attributeName)
            .Computation<ValueType>(
                ExecIrComputations->computeDesiredValue)
            .InputName(attributeName));

    // Register an expression on the invertible output that pulls its value
    // from the forward computation result map.
    _self.AttributeExpression(attributeName)
        .Callback<ValueType>([attributeName](const VdfContext &ctx) {
            const ExecIrResult &resultMap =
                ctx.GetInputValue<ExecIrResult>(
                    _privateComputations->forwardCompute);
            const auto it = resultMap.find(attributeName);
            if (it != resultMap.end()) {
                ctx.SetOutput(it->second.Get<ValueType>());
            }
            else {
                TF_CODING_ERROR(
                    "Failed to find a result value for output attribute '%s' "
                    "when computing %s",
                    attributeName.GetText(),
                    ctx.GetNodeDebugName().c_str());

                ctx.SetOutput(
                    VdfExecutionTypeRegistry::GetInstance()
                        .GetFallback<ValueType>());
            }
        })
        .Inputs(
            Prim().Computation<ExecIrResult>(
                _privateComputations->forwardCompute));
}

template <typename ValueType>
void
ExecIrControllerBuilder::SwitchAttribute(
    const TfToken &attributeName)
{
    // TODO: We will have work to do here that is specific to switch attributes
    // when we implement more invertible rigging features. E.g., switch
    // attributes play a key role in determining which controllers contribute to
    // the current pose, and determining contributing controllers is critical
    // for supporing advanced authoring behaviors for invertible rigs.

    // Switch attributes have all the exec registrations that non-invertible
    // attributes have.
    NonInvertibleInputAttribute<ValueType>(attributeName);
}

template <typename ValueType>
void
ExecIrControllerBuilder::PassthroughAttributes(
    const TfToken &inAttributeName,
    const TfToken &outAttributeName)
{
    using namespace exec_registration;

    // Passthrough attributes are inputs to the forward and inverse
    // computations.
    _forwardComputeReg.Inputs(AttributeValue<ValueType>(inAttributeName));
    _inverseComputeReg.Inputs(AttributeValue<ValueType>(inAttributeName));

    // Passthrough attributes pass the value from the input attribute to the
    // output attribute.
    _self.AttributeExpression(outAttributeName)
        .Inputs(Prim().AttributeValue<ValueType>(inAttributeName))
        .template Callback<ValueType>([inAttributeName](const VdfContext &ctx) {
            ctx.SetOutputToReferenceInput(inAttributeName);
        });
}

template <typename ValueType>
void
ExecIr_ControllerBuilderBase::_DesiredValueComputations(
    const TfToken &attributeName)
{
    using namespace exec_registration;

    // The 'explicitDesiredValue' computation only exists to provide an output
    // where desired values can be specified as overrides passed to
    // ComputeWithOverrides.
    _self.AttributeComputation(
        attributeName, ExecIrComputations->explicitDesiredValue)
        .Callback<ValueType>(+[](const VdfContext &ctx) {
            ctx.SetEmptyOutput();
        });

    // The 'computeDesiredValue' computation gets its value from the
    // 'explicitDesiredValue' computation if it provides one, or from the
    // 'computeDesiredValue' computation via incoming connections--but only if
    // there is exactly one desired value present on these inputs. Otherwise, no
    // value is returned. An error is emitted if more than one desired value is
    // present.
    _self.AttributeComputation(
        attributeName, ExecIrComputations->computeDesiredValue)
        .Callback<ValueType>(&_GetExactlyOneDesiredValue<ValueType>)
        .Inputs(
            Computation<ValueType>(
                ExecIrComputations->explicitDesiredValue),
            IncomingConnections<ValueType>(
                ExecIrComputations->computeDesiredValue));
}

template <typename ValueType>
void
ExecIr_ControllerBuilderBase::_GetExactlyOneDesiredValue(
    const VdfContext &ctx)
{
    const ValueType *const inputValue = [&]() -> const ValueType*
    {
        const ValueType *foundInputValue =
            ctx.GetInputValuePtr<ValueType>(
                ExecIrComputations->explicitDesiredValue);

        for (const ValueType &value :
                 VdfReadIteratorRange<ValueType>(
                     ctx, ExecIrComputations->computeDesiredValue)) {
            if (!foundInputValue) {
                foundInputValue = &value;
            } else {
                // TODO: We will introduce an ExecValidationErrorType enum to
                // specifically identifiy this error condition.
                TF_RUNTIME_ERROR(
                    "Found more than one desired value for node %s",
                    ctx.GetNodeDebugName().c_str());
                return nullptr;
            }
        }

        return foundInputValue;
    }();

    if (inputValue) {
        ctx.SetOutput(*inputValue);
    } else {
        ctx.SetEmptyOutput();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
