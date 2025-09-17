//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/eval.h"
#include "pxr/base/ts/splineData.h"
#include "pxr/base/ts/regressionPreventer.h"
#include "pxr/base/ts/debugCodes.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/tf/diagnostic.h"

#include <algorithm>
#include <cmath>

PXR_NAMESPACE_OPEN_SCOPE

// This epsilon value is used in several places when evaluating Bezier curves.
// Specifically in the calculation of the Bezier parameter that corresponds to a
// given time. So this epsilon is being applied to a unitless bezier parameter
// parameter that should be in the range [0..1].
static constexpr double _parameterEpsilon = 1.0e-10;

////////////////////////////////////////////////////////////////////////////////
// BEZIER MATH

namespace
{
    // Coefficients for a quadratic function.  May be a cubic derivative, or
    // just a quadratic.
    //
    struct _Quadratic
    {
    public:
        double Eval(const double t) const
        {
            return t * (t * a + b) + c;
        }

    public:
        // Coefficients of quadratic function, in power form.
        // f(t) = at^2 + bt + c.
        double a = 0;
        double b = 0;
        double c = 0;
    };

    // Coefficients for one of a Bezier's two cubic functions,
    // either time (x = f(t)) or value (y = f(t)).
    //
    struct _Cubic
    {
    public:
        // Compute cubic coefficients from Bezier control points.
        // The segment starts at p0.
        // The start tangent endpoint is p1.
        // The end tangent endpoint is p2.
        // The segment ends at p3.
        static _Cubic FromPoints(
            const double p0,
            const double p1,
            const double p2,
            const double p3)
        {
            _Cubic result;

            result.a = -p0 + 3*p1 - 3*p2 + p3;
            result.b = 3*p0 - 6*p1 + 3*p2;
            result.c = -3*p0 + 3*p1;
            result.d = p0;

            return result;
        }

        double Eval(const double t) const
        {
            return t * (t * (t * a + b) + c) + d;
        }

        _Quadratic GetDerivative() const
        {
            // Power rule.
            return _Quadratic{3*a, 2*b, c};
        }

    public:
        // Coefficients of cubic function, in power form.
        // f(t) = at^3 + bt^2 + ct + d.
        double a = 0;
        double b = 0;
        double c = 0;
        double d = 0;
    };
}

// Filter the zeros, looking for the answer between 0 and 1.
static double _FilterZeros(double z0, double z1, double z2)
{
    double result = z0;
    double minError = std::abs(z0 - 0.5);
    double error = std::abs(z1 - 0.5);

    if (error < minError) {
        // Check for multiple zeros and warn of possible regressive spline.
        if (minError < 0.5) {
            // XXX: Possibly this should just be a TF_DEBUG message
            TF_WARN("Possibly regressive spline. Zeros at %g, %g, and %g",
                    z0, z1, z2);
        }
        result = z1;
        minError = error;
    }

    error = std::abs(z2 - 0.5);
    if (error < minError) {
        // Check for multiple zeros again and warn.
        if (minError < 0.5) {
            TF_WARN("Possibly regressive spline. Zeros at %g, %g, and %g.",
                    z0, z1, z2);
        }
        result = z2;
        minError = error;
    }

    if (minError > 0.5 + _parameterEpsilon) {
        TF_WARN("No zero found in [0..1]. Zeros at %g, %g, and %g."
                " Using %g.",
                z0, z1, z2, result);
    }

    return result;
}

// Filter the zeros, looking for the answer between 0 and 1.
static double _FilterZeros(double z0, double z1)
{
    double result = z0;
    double minError = std::abs(z0 - 0.5);
    double error = std::abs(z1 - 0.5);

    if (error < minError) {
        // Check for multiple zeros and warn of possible regressive spline.
        if (minError < 0.5) {
            // XXX: Possibly this should just be a TF_DEBUG message
            TF_WARN("Possibly regressive spline. Zeros at %g and %g",
                    z0, z1);
        }
        result = z1;
        minError = error;
    }

    if (minError > 0.5 + _parameterEpsilon) {
        TF_WARN("No zero found in [0..1]. Zeros at %g and %g."
                " Using %g.",
                z0, z1, result);
    }

    return result;
}

// Given the specified quadratic coefficients; given that the caller has ensured
// that the function is monotonically increasing on t in [0, 1], and its range
// includes zero: find the unique t-value in [0, 1] that causes the function to
// have a zero value.
//
// Uses quadratic formula.
//
static double _FindMonotonicZero(
    const _Quadratic &quad)
{
    const double discrim = std::sqrt(std::pow(quad.b, 2) - 4 * quad.a * quad.c);
    const double root0 = (-quad.b - discrim) / (2 * quad.a);
    const double root1 = (-quad.b + discrim) / (2 * quad.a);
    return _FilterZeros(root0, root1);
}

// Finds the unique real t-value in [0, 1] that satisfies
// t^3 + bt^2 + ct + d = 0, given that the function is known to be monotonically
// increasing.  See the Cardano reference below.
//
static double _FindMonotonicZero(
    const double b,
    const double c,
    const double d)
{
    const double p = (3*c - b*b) / 3;
    const double p3 = p/3;
    const double p33 = p3*p3*p3;
    const double q = (2*b*b*b - 9*b*c + 27*d) / 27;
    const double q2 = q/2;
    const double discrim = q2*q2 + p33;
    const double b3 = b/3;

    if (discrim < 0)
    {
        // Three real roots.
        const double r = std::sqrt(-p33);
        const double t = -q / (2*r);
        const double phi = std::acos(GfClamp(t, -1, 1));
        const double t1 = 2 * std::cbrt(r);
        const double root1 = t1 * std::cos(phi/3) - b3;
        const double root2 = t1 * std::cos((phi + 2*M_PI) / 3) - b3;
        const double root3 = t1 * std::cos((phi + 4*M_PI) / 3) - b3;
        return _FilterZeros(root1, root2, root3);
    }
    else if (discrim == 0)
    {
        // Two real roots.
        const double u1 = -std::cbrt(q2);
        const double root1 = 2*u1 - b3;
        const double root2 = -u1 - b3;
        return _FilterZeros(root1, root2);
    }
    else
    {
        // One real root.
        const double sd = std::sqrt(discrim);
        const double u1 = std::cbrt(sd - q2);
        const double v1 = std::cbrt(sd + q2);
        return u1 - v1 - b3;
    }
}

// Given the specified cubic coefficients; given that the caller has ensured
// that the function is monotonically increasing on t in [0, 1], and its range
// includes zero: find the unique t-value in [0, 1] that causes the function to
// have a zero value.
//
// Uses Cardano's algorithm.
// See, e.g., https://pomax.github.io/bezierinfo/#yforx
// What that reference calls (a, b, c, d), we call (b, c, d, a).
// The monotonic assumption allows us to assert that there is only one zero.
//
static double _FindMonotonicZero(
    const _Cubic &cubic)
{
    // Fairly arbitrary tininess constant, not tuned carefully.
    // We can lose precision in some cases if this is too small or too big.
    //
    // XXX: Note: A better epsilon is probably something like:
    //
    //    epsilon = 1e-10 * (abs(a) + abs(b) + abs(c) + abs(d))
    //
    // This scales epsilon roughly by the range of time coordinates and means
    // that the epsilon for a spline using nanoseconds is very different than
    // one using seconds (as it should be). But we should put some computational
    // mathimatical rigor behind that decision before changing it. Leaving it
    // for now.
    static constexpr double epsilon = 1e-10;

    // Check for coefficients near zero.
    const bool aZero = GfIsClose(cubic.a, 0, epsilon);
    const bool bZero = GfIsClose(cubic.b, 0, epsilon);
    const bool cZero = GfIsClose(cubic.c, 0, epsilon);

    // Check for no solutions (constant function).  Should never happen.
    if (!TF_VERIFY(!aZero || !bZero || !cZero))
    {
        return 0.0;
    }

    // Check for linearity.  Makes cubic and quadratic formulas degenerate.
    if (aZero && bZero)
    {
        return -cubic.d / cubic.c;
    }

    // Check for quadraticity.  Makes cubic formula degenerate.
    if (aZero)
    {
        return _FindMonotonicZero(_Quadratic{cubic.b, cubic.c, cubic.d});
    }

    // Compute cubic solution.  Scale the curve to force the t^3 coefficient to
    // be 1, which simplifies the math without changing the result.
    return _FindMonotonicZero(
        cubic.b / cubic.a,
        cubic.c / cubic.a,
        cubic.d / cubic.a);
}

// Finds and returns the normalized parameter t (in the range [0..1]) along a
// cubic Bezier curve defined by beginData and endData at which the Bezier
// curve's time reaches the given time.
//
// Note that if t is  outside [0..1] due to floating-point precision, it will
// be clamped. If it is more than slightly outside the range, a warning will
// be emitted.
static double
_FindBezierParameter(
    const Ts_TypedKnotData<double> &beginData,
    const Ts_TypedKnotData<double> &endData,
    const TsTime time)
{
    // Find the coefficients for x = f(t).
    // Offset everything by the eval time, so that we can just find a zero.
    const _Cubic timeCubic = _Cubic::FromPoints(
        beginData.time - time,
        beginData.time + beginData.GetPostTanWidth() - time,
        endData.time - endData.GetPreTanWidth() - time,
        endData.time - time);

    // Find the value of t for which f(t) = 0.
    // Due to the offset, this is the t-value at which we reach the eval time.
    double t = _FindMonotonicZero(timeCubic);

    if (t < 0)
    {
        if (t < -_parameterEpsilon) {
            TF_WARN("Bezier spline parameter t < -epsilon, t=%g, -epsilon=%g",
                    t, -_parameterEpsilon);
        }
        t = 0;
    }
    else if (t > 1)
    {
        if (t > 1 + _parameterEpsilon) {
            TF_WARN("Bezier spline parameter t > 1 + epsilon, t=%g,"
                    " 1 + epsilon=%g",
                    t, 1 + _parameterEpsilon);
        }
        t = 1;
    }

    return t;
}

static double
_EvalBezier(
    const Ts_TypedKnotData<double> &beginDataIn,
    const Ts_TypedKnotData<double> &endDataIn,
    const TsTime time,
    const Ts_EvalAspect aspect)
{
    // If the segment is regressive, de-regress it.
    // Our eval-time behavior always uses the Keep Ratio strategy.
    Ts_TypedKnotData<double> beginData = beginDataIn;
    Ts_TypedKnotData<double> endData = endDataIn;
    Ts_RegressionPreventerBatchAccess::ProcessSegment(
        &beginData, &endData, TsAntiRegressionKeepRatio);

    double t = _FindBezierParameter(beginData, endData, time);

    // Find the coefficients for y = f(t).
    const _Cubic valueCubic = _Cubic::FromPoints(
        beginData.value,
        beginData.value + beginData.GetPostTanHeight(),
        endData.GetPreValue() + endData.GetPreTanHeight(),
        endData.GetPreValue());

    if (aspect == Ts_EvalValue)
    {
        // Evaluate y = f(t).
        return valueCubic.Eval(t);
    }
    else
    {
        // Evaluate dy/dx (value delta over time delta)
        // as dy/dt / dx/dt (quotient of derivatives).

        const _Cubic timeCubic = _Cubic::FromPoints(
            beginData.time,
            beginData.time + beginData.GetPostTanWidth(),
            endData.time - endData.GetPreTanWidth(),
            endData.time);

        const _Quadratic valueDeriv = valueCubic.GetDerivative();
        const _Quadratic timeDeriv = timeCubic.GetDerivative();
        return valueDeriv.Eval(t) / timeDeriv.Eval(t);
    }
}

////////////////////////////////////////////////////////////////////////////////
// HERMITE MATH

static double
_EvalHermite(
    const Ts_TypedKnotData<double> &beginData,
    const Ts_TypedKnotData<double> &endData,
    const TsTime time,
    const Ts_EvalAspect aspect)
{
    // For a unit time range (t in [0..1]), and given:
    //     v0 = value at time 0
    //     m0 = first derivative at time 0
    //     v1 = value at time 1
    //     m1 = first derivative at time 1
    // the cubic hermite equation is:
    //   v(t) = (( 2 * t^3 - 3 * t^2     +  1) * v0 +
    //           (     t^3 - 2 * t^2 + t     ) * m0 +
    //           (-2 * t^3 + 3 * t^2         ) * v1 +
    //           (     t^3 -     t^2         ) * m1)
    //
    // This can be refactored to be:
    //   v(t) = (t^3 * ( 2 * v0 - 2 * v1 +     m0 + m1) +
    //           t^2 * (-3 * v0 + 3 * v1 - 2 * m0 - m1) +
    //           t   * (                       m0     ) +
    //                       v0)
    //
    // yielding the coefficients of a polynomial:
    //   a = ( 2 * v0 - 2 * v1 +     m0 + m1)
    //   b = (-3 * v0 + 3 * v1 - 2 * m0 - m1)
    //   c = m0
    //   d = v0
    //   v(t) = a * t^3 + b * t^2 + c * t + d
    //
    // If we let:
    //   dv = v1 - v0
    // we can further simplify the a and b coefficients to:
    //   a = (-2 * dv +     m0 + m1)
    //   b = ( 3 * dv - 2 * m0 - m1)
    //
    // For curves evaluated from [t0..t1] instead of [0..1], a change of
    // variables is in order. We subtract t0 from t and divide by (t1 - t0).
    // When we do this, we have to multiply the slopes by (t1 - t0) as they
    // represent rise/run and we just reduced run by a factor of (t1 - t0).
    //
    //   dt = t1 - t0
    //   u = (t - t0) / dt
    //   um0 = m0 * dt
    //   um1 = m1 * dt
    //
    //   a = -2 * dv + um0 + um1
    //   b = 3 * dv - 2 * um0 - um1
    //   c = um0
    //   d = v0
    //
    // Then:
    //
    //   v(t) = a * u^3 + b * u^2 + c * u + d; where u = (t - t0)/(t1 - t0)
    //
    // If we are asked to calculate the derivative, the answer is a simple chain
    // rule.
    //
    //   v'(t) = (3*a * u^2 + 2*b * u + c) * (du/dt)
    //
    // where du/dt == 1.0/(t1 - t0). (Just to add confusion, (t1 - t0) is stored
    // in a variable named "dt"). So
    //
    //   v'(t) == (3*a * u^2 + 2*b * u + c) / (t1 - t0);
    //            where u = (t - t0)/(t1 - t0)
    //
    const double t0 = beginData.time;
    const double v0 = beginData.value;
    const double m0 = beginData.postTanSlope;

    const double t1 = endData.time;
    const double v1 = endData.GetPreValue();
    const double m1 = endData.preTanSlope;

    if (!TF_VERIFY(t0 < t1,
                   "Cannot interpolate Hermite segment whose length <= 0.0"))
    {
        return v0;
    }

    const double dt = t1 - t0;
    const double dv = v1 - v0;

    // Convert time into the [0..1] range and adjust slopes to match.
    const double u = (time - t0) / dt;
    const double um0 = m0 * dt;
    const double um1 = m1 * dt;

    // Calculate the coefficients
    const double a = -2 * dv + um0 + um1;
    const double b = 3 * dv - 2 * um0 - um1;
    const double c = um0;
    const double d = v0;

    if (aspect == Ts_EvalDerivative) {
        // Derivative evaluation via chain rule
        return (u * (u * 3 * a + 2 * b) + c) / dt;
    } else {
        // Normal value evaluation.
        return u * (u * (u * a + b) + c) + d;
    }
}

////////////////////////////////////////////////////////////////////////////////
// EVAL HELPERS

// Find the slope from one knot to another in a linear segment.  Such slopes are
// implicit: based on times and values, not tangents.
//
static double
_GetSegmentSlope(
    const Ts_TypedKnotData<double> &beginData,
    const Ts_TypedKnotData<double> &endData)
{
    return (endData.GetPreValue() - beginData.value) /
        (endData.time - beginData.time);
}

// Find the slope in an extrapolation region.
//
static std::optional<double>
_GetExtrapolationSlope(
    const TsExtrapolation &extrap,
    const bool haveMultipleKnots,
    const Ts_TypedKnotData<double> &endKnotData,
    const Ts_TypedKnotData<double> &adjacentData,
    const Ts_EvalLocation location)
{
    // None, Held, and Sloped have simple answers.
    if (extrap.mode == TsExtrapValueBlock)
    {
        return std::nullopt;
    }
    if (extrap.mode == TsExtrapHeld)
    {
        return 0.0;
    }
    if (extrap.mode == TsExtrapSloped)
    {
        return extrap.slope;
    }

    // If there is only one knot, the slope is flat.
    if (!haveMultipleKnots)
    {
        return 0.0;
    }

    // Otherwise extrapolation is Linear (extrapolating loops are resolved
    // before we get here), and the slope depends on the first segment.
    if (!TF_VERIFY(extrap.mode == TsExtrapLinear))
    {
        return 0.0;
    }

    // If the end knot is dual-valued, the slope is flat.
    if (endKnotData.dualValued)
    {
        return 0.0;
    }

    if (location == Ts_EvalPre)
    {
        // If the first segment is held or value-blocked, the slope is flat.
        if (endKnotData.nextInterp == TsInterpHeld
            || endKnotData.nextInterp == TsInterpValueBlock)
        {
            return 0.0;
        }

        // If the first segment is linear, the slope is the straight line
        // between the first two knots.
        if (endKnotData.nextInterp == TsInterpLinear)
        {
            return _GetSegmentSlope(endKnotData, adjacentData);
        }

        // Otherwise the first segment is curved.  The slope is continued from
        // the inward-facing side of the first knot.
        return endKnotData.postTanSlope;
    }
    else
    {
        // If the last segment is held or value-blocked, the slope is flat.
        if (adjacentData.nextInterp == TsInterpHeld
            || adjacentData.nextInterp == TsInterpValueBlock)
        {
            return 0.0;
        }

        // If the last segment is linear, the slope is the straight line
        // between the last two knots.
        if (adjacentData.nextInterp == TsInterpLinear)
        {
            return _GetSegmentSlope(adjacentData, endKnotData);
        }

        // Otherwise the last segment is curved.  The slope is continued from
        // the inward-facing side of the last knot.
        return endKnotData.preTanSlope;
    }
}

// Extrapolate a straight line from a knot.
//
static double
_ExtrapolateLinear(
    const Ts_TypedKnotData<double> &knotData,
    const double slope,
    const TsTime time,
    const Ts_EvalLocation location)
{
    if (location == Ts_EvalPre)
    {
        return knotData.GetPreValue() - slope * (knotData.time - time);
    }
    else
    {
        return knotData.value + slope * (time - knotData.time);
    }
}

////////////////////////////////////////////////////////////////////////////////
// LOOPING

namespace
{
    // When we evaluate in a loop echo region, we must consider copies of knots
    // from the prototype region.  Rather than actually make those copies, we
    // determine a location within the prototype region where we will evaluate
    // instead.  This class computes that shift, accounting for both inner loops
    // and extrapolating loops.
    //
    // GetEvalTime() returns the time at which to evaluate, which is in a
    // non-echoed region.  GetEvalLocation() returns the location at which to
    // evaluate, which can differ from the original in the case of oscillating
    // extrapolation.  IsBetweenLastProtoAndEnd() returns whether we are in the
    // special case of evaluating (after the shift) between the last prototype
    // knot and the end of the prototype region.
    //
    // ReplaceBoundaryKnots() handles some inner-looping interpolation cases:
    // between the last knot of one loop iteration and the first knot of the
    // next, and between an echoed knot and an unlooped one.  The previous and
    // next knots are passed in, and one of them may be replaced by a shifted
    // copy of the first prototype knot.  Replace{Pre,Post}ExtrapKnots does the
    // same thing for extrapolating cases, where the final knots may be created
    // by inner-loop copying.
    //
    // GetValueOffset() returns an amount to add to the value obtained at the
    // shifted evaluation time.  This supports cases where the copied knots are
    // offset in the value dimension.  GetValueOffset() is always zero when
    // evaluating derivatives, which aren't affected by value offsets.
    //
    // GetNegate() returns whether the value should be negated.  This can be
    // needed for derivatives in oscillating loops.
    //
    class _LoopResolver
    {
    public:
        // Constructor performs all computation.
        _LoopResolver(
            const Ts_SplineData *data,
            TsTime time,
            Ts_EvalAspect aspect,
            Ts_EvalLocation location);

        // Output accessors.
        TsTime GetEvalTime() const { return _evalTime; }
        Ts_EvalLocation GetEvalLocation() const { return _location; }
        bool IsBetweenLastProtoAndEnd() const
            { return _betweenLastProtoAndEnd; }
        double GetValueOffset() const { return _valueOffset; }
        bool GetNegate() const { return _negate; }

        // Knot copiers for special cases.
        void ReplaceBoundaryKnots(
            Ts_TypedKnotData<double> *prevData,
            Ts_TypedKnotData<double> *nextData,
            bool* generatedPrev = nullptr,
            bool* generatedNext = nullptr) const;
        void ReplacePreExtrapKnots(
            Ts_TypedKnotData<double> *nextData,
            Ts_TypedKnotData<double> *nextData2,
            bool* generatedNext = nullptr) const;
        void ReplacePostExtrapKnots(
            Ts_TypedKnotData<double> *prevData,
            Ts_TypedKnotData<double> *prevData2,
            bool* generatedPrev = nullptr) const;

    private:
        void _ResolveInner();
        void _ResolveExtrap();
        void _DoExtrap(
            const TsExtrapolation &extrapolation,
            TsTime offset,
            bool isPre);
        void _ComputeExtrapValueOffset();
        Ts_TypedKnotData<double> _CopyProtoKnotData(
            size_t index,
            int shiftIters) const;

    private:
        // Inputs.
        const Ts_SplineData* const _data;
        const Ts_EvalAspect _aspect;

        // Inputs that may be altered, and serve as outputs.
        TsTime _evalTime;
        Ts_EvalLocation _location;

        // Outputs.
        double _valueOffset = 0;
        bool _negate = false;
        bool _betweenLastProtoAndEnd = false;

        // Intermediate data.
        bool _haveInnerLoops = false;
        size_t _firstInnerProtoIndex = 0;
        bool _havePreExtrapLoops = false;
        bool _havePostExtrapLoops = false;
        TsTime _firstTime = 0;
        TsTime _lastTime = 0;
        bool _firstTimeLooped = false;
        bool _lastTimeLooped = false;
        bool _doPreExtrap = false;
        bool _doPostExtrap = false;
        double _extrapValueOffset = 0;
        bool _betweenPreUnloopedAndLooped = false;
        bool _betweenLoopedAndPostUnlooped = false;
        Ts_TypedKnotData<double> _extrapKnot1;
        Ts_TypedKnotData<double> _extrapKnot2;
    };
}

_LoopResolver::_LoopResolver(
    const Ts_SplineData* const data,
    const TsTime timeIn,
    const Ts_EvalAspect aspect,
    const Ts_EvalLocation location)
    : _data(data),
      _aspect(aspect),
      _evalTime(timeIn),
      _location(location)
{
    // Is inner looping enabled?
    _haveInnerLoops = _data->HasInnerLoops(&_firstInnerProtoIndex);

    // We have multiple knots if there are multiple authored.  We also always
    // have at least two knots if there is valid inner looping.
    const bool haveMultipleKnots =
        (_haveInnerLoops || _data->times.size() > 1);

    // Are any extrapolating loops enabled?
    _havePreExtrapLoops =
        haveMultipleKnots && _data->preExtrapolation.IsLooping();
    _havePostExtrapLoops =
        haveMultipleKnots && _data->postExtrapolation.IsLooping();

    // Anything to do?
    if (!_haveInnerLoops && !_havePreExtrapLoops && !_havePostExtrapLoops)
    {
        return;
    }

    // Find first and last knot times.  These may be authored, or they may be
    // echoed.
    const TsTime rawFirstTime = _firstTime = _data->times.front();
    const TsTime rawLastTime = _lastTime = _data->times.back();
    if (_haveInnerLoops)
    {
        const GfInterval loopedInterval = _data->loopParams.GetLoopedInterval();

        if (loopedInterval.GetMin() < rawFirstTime)
        {
            _firstTime = loopedInterval.GetMin();
            _firstTimeLooped = true;
        }

        if (loopedInterval.GetMax() > rawLastTime)
        {
            _lastTime = loopedInterval.GetMax();
            _lastTimeLooped = true;
        }
    }

    TF_DEBUG_MSG(
        TS_DEBUG_LOOPS,
        "\n"
        "At construction:\n"
        "  evalTime: %g\n"
        "  haveInnerLoops: %d\n"
        "  havePreExtrapLoops: %d\n"
        "  havePostExtrapLoops: %d\n"
        "  firstTimeLooped: %d\n"
        "  lastTimeLooped: %d\n",
        _evalTime,
        _haveInnerLoops,
        _havePreExtrapLoops,
        _havePostExtrapLoops,
        _firstTimeLooped,
        _lastTimeLooped);

    // Resolve.  If we have both extrapolating and inner loops, handle
    // extrapolating loops first, then inner loops.  We are reversing the
    // procedure of knot copying, which copies knots from inner loops first,
    // then from extrapolating loops.
    if (_havePreExtrapLoops || _havePostExtrapLoops)
    {
        _ResolveExtrap();
    }
    if (_haveInnerLoops)
    {
        _ResolveInner();
    }
}

void _LoopResolver::_ResolveInner()
{
    TF_DEBUG_MSG(
        TS_DEBUG_LOOPS,
        "Before resolving inner loops:\n"
        "  firstInnerProtoIndex: %zu\n"
        "Loop params:\n"
        "  protoStart: %g\n"
        "  protoEnd: %g\n"
        "  numPreLoops: %d\n"
        "  numPostLoops: %d\n"
        "  valueOffset: %g\n",
        _firstInnerProtoIndex,
        _data->loopParams.protoStart,
        _data->loopParams.protoEnd,
        _data->loopParams.numPreLoops,
        _data->loopParams.numPostLoops,
        _data->loopParams.valueOffset);

    const TsLoopParams &lp = _data->loopParams;
    const GfInterval loopedInterval = lp.GetLoopedInterval();
    const GfInterval protoInterval = lp.GetPrototypeInterval();

    // Handle evaluation in echo regions.
    if (loopedInterval.Contains(_evalTime)
        && !protoInterval.Contains(_evalTime))
    {
        const TsTime protoSpan = protoInterval.GetSize();

        // Handle evaluation in pre-echo.
        if (_evalTime < lp.protoStart)
        {
            // Figure out which pre-iteration we're in.
            const TsTime loopOffset = lp.protoStart - _evalTime;
            const int iterNum = int(std::ceil(loopOffset / protoSpan));

            // Hop forward to the prototype region.
            _evalTime += iterNum * protoSpan;

            // Adjust for value offset.
            if (_aspect == Ts_EvalValue)
            {
                _valueOffset -= iterNum * lp.valueOffset;
            }
        }

        // Handle iteration in post-echo.
        else
        {
            // Figure out which post-iteration we're in.
            const TsTime loopOffset = _evalTime - lp.protoEnd;
            const int iterNum = int(loopOffset / protoSpan) + 1;

            // Hop backward to the prototype region.
            _evalTime -= iterNum * protoSpan;

            // Adjust for value offset.
            if (_aspect == Ts_EvalValue)
            {
                _valueOffset += iterNum * lp.valueOffset;
            }
        }
    }

    // Look for special interpolation and extrapolation cases.

    const std::vector<TsTime> &times = _data->times;
    const auto firstProtoIt = times.begin() + _firstInnerProtoIndex;

    // Case 1: between last prototype knot and prototype end, after performing
    // shift out of echo region, if any.
    if (protoInterval.Contains(_evalTime))
    {
        // Use binary search to find first knot at or after prototype end.
        const auto lbIt = std::lower_bound(
            firstProtoIt, times.end(), lp.protoEnd);

        // Unconditionally take the preceding knot as the last in the
        // prototype.  If there is no knot equal or greater, we want the last
        // knot.  If there is a knot that is greater but not one that is equal,
        // we want the one before that.  If there is a knot that is exactly at
        // the end of the prototype, that isn't part of the prototype, and we
        // want the one before it.  In all cases, it is OK if the last prototype
        // knot is also the first and only prototype knot.
        const TsTime lastProtoKnotTime = *(lbIt - 1);

        // Check whether we are evaluating after the last prototype knot.
        if (_evalTime > lastProtoKnotTime)
        {
            _betweenLastProtoAndEnd = true;
        }
    }

    // Case 2: pre-extrapolating, and the first knots are copies made by inner
    // looping.
    else if (_evalTime < _firstTime)
    {
        if (_firstTimeLooped)
        {
            // First knot is always a copy of the first prototype knot.
            _extrapKnot1 = _CopyProtoKnotData(
                _firstInnerProtoIndex, -lp.numPreLoops);

            if (_data->times.size() > _firstInnerProtoIndex + 1
                && protoInterval.Contains(
                    _data->times[_firstInnerProtoIndex + 1]))
            {
                // Second knot is a copy of the second prototype knot.
                _extrapKnot2 = _CopyProtoKnotData(
                    _firstInnerProtoIndex + 1, -lp.numPreLoops);
            }
            else
            {
                // There are no knots after the first prototype knot, so the
                // second is another copy of the first.
                _extrapKnot2 = _CopyProtoKnotData(
                    _firstInnerProtoIndex, -lp.numPreLoops + 1);
            }
        }
        // TODO: Handle the "second time looped" case.  There is an issue where
        // the first knot in the spline is not generated by inner looping, but
        // the second knot is. In this case, extrapolation using TsExtrapLinear
        // should compute the linear slope using the first knot and the
        // generated second knot, but it appears that we are not yet handling
        // that case.
    }

    // Case 3: post-extrapolating, and the last knots are copies mad by inner
    // looping.
    else if (_evalTime > _lastTime)
    {
        if (_lastTimeLooped)
        {
            // Last knot is always a copy of the first prototype knot.
            _extrapKnot1 = _CopyProtoKnotData(
                _firstInnerProtoIndex, lp.numPostLoops + 1);

            // Find last authored prototype knot, which may also be the first.
            // See comments in Case 1 above.
            const auto lastProtoIt = std::lower_bound(
                firstProtoIt, times.end(), lp.protoEnd) - 1;
            const size_t lastProtoIndex = lastProtoIt - _data->times.begin();

            // Second-to-last knot is a copy of the last prototype knot.
            _extrapKnot2 = _CopyProtoKnotData(
                lastProtoIndex, lp.numPostLoops);
        }
        // TODO: Handle "penultimate time looped" case. See the "second time
        // looped" comment just above. The same logic applies at the end of the
        // spline.
    }

    // Case 4: between last knot before looping region and start of looping
    // region.
    else if (_evalTime < loopedInterval.GetMin())
    {
        // Use binary search to find first authored knot at or after start of
        // looping region.  This may be a shadowed knot or a prototype knot.
        const auto lbIt = std::lower_bound(
            times.begin(), firstProtoIt, loopedInterval.GetMin());

        // If the first knot in the looping region isn't the overall first knot,
        // take the preceding one as the last pre-unlooped knot.
        if (lbIt != times.begin())
        {
            const TsTime lastPreUnloopedKnotTime = *(lbIt - 1);

            // Check whether we are evaluating after last pre-unlooped knot.
            if (_evalTime > lastPreUnloopedKnotTime)
            {
                _betweenPreUnloopedAndLooped = true;
            }
        }
    }

    // Case 5: between end of looping region and first knot after looping
    // region.
    else if (_evalTime > loopedInterval.GetMax())
    {
        // Use binary search to find first authored knot strictly after end of
        // looping region.  (Note upper_bound here instead of lower_bound.)
        const auto ubIt = std::upper_bound(
            firstProtoIt + 1, times.end(), loopedInterval.GetMax());

        // If we found such a knot, it's the one we want.
        if (ubIt != times.end())
        {
            const TsTime firstPostUnloopedKnotTime = *ubIt;

            // Check whether we are evaluating before first post-unlooped knot.
            if (_evalTime < firstPostUnloopedKnotTime)
            {
                _betweenLoopedAndPostUnlooped = true;
            }
        }
    }

    TF_DEBUG_MSG(
        TS_DEBUG_LOOPS,
        "After resolving inner loops:\n"
        "  evalTime: %g\n"
        "  valueOffset: %g\n"
        "  betweenLastProtoAndEnd: %d\n"
        "  betweenPreUnloopedAndLooped: %d\n"
        "  betweenLoopedAndPostUnlooped: %d\n",
        _evalTime,
        _valueOffset,
        _betweenLastProtoAndEnd,
        _betweenPreUnloopedAndLooped,
        _betweenLoopedAndPostUnlooped);
}

void _LoopResolver::_ResolveExtrap()
{
    // Determine the interval that doesn't require extrapolation.  One end is
    // closed, the other is open; which one depends on the eval location.
    const GfInterval knotInterval(
        _firstTime, _lastTime,
        /* minClosed = */ (_location != Ts_EvalPre),
        /* maxClosed = */ (_location == Ts_EvalPre));

    // Are we extrapolating?
    if (knotInterval.Contains(_evalTime))
    {
        return;
    }

    // Is the extrapolation looped?
    _doPreExtrap = (_havePreExtrapLoops && _evalTime < _lastTime);
    _doPostExtrap = (_havePostExtrapLoops && _evalTime > _firstTime);
    if (!_doPreExtrap && !_doPostExtrap)
    {
        return;
    }

    // Handle looped extrapolation.
    if (_doPreExtrap)
    {
        _DoExtrap(_data->preExtrapolation, _firstTime - _evalTime, true);
    }
    else if (_doPostExtrap)
    {
        _DoExtrap(_data->postExtrapolation, _evalTime - _lastTime, false);
    }

    TF_DEBUG_MSG(
        TS_DEBUG_LOOPS,
        "After resolving extrapolating loops:\n"
        "  evalTime: %g\n"
        "  valueOffset: %g\n"
        "  doPreExtrap: %d\n"
        "  doPostExtrap: %d\n"
        "  extrapValueOffset: %g\n"
        "  negate: %d\n",
        _evalTime,
        _valueOffset,
        _doPreExtrap,
        _doPostExtrap,
        _extrapValueOffset,
        _negate);
}

// The offset parameter specifies the distance between the evaluation time and
// the non-extrapolating region.  It is always non-negative.
//
void _LoopResolver::_DoExtrap(
    const TsExtrapolation &extrapolation,
    const TsTime offset,
    const bool isPre)
{
    // Figure out how many whole iterations the extrapolation distance covers.
    // Also determine if we're exactly at an iteration boundary.
    const TsTime protoSpan = _lastTime - _firstTime;
    const double numItersFrac = offset / protoSpan;
    const int numItersTrunc = int(numItersFrac);
    const bool boundary = (numItersTrunc == numItersFrac);

    // Typically we want to hop one more than the number of whole iterations.
    // But if we're exactly at an iteration boundary, then evaluating on the
    // short side takes up one iteration less.
    const bool shortOffset =
        boundary && (
            (isPre && _location != Ts_EvalPre)
            || (!isPre && _location == Ts_EvalPre));
    const int numIters = (shortOffset ? numItersTrunc : numItersTrunc + 1);

    // Figure out the signed evaluation offset.
    const int iterHop = (isPre ? numIters : -numIters);

    // Hop forward or back into the non-extrapolating region.
    _evalTime += iterHop * protoSpan;

    // Repeat mode: each extrapolating loop iteration begins with the value
    // from the end of the previous one, and the offsets accumulate.  We adjust
    // the value offset in the opposite direction from the evaluation time,
    // because we first hop forward to evaluate, then apply the value offset
    // backward to obtain the value at the original time.
    if (_data->preExtrapolation.mode == TsExtrapLoopRepeat
        && _aspect != Ts_EvalDerivative)
    {
        _ComputeExtrapValueOffset();
        _valueOffset -= iterHop * _extrapValueOffset;
    }

    // Oscillate mode: every other extrapolating loop iteration is reflected
    // in time.
    else if (
        _data->preExtrapolation.mode == TsExtrapLoopOscillate
        && iterHop % 2 != 0)
    {
        _evalTime = _firstTime + (protoSpan - (_evalTime - _firstTime));
        _location = (_location == Ts_EvalPre ? Ts_EvalPost : Ts_EvalPre);
        if (_aspect == Ts_EvalDerivative)
        {
            _negate = true;
        }
    }

    // Nothing special for Reset mode.  There is no value offset, and each
    // iteration resets to the start value with a discontinuity.  That
    // discontinuity will occur because, when we're exactly at an iteration
    // boundary, we make different shifts depending on whether we're
    // evaluating on the pre-side or post-side.
}

void _LoopResolver::_ComputeExtrapValueOffset()
{
    const TsLoopParams &lp = _data->loopParams;

    double firstValue;
    if (!_firstTimeLooped)
    {
        // Earliest knot is not from inner loops.  Read its value.
        firstValue = _data->GetKnotDataAsDouble(0).GetPreValue();
    }
    else
    {
        // Earliest knot is from inner loops.  Compute its value.
        firstValue =
            _data->GetKnotDataAsDouble(_firstInnerProtoIndex).GetPreValue()
            - lp.numPreLoops * lp.valueOffset;
    }

    double lastValue;
    if (!_lastTimeLooped)
    {
        // Latest knot is not from inner loops.  Read its value.
        lastValue = _data->GetKnotDataAsDouble(_data->times.size() - 1).value;
    }
    else
    {
        // Latest knot is from inner loops.  It is the final echo of the
        // prototype start knot.  Compute its value.
        lastValue =
            _data->GetKnotDataAsDouble(_firstInnerProtoIndex).value
            + (lp.numPostLoops + 1) * lp.valueOffset;
    }

    _extrapValueOffset = lastValue - firstValue;
}

// Handle some oddball interpolation cases arising from inner loops.
// Extrapolating loops don't cause these cases, because their prototype region
// (the set of all authored knots) always includes knots at the start and end,
// and there are no regions that come before or after the extrapolating loops.
//
void _LoopResolver::ReplaceBoundaryKnots(
    Ts_TypedKnotData<double> *prevData,
    Ts_TypedKnotData<double> *nextData,
    bool* generatedPrev /* = nullptr */,
    bool* generatedNext /* = nullptr */) const
{
    const TsLoopParams &lp = _data->loopParams;

    if (generatedPrev) {
        *generatedPrev = false;
    }
    if (generatedNext) {
        *generatedNext = false;
    }

    // Case 1: between last prototype knot and prototype end, after performing
    // shift out of echo region, if any.  Make a copy of the first prototype
    // knot at the end of the prototype region, and use that as nextData.
    if (_betweenLastProtoAndEnd)
    {
        *nextData = _CopyProtoKnotData(
            _firstInnerProtoIndex, 1);
        if (generatedNext) {
            *generatedNext = true;
        }
    }

    // Case 2: between last knot before looping region and start of looping
    // region.  Make a copy of the first prototype knot at the start of the
    // looping region, and use that as nextData.
    else if (_betweenPreUnloopedAndLooped)
    {
        *nextData = _CopyProtoKnotData(
            _firstInnerProtoIndex, -lp.numPreLoops);
        if (generatedNext) {
            *generatedNext = true;
        }
    }

    // Case 3: between end of looping region and first knot after looping
    // region.  Make a copy of the first prototype knot at the end of the
    // looping region, and use that as prevData.
    else if (_betweenLoopedAndPostUnlooped)
    {
        *prevData = _CopyProtoKnotData(
            _firstInnerProtoIndex, lp.numPostLoops + 1);
        if (generatedPrev) {
            *generatedPrev = true;
        }
    }
}

void _LoopResolver::ReplacePreExtrapKnots(
    Ts_TypedKnotData<double>* const nextData,
    Ts_TypedKnotData<double>* const nextData2,
    bool* generatedNext /* = nullptr */) const
{
    if (!_firstTimeLooped) {
        if (generatedNext) {
            *generatedNext = false;
        }
        return;
    }

    *nextData = _extrapKnot1;
    *nextData2 = _extrapKnot2;

    if (generatedNext) {
        *generatedNext = true;
    }
}

void _LoopResolver::ReplacePostExtrapKnots(
    Ts_TypedKnotData<double>* const prevData,
    Ts_TypedKnotData<double>* const prevData2,
    bool* generatedPrev /* = nullptr */) const
{
    if (!_lastTimeLooped) {
        if (generatedPrev) {
            *generatedPrev = false;
        }
        return;
    }

    *prevData = _extrapKnot1;
    *prevData2 = _extrapKnot2;

    if (generatedPrev) {
        *generatedPrev = true;
    }
}

Ts_TypedKnotData<double>
_LoopResolver::_CopyProtoKnotData(
    const size_t index,
    const int shiftIters) const
{
    const TsLoopParams &lp = _data->loopParams;
    const TsTime protoSpan = lp.GetPrototypeInterval().GetSize();

    // Copy the knot.
    Ts_TypedKnotData<double> knotCopy = _data->GetKnotDataAsDouble(index);

    // Shift time.
    knotCopy.time += shiftIters * protoSpan;

    // Shift value.
    if (_aspect == Ts_EvalValue)
    {
        const double offset = shiftIters * lp.valueOffset;
        knotCopy.value += offset;
        if (knotCopy.dualValued)
        {
            knotCopy.preValue += offset;
        }
    }

    return knotCopy;
}

////////////////////////////////////////////////////////////////////////////////
// MAIN EVALUATION

// Interpolate between two knots.
//
static std::optional<double>
_Interpolate(
    const Ts_TypedKnotData<double> &beginData,
    const Ts_TypedKnotData<double> &endData,
    const TsTime time,
    const Ts_EvalAspect aspect,
    const TsCurveType curveType)
{
    // Special-case value blocks
    if (beginData.nextInterp == TsInterpValueBlock) {
        return std::nullopt;
    }

    // Special-case held evaluation.
    if (aspect == Ts_EvalHeldValue)
    {
        return beginData.value;
    }

    // Curved segment: Bezier/Hermite math.
    if (beginData.nextInterp == TsInterpCurve)
    {
        if (curveType == TsCurveTypeBezier)
        {
            return _EvalBezier(beginData, endData, time, aspect);
        }
        else
        {
            return _EvalHermite(beginData, endData, time, aspect);
        }
    }

    // Held segment: determined by previous knot.
    if (beginData.nextInterp == TsInterpHeld)
    {
        return (aspect == Ts_EvalValue ? beginData.value : 0.0);
    }

    // Linear segment: find slope, extrapolate from previous knot.
    if (beginData.nextInterp == TsInterpLinear)
    {
        const double slope = _GetSegmentSlope(beginData, endData);
        if (aspect == Ts_EvalDerivative)
        {
            return slope;
        }

        return _ExtrapolateLinear(beginData, slope, time, Ts_EvalPost);
    }

    // Should be unreachable.
    TF_CODING_ERROR("Unexpected interpolation type");
    return std::nullopt;
}

static std::optional<double>
_EvalMain(
    const Ts_SplineData* const data,
    const _LoopResolver &loopRes,
    const Ts_EvalAspect aspect)
{
    const TsTime time = loopRes.GetEvalTime();
    const Ts_EvalLocation location = loopRes.GetEvalLocation();
    const std::vector<TsTime> &times = data->times;

    // Use binary search to find first knot at or after the specified time.
    const auto lbIt = std::lower_bound(times.begin(), times.end(), time);

    // Figure out where we are in the sequence.  Find the bracketing knots, the
    // knot we're at, if any, and what type of position (before start, after
    // end, at first knot, at last knot, at another knot, between knots).
    const bool atKnot = (lbIt != times.end() && *lbIt == time);
    const auto prevIt = (lbIt != times.begin() ? lbIt - 1 : times.end());
    const auto knotIt = (atKnot ? lbIt : times.end());
    const auto nextIt = (atKnot ? lbIt + 1 : lbIt);
    const bool beforeStart = (nextIt == times.begin());
    const bool afterEnd =
        (loopRes.IsBetweenLastProtoAndEnd() ?
            false : prevIt == times.end() - 1);
    const bool atFirst = (knotIt == times.begin());
    const bool atLast = (knotIt == times.end() - 1);
    const bool haveMultipleKnots = (times.size() > 1);

    // Retrieve knot parameters.
    Ts_TypedKnotData<double> knotData, prevData, nextData;
    if (knotIt != times.end())
    {
        knotData = data->GetKnotDataAsDouble(knotIt - times.begin());
    }
    if (prevIt != times.end())
    {
        prevData = data->GetKnotDataAsDouble(prevIt - times.begin());
    }
    if (nextIt != times.end())
    {
        nextData = data->GetKnotDataAsDouble(nextIt - times.begin());
    }

    // Handle times at knots.
    if (atKnot)
    {
        // Handle values.
        if (aspect == Ts_EvalValue
            || aspect == Ts_EvalHeldValue)
        {
            // Pre-value after held segment = previous knot value.
            if (location == Ts_EvalPre) {
                if (atFirst) {
                    if (data->preExtrapolation.mode == TsExtrapValueBlock) {
                        return std::nullopt;
                    }
                } else {
                    if (prevData.nextInterp == TsInterpValueBlock) {
                        return std::nullopt;
                    } else if (prevData.nextInterp == TsInterpHeld
                               || aspect == Ts_EvalHeldValue)
                    {
                        return prevData.value;
                    }
                }
            } else {
                if (atLast) {
                    if (data->postExtrapolation.mode == TsExtrapValueBlock) {
                        return std::nullopt;
                    }
                } else {
                    if (knotData.nextInterp == TsInterpValueBlock) {
                        return std::nullopt;
                    }
                }
            }

            // Not a special case.  Return what's stored in the knot.
            return (location == Ts_EvalPre ?
                knotData.GetPreValue() : knotData.value);
        }

        // Handle derivatives.
        else
        {
            if (location == Ts_EvalPre)
            {
                // Pre-derivative at first knot = extrapolation slope.
                if (atFirst)
                {
                    return _GetExtrapolationSlope(
                        data->preExtrapolation,
                        haveMultipleKnots, knotData, nextData,
                        Ts_EvalPre);
                }

                switch (prevData.nextInterp) {
                  case TsInterpValueBlock:
                    return std::nullopt;

                  case TsInterpHeld:
                    return 0.0;

                  case TsInterpLinear:
                    return _GetSegmentSlope(prevData, knotData);

                  case TsInterpCurve:
                    return knotData.preTanSlope;
                }
            }
            else
            {
                // Post-derivative at last knot = extrapolation slope.
                if (atLast)
                {
                    return _GetExtrapolationSlope(
                        data->postExtrapolation,
                        haveMultipleKnots, knotData, prevData,
                        Ts_EvalPost);
                }

                switch (knotData.nextInterp) {
                  case TsInterpValueBlock:
                    return std::nullopt;

                  case TsInterpHeld:
                    return 0.0;

                  case TsInterpLinear:
                    return _GetSegmentSlope(knotData, nextData);

                  case TsInterpCurve:
                    return knotData.postTanSlope;
                }
            }
        }
    }

    // Extrapolate before first knot.
    if (beforeStart)
    {
        if (data->preExtrapolation.mode == TsExtrapValueBlock) {
            return std::nullopt;
        }

        // nextData is the first knot.  We also need the knot after that, if
        // there is one.
        Ts_TypedKnotData<double> nextData2;
        if (nextIt + 1 != times.end())
        {
            nextData2 = data->GetKnotDataAsDouble((nextIt + 1) - times.begin());
        }

        loopRes.ReplacePreExtrapKnots(&nextData, &nextData2);

        // Special-case held evaluation.
        if (aspect == Ts_EvalHeldValue)
        {
            // There's really no reasonable value to return as the held
            // pre-extrapolation value. This answer is similar to the post-
            // extrapolation answer.
            return nextData.GetPreValue();
        }

        // Find the extrapolation slope.
        const std::optional<double> slope =
            _GetExtrapolationSlope(
                data->preExtrapolation,
                haveMultipleKnots, nextData, nextData2,
                Ts_EvalPre);

        // No slope -> no extrapolation.
        if (!slope)
        {
            return std::nullopt;
        }

        // If computing derivative, done.
        if (aspect == Ts_EvalDerivative)
        {
            return *slope;
        }

        // Extrapolate value.
        return _ExtrapolateLinear(nextData, *slope, time, Ts_EvalPre);
    }

    // Extrapolate after last knot.
    if (afterEnd)
    {
        if (data->postExtrapolation.mode == TsExtrapValueBlock) {
            return std::nullopt;
        }

        // prevData is the last knot.  We also need the knot before that, if
        // there is one.
        Ts_TypedKnotData<double> prevData2;
        if (prevIt != times.begin())
        {
            prevData2 = data->GetKnotDataAsDouble((prevIt - 1) - times.begin());
        }

        loopRes.ReplacePostExtrapKnots(&prevData, &prevData2);

        // Special-case held evaluation.
        if (aspect == Ts_EvalHeldValue)
        {
            return prevData.value;
        }

        // Find the extrapolation slope.
        const std::optional<double> slope =
            _GetExtrapolationSlope(
                data->postExtrapolation,
                haveMultipleKnots, prevData, prevData2,
                Ts_EvalPost);

        // No slope -> no extrapolation.
        if (!slope)
        {
            return std::nullopt;
        }

        // If computing derivative, done.
        if (aspect == Ts_EvalDerivative)
        {
            return *slope;
        }

        // Extrapolate value.
        return _ExtrapolateLinear(prevData, *slope, time, Ts_EvalPost);
    }

    // Otherwise we are between knots.

    // Account for loop-boundary cases.
    loopRes.ReplaceBoundaryKnots(&prevData, &nextData);

    // Interpolate.
    return _Interpolate(prevData, nextData, time, aspect, data->curveType);
}

////////////////////////////////////////////////////////////////////////////////
// EVAL ENTRY POINT

std::optional<double>
Ts_Eval(
    const Ts_SplineData* const data,
    const TsTime timeIn,
    const Ts_EvalAspect aspect,
    const Ts_EvalLocation location)
{
    // If no knots, no value or slope.
    if (data->times.empty())
    {
        return std::nullopt;
    }

    // If loops are in use, and we're evaluating in an echo region, figure out
    // time and value shifts, and special interpolation cases.
    _LoopResolver loopRes(data, timeIn, aspect, location);

    // Perform the main evaluation.
    std::optional<double> result = _EvalMain(data, loopRes, aspect);
    if (!result)
    {
        return std::nullopt;
    }

    // Add value offset, and/or negate, if applicable.
    return (*result + loopRes.GetValueOffset())
        * (loopRes.GetNegate() ? -1 : 1);
}

////////////////////////////////////////////////////////////////////////////////
// BREAKDOWN Methods
static bool
_BreakdownHermite(
    const Ts_SplineData* const data,
    Ts_TypedKnotData<double>& prevData,
    Ts_TypedKnotData<double>& knotData,
    Ts_TypedKnotData<double>& nextData)
{
    // Hermites are easy to breakdown. The new knot simply has the evaluated
    // value and slope of the existing spline.
    double value, slope;
    
    value = _EvalHermite(prevData, nextData, knotData.time, Ts_EvalValue);
    slope = _EvalHermite(prevData, nextData, knotData.time, Ts_EvalDerivative);

    knotData.value = value;
    knotData.preTanSlope = slope;
    knotData.postTanSlope = slope;

    // Turn off any tangent algorithms to keep them from messing things up.
    prevData.postTanAlgorithm = TsTangentAlgorithmNone;
    knotData.preTanAlgorithm = TsTangentAlgorithmNone;
    knotData.postTanAlgorithm = TsTangentAlgorithmNone;
    nextData.preTanAlgorithm = TsTangentAlgorithmNone;

    return true;
}

static bool
_BreakdownBezier(
    const Ts_SplineData* const data,
    Ts_TypedKnotData<double>& prevData,
    Ts_TypedKnotData<double>& knotData,
    Ts_TypedKnotData<double>& nextData)
{
    // Bezier curves are easy to breakdown if you know the parametric point at
    // which you wish to breakdown them. To find that we have to solve for the u
    // value at which the time curve reaches knotData.time.

    // If the segment is regressive, de-regress it.  Our eval-time behavior
    // always uses the Keep Ratio strategy. Note that this may change prevData
    // and nextData. This is fine since we're changing their tangents anyway
    // once we breakdown the segment.
    Ts_RegressionPreventerBatchAccess::ProcessSegment(
        &prevData, &nextData, TsAntiRegressionKeepRatio);

    double u = _FindBezierParameter(prevData, nextData, knotData.time);

    // Get the Bezier control points.  Note that there is similar code in
    // _Sampler::_SubdivideBezier in sample.cpp
    GfVec2d cp[4] = {
        {prevData.time, prevData.value},
        {prevData.time + prevData.postTanWidth, 
         prevData.value + prevData.GetPostTanHeight()},
        {nextData.time - nextData.preTanWidth,
         nextData.value + nextData.GetPreTanHeight()},
        {nextData.time, nextData.value}};

    // Run one De Casteljau interpolation
    GfVec2d cp01   = GfLerp(u, cp[0], cp[1]);
    GfVec2d cp12   = GfLerp(u, cp[1], cp[2]);
    GfVec2d cp23   = GfLerp(u, cp[2], cp[3]);

    GfVec2d cp012  = GfLerp(u, cp01, cp12);
    GfVec2d cp123  = GfLerp(u, cp12, cp23);

    GfVec2d cp0123 = GfLerp(u, cp012, cp123);

    GfVec2d prevCp[4] = {cp[0], cp01, cp012, cp0123};
    GfVec2d nextCp[4] = {cp0123, cp123, cp23, cp[3]};

    // Rebuild the knot values.
    GfVec2d prevDataPostTan = prevCp[1] - prevCp[0];
    GfVec2d knotDataPreTan  = prevCp[3] - prevCp[2];
    GfVec2d knotDataPostTan = nextCp[1] - nextCp[0];
    GfVec2d nextDataPreTan  = nextCp[3] - nextCp[2];

    // Since nothing was vertical going in, nothing should be vertical coming
    // out, but someone could try to breakdown a Bezier at a time that is very,
    // very, very close to one of the end points and we'd get some numerical
    // instability. Or perhaps a chemist is animating something in
    // femtoseconds...
    //

    // Convert a tangent from a direction vector to a width, slope
    auto _ConvertTangent =
        [](const GfVec2d& tangent) -> GfVec2d
        {
            // Tweak non-zero vertical tangents slightly off vertical so we can
            // compute a slope. Guessing that 1 arc second (1/3600 of 1 degree)
            // is a safe value. We're going to approximate that by 5.0e-6, so a
            // near vertical slope is 1.0 / 5.0e-6 or 200000
            constexpr double nearVertical = 200000.0;

            GfVec2d result;

            if (tangent[0] == 0) {
                if (tangent[1] == 0) {
                    // Zero length tangent. Slope could be any value so pick 0.
                    // Width can also safely be 0
                    result[0] = 0.0;
                    result[1] = 0.0;
                } else {
                    // Vertical but non-zero length tangent. Rotate it slightly
                    // approximating sin(angle) == angle and cos(angle) == 1.
                    result[0] = std::abs(tangent[1]) / nearVertical;
                    result[1] = std::copysign(nearVertical, tangent[1]);
                }
            } else {
                result[0] = tangent[0];
                result[1] = tangent[1] / tangent[0];
            }
            return result;
        };

    // Convert our tangent vectors into (width, slope) tuples
    prevDataPostTan = _ConvertTangent(prevDataPostTan);
    knotDataPreTan = _ConvertTangent(knotDataPreTan);
    knotDataPostTan = _ConvertTangent(knotDataPostTan);
    nextDataPreTan = _ConvertTangent(nextDataPreTan);
    
    prevData.postTanWidth = prevDataPostTan[0];
    prevData.postTanSlope = prevDataPostTan[1];

    knotData.preTanWidth = knotDataPreTan[0];
    knotData.preTanSlope = knotDataPreTan[1];

    knotData.value = cp0123[1];

    knotData.postTanWidth = knotDataPostTan[0];
    knotData.postTanSlope = knotDataPostTan[1];

    nextData.preTanWidth = nextDataPreTan[0];
    nextData.preTanSlope = nextDataPreTan[1];

    // Turn off any tangent algorithms to keep them from messing things up.
    prevData.postTanAlgorithm = TsTangentAlgorithmNone;
    knotData.preTanAlgorithm = TsTangentAlgorithmNone;
    knotData.postTanAlgorithm = TsTangentAlgorithmNone;
    nextData.preTanAlgorithm = TsTangentAlgorithmNone;

    // If the segment was not regressive then the split pieces should not be
    // either, but if the original segment was on the edge of being regressive
    // then round-off errors may make one of the new segments ever so slightly
    // regressive. Rather than handling that at each evaluation, handle it now.
    Ts_RegressionPreventerBatchAccess::ProcessSegment(
        &prevData, &knotData, TsAntiRegressionKeepRatio);
    Ts_RegressionPreventerBatchAccess::ProcessSegment(
        &knotData, &nextData, TsAntiRegressionKeepRatio);

    return true;
}

// Interpolate between two knots.
//
static bool
_BreakdownBetweenKnots(
    const Ts_SplineData* const data,
    Ts_TypedKnotData<double>& prevData,
    Ts_TypedKnotData<double>& knotData,
    Ts_TypedKnotData<double>& nextData)
{
    switch (prevData.nextInterp) {
      case TsInterpValueBlock:
        knotData.nextInterp = TsInterpValueBlock;
        return true;

      case TsInterpHeld:
        knotData.nextInterp = TsInterpHeld;
        knotData.value = prevData.value;
        return true;

      case TsInterpLinear:
        {
            const double slope = _GetSegmentSlope(prevData, nextData);
            knotData.nextInterp = TsInterpLinear;
            knotData.value = _ExtrapolateLinear(prevData, slope,
                                                knotData.time, Ts_EvalPost);
            return true;
        }

      case TsInterpCurve:
        knotData.nextInterp = TsInterpCurve;
        if (data->curveType == TsCurveTypeBezier)
        {
            return _BreakdownBezier(data, prevData, knotData, nextData);
        }
        else
        {
            return _BreakdownHermite(data, prevData, knotData, nextData);
        }
    }

    return false;
}

bool
_BreakdownMain(
    Ts_SplineData* const data,
    const TsTime atTime,
    const bool testOnly,
    const _LoopResolver &loopRes,
    GfInterval* affectedIntervalOut,
    std::string* reason)
{
    size_t idx;
    // Get a handy alias to the times array.
    const std::vector<TsTime> &times = data->times;

    const bool haveInnerLoops = data->HasInnerLoops();
    const bool haveMultipleKnots =
        haveInnerLoops || (times.size() > 1);

    // For now, we're not allowing breakdown on a time that is either masked
    // by inner looping or is in the extrapolated looping section.
    //
    // A breakdown in the region masked by inner looping would have no effect
    // and a breakdown in an extrapolated looping section would have widespread
    // effect.
    //
    // You can breakdown in an inner looping prototype or in an extrapolated
    // region that is not looping.

    // If we're at a time that's masked by inner looping, do not breakdown.
    GfInterval protoInterval, loopedInterval;
    if (haveInnerLoops) {
        protoInterval = data->loopParams.GetPrototypeInterval();
        loopedInterval = data->loopParams.GetLoopedInterval();
        if (loopedInterval.Contains(atTime) &&
            !protoInterval.Contains(atTime))
        {
            *reason = TfStringPrintf(
                "Cannot breakdown a spline in a region masked by inner looping"
                " (time=%g).", atTime);
            if (!testOnly) {
                TF_WARN(*reason);
            }

            return false;
        }
    }

    GfInterval knotInterval(times.front(), times.back());
    if (haveInnerLoops) {
        // Union in the looped interval.
        knotInterval |= loopedInterval;
    }

    // Or, if we're in a looped extrapolation region then we cannot breakdown
    // without significantly changing the shape of the curve.
    if (haveMultipleKnots
        && ((atTime < knotInterval.GetMin()
             && data->preExtrapolation.IsLooping())
            || (atTime > knotInterval.GetMax()
                && data->postExtrapolation.IsLooping())))
    {
        *reason = TfStringPrintf(
            "Cannot breakdown a spline in a region generated by extrapolation"
            " looping (time=%g).", atTime);
        if (!testOnly) {
            TF_WARN(*reason);
        }

        return false;
    }
    

    // _LoopResolver is for resolving loops. It generally resolves them by
    // mapping a requested input time to an eval time that is somewhere in the
    // loop prototype. So if the computed eval time is different than the
    // requested time, we have requested a time that is generated by looping and
    // breakdown is not available.
    //
    // These checks were done explicity above so it should not be possible to
    // fail this test. The test is here mainly to protect against future code
    // changes that may challenge these assumptions.
    if (!TF_VERIFY(loopRes.GetEvalTime() == atTime,
                   "Verification failure: Cannot breakdown a spline in a looped"
                   " region at time = %g. Please report a bug.", atTime))
    {
        *reason = TfStringPrintf(
            "Verification failure: Cannot breakdown a spline in a looped"
            " region at time = %g. Please report a bug.", atTime);
            
        return false;
    }

    // Track whether the prev and next knots are real knots in the spline or
    // are generated by looping.
    bool generatedPrev, generatedNext;

    // Use binary search to find first knot at or after the specified time.
    const auto lbIt = std::lower_bound(times.begin(), times.end(), atTime);

    // Short circuit if we are exactly at a knot time. If there's already
    // a knot there then we're done.
    const bool atKnot = (lbIt != times.end() && *lbIt == atTime);
    if (atKnot) {
        // There's already a knot at that time.
        *reason = TfStringPrintf(
            "Cannot breakdown a spline at an existing knot. (time=%g)",
            atTime);
        if (!testOnly) {
            TF_WARN(*reason);
        }

        return false;
    }

    // If we passed all the above tests, the breakdown should succeed.
    if (testOnly) {
        return true;
    }

    // We're going to affect the current time.
    *affectedIntervalOut = GfInterval(atTime);

    // Figure out where we are in the sequence.  Find the bracketing knots, the
    // knot we're at, if any, and what type of position (before start, after
    // end, at first knot, at last knot, at another knot, between knots).
    const auto prevIt = (lbIt != times.begin() ? lbIt - 1 : times.end());
    const auto nextIt = lbIt;
    const bool beforeStart = (nextIt == times.begin());
    const bool afterEnd = (!loopRes.IsBetweenLastProtoAndEnd() &&
                           nextIt == times.end());

    // Neighboring knot parameters.
    Ts_TypedKnotData<double> knotData, prevData, nextData;
    
    if (prevIt != times.end())
    {
        prevData = data->GetKnotDataAsDouble(prevIt - times.begin());
    }
    if (nextIt != times.end())
    {
        nextData = data->GetKnotDataAsDouble(nextIt - times.begin());
    }

    // Data for the new knot. (We'll convert it to the right type after
    // we figure out the values.
    knotData.time = atTime;

    // Extrapolate before first knot.
    if (beforeStart)
    {
        // nextData is the first knot.  We also need the knot after that, if
        // there is one.
        Ts_TypedKnotData<double> nextData2;
        if (nextIt + 1 != times.end())
        {
            nextData2 = data->GetKnotDataAsDouble((nextIt + 1) - times.begin());
        }

        loopRes.ReplacePreExtrapKnots(&nextData, &nextData2, &generatedNext);

        if (data->preExtrapolation.mode == TsExtrapValueBlock) {
            // Time is already set. Everything else default initialized.
            // No need to change the next knot.
            knotData.nextInterp = TsInterpValueBlock;
            idx = data->SetKnotFromDouble(&knotData, VtDictionary());
            data->UpdateKnotTangentsAtIndex(idx);
            data->UpdateKnotTangentsAtIndex(idx + 1);
            affectedIntervalOut->SetMax(nextData.time,
                                        !generatedNext);  // maxClosed
            return true;
        }

         // Find the extrapolation slope.
        const std::optional<double> slope =
            _GetExtrapolationSlope(
                data->preExtrapolation,
                haveMultipleKnots, nextData, nextData2,
                Ts_EvalPre);

        // No slope should only occur for value blocks, and value blocks were
        // handled above.
        if (!TF_VERIFY(slope,
                       "Verification failure: Cannot compute pre-extrapolation"
                       " slope for spline breakdown. Please report a bug."))
        {
            *reason = "Verification failure: Cannot compute pre-extrapolation"
                      " slope for spline breakdown. Please report a bug.";
            return false;
        }

        // Extrapolate value.
        knotData.value = _ExtrapolateLinear(nextData, *slope, atTime,
                                            Ts_EvalPre);

        switch (data->preExtrapolation.mode)
        {
          case TsExtrapHeld:
            knotData.nextInterp = TsInterpHeld;
            break;

          case TsExtrapLinear:
          case TsExtrapSloped:
            knotData.nextInterp = TsInterpLinear;
            knotData.preTanSlope = knotData.postTanSlope = *slope;
            break;

          default:
            // This covers TsExtrapLoopRepeat, TsExtrapLoopReset,
            // TsExtrapLoopOscillate, TsExtrapValueBlock, and any other future
            // extrapolation modes that were not handled above.  (They should
            // all be handled above.)
            *reason = TfStringPrintf(
                "Verification failure: Invalid pre-extrapolation mode (%s)"
                " when breaking down spline. Please report a bug.",
                TfEnum::GetName(data->preExtrapolation.mode).c_str());
            TF_CODING_ERROR(*reason);

            return false;
        }

        // There's no prevData from which to compute a preTanWidth, so match the
        // next knot instead.
        knotData.preTanWidth = nextData.preTanWidth;
        knotData.preTanAlgorithm = nextData.preTanAlgorithm;
        knotData.postTanWidth = (nextData.time - knotData.time) / 3;
        knotData.postTanAlgorithm = nextData.preTanAlgorithm;

        // XXX: Think about propagating customData.
        idx = data->SetKnotFromDouble(&knotData, VtDictionary());
        data->UpdateKnotTangentsAtIndex(idx);
        data->UpdateKnotTangentsAtIndex(idx + 1);

        affectedIntervalOut->SetMax(nextData.time,
                                    !generatedNext);  // maxClosed

        return true;
    }

    // Extrapolate after last knot.
    if (afterEnd)
    {
        // prevData is the last knot.  We also need the knot before that, if
        // there is one.
        Ts_TypedKnotData<double> prevData2;
        if (prevIt != times.begin())
        {
            prevData2 = data->GetKnotDataAsDouble((prevIt - times.begin()) - 1);
        }

        loopRes.ReplacePostExtrapKnots(&prevData, &prevData2, &generatedPrev);

        if (data->postExtrapolation.mode == TsExtrapValueBlock) {
            // Time is already set. Everything else default initialized.
            knotData.nextInterp = TsInterpValueBlock;
            size_t idx = data->SetKnotFromDouble(&knotData, VtDictionary());
            idx = data->UpdateKnotTangentsAtIndex(idx);
            data->UpdateKnotTangentsAtIndex(idx);
            *affectedIntervalOut = GfInterval(atTime);

            // Ensure that prevData's interpolation type is value-block. Note
            // that if prevData was generated by looping that changing the
            // generated data will not change spline's data and the spline shape
            // will change due to the inserted knot.
            prevData.nextInterp = TsInterpValueBlock;
            idx = data->SetKnotFromDouble(&prevData, VtDictionary());
            data->UpdateKnotTangentsAtIndex(idx);
            affectedIntervalOut->SetMin(prevData.time,
                                        !generatedPrev);  // minClosed
            
            return true;
        }

        // Find the extrapolation slope.
        const std::optional<double> slope =
            _GetExtrapolationSlope(
                data->postExtrapolation,
                haveMultipleKnots, prevData, prevData2,
                Ts_EvalPost);

        // No slope should only occur for value blocks, and value blocks were
        // handled above.
        if (!TF_VERIFY(slope,
                       "Cannot compute post-extrapolation slope in order to"
                       " breakdown spline."))
        {
            *reason = "Coding error: Cannot compute post-extrapolation slope in"
                      " order to breakdown spline.";
            return false;
        }

        // Extrapolate value.
        knotData.value = _ExtrapolateLinear(prevData, *slope, atTime,
                                            Ts_EvalAtTime);

        switch (data->postExtrapolation.mode)
        {
          case TsExtrapHeld:
            knotData.nextInterp = TsInterpHeld;
            if (prevData.nextInterp != TsInterpHeld) {
                prevData.nextInterp = TsInterpHeld;
                idx = data->SetKnotFromDouble(&prevData, VtDictionary());
                data->UpdateKnotTangentsAtIndex(idx);
            }
            break;

          case TsExtrapLinear:
          case TsExtrapSloped:
            knotData.nextInterp = TsInterpLinear;
            knotData.preTanSlope = knotData.postTanSlope = *slope;
            prevData.nextInterp = TsInterpLinear;
            break;

          default:
            // This covers TsExtrapLoopRepeat, TsExtrapLoopReset,
            // TsExtrapLoopOscillate, TsExtrapValueBlock, and any other future
            // extrapolation modes that were not handled above.  (They should
            // all be handled above.)
            *reason = TfStringPrintf(
                "Verification failure: Invalid post-extrapolation mode (%s)"
                " when breaking down spline. Please report a bug.",
                TfEnum::GetName(data->postExtrapolation.mode).c_str());
            TF_CODING_ERROR(*reason);
            return false;
        }

        // There's no nextKnot from which to compute a postTanWidth, so match
        // the prev knot instead.
        knotData.preTanWidth = (prevData.time - knotData.time) / 3;
        knotData.preTanAlgorithm = prevData.postTanAlgorithm;
        knotData.postTanWidth = prevData.postTanWidth;
        knotData.postTanAlgorithm = prevData.postTanAlgorithm;

        // XXX: Think about propagating customData.
        idx = data->SetKnotFromDouble(&knotData, VtDictionary());
        data->UpdateKnotTangentsAtIndex(idx);
        data->UpdateKnotTangentsAtIndex(idx - 1);

        affectedIntervalOut->SetMin(prevData.time,
                                    !generatedPrev);  // minClosed

        return true;
    }

    // Otherwise we are between knots.

    // Account for loop-boundary cases.
    loopRes.ReplaceBoundaryKnots(&prevData, &nextData,
                                 &generatedPrev, &generatedNext);

    
    // Interpolate. Note that prevData, knotData, and nextData may be modified
    // in place.
    bool result = _BreakdownBetweenKnots(data, prevData, knotData, nextData);

    if (result) {
        if (!generatedPrev) {
            data->SetKnotFromDouble(&prevData, VtDictionary());
        }

        idx = data->SetKnotFromDouble(&knotData, VtDictionary());

        if (!generatedNext) {
            data->SetKnotFromDouble(&nextData, VtDictionary());
        }

        data->UpdateKnotTangentsAtIndex(idx - 1);
        data->UpdateKnotTangentsAtIndex(idx);
        data->UpdateKnotTangentsAtIndex(idx + 1);

        *affectedIntervalOut = GfInterval(prevData.time,
                                          nextData.time,
                                          !generatedPrev,   // minClosed
                                          !generatedNext);  // maxClosed
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////
// BREAKDOWN ENTRY POINT

bool
Ts_Breakdown(
    Ts_SplineData* const data,
    const TsTime atTime,
    const bool testOnly,
    GfInterval *affectedIntervalOut,
    std::string* reason)
{
    // Cannot split an empty spline.
    if (data->times.empty())
    {
        *reason = "Cannot breakdown an empty spline.";
        return false;
    }

    // We use the same loop resolver as Ts_Eval so we can breakdown the presence
    // of looping and special interpolation cases.
    _LoopResolver loopRes(data, atTime, Ts_EvalValue, Ts_EvalAtTime);

    return _BreakdownMain(data, atTime, testOnly, loopRes, affectedIntervalOut,
                          reason);
}


PXR_NAMESPACE_CLOSE_SCOPE
