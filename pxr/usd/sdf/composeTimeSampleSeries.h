//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_COMPOSE_TIME_SAMPLE_SERIES_H
#define PXR_USD_SDF_COMPOSE_TIME_SAMPLE_SERIES_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/base/gf/math.h"

PXR_NAMESPACE_OPEN_SCOPE

// The default `timesEqual` function for SdfComposeTimeSampleSeries.
inline auto Sdf_timesEqualDefaultFn = [](double t1, double t2) {
    return GfIsClose(t1, t2, 1e-6);
};

///
/// A helper function for composing a stronger time-sample series (times &
/// values) over a weaker one.  This is mostly used as an implementation detail.
///
/// Compose the `[strongBegin, strongEnd)` series over `[weakBegin, weakEnd)`.
/// The elements of each series must have an associated Time (double-valued) and
/// Value (arbitrary), fetched by the `getTime(Iter)` and `getValue(Iter)`
/// functions respectively.  Iter must be a bidirectional iterator.
///
/// The `composeFn(strong, weak)` must accept two Values (as obtained by
/// getValue()) and return a `std::optional<Value>`.  If the `optional` has a
/// value it is used as the result of composing the values, otherwise the
/// `strong` value is used.
///
/// The `outputFn(Value, Time)` is called to emit the results of the
/// composition.  This function is always called with strictly increasing Times.
///
/// The `timesEqual(Time, Time)` function can optionally be supplied to check
/// Time equivalence with a customized epsilon.  The default calls GfIsClose
/// with an epsilon of 1e-6.
/// 
template <class Iter,
          class GetTimeFn, class GetValueFn,
          class ComposeFn, class OutputFn,
          class TimesEqualFn = decltype(Sdf_timesEqualDefaultFn)>
void
SdfComposeTimeSampleSeries(
    Iter strongBegin, Iter strongEnd,
    Iter weakBegin, Iter weakEnd,
    GetTimeFn const &getTime,
    GetValueFn const &getValue,
    ComposeFn const &composeFn,
    OutputFn const &outputFn,
    TimesEqualFn const &timesEqual = Sdf_timesEqualDefaultFn)
{
    // If either series is empty, just copy the other.
    if (weakBegin == weakEnd) {
        while (strongBegin != strongEnd) {
            outputFn(getValue(strongBegin), getTime(strongBegin));
            ++strongBegin;
        }
        return;
    }
    if (strongBegin == strongEnd) {
        while (weakBegin != weakEnd) {
            outputFn(getValue(weakBegin), getTime(weakBegin));
            ++weakBegin;
        }
        return;
    }

    auto held = [&getTime, &timesEqual](Iter iter, Iter begin, Iter end,
                                        double time) {
        return iter == end || (!timesEqual(
                                   getTime(iter), time) && iter != begin)
            ? std::prev(iter)
            : iter;
    };

    constexpr double inf = std::numeric_limits<double>::infinity();
    
    Iter strongIter = strongBegin;
    Iter weakIter = weakBegin;
    
    while (strongIter != strongEnd || weakIter != weakEnd) {
        const double strongTime =
            strongIter == strongEnd ? inf : getTime(strongIter);
        const double weakTime =
            weakIter == weakEnd ? inf : getTime(weakIter);
        if (strongTime <= weakTime) {
            if (auto composed = composeFn(
                    getValue(strongIter),
                    getValue(held(weakIter, weakBegin, weakEnd,
                                  strongTime)))) {
                outputFn(std::move(*composed), strongTime);
            }
            else {
                outputFn(getValue(strongIter), strongTime);
            }
        }
        else {
            if (auto composed = composeFn(
                    getValue(held(strongIter, strongBegin, strongEnd,
                                  weakTime)),
                    getValue(weakIter))) {
                outputFn(std::move(*composed), weakTime);
            }
            else {
                // Do nothing -- a non-composing stronger sample hides weaker
                // samples.
            }
        }

        // Advance the iterator whose next time is less, or the one that has
        // samples remaining.  If they are both at the same time advance both.
        if (strongIter == strongEnd) {
            ++weakIter;
        }
        else if (weakIter == weakEnd) {
            ++strongIter;
        }
        else {
            if (timesEqual(strongTime, weakTime)) {
                ++strongIter, ++weakIter;
            }
            else if (strongTime < weakTime) {
                ++strongIter;
            }
            else {
                ++weakIter;
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_COMPOSE_TIME_SAMPLE_SERIES_H
