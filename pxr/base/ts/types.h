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
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"

#include <cstdint>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \anchor TS_SPLINE_SUPPORTED_VALUE_TYPES
/// Sequence of value types that are supported by the spline system.
/// \li <b>double</b>
/// \li <b>float</b>
/// \li <b>GfHalf</b>
/// \hideinitializer
#define TS_SPLINE_SUPPORTED_VALUE_TYPES         \
    ((Double,   double))                        \
    ((Float,    float))                         \
    ((Half,     GfHalf))

#define TS_SPLINE_SAMPLE_VERTEX_TYPES \
    ((Vec2d,    GfVec2d))                       \
    ((Vec2f,    GfVec2f))                       \
    ((Vec2h,    GfVec2h))

#define TS_SPLINE_VALUE_TYPE_NAME(x) TF_PP_TUPLE_ELEM(0, x)
#define TS_SPLINE_VALUE_CPP_TYPE(x) TF_PP_TUPLE_ELEM(1, x)

/// \brief True if template parameter T is a supported spline data type.
template <class T>
inline constexpr bool
TsSplineIsValidDataType = false;

#define _TS_SUPPORT_DATA_TYPE(unused, tuple)                            \
    template <>                                                         \
    inline constexpr bool                                               \
    TsSplineIsValidDataType< TS_SPLINE_VALUE_CPP_TYPE(tuple) > = true;
TF_PP_SEQ_FOR_EACH(_TS_SUPPORT_DATA_TYPE,
                   ~,
                   TS_SPLINE_SUPPORTED_VALUE_TYPES)
#undef _TS_SUPPORT_DATA_TYPE

/// \brief True if template parameter T is a supported spline sampling vertex
/// type.
template <class T>
inline constexpr bool
TsSplineIsValidSampleType = false;

#define _TS_SUPPORT_SAMPLE_TYPE(unused, tuple)                          \
    template <>                                                         \
    inline constexpr bool                                               \
    TsSplineIsValidSampleType< TS_SPLINE_VALUE_CPP_TYPE(tuple) > = true;
TF_PP_SEQ_FOR_EACH(_TS_SUPPORT_SAMPLE_TYPE,
                   ~,
                   TS_SPLINE_SAMPLE_VERTEX_TYPES)
#undef _TS_SUPPORT_SAMPLE_TYPE

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

/// The source for a particular part of a sampled spline. A \c TsSpline can have
/// a number of different regions. The source is not important to the values
/// that vary over time, but if the spline is sampled and displayed in a user
/// interface, the source can be used to highlight different regions of the
/// displayed spline.
///
enum TsSplineSampleSource
{
    TsSourcePreExtrap,          //< Extrapolation before the first knot
    TsSourcePreExtrapLoop,      //< Looped extrapolation before the first knot
    TsSourceInnerLoopPreEcho,   //< Echoed copy of an inner loop prototype
    TsSourceInnerLoopProto,     //< This is the inner loop prototype
    TsSourceInnerLoopPostEcho,  //< Echoed copy of an inner loop prototype
    TsSourceKnotInterp,         //< "Normal" knot interpolation
    TsSourcePostExtrap,         //< Extrapolation after the last knot
    TsSourcePostExtrapLoop,     //< Looped extrapolation after the last knot
};

/// Automatic tangent calculation algorithms.
///
/// Which automatic tangent algorithm to use.
///
/// \li None - Tangents are not automatically calculated, the provided values
/// are used. Note that the tangent values are still subject to modification
/// by the spline's anti-regression setting.
///
/// \li Custom - The tangent algorithm is determined by the "preTanAlgorithm"
/// and "postTanAlgorithm" keys in the knot's custom data. These custom
/// data keys are reserved for this purpose. If the custom data values do not
/// exist or if their value cannot be understood, then Custom behaves as if
/// the None algorithm was used. Note that the Custom algorithm is not yet
/// implemented so it currently always behaves like None.
///
/// \li AutoEase - Use the "Auto Ease" algorithm from Maya/animX. This is a
/// cubic controlled blending algorithm that computes a slope between the slopes
/// to the knots on either side of this knot. If there is a discontinuity in the
/// spline at this knot (this knot has no previous or next knot, is dual valued,
/// or is adjacent to a value blocked segment of the spline) then the slope will
/// be 0 (flat).
///
/// \see \ref TsKnot::SetPreTanAlgorithm, \ref TsKnot::SetPostTanAlgorithm
enum TsTangentAlgorithm
{
    TsTangentAlgorithmNone,
    TsTangentAlgorithmCustom,
    TsTangentAlgorithmAutoEase
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

/// \brief \c TsSplineSamples<Vertex> holds a collection of piecewise linear
/// polylines that approximate a \c TsSpline.
///
/// The vertex must be one of \c GfVec2d, \c GfVec2f, or \c GfVec2h. Note that
/// you may have precision or overflow issues if you use \c GfVec2h.
///
/// \sa \ref TsSplineSamplesWithSources and \ref TsSpline::Sample
template <typename Vertex>
class TsSplineSamples
{
public:
    static_assert(TsSplineIsValidSampleType<Vertex>,
                  "The Vertex template parameter to TsSplineSamples must be one"
                  " of GfVec2d, GfVec2f, or GfVec2h.");

    using Polyline = std::vector<Vertex>;

    std::vector<Polyline> polylines;
};

/// \brief \c TsSplineSamplesWithSources<Vertex> is a \c TsSplineSamples<Vertex>
/// that also includes source information for each polyline.
///
/// The vertex must be one of \c GfVec2d, \c GfVec2f, or \c GfVec2h. Note that
/// you may have precision or overflow issues if you use \c GfVec2h.
///
/// The \c polylines and \c sources vectors are parallel arrays. In other words,
/// the source for the \c Polyline in \c polylines[i] is in \c sources[i] and
/// the two vectors have the same size.
/// \sa \ref TsSplineSamples and \ref TsSpline::SampleWithSources
template <typename Vertex>
class TsSplineSamplesWithSources
{
public:
    static_assert(TsSplineIsValidSampleType<Vertex>,
                  "The Vertex template parameter to TsSplineSamplesWithSources"
                  " must be one of GfVec2d, GfVec2f, or GfVec2h.");

    using Polyline = std::vector<Vertex>;

    std::vector<Polyline> polylines;
    std::vector<TsSplineSampleSource> sources;
};

// Declare sampling classes as extern templates. They are explicitly
// instantiated in types.cpp
#define TS_SAMPLE_EXTERN_IMPL(unused, tuple)                            \
    TS_API_TEMPLATE_CLASS(                                              \
        TsSplineSamples< TS_SPLINE_VALUE_CPP_TYPE(tuple) >);            \
    TS_API_TEMPLATE_CLASS(                                              \
        TsSplineSamplesWithSources< TS_SPLINE_VALUE_CPP_TYPE(tuple) >);
TF_PP_SEQ_FOR_EACH(TS_SAMPLE_EXTERN_IMPL, ~, TS_SPLINE_SAMPLE_VERTEX_TYPES)
#undef TS_SAMPLE_EXTERN_IMPL

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
