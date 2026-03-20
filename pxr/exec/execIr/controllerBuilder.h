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
/// attributes and produce output values. Controllers also define inverse
/// computations that take desired values for outputs and produce the input
/// values necessary for the forward computation to produce the desired results.
///

#include "pxr/pxr.h"

#include "pxr/exec/execIr/tokens.h"
#include "pxr/exec/execIr/types.h"

#include "pxr/exec/exec/builtinComputations.h"
#include "pxr/exec/exec/computationBuilders.h"
#include "pxr/exec/vdf/executionTypeRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Builder class used to register invertible controller computations.
///
/// This class can only be used in the context of schema computation
/// registration. The constructor takes the `self` builder object that is
/// defined by the `EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA` macro. The
/// constructor also takes the callbacks that implement the forward and inverse
/// computations for the controller. The client uses member functions to
/// register controller atributes as inputs, outputs, switches, etc. (see the
/// documentation on the corresonding registration methods for details). These
/// registrations, in turn, generate the computation inputs for the callbacks
/// (as documented in the class function documentation), as well as other
/// computations that are required to implement invertible controllers within
/// OpenExec.
///
/// # Example
///
/// ```cpp
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
class ExecIrControllerBuilder {
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
        Exec_ComputationBuilder &self,
        Callback forwardCallback,
        Callback inverseCallback);

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
    NoninvertibleInputAttribute(
        const TfToken &attributeName);

    /// Registers an invertible output attribute; the output is inverible if \p
    /// invertible is `true`.
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

    /// Registers a passthrough attribute.
    /// 
    /// - Passthrough attributes provide input to both the forward and the
    ///   inverse computation.
    /// 
    template <typename ValueType>
    void
    PassthroughAttribute(
        const TfToken &attributeName);

private:
    // Returns a private token used to name constant inputs.
    static const TfToken &_GetConstantInputName();

private:
    Exec_ComputationBuilder &_self;
    Exec_PrimComputationBuilder _forwardComputeReg;
    Exec_PrimComputationBuilder _inverseComputeReg;
};

template <typename ValueType>
void
ExecIrControllerBuilder::InvertibleInputAttribute(
    const TfToken &attributeName)
{
    using namespace exec_registration;

    // All input attributes (invertible or not) are inputs to the forward
    // computation.
    _forwardComputeReg.Inputs(AttributeValue<ValueType>(attributeName));
}

template <typename ValueType>
void
ExecIrControllerBuilder::NoninvertibleInputAttribute(
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

    // All outputs (invertible or not) are inputs to the inverse computation.
    //
    // TODO: We pull on the resolved value because otherwise, for invertible
    // outputs, we get the computed value from the forward computation. But what
    // we really want here is the "desired value," i.e., the value the inversion
    // is trying to satisfy. For now, we use the authored value as the way the
    // desired value is specified.
    _inverseComputeReg.Inputs(
        Attribute(attributeName)
        .Computation<ValueType>(ExecBuiltinComputations->computeResolvedValue)
        .InputName(attributeName));

    // Register an expression on the invertible output that pulls its value
    // from the forward computation result map.
    _self.AttributeExpression(attributeName)
        .Callback(+[](const VdfContext &ctx) -> ValueType {
            const TfToken &attributeName =
                ctx.GetInputValue<TfToken>(_GetConstantInputName());
            const ExecIrResult &resultMap =
                ctx.GetInputValue<ExecIrResult>(ExecIrTokens->forwardCompute);
            const auto it = resultMap.find(attributeName);
            if (it != resultMap.end()) {
                return it->second.Get<ValueType>();
            }

            TF_CODING_ERROR(
                "Failed to find a result value for output attribute '%s' "
                "when computing %s",
                attributeName.GetText(),
                ctx.GetNodeDebugName().c_str());
            return VdfExecutionTypeRegistry::GetInstance()
                .GetFallback<ValueType>();
        })
        .Inputs(
            Prim().Computation<ExecIrResult>(ExecIrTokens->forwardCompute),
            Constant(attributeName).InputName(_GetConstantInputName()));
}

template <typename ValueType>
void
ExecIrControllerBuilder::SwitchAttribute(
    const TfToken &attributeName)
{
    using namespace exec_registration;

    // Switch attributes are inputs to the forward and inverse computations.
    _forwardComputeReg.Inputs(AttributeValue<ValueType>(attributeName));
    _inverseComputeReg.Inputs(AttributeValue<ValueType>(attributeName));
}

template <typename ValueType>
void
ExecIrControllerBuilder::PassthroughAttribute(
    const TfToken &attributeName)
{
    using namespace exec_registration;

    // Passthrough attributes are inputs to the forward and inverse
    // computations.
    _forwardComputeReg.Inputs(AttributeValue<ValueType>(attributeName));
    _inverseComputeReg.Inputs(AttributeValue<ValueType>(attributeName));
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
