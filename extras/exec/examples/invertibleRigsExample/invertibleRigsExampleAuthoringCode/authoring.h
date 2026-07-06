//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTRAS_EXEC_EXAMPLES_INVERTIBLE_RIGS_EXAMPLE_AUTHORING_H
#define PXR_EXTRAS_EXEC_EXAMPLES_INVERTIBLE_RIGS_EXAMPLE_AUTHORING_H

#include "pxr/pxr.h"

#include "api.h"

#include "pxr/exec/execUsd/system.h"
#include "pxr/usd/usd/attribute.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdTimeCode;
class VtValue;

/// Class that implements authoring functionality needed to demonstrate an
/// invertible rig with compensation.
///
class InvertibleRigsExample_Authoring {
public:
    EXEC_INVERTIBLERIGSEXAMPLE_API
    InvertibleRigsExample_Authoring(const UsdStageRefPtr &stage);

    /// For each input avar, authors a knot or a time sample at the given time,
    /// with the value of the avar at that time.
    ///
    /// This operation can be used to preserve the pose at the given time, in
    /// preparation for authoring knots at other times, which might otherwise
    /// cause the pose at the broken down time to change, due to interpolation
    /// or extrapolation caused by spline knots or time samples authored at
    /// times before or after the breakdown time.
    ///
    /// \warning
    /// This function currently hardcodes the set of input avars in the rig used
    /// in testSwitchCompensation. A production-ready version of this function
    /// would find the input avars by analysis, based on the switch attribute,
    /// but that requires additional core support for invertible rigs.
    /// 
    EXEC_INVERTIBLERIGSEXAMPLE_API
    void
    BreakdownInputAvars(
        const UsdAttribute &switchAttribute,
        const UsdTimeCode &time);

    /// Authors a value to a switch attribute with compensation that authors
    /// input avar values such that the overall pose is the same before and
    /// after the change to the switch attribute value.
    /// 
    /// Switching control rigs with compensation is a primary workflow supported
    /// by invertible rigs. The value of the switch controller 'switch'
    /// attribute determines which control rig is used to compute the rig's pose
    /// at any given time. If we simply author a new value to the 'switch'
    /// attribute, then the resulting pose will, in general, change
    /// discontinuously. Compensation invokes inversion to compute the input
    /// avar values that preserve the pose and authors those values, along with
    /// the new 'switch' value, changing the active control rig such that the
    /// pose is continuous across the transition.
    /// 
    /// \warning
    /// This function currently hardcodes the set of input avars in the rig used
    /// in testSwitchCompensation. A production-ready version of this function
    /// would find the input avars by analysis, based on the switch attribute,
    /// but that requires additional core support for invertible rigs.
    ///
    EXEC_INVERTIBLERIGSEXAMPLE_API
    void
    CompensateSwitch(
        const UsdAttribute &switchAttribute,
        const UsdTimeCode &time,
        const TfToken &switchValue);

private:
    ExecUsdSystem _execSystem;
    UsdAttributeVector _inputAvars;
    UsdAttributeVector _outputSpaces;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
