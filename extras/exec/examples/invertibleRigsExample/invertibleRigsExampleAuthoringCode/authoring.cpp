//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "authoring.h"

#include "pxr/exec/execIr/computations.h"
#include "pxr/exec/execIr/tokens.h"

#include "pxr/exec/exec/builtinComputations.h"
#include "pxr/exec/execIr/tokens.h"
#include "pxr/exec/execUsd/cacheView.h"
#include "pxr/exec/execUsd/request.h"
#include "pxr/exec/execUsd/system.h"
#include "pxr/exec/execUsd/valueKey.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/ts/spline.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/timeCode.h"

#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

InvertibleRigsExample_Authoring::InvertibleRigsExample_Authoring(
    const UsdStageRefPtr &stage)
    : _execSystem(stage)
{
    // Find all IrJointScope prims on the stage and gather up the input avars
    // and the output spaces, since we will need these to do compensation.
    //
    // TODO: This can and should be done by analysis, without relying on
    // hard-coded attribute names, and without traversing the whole
    // stage. However, this will require additional core support for invertible
    // rigs.

    static const std::vector<TfToken> inputAvarNames = {
        ExecIrTokens->avarsTx,
        ExecIrTokens->avarsTy,
        ExecIrTokens->avarsTz,
        ExecIrTokens->avarsRx,
        ExecIrTokens->avarsRy,
        ExecIrTokens->avarsRz,
        ExecIrTokens->avarsRspin
    };

    for (const UsdPrim &prim : stage->Traverse()) {
        if (prim.IsA(ExecIrTokens->IrJointScope)) {
            for (const TfToken &name : inputAvarNames) {
                if (UsdAttribute attr = prim.GetAttribute(name)) {
                    _inputAvars.push_back(attr);
                } else {
                    TF_CODING_ERROR(
                        "Failed to find attribute '%s' on <%s>",
                        name.GetText(), prim.GetPath().GetText());
                }
            }
            if (UsdAttribute attr =
                prim.GetAttribute(ExecIrTokens->posedSpace)) {
                _outputSpaces.push_back(attr);
            } else {
                TF_CODING_ERROR(
                    "Failed to find attribute '%s' on <%s>",
                    ExecIrTokens->posedSpace.GetText(),
                    prim.GetPath().GetText());
            }
        }
    }
}

static void
_SetSplineKnot(
    const UsdAttribute &attr,
    const UsdTimeCode &time,
    const VtValue &value)
{
    const TfType valueType = attr.GetTypeName().GetType();

    // Start with the spline from the attribute if there is one, or an empty
    // one otherwise.
    TsSpline spline = TsSpline(valueType); 
    if (attr.HasSpline()) {
        spline = attr.GetSpline();
    }

    // Get or create a knot at the given time.
    TsKnot knot;
    if (!spline.GetKnot(time.GetValue(), &knot)) {
        knot = TsKnot(valueType);
        knot.SetTime(time.GetValue());
    }

    // Set the knots value and interpolation for the segment after the knot.
    knot.SetValue(value);
    knot.SetNextInterpolation(TsInterpCurve);

    // Set the knot on the spline and author the spline onto the attribute.
    spline.SetKnot(knot);
    attr.SetSpline(spline);
}

// Breaks down the given attribute's spline and copies all broken down values
// (which are _at_ the given time) to the corresponding pre-time, in preparation
// for authoring other values at the given time without disturbing animation in
// the interval before that time.
//
static void
_BreakdownPreTime(
    const UsdAttribute &attr,
    const UsdTimeCode &time)
{
    // If the attribute doesn't already have a spline, author a spline with a
    // single knot with the attribute's default value.
    if (!attr.HasSpline()) {
        VtValue value;
        if (!attr.Get(&value)) {
            TF_CODING_ERROR("Can't get default value for <%s>", 
                            attr.GetPath().GetText());
            return;
        }

        _SetSplineKnot(attr, time, value);
    }

    TsSpline spline = attr.GetSpline();

    if (spline.CanBreakdown(time.GetValue())) {
        spline.Breakdown(time.GetValue());
    }
    
    VtValue value;
    TsKnot knot;
    if (!spline.GetKnot(time.GetValue(), &knot)) {
        TF_CODING_ERROR("Can't breakdown spline on <%s> at time %g",
                        attr.GetPath().GetText(), time.GetValue());
        return;
    }
    
    knot.GetValue(&value);
    knot.SetPreValue(value);
    if (!spline.SetKnot(knot)) {
        TF_CODING_ERROR("Can't set knot on spline on <%s> at time %g",
                        attr.GetPath().GetText(), time.GetValue());
        return;
    }

    attr.SetSpline(spline);
}

void
InvertibleRigsExample_Authoring::BreakdownInputAvars(
    const UsdAttribute &switchAttribute,
    const UsdTimeCode &time)
{
    // Author a time sample that breaks down the switch avar value.
    VtValue value;
    if (!switchAttribute.Get(&value, time)) {
        TF_CODING_ERROR(
            "Can't get value for switch attribute <%s> at time %s",
            switchAttribute.GetPath().GetText(), TfStringify(time).c_str());
    } else {
        switchAttribute.Set(value, time);
    }

    for (const UsdAttribute &attr : _inputAvars) {
        if (attr.HasSpline()) {
            TsSpline spline = attr.GetSpline();

            // We won't be able to break down if there is already a knot at the
            // given time, but we don't need to break down if that's the case.
            if (spline.CanBreakdown(time.GetValue())) {
                spline.Breakdown(time.GetValue());
                attr.SetSpline(spline);
            }
        }

        // If the attribute doesn't have a spline, add one with a knot that
        // breaks down the default or fallback value.
        else {
            VtValue value;
            if (!attr.Get(&value)) {
                TF_CODING_ERROR("Can't get default value for <%s>",
                                attr.GetPath().GetText());
                continue;
            }

            _SetSplineKnot(attr, time, value);
        }
    }
}

void
InvertibleRigsExample_Authoring::CompensateSwitch(
    const UsdAttribute &switchAttribute,
    const UsdTimeCode &time,
    const TfToken &switchValue)
{
    _execSystem.ChangeTime(time);

    // Breakdown all input avar values to the pre-time, in preparation for
    // setting new values at the current time as we compensate.
    for (const UsdAttribute &attr : _inputAvars) {
        _BreakdownPreTime(attr, time);
    }

    // Compute in the forward direction to get the current values of the output
    // spaces.
    std::vector<GfMatrix4d> preSwitchSpaceValues;
    {
        std::vector<ExecUsdValueKey> valueKeys;
        for (const UsdAttribute &avar : _outputSpaces) {
            valueKeys.emplace_back(avar);
        }
        const ExecUsdRequest request =
            _execSystem.BuildRequest(std::move(valueKeys));
        ExecUsdCacheView cache = _execSystem.Compute(request);

        preSwitchSpaceValues.reserve(_outputSpaces.size());
        for (size_t i=0; i<_outputSpaces.size(); ++i) {
            const VtValue value = cache.Get(i);
            TF_AXIOM(value.IsHolding<GfMatrix4d>());
            preSwitchSpaceValues.push_back(value.Get<GfMatrix4d>());
        }
    }

    // Invert to compute the input avar values that will keep the output spaces
    // the same when we author the new switch avar value.
    std::vector<double> inputAvarValues;
    {
        // Create a request containing value keys that request the
        // 'computeDesiredValue' computation for each of the input avars.
        std::vector<ExecUsdValueKey> valueKeys;
        for (const UsdAttribute &avar : _inputAvars) {
            valueKeys.emplace_back(
                avar, ExecIrComputations->computeDesiredValue);
        }
        const ExecUsdRequest request =
            _execSystem.BuildRequest(std::move(valueKeys));

        // Create overrides that specify:
        // - The new value of the switch attribute
        // - The pre-switch values of the output spaces
        ExecUsdValueOverrideVector overrides;
        overrides.reserve(_outputSpaces.size() + 1);
        overrides.push_back(
            {{switchAttribute,
              ExecBuiltinComputations->computeValue},
             VtValue(switchValue)});
        for (size_t i=0; i<_outputSpaces.size(); ++i) {
            overrides.push_back(
                {{_outputSpaces[i],
                  ExecIrComputations->explicitDesiredValue},
                 VtValue(preSwitchSpaceValues[i])});
        };

        ExecUsdCacheView cache =
            _execSystem.ComputeWithOverrides(request, std::move(overrides));

        inputAvarValues.reserve(_inputAvars.size());
        for (unsigned int i=0; i<_inputAvars.size(); ++i) {
            const VtValue value = cache.Get(i);
            inputAvarValues.push_back(value.Get<double>());
        }
    }

    // Author the switch value and the input avar values that compensate for the
    // switch value change.
    switchAttribute.Set(VtValue(switchValue), time);
    for (unsigned int i=0; i<_inputAvars.size(); ++i) {
        _SetSplineKnot(_inputAvars[i], time, VtValue(inputAvarValues[i]));
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
