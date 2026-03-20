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
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_USING_DIRECTIVE

static ExecIrResult _Compute(const VdfContext &ctx);
static ExecIrResult _Invert(const VdfContext &ctx);

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(ExecIrFkController)
{
    ExecIrControllerBuilder builder(self, &_Compute, &_Invert);

    builder.InvertibleInputAttribute<double>(ExecIrTokens->txToken);
    builder.InvertibleInputAttribute<double>(ExecIrTokens->tyToken);
    builder.InvertibleInputAttribute<double>(ExecIrTokens->tzToken);
    builder.InvertibleInputAttribute<double>(ExecIrTokens->rxToken);
    builder.InvertibleInputAttribute<double>(ExecIrTokens->ryToken);
    builder.InvertibleInputAttribute<double>(ExecIrTokens->rzToken);
    builder.InvertibleInputAttribute<double>(ExecIrTokens->rspinToken);

    builder.NoninvertibleInputAttribute<GfMatrix4d>(
        ExecIrTokens->parentSpaceToken);

    builder.InvertibleOutputAttribute<GfMatrix4d>(ExecIrTokens->outSpaceToken);

    builder.SwitchAttribute<TfToken>(
        ExecIrTokens->rotationOrderToken);

    builder.PassthroughAttribute<GfMatrix4d>(
        ExecIrTokens->defaultSpaceToken);
}

// Returns the forward-computed result for Out:Space.
//
static ExecIrResult
_Compute(const VdfContext &ctx)
{
    const GfMatrix4d startingSpace =
        ExecIr_UtilsComputeStandardStartingSpace(ctx);

    const ExecIr_UtilsParams params = {
        startingSpace,
        ExecIr_UtilsComputeStandardTranslationOrientation(ctx, startingSpace),
        ExecIr_UtilsComputeStandardRotationOrientation(ctx, startingSpace)
    };

    const GfMatrix4d outSpaceValue = ExecIr_UtilsCompute(
        params,
        ExecIr_UtilsComputeLocalTranslation(ctx),
        ExecIr_UtilsComputeLocalRotation(ctx));

    return ExecIrResult({
        {ExecIrTokens->outSpaceToken, VtValue(outSpaceValue)}});
}

// Populates \p resultMap with inverted values that attempt to satisfy the given
// \p posedSpace.
//
static ExecIrResult
_Invert(const VdfContext &ctx)
{
    const GfMatrix4d &posedSpace =
        ctx.GetInputValue<GfMatrix4d>(ExecIrTokens->outSpaceToken);

    const GfMatrix4d startingSpace =
        ExecIr_UtilsComputeStandardStartingSpace(ctx);

    const ExecIr_UtilsParams params = {
        startingSpace,
        ExecIr_UtilsComputeStandardTranslationOrientation(ctx, startingSpace),
        ExecIr_UtilsComputeStandardRotationOrientation(ctx, startingSpace)
    };

    ExecIrResult resultMap;
    ExecIr_UtilsInvert(ctx, posedSpace, params, &resultMap);
    return resultMap;
}
