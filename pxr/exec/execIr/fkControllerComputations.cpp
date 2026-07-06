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

    builder.InvertibleInputAttribute<double>(ExecIrTokens->inTx);
    builder.InvertibleInputAttribute<double>(ExecIrTokens->inTy);
    builder.InvertibleInputAttribute<double>(ExecIrTokens->inTz);
    builder.InvertibleInputAttribute<double>(ExecIrTokens->inRx);
    builder.InvertibleInputAttribute<double>(ExecIrTokens->inRy);
    builder.InvertibleInputAttribute<double>(ExecIrTokens->inRz);
    builder.InvertibleInputAttribute<double>(ExecIrTokens->inRspin);

    builder.NonInvertibleInputAttribute<GfMatrix4d>(
        ExecIrTokens->parentInSpace);
    builder.NonInvertibleInputAttribute<GfMatrix4d>(
        ExecIrTokens->parentInDefaultSpace);

    builder.InvertibleOutputAttribute<GfMatrix4d>(
        ExecIrTokens->outSpace);

    builder.SwitchAttribute<TfToken>(
        ExecIrTokens->inRotationOrder);

    builder.PassthroughAttributes<GfMatrix4d>(
        ExecIrTokens->inDefaultSpace,
        ExecIrTokens->outDefaultSpace);
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
        {ExecIrTokens->outSpace, VtValue(outSpaceValue)}});
}

// Populates \p resultMap with inverted values that attempt to satisfy the given
// \p posedSpace.
//
static ExecIrResult
_Invert(const VdfContext &ctx)
{
    const GfMatrix4d &posedSpace =
        ctx.GetInputValue<GfMatrix4d>(ExecIrTokens->outSpace);

    ExecIrResult resultMap;
    ExecIr_UtilsInvert(ctx, posedSpace, ExecIr_ComputeFkParams(ctx), &resultMap);
    return resultMap;
}
