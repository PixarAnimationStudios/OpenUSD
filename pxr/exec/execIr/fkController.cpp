//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/execIr/tokens.h"
#include "pxr/exec/execIr/types.h"
#include "pxr/exec/execIr/utils.h"

#include "pxr/exec/exec/registerSchema.h"
#include "pxr/exec/vdf/context.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_USING_DIRECTIVE

static GfMatrix4d
_Compute(const VdfContext &ctx);

static ExecIrInversionResult
_Invert(const VdfContext &ctx);

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(ExecIrFkController)
{
    self.PrimComputation(ExecIrTokens->forwardCompute)
        .Callback<GfMatrix4d>(&_Compute)
        .Inputs(
            AttributeValue<double>(ExecIrTokens->txToken),
            AttributeValue<double>(ExecIrTokens->tyToken),
            AttributeValue<double>(ExecIrTokens->tzToken),

            AttributeValue<double>(ExecIrTokens->rxToken),
            AttributeValue<double>(ExecIrTokens->ryToken),
            AttributeValue<double>(ExecIrTokens->rzToken),
            AttributeValue<double>(ExecIrTokens->rspinToken),
            AttributeValue<TfToken>(ExecIrTokens->rotationOrderToken),

            AttributeValue<GfMatrix4d>(ExecIrTokens->defaultSpaceToken),
            AttributeValue<GfMatrix4d>(ExecIrTokens->parentSpaceToken)
        )
        ;

    self.PrimComputation(ExecIrTokens->inverseCompute)
        .Callback<ExecIrInversionResult>(&_Invert)
        .Inputs(
            AttributeValue<GfMatrix4d>(ExecIrTokens->outSpaceToken),
            AttributeValue<TfToken>(ExecIrTokens->rotationOrderToken),
            AttributeValue<GfMatrix4d>(ExecIrTokens->defaultSpaceToken),
            AttributeValue<GfMatrix4d>(ExecIrTokens->parentSpaceToken)
        )
        ;
}

// Returns the forward-computed result space.
//
static GfMatrix4d
_Compute(const VdfContext &ctx)
{
    const GfMatrix4d startingSpace =
        ExecIr_UtilsComputeStandardStartingSpace(ctx);

    const ExecIr_UtilsParams params = {
        startingSpace,
        ExecIr_UtilsComputeStandardTranslationOrientation(ctx, startingSpace),
        ExecIr_UtilsComputeStandardRotationOrientation(ctx, startingSpace)
    };

    return ExecIr_UtilsCompute(
        params,
        ExecIr_UtilsComputeLocalTranslation(ctx),
        ExecIr_UtilsComputeLocalRotation(ctx));
}

// Populates \p resultMap with inverted values that attempt to satisfy the given
// \p posedSpace.
//
static ExecIrInversionResult
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

    ExecIrInversionResult resultMap;
    ExecIr_UtilsInvert(ctx, posedSpace, params, &resultMap);
    return resultMap;
}
