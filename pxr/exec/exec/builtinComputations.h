//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_BUILTIN_COMPUTATIONS_H
#define PXR_EXEC_EXEC_BUILTIN_COMPUTATIONS_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"

#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

class Exec_BuiltinComputationRegistry;

/// Tokens representing the built-in computations available on various provider
/// types.
///
/// \defgroup group_Exec_Builtin_Computations Builtin Exec Computations
///
/// These tokens can all be used in [input
/// registrations](#group_Exec_InputRegistrations) to request input values for
/// plugin computations. They can also be passed to compute APIs to request
/// computed values.
///
/// These computation tokens are publicly accessible by dereferencing the
/// `ExecBuiltinComputationTokens` static data.
///
class Exec_BuiltinComputationTokens
{
public:
    EXEC_API
    Exec_BuiltinComputationTokens();

    /// \defgroup group_Mf_ExecBuiltinComputations_Stage Stage Computations
    /// 
    /// Builtin computations for computing information about the provider's
    /// stage.
    /// 
    /// \ingroup group_Exec_Builtin_Computations
    /// @{

    /// Computes the current time on the stage.
    ///
    /// \returns an EfTime value.
    ///
    /// \note
    /// The computation provider must be the stage.
    ///
    /// # Example
    /// 
    /// ```{.cpp}
    /// self.PrimComputation(_tokens->myComputation)
    ///     .Callback<EfTime>(&_MyCallback)
    ///     .Inputs(
    ///         Stage()
    ///             .Computation<EfTime>(ExecBuiltinComputations->computeTime)
    ///     );
    /// ```
    ///
    /// \hideinitializer
    const TfToken computeTime;

    /// @} // Stage computations


    /// \defgroup group_Exec_Attribute_Comptuations Attribute Computations
    /// 
    /// Builtin computations for computing information about attributes.
    /// 
    /// \ingroup group_Exec_Builtin_Computations
    /// @{

    /// Computes the provider attribute's value.
    ///
    /// \returns a value whose type is the provider attribute's scalar value
    /// type. If the attribute has registered an
    /// [attribute expression](#Exec_ComputationBuilder::AttributeExpression),
    /// this may produce a value of any type.
    ///
    /// \note
    /// The computation provider must be an attribute.
    ///
    /// # Example
    /// 
    /// ```{.cpp}
    /// self.PrimComputation(_tokens->myComputation)
    ///     .Callback<double>(&_MyCallback)
    ///     .Inputs(
    ///         Attribute(_tokens->myAttribute)
    ///             .Computation<double>(ExecBuiltinComputations->computeValue)
    ///             .Required()
    ///     );
    /// ```
    ///
    /// \hideinitializer
    const TfToken computeValue;

    /// Computes the provider attribute's resolved value as authored in scene
    /// description.
    ///
    /// This computation always produces the resolved value of an attribute,
    /// even if an attribute has registered an
    /// [attribute expression](#Exec_ComputationBuilder::AttributeExpression).
    ///
    /// \returns a value whose type is the provider attribute's scalar value
    /// type.
    ///
    /// # Example
    ///
    /// ```{.cpp}
    /// self.PrimComputation(_tokens->myComputation)
    ///     .Callback<double>(&_MyCallback)
    ///     .Inputs(
    ///         Attribute(_tokens->myAttribute)
    ///             .Computation(ExecBuiltinComputations->computeResolvedValue)
    ///     );
    /// ```
    ///
    /// \hideinitializer
    const TfToken computeResolvedValue;

    /// Computes the provider's scene path.
    ///
    /// \returns the path of the provider object, as an SdfPath.
    ///
    /// # Example
    ///
    /// ```{.cpp}
    /// self.PrimComputation(_tokens->pathAsString)
    ///     .Callback<std::string>(+[](const VdfContext &ctx) {
    ///         return ctx.GetInputValue<SdfPath>(
    ///             ExecBuiltinComputations->computePath).GetString();
    ///     })
    ///     .Inputs(
    ///         Computation(ExecBuiltinComputations->computePath)
    ///     );
    /// ```
    ///
    /// \hideinitializer
    const TfToken computePath;

    /// @} // Attribute computations

private:
    Exec_BuiltinComputationTokens(Exec_BuiltinComputationRegistry &registry);
};

// Used to publicly access builtin computation tokens.
EXEC_API
extern TfStaticData<Exec_BuiltinComputationTokens> ExecBuiltinComputations;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
