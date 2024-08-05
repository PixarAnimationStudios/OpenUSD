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

static double _RealCubeRoot(
    const double x)
{
    static constexpr double oneThird = 1.0/3.0;
    return (x >= 0 ? std::pow(x, oneThird) : -std::pow(-x, oneThird));
}

static double _FilterZeroes(
    const std::vector<double> &candidates)
{
    double result = 0;
    int numFound = 0;

    for (double c : candidates)
    {
        if (c >= 0 && c <= 1)
        {
            result = c;
            numFound++;
        }
    }

    TF_VERIFY(numFound == 1);
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
    return _FilterZeroes({root0, root1});
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
        const double t1 = 2 * _RealCubeRoot(r);
        const double root1 = t1 * std::cos(phi/3) - b3;
        const double root2 = t1 * std::cos((phi + 2*M_PI) / 3) - b3;
        const double root3 = t1 * std::cos((phi + 4*M_PI) / 3) - b3;
        return _FilterZeroes({root1, root2, root3});
    }
    else if (discrim == 0)
    {
        // Two real roots.
        const double u1 = -_RealCubeRoot(q2);
        const double root1 = 2*u1 - b3;
        const double root2 = -u1 - b3;
        return _FilterZeroes({root1, root2});
    }
    else
    {
        // One real root.
        const double sd = std::sqrt(discrim);
        const double u1 = _RealCubeRoot(sd - q2);
        const double v1 = _RealCubeRoot(sd + q2);
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

    // Find the coefficients for x = f(t).
    // Offset everything by the eval time, so that we can just find a zero.
    const _Cubic timeCubic = _Cubic::FromPoints(
        beginData.time - time,
        beginData.time + beginData.GetPostTanWidth() - time,
        endData.time - endData.GetPreTanWidth() - time,
        endData.time - time);

    // Find the value of t for which f(t) = 0.
    // Due to the offset, this is the t-value at which we reach the eval time.
    const double t = _FindMonotonicZero(timeCubic);

    // t should always be in [0, 1], but tolerate some slight imprecision.
    static constexpr double epsilon = 1e-10;
    if (t <= 0)
    {
        TF_VERIFY(t > -epsilon);
        return beginData.value;
    }
    else if (t >= 1)
    {
        TF_VERIFY(t < 1 + epsilon);
        return endData.value;
    }

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
    // XXX TODO
    TF_WARN("Hermite evaluation is not yet implemented");
    return 0.0;
}

////////////////////////////////////////////////////////////////////////////////
// EVAL HELPERS

// Find the slope at a knot, facing into a curved segment.
//
// Accounts for Maya vs. standard tangent forms, and forced tangent widths for
// Hermite curves.
//
static double
_GetCurveKnotSlope(
    const Ts_TypedKnotData<double> &knotData,
    const TsTime adjacentTime,
    const TsCurveType curveType,
    const Ts_EvalLocation location)
{
    if (location == Ts_EvalPre)
    {
        if (!knotData.preTanMayaForm)
        {
            return knotData.preTanSlope;
        }
        else if (curveType == TsCurveTypeHermite)
        {
            return -knotData.preTanMayaHeight / (knotData.time - adjacentTime);
        }
        else
        {
            return -knotData.preTanMayaHeight / knotData.preTanWidth;
        }
    }
    else
    {
        if (!knotData.postTanMayaForm)
        {
            return knotData.postTanSlope;
        }
        else if (curveType == TsCurveTypeHermite)
        {
            return knotData.postTanMayaHeight / (adjacentTime - knotData.time);
        }
        else
        {
            return knotData.postTanMayaHeight / knotData.postTanWidth;
        }
    }
}

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
    const TsCurveType curveType,
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
        // If the first segment is held, the slope is flat.
        if (endKnotData.nextInterp == TsInterpHeld)
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
        return _GetCurveKnotSlope(
            endKnotData, adjacentData.time, curveType, Ts_EvalPost);
    }
    else
    {
        // If the last segment is held, the slope is flat.
        if (adjacentData.nextInterp == TsInterpHeld)
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
        return _GetCurveKnotSlope(
            endKnotData, adjacentData.time, curveType, Ts_EvalPre);
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
            Ts_TypedKnotData<double> *nextData) const;
        void ReplacePreExtrapKnots(
            Ts_TypedKnotData<double> *nextData,
            Ts_TypedKnotData<double> *nextData2) const;
        void ReplacePostExtrapKnots(
            Ts_TypedKnotData<double> *prevData,
            Ts_TypedKnotData<double> *prevData2) const;

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
    const TsTime rawFirstTime = _firstTime = *(_data->times.begin());
    const TsTime rawLastTime = _lastTime = *(_data->times.rbegin());
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
    Ts_TypedKnotData<double> *nextData) const
{
    const TsLoopParams &lp = _data->loopParams;

    // Case 1: between last prototype knot and prototype end, after performing
    // shift out of echo region, if any.  Make a copy of the first prototype
    // knot at the end of the prototype region, and use that as nextData.
    if (_betweenLastProtoAndEnd)
    {
        *nextData = _CopyProtoKnotData(
            _firstInnerProtoIndex, 1);
    }

    // Case 2: between last knot before looping region and start of looping
    // region.  Make a copy of the first prototype knot at the start of the
    // looping region, and use that as nextData.
    else if (_betweenPreUnloopedAndLooped)
    {
        *nextData = _CopyProtoKnotData(
            _firstInnerProtoIndex, -lp.numPreLoops);
    }

    // Case 3: between end of looping region and first knot after looping
    // region.  Make a copy of the first prototype knot at the end of the
    // looping region, and use that as prevData.
    else if (_betweenLoopedAndPostUnlooped)
    {
        *prevData = _CopyProtoKnotData(
            _firstInnerProtoIndex, lp.numPostLoops + 1);
    }
}

void _LoopResolver::ReplacePreExtrapKnots(
    Ts_TypedKnotData<double>* const nextData,
    Ts_TypedKnotData<double>* const nextData2) const
{
    if (!_firstTimeLooped)
        return;

    *nextData = _extrapKnot1;
    *nextData2 = _extrapKnot2;
}

void _LoopResolver::ReplacePostExtrapKnots(
    Ts_TypedKnotData<double>* const prevData,
    Ts_TypedKnotData<double>* const prevData2) const
{
    if (!_lastTimeLooped)
        return;

    *prevData = _extrapKnot1;
    *prevData2 = _extrapKnot2;
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
    const Ts_EvalAspect aspect)
{
    // Special-case held evaluation.
    if (aspect == Ts_EvalHeldValue)
    {
        return beginData.value;
    }

    // Curved segment: Bezier/Hermite math.
    if (beginData.nextInterp == TsInterpCurve)
    {
        if (beginData.curveType == TsCurveTypeBezier)
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

    // Disabled interpolation -> no value.
    if (beginData.nextInterp == TsInterpValueBlock)
    {
        return std::nullopt;
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
    const auto prevIt = (lbIt != times.begin() ? lbIt - 1 : times.end());
    const bool atKnot = (lbIt != times.end() && *lbIt == time);
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
            if (location == Ts_EvalPre
                    && !atFirst && prevData.nextInterp == TsInterpHeld)
            {
                return prevData.value;
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
                        data->curveType, Ts_EvalPre);
                }

                // Derivative in held segment = zero.
                if (prevData.nextInterp == TsInterpHeld)
                {
                    return 0.0;
                }

                // Derivative in linear segment = slope to adjacent knot.
                if (prevData.nextInterp == TsInterpLinear)
                {
                    return _GetSegmentSlope(prevData, knotData);
                }

                // Not a special case.  Return what's stored in the knot.
                return _GetCurveKnotSlope(
                    knotData, prevData.time, data->curveType, Ts_EvalPre);
            }
            else
            {
                // Post-derivative at last knot = extrapolation slope.
                if (atLast)
                {
                    return _GetExtrapolationSlope(
                        data->postExtrapolation,
                        haveMultipleKnots, knotData, prevData,
                        data->curveType, Ts_EvalPost);
                }

                // Derivative in held segment = zero.
                if (knotData.nextInterp == TsInterpHeld)
                {
                    return 0.0;
                }

                // Derivative in linear segment = slope to adjacent knot.
                if (knotData.nextInterp == TsInterpLinear)
                {
                    return _GetSegmentSlope(knotData, nextData);
                }

                // Not a special case.  Return what's stored in the knot.
                return _GetCurveKnotSlope(
                    knotData, nextData.time, data->curveType, Ts_EvalPost);
            }
        }
    }

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

        loopRes.ReplacePreExtrapKnots(&nextData, &nextData2);

        // Special-case held evaluation.
        if (aspect == Ts_EvalHeldValue)
        {
            return nextData.GetPreValue();
        }

        // Find the extrapolation slope.
        const std::optional<double> slope =
            _GetExtrapolationSlope(
                data->preExtrapolation,
                haveMultipleKnots, nextData, nextData2,
                data->curveType, Ts_EvalPre);

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
                data->curveType, Ts_EvalPost);

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
    return _Interpolate(prevData, nextData, time, aspect);
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


PXR_NAMESPACE_CLOSE_SCOPE
