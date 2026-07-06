//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/execIr/computations.h"
#include "pxr/exec/execIr/controllerBuilder.h"
#include "pxr/exec/execIr/tokens.h"
#include "pxr/exec/execIr/utils.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/exec/exec/registerSchema.h"
#include "pxr/exec/execGeom/tokens.h"
#include "pxr/exec/vdf/context.h"
#include "pxr/usd/usdGeom/xformable.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Builder used to register computations for input and output attributes.
//
// TODO: When the OpenExec core provides builtin support for inversion, we 
// won't need to define any of the plugin computations defined by this builder.
// 
class _Builder : ExecIr_ControllerBuilderBase {
public:
    _Builder(ExecComputationBuilder &self)
        : ExecIr_ControllerBuilderBase(self)
    {}

    // Defines the computations needed for an attribute that provides input
    // values for an invertible controller, via an attribute connection, when
    // performing a forward computation.
    //
    template <typename ValueType>
    void InputAttribute(const TfToken &attributeName);

    // Defines the computations needed for an attribute that receives values
    // values from an invertible controller output, via an attribute connection,
    // when performing a forward computation.
    //
    template <typename ValueType>
    void OutputAttribute(const TfToken &attributeName);

    // Defines a computation named \p computationName that computes a
    // transformation inherited from a namespace ancestor.
    //
    // Finds the nearest namespace ancestor that is either IrXformable or
    // Xformable:
    // - If the ancestor is IrXformable, yields the value of the attribute named
    //   \p spaceAttributeName.
    // - If the ancestor is Xformable, yields the value of the ancestor's
    //   computeLocalToWorldTransform computation.
    //
    // This is done by defining (for IrXformable prims) and dispatching (for
    // Xformable prims) a computation named \p ancestorComputationName.
    //
    void
    InheritedTransformationComputation(
        const TfToken &computationName,
        const TfToken &ancestorComputationName,
        const TfToken &spaceAttributeName);
};

} // anonymous namespace

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (computedDefaultSpace)
    (computedRestSpace)
    (defaultTransRotOffsetXf)
    (localRestXf)
    (parentDefaultSpace)
    (parentRestSpace)
    (restTransRotOffsetXf)
);

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(ExecIrXformable)
{
    _Builder builder(self);

    builder.InputAttribute<double>(ExecIrTokens->avarsTx);
    builder.InputAttribute<double>(ExecIrTokens->avarsTy);
    builder.InputAttribute<double>(ExecIrTokens->avarsTz);
    builder.InputAttribute<double>(ExecIrTokens->avarsRx);
    builder.InputAttribute<double>(ExecIrTokens->avarsRy);
    builder.InputAttribute<double>(ExecIrTokens->avarsRz);
    builder.InputAttribute<double>(ExecIrTokens->avarsRspin);
    builder.InputAttribute<TfToken>(
        ExecIrTokens->avarsRotationOrder);
    builder.InputAttribute<GfMatrix4d>(
        ExecIrTokens->avarsDefaultSpace);

    // avars:defaultSpace has an expression that simply returns the value of
    // default:space.
    self.AttributeExpression(ExecIrTokens->avarsDefaultSpace)
        .Inputs(Prim().AttributeValue<GfMatrix4d>(
                    ExecIrTokens->defaultSpace))
        .Callback(+[](const VdfContext &ctx) -> GfMatrix4d {
            return ctx.GetInputValue<GfMatrix4d>(
                ExecIrTokens->defaultSpace);
        });

    builder.InputAttribute<double>(
        ExecIrTokens->avarsUnitScaleFactor);

    builder.InputAttribute<double>(ExecIrTokens->restTx);
    builder.InputAttribute<double>(ExecIrTokens->restTy);
    builder.InputAttribute<double>(ExecIrTokens->restTz);
    builder.InputAttribute<double>(ExecIrTokens->restRx);
    builder.InputAttribute<double>(ExecIrTokens->restRy);
    builder.InputAttribute<double>(ExecIrTokens->restRz);
    builder.InputAttribute<GfMatrix4d>(ExecIrTokens->restSpace);

    builder.InputAttribute<double>(ExecIrTokens->defaultTx);
    builder.InputAttribute<double>(ExecIrTokens->defaultTy);
    builder.InputAttribute<double>(ExecIrTokens->defaultTz);
    builder.InputAttribute<double>(ExecIrTokens->defaultRx);
    builder.InputAttribute<double>(ExecIrTokens->defaultRy);
    builder.InputAttribute<double>(ExecIrTokens->defaultRz);
    builder.InputAttribute<GfMatrix4d>(ExecIrTokens->defaultSpace);

    // Compute default:space by taking the offsets defined by the default
    // scalars and combining them with the parent default space.
    // 
    // Default space is a world space transform representing the 'zero' position
    // for posing. This may be different from the rest pose in order to provide
    // default scaling for a character, to make variants, or to set a more
    // natural starting place for animation controls. Default space combines the
    // effect of that local transform with the rest space offset.
    self.AttributeExpression(ExecIrTokens->defaultSpace)
        .Inputs(
            Prim().Computation<GfMatrix4d>(_tokens->defaultTransRotOffsetXf),
            Prim().Computation<GfMatrix4d>(_tokens->localRestXf),
            Prim().AttributeValue<GfMatrix4d>(
                ExecIrTokens->parentDefaultSpace))
        .Callback(+[](const VdfContext &ctx) -> GfMatrix4d {
            return ExecIr_ComputeDefaultSpace(
                ctx.GetInputValue<GfMatrix4d>(_tokens->defaultTransRotOffsetXf),
                GfMatrix4d(1), // defaultScaleXf
                ctx.GetInputValue<GfMatrix4d>(_tokens->localRestXf),
                ctx.GetInputValue<GfMatrix4d>(
                    ExecIrTokens->parentDefaultSpace));
        });

    // The defaultTransRotOffsetXf computation represents the local authored
    // offset from where defaultSpace would normally be relative to the
    // parent. Note that we don't pass in handedness here, because this offset
    // will be applied to rest space, which already incorporates handedness.
    self.PrimComputation(_tokens->defaultTransRotOffsetXf)
        .Inputs(
            AttributeValue<double>(ExecIrTokens->defaultTx),
            AttributeValue<double>(ExecIrTokens->defaultTy),
            AttributeValue<double>(ExecIrTokens->defaultTz),
            AttributeValue<double>(ExecIrTokens->defaultRx),
            AttributeValue<double>(ExecIrTokens->defaultRy),
            AttributeValue<double>(ExecIrTokens->defaultRz))
        .Callback(+[](const VdfContext &ctx) {
            return ExecIr_ComputeLocalXf(
                ctx.GetInputValue<double>(ExecIrTokens->defaultTx),
                ctx.GetInputValue<double>(ExecIrTokens->defaultTy),
                ctx.GetInputValue<double>(ExecIrTokens->defaultTz),
                0.0, // rSpin
                ctx.GetInputValue<double>(ExecIrTokens->defaultRx),
                ctx.GetInputValue<double>(ExecIrTokens->defaultRy),
                ctx.GetInputValue<double>(ExecIrTokens->defaultRz),
                ExecIrTokens->XYZ,
                ctx);
        });

    // rest:space is the value of the restTransRotOffsetXf computation relative
    // to the parentRestSpace.
    // 
    // Rest space is a world space transform representing the position of this
    // transformable object before any posing has happened. The fallback value
    // combines the effect of the restTx/y/z/... attributes to yield a local
    // transform. restSpace inherits from the transform parent. The connected
    // value is orthonormalized.
    self.AttributeExpression(ExecIrTokens->restSpace)
        .Inputs(
            Prim().Computation<GfMatrix4d>(_tokens->restTransRotOffsetXf),
            Prim().Computation<GfMatrix4d>(_tokens->parentRestSpace))
        .Callback(+[](const VdfContext &ctx) {
            return
                ctx.GetInputValue<GfMatrix4d>(_tokens->restTransRotOffsetXf) *
                ctx.GetInputValue<GfMatrix4d>(_tokens->parentRestSpace);
        });

    // Compute the local transform that is the result of combining the authored
    // values of the rest space scalars.
    self.PrimComputation(_tokens->restTransRotOffsetXf)
        .Inputs(
            AttributeValue<double>(ExecIrTokens->restTx),
            AttributeValue<double>(ExecIrTokens->restTy),
            AttributeValue<double>(ExecIrTokens->restTz),
            AttributeValue<double>(ExecIrTokens->restRx),
            AttributeValue<double>(ExecIrTokens->restRy),
            AttributeValue<double>(ExecIrTokens->restRz))
        .Callback(+[](const VdfContext &ctx) {
            return ExecIr_ComputeLocalXf(
                ctx.GetInputValue<double>(ExecIrTokens->restTx),
                ctx.GetInputValue<double>(ExecIrTokens->restTy),
                ctx.GetInputValue<double>(ExecIrTokens->restTz),
                0.0, // rSpin
                ctx.GetInputValue<double>(ExecIrTokens->restRx),
                ctx.GetInputValue<double>(ExecIrTokens->restRy),
                ctx.GetInputValue<double>(ExecIrTokens->restRz),
                ExecIrTokens->XYZ,
                ctx);
        });

    // Compute local transform that defines the delta from the parent.
    self.PrimComputation(_tokens->localRestXf)
        .Inputs(
            AttributeValue<GfMatrix4d>(ExecIrTokens->restSpace),
            Computation<GfMatrix4d>(_tokens->parentRestSpace))
        .Callback(+[](const VdfContext &ctx) {
            return
                ctx.GetInputValue<GfMatrix4d>(
                    ExecIrTokens->restSpace) *
                ctx.GetInputValue<GfMatrix4d>(
                    _tokens->parentRestSpace).GetInverse();
        });

    // Define the parentRestSpace computation, which computes the parent rest
    // space.
    builder.InheritedTransformationComputation(
        _tokens->parentRestSpace,
        _tokens->computedRestSpace,
        ExecIrTokens->restSpace);

    builder.OutputAttribute<GfMatrix4d>(ExecIrTokens->posedSpace);
    builder.OutputAttribute<GfMatrix4d>(
        ExecIrTokens->posedDefaultSpace);
    
    builder.InputAttribute<GfMatrix4d>(ExecIrTokens->parentSpace);
    builder.InputAttribute<GfMatrix4d>(
        ExecIrTokens->parentDefaultSpace);

    self.AttributeExpression(ExecIrTokens->parentDefaultSpace)
        .Inputs(Prim().Computation<GfMatrix4d>(_tokens->parentDefaultSpace))
        .Callback(+[](const VdfContext &ctx) -> GfMatrix4d {
            return ctx.GetInputValue<GfMatrix4d>(_tokens->parentDefaultSpace);
        });

    // Define the parentDefaultSpace computation, which computes the parent
    // default space.
    builder.InheritedTransformationComputation(
        _tokens->parentDefaultSpace,
        _tokens->computedDefaultSpace,
        ExecIrTokens->defaultSpace);
}

template <typename ValueType>
void
_Builder::InputAttribute(const TfToken &attributeName)
{
    using namespace exec_registration;

    // The 'computeDesiredValue' computation gets its value from the
    // 'computeDesiredValue' computation via incoming connections--but only if
    // there is exactly one desired value present. Otherwise, no value is
    // returned. An error is emitted if more than one desired value is present.
    _self.AttributeComputation(
        attributeName, ExecIrComputations->computeDesiredValue)
        .Callback<ValueType>(&_GetExactlyOneDesiredValue<ValueType>)
        .Inputs(
            IncomingConnections<ValueType>(
                ExecIrComputations->computeDesiredValue));
}

template <typename ValueType>
void
_Builder::OutputAttribute(const TfToken &attributeName)
{
    // Output attributes support computing desired values, for inversion.
    _DesiredValueComputations<ValueType>(attributeName);
}

void
_Builder::InheritedTransformationComputation(
    const TfToken &computationName,
    const TfToken &ancestorComputationName,
    const TfToken &spaceAttributeName)
{
    using namespace exec_registration;

    // This computation finds the ancestorComputationName computation on the
    // nearest namespace ancestor that defines it.
    _self.PrimComputation(computationName)
        .Inputs(
            NamespaceAncestor<GfMatrix4d>(ancestorComputationName)
                .FallsBackToDispatched())
        .Callback<GfMatrix4d>([ancestorComputationName](const VdfContext &ctx) {
            const GfMatrix4d *const valuePtr =
                ctx.GetInputValuePtr<GfMatrix4d>(ancestorComputationName);
            ctx.SetOutput(valuePtr ? *valuePtr : GfMatrix4d(1));
        });

    // Define an ancestorComputationName computation for all IrXformable prims
    // that returns the value of the attribute with the name given by
    // spaceAttributeName.
    _self.PrimComputation(ancestorComputationName)
        .Inputs(AttributeValue<GfMatrix4d>(spaceAttributeName))
        .Callback<GfMatrix4d>([spaceAttributeName](const VdfContext &ctx) {
            ctx.SetOutputToReferenceInput(spaceAttributeName);
        });

    // Dispatch an ancestorComputationName computation onto all Xformable prims
    // that returns the value of the 'computeLocalToWorldTransform' computation.
    _self.DispatchedPrimComputation(
        ancestorComputationName,
        TfType::Find<UsdGeomXformable>())
        .Inputs(Computation<GfMatrix4d>(
                    ExecGeomXformableTokens->computeLocalToWorldTransform))
        .Callback<GfMatrix4d>(+[](const VdfContext &ctx) {
            ctx.SetOutputToReferenceInput(
                ExecGeomXformableTokens->computeLocalToWorldTransform);
        });
}
