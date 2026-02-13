//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/diff.h"
#include "pxr/base/ts/iterator.h"
#include "pxr/base/tf/diagnostic.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

namespace {  // anonymous namespace

constexpr double inf = std::numeric_limits<double>::infinity();

// Constants for constructing GfInterval objects
constexpr bool OPEN = false;
constexpr bool CLOSED = true;

static
GfInterval _CompareSegments(const Ts_SplineData* data1,
                            const Ts_SplineData* data2,
                            const GfInterval& iterInterval)
{
    Ts_SegmentIterator iter1(data1, iterInterval);
    Ts_SegmentIterator iter2(data2, iterInterval);

    double minDiffTime = +inf;
    double maxDiffTime = -inf;

    while (!iter1.AtEnd() && !iter2.AtEnd()) {
        const Ts_Segment seg1 = *iter1;
        const Ts_Segment seg2 = *iter2;

        if (seg1 == seg2) {
            ++iter1;
            ++iter2;
            continue;
        }

        minDiffTime = std::min(minDiffTime, std::min(seg1.p0[0], seg2.p0[0]));
        maxDiffTime = std::max(maxDiffTime, std::max(seg1.p1[0], seg2.p1[0]));

        // Advance the segment that ends the earliest. Advance both if they end
        // at the same time.
        if (seg1.p1[0] <= seg2.p1[0]) {
            ++iter1;
        }

        if (seg1.p1[0] >= seg2.p1[0]) {
            ++iter2;
        }
    }

    while (!iter1.AtEnd()) {
        const Ts_Segment seg1 = *iter1;
        minDiffTime = std::min(minDiffTime, seg1.p0[0]);
        maxDiffTime = std::max(maxDiffTime, seg1.p1[0]);
        ++iter1;
    }

    while (!iter2.AtEnd()) {
        const Ts_Segment seg2 = *iter2;
        minDiffTime = std::min(minDiffTime, seg2.p0[0]);
        maxDiffTime = std::max(maxDiffTime, seg2.p1[0]);
        ++iter2;
    }
    
    return GfInterval(minDiffTime, maxDiffTime, CLOSED, OPEN);
}

}  // anonymous namespace

////////////////////////////////////////////////////////////////////////////////
// Comparing

GfInterval
Ts_Diff(const Ts_SplineData* data1,
        const Ts_SplineData* data2,
        const GfInterval& compareInterval)
{
    // Assume they're going to be completely different
    GfInterval result = compareInterval;

    if (compareInterval.IsEmpty()) {
        return result;
    }

    // Special case empty splines
    if ((!data1 || data1->times.empty()) &&
        (!data2 || data2->times.empty()))
    {
        // Both splines are empty, value-blocks at all time. No differences.
        result = GfInterval();
        return result;
    }

    if ((!data1 || data1->times.empty()) ||
        (!data2 || data2->times.empty()))
    {
        // One spline is empty. They're completely different.
        //
        // XXX: For future value comparisons we need to return the portion of
        // compareInterval that is not a value-block.
        return result;
    }

    const TsTime preExtrapTime1 = data1->GetPreExtrapTime();
    const TsTime preExtrapTime2 = data2->GetPreExtrapTime();
    const TsTime postExtrapTime1 = data1->GetPostExtrapTime();
    const TsTime postExtrapTime2 = data2->GetPostExtrapTime();

    bool haveInfinitePreLoop = false, haveInfinitePostLoop = false;
    bool preExtrapDifferent = false, postExtrapDifferent = false;

    GfInterval iterInterval = compareInterval;

    // Handle infinite intervals with looped extrapolation. Infinite non-looped
    // extrapolation simply becomes a single segment.
    if (compareInterval.GetMin() == -inf) {
        if (data1->preExtrapolation.IsLooping() ||
            data2->preExtrapolation.IsLooping())
        {
            // We have an infinite time range and pre-extrap looping. Flag this
            // state and update our comparison interval.
            haveInfinitePreLoop = true;

            // If the time ranges are different or if either of the splines are
            // not looping, the pre-extrapolation is definitely different. Just
            // check the knots. Otherwise, iterate over one iteration of the
            // pre-extrapolation loop.
            if (preExtrapTime1 != preExtrapTime2 ||
                postExtrapTime1 != postExtrapTime2 ||
                !data1->preExtrapolation.IsLooping() ||
                !data2->preExtrapolation.IsLooping())
            {
                preExtrapDifferent = true;
                iterInterval.SetMin(std::min(preExtrapTime1, preExtrapTime2),
                                    CLOSED);
            } else {
                const double loopSpan = postExtrapTime1 - preExtrapTime1;
                iterInterval.SetMin(preExtrapTime1 - loopSpan, CLOSED);
            }
        }
    }

    if (compareInterval.GetMax() == +inf) {
        if (data1->postExtrapolation.IsLooping() ||
            data2->postExtrapolation.IsLooping())
        {
            // We have an infinite time range and post-extrap looping. Flag this
            // state and update our comparison interval.
            haveInfinitePostLoop = true;

            // If the time ranges are different or if either of the splines are
            // not looping, the post-extrapolation is definitely different. Just
            // check the knots. Otherwise, iterate over one iteration of the
            // post-extrapolation loop.
            if (preExtrapTime1 != preExtrapTime2 ||
                postExtrapTime1 != postExtrapTime2 ||
                !data1->postExtrapolation.IsLooping() ||
                !data2->postExtrapolation.IsLooping())
            {
                postExtrapDifferent = true;
                iterInterval.SetMax(std::max(postExtrapTime1, postExtrapTime2),
                                    OPEN);
            } else {
                const double loopSpan = postExtrapTime1 - preExtrapTime1;
                iterInterval.SetMax(postExtrapTime1 + loopSpan, OPEN);
            }
        }
    }

    // If we're processing all time and extrapolation is looping but different,
    // then the splines are infinitely different in both directions and we can
    // stop right here.  (Recall that result == compareInterval)
    if (preExtrapDifferent && postExtrapDifferent) {
        return result;
    }

    // Compare the segments.
    result = _CompareSegments(data1, data2, iterInterval);

    // Do we need to extend these results to infinity?
    //
    // If we've already determined that the extrapolation is different, or if we
    // have a pre-extrap loop and there are differences that would affect that
    // pre-extrap loop, those differences will repeat to -inf.
    const GfInterval affectsPreExtrap(
        -inf, std::max(postExtrapTime1, postExtrapTime2), CLOSED, OPEN);
    if (preExtrapDifferent ||
        (haveInfinitePreLoop &&
         result.Intersects(affectsPreExtrap)))
    {
        // The pre-extrap region is different to infinity. Update result.
        result |= GfInterval(-inf, iterInterval.GetMin(), OPEN, OPEN);
    }

    // If we've already determined that the extrapolation is different, or if we
    // have a post-extrap loop and there are differences that would affect that
    // post-extrap loop, those differences will repeat to +inf.
    const GfInterval affectsPostExtrap(
        std::min(preExtrapTime1, preExtrapTime2), +inf, CLOSED, OPEN);
    if (postExtrapDifferent ||
        (haveInfinitePostLoop &&
         result.Intersects(affectsPostExtrap)))
    {
        // The post-extrap region is different to infinity. Update result.
        result |= GfInterval(iterInterval.GetMax(), +inf, CLOSED, OPEN);
    }

    // Clamp the result to compareInterval
    result &= compareInterval;

    return result;
}


PXR_NAMESPACE_CLOSE_SCOPE
