//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_TYPES_H
#define PXR_BASE_TS_TYPES_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"

#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/base/gf/quatd.h"
#include "pxr/base/gf/quatf.h"

#include "pxr/base/gf/matrix2d.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/interval.h"
#include "pxr/base/gf/multiInterval.h"
#include "pxr/base/gf/range1d.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
// Including weakPtrFacade.h before vt/value.h works around a problem
// finding get_pointer.
#include "pxr/base/tf/weakPtrFacade.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"
#include <string>
#include <map>

PXR_NAMESPACE_OPEN_SCOPE

/// The time type used by Ts.
typedef double TsTime;

/// \brief Keyframe knot types.
///
/// These specify the method used to interpolate keyframes.
/// This enum is registered with TfEnum for conversion to/from std::string.
///
enum TsKnotType {
    TsKnotHeld = 0,     //!< A held-value knot; tangents will be ignored
    TsKnotLinear,       //!< A Linear knot; tangents will be ignored
    TsKnotBezier,       //!< A Bezier knot

    TsKnotNumTypes
};

/// \brief Spline extrapolation types.
///
/// These specify the method used to extrapolate splines.
/// This enum is registered with TfEnum for conversion to/from std::string.
///
enum TsExtrapolationType {
    TsExtrapolationHeld = 0, //!< Held;  splines hold values at edges
    TsExtrapolationLinear,   //!< Linear;  splines hold slopes at edges

    TsExtrapolationNumTypes
};

/// \brief A pair of TsExtrapolationTypes indicating left
/// and right extrapolation in first and second, respectively.
typedef std::pair<TsExtrapolationType,TsExtrapolationType> 
    TsExtrapolationPair;

/// \brief Dual-value keyframe side.
enum TsSide {
    TsLeft,
    TsRight
};

/// \brief An individual sample.  A sample is either a blur, defining a
/// rectangle, or linear, defining a line for linear interpolation.
/// In both cases the sample is half-open on the right.
typedef struct TsValueSample {
public:
    TsValueSample(TsTime inLeftTime, const VtValue& inLeftValue,
        TsTime inRightTime, const VtValue& inRightValue,
        bool inBlur = false) :
        isBlur(inBlur),
        leftTime(inLeftTime),
        rightTime(inRightTime),
        leftValue(inLeftValue),
        rightValue(inRightValue)
    {}

public:
    bool isBlur;        //!< True if a blur sample
    TsTime leftTime;  //!< Left side time (inclusive)
    TsTime rightTime; //!< Right side time (exclusive)
    VtValue leftValue;  //!< Value at left or, for blur, min value
    VtValue rightValue; //!< Value at right or, for blur, max value
} TsValueSample;

/// A sequence of samples.
typedef std::vector<TsValueSample> TsSamples;

// Traits for types used in TsSplines.
//
// Depending on a type's traits, different interpolation techniques are
// available:
//
// * if not interpolatable, only TsKnotHeld can be used
// * if interpolatable, TsKnotHeld and TsKnotLinear can be used
// * if supportsTangents, any knot type can be used
//
template <typename T>
struct TsTraits {
    // True if this is a valid value type for splines.
    // Default is false; set to true for all supported types.
    static const bool isSupportedSplineValueType = false;

    // True if the type can be interpolated by taking linear combinations.
    // If this is false, only TsKnotHeld is isSupportedSplineValueType.
    static const bool interpolatable = true;

    // True if the value can be extrapolated outside of the keyframe
    // range. If this is false we always use TsExtrapolateHeld behaviour.
    // This is true if a slope can be computed from the line between two knots
    // of this type.
    static const bool extrapolatable = false;

    // True if the value type supports tangents.
    // If true, interpolatable must also be true.
    static const bool supportsTangents = true;

    // The origin or zero vector for this type.
    static const T zero;
};

template <>
struct TS_API TsTraits<std::string> {
    static const bool isSupportedSplineValueType = true;
    static const bool interpolatable = false;
    static const bool extrapolatable = false;
    static const bool supportsTangents = false;
    static const std::string zero;
};

template <>
struct TS_API TsTraits<double> {
    static const bool isSupportedSplineValueType = true;
    static const bool interpolatable = true;
    static const bool extrapolatable = true;
    static const bool supportsTangents = true;
    static const double zero;
};

template <>
struct TS_API TsTraits<float> {
    static const bool isSupportedSplineValueType = true;
    static const bool interpolatable = true;
    static const bool extrapolatable = true;
    static const bool supportsTangents = true;
    static const float zero;
};

template <>
struct TS_API TsTraits<int> {
    static const bool isSupportedSplineValueType = true;
    static const bool interpolatable = false;
    static const bool extrapolatable = false;
    static const bool supportsTangents = false;
    static const int zero;
};

template <>
struct TS_API TsTraits<bool> {
    static const bool isSupportedSplineValueType = true;
    static const bool interpolatable = false;
    static const bool extrapolatable = false;
    static const bool supportsTangents = false;
    static const bool zero;
};

template <>
struct TS_API TsTraits<GfVec2d> {
    static const bool isSupportedSplineValueType = true;
    static const bool interpolatable = true;
    static const bool extrapolatable = true;
    static const bool supportsTangents = false;
    static const GfVec2d zero;
};

template <>
struct TS_API TsTraits<GfVec2f> {
    static const bool isSupportedSplineValueType = true;
    static const bool interpolatable = true;
    static const bool extrapolatable = true;
    static const bool supportsTangents = false;
    static const GfVec2f zero;
};

template <>
struct TS_API TsTraits<GfVec3d> {
    static const bool isSupportedSplineValueType = true;
    static const bool interpolatable = true;
    static const bool extrapolatable = true;
    static const bool supportsTangents = false;
    static const GfVec3d zero;
};

template <>
struct TS_API TsTraits<GfVec3f> {
    static const bool isSupportedSplineValueType = true;
    static const bool interpolatable = true;
    static const bool extrapolatable = true;
    static const bool supportsTangents = false;
    static const GfVec3f zero;
};

template <>
struct TS_API TsTraits<GfVec4d> {
    static const bool isSupportedSplineValueType = true;
    static const bool interpolatable = true;
    static const bool extrapolatable = true;
    static const bool supportsTangents = false;
    static const GfVec4d zero;
};

template <>
struct TS_API TsTraits<GfVec4f> {
    static const bool isSupportedSplineValueType = true;
    static const bool interpolatable = true;
    static const bool extrapolatable = true;
    static const bool supportsTangents = false;
    static const GfVec4f zero;
};

template <>
struct TS_API TsTraits<GfQuatd>  {
    static const bool isSupportedSplineValueType = true;
    static const bool interpolatable = true;
    static const bool extrapolatable = false;
    static const bool supportsTangents = false;
    static const GfQuatd zero;
};

template <>
struct TS_API TsTraits<GfQuatf>  {
    static const bool isSupportedSplineValueType = true;
    static const bool interpolatable = true;
    static const bool extrapolatable = false;
    static const bool supportsTangents = false;
    static const GfQuatf zero;
};

template <>
struct TS_API TsTraits<GfMatrix2d> {
    static const bool isSupportedSplineValueType = true;
    static const bool interpolatable = true;
    static const bool extrapolatable = true;
    static const bool supportsTangents = false;
    static const GfMatrix2d zero;
};

template <>
struct TS_API TsTraits<GfMatrix3d> {
    static const bool isSupportedSplineValueType = true;
    static const bool interpolatable = true;
    static const bool extrapolatable = true;
    static const bool supportsTangents = false;
    static const GfMatrix3d zero;
};

template <>
struct TS_API TsTraits<GfMatrix4d> {
    static const bool isSupportedSplineValueType = true;
    static const bool interpolatable = true;
    static const bool extrapolatable = true;
    static const bool supportsTangents = false;
    static const GfMatrix4d zero;
};

template <>
struct TS_API TsTraits< VtArray<double> > {
    static const bool isSupportedSplineValueType = true;
    static const bool interpolatable = true;
    static const bool extrapolatable = true;
    static const bool supportsTangents = false;
    static const VtArray<double> zero;
};

template <>
struct TS_API TsTraits< VtArray<float> > {
    static const bool isSupportedSplineValueType = true;
    static const bool interpolatable = true;
    static const bool extrapolatable = true;
    static const bool supportsTangents = false;
    static const bool supportsVaryingShapes = false;
    static const VtArray<float> zero;
};

template <>
struct TS_API TsTraits<TfToken> {
    static const bool isSupportedSplineValueType = true;
    static const bool interpolatable = false;
    static const bool extrapolatable = false;
    static const bool supportsTangents = false;
    static const TfToken zero;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
