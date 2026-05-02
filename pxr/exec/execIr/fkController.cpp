//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/execIr/controllerBuilder.h"
#include "pxr/exec/execIr/tokens.h"
#include "pxr/exec/execIr/types.h"
#include "pxr/exec/execIr/utils.h"

#include "pxr/exec/exec/registerSchema.h"
#include "pxr/exec/vdf/context.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_USING_DIRECTIVE

static ExecIrResult _Compute(const VdfContext &);
static ExecIrResult _Invert(const VdfContext &);

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(ExecIrFkController)
{
    ExecIrControllerBuilder builder(self, &_Compute, &_Invert);

    builder.InvertibleInputAttribute<double>(ExecIrFkControllerTokens->inTx);
    builder.InvertibleInputAttribute<double>(ExecIrFkControllerTokens->inTy);
    builder.InvertibleInputAttribute<double>(ExecIrFkControllerTokens->inTz);
    builder.InvertibleInputAttribute<double>(ExecIrFkControllerTokens->inRx);
    builder.InvertibleInputAttribute<double>(ExecIrFkControllerTokens->inRy);
    builder.InvertibleInputAttribute<double>(ExecIrFkControllerTokens->inRz);
    builder.InvertibleInputAttribute<double>(ExecIrFkControllerTokens->inRspin);

    builder.NonInvertibleInputAttribute<GfMatrix4d>(
        ExecIrFkControllerTokens->parentInSpace);
    builder.NonInvertibleInputAttribute<GfMatrix4d>(
        ExecIrFkControllerTokens->parentInDefaultSpace);

    builder.InvertibleOutputAttribute<GfMatrix4d>(
        ExecIrFkControllerTokens->outSpace);

    builder.SwitchAttribute<TfToken>(
        ExecIrFkControllerTokens->inRotationOrder);

    builder.PassthroughAttributes<GfMatrix4d>(
        ExecIrFkControllerTokens->inDefaultSpace,
        ExecIrFkControllerTokens->outDefaultSpace);
}

// Returns the forward-computed result for out:space.
//
static ExecIrResult
_Compute(const VdfContext &ctx)
{
    const GfMatrix4d outSpaceValue = ExecIr_UtilsCompute(
        ExecIr_ComputeFkParams(ctx),
        ExecIr_UtilsComputeLocalTranslation(ctx),
        ExecIr_UtilsComputeLocalRotation(ctx));

    return ExecIrResult({
        {ExecIrFkControllerTokens->outSpace, VtValue(outSpaceValue)}});
}

// Populates \p resultMap with inverted values that attempt to satisfy the given
// \p posedSpace.
//
static ExecIrResult
_Invert(const VdfContext &ctx)
{
    const GfMatrix4d &posedSpace =
        ctx.GetInputValue<GfMatrix4d>(ExecIrFkControllerTokens->outSpace);

    ExecIrResult resultMap;
    ExecIr_UtilsInvert(ctx, posedSpace, ExecIr_ComputeFkParams(ctx), &resultMap);
    return resultMap;
}
