//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/sample.h"
#include "pxr/base/ts/splineData.h"
#include "pxr/base/ts/regressionPreventer.h"
#include "pxr/base/ts/debugCodes.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/tf/diagnostic.h"

#include <algorithm>
#include <cmath>

PXR_NAMESPACE_OPEN_SCOPE

// XXX: Should this go in sample.h? Or maybe even knotData.h?
using Ts_DoubleKnotData = Ts_TypedKnotData<double>;

////////////////////////////////////////////////////////////////////////////////
// SAMPLING

namespace
{
    // Each spline can have as many as seven intervals that are populated from
    // different sources, for example, pre-extrapolation loops, inner loops,
    // post-extrapolation, etc.
    //
    // _SourceInterval holds the time interval for a source.
    struct _SourceInterval
    {
        TsSplineSampleSource source;
        GfInterval interval;

        _SourceInterval(TsSplineSampleSource inSource,
                        TsTime t1,
                        TsTime t2)
        : source(inSource)
        , interval{t1, t2, true, false}
        {}
    };

    // _Sampler constructs a partially unrolled version of the spline and then
    // samples that version. Only the inner loops are unrolled and only in the
    // region where sampling will be occurring.
    //
    // The unrolled version enables random access to all the relevant knots
    // and we implement extrapolation looping with simple time and value
    // shifting.
    //
    // If includeExtrapLoops is true, the baking routines will include knots from
    // extrapolation in the baked knots. It is an error to include extrapolation
    // knots with an infinite time range because it would result in an infinite
    // number of knots.
    class _Sampler
    {
        struct _UnrolledKnot {
            const size_t knotIndex;
            const TsTime timeOffset;
            const double valueOffset;

            _UnrolledKnot(const size_t i, const TsTime tOff, const double vOff)
            : knotIndex(i)
            , timeOffset(tOff)
            , valueOffset(vOff)
            { }

            const Ts_KnotData* GetKnotDataPtr(const Ts_SplineData* data) const
            {
                return data->GetKnotPtrAtIndex(knotIndex);
            }

            template <typename T>
            const Ts_TypedKnotData<T>&
            GetKnotData(const Ts_TypedSplineData<T>* typedData) const {
                return typedData->knots[knotIndex];
            }

            TsTime GetTime(const Ts_SplineData* data) const
            {
                return GetKnotDataPtr(data)->time + timeOffset;
            }

            double GetValue(const Ts_SplineData* data) const {
                return data->GetKnotValueAsDouble(knotIndex) + valueOffset;
            }

            // More efficient templated version.
            template <typename T>
            double GetValue(const Ts_TypedSplineData<T>* typedData) const {
                return GetKnotData(typedData).value + valueOffset;
            }

            // More efficient templated version.
            template <typename T>
            double GetPreValue(const Ts_TypedSplineData<T>* typedData) const {
                return GetKnotData(typedData).GetPreValue() + valueOffset;
            }
        };

        struct _UnrolledKnotTimeLess {
            _UnrolledKnotTimeLess(const Ts_SplineData* data)
            : _data(data)
            { }

            // Overload operator() and allow comparison to other double as well
            // as other _UnrolledKnot objects
            bool operator()(const _UnrolledKnot& lhs, const _UnrolledKnot& rhs) {
                return lhs.GetTime(_data) < rhs.GetTime(_data);
            }
            bool operator()(const _UnrolledKnot& lhs, TsTime rhsTime) {
                return lhs.GetTime(_data) < rhsTime;
            }
            bool operator()(TsTime lhsTime, const _UnrolledKnot& rhs) {
                return lhsTime < rhs.GetTime(_data);
            }

            const Ts_SplineData* _data;
        };
        
    public:
        // Constructor for sampling
        _Sampler(
            const Ts_SplineData* data,
            const GfInterval& timeInterval,
            double timeScale,
            double valueScale,
            double tolerance);

        // Constructor for baking
        _Sampler(
            const Ts_SplineData* const data,
            const GfInterval& timeInterval,
            const bool includeExtrapLoops,
            Ts_SplineData* bakedData);

        bool Sample(
            Ts_SampleDataInterface* sampledSpline);

        bool SampleInterval(
            const GfInterval& subInterval,
            Ts_SampleDataInterface* sampledSpline);

        bool Bake(Ts_SplineData* bakedData);

    private:
        // Common initialization code for both baking and sampling
        void _Init();

        // Sample knots in sampleInterval. Sampled knot times are converted to
        // sample times with _ToSampleTime and values are offset by valueOffset
        // before being stored in sampledSpline.
        void _SampleKnots(
            const GfInterval& sampleInterval,
            const TsSplineSampleSource source,
            const double knotToSampleTimeScale,
            const TsTime knotToSampleTimeOffset,
            const double valueOffset,
            Ts_SampleDataInterface* sampledSpline);

        // Sample knots in sampleInterval in reverse. Sampled knot times are
        // converted to sample times with _ToSampleTime before being stored in
        // sampledSpline. Note that oscillating loops always have a valueOffset
        // of 0.0 so we do not need it as an argument.
        void _SampleKnotsReversed(
            const GfInterval& sampleInterval,
            const TsSplineSampleSource source,
            const double knotToSampleTimeScale,
            const TsTime knotToSampleTimeOffset,
            Ts_SampleDataInterface* sampledSpline);

        // Sample a segment of the spline between 2 adjacent knots.
        void _SampleSegment(const Ts_DoubleKnotData* prevKnot,
                            const Ts_DoubleKnotData* nextKnot,
                            const GfInterval& segmentInterval,
                            TsSplineSampleSource source,
                            double knotToSampleTimeScale,
                            double knotToSampleTimeOffset,
                            double valueOffset,
                            Ts_SampleDataInterface* sampledSpline);

        // Sample a segment of the spline between 2 adjacent knots.
        void _SampleCurveSegment(const Ts_DoubleKnotData* prevKnot,
                                 const Ts_DoubleKnotData* nextKnot,
                                 const GfInterval& segmentInterval,
                                 TsSplineSampleSource source,
                                 double knotToSampleTimeScale,
                                 double knotToSampleTimeOffset,
                                 double valueOffset,
                                 Ts_SampleDataInterface* sampledSpline);

        void _SampleBezier(GfVec2d cp[4],
                           const GfInterval& segmentInterval,
                           TsSplineSampleSource source,
                           double knotToSampleTimeScale,
                           double knotToSampleTimeOffset,
                           double valueOffset,
                           Ts_SampleDataInterface* sampledSpline);

        // Given a set of bezier control points and a u parameter in the
        // range [0..1], return 2 sets of control points for the left and
        // right parts of the original curve, split at u. It is allowable
        // for the cp input to also be one of the outputs.
        void _SubdivideBezier(const GfVec2d cp[4],
                              const double u,
                              GfVec2d leftCp[4],
                              GfVec2d rightCp[4]);

        void _ExtrapLinear(
            const GfInterval& regionInterval,
            const TsSplineSampleSource source,
            Ts_SampleDataInterface* sampledSpline);

        void _ExtrapLoop(
            const GfInterval& regionInterval,
            const TsSplineSampleSource source,
            Ts_SampleDataInterface* sampledSpline);

        // Unroll inner loops into the _unrolledKnots vector
        void _UnrollInnerLoops();

        // Convert the contents of _unrolledKnots to real knots in
        // _internalTimes and _internalKnots
        void _ConvertUnrolledKnotsForSampling();

        template <typename T>
        void _BakeTypedKnots(const Ts_TypedSplineData<T>* typedData,
                             const GfInterval& interval,
                             const TsExtrapolation* preExtrapPtr,
                             const TsExtrapolation* postExtrapPtr,
                             Ts_TypedSplineData<T>* typedBakedData);

        // Convert sample time to knot time.
        TsTime _ToKnotTime(TsTime sTime,
                           double knotToSampleTimeScale,
                           TsTime knotToSampleTimeOffset) {
            return (sTime - knotToSampleTimeOffset) / knotToSampleTimeScale;
        }

        // Convert knot time back to sample time.
        TsTime _ToSampleTime(TsTime kTime,
                             double knotToSampleTimeScale,
                             TsTime knotToSampleTimeOffset) {
            return kTime * knotToSampleTimeScale + knotToSampleTimeOffset;
        }

        // Inputs.
        const Ts_SplineData* const _data;
        const GfInterval _timeInterval;
        const double _timeScale = 1.0;
        const double _valueScale = 1.0;
        const double _tolerance = 1.0;
        const bool _includeExtrapLoops = false;
        Ts_SplineData* _bakedData = nullptr;  // in-out parameter

        // Intermediate data.
        bool _haveInnerLoops = false;
        bool _haveMultipleKnots = false;
        size_t _firstInnerProtoIndex = 0;
        bool _havePreExtrapLoops = false;
        bool _havePostExtrapLoops = false;
        TsTime _firstTime = 0;
        TsTime _lastTime = 0;
        TsTime _firstInnerLoop = 0;
        TsTime _lastInnerLoop = 0;
        TsTime _firstInnerProto = 0;
        TsTime _lastInnerProto = 0;
        bool _firstTimeLooped = false;
        bool _lastTimeLooped = false;

        std::vector<_SourceInterval> _sourceIntervals;

        // Pointers to vectors of knots and their times. If we are sampling and
        // there is no inner looping then these will point directly to the
        // spline data. Otherwise, the "internal" vectors below will be
        // populated and these will point at those. At no time does _knots nor
        // _times ever own the data that they point to.
        const std::vector<Ts_DoubleKnotData>* _knots;
        const std::vector<TsTime>* _times;

        std::vector<_UnrolledKnot> _unrolledKnots;

        // If we have to bake out the knots or times then we do so here and
        // point _knots and _times at these arrays.
        std::vector<Ts_DoubleKnotData> _internalKnots;
        std::vector<TsTime> _internalTimes;
    };
}

// Constructor for baking.
_Sampler::_Sampler(
    const Ts_SplineData* const data,
    const GfInterval& timeInterval,
    const bool includeExtrapLoops,
    Ts_SplineData* bakedData)
    : _data(data)
    , _timeInterval(timeInterval)
    , _timeScale(1.0)
    , _valueScale(1.0)
    , _tolerance(1.0)
    , _includeExtrapLoops(includeExtrapLoops)
    , _bakedData(bakedData)
{
    // It should be impossible to fail this check. If we do, we're likely to
    // crash or produce nonsense.
    TF_AXIOM(data && bakedData);

    _Init();
}

// Constructor for sampling.
_Sampler::_Sampler(
    const Ts_SplineData* const data,
    const GfInterval& timeInterval,
    const double timeScale,
    const double valueScale,
    const double tolerance)
    : _data(data)
    , _timeInterval(timeInterval)
    , _timeScale(timeScale)
    , _valueScale(valueScale)
    , _tolerance(tolerance)
    , _includeExtrapLoops(false)
    , _bakedData(nullptr)
{
    // It should be impossible to fail this check. If we do, we're likely to
    // crash or produce nonsense.
    TF_AXIOM(data &&
               !data->times.empty() &&
               !timeInterval.IsEmpty() &&
               timeScale > 0.0 &&
               valueScale > 0.0 &&
             tolerance > 0.0);

    _Init();
}

// Common initialization code for both baking and sampling
void
_Sampler::_Init()
{

    // Characterize the spline
    // Is inner looping enabled?
    _haveInnerLoops = _data->HasInnerLoops(&_firstInnerProtoIndex);

    // We have multiple knots if there are multiple authored.  We also always
    // have at least two knots if there is valid inner looping.
    _haveMultipleKnots =
        (_haveInnerLoops || _data->times.size() > 1);

    // Are any extrapolating loops enabled?
    _havePreExtrapLoops =
        _haveMultipleKnots && _data->preExtrapolation.IsLooping();
    _havePostExtrapLoops =
        _haveMultipleKnots && _data->postExtrapolation.IsLooping();

    // Find first and last knot times.  These may be authored, or they may be
    // echoed.
    const TsTime rawFirstTime = _firstTime = _data->times.front();
    const TsTime rawLastTime = _lastTime = _data->times.back();
    if (_haveInnerLoops)
    {
        _firstInnerProto = _data->loopParams.protoStart;
        _lastInnerProto = _data->loopParams.protoEnd;

        const GfInterval loopedInterval = _data->loopParams.GetLoopedInterval();

        _firstInnerLoop = loopedInterval.GetMin();
        _lastInnerLoop = loopedInterval.GetMax();

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

    // Populate _sourceIntervals
    if (_data->preExtrapolation.mode != TsExtrapValueBlock) {
        _sourceIntervals.emplace_back(
            _havePreExtrapLoops ? TsSourcePreExtrapLoop
                                : TsSourcePreExtrap,
            -std::numeric_limits<double>::infinity(),
            _firstTime);
    }

    if (_haveInnerLoops) {
        if (_firstTime < _firstInnerLoop) {
            _sourceIntervals.emplace_back(
                TsSourceKnotInterp,
                _firstTime, _firstInnerLoop);
        }
        if (_firstInnerLoop < _firstInnerProto) {
            _sourceIntervals.emplace_back(
                TsSourceInnerLoopPreEcho,
                _firstInnerLoop, _firstInnerProto);
        }
        _sourceIntervals.emplace_back(
            TsSourceInnerLoopProto,
            _firstInnerProto, _lastInnerProto);
        if (_lastInnerProto < _lastInnerLoop) {
            _sourceIntervals.emplace_back(
                TsSourceInnerLoopPostEcho,
                _lastInnerProto, _lastInnerLoop);
        }
        if (_lastInnerLoop < _lastTime) {
            _sourceIntervals.emplace_back(
                TsSourceKnotInterp,
                _lastInnerLoop, _lastTime);
        }
    } else {
        if (_firstTime < _lastTime) {
            _sourceIntervals.emplace_back(
                TsSourceKnotInterp,
                _firstTime, _lastTime);
        }
    }

    if (_data->postExtrapolation.mode != TsExtrapValueBlock) {
        _sourceIntervals.emplace_back(
            _havePostExtrapLoops ? TsSourcePostExtrapLoop
                                 : TsSourcePostExtrap,
            _lastTime,
            std::numeric_limits<double>::infinity());
    }

    // See if can avoid unrolling inner loops. If we're sampling (not baking),
    // there are no inner loops, and the knot dataType is already double, just
    // use the existing knot data.
    if (!_bakedData &&
        !_haveInnerLoops &&
        _data->GetValueType() == Ts_GetType<double>())
    {
        // We're sampling (not baking), there are no inner loops, and the knot
        // dataType is already double. Just point to the existing knots and use
        // them.
        const Ts_TypedSplineData<double>* _doubleData =
            dynamic_cast<const Ts_TypedSplineData<double>*>(_data);
        // The spline data already has everything we need.
        _knots = &_doubleData->knots;
        _times = &_data->times;
    } else {
        // We have to unroll the knots. They get unrolled into _unrolledKnots.
        _UnrollInnerLoops();

        // If we are baking, then we can work with _unrolledKnots. Otherwise we
        // need to convert them and update the _knots and _times pointers.
        if (!_bakedData) {
            // This populates _internalKnots and _internalTimes which is what
            // _knots and _times point at.
            _ConvertUnrolledKnotsForSampling();
        }
    }

    TF_DEBUG_MSG(
        TS_DEBUG_SAMPLE,
        "\n"
        "At _Sampler construction for %s:\n"
        "  _timeInterval: [%g .. %g]\n"
        "  _haveInnerLoops: %d\n"
        "  _havePreExtrapLoops: %d\n"
        "  _havePostExtrapLoops: %d\n"
        "  # of source regions: %zu\n"
        "  _firstTime:       %g\n"
        "  _firstInnerLoop:  %g\n"
        "  _firstInnerProto: %g\n"
        "  _lastInnerProto:  %g\n"
        "  _lastInnerLoop:   %g\n"
        "  _lastTime:        %g\n",
        (_bakedData ? "baking" : "sampling"),
        _timeInterval.GetMin(),
        _timeInterval.GetMax(),
        _haveInnerLoops,
        _havePreExtrapLoops,
        _havePostExtrapLoops,
        _sourceIntervals.size(),
        _firstTime,
        _firstInnerLoop,
        _firstInnerProto,
        _lastInnerProto,
        _lastInnerLoop,
        _lastTime);
}

bool
_Sampler::Sample(
    Ts_SampleDataInterface* sampledSpline)
{
    // Sample the entire input region _timeInterval
    return SampleInterval(_timeInterval,
                          sampledSpline);
}

bool
_Sampler::SampleInterval(
    const GfInterval& subInterval,
    Ts_SampleDataInterface* sampledSpline)
{
    if (_knots->empty()) {
        TF_CODING_ERROR("Cannot sample an empty spline!");
        return false;
    }

    for (const auto& si : _sourceIntervals) {
        GfInterval regionInterval = subInterval & si.interval;
        if (regionInterval.GetSize() > 0.0) {
            switch (si.source) {
              case TsSourcePreExtrap:
              case TsSourcePostExtrap:
                // All non-looping extrapolation modes are linear
                _ExtrapLinear(regionInterval, si.source, sampledSpline);
                break;

              case TsSourcePreExtrapLoop:
              case TsSourcePostExtrapLoop:
                _ExtrapLoop(regionInterval, si.source, sampledSpline);
                break;

              case TsSourceInnerLoopPreEcho:
              case TsSourceInnerLoopProto:
              case TsSourceInnerLoopPostEcho:
              case TsSourceKnotInterp:
                // Sample and knot times are the same here.
                _SampleKnots(regionInterval,
                             si.source,
                             1.0,       // knotToSampleTimeScale
                             0.0,       // knotToSampleTimeOffset
                             0.0,       // valueOffset
                             sampledSpline);
            }
        }
    }

    return true;
}

void
_Sampler::_ExtrapLinear(
    const GfInterval& regionInterval,
    const TsSplineSampleSource source,
    Ts_SampleDataInterface* sampledSpline)
{
    const TsExtrapolation* extrap;
    const Ts_DoubleKnotData* knot1;
    const Ts_DoubleKnotData* knot2;

    const bool isPre = (source == TsSourcePreExtrap);
    if (isPre) {
        extrap = &_data->preExtrapolation;
        knot1 = &_knots->front();
        knot2 = (_knots->size() > 1 ? knot1 + 1 : nullptr);
    } else {
        extrap = &_data->postExtrapolation;
        knot2 = &_knots->back();
        knot1 = (_knots->size() > 1 ? knot2 - 1 : nullptr);
    }

    double slope = 0.0;
    switch (extrap->mode) {
      case TsExtrapValueBlock:
        // No extrapolation, just return
        return;

      case TsExtrapHeld:
        // Extrapolation is flat
        slope = 0.0;
        break;

      case TsExtrapSloped:
        // Extrapolation slope is given
        slope = extrap->slope;
        break;

      case TsExtrapLoopRepeat:
      case TsExtrapLoopReset:
      case TsExtrapLoopOscillate:
        // Should have called _ExtrapLoop instead! This should be unreachable.
        TF_VERIFY(false,
                  "Invalid extrapolation mode (%s) in _Sampler::_ExtrapLinear",
                  TfEnum::GetName(extrap->mode).c_str());
        return;

      case TsExtrapLinear:
        // Extrapolate a straight line continuation using the slope at the
        // interpolated side of the end knot. If the end knot is dual valued or
        // the end segment is held or value blocked then the slope is flat. If
        // the end segment is linear the use the slope to the next-to-end
        // knot. And if the end segment is curved, use the slope specified by
        // the end knot's interpolated tangent.
        slope = 0.0;

        // If we have multiple knots and the knot at the end is not dual valued.
        if (_haveMultipleKnots &&
            ((isPre && !knot1->dualValued) ||
             (!isPre && !knot2->dualValued)))
        {
            if (knot1->nextInterp == TsInterpLinear) {
                // The last segment of the spline is linear, use the slope
                // between those two knots. They should never be at the
                // same time but let's ensure we don't divide by 0.
                if (knot1->time != knot2->time) {
                    slope = (knot2->GetPreValue() - knot1->value) /
                            (knot2->time - knot1->time);
                }
            } else if (knot1->nextInterp == TsInterpCurve) {
                // The last segment of the spline is curved, use the tangent on
                // the interpolated side of the edge knot.
                slope = (isPre ? knot1->postTanSlope : knot2->preTanSlope);
            }
        }
        // In all other cases, the extrapolation slope remains 0.0

        break;
    }

    TsTime t1 = regionInterval.GetMin();
    TsTime t2 = regionInterval.GetMax();

    double v1, v2;
    if (isPre) {
        v2 = knot1->GetPreValue();
        v1 = v2 - slope * (t2 - t1);
    } else {
        v1 = knot2->value;
        v2 = v1 + slope * (t2 - t1);
    }

    // There's only ever 1 segment
    sampledSpline->AddSegment(t1, v1, t2, v2, source);
}

void
_Sampler::_ExtrapLoop(
    const GfInterval& regionInterval,
    const TsSplineSampleSource source,
    Ts_SampleDataInterface* sampledSpline)
{
    // Figure out the time and value conversions and then invoke _SampleKnots,
    // possibly multiple times. Fortunately, for extrapolation looping we
    // are guaranteed that there is a knot at each end of the looped region.
    //
    // There are two different time ranges when we are extrapolating loops,
    // sample times and knot times. Sample times are the inputs and outputs
    // of this _ExtrapLoop. Knot times are the times that are stored in the
    // knots in the _knots array.
    //
    // Converting between these two time ranges involves both a scale and an
    // offset. We chose these values so we can use these equations:
    //    sampleTime = knotTime * knotToSampleScale + knotToSampleOffset
    // or the inverse:
    //    knotTime = (sampleTime - knotToSampleOffset) / knotToSampleScale
    // which are embodied in the methods _ToSampleTime and _ToKnotTime.
    //
    // _ExtrapLoop computes the appropriate scale and offset values.
    // _SampleKnots then samples the data using knot time values and
    // converts the results back to sample times when they are added
    // to sampledSpline.

    const bool isPre = (source == TsSourcePreExtrapLoop);
    const TsExtrapolation* const extrap =
        (isPre ? &_data->preExtrapolation : &_data->postExtrapolation);

    const Ts_DoubleKnotData& first = _knots->front();
    const Ts_DoubleKnotData& last  = _knots->back();

    const GfInterval knotInterval(_firstTime, _lastTime);
    const TsTime timeDelta = knotInterval.GetSize();

    // Do not use pre-value for the last knot. If the last knot is dual valued
    // we want to include that discontinuity in valueDelta.
    const double valueDelta =
        (extrap->mode == TsExtrapLoopRepeat ? last.value - first.value : 0.0);
    const bool oscillate = (extrap->mode == TsExtrapLoopOscillate);

    const TsTime minTime = regionInterval.GetMin();
    const TsTime maxTime = regionInterval.GetMax();

    const TsTime timeTolerance = _tolerance / _timeScale;

    // The entire timeline can be divided up into timeDelta sized spans
    // that we iterate over repeating the loop. Iteration 0 is the span
    // that contains the knots themselves, [_firstTime .. _lastTime).
    //
    // Determine the iteration numbers that we're asked to sample.
    const double minIter = (minTime - _firstTime) / timeDelta;
    const double maxIter = (maxTime - _firstTime) / timeDelta;

    // We don't want really tiny fractions of an iteration so round them
    // toward a smaller number of iterations within iterTolerance.
    const double iterTolerance = timeTolerance / timeDelta;

    const int64_t minIterNum = int64_t(std::floor(minIter + iterTolerance));
    const int64_t maxIterNum = int64_t(std::ceil(maxIter - iterTolerance));


    for (int64_t iterNum = minIterNum; iterNum < maxIterNum; ++iterNum) {
        if (iterNum == 0) continue;

        const bool reversed = (oscillate && (iterNum % 2 != 0));

        double knotToSampleTimeScale;
        TsTime knotToSampleTimeOffset;

        // Sample time values for the beginning and end of this
        // iteration.
        const TsTime firstIterTime = _firstTime + iterNum * timeDelta;
        const TsTime lastIterTime = _firstTime + (iterNum + 1) * timeDelta;

        if (reversed) {
            // Map from knot time to sample time.
            knotToSampleTimeScale = -1.0;
            knotToSampleTimeOffset = _lastTime + firstIterTime;
        } else {
            knotToSampleTimeScale = 1.0;
            knotToSampleTimeOffset = iterNum * timeDelta;
        }
        const double iterValueOffset = iterNum * valueDelta;

        // Interval for this single iteration of the loop in sample time
        const GfInterval iterInterval(firstIterTime, lastIterTime);
        // Clamped to the input sample region.
        const GfInterval sampleInterval = regionInterval & iterInterval;
        if (reversed) {
            _SampleKnotsReversed(sampleInterval,
                                 source,
                                 knotToSampleTimeScale,
                                 knotToSampleTimeOffset,
                                 sampledSpline);
        } else {
            _SampleKnots(sampleInterval,
                         source,
                         knotToSampleTimeScale,
                         knotToSampleTimeOffset,
                         iterValueOffset,
                         sampledSpline);
        }
    }
}

void
_Sampler::_SampleKnots(
    const GfInterval& sampleInterval,
    const TsSplineSampleSource source,
    const double knotToSampleTimeScale,
    const TsTime knotToSampleTimeOffset,
    const double valueOffset,
    Ts_SampleDataInterface* sampledSpline)
{
    const Ts_DoubleKnotData* prevKnot = nullptr;
    const Ts_DoubleKnotData* nextKnot = nullptr;
    const Ts_DoubleKnotData* endKnot = nullptr;

    // Shift the interval from sample to knot times by subtracting time offset
    // and clamping any rounding errors.
    const GfInterval knotInterval = (sampleInterval - GfInterval(knotToSampleTimeOffset)) &
                                    GfInterval(_firstTime, _lastTime);

    TsTime knotTime = knotInterval.GetMin();
    const TsTime knotEndTime = knotInterval.GetMax();

    // nextIt points to the knot after knotTime.
    //
    // endIt points to the knot at or after knotEndTime. Since knotEndTime is
    // clamped to never exceed _lastTime, there is always a knot at or after
    // knotEndTime. endIt points to the beginning of the segment that should not
    // be sampled (instead of the end of the segment that should be); it is
    // never _times->end().
    auto nextIt = std::upper_bound(_times->begin(), _times->end(),
                                   knotTime);
    auto endIt = std::lower_bound(_times->begin(), _times->end(),
                                  knotEndTime);

    // Convert the iterators to indices so we can find the corresponding
    // item in the _knots vector.
    ptrdiff_t nextIndex = nextIt - _times->begin();
    ptrdiff_t endIndex = endIt - _times->begin();

    // Raw pointers to knotData that corresponds to the _times values that
    // the iterators were referencing.
    nextKnot = _knots->data() + nextIndex;
    prevKnot = nextKnot - 1;
    endKnot = _knots->data() + endIndex;

    for(; prevKnot < endKnot; ++prevKnot, ++nextKnot) {
        GfInterval segmentInterval(prevKnot->time, nextKnot->time);
        segmentInterval &= knotInterval;
        _SampleSegment(prevKnot,
                       nextKnot,
                       segmentInterval,
                       source,
                       knotToSampleTimeScale,
                       knotToSampleTimeOffset,
                       valueOffset,
                       sampledSpline);

    }
}

void
_Sampler::_SampleKnotsReversed(
    const GfInterval& sampleInterval,
    const TsSplineSampleSource source,
    const double knotToSampleTimeScale,
    const TsTime knotToSampleTimeOffset,
    Ts_SampleDataInterface* sampledSpline)
{
    // _SampleKnotsReversed is used only for extrapolation loops that oscillate
    // and only for the iterations that traverse backward through time. The
    // sampleInterval is guaranteed to fit within a single iteration of the
    // loop.
    //
    // Note that we are processing the knots in reverse order, using iterators
    // like rbegin() and rend(). This means that "begin" or "prev" knots will
    // contain larger time values than "end" or "next" knots. Once these time
    // values have been passed throught _ToSampleTime(), the "begin" knot will
    // generate a sample that is to the left of the "end" knot.
    const Ts_DoubleKnotData* prevKnot = nullptr;
    const Ts_DoubleKnotData* nextKnot = nullptr;
    const Ts_DoubleKnotData* beginKnot = nullptr;

    // We are time reversed, so sampleInterval.GetMin() will yield the largest
    // knot time value (the starting point for sampling in reverse) when passed
    // through _ToKnotTime.
    TsTime knotTime = _ToKnotTime(sampleInterval.GetMin(),
                                  knotToSampleTimeScale,
                                  knotToSampleTimeOffset);

    // Knot begin time is the smallest knot time that we're going to
    // sample. This is the ending point for our reversed sampling.
    const TsTime knotBeginTime = _ToKnotTime(sampleInterval.GetMax(),
                                             knotToSampleTimeScale,
                                             knotToSampleTimeOffset);

    // prevIt points to the knot at the (reversed) begining of the segment
    // containing knotTime.
    //
    // beginIt points to the knot at or before knotBeginTime. Since
    // knotBeginTime is clamped to never exceed _firstTime, beginIt points to
    // the beginning of the segment that should not be sampled (instead of the
    // end of the segment that should be); it is never _times->rend().
    auto prevIt = std::upper_bound(_times->rbegin(), _times->rend(),
                                   knotTime, std::greater<TsTime>());
    auto beginIt = std::lower_bound(_times->rbegin(), _times->rend(),
                                    knotBeginTime, std::greater<TsTime>());

    // Convert the iterators to indices so we can find the corresponding
    // item in the _knots vector.
    ptrdiff_t prevRindex = prevIt - _times->rbegin();
    ptrdiff_t beginRindex = beginIt - _times->rbegin();

    // Raw pointers to knotData that corresponds to the _times values that
    // the iterators were referencing.
    nextKnot = _knots->data() + (_knots->size() - 1) - prevRindex;
    prevKnot = nextKnot + 1;
    beginKnot = _knots->data() + (_knots->size() - 1) - beginRindex;

    // Remember we're traversing toward the beginning.
    for(; prevKnot > beginKnot; --prevKnot, --nextKnot) {
        // We need to update the data in the knots.
        Ts_DoubleKnotData prevData = *prevKnot;
        prevData.time = _ToSampleTime(prevData.time,
                                      knotToSampleTimeScale,
                                      knotToSampleTimeOffset);
        prevData.value = prevData.GetPreValue();
        prevData.postTanWidth = prevData.preTanWidth;
        prevData.postTanSlope = -prevData.preTanSlope;

        Ts_DoubleKnotData nextData = *nextKnot;
        nextData.dualValued = false;
        nextData.time = _ToSampleTime(nextData.time,
                                      knotToSampleTimeScale,
                                      knotToSampleTimeOffset);
        nextData.preTanWidth = nextData.postTanWidth;
        nextData.preTanSlope = -nextData.postTanSlope;

        // Change the interpolation of the segment.
        prevData.nextInterp = nextData.nextInterp;

        GfInterval segmentInterval(prevData.time, nextData.time);
        segmentInterval &= sampleInterval;

        // Add the segment. We've already applied the knot time and value
        // scaling so set them to identity.
        _SampleSegment(&prevData,
                       &nextData,
                       segmentInterval,
                       source,
                       1.0,  // knot to sample time scale
                       0.0,  // knot to sample time offset
                       0.0,  // valueOffset
                       sampledSpline);
    }
}

void
_Sampler::_SampleSegment(
    const Ts_DoubleKnotData* prevKnot,
    const Ts_DoubleKnotData* nextKnot,
    const GfInterval& segmentInterval,
    const TsSplineSampleSource source,
    const double knotToSampleTimeScale,
    const double knotToSampleTimeOffset,
    const double valueOffset,
    Ts_SampleDataInterface* sampledSpline)
{
    // Interpolate from prevKnot to nextKnot and store sample segments into
    // sampledSpline

    if (prevKnot->nextInterp == TsInterpValueBlock) {
        // No value, nothing to do.
        return;
    } else if (prevKnot->nextInterp == TsInterpCurve) {
        _SampleCurveSegment(prevKnot,
                            nextKnot,
                            segmentInterval,
                            source,
                            knotToSampleTimeScale,
                            knotToSampleTimeOffset,
                            valueOffset,
                            sampledSpline);
        return;
    }

    // This segment is a single straight line (converted into sample space).
    // Note that _SampleKnotsReversed may have already converted its knots
    // values into sample space (and reset the transform to identity), but
    // _SampleKnots has not. In either case, convert from the times in the
    // knots to actual sample times before seeing if this segment needs to
    // be clipped to the _timeInterval.
    TsTime t1 = _ToSampleTime(prevKnot->time,
                              knotToSampleTimeScale,
                              knotToSampleTimeOffset);
    double v1 = prevKnot->value + valueOffset;
    TsTime t2 = _ToSampleTime(nextKnot->time,
                              knotToSampleTimeScale,
                              knotToSampleTimeOffset);
    double v2 = (prevKnot->nextInterp == TsInterpHeld
                 ? prevKnot->value            // held value
                 : nextKnot->GetPreValue())   // linear value
                + valueOffset;

    // Adjust for sampling just part of the segment.
    TsTime t = _timeInterval.GetMin();
    if (t > t1) {
        double u = (t - t1) / (t2 - t1);
        t1 = t;
        // Only lerp if the value is changing to avoid rounding errors.
        if (v1 != v2) {
            v1 = GfLerp(u, v1, v2);
        }
    }
    t = _timeInterval.GetMax();
    if (t < t2) {
        double u = (t - t1) / (t2 - t1);
        t2 = t;
        // Only lerp if the value is changing to avoid rounding errors.
        if (v1 != v2) {
            v2 = GfLerp(u, v1, v2);
        }
    }

    sampledSpline->AddSegment(t1, v1, t2, v2, source);
}

void
_Sampler::_SampleCurveSegment(
    const Ts_DoubleKnotData* prevKnot,
    const Ts_DoubleKnotData* nextKnot,
    const GfInterval& segmentInterval,
    const TsSplineSampleSource source,
    const double knotToSampleTimeScale,
    const double knotToSampleTimeOffset,
    const double valueOffset,
    Ts_SampleDataInterface* sampledSpline)
{
    // Bezier control points. We sample the Bezier version even for Hermite
    // curves since the math is equivalent.
    GfVec2d cp[4];

    // A switch statement will generate a compile error if we ever add a new
    // curve type without adding a case for it.
    switch (_data->curveType) {
      case TsCurveTypeBezier:
        {
            // Bezier curves may be regressive, prevent that.
            Ts_DoubleKnotData pKnot = *prevKnot;
            Ts_DoubleKnotData nKnot = *nextKnot;
            Ts_RegressionPreventerBatchAccess::ProcessSegment(
                &pKnot, &nKnot, TsAntiRegressionKeepRatio);

            // Get the 4 Bezier control points. Note that the value returned by
            // GetPreTanWidth() is always non-negative, but GetPreTanHeight()
            // has the correct sign.
            cp[0] = GfVec2d(pKnot.time, pKnot.value);
            cp[3] = GfVec2d(nKnot.time, nKnot.GetPreValue());
            cp[1] = cp[0] + GfVec2d(pKnot.GetPostTanWidth(),
                                    pKnot.GetPostTanHeight());
            cp[2] = cp[3] + GfVec2d(-nKnot.GetPreTanWidth(),
                                    nKnot.GetPreTanHeight());
        }
        break;

      case TsCurveTypeHermite:
        {
            // Convert the Hermite curve to Bezier control points. Note, Hermite
            // curves are never regressive so regression prevention is not
            // needed.
            const double dt = nextKnot->time - prevKnot->time;
            const double dt_3 = dt / 3.0;

            cp[0] = GfVec2d(prevKnot->time, prevKnot->value);
            cp[3] = GfVec2d(nextKnot->time, nextKnot->GetPreValue());
            cp[1] = cp[0] + GfVec2d(dt_3, dt_3 * prevKnot->GetPostTanSlope());
            cp[2] = cp[3] - GfVec2d(dt_3, dt_3 * nextKnot->GetPreTanSlope());
        }
        break;
    }

    _SampleBezier(cp, segmentInterval, source,
                  knotToSampleTimeScale, knotToSampleTimeOffset,
                  valueOffset, sampledSpline);
}

void
_Sampler::_SampleBezier(GfVec2d cp[4],
                        const GfInterval& segmentInterval,
                        TsSplineSampleSource source,
                        double knotToSampleTimeScale,
                        double knotToSampleTimeOffset,
                        double valueOffset,
                        Ts_SampleDataInterface* sampledSpline)
{
    // Bezier curves exist entirely within the bounds of their control points
    // so we compute the height of the bounding box. This is the length of the
    // vectors perpendicular to the baseline from cp[0] to cp[3].
    //
    // All height computations are done in "tolerance space", scaled by
    // _timeScale and _valueScale so we can just compare the length of the
    // perpendicular vectors to _tolerance (really compare length squared to
    // _tolerance squared). If greater than _tolerance then we split the Bezier
    // into 2 halves and recurse on each one.
    GfVec2d scaleVec(_timeScale, _valueScale);
    GfVec2d baseVec = GfCompMult(scaleVec, cp[3] - cp[0]);
    GfVec2d vec1 = GfCompMult(scaleVec, cp[1] - cp[0]);
    GfVec2d vec2 = GfCompMult(scaleVec, cp[2] - cp[0]);

    // baseVec is the vector from cp[0] to cp[3]. Compute the perpendicular
    // distance from that base line to each of cp[1] and cp[2]. The values
    // t1 * baseVec and t2 * baseVec are the projections of vec1 and vec2 onto
    // baseVec. So (vec1 - t1 * baseVec) is the perpendicular component of vec1.
    double lenSquared = baseVec.GetLengthSq();
    double t1 = vec1 * baseVec / lenSquared;
    double t2 = vec2 * baseVec / lenSquared;

    double h1Squared = (vec1 - t1 * baseVec).GetLengthSq();
    double h2Squared = (vec2 - t2 * baseVec).GetLengthSq();

    // If the length of both perpendiculars are <= _tolerance, we're done, baseVec
    // is our linear approximation of this part of the curve.
    if (std::max(h1Squared, h2Squared) <= _tolerance * _tolerance) {
        double t1 = cp[0][0];
        double t2 = cp[3][0];
        double v1 = cp[0][1];
        double v2 = cp[3][1];

        if (t1 < segmentInterval.GetMin()) {
            double u = (segmentInterval.GetMin() - t1) / (t2 - t1);
            t1 = GfLerp(u, t1, t2);
            v1 = GfLerp(u, v1, v2);
        }
        if (t2 > segmentInterval.GetMax()) {
            double u = (segmentInterval.GetMax() - t1) / (t2 - t1);
            t2 = GfLerp(u, t1, t2);
            v2 = GfLerp(u, v1, v2);
        }

        sampledSpline->AddSegment(_ToSampleTime(t1,
                                                knotToSampleTimeScale,
                                                knotToSampleTimeOffset),
                                  v1 + valueOffset,
                                  _ToSampleTime(t2,
                                                knotToSampleTimeScale,
                                                knotToSampleTimeOffset),
                                  v2 + valueOffset,
                                  source);
        return;
    } else {
        // The height of the control point bounding box is greater than
        // _tolerance, so split the curve and recurse on the halves.
        GfVec2d leftCp[4], rightCp[4];
        _SubdivideBezier(cp, 0.5, leftCp, rightCp);
        bool doLeft = segmentInterval.Intersects(GfInterval(leftCp[0][0],
                                                            leftCp[3][0]));
        bool doRight = segmentInterval.Intersects(GfInterval(rightCp[0][0],
                                                             rightCp[3][0]));
        if (knotToSampleTimeScale < 0) {
            // time scale is negative (due to oscillating loops) so sample the
            // right hand side first as it will get scaled to the left.
            if (doRight) {
                _SampleBezier(rightCp,
                              segmentInterval,
                              source,
                              knotToSampleTimeScale,
                              knotToSampleTimeOffset,
                              valueOffset,
                              sampledSpline);
            }
            if (doLeft) {
                _SampleBezier(leftCp,
                              segmentInterval,
                              source,
                              knotToSampleTimeScale,
                              knotToSampleTimeOffset,
                              valueOffset,
                              sampledSpline);
            }
        } else {
            if (doLeft) {
                _SampleBezier(leftCp,
                              segmentInterval,
                              source,
                              knotToSampleTimeScale,
                              knotToSampleTimeOffset,
                              valueOffset,
                              sampledSpline);
            }
            if (doRight) {
                _SampleBezier(rightCp,
                              segmentInterval,
                              source,
                              knotToSampleTimeScale,
                              knotToSampleTimeOffset,
                              valueOffset,
                              sampledSpline);
            }
        }
    }
}

void
_Sampler::_SubdivideBezier(const GfVec2d cp[4],
                           const double u,
                           GfVec2d leftCp[4],
                           GfVec2d rightCp[4])
{
    // Intermediate points
    GfVec2d cp01   = GfLerp(u, cp[0], cp[1]);
    GfVec2d cp12   = GfLerp(u, cp[1], cp[2]);
    GfVec2d cp23   = GfLerp(u, cp[2], cp[3]);

    GfVec2d cp012  = GfLerp(u, cp01, cp12);
    GfVec2d cp123  = GfLerp(u, cp12, cp23);

    GfVec2d cp0123 = GfLerp(u, cp012, cp123);

    // Left Bezier
    leftCp[0] = cp[0];
    leftCp[1] = cp01;
    leftCp[2] = cp012;
    leftCp[3] = cp0123;

    // Right Bezier
    rightCp[0] = cp0123;
    rightCp[1] = cp123;
    rightCp[2] = cp23;
    rightCp[3] = cp[3];
}

// Unroll inner loops and convert the relevant knot data to Ts_DoubleKnotData.
// Intermediate computations are all done double precision to match eval and to
// avoid precision problems. Since we're going to call GetKnotDataAsDouble
// eventually, do it up front.
void
_Sampler::_UnrollInnerLoops()
{
    // We're going to have to convert the knots and times info. Point _knots and
    // _times at the internal arrays that we're about to populate.
    _knots = &_internalKnots;
    _times = &_internalTimes;

    GfInterval unrollInterval = _timeInterval;

    // Inner loops are defined over a closed interval. The end of the looped
    // interval has a knot that is a copy of the knot at the start of the
    // interval. It will overrule any knot that may be in the spline data at
    // that time. So any regular knots that come after the inner loops start
    // with an open interval, (_lastInnerLoop .. _lastTime].
    //
    // Also, because of the point above, there is a "fencepost" issue to keep
    // in mind where there is one more copy of the first knot than there are
    // loops because there is a copy at both the beginning and the end of the
    // looped range.
    GfInterval loopedInterval;  // empty interval;
    if (_haveInnerLoops) {
        loopedInterval = GfInterval(_firstInnerLoop, _lastInnerLoop);
        if (_havePreExtrapLoops || _havePostExtrapLoops) {
            // If we are using a looping extrapolation mode then we need to be
            // more careful about the region of knots that we unroll. If the
            // requested time samples extend beyond the last knot then sampling
            // will wrap around to the beginning again. So limit loopedInterval
            // only if _timeInterval is entirely within it. If _timeInterval is
            // at all outside loopInterval we want to unroll the entire
            // loopInterval.
            if (loopedInterval.Contains(_timeInterval)) {
                loopedInterval = _timeInterval;
            }
        } else {
            // No extrapolation looping so we only need to unroll inner loop
            // knots that affect _timeInterval
            loopedInterval &= _timeInterval;
        }
    } else {
        // No inner loops, but there may be extrapolation loops.
        if (_havePreExtrapLoops || _havePostExtrapLoops) {
            GfInterval knotsInterval(_firstTime, _lastTime);
            if (!knotsInterval.Contains(unrollInterval)) {
                // unrollInterval extends outside the knotsInterval, at least
                // partly into the extrapolation loops. Unroll all the knots to
                // ensure we have the knots we need.
                unrollInterval = knotsInterval;
            }
        }
    }

    // Iterators for the range that is pre-looping, looping prototype, and
    // post-looping.
    std::vector<TsTime>::const_iterator preBegin, preEnd;      // before looping
    std::vector<TsTime>::const_iterator protoBegin, protoEnd;  // looping prototype
    std::vector<TsTime>::const_iterator postBegin, postEnd;    // after looping

    // Save some typing and wrapping of long lines.
    std::vector<TsTime>::const_iterator timesBegin = _data->times.begin();
    std::vector<TsTime>::const_iterator timesEnd = _data->times.end();

    preBegin = std::lower_bound(timesBegin, timesEnd, unrollInterval.GetMin());
    if ((preBegin == timesEnd || *preBegin > unrollInterval.GetMin()) &&
        preBegin != timesBegin)
    {
        --preBegin;
    }

    // Find the knot at or after the end of unrollInterval. This is the knot on
    // the far end of the last segment we are sampling (or is timesEnd if we're
    // sampling past the last knot).
    postEnd = std::lower_bound(preBegin, timesEnd, unrollInterval.GetMax());
    if (postEnd != timesEnd) {
        // Increment the iterator so copying [preBegin .. postEnd) will copy the
        // last knot.
        ++postEnd;
    }

    if (loopedInterval.IsEmpty()) {
        // Even if there are inner loops, we're not interested in
        // that portion of the spline. Copy what we need
        ptrdiff_t offset = std::distance(timesBegin, preBegin);
        ptrdiff_t count = std::distance(preBegin, postEnd);
        _unrolledKnots.reserve(count);

        for (ptrdiff_t i = 0; i < count; ++i) {
            _unrolledKnots.emplace_back(i + offset, 0.0, 0.0);
        }

        return;
    }

    preEnd = std::lower_bound(preBegin, timesEnd, _firstInnerLoop);

    protoBegin = std::lower_bound(preEnd, timesEnd, _firstInnerProto);
    protoEnd = std::lower_bound(protoBegin, timesEnd, _lastInnerProto);

    // There will be copy of the first prototype region knot at _lastInnerLoop.
    // Use upper_bound because the post looping data starts after the copy.
    postBegin = std::upper_bound(protoEnd, timesEnd, _lastInnerLoop);

    // Note that Ts_SplineData::HasInnerLoops has already validated the
    // loopParams struct so we know we have a positive size for protoSpan
    // and at least 1 loop of the spanned range.
    const TsLoopParams &lp = _data->loopParams;
    const TsTime protoSpan = lp.protoEnd - lp.protoStart;

    // Figure out the number of pre- and post-loops that we need. This may be
    // less than the number of pre- and post-loops that exist because
    // loopedInterval is only the looped portion of the spline that we want to
    // sample.
    const TsTime preOffset = _firstInnerProto - loopedInterval.GetMin();
    const int preLoops = std::max(0, int(std::ceil(preOffset / protoSpan)));

    const TsTime postOffset = loopedInterval.GetMax() - _lastInnerProto;
    const int postLoops = std::max(0, int(std::ceil(postOffset / protoSpan)));

    // Count the knots to minimize memory allocations
    ptrdiff_t count = (preEnd - preBegin) +
                      (protoEnd - protoBegin) * (preLoops + 1 + postLoops) + 1 +
                      (postEnd - postBegin);

    _unrolledKnots.reserve(count);

    // Convert iterators to indices so they work for both knots and times.
    ptrdiff_t preBeginIndex   = preBegin   - timesBegin;
    ptrdiff_t preEndIndex     = preEnd     - timesBegin;

    ptrdiff_t protoBeginIndex = protoBegin - timesBegin;
    ptrdiff_t protoEndIndex   = protoEnd   - timesBegin;

    ptrdiff_t postBeginIndex  = postBegin  - timesBegin;
    ptrdiff_t postEndIndex    = postEnd    - timesBegin;

    // Populate the arrays. Just copy values from before looping starts.
    for (ptrdiff_t i = preBeginIndex; i < preEndIndex; ++i) {
        _unrolledKnots.emplace_back(i, 0.0, 0.0);
    }

    // Copy data for the loops, offsetting the times and values.
    for (int loopIndex = -preLoops; loopIndex <= postLoops; ++loopIndex) {
        TsTime timeOffset = protoSpan * loopIndex;
        double valueOffset = lp.valueOffset * loopIndex;
        for (ptrdiff_t i = protoBeginIndex; i < protoEndIndex; ++i) {
            _unrolledKnots.emplace_back(i, timeOffset, valueOffset);
        }
    }

    // One last copy of the first prototype knot.
    _unrolledKnots.emplace_back(_firstInnerProtoIndex,
                                protoSpan * (postLoops + 1),
                                lp.valueOffset * (postLoops + 1));

    // Copy knots that are after looping ends.
    for (ptrdiff_t i = postBeginIndex; i < postEndIndex; ++i) {
        _unrolledKnots.emplace_back(i, 0.0, 0.0);
    }
}

void
_Sampler::_ConvertUnrolledKnotsForSampling()
{
    TF_VERIFY(std::is_sorted(_unrolledKnots.begin(),
                             _unrolledKnots.end(),
                             _UnrolledKnotTimeLess(_data)),
              "_unrolledKnots is not sorted!");

    _internalTimes.reserve(_unrolledKnots.size());
    _internalKnots.reserve(_unrolledKnots.size());

    for (const auto& uKnot : _unrolledKnots) {
        _internalKnots.emplace_back(
            _data->GetKnotDataAsDouble(uKnot.knotIndex));
        Ts_DoubleKnotData& last = _internalKnots.back();
        last.time += uKnot.timeOffset;
        last.value += uKnot.valueOffset;
        last.preValue += uKnot.valueOffset;

        _internalTimes.push_back(last.time);
    }
}

// Figure out knot indices outside the [0..n] range of _unrolledKnots and
// iterate through them to assemble the output knots in bakedData.
template <typename T>
void
_Sampler::_BakeTypedKnots(const Ts_TypedSplineData<T>* typedData,
                          const GfInterval& interval,
                          const TsExtrapolation* preExtrapPtr,
                          const TsExtrapolation* postExtrapPtr,
                          Ts_TypedSplineData<T>* bakedData)
{
    // Iteration counts and loop times. Given a spline whose sequence of knots
    // span times from t0 to t1, we can map any time t to an iteration and a
    // time within that iteration. Define tDelta as t1 - t0 and we can define
    // each iteration as spanning the range from (t0 + k * tDelta) to
    // (t1 + k * tDelta) where k is the iteration number. When k = 0, this is
    // just the normal knot span. For pre-extrapolation loops, k < 0 and for
    // post-extrapolation loops, k > 0.
    //
    // Within each loop, loopTime = time - (t0 + k * tDelta), yielding a
    // looptime value in the range [t0 .. t1].
    //
    // This is further complicated because each iteration meets its neighbors at
    // a knot. That is (t1 * k * tDelta) == (t0 + (k+1) * tDelta). To keep the
    // shape of the curve fixed, the boundary knots are formed by combining the
    // two overlapping knots. The left side of the boundary is part of iteration
    // k and gets its values from the last knot in the spline.  The right side
    // of the boundary is part of iteration k+1 and gets its values from the
    // first knot in the spline.
    
    // The amount by which time changes every loop.
    const double timeDelta = _lastTime - _firstTime;

    // Get the number of knots in each loop. This is one less than the number of
    // knots in the original spline.
    // 
    // Consider a spline with 3 knots that we'll label A, B, and C. This spline
    // has two segments A-B and B-C. When looping we want to repeat these two
    // segments, as in A-B, B-C, A-B, B-C, etc. Segment B-C must be followed by
    // A-B. So the knot between the two segments must look like C from the left
    // and A from the right, call it C|A. So the full looped sequence becomes
    // C|A-B, B-C|A, C|A-B, B-C|A, etc. So we have a loop size of 2 knots and we
    // have to construct a hybrid knot C|A that looks like C from the left and A
    // from the right.
    const ptrdiff_t loopSize = _unrolledKnots.size() - 1;

    // We need to figure out the iteration for the min time so we can
    // find the right knot to start with which may update the iteration.
    ptrdiff_t iteration = ptrdiff_t(
        std::floor((interval.GetMin() - _firstTime) / timeDelta));
    
    const TsExtrapolation* extrap = (iteration <= 0
                                     ? preExtrapPtr
                                     : postExtrapPtr);

    if (!extrap || !extrap->IsLooping()) {
        // We're not looping. Reset iteration
        iteration = 0;
    }

    double loopTime = interval.GetMin() - (iteration * timeDelta);

    auto beginIt = std::lower_bound(_unrolledKnots.begin(),
                                    _unrolledKnots.end(),
                                    loopTime,
                                    _UnrolledKnotTimeLess(typedData));
    
    // If beginIt is past the end or is between knots, back up to the knot at
    // the beginning of the segment.
    if (beginIt == _unrolledKnots.end() ||
        (beginIt != _unrolledKnots.begin() &&
         beginIt->GetTime(_data) > loopTime))
    {
        --beginIt;
    }

    // When looping, knotIndex is often outside the range of _unrolledKnots
    // indices. If _unrolledKnots had 3 knots, their indices would be 0, 1, and
    // 2. Pre-extrapolation looped knots would have negative indices and
    // post-extrapolation knots would have indices greater than 2.
    //
    // Note, however, that we treat index 0 as if it were a pre-extrapolation
    // knot and 2 as if it were a post-extrapolation knot if looping being used.
    // This is because those knots are the beginning or end of loops and need
    // to be generated by combining the data from 2 knots.
    ptrdiff_t knotIndex = std::distance(_unrolledKnots.begin(), beginIt)
                          + iteration * loopSize;

    Ts_TypedKnotData<T> knot;
    do
    {
        // Are we looping, oscillating, or repeating?
        const bool looping = extrap &&
                             extrap->IsLooping();
        const bool oscillating = extrap &&
                                 (extrap->mode == TsExtrapLoopOscillate);
        const bool repeating = extrap &&
                               (extrap->mode == TsExtrapLoopRepeat);

        // Do we go forward in time every other loop, or every loop?
        const int forwardFrequency = (oscillating ? 2 : 1);

        // The amount by which the value changes every loop. It only changes if
        // the mode is TsExtrapLoopRepeat. "Reset" and "oscillate" modes have no
        // value change between iterations.
        const double valueDelta = (repeating
                                   ? (_unrolledKnots.back().GetValue(typedData) -
                                      _unrolledKnots.front().GetValue(typedData))
                                   : 0.0);
    
        iteration = (looping
                     ? ptrdiff_t(std::floor(knotIndex / double(loopSize)))
                     : 0);

        ptrdiff_t loopIndex = knotIndex;
        if (looping) {
            // knotIndex % loopSize will return negative numbers if knotIndex is
            // negative, so compute it from the knotIndex and iteration instead
            // to ensure it is in the range [0..loopSize].
            loopIndex = knotIndex - iteration * loopSize;
        }

        const bool reversed = ((iteration % forwardFrequency) != 0);

        if (reversed) {
            // Handle reverse oscillation. For this iteration we're iterating
            // from the last knot toward 0.
            const ptrdiff_t revLoopIndex = loopSize - loopIndex;
            const _UnrolledKnot& uKnot = _unrolledKnots[revLoopIndex];
            knot = uKnot.GetKnotData(typedData);

            // Adjust the time
            knot.time = _firstTime + _lastTime - uKnot.GetTime(typedData)
                        + timeDelta * iteration;

            // Since we're reversed, we're clearly oscillating and valueDelta
            // must be 0.0, so don't bother adding it in.
            knot.value = uKnot.GetValue(typedData);

            // In order to preserve the shape of the spline we have to:
            //   * swap the value and preValue.
            //   * swap the preTanWidth and postTanWidth
            //   * swap the preTanSlope and postTanSlope and negate them
            //   * handle held interpolation by copying the value from the other
            //     end of the segment.
            //   * move the interpolation flag from the other end of the segment

            using std::swap;   // enable ADL for swap
            if (knot.dualValued) {
                knot.preValue = uKnot.GetPreValue(typedData);
                swap(knot.preValue, knot.value);
            } else {
                knot.preValue = knot.value;
            }

            swap(knot.preTanWidth, knot.postTanWidth);
            swap(knot.preTanSlope, knot.postTanSlope);
            knot.preTanSlope *= -1;
            knot.postTanSlope *= -1;

            // Get the knot at the other end of the segment
            const _UnrolledKnot& otherUKnot = _unrolledKnots[revLoopIndex - 1];
            const Ts_TypedKnotData<T>& otherKnot =
                otherUKnot.GetKnotData(typedData);

            if (otherKnot.nextInterp == TsInterpHeld) {
                // The held interpolation on otherKnot created a discontinuity
                // at knot. Make it dualValued and copy in the correct value to
                // preserve the held segment unchanged.
                knot.dualValued = true;
                knot.value = otherUKnot.GetValue(typedData);
            }

            // Make sure the segment interpolates the same way.
            knot.nextInterp = otherKnot.nextInterp;

            // If this is the first knot in this iteration we need to hybridize
            // it with the last knot in the previous iteration.
            if (loopIndex == 0) {
                // This knot is the point where the oscillation reverses
                // direction. The spline arrives at the left side of this knot
                // (in normal time direction) and departs from a copy of pre
                // side moving reversed in time.

                // There are never discontinuities at the reflection point
                knot.dualValued = false;

                // The pre-tangent is an inverted version of the post-tangent
                knot.preTanSlope = -knot.postTanSlope;
                knot.preTanWidth = knot.postTanWidth;

                // Reset the tangent algorithms since baking out the loops
                // would change algorithm results
                knot.preTanAlgorithm = TsTangentAlgorithmNone;
                knot.postTanAlgorithm = TsTangentAlgorithmNone;
            }
        } else {
            // Not reversed
            const _UnrolledKnot& uKnot = _unrolledKnots[loopIndex];
            knot = uKnot.GetKnotData(typedData);

            // Adjust the time
            knot.time = uKnot.GetTime(typedData) + timeDelta * iteration;
            knot.value = uKnot.GetValue(typedData) + valueDelta * iteration;

            if (looping && loopIndex == 0) {
                // Hybridize the knot. This iteration is moving forward in time
                // which makes dealing with interpolation and dual valued knots
                // much easier, but the previous iteration may be time reversed.

                if (oscillating) {
                    // We're oscillating. Hybridize the knot with itself. So it
                    // is never dual valued and we negate the incoming tangent
                    // slope.
                    knot.dualValued = false;
                    knot.preTanSlope *= -knot.postTanSlope;
                    knot.preTanWidth = knot.postTanWidth;
                } else {
                    // We're looping with repeat or reset. Dual valued knots are
                    // preserved (from the pre side) and time proceeds into the
                    // future.

                    // The the knot data for the pre side.
                    const _UnrolledKnot& preUKnot = _unrolledKnots[loopSize];
                    const Ts_TypedKnotData<T>& preKnot =
                        preUKnot.GetKnotData(typedData);

                    knot.preValue = preUKnot.GetPreValue(typedData)
                                    + valueDelta * (iteration - 1);
                    knot.dualValued = preKnot.dualValued
                                      || extrap->mode == TsExtrapLoopReset;
                                  
                    knot.preTanWidth = preKnot.preTanWidth;
                    knot.preTanSlope = preKnot.preTanSlope;
                }

                knot.preTanAlgorithm = TsTangentAlgorithmNone;
                knot.postTanAlgorithm = TsTangentAlgorithmNone;
            }
        }

        bakedData->PushKnot(&knot, VtDictionary());

        ++knotIndex;

        // Make sure we're pointing at the right extrapolation parameters
        if (knotIndex >= loopSize) {
            extrap = postExtrapPtr;
        } else if (knotIndex <= 0) {
            extrap = preExtrapPtr;
        } else {
            extrap = nullptr;
        }

        // If we're not looping and we've exhausted the regular knots, stop.
        if (!looping && knotIndex > loopSize) {
            break;
        }

        // Otherwise, continue until we've stored a knot at a time >= the
        // max time of the interval.
    } while (knot.time < interval.GetMax());
}

bool
_Sampler::Bake(Ts_SplineData* bakedData)
{
    // This should have already been handled.
    if (_unrolledKnots.empty()) {
        TF_CODING_ERROR("_Sampler::Bake invoked with an empty spline."
                        " Please report a bug.");
        return false;
    }

    // If we're including extrapolation loops, bake the caller provided
    // time interval. But if we're only baking inner loops, then bake
    // the whole inner loop time interval.
    const GfInterval bakeInterval = _includeExtrapLoops
                                    ? _timeInterval
                                    : GfInterval(_firstTime, _lastTime);

    const TsExtrapolation* preExtrapPtr =
        (bakeInterval.GetMin() <= _firstTime
         ? &_data->preExtrapolation
         : nullptr);
    const TsExtrapolation* postExtrapPtr =
        (bakeInterval.GetMax() >= _lastTime
         ? &_data->postExtrapolation
         : nullptr);

#define _MAKE_CLAUSE(unused, tuple)                                     \
    if (_data->GetValueType() == Ts_GetType<TS_SPLINE_VALUE_CPP_TYPE(tuple)>()) \
    {                                                                   \
        using T = TS_SPLINE_VALUE_CPP_TYPE(tuple);                      \
        const Ts_TypedSplineData<T>* typedData =                        \
            dynamic_cast<const Ts_TypedSplineData<T>*>(_data);          \
        Ts_TypedSplineData<T>* typedBakedData =                         \
            dynamic_cast<Ts_TypedSplineData<T>*>(bakedData);            \
                                                                        \
        _BakeTypedKnots(typedData, bakeInterval,                        \
                        preExtrapPtr, postExtrapPtr,                    \
                        typedBakedData);                                \
        return true;                                                    \
    }

    TF_PP_SEQ_FOR_EACH(_MAKE_CLAUSE, ~, TS_SPLINE_SUPPORTED_VALUE_TYPES);

    TF_CODING_ERROR("Unsupported spline value type in _Sampler::Bake: %s",
                    _data->GetValueType().GetTypeName().c_str());
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////
// SAMPLE ENTRY POINT

void
Ts_Sample(
    const Ts_SplineData* const data,
    const GfInterval& timeInterval,
    const double timeScale,
    const double valueScale,
    const double tolerance,
    Ts_SampleDataInterface* sampledSpline)
{
    // All arguments should have been validated before reaching this point,
    // but just to be safe...
    if (!TF_VERIFY((data &&
                    !timeInterval.IsEmpty() &&
                    timeScale > 0.0 &&
                    valueScale > 0.0 &&
                    tolerance > 0.0 &&
                    sampledSpline),
                   "Invalid argument to Ts_Sample."))
    {
        return;
    }

    if (data->times.empty()) {
        return;
    }

    // Construct a _Sampler to sort out looping and extrapolation.
    _Sampler sampler(data,
                     timeInterval,
                     timeScale,
                     valueScale,
                     tolerance);

    // Perform the main evaluation.
    sampler.Sample(sampledSpline);
}

////////////////////////////////////////////////////////////////////////////////
// BAKE ENTRY POINT

Ts_SplineData* Ts_Bake(
    const Ts_SplineData* const data,
    const GfInterval& timeInterval,
    const bool includeExtrapLoops)
{
    Ts_SplineData* bakedData = nullptr;
    
    // Check some special cases
    if (!data) {
        // We're done.
        return nullptr;
    }

    // Only a finite number of knots allowed.
    if (includeExtrapLoops &&
        (data->times.size() > 1 || data->HasInnerLoops()) &&
        ((data->preExtrapolation.IsLooping() && !timeInterval.IsMinFinite()) ||
         (data->postExtrapolation.IsLooping() && !timeInterval.IsMaxFinite())))
    {
        TF_CODING_ERROR("Attempt to bake an infinite number of knots.");
        return nullptr;
    }

    // Populate baked data with an empty spline data of the right type. Copy the
    // looping and other parameters from data.
    bakedData = Ts_SplineData::Create(data->GetValueType(), data);

    if (data->times.empty()) {
        // Nothing to bake, just reset the inner loop params. Baked data no
        // longer has inner loops.
        bakedData->loopParams = TsLoopParams();
        return bakedData;
    }

    // Construct a sampler for baking.
    _Sampler sampler(data, timeInterval, includeExtrapLoops, bakedData);

    // Bake it
    sampler.Bake(bakedData);

    // reset the inner loop params. Baked data no longer has inner loops.
    bakedData->loopParams = TsLoopParams();

    return bakedData;
}


PXR_NAMESPACE_CLOSE_SCOPE
