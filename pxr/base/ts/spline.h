//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_SPLINE_H
#define PXR_BASE_TS_SPLINE_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/splineData.h"
#include "pxr/base/ts/knotMap.h"
#include "pxr/base/ts/knot.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/ts/typeHelpers.h"
#include "pxr/base/ts/eval.h"
#include "pxr/base/vt/traits.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/gf/interval.h"
#include "pxr/base/tf/type.h"

#include <string>
#include <memory>
#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE

class VtDictionary;


/// A mathematical description of a curved function from time to value.
///
/// This class is <b>STILL IN DEVELOPMENT.</b>
///
/// Splines are are supported only for floating-point scalar value types.
/// This class is non-templated, but can hold data for varying value types
/// (double, float, and half).  All knots in a spline must have the same value
/// type.
///
/// Splines are defined by <i>knots</i>.  The curve passes through each knot,
/// and in between, the shape of the curve is controlled by <i>tangents</i>
/// specified at the knots.
///
/// Splines typically have Bezier or Hermite curve segments with controllable
/// tangents; linear and <i>held</i> (flat) interpolation are also supported.
/// Outside of the time span of knots, the <i>extrapolation</i> of the curve can
/// be specified.
///
/// The main service provided by splines is <i>evaluation</i>: determining the
/// curve's value at a given time.
///
/// Splines are copy-on-write.  Copying a spline object is cheap; the copy will
/// point to the same data on the heap.  Copying, and then modifying one of the
/// copies, will incur the cost of duplicating the data, including all the
/// knots.
///
class TsSpline
{
public:
    /// \name Construction and value semantics
    ///
    /// This is a lightweight class that wraps a shared pointer.  It is intended
    /// to be used as a value type, and copied freely.  Move semantics are not
    /// implemented; there would be no benefit.
    ///
    /// @{

    /// Default constructor creates a spline without a value type.  The value
    /// type becomes established when the first knot is added.
    TS_API
    TsSpline();

    /// Creates a spline with a specified value type.
    TS_API
    TsSpline(TfType valueType);

    TS_API
    TsSpline(const TsSpline &other);

    TS_API
    TsSpline& operator=(const TsSpline &other);

    TS_API
    bool operator==(const TsSpline &other) const;

    TS_API
    bool operator!=(const TsSpline &other) const;

    /// @}
    /// \name Value types
    /// @{

    TS_API
    static bool IsSupportedValueType(TfType valueType);

    TS_API
    TfType GetValueType() const;

    template <typename T>
    bool IsHolding() const;

    TS_API
    void SetTimeValued(bool timeValued);

    TS_API
    bool IsTimeValued() const;

    /// @}
    /// \name Curve types
    /// @{

    TS_API
    void SetCurveType(TsCurveType curveType);

    TS_API
    TsCurveType GetCurveType() const;

    /// @}
    /// \name Extrapolation
    /// @{

    TS_API
    void SetPreExtrapolation(
        const TsExtrapolation &extrap);

    TS_API
    TsExtrapolation GetPreExtrapolation() const;

    TS_API
    void SetPostExtrapolation(
        const TsExtrapolation &extrap);

    TS_API
    TsExtrapolation GetPostExtrapolation() const;

    /// @}
    /// \name Inner loops
    ///
    /// Loop params are only valid when all of the following are true:
    ///
    /// - protoEnd > protoStart.
    /// - At least one of numPreLoops or numPostLoops is nonzero and positive.
    /// - There is a knot at protoStart.
    ///
    /// Any loop params may be set, and will be stored.  Whenever the above
    /// conditions are not met, the stored params will be ignored.
    ///
    /// To determine if loop params are currently valid, call HasInnerLoops.
    ///
    /// To disable inner loops, call
    /// <code>SetInnerLoopParams(TsLoopParams())</code>.
    ///
    /// @{

    TS_API
    void SetInnerLoopParams(
        const TsLoopParams &params);

    TS_API
    TsLoopParams GetInnerLoopParams() const;

    /// @}
    /// \name Knots
    /// @{

    TS_API
    void SetKnots(
        const TsKnotMap &knots);

    TS_API
    bool CanSetKnot(
        const TsKnot &knot,
        std::string *reasonOut = nullptr) const;

    /// <b>Incompletely implemented</b>; \p affectedIntervalOut is not yet
    /// populated.
    TS_API
    bool SetKnot(
        const TsKnot &knot,
        GfInterval *affectedIntervalOut = nullptr);

    /// Returns the spline's knots.  These are the original knots; if inner or
    /// extrapolating loops are present, this set of knots does not reflect
    /// that.
    TS_API
    TsKnotMap GetKnots() const;

    /// \overload
    /// Get the spline's knots that affect the specified time interval.
    ///
    /// Return a TsKnotMap containing the knots that affect the time
    /// interval. This may include knots outside the time interval if they
    /// affect the curve inside the time interval. For example, if there
    /// are knots at times 10, 20, and 30. Calling \c GetKnots with a
    /// \c timeInteval of [15 .. 25] may return all 3 knots since the
    /// knots at 10 and 30 may affect the shape of the spline at times
    /// that are inside the \c timeInterval.
    TS_API
    TsKnotMap GetKnots(const GfInterval& timeInterval) const;

    /// Retrieves a copy of the knot at the specified time, if one exists.  This
    /// must be an original knot, not a knot that is echoed due to looping.
    /// Returns true on success, false if there is no such knot.
    TS_API
    bool GetKnot(
        TsTime time,
        TsKnot *knotOut) const;

    /// @}
    /// \name Removing knots
    /// @{

    TS_API
    void ClearKnots();

    /// <b>Incompletely implemented</b>; \p affectedIntervalOut is not yet
    /// populated.
    TS_API
    void RemoveKnot(
        TsTime time,
        GfInterval *affectedIntervalOut = nullptr);

    /// <b>Not yet implemented.</b>
    TS_API
    bool ClearRedundantKnots(
        VtValue defaultValue = VtValue(),
        const GfInterval &interval = GfInterval::GetFullInterval());

    /// @}
    /// \name Loop baking
    /// @{

    /// Bake inner loops in the spline.
    ///
    /// Modify the spline by baking out the knots generated by inner looping to
    /// explicit knots and resetting the looping parameters to default.
    TS_API
    bool BakeInnerLoops();

    /// Return all the knots on the spline, including knots generated by
    /// inner looping.
    ///
    /// These knots can be used to create a new spline that does not use looping
    /// but has the same shape as this spline. Note that extrapolated knots are
    /// not included in the result. If you want to include extrapolated knots
    /// use \c GetKnotsWithLoopsBaked and provide a time interval over which knots
    /// should be expanded.
    TS_API
    TsKnotMap GetKnotsWithInnerLoopsBaked() const;

    /// Return baked knots that will replicate this spline over the given time
    /// interval without any looping.
    ///
    /// Return a \c TsKnotMap containing baked knots that will replicate the
    /// shape of the spline over the time interval without using any looping.
    /// Both inner and extrapolation loops are included in the returned knots.
    ///
    /// If extrapolation loops are in use, the specified \p interval argument,
    /// \e must specify a finite time interval since extrapolation looping
    /// extends to infinity and will generate a theoretically infinite number of
    /// knots. Attempts to bake an infinite number of knots will emit a coding
    /// error and return an empty \c TsKnotMap.
    ///
    /// \note Knots baked from extrapolation loops may include knots that are
    /// generated from multiple input knots. Knots at loop boundaries get their
    /// "pre" values from the end of the loop and their "post" values from the
    /// beginning.

    TS_API
    TsKnotMap GetKnotsWithLoopsBaked(
        const GfInterval &interval) const;

    /// @}
    /// \name Breakdowns
    ///
    /// A Breakdown in animation is a pose between the key poses. Breakdown
    /// applied to a \c TsSpline will insert a TsKnot between existing knots
    /// with as little disruption as possible to the overall shape of the
    /// spline.
    /// @{

    /// Add a knot at the specified time.  The new knot is defined so that the
    /// shape of the curve is changed as little as possible. If necessary,
    /// neighboring knots may also be modified.
    ///
    /// There are some situations where a new knot cannot be inserted. For
    /// example, if there is already a knot at the requested time, or if the
    /// requested insertion time is in a region of the spline that is looped
    /// from either extrapolation or inner looping. Use \c CanBreakdown to see
    /// if a breakdown would succeed.
    ///
    /// \return true if a knot was successfully inserted or false if not.
    TS_API
    bool Breakdown(
        TsTime time,
        GfInterval *affectedIntervalOut = nullptr);

    /// Test if a knot could be inserted by \c Breakdown at \c time.
    ///
    /// \return true if a knot could be successfully inserted by \c Breakdown
    /// or false if not. If false is returned and \c reason is not \c nullptr
    /// then a description of the failure will be stored in \c reason. This is
    /// the same error or warning message that would have been emitted by
    /// \c Breakdown if it had failed to insert a knot.
    TS_API
    bool CanBreakdown(
        TsTime time,
        std::string* reason = nullptr);

    /// @}
    /// \name Anti-regression
    ///
    /// See \ref page_ts_regression for a general introduction to regression and
    /// anti-regression.
    ///
    /// \sa TsAntiRegressionAuthoringSelector
    /// \sa TsRegressionPreventer
    /// @{

    /// Returns the current effective anti-regression authoring mode.  This may
    /// come from the overall default of Keep Ratio; the build-configured
    /// default defined by \c PXR_TS_DEFAULT_ANTI_REGRESSION_AUTHORING_MODE; or
    /// a TsAntiRegressionAuthoringSelector.
    TS_API
    static TsAntiRegressionMode GetAntiRegressionAuthoringMode();

    /// Returns whether this spline has any tangents long enough to cause
    /// regression; or, if the current authoring mode is Contain, whether this
    /// spline has any tangents that exceed their segment interval.
    TS_API
    bool HasRegressiveTangents() const;

    /// Shorten any regressive tangents; or, if the current authoring mode is
    /// Contain, any tangents that exceed their segment interval.  Return
    /// whether anything was changed.
    TS_API
    bool AdjustRegressiveTangents();

    /// @}
    /// \name Evaluation
    /// @{
    ///
    /// In all of these templated methods, the T parameter may be the value type
    /// of the spline (double/float/GfHalf), or VtValue.

    template <typename T>
    bool Eval(
        TsTime time,
        T *valueOut) const;

    template <typename T>
    bool EvalPreValue(
        TsTime time,
        T *valueOut) const;

    template <typename T>
    bool EvalDerivative(
        TsTime time,
        T *valueOut) const;

    template <typename T>
    bool EvalPreDerivative(
        TsTime time,
        T *valueOut) const;

    template <typename T>
    bool EvalHeld(
        TsTime time,
        T *valueOut) const;

    template <typename T>
    bool EvalPreValueHeld(
        TsTime time,
        T *valueOut) const;

    TS_API
    bool DoSidesDiffer(
        TsTime time) const;

    /// \brief Evaluates the value of the TsSpline over the given time interval,
    /// typically for drawing.
    ///
    /// \c Sample creates a piecewise linear approximation of the spline curve.
    /// When the returned samples are scaled by \e timeScale and \e valueScale
    /// and linearly interpolated, the reconstructed curve will nowhere have an
    /// error greater than \e tolerance.
    ///
    /// The values of \e timeScale and \e valueScale are typically chosen to
    /// scale the spline's units to pixels and then \e tolerance represents
    /// the allowed deviation in pixel space from a theoretical exact answer.
    ///
    /// \c timeInterval must not be empty and \c timeScale, \c valueScale, and
    /// \c tolerance must all be greater than 0.0. If any of these conditions
    /// are not met, \c Sample returns false and \c *splineSamples is unchanged.
    /// Otherwise, true is returned and \c splineSamples is populated.
    template <typename Vertex>
    bool
    Sample(
        const GfInterval& timeInterval,
        double timeScale,
        double valueScale,
        double tolerance,
        TsSplineSamples<Vertex>* splineSamples) const
    {
        return _Sample(timeInterval, timeScale, valueScale, tolerance,
                       splineSamples);
    }

    /// \overload
    /// When passed a \c TsSplineSamplesWithSources<Vertex> class, the returned
    /// information contains a \c TsSplineSampleSource value for each
    /// polyline. The \c TsSplineSampleSource indicates the source region
    /// (extrapolation, looping, normal interpolation, etc.) of the spline
    /// generated that polyline.
    template <typename Vertex>
    bool
    Sample(
        const GfInterval& timeInterval,
        double timeScale,
        double valueScale,
        double tolerance,
        TsSplineSamplesWithSources<Vertex>* splineSamples) const
    {
        return _Sample(timeInterval, timeScale, valueScale, tolerance,
                       splineSamples);
    }

    /// @}
    /// \name Spline comparison
    /// @{

    /// \brief Compare two splines.
    ///
    /// Returns the time interval over which this spline and \p other have
    /// differences. \c Diff compares the "segments" of a spline where a segment
    /// is the span between adjacent knots (or between the end knots and
    /// infinity). The returned time interval encompases all of the differing
    /// segments in the splines.
    ///
    /// For example, if \c spline1 has knots at times 1, 2, 3, 4, and 5,
    /// and \c spline2 is a copy of \c spline1 with the value at time 3 changed,
    /// calling:
    /// \code
    /// GfInterval diffInterval = spline1.Diff(spline2);
    /// \endcode
    /// Would set \c diffInterval to the half-open interval [2.0 .. 4.0).
    ///
    /// Note that this implementation is fairly conservative, in that it will
    /// never miss any changes but may report differences that do not appear to
    /// be obviously different to the casual observer.
    ///
    /// \c Diff considers interpolated segments with different interpolation
    /// modes to be different, even if the evaluated numeric values are
    /// ultimately the same.  A linear segment with a slope of 0.0 is considered
    /// different than a held segment with the same end points. Two linear
    /// segments with the same end points but different tangents are also
    /// considered different. Extrapolation on the other hand is always either a
    /// value-block, a straight ray from an end point to infinity, or some
    /// repeated loop over the spline's knots. Value-blocks and extrapolated
    /// rays \e are considered identical if they start from the same end-point
    /// with the same slope, even if they are generated by different types of
    /// extrapolation. Extrapolated loops are considered identical if they
    /// produce the same segments.
    TS_API
    GfInterval Diff(const TsSpline& other) const;

    /// \overload
    /// \brief Compare two splines in a specific input time interval.
    ///
    /// This method is identical to the above method but it limits the reported
    /// differences to the provided \p compareInterval. If there are no
    /// differences in the compareInterval, an empty \c GfInterval is returned,
    /// even if there may be differences outside of \c compareInterval
    TS_API
    GfInterval Diff(const TsSpline& other,
                    const GfInterval& compareInterval) const;

    /// @}
    /// \name Whole-spline queries
    /// @{

    TS_API
    bool IsEmpty() const;

    TS_API
    bool HasValueBlocks() const;

    /// <b>Not yet implemented.</b>
    TS_API
    bool IsVarying() const;

    /// Convenience for HasInnerLoops() || HasExtrapolatingLoops().
    TS_API
    bool HasLoops() const;

    TS_API
    bool HasInnerLoops() const;

    TS_API
    bool HasExtrapolatingLoops() const;

    /// <b>Not yet implemented.</b>
    TS_API
    bool IsLinear() const;

    /// <b>Not yet implemented.</b>
    TS_API
    bool IsC0Continuous() const;

    /// <b>Not yet implemented.</b>
    TS_API
    bool IsG1Continuous() const;

    /// <b>Not yet implemented.</b>
    TS_API
    bool IsC1Continuous() const;

    /// <b>Not yet implemented.</b>
    TS_API
    bool GetValueRange(
        const GfInterval &timeSpan,
        std::pair<VtValue, VtValue> *rangeOut) const;

    /// <b>Not yet implemented.</b>
    template <typename T>
    bool GetValueRange(
        const GfInterval &timeSpan,
        std::pair<T, T> *rangeOut) const;

    /// @}
    /// \name Within-spline queries
    /// @{

    TS_API
    bool HasValueBlockAtTime(
        TsTime time) const;

    /// <b>Not yet implemented.</b>
    TS_API
    bool IsSegmentFlat(
        TsTime startTime) const;

    /// <b>Not yet implemented.</b>
    TS_API
    bool IsSegmentMonotonic(
        TsTime startTime) const;

    /// <b>Not yet implemented.</b>
    TS_API
    bool IsKnotRedundant(
        TsTime time,
        VtValue defaultValue = VtValue()) const;

    /// @}

public:
    // Hash function.  For now this is cheap, and only hashes by data pointer.
    // If there are two identical but independent splines, they will hash
    // unequal.
    template <typename HashState>
    friend void TfHashAppend(
        HashState &h,
        const TsSpline &spline)
    {
        h.Append(spline._data.get());
    }

private:
    friend class TsRegressionPreventer;

    // Direct access method used by TsRegressionPreventer.
    void _SetKnotUnchecked(const TsKnot & knot);

    template <typename SampleHolder>
    bool _Sample(
        const GfInterval& timeInterval,
        double timeScale,
        double valueScale,
        double tolerance,
        SampleHolder* splineSamples) const;

    // External helpers provide direct data access for Ts implementation.
    friend Ts_SplineData* Ts_GetSplineData(TsSpline &spline);
    friend const Ts_SplineData* Ts_GetSplineData(const TsSpline &spline);

    friend struct Ts_BinaryDataAccess;
    friend struct Ts_SplineOffsetAccess;

private:
    // Get data to read from.  Will be either actual data or default data.
    TS_API
    const Ts_SplineData* _GetData() const;

    // Ensure we have our own independent data, in preparation for writing.  If
    // a value type is passed, and we don't yet have typed data, ensure we have
    // data of the specified type.
    void _PrepareForWrite(TfType valueType = TfType());

    template <typename T>
    bool _Eval(
        TsTime time,
        T *valueOut,
        Ts_EvalAspect aspect,
        Ts_EvalLocation location) const;

    // Update all the tangents based on the tangent algorithms in the knots and
    // follow that with a call to AdjustRegressiveTangents() to remove any
    // remaining regressive spline segments.  Return true if any changes were
    // made.
    TS_API
    bool _UpdateAllTangents();

    // Update the tangents of a single knot based on its tangent algorithms and
    // the regression prevention settings.
    TS_API
    bool _UpdateKnotTangents(const size_t knotIndex);

private:
    // Our parameter data.  Copy-on-write.  Null only if we are in the default
    // state, with no knots, and all overall parameters set to defaults.  To
    // deal with the possibility of null data, call _GetData for reading, and
    // _PrepareForWrite before writing.
    std::shared_ptr<Ts_SplineData> _data;
};

// TsSpline supports value transforms.
VT_VALUE_TYPE_CAN_TRANSFORM(TsSpline);

/// Output a text representation of a spline to a stream.
TS_API
std::ostream& operator<<(std::ostream& out, const TsSpline &spline);

// XXX: This should not be necessary.  All it does is call std::swap.  This is
// here as a workaround for a downstream library that tries to call swap on
// splines, with a "using namespace std" that doesn't appear to work when pxr
// namespaces are in use.
TS_API
void swap(TsSpline &lhs, TsSpline &rhs);

// For applying layer offsets.
struct Ts_SplineOffsetAccess
{
    TS_API
    static void ApplyOffsetAndScale(
        TsSpline *spline,
        const TsTime offset,
        const double scale);
};


////////////////////////////////////////////////////////////////////////////////
// TEMPLATE IMPLEMENTATIONS

template <typename T>
bool TsSpline::IsHolding() const
{
    if constexpr (!Ts_IsSupportedValueType<T>::value)
    {
        return false;
    }

    return GetValueType() == Ts_GetType<T>();
}

template <typename T>
bool TsSpline::_Eval(
    const TsTime time,
    T* const valueOut,
    const Ts_EvalAspect aspect,
    const Ts_EvalLocation location) const
{
    const std::optional<double> result =
        Ts_Eval(_GetData(), time, aspect, location);

    if (!result)
    {
        return false;
    }

    *valueOut = T(*result);
    return true;
}

// Implement a special case that will ensure the contents of the VtValue output
// variable contain a value of the same type (double, float, or GfHalf) as the
// spline.
template <>
TS_API
bool TsSpline::_Eval(
    const TsTime time,
    VtValue* const valueOut,
    const Ts_EvalAspect aspect,
    const Ts_EvalLocation location) const;

template <typename T>
bool TsSpline::Eval(const TsTime time, T* const valueOut) const
{
    return _Eval(time, valueOut, Ts_EvalValue, Ts_EvalAtTime);
}

template <typename T>
bool TsSpline::EvalPreValue(const TsTime time, T* const valueOut) const
{
    return _Eval(time, valueOut, Ts_EvalValue, Ts_EvalPre);
}

template <typename T>
bool TsSpline::EvalDerivative(const TsTime time, T* const valueOut) const
{
    return _Eval(time, valueOut, Ts_EvalDerivative, Ts_EvalAtTime);
}

template <typename T>
bool TsSpline::EvalPreDerivative(const TsTime time, T* const valueOut) const
{
    return _Eval(time, valueOut, Ts_EvalDerivative, Ts_EvalPre);
}

template <typename T>
bool TsSpline::EvalHeld(const TsTime time, T* const valueOut) const
{
    return _Eval(time, valueOut, Ts_EvalHeldValue, Ts_EvalAtTime);
}

template <typename T>
bool TsSpline::EvalPreValueHeld(const TsTime time, T* const valueOut) const
{
    return _Eval(time, valueOut, Ts_EvalHeldValue, Ts_EvalPre);
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif
