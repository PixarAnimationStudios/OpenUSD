//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_TYPES_H
#define PXR_BASE_TS_TYPES_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/gf/interval.h"

#include <cstdint>

PXR_NAMESPACE_OPEN_SCOPE


// Times are encoded as double.
using TsTime = double;

//////////////////////////////
// ** NOTE TO MAINTAINERS **
//
// The following enum values are used in the binary crate format.
// Do not change them; only add.

/// Interpolation mode for a spline segment (region between two knots).
///
enum TsInterpMode
{
    TsInterpValueBlock  = 0,  //< No value in this segment.
    TsInterpHeld        = 1,  //< Constant value in this segment.
    TsInterpLinear      = 2,  //< Linear interpolation.
    TsInterpCurve       = 3   //< Bezier or Hermite, depends on curve type.
};

/// Type of interpolation for a spline's \c Curve segments.
///
enum TsCurveType
{
    TsCurveTypeBezier  = 0,  //< Bezier curve, free tangent widths.
    TsCurveTypeHermite = 1   //< Hermite curve, like Bezier but fixed tan width.
};

/// Curve-shaping mode for one of a spline's extrapolation regions (before all
/// knots and after all knots).
///
enum TsExtrapMode
{
    TsExtrapValueBlock    = 0, //< No value in this region.
    TsExtrapHeld          = 1, //< Constant value in this region.
    TsExtrapLinear        = 2, //< Linear interpolation based on edge knots.
    TsExtrapSloped        = 3, //< Linear interpolation with specified slope.
    TsExtrapLoopRepeat    = 4, //< Knot curve repeated, offset so ends meet.
    TsExtrapLoopReset     = 5, //< Curve repeated exactly, discontinuous joins.
    TsExtrapLoopOscillate = 6  //< Like Reset, but every other copy reversed.
};

/// Inner-loop parameters.
///
/// At most one inner-loop region can be specified per spline.  Only whole
/// numbers of pre- and post-iterations are supported.
///
/// The value offset specifies the difference between the values at the starts
/// of consecutive iterations.
///
/// There must always be a knot at the protoStart time; otherwise the loop
/// parameters are invalid and will be ignored.
///
/// A copy of the start knot is always made at the end of the prototype region.
/// This is true even if there is no post-looping; it ensures that all
/// iterations (including pre-loops) match the prototype region exactly.
///
/// Enabling inner looping will generally change the shape of the prototype
/// interval (and thus all looped copies), because the first knot is echoed as
/// the last.  Inner looping does not aim to make copies of an existing shape;
/// it aims to set up for continuity at loop joins.
///
/// When inner looping is applied, any knots specified in the pre-looped or
/// post-looped intervals are removed from consideration, though they remain in
/// the spline parameters.  A knot exactly at the end of the prototype interval
/// is not part of the prototype; it will be ignored, and overwritten by the
/// start-knot copy.
///
/// When protoEnd <= protoStart, inner looping is disabled.
///
/// Negative numbers of loops are not meaningful; they are treated the same as
/// zero counts.  These quantities are signed only so that accidental underflow
/// does not result in huge loop counts.
///
class TsLoopParams
{
public:
    TsTime protoStart = 0.0;
    TsTime protoEnd = 0.0;
    int32_t numPreLoops = 0;
    int32_t numPostLoops = 0;
    double valueOffset = 0.0;

public:
    TS_API
    bool operator==(const TsLoopParams &other) const;

    TS_API
    bool operator!=(const TsLoopParams &other) const;

    /// Returns the prototype region, [protoStart, protoEnd).
    TS_API
    GfInterval GetPrototypeInterval() const;

    /// Returns the union of the prototype region and the echo region(s).
    TS_API
    GfInterval GetLoopedInterval() const;
};

/// Extrapolation parameters for the ends of a spline beyond the knots.
///
class TsExtrapolation
{
public:
    TsExtrapMode mode = TsExtrapHeld;
    double slope = 0.0;

public:
    TS_API
    TsExtrapolation();

    TS_API
    TsExtrapolation(TsExtrapMode mode);

    TS_API
    bool operator==(const TsExtrapolation &other) const;

    TS_API
    bool operator!=(const TsExtrapolation &other) const;

    /// Returns whether our mode is one of the looping extrapolation modes.
    TS_API
    bool IsLooping() const;
};

/// Modes for enforcing non-regression in splines.
///
/// See \ref page_ts_regression for a general introduction to regression and
/// anti-regression.
///
enum TsAntiRegressionMode
{
    /// Do not enforce.  If there is regression, runtime evaluation will use
    /// KeepRatio.
    TsAntiRegressionNone,

    /// Prevent tangents from crossing neighboring knots.  This guarantees
    /// non-regression, but is slightly over-conservative, preventing the
    /// authoring of some extreme curves that cannot be created without
    /// non-contained tangents.
    TsAntiRegressionContain,

    /// If there is regression in a segment, shorten both of its tangents until
    /// the regression is just barely prevented (the curve comes to a
    /// near-standstill at some time).  Preserve the ratio of the tangent
    /// lengths.
    TsAntiRegressionKeepRatio,

    /// If there is regression in a segment, leave its start tangent alone, and
    /// shorten its end tangent until the regression is just barely prevented.
    /// This matches Maya behavior.
    TsAntiRegressionKeepStart
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
