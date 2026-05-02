//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/execIr/controllerBuilder.h"
#include "pxr/exec/execIr/tokens.h"
#include "pxr/exec/execIr/utils.h"

#include "pxr/base/gf/matrix3d.h"
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
// TODO: When the OpenExec core provides builtin support for attribute
// connection data flow and for inversion, we won't need to define any of the
// plugin computations defined by this builder.
// 
class _Builder {
public:
    _Builder(ExecComputationBuilder &self)
        : _self(self)
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
    // Finds the nearest namespace ancestor that is either IrTransformable or
    // Xformable:
    // - If the ancestor is IrTransformable, yields the value of the attribute
    //   named \p spaceAttributeName.
    // - If the ancestor is Xformable, yields the value of the ancestor's
    //   computeLocalToWorldTransform computation.
    //
    // This is done by defining (for IrTransformable prims) and dispatching (for
    // Xformable prims) a computation named \p ancestorComputationName.
    //
    void
    InheritedTransformationComputation(
        const TfToken &computationName,
        const TfToken &ancestorComputationName,
        const TfToken &spaceAttributeName);

private:
    // Registers an expression that implements dataflow across attribute
    // connections.
    //
    // TODO: This expression won't be needed when the OpenExec core defines
    // default attribute connection dataflow behavior for all attributes.
    template <typename ValueType>
    void
    _ConnectionDataflowExpression(
        const TfToken &attributeName);

    // Returns a pointer to the desired value provided by the
    // 'explicitDesiredValue' and 'computedDesiredValue' inputs; otherwise,
    // returns null.
    //
    // If more than one input value is provided, an error is emitted.
    //
    template <typename ValueType>
    static const ValueType *
    _GetExactlyOneDesiredValue(
        const VdfContext &ctx);

private:
    ExecComputationBuilder &_self;
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

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(ExecIrJointScope)
{
    _Builder builder(self);

    builder.InputAttribute<double>(ExecIrTransformableTokens->avarsTx);
    builder.InputAttribute<double>(ExecIrTransformableTokens->avarsTy);
    builder.InputAttribute<double>(ExecIrTransformableTokens->avarsTz);
    builder.InputAttribute<double>(ExecIrTransformableTokens->avarsRx);
    builder.InputAttribute<double>(ExecIrTransformableTokens->avarsRy);
    builder.InputAttribute<double>(ExecIrTransformableTokens->avarsRz);
    builder.InputAttribute<double>(ExecIrTransformableTokens->avarsRspin);
    builder.InputAttribute<TfToken>(
        ExecIrTransformableTokens->avarsRotationOrder);
    builder.InputAttribute<GfMatrix4d>(
        ExecIrTransformableTokens->avarsDefaultSpace);

    // avars:defaultSpace has an expression that simply returns the value of
    // default:space.
    self.AttributeExpression(ExecIrTransformableTokens->avarsDefaultSpace)
        .Inputs(Prim().AttributeValue<GfMatrix4d>(
                    ExecIrTransformableTokens->defaultSpace))
        .Callback(+[](const VdfContext &ctx) -> GfMatrix4d {
            return ctx.GetInputValue<GfMatrix4d>(
                ExecIrTransformableTokens->defaultSpace);
        });

    builder.InputAttribute<double>(
        ExecIrTransformableTokens->avarsUnitScaleFactor);

    builder.InputAttribute<double>(ExecIrTransformableTokens->restTx);
    builder.InputAttribute<double>(ExecIrTransformableTokens->restTy);
    builder.InputAttribute<double>(ExecIrTransformableTokens->restTz);
    builder.InputAttribute<double>(ExecIrTransformableTokens->restRx);
    builder.InputAttribute<double>(ExecIrTransformableTokens->restRy);
    builder.InputAttribute<double>(ExecIrTransformableTokens->restRz);
    builder.InputAttribute<GfMatrix4d>(ExecIrTransformableTokens->restSpace);

    builder.InputAttribute<double>(ExecIrTransformableTokens->defaultTx);
    builder.InputAttribute<double>(ExecIrTransformableTokens->defaultTy);
    builder.InputAttribute<double>(ExecIrTransformableTokens->defaultTz);
    builder.InputAttribute<double>(ExecIrTransformableTokens->defaultRx);
    builder.InputAttribute<double>(ExecIrTransformableTokens->defaultRy);
    builder.InputAttribute<double>(ExecIrTransformableTokens->defaultRz);
    builder.InputAttribute<GfMatrix4d>(ExecIrTransformableTokens->defaultSpace);

    // Compute default:space by taking the offsets defined by the default
    // scalars and combining them with the parent default space.
    // 
    // Default space is a world space transform representing the 'zero' position
    // for posing. This may be different from the rest pose in order to provide
    // default scaling for a character, to make variants, or to set a more
    // natural starting place for animation controls. Default space combines the
    // effect of that local transform with the rest space offset.
    self.AttributeExpression(ExecIrTransformableTokens->defaultSpace)
        .Inputs(
            Prim().Computation<GfMatrix4d>(_tokens->defaultTransRotOffsetXf),
            Prim().Computation<GfMatrix4d>(_tokens->localRestXf),
            Prim().AttributeValue<GfMatrix4d>(
                ExecIrTransformableTokens->parentDefaultSpace))
        .Callback(+[](const VdfContext &ctx) -> GfMatrix4d {
            return ExecIr_ComputeDefaultSpace(
                ctx.GetInputValue<GfMatrix4d>(_tokens->defaultTransRotOffsetXf),
                GfMatrix4d(1), // defaultScaleXf
                ctx.GetInputValue<GfMatrix4d>(_tokens->localRestXf),
                ctx.GetInputValue<GfMatrix4d>(
                    ExecIrTransformableTokens->parentDefaultSpace));
        });

    // The defaultTransRotOffsetXf computation represents the local authored
    // offset from where defaultSpace would normally be relative to the
    // parent. Note that we don't pass in handedness here, because this offset
    // will be applied to rest space, which already incorporates handedness.
    self.PrimComputation(_tokens->defaultTransRotOffsetXf)
        .Inputs(
            AttributeValue<double>(ExecIrTransformableTokens->defaultTx),
            AttributeValue<double>(ExecIrTransformableTokens->defaultTy),
            AttributeValue<double>(ExecIrTransformableTokens->defaultTz),
            AttributeValue<double>(ExecIrTransformableTokens->defaultRx),
            AttributeValue<double>(ExecIrTransformableTokens->defaultRy),
            AttributeValue<double>(ExecIrTransformableTokens->defaultRz))
        .Callback(+[](const VdfContext &ctx) {
            return ExecIr_ComputeLocalXf(
                ctx.GetInputValue<double>(ExecIrTransformableTokens->defaultTx),
                ctx.GetInputValue<double>(ExecIrTransformableTokens->defaultTy),
                ctx.GetInputValue<double>(ExecIrTransformableTokens->defaultTz),
                0.0, // rSpin
                ctx.GetInputValue<double>(ExecIrTransformableTokens->defaultRx),
                ctx.GetInputValue<double>(ExecIrTransformableTokens->defaultRy),
                ctx.GetInputValue<double>(ExecIrTransformableTokens->defaultRz),
                ExecIrRotationOrderTokens->XYZ,
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
    self.AttributeExpression(ExecIrTransformableTokens->restSpace)
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
            AttributeValue<double>(ExecIrTransformableTokens->restTx),
            AttributeValue<double>(ExecIrTransformableTokens->restTy),
            AttributeValue<double>(ExecIrTransformableTokens->restTz),
            AttributeValue<double>(ExecIrTransformableTokens->restRx),
            AttributeValue<double>(ExecIrTransformableTokens->restRy),
            AttributeValue<double>(ExecIrTransformableTokens->restRz))
        .Callback(+[](const VdfContext &ctx) {
            return ExecIr_ComputeLocalXf(
                ctx.GetInputValue<double>(ExecIrTransformableTokens->restTx),
                ctx.GetInputValue<double>(ExecIrTransformableTokens->restTy),
                ctx.GetInputValue<double>(ExecIrTransformableTokens->restTz),
                0.0, // rSpin
                ctx.GetInputValue<double>(ExecIrTransformableTokens->restRx),
                ctx.GetInputValue<double>(ExecIrTransformableTokens->restRy),
                ctx.GetInputValue<double>(ExecIrTransformableTokens->restRz),
                ExecIrRotationOrderTokens->XYZ,
                ctx);
        });

    // Compute local transform that defines the delta from the parent.
    self.PrimComputation(_tokens->localRestXf)
        .Inputs(
            AttributeValue<GfMatrix4d>(ExecIrTransformableTokens->restSpace),
            Computation<GfMatrix4d>(_tokens->parentRestSpace))
        .Callback(+[](const VdfContext &ctx) {
            return
                ctx.GetInputValue<GfMatrix4d>(
                    ExecIrTransformableTokens->restSpace) *
                ctx.GetInputValue<GfMatrix4d>(
                    _tokens->parentRestSpace).GetInverse();
        });

    // Define the parentRestSpace computation, which computes the parent rest
    // space.
    builder.InheritedTransformationComputation(
        _tokens->parentRestSpace,
        _tokens->computedRestSpace,
        ExecIrTransformableTokens->restSpace);

    builder.OutputAttribute<GfMatrix4d>(ExecIrTransformableTokens->posedSpace);
    builder.OutputAttribute<GfMatrix4d>(
        ExecIrTransformableTokens->posedDefaultSpace);
    
    builder.InputAttribute<GfMatrix4d>(ExecIrTransformableTokens->parentSpace);
    builder.InputAttribute<GfMatrix4d>(
        ExecIrTransformableTokens->parentDefaultSpace);

    self.AttributeExpression(ExecIrTransformableTokens->parentDefaultSpace)
        .Inputs(Prim().Computation<GfMatrix4d>(_tokens->parentDefaultSpace))
        .Callback(+[](const VdfContext &ctx) -> GfMatrix4d {
            return ctx.GetInputValue<GfMatrix4d>(_tokens->parentDefaultSpace);
        });

    // Define the parentDefaultSpace computation, which computes the parent
    // default space.
    builder.InheritedTransformationComputation(
        _tokens->parentDefaultSpace,
        _tokens->computedDefaultSpace,
        ExecIrTransformableTokens->defaultSpace);
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
    //
    // TODO: This plugin computation won't be necessary when OpenExec provides
    // core inversion support.
    _self.AttributeComputation(
        attributeName, ExecIrComputationTokens->computeDesiredValue)
        .Callback<ValueType>(+[](const VdfContext &ctx) {
            if (const ValueType *const valuePtr =
                _GetExactlyOneDesiredValue<ValueType>(ctx)) {
                ctx.SetOutput(*valuePtr);
            } else {
                ctx.SetEmptyOutput();
            }
        })
        .Inputs(
            IncomingConnections<ValueType>(
                ExecIrComputationTokens->computeDesiredValue));
}

template <typename ValueType>
void
_Builder::OutputAttribute(const TfToken &attributeName)
{
    using namespace exec_registration;

    // Output attributes support dataflow across connections.
    _ConnectionDataflowExpression<ValueType>(attributeName);

    // The 'explicitDesiredValue' computation only exists to provide an
    // output where desired values can be specified as overrides passed to
    // ComputeWithOverrides.
    //
    // TODO: This plugin computation won't be necessary when OpenExec
    // provides core inversion support.
    _self.AttributeComputation(
        attributeName, ExecIrComputationTokens->explicitDesiredValue)
        .Callback<ValueType>(+[](const VdfContext &ctx) {
            ctx.SetEmptyOutput();
        });

    // The 'computeDesiredValue' computation gets its value from the
    // 'explicitDesiredValue' computation if it provides one, or the
    // `computeDesiredValue` via incoming connections if it provides
    // one. Otherwise, no value is returned.
    //
    // TODO: This plugin computation won't be necessary when OpenExec
    // provides core inversion support.
    _self.AttributeComputation(
        attributeName, ExecIrComputationTokens->computeDesiredValue)
        .Callback<ValueType>(+[](const VdfContext &ctx) {
            if (const ValueType *const valuePtr =
                ctx.GetInputValuePtr<ValueType>(
                    ExecIrComputationTokens->explicitDesiredValue)) {
                ctx.SetOutput(*valuePtr);
            } else if (const ValueType *const valuePtr =
                       ctx.GetInputValuePtr<ValueType>(
                           ExecIrComputationTokens->computeDesiredValue)) {
                ctx.SetOutput(*valuePtr);
            } else {
                ctx.SetEmptyOutput();
            }
        })
        .Inputs(
            Computation<ValueType>(
                ExecIrComputationTokens->explicitDesiredValue),
            IncomingConnections<ValueType>(
                ExecIrComputationTokens->computeDesiredValue));
}

template <typename ValueType>
void
_Builder::_ConnectionDataflowExpression(
    const TfToken &attributeName)
{
    using namespace exec_registration;

    _self.AttributeExpression(attributeName)
        .Inputs(
            Connections<ValueType>(
                ExecBuiltinComputations->computeValue),
            Computation<ValueType>(
                ExecBuiltinComputations->computeResolvedValue))
        .Callback(+[](const VdfContext &ctx) -> ValueType {
            // A value that flows across a connection takes precedence.
            if (const ValueType *const connectedValuePtr =
                ctx.GetInputValuePtr<ValueType>(
                    ExecBuiltinComputations->computeValue)) {
                return *connectedValuePtr;
            }

            // Otherwise, the attribute's resolved value is its computed value.
            return ctx.GetInputValue<ValueType>(
                ExecBuiltinComputations->computeResolvedValue);
        });
}

template <typename ValueType>
const ValueType *
_Builder::_GetExactlyOneDesiredValue(const VdfContext &ctx)
{
    const ValueType *foundInputValue = 
        ctx.GetInputValuePtr<ValueType>(
            ExecIrComputationTokens->explicitDesiredValue);

    for (const ValueType &value :
             VdfReadIteratorRange<ValueType>(
                 ctx, ExecIrComputationTokens->computeDesiredValue)) {
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

    // Define an ancestorComputationName computation for all IrTransformable
    // prims that returns the value of the attribute with the name given by
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
