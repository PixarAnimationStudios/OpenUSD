//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/ef/time.h"

#include "pxr/base/tf/type.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/exec/vdf/executionTypeRegistry.h"

#include <bitset>
#include <climits>
#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<EfTime>();
}

TF_REGISTRY_FUNCTION(VdfExecutionTypeRegistry)
{
    VdfExecutionTypeRegistry::Define(EfTime());
}

static std::string
_SplineFlagsToString(const EfTime::SplineEvaluationFlags flags)
{
    const std::bitset<sizeof(EfTime::SplineEvaluationFlags) * CHAR_BIT>
        bits(flags);
    return bits.to_string();
} 

std::string
EfTime::GetAsString() const
{
    std::string res = _timeCode.IsDefault()
        ? "(default)"
        : std::to_string(_timeCode.GetValue());
        
    if (_timeCode.IsPreTime()) {
        res += " (pre)";
    }
   
    if (_splineFlags != 0) {
        res += _SplineFlagsToString(_splineFlags);
    }
    
    return res;
}

std::ostream &
operator<<(std::ostream &os, const EfTime &time)
{
    os << "( frame=";

    if (time.GetTimeCode().IsDefault()) {
        os << "default";
    } else {
        os << time.GetTimeCode().GetValue();
    }

    os << " location=" << (time.GetTimeCode().IsPreTime() ? "Pre" : "AtTime");

    const EfTime::SplineEvaluationFlags flags = time.GetSplineEvaluationFlags();
    os << " flags=" << _SplineFlagsToString(flags);

    return os << " )";
}

PXR_NAMESPACE_CLOSE_SCOPE
