//
// Copyright 2023 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#ifndef PXR_BASE_TS_EVALUATOR_H
#define PXR_BASE_TS_EVALUATOR_H

#include "pxr/pxr.h"
#include "pxr/base/ts/evalCache.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/ts/types.h"

#include "pxr/base/trace/trace.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TsEvaluator
/// \brief Opaque interface to a spline for evaluations using cached segments.
///
/// Use this evaluator when performing many evaluations on an unchanging
/// TsSpline whose knots support tangents (e.g., Bezier splines). Evals on
/// this class are required to be thread-safe.
///
template <typename T>
class TsEvaluator {

public:

    /// Default constructor; falls back to empty spline.
    TsEvaluator();

    /// Constructs the evaluator and its caches for the given spline.
    TsEvaluator(TsSpline spline);

    /// Evaluates the spline at the given time. Note that left side evals do not
    /// benefit from the cached segments.
    T Eval(const TsTime &time, TsSide side=TsRight) const;

private:

    // Vector of typed Ts_EvalCaches, one for each Bezier segment in the
    // spline.
    std::vector<std::shared_ptr<Ts_EvalCache<T> > > _segments;

    // The spline being evaluated.
    TsSpline _spline;
};

template <typename T>
TsEvaluator<T>::TsEvaluator()
{
}

template <typename T>
TsEvaluator<T>::TsEvaluator(TsSpline spline) :
_spline(spline)
{
    TRACE_FUNCTION();

    if (spline.size() > 1) {

        // Only set up eval caches when there are Bezier segments.
        bool bezier = false;
        for (const TsKeyFrame &kf : spline) {
            if (kf.GetKnotType() == TsKnotBezier) {
                bezier = true;
                break;
            }
        }
        if (!bezier) {
            return;
        }

        _segments.reserve(spline.size() - 1);

        TF_FOR_ALL(splItr, spline) {

            // Create and store an eval cache for each segment (defined by a
            // pair of adjacent keyframes) of the spline.

            TsSpline::const_iterator iAfterTime = splItr;
            iAfterTime++;

            if (iAfterTime == spline.end()) {
                break;
            }

            std::shared_ptr<Ts_EvalCache<T> > segmentCache =
                Ts_EvalCache<T>::New(*splItr, *iAfterTime);

            if (TF_VERIFY(segmentCache)) {
                _segments.push_back(segmentCache);
            }
        }

    }
}

template <typename T>
T
TsEvaluator<T>::Eval(const TsTime &time,
                                  TsSide side) const
{

    // Only right-side evals can benefit from cached segments.
    if (!_segments.empty() && side == TsRight) {

        // Only use eval caches for times that are between the authored knots on
        // the spline. Boundary extrapolation cases are evaluated directly.
        if (time >= _spline.begin()->GetTime() &&
            time <= _spline.rbegin()->GetTime()) {

            // Get the closest keyframe <= the requested time.
            TsSpline::const_iterator sample = _spline.lower_bound(time);
            if (TF_VERIFY(sample != _spline.end())) {

                // We will index into the _segments vector using the iterator
                // offset of the given sample. We need another decrement if our
                // sample is > than the requested time (we want the requested
                // time to be in between the two keyframes contained in the eval
                // cache entry.
                size_t idx = sample - _spline.begin();
                if (sample->GetTime() > time && TF_VERIFY(idx > 0)) {
                    idx--;
                }
                
                if (TF_VERIFY(idx < _segments.size())
                    && TF_VERIFY(_segments[idx])) {
                    return _segments[idx]->TypedEval(time);
                }
            }
        }
    }

    // If we did not get a cache hit, evaluate directly on the spline.
    if (!_spline.empty()) {
        return _spline.Eval(time).template Get<T>();
    }

    // If we're evaluating an empty spline, fall back to zero.
    return TsTraits<T>::zero;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
