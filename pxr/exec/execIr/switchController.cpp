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

#include "pxr/exec/exec/registerSchema.h"
#include "pxr/exec/vdf/context.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_USING_DIRECTIVE

static ExecIrResult _Compute(const VdfContext &);
static ExecIrResult _Invert(const VdfContext &);

// TODO: This switch controller is hard coded to support two rigs, each of which
// controls two joint scopes. In the future, a general switch controller schema
// will be configurable via application of multi-apply schemas.
EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(ExecIrSwitchController)
{
    ExecIrControllerBuilder builder(self, &_Compute, &_Invert);

    builder.SwitchAttribute<TfToken>(
        ExecIrSwitchControllerTokens->switchToken);

    builder.InvertibleInputAttributes<GfMatrix4d>({
        ExecIrSwitchControllerTokens->rig1Joint1Space,
        ExecIrSwitchControllerTokens->rig1Joint2Space,

        ExecIrSwitchControllerTokens->rig2Joint1Space,
        ExecIrSwitchControllerTokens->rig2Joint2Space,
    });

    builder.InvertibleOutputAttributes<GfMatrix4d>({
        ExecIrSwitchControllerTokens->outJoint1Space,
        ExecIrSwitchControllerTokens->outJoint2Space,
    });
}

// The switch controller forward computation passes through the computed values
// for the rig that is currently selected, based on the value of the swtich
// avar.
//
static ExecIrResult
_Compute(const VdfContext &ctx)
{
    const TfToken &switchValue = ctx.GetInputValue<TfToken>(
            ExecIrSwitchControllerTokens->switchToken);
    if (switchValue == ExecIrSwitchControllerTokens->rig1) {
        return {{
            {ExecIrSwitchControllerTokens->outJoint1Space,
             VtValue(ctx.GetInputValue<GfMatrix4d>(
                         ExecIrSwitchControllerTokens->rig1Joint1Space))},
            {ExecIrSwitchControllerTokens->outJoint2Space,
             VtValue(ctx.GetInputValue<GfMatrix4d>(
                         ExecIrSwitchControllerTokens->rig1Joint2Space))},
        }};
    }
    else if (switchValue == ExecIrSwitchControllerTokens->rig2) {
        return {{
            {ExecIrSwitchControllerTokens->outJoint1Space,
             VtValue(ctx.GetInputValue<GfMatrix4d>(
                         ExecIrSwitchControllerTokens->rig2Joint1Space))},
            {ExecIrSwitchControllerTokens->outJoint2Space,
             VtValue(ctx.GetInputValue<GfMatrix4d>(
                         ExecIrSwitchControllerTokens->rig2Joint2Space))},
        }};
    } else {
        TF_VERIFY(false, "Unexpected switch value '%s'", switchValue.GetText());
        return {{}};
    }
}

// The switch controller inverse computation passes through the inverted values
// for the rig that is currently selected, based on the value of the swtich
// avar.
//
static ExecIrResult
_Invert(const VdfContext &ctx)
{
    const TfToken &switchValue = ctx.GetInputValue<TfToken>(
            ExecIrSwitchControllerTokens->switchToken);
    if (switchValue == ExecIrSwitchControllerTokens->rig1) {
        return {{
            {ExecIrSwitchControllerTokens->rig1Joint1Space,
             VtValue(ctx.GetInputValue<GfMatrix4d>(
                         ExecIrSwitchControllerTokens->outJoint1Space))},
            {ExecIrSwitchControllerTokens->rig1Joint2Space,
             VtValue(ctx.GetInputValue<GfMatrix4d>(
                         ExecIrSwitchControllerTokens->outJoint2Space))},
        }};
    }
    else if (switchValue == ExecIrSwitchControllerTokens->rig2) {
        return {{
            {ExecIrSwitchControllerTokens->rig2Joint1Space,
             VtValue(ctx.GetInputValue<GfMatrix4d>(
                         ExecIrSwitchControllerTokens->outJoint1Space))},
            {ExecIrSwitchControllerTokens->rig2Joint2Space,
             VtValue(ctx.GetInputValue<GfMatrix4d>(
                         ExecIrSwitchControllerTokens->outJoint2Space))},
        }};
    } else {
        TF_VERIFY(false, "Unexpected switch value '%s'", switchValue.GetText());
        return {{}};
    }
}
