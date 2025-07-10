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

    /// <b>Not yet implemented.</b>
    TS_API
    bool BakeLoops(
        const GfInterval &interval);

    /// <b>Not yet implemented.</b>
    //
    // Result cached
    TS_API
    const TsKnotMap& GetKnotsWithInnerLoopsBaked() const;

    /// <b>Not yet implemented.</b>
    //
    // Bakes inner loops (finite) and extrapolating loops (infinite)
    // Result cached, but only for last specified interval
    TS_API
    const TsKnotMap& GetKnotsWithLoopsBaked(
        const GfInterval &interval) const;

    /// @}
    /// \name Splitting
    /// @{

    /// <b>Not yet implemented.</b>
    ///
    /// Adds a knot at the specified time.  The new knot is arranged so that the
    /// shape of the curve is as unchanged as possible.
    TS_API
    bool Split(
        TsTime time,
        GfInterval *affectedIntervalOut = nullptr);

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
