//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/clip.h"
#include "pxr/usd/usd/interpolators.h"
#include "pxr/usd/usd/timeCode.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverScopedCache.h"
#include "pxr/usd/ar/resolverContextBinder.h"

#include "pxr/usd/pcp/layerStack.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/layerUtils.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/usdaFileFormat.h"

#include "pxr/usd/usd/tokens.h"

#include "pxr/base/gf/interval.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/arrayEdit.h"

#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "pxr/base/arch/pragmas.h"
ARCH_PRAGMA_MAYBE_UNINITIALIZED

PXR_NAMESPACE_OPEN_SCOPE

bool
UsdIsClipRelatedField(const TfToken& fieldName)
{
    return fieldName == UsdTokens->clips
        || fieldName == UsdTokens->clipSets;
}

std::vector<TfToken>
UsdGetClipRelatedFields()
{
    return std::vector<TfToken>{ 
        UsdTokens->clips, 
        UsdTokens->clipSets 
    };
}

std::ostream&
operator<<(std::ostream& out, const Usd_ClipRefPtr& clip)
{
    out << TfStringPrintf(
        "%s<%s> (start: %s end: %s)",
        TfStringify(clip->assetPath).c_str(),
        clip->primPath.GetString().c_str(),
        (clip->startTime == Usd_ClipTimesEarliest ?
            "-inf" : TfStringPrintf("%.3f", clip->startTime).c_str()),
        (clip->endTime == Usd_ClipTimesLatest ? 
            "inf" : TfStringPrintf("%.3f", clip->endTime).c_str()));
    return out;
}

// ------------------------------------------------------------

Usd_Clip::Usd_Clip()
    : startTime(0)
    , endTime(0)
    , _hasLayer(false)
{ 
}

Usd_Clip::Usd_Clip(
    const PcpLayerStackPtr& clipSourceLayerStack,
    const SdfPath& clipSourcePrimPath,
    size_t clipSourceLayerIndex,
    const SdfAssetPath& clipAssetPath,
    const SdfPath& clipPrimPath,
    ExternalTime clipAuthoredStartTime,
    ExternalTime clipStartTime,
    ExternalTime clipEndTime,
    const std::shared_ptr<TimeMappings> &timeMapping)
    : sourceLayerStack(clipSourceLayerStack)
    , sourcePrimPath(clipSourcePrimPath)
    , sourceLayer(
        TF_VERIFY(clipSourceLayerIndex 
                      < clipSourceLayerStack->GetLayers().size()) ?
            SdfLayerHandle(clipSourceLayerStack->GetLayers()[clipSourceLayerIndex]) :
            SdfLayerHandle())
    , assetPath(clipAssetPath)
    , primPath(clipPrimPath)
    , authoredStartTime(clipAuthoredStartTime)
    , startTime(clipStartTime)
    , endTime(clipEndTime)
    , times(timeMapping)
{ 
    // For performance reasons, we want to defer the loading of the layer
    // for this clip until absolutely needed. However, if the layer happens
    // to already be opened, we can take advantage of that here. 
    //
    // This is important for change processing. Clip layers will be kept
    // alive during change processing, so any clips that are reconstructed
    // will have the opportunity to reuse the already-opened layer.
    if (sourceLayer) {
        const ArResolverContextBinder binder(
            sourceLayerStack->GetIdentifier().pathResolverContext);
        _layer = SdfLayer::FindRelativeToLayer(
            sourceLayer, assetPath.GetAssetPath());
    }

    _hasLayer = (bool)_layer;
}

// Helper function to determine the linear segment in the given
// time mapping that applies to the given time.
static bool
_GetBracketingTimeSegment(
    const Usd_Clip::TimeMappings& times,
    Usd_Clip::ExternalTime time,
    size_t* m1, size_t* m2)
{
    if (times.empty()) {
        return false;
    }
    
    // This relies on the Usd_Clip c'tor inserting sentinel values at the
    // beginning and end of the TimeMappings object. Consumers rely on this
    // function never returning m1 == m2.
    if (time <= times.front().externalTime) {
        *m1 = 0;
        *m2 = 1;
    }
    else if (time >= times.back().externalTime) {
        *m1 = times.size() - 2;
        *m2 = times.size() - 1;
    }
    else {
        *m2 = std::distance(times.begin(), 
                std::lower_bound(times.begin(), times.end(),
                                 time, Usd_Clip::Usd_SortByExternalTime()));
        *m1 = *m2 - 1;
    }

    TF_VERIFY(*m1 < *m2);
    TF_VERIFY(0 <= *m1 && *m1 < times.size());
    TF_VERIFY(0 <= *m2 && *m2 < times.size());
    
    return true;
}

// Overload of _GetBracketingTimeSegment for usage relying on differentiation
// on pre time vs. regular time.
//
// XXX: We should unify the two overloads if possible, since the segment
// resolution logic should in theory not change whether the underlying data
// format is time samples or splines.
static bool
_GetBracketingTimeSegment(
    const Usd_Clip::TimeMappings& times,
    const UsdTimeCode externalTime,
    size_t* m1, size_t* m2)
{
    if (!_GetBracketingTimeSegment(times, externalTime.GetValue(), m1, m2)) {
        return false;
    }

    if (times[*m2].externalTime == externalTime.GetValue()) {
        if (externalTime.IsPreTime()
            && *m1 > 0 && times[*m1].isJumpDiscontinuity)
        {
            // Adjust segment out of the jump discontinuity region
            (*m1)--;
            (*m2)--;
        } else if (*m2 < times.size() - 1) {
            // We must adjust the segment for a regular time query in certain
            // scenarios. For example: `times` is [(0, 0), (5, -5), (10, 10)].
            //
            // _GetBracketingTimeSegment with a Usd_Clip::ExternalTime query of
            // 5 will give us the segment from (0, 0) to (5, -5). A caller that
            // is trying to determine whether a segment is flipped will see
            // that the section is flipped. However, that determination is only
            // correct if computed from the intended segment from (5, -5) to
            // (10, 10) for regular time queries.
            (*m1)++;
            (*m2)++;
        }
    }
    return true;
}

static 
Usd_Clip::ExternalTime
_GetTime(Usd_Clip::ExternalTime d)
{
    return d;
}

static 
Usd_Clip::ExternalTime
_GetTime(const Usd_Clip::TimeMapping& t) 
{
    return t.externalTime;    
}

static
Usd_Clip::TimeMappings::const_iterator
_GetLowerBound(
    Usd_Clip::TimeMappings::const_iterator begin,
    Usd_Clip::TimeMappings::const_iterator end,
    Usd_Clip::ExternalTime time)
{
    return std::lower_bound( 
            begin, end, time, 
            [](const Usd_Clip::TimeMapping& t, 
               const Usd_Clip::ExternalTime e) {
                return t.externalTime < e;
            }     
    ); 
}

template <typename Iterator>
static
Iterator
_GetLowerBound(
    Iterator begin, Iterator end, Usd_Clip::ExternalTime time)
{
    return std::lower_bound(begin, end, time);
}

// XXX: This is taken from sdf/data.cpp with slight modification.
// We should provide a free function in sdf to expose this behavior.
// This function is different in that it works on time mappings instead
// of raw doubles.
template <typename Iterator>
static
bool
_GetBracketingTimeSamples(
    Iterator begin, Iterator end,
    const Usd_Clip::ExternalTime time, 
    Usd_Clip::ExternalTime* tLower, 
    Usd_Clip::ExternalTime* tUpper) 
{
    if (begin == end) {
        return false;
    }

    if (time <= _GetTime(*begin)) {
        // Time is at-or-before the first sample.
        *tLower = *tUpper = _GetTime(*begin);
    } else if (time >= _GetTime(*(end - 1))) {
        // Time is at-or-after the last sample.
        *tLower = *tUpper = _GetTime(*(end - 1));
    } else {
        auto iter = _GetLowerBound(begin, end, time);
        if (_GetTime(*iter) == time) {
            // Time is exactly on a sample.
            *tLower = *tUpper = _GetTime(*iter);
        } else {
            // Time is in-between two samples; return the bracketing times.
            *tUpper = _GetTime(*iter);
            --iter;
            *tLower = _GetTime(*iter);
        }
    }
    return true;
}

bool 
Usd_Clip::_GetBracketingTimeSamplesForPathFromClipLayer(
    const SdfPath& path, ExternalTime time, 
    ExternalTime* tLower, ExternalTime* tUpper) const
{
    const SdfLayerRefPtr& clip = _GetLayerForClip();
    const SdfPath clipPath = TranslatePathToClip(path);
    const InternalTime timeInClip = _TranslateTimeToInternal(time);
    InternalTime lowerInClip, upperInClip;

    if (!clip->GetBracketingTimeSamplesForPath(
            clipPath, timeInClip, &lowerInClip, &upperInClip)) {
        return false;
    }

    // Need to translate the time samples in the internal time domain
    // to the external time domain. The external -> internal mapping
    // is many-to-one; a given internal time could translate to multiple
    // external times. We need to look for the translation that is closest
    // to the time we were given.
    //
    // An example case:
    //
    // int. time
    //  -
    //  |
    //  |                     m3    m1, m2, m3 are mappings in the times vector
    //  |                    ,*     s1, s2 are time samples in the clip
    // s2..................,'      
    //  |                ,'.
    // i0..............,'  .
    //  |            ,'.   .
    //  |          ,*  .   .
    // s1........,' m2 .   .
    //  |      ,'      .   .
    //  |    ,' .      .   .
    //  |   *   .      .   .
    //  | m1    .      .   .
    //  |-------.------.---.------| ext. time
    //          e1     e0  e2
    // 
    // Suppose we are asked for bracketing samples at external time t0.
    // We map this into the internal time domain, which gives us i0. The
    // bracketing samples for i0 in the internal domain are (s1, s2). 
    // 
    // Now we need to map these back to the external domain. The bracketing
    // time segment for e0 is (m2, m3). s1 is not in the range of this segment,
    // so we walk backwards to the previous segment (m1, m2). s1 *is* in the
    // range of this segment, so we use these mappings to map s1 to e1. For
    // s2, since s2 is in the range of (m2, m3), we use those mappings to map
    // s2 to e2. So, our final answer is (e1, e2).
    size_t m1, m2;
    if (!_GetBracketingTimeSegment(*times, time, &m1, &m2)) {
        *tLower = lowerInClip;
        *tUpper = upperInClip;
        return true;
    }

    std::optional<ExternalTime> translatedLower, translatedUpper;
    auto _CanTranslate = [&time, &upperInClip, &lowerInClip, this, 
                          &translatedLower, &translatedUpper](
        const TimeMappings& mappings, size_t i1, size_t i2,
        const bool translatingLower) 
    {
        const TimeMapping& map1 = mappings[i1];
        const TimeMapping& map2 = mappings[i2];

        // If this segment is a jump discontinuity it should not be used
        // to map any internal times to external times.
        if (map1.isJumpDiscontinuity) {
            return false;
        }

        const InternalTime timeInClip = 
            translatingLower ? lowerInClip : upperInClip;
        auto& translated = translatingLower ? translatedLower : translatedUpper;

        const InternalTime lower = 
            std::min(map1.internalTime, map2.internalTime);
        const InternalTime upper = 
            std::max(map1.internalTime, map2.internalTime);

        if (lower <= timeInClip && timeInClip <= upper) {
            if (map1.internalTime != map2.internalTime) {
                translated =
                    this->_TranslateTimeToExternal(timeInClip, i1, i2);
            } else {
                const bool lowerUpperMatch = (lowerInClip == upperInClip);
                if (lowerUpperMatch && time == map1.externalTime) {
                    translated = map1.externalTime;
                } else if (lowerUpperMatch && time == map2.externalTime) {
                    translated = map2.externalTime;
                } else {
                    if (translatingLower) {
                        translated = map1.externalTime;
                    } else {
                        translated = map2.externalTime;
                    }
                }
            }
        }
        return static_cast<bool>(translated);
    };

    for (int i1 = m1, i2 = m2; i1 >= 0 && i2 >= 0; --i1, --i2) {
        if (_CanTranslate(*times, i1, i2, /*lower=*/true)) { break; }
    }
        
    for (size_t i1 = m1, i2 = m2, sz = times->size(); i1 < sz && i2 < sz; ++i1, ++i2) {
        if (_CanTranslate(*times, i1, i2, /*lower=*/false)) { break; }
    }

    if (translatedLower && !translatedUpper) {
        translatedUpper = translatedLower;
    }
    else if (!translatedLower && translatedUpper) {
        translatedLower = translatedUpper;
    }
    else if (!translatedLower && !translatedUpper) {
        // If we haven't been able to translate either internal time, it's
        // because they are outside the range of the clip time mappings. We
        // clamp them to the nearest external time to match the behavior of
        // SdfLayer::GetBracketingTimeSamples.
        //
        // The issue here is that the clip may not have a sample at these
        // times. Usd_Clip::QueryTimeSample does a secondary step of finding
        // the corresponding time sample if it determines this is the case.
        //
        // The 'timingOutsideClip' test case in testUsdModelClips exercises
        // this behavior.
        if (lowerInClip < times->front().internalTime) {
            translatedLower = times->front().externalTime;
        }
        else if (lowerInClip > times->back().internalTime) {
            translatedLower = times->back().externalTime;
        }

        if (upperInClip < times->front().internalTime) {
            translatedUpper = times->front().externalTime;
        }
        else if (upperInClip > times->back().internalTime) {
            translatedUpper = times->back().externalTime;
        }
    }
            
    *tLower = *translatedLower;
    *tUpper = *translatedUpper;
    return true;
}

bool 
Usd_Clip::GetBracketingTimeSamplesForPath(
    const SdfPath& path, ExternalTime time, 
    ExternalTime* tLower, ExternalTime* tUpper) const
{
    std::array<Usd_Clip::ExternalTime, 5> bracketingTimes = { 0.0 };
    size_t numTimes = 0;

    // Add time samples from the clip layer.
    if (_GetBracketingTimeSamplesForPathFromClipLayer(
            path, time, 
            &bracketingTimes[numTimes], &bracketingTimes[numTimes + 1])) {
        numTimes += 2;
    }

    // Each external time in the clip times array is considered a time
    // sample.
    if (_GetBracketingTimeSamples(
            times->cbegin(), times->cend(), time,
            &bracketingTimes[numTimes], &bracketingTimes[numTimes + 1])) {
        numTimes += 2;
    }

    // Clips introduce time samples at their start time even
    // if time samples don't actually exist. This isolates each
    // clip from its neighbors and means that value resolution
    // never has to look at more than one clip to answer a
    // time sample query.
    bracketingTimes[numTimes] = authoredStartTime;
    numTimes++;

    // Remove bracketing times that are outside the clip's active range.
    {
        auto removeIt = std::remove_if(
            bracketingTimes.begin(), bracketingTimes.begin() + numTimes,
            [this](ExternalTime t) { return t < startTime || t >= endTime; });
        numTimes = std::distance(bracketingTimes.begin(), removeIt);
    }
        
    if (numTimes == 0) {
        return false;
    }
    else if (numTimes == 1) {
        *tLower = *tUpper = bracketingTimes[0];
        return true;
    }

    TF_AXIOM(numTimes <= bracketingTimes.size());
    std::sort(bracketingTimes.begin(), bracketingTimes.begin() + numTimes);
    auto uniqueIt = std::unique(
        bracketingTimes.begin(), bracketingTimes.begin() + numTimes);
    return _GetBracketingTimeSamples(
        bracketingTimes.begin(), uniqueIt, time, tLower, tUpper);
}

size_t
Usd_Clip::GetNumTimeSamplesForPath(const SdfPath& path) const
{
    // XXX: This is simple but inefficient. However, this function is
    // currently only used in one corner case in UsdStage, see
    // _ValueFromClipsMightBeTimeVarying. So for now, we can just
    // go with this until it becomes a bigger performance concern.
    return ListTimeSamplesForPath(path).size();
}

void
Usd_Clip::_ListTimeSamplesForPathFromClipLayer(
    const SdfPath& path,
    std::set<ExternalTime>* timeSamples) const
{
    std::set<InternalTime> timeSamplesInClip = 
        _GetLayerForClip()->ListTimeSamplesForPath(TranslatePathToClip(path));
    if (times->empty()) {
        *timeSamples = std::move(timeSamplesInClip);

        // Filter out all samples that are outside the clip's active range
        timeSamples->erase(
            timeSamples->begin(), timeSamples->lower_bound(startTime));
        timeSamples->erase(
            timeSamples->lower_bound(endTime), timeSamples->end());
        return;
    }

    // A clip is active in the time range [startTime, endTime).
    const GfInterval clipTimeInterval(
        startTime, endTime, /* minClosed = */ true, /* maxClosed = */ false);

    // We need to convert the internal time samples to the external
    // domain using the clip's time mapping. This is tricky because the
    // mapping is many-to-one: multiple external times may map to the
    // same internal time, e.g. mapping { 0:5, 5:10, 10:5 }. 
    //
    // To deal with this, every internal time sample has to be checked 
    // against the entire mapping function.
    for (InternalTime t: timeSamplesInClip) {
        for (size_t i = 0; i < times->size() - 1; ++i) {
            const TimeMapping& m1 = (*times)[i];
            const TimeMapping& m2 = (*times)[i+1];

            // Ignore time mappings whose external time domain does not 
            // intersect the times at which this clip is active.
            const GfInterval mappingInterval(m1.externalTime, m2.externalTime);
            if (!mappingInterval.Intersects(clipTimeInterval)) {
                continue;
            }

            // If this segment is a jump discontinuity it should not be used
            // to map any internal times to external times.
            if (m1.isJumpDiscontinuity) {
                continue;
            }

            if (std::min(m1.internalTime, m2.internalTime) <= t
                && t <= std::max(m1.internalTime, m2.internalTime)) {
                if (m1.internalTime == m2.internalTime) {
                    if (clipTimeInterval.Contains(m1.externalTime)) {
                        timeSamples->insert(m1.externalTime);
                    }
                    if (clipTimeInterval.Contains(m2.externalTime)) {
                        timeSamples->insert(m2.externalTime);
                    }
                }
                else {
                    const ExternalTime extTime = 
                        _TranslateTimeToExternal(t, i, i+1);
                    if (clipTimeInterval.Contains(extTime)) {
                        timeSamples->insert(extTime);
                    }
                }
            }
        }
    }
}

std::set<Usd_Clip::ExternalTime>
Usd_Clip::ListTimeSamplesForPath(const SdfPath& path) const
{
    // Retrieve time samples from the clip layer mapped to external times.
    std::set<ExternalTime> timeSamples;
    _ListTimeSamplesForPathFromClipLayer(path, &timeSamples);

    // Each entry in the clip's time mapping is considered a time sample,
    // so add them in here.
    for (const TimeMapping& t : *times) {
        if (startTime <= t.externalTime && t.externalTime < endTime) {
            timeSamples.insert(t.externalTime);
        }
    }

    // Clips introduce time samples at their start time to
    // isolate them from surrounding clips.
    //
    // See GetBracketingTimeSamplesForPath for more details.
    timeSamples.insert(authoredStartTime);

    return timeSamples;
}

bool 
Usd_Clip::HasField(const SdfPath& path, const TfToken& field) const
{
    return _GetLayerForClip()->HasField(TranslatePathToClip(path), field);
}

bool
Usd_Clip::HasAuthoredTimeSamples(const SdfPath& path) const
{
    return _GetLayerForClip()->GetNumTimeSamplesForPath(
        TranslatePathToClip(path)) > 0;    
}

bool
Usd_Clip::HasAuthoredSpline(const SdfPath& path) const
{
    return _GetLayerForClip()->HasField(TranslatePathToClip(path),
                                        SdfFieldKeys->Spline);
}

SdfPath
Usd_Clip::TranslatePathToClip(const SdfPath& path) const
{
    return path.ReplacePrefix(sourcePrimPath, primPath);
}

static Usd_Clip::InternalTime
_TranslateTimeToInternalHelper(
    Usd_Clip::ExternalTime extTime,
    const Usd_Clip::TimeMapping& m1,
    const Usd_Clip::TimeMapping& m2)
{
    // Early out in some special cases to avoid unnecessary
    // math operations that could introduce precision issues.
    if (m1.externalTime == m2.externalTime) {
        return m1.internalTime;
    }
    else if (extTime == m1.externalTime) {
        return m1.internalTime;
    }
    else if (extTime == m2.externalTime) {
        return m2.internalTime;
    }

    return (m2.internalTime - m1.internalTime) /
           (m2.externalTime - m1.externalTime)
        * (extTime - m1.externalTime)
        + m1.internalTime;
}

Usd_Clip::InternalTime
Usd_Clip::_TranslateTimeToInternal(
    UsdTimeCode extTime, TimeMapping* m1Out, TimeMapping* m2Out) const
{
    // Report the TimeMapping segment used for the translation so callers can
    // apply the same linear map (scale and offset) to time-based values.
    auto setOut = [&](const TimeMapping& a, const TimeMapping& b) {
        if (m1Out) { *m1Out = a; }
        if (m2Out) { *m2Out = b; }
    };

    size_t i1, i2;
    if (!_GetBracketingTimeSegment(*times, extTime.GetValue(), &i1, &i2)) {
        // No time mapping applies; times (and values) pass through unchanged.
        setOut(TimeMapping(0.0, 0.0), TimeMapping(1.0, 1.0));
        return extTime.GetValue();
    }

    const TimeMapping& m1 = (*times)[i1];
    const TimeMapping& m2 = (*times)[i2];

    // If the time segment ends on the left side of a jump discontinuity
    // we use the authored external time for the translation. 
    //
    // For example, if the authored times metadata looked like:
    //   [(0, 0), (10, 10), (10, 0), ...]
    //
    // Our time mappings would be:
    //   [(0, 0), (9.99..., 10), (10, 0), ...]
    //
    // Let's say we had a clip with a time sample at t = 3. If we were
    // to query the attribute at extTime = 3, using the time mappings as-is
    // would lead us to use the mappings (0, 0) and (9.99..., 10) to
    // translate to an internal time. This would give a translated internal
    // time like 3.00000001. Since the clip doesn't have a time sample at 
    // that exact time, QueryTimeSample would wind up performing additional 
    // interpolation, which decreases performance and also introduces
    // precision errors.
    //
    // With this code, we wind up translating using the mappings
    // (0, 0) and (10, 10), which gives a translated internal time of 3.
    // This avoids all of the issues above and more closely matches the intent
    // expressed in the authored times metadata.
    // 
    // We also need to make sure pretime time segments are handled properly when 
    // we are at a jump discontinuity.
    if (extTime.IsPreTime() && m1.isJumpDiscontinuity) {
        // We are querying for a pre-time, and we are at a jump
        // discontinuity, instead of using the internal time from next time
        // and interpolating, we should use the internalTime from this jump
        // discontinuity mapping to query for this clip's internal time.
        //
        // For value scaling the pre-time approaches the jump from the left, so
        // report the real segment ending at the jump (using the jump's
        // authored external time, which is m2.externalTime).
        if (i1 > 0) {
            setOut((*times)[i1 - 1],
                   TimeMapping(m2.externalTime, m1.internalTime));
        }
        else {
            setOut(m1, m2);
        }
        return m1.internalTime;
    }

    if (m2.isJumpDiscontinuity) {
        TF_VERIFY(i2 + 1 < times->size());
        const TimeMapping& m3 = (*times)[i2 + 1];
        const TimeMapping m2Sub(m3.externalTime, m2.internalTime);
        setOut(m1, m2Sub);
        return _TranslateTimeToInternalHelper(extTime.GetValue(), m1, m2Sub);
    }

    // Report the bracketing segment used for scaling time-based values. A few
    // cases don't carry a usable scale in (m1, m2) themselves:
    //  * A regular query at/after a jump discontinuity (m1 is the jump's left
    //    endpoint): the value belongs to the real segment starting on the
    //    right side of the jump, i.e. (i2, i2 + 1).
    //  * The clip's boundaries: _GetBracketingTimeSegment returns a degenerate
    //    sentinel pair (the duplicated first/last mapping) whose equal internal
    //    times carry no scale; report the adjacent real segment so values scale
    //    by the first/last real segment's rate.
    {
        const size_t n = times->size();
        size_t r1 = i1, r2 = i2;
        if (m1.isJumpDiscontinuity && i2 + 1 < n) {
            r1 = i2; r2 = i2 + 1;
        }
        else if (i1 == 0 && n >= 3) {
            r1 = 1; r2 = 2;
        }
        else if (i2 == n - 1 && n >= 3) {
            r1 = n - 3; r2 = n - 2;
        }
        const TimeMapping& rm1 = (*times)[r1];
        TimeMapping rm2 = (*times)[r2];
        if (rm2.isJumpDiscontinuity && r2 + 1 < n) {
            rm2 = TimeMapping((*times)[r2 + 1].externalTime, rm2.internalTime);
        }
        setOut(rm1, rm2);
    }
    return _TranslateTimeToInternalHelper(extTime.GetValue(), m1, m2);
}

static Usd_Clip::ExternalTime
_TranslateTimeToExternalHelper(
    Usd_Clip::InternalTime intTime, 
    const Usd_Clip::TimeMapping& m1, 
    const Usd_Clip::TimeMapping& m2)
{
    // Early out in some special cases to avoid unnecessary
    // math operations that could introduce precision issues.
    if (m1.internalTime == m2.internalTime) {
        return m1.externalTime;
    }
    else if (intTime == m1.internalTime) {
        return m1.externalTime;
    }
    else if (intTime == m2.internalTime) {
        return m2.externalTime;
    }

    return (m2.externalTime - m1.externalTime) /
           (m2.internalTime - m1.internalTime)
        * (intTime - m1.internalTime)
        + m1.externalTime;
}

// Returns the scale (Delta external / Delta internal) of the time mapping
// segment [m1, m2]. This is the factor by which a duration in internal time
// is stretched or compressed when expressed in external time.
static double
_GetTimeScale(
    const Usd_Clip::TimeMapping& m1,
    const Usd_Clip::TimeMapping& m2)
{
    // Degenerate/held segment: no scaling (also avoids divide-by-zero).
    if (m1.internalTime == m2.internalTime) {
        return 1.0;
    }
    return (m2.externalTime - m1.externalTime) /
           (m2.internalTime - m1.internalTime);
}

Usd_Clip::ExternalTime
Usd_Clip::_TranslateTimeToExternal(
    InternalTime intTime, size_t i1, size_t i2) const
{
    const TimeMapping& m1 = (*times)[i1];
    const TimeMapping& m2 = (*times)[i2];

    // Clients should never be trying to map an internal time through a jump
    // discontinuity.
    TF_VERIFY(!m1.isJumpDiscontinuity);

    // If the time segment ends on the left side of a jump discontinuity,
    // we use the authored external time for the translation. 
    //
    // For example, if the authored times metadata looked like:
    //   [(0, 0), (10, 10), (10, 0), ...]
    //
    // Our time mappings would be:
    //   [(0, 0), (9.99..., 10), (10, 0), ...]
    //
    // Let's say we had a clip with a time sample at t = 3. If we were to
    // query the attribute's time samples, using the time mappings as-is
    // would lead us to use the mappings (0, 0) and (9.99..., 10) to translate
    // to an external time. This would give us a translated external time like
    // 2.999999, which is unexpected. If this value was used to query for
    // attribute values, we would run into the same issues described in
    // _TranslateTimeToInternal.
    //
    // With this code, we wind up translating using the mappings
    // (0, 0) and (10, 10), which gives a translated external time of 3.
    // This avoids all of the issues above and more closely matches the intent
    // expressed in the authored times metadata.
    if (m2.isJumpDiscontinuity) {
        TF_VERIFY(i2 + 1 < times->size());
        const TimeMapping& m3 = (*times)[i2 + 1];
        return _TranslateTimeToExternalHelper(
            intTime, m1, TimeMapping(m3.externalTime, m2.internalTime));
    }

    return _TranslateTimeToExternalHelper(intTime, m1, m2);
}

UsdTimeCode
Usd_Clip::_TranslateTimeToInternalDualValued(
    UsdTimeCode extTime, TimeMapping* m1Out, TimeMapping* m2Out) const
{
    bool localPreTime = extTime.IsPreTime();

    // If we're in a reversed clip times section, flip the desired time query
    // from pre time to regular time or vice versa. When negative time scaling
    // affects a dual valued item like a spline's knot, pre and post values of
    // that item are flipped.
    size_t m1, m2;
    if (_GetBracketingTimeSegment(*times, extTime, &m1, &m2) &&
        (*times)[m1].internalTime > (*times)[m2].internalTime)
    {
        localPreTime = !localPreTime;
    }

    // Clamp the external time to the clip times range if it exists;
    // past the times range the active clip's attribute's value is held
    // at the clip time associated with the clip times range boundary.
    // Note that this clip time is always a regular time code, not a
    // pre time code.
    if (times && !times->empty()) {
        if (times->front().externalTime > extTime) {
            extTime = times->front().externalTime;
            localPreTime = false;
        } else if (times->back().externalTime < extTime) {
            extTime = times->back().externalTime;
            localPreTime = false;
        }
    }
    const InternalTime clipTime =
        _TranslateTimeToInternal(extTime, m1Out, m2Out);

    return localPreTime ? UsdTimeCode::PreTime(clipTime)
                        : UsdTimeCode(clipTime);
}


SdfPropertySpecHandle
Usd_Clip::GetPropertyAtPath(const SdfPath &path) const
{
    return _GetLayerForClip()->GetPropertyAtPath(TranslatePathToClip(path));
}

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (dummy_clip)
    ((dummy_clipFormat, "dummy_clip.%s"))
    );

SdfLayerRefPtr
Usd_Clip::_GetLayerForClip() const
{
    if (_hasLayer) {
        return _layer; 
    }

    SdfLayerRefPtr layer;

    if (TF_VERIFY(sourceLayer)) {
        const ArResolverContextBinder binder(
            sourceLayerStack->GetIdentifier().pathResolverContext);
        layer = SdfLayer::FindOrOpenRelativeToLayer(
            sourceLayer, assetPath.GetAssetPath());
    }

    if (!layer) {
        // If we failed to open the specified layer, report an error
        // and use a dummy anonymous layer instead, to avoid having
        // to check layer validity everywhere and to avoid reissuing
        // this error.
        // XXX: Better way to report this error?
        TF_WARN("Unable to open clip layer @%s@", 
                assetPath.GetAssetPath().c_str());
        layer = SdfLayer::CreateAnonymous(TfStringPrintf(
                     _tokens->dummy_clipFormat.GetText(), 
                     SdfUsdaFileFormatTokens->Id.GetText()));
    }

    std::lock_guard<std::mutex> lock(_layerMutex);
    if (!_layer) { 
        _layer = layer;
        _hasLayer = true;
    }

    return _layer;
}

SdfLayerHandle
Usd_Clip::GetLayer() const
{
    const SdfLayerRefPtr& layer = _GetLayerForClip();
    return TfStringStartsWith(layer->GetIdentifier(), 
                              _tokens->dummy_clip.GetString()) ?
        SdfLayerHandle() : SdfLayerHandle(layer);
}

SdfLayerHandle
Usd_Clip::GetLayerIfOpen() const
{
    if (!_hasLayer) {
        return SdfLayerHandle();
    }
    return GetLayer();
}

namespace { // Anonymous namespace

// GfTimeCode values from clips need to be converted from internal time to
// external time. A time code is a point on the time axis, so we apply the
// full linear map of the bracketing segment [m1, m2] (both scale and offset),
// exactly as _TranslateTimeToExternal maps an internal time to external time.
inline
void
_ConvertValueForTime(const Usd_Clip::TimeMapping &m1,
                     const Usd_Clip::TimeMapping &m2,
                     GfTimeCode *value)
{
    *value = GfTimeCode(
        _TranslateTimeToExternalHelper(value->GetValue(), m1, m2));
}

// Similarly we convert arrays of GfTimeCodes.
inline
void
_ConvertValueForTime(const Usd_Clip::TimeMapping &m1,
                     const Usd_Clip::TimeMapping &m2,
                     VtArray<GfTimeCode> *value)
{
    for (size_t i = 0; i < value->size(); ++i) {
        _ConvertValueForTime(m1, m2, &(*value)[i]);
    }
}

// Similarly we convert arrayEdits of GfTimeCodes.
inline
void
_ConvertValueForTime(const Usd_Clip::TimeMapping &m1,
                     const Usd_Clip::TimeMapping &m2,
                     VtArrayEdit<GfTimeCode> *value)
{
    for (GfTimeCode &tc: value->GetMutableLiterals()) {
        _ConvertValueForTime(m1, m2, &tc);
    }
}

// GfDuration values from clips need to be scaled from internal time to
// external time. A duration is a length on the time axis (a difference of
// time codes), so only the segment's scale applies; any offset cancels out.
inline
void
_ConvertValueForTime(const Usd_Clip::TimeMapping &m1,
                     const Usd_Clip::TimeMapping &m2,
                     GfDuration *value)
{
    *value = *value * _GetTimeScale(m1, m2);
}

// Similarly we convert arrays of GfDurations.
inline
void
_ConvertValueForTime(const Usd_Clip::TimeMapping &m1,
                     const Usd_Clip::TimeMapping &m2,
                     VtArray<GfDuration> *value)
{
    for (size_t i = 0; i < value->size(); ++i) {
        _ConvertValueForTime(m1, m2, &(*value)[i]);
    }
}

// Similarly we convert arrayEdits of GfDurations.
inline
void
_ConvertValueForTime(const Usd_Clip::TimeMapping &m1,
                     const Usd_Clip::TimeMapping &m2,
                     VtArrayEdit<GfDuration> *value)
{
    for (GfDuration &dur: value->GetMutableLiterals()) {
        _ConvertValueForTime(m1, m2, &dur);
    }
}

// Helpers for accessing the typed value from type erased values, needed for
// converting GfTimeCodes.
template <class T>
inline
void _UncheckedSwap(SdfAbstractDataValue *value, T& val) {
    std::swap(*static_cast<T*>(value->value), val);
}

template <class T>
inline
void _UncheckedSwap(VtValue *value, T& val) {
    value->UncheckedSwap(val);
}

template <class T>
inline
bool _IsHolding(const SdfAbstractDataValue &value) {
    return TfSafeTypeCompare(typeid(T), value.valueType);
}

template <class T>
inline
bool _IsHolding(const VtValue &value) {
    return value.IsHolding<T>();
}

// For type erased values, we need to convert them if they hold GfTimeCode 
// based types.
template <class Storage>
inline
void
_ConvertTypeErasedValueForTime(const Usd_Clip::TimeMapping &m1,
                               const Usd_Clip::TimeMapping &m2,
                               Storage *value)
{
    if (_IsHolding<GfTimeCode>(*value)) {
        GfTimeCode rawVal;
        _UncheckedSwap(value, rawVal);
        _ConvertValueForTime(m1, m2, &rawVal);
        _UncheckedSwap(value, rawVal);
    } else if (_IsHolding<VtArray<GfTimeCode>>(*value)) {
        VtArray<GfTimeCode> rawVal;
        _UncheckedSwap(value, rawVal);
        _ConvertValueForTime(m1, m2, &rawVal);
        _UncheckedSwap(value, rawVal);
    } else if (_IsHolding<VtArrayEdit<GfTimeCode>>(*value)) {
        VtArrayEdit<GfTimeCode> rawVal;
        _UncheckedSwap(value, rawVal);
        _ConvertValueForTime(m1, m2, &rawVal);
        _UncheckedSwap(value, rawVal);
    } else if (_IsHolding<GfDuration>(*value)) {
        GfDuration rawVal;
        _UncheckedSwap(value, rawVal);
        _ConvertValueForTime(m1, m2, &rawVal);
        _UncheckedSwap(value, rawVal);
    } else if (_IsHolding<VtArray<GfDuration>>(*value)) {
        VtArray<GfDuration> rawVal;
        _UncheckedSwap(value, rawVal);
        _ConvertValueForTime(m1, m2, &rawVal);
        _UncheckedSwap(value, rawVal);
    } else if (_IsHolding<VtArrayEdit<GfDuration>>(*value)) {
        VtArrayEdit<GfDuration> rawVal;
        _UncheckedSwap(value, rawVal);
        _ConvertValueForTime(m1, m2, &rawVal);
        _UncheckedSwap(value, rawVal);
    }
}

void
_ConvertValueForTime(const Usd_Clip::TimeMapping &m1,
                     const Usd_Clip::TimeMapping &m2,
                     VtValue *value)
{
    _ConvertTypeErasedValueForTime(m1, m2, value);
}

void
_ConvertValueForTime(const Usd_Clip::TimeMapping &m1,
                     const Usd_Clip::TimeMapping &m2,
                     SdfAbstractDataValue *value)
{
    _ConvertTypeErasedValueForTime(m1, m2, value);
}

// Fallback no-op default for the rest of the value types; there is no time
// conversion necessary for non-timecode types.
template <class T>
inline
void _ConvertValueForTime(const Usd_Clip::TimeMapping &m1,
                          const Usd_Clip::TimeMapping &m2,
                          T *value)
{
}

template <class T>
static bool
_Interpolate(
    const SdfLayerRefPtr& clip, const SdfPath &clipPath,
    Usd_Clip::InternalTime clipTime, Usd_Interpolator const & interpolator,
    T* value)
{
    double lowerInClip, upperInClip;
    if (clip->GetBracketingTimeSamplesForPath(
            clipPath, clipTime, &lowerInClip, &upperInClip)) {

        Usd_InterpolationSampleSeries samples;
        if (interpolator.GetInterpolatingSamples(
                clip, clipPath, clipTime,
                lowerInClip, upperInClip, &samples)) {
            Usd_Interpolate(&samples, clipTime);
            Usd_SetValue(value, samples[0].value);
            return true;
        }
        return false;
    }

    return false;
}

TsSpline
_MakeHeldSpline(
    const TsSpline& clipSpline,
    Usd_Clip::ExternalTime sectionStart,
    Usd_Clip::ExternalTime sectionEnd,
    Usd_Clip::InternalTime clipQueryTime)
{
    TsSpline spline(clipSpline.GetValueType());
    TsKnot knot(clipSpline.GetValueType());

    // Note that held sections of spline always set the held section value to
    // the result of the value query (not pre-value).
    VtValue knotValue;
    const bool hasValue = Usd_QuerySpline(clipSpline, clipQueryTime,
                                          &knotValue);

    if (sectionStart == Usd_ClipTimesEarliest) {
        spline.SetPreExtrapolation(hasValue ? TsExtrapHeld
                                            : TsExtrapValueBlock);
    } else {
        knot.SetTime(sectionStart);
        hasValue ? knot.SetValue(knotValue)
                 : knot.SetNextInterpolation(TsInterpValueBlock);
        spline.SetKnot(knot);
    }

    if (sectionEnd == Usd_ClipTimesLatest) {
        spline.SetPostExtrapolation(hasValue ? TsExtrapHeld
                                             : TsExtrapValueBlock);
    } else {
        knot.SetTime(sectionEnd);
        if (hasValue) {
            knot.SetValue(knotValue);
        }
        spline.SetKnot(knot);
    }

    return spline;
}

}; // End anonymous namespace

const std::type_info &
Usd_Clip::QueryTimeSampleTypeid(const SdfPath &path, UsdTimeCode time) const
{
    const SdfPath clipPath = TranslatePathToClip(path);
    const InternalTime clipTime = _TranslateTimeToInternal(time);
    const SdfLayerRefPtr& clip = _GetLayerForClip();

    return clip->QueryTimeSampleTypeid(clipPath, clipTime);
}

template <class T>
bool 
Usd_Clip::QueryTimeSample(
    const SdfPath& path, UsdTimeCode time, 
    Usd_Interpolator const & interpolator, T* value) const
{
    const SdfPath clipPath = TranslatePathToClip(path);
    TimeMapping m1, m2;
    const InternalTime clipTime = _TranslateTimeToInternal(time, &m1, &m2);
    const SdfLayerRefPtr& clip = _GetLayerForClip();

    if (!clip->QueryTimeSample(clipPath, clipTime, value)) {
        // See comment in Usd_Clip::GetBracketingTimeSamples.
        if (!_Interpolate(clip, clipPath, clipTime, interpolator, value)) {
            return false;
        }
    }

    // Convert values containing GfTimeCodes or GfDurations if necessary.
    _ConvertValueForTime(m1, m2, value);
    return true;
}

#define _INSTANTIATE_QUERY_TIME_SAMPLE(unused, elem)            \
    template bool Usd_Clip::QueryTimeSample(                    \
        const SdfPath&, UsdTimeCode,                            \
        Usd_Interpolator const &,                               \
        SDF_VALUE_CPP_TYPE(elem)*) const;                       \
    template bool Usd_Clip::QueryTimeSample(                    \
        const SdfPath&, UsdTimeCode,                            \
        Usd_Interpolator const &,                               \
        SDF_VALUE_CPP_ARRAY_TYPE(elem)*) const;

TF_PP_SEQ_FOR_EACH(_INSTANTIATE_QUERY_TIME_SAMPLE, ~, SDF_VALUE_TYPES)
#undef _INSTANTIATE_QUERY_TIME_SAMPLE

template bool Usd_Clip::QueryTimeSample(
    const SdfPath&, UsdTimeCode,
    Usd_Interpolator const &,
    SdfAbstractDataValue*) const;

template bool Usd_Clip::QueryTimeSample(
    const SdfPath&, UsdTimeCode,
    Usd_Interpolator const &,
    VtValue*) const;

bool
Usd_Clip::GetSplineForClip(const SdfPath& path, TsSpline* result) const
{
    const SdfPath clipPath = TranslatePathToClip(path);
    const SdfLayerRefPtr& clip = _GetLayerForClip();
    return clip->HasField(clipPath, SdfFieldKeys->Spline, result);
}

template <class T>
bool
Usd_Clip::QuerySpline(
    const SdfPath& path,
    UsdTimeCode time,
    T* value) const
{
    TsSpline spline;
    if (!GetSplineForClip(path, &spline)) {
        return false;
    }

    TimeMapping m1, m2;
    const UsdTimeCode clipTime =
        _TranslateTimeToInternalDualValued(time, &m1, &m2);

    // Note that we don't need to apply a layer offset, since it's baked
    // into clip times upon clipset construction.
    bool ok = Usd_QuerySpline(spline, clipTime, value);
    if (!ok) {
        return false;
    }

    // Convert GfTimeCode or GfDuration if necessary.
    // Conversion of the evaluation vs. evaluation of a converted
    // spline may result in slightly different numbers because of differences
    // in floating point operator rounding accumulation.
    _ConvertValueForTime(m1, m2, value);
    return true;
}

#define _INSTANTIATE_QUERY_SPLINE(unused, elem)                 \
    template bool Usd_Clip::QuerySpline(                        \
        const SdfPath&, UsdTimeCode,                            \
        TS_SPLINE_VALUE_CPP_TYPE(elem)*) const;

TF_PP_SEQ_FOR_EACH(_INSTANTIATE_QUERY_SPLINE, ~, TS_SPLINE_SUPPORTED_VALUE_TYPES)
#undef _INSTANTIATE_QUERY_SPLINE

template bool Usd_Clip::QuerySpline(
    const SdfPath&, UsdTimeCode,
    SdfAbstractDataValue*) const;
template bool Usd_Clip::QuerySpline(
    const SdfPath&, UsdTimeCode,
    VtValue*) const;

bool
Usd_Clip::BuildSpline(const SdfPath& path, TsSpline* result) const
{
    TsSpline clipSpline;
    const SdfPath clipPath = TranslatePathToClip(path);
    const SdfLayerRefPtr& clip = _GetLayerForClip();
    if (!clip->HasField(clipPath, SdfFieldKeys->Spline, &clipSpline)) {
        return false;
    }

    if (clipSpline.IsEmpty()) {
        // We set queryTime to 0, but really any query time will work
        // because we should get a value block no matter the query time.
        *result = _MakeHeldSpline(clipSpline, startTime, endTime, 0);
        return true;
    }


    // If there are no times, truncate and return the spline as-is. This is
    // likely the most common case.
    if (times->empty()) {
        const GfInterval interval(startTime, endTime,
                  /* minClosed */ startTime != Usd_ClipTimesEarliest,
                  /* maxClosed */ false);
        *result = clipSpline.GetTruncated(interval);
        return true;
    }

    // There are times. Collect one timing section at a time, adding truncated
    // and scaled sub-splines to `splines` as we go.
    std::vector<TsSpline> splines;
    auto it = std::upper_bound(times->begin(), times->end(), startTime,
                               Usd_Clip::Usd_SortByExternalTime());
    ExternalTime extSectionStart = startTime; 
    InternalTime clipSectionStart = _TranslateTimeToInternal(extSectionStart);

    if (it == times->begin()) {
        // Encountered a time section that precedes values in `times`, so
        // we need to build out a held section of spline. The first TimeMapping
        // is repeated, so we can skip it.
        ++it;
        ExternalTime extSectionEnd = std::min(it->externalTime, endTime);
        InternalTime queryTime = it->internalTime;
        splines.push_back(_MakeHeldSpline(clipSpline, extSectionStart,
                                          extSectionEnd, queryTime));

        // Start the next section at the current segment.
        extSectionStart = extSectionEnd;
        clipSectionStart = queryTime;
        it++;
    } else if (it != times->end()) {
        // We want to start at the first time that is <= startTime, so we
        // need to decrement the iterator to get the section start data.
        it--;
        extSectionStart = std::max(it->externalTime, startTime);
        clipSectionStart = _TranslateTimeToInternal(extSectionStart);
        it++;
    }

    // Each iteration builds a spline for [extSectionStart, extSectionEnd).
    // Skip the last entry in times because it's a sentinel value -- a repeat
    // of the real last TimeMapping.
    while (std::distance(it, times->end()) > 1 && endTime > extSectionStart)
    {
        // If we're ending at a jump discontinuity, we need to get the actual
        // external time instead of the stored "external - SafeStep()" time.
        const ExternalTime extTime =
            it->isJumpDiscontinuity ? (it+1)->externalTime
                                    : it->externalTime;

        // Compute the endpoint for the current timing section.
        const ExternalTime extSectionEnd = std::min(extTime, endTime);
        const UsdTimeCode extTimeCode =
            it->isJumpDiscontinuity ? UsdTimeCode::PreTime(extSectionEnd)
                                    : UsdTimeCode(extSectionEnd);
        const InternalTime clipSectionEnd =
            _TranslateTimeToInternal(extTimeCode);

        TsSpline spline;
        if (clipSectionEnd == clipSectionStart) {
            spline = _MakeHeldSpline(clipSpline, extSectionStart,
                                     extSectionEnd, clipSectionStart);
        } else {
            // Get a truncated spline for the current timing section. We set the
            // pre and post extrap fallbacks to held because we need the
            // overall pre and post extrapolations of the concatenation result
            // to be held. Any splines in the middle don't contribute extraps.
            const InternalTime clipTimeStart =
                std::min(clipSectionStart, clipSectionEnd);
            const InternalTime clipTimeEnd =
                std::max(clipSectionStart, clipSectionEnd);
            const GfInterval interval(clipTimeStart, clipTimeEnd,
                      /* minClosed */ clipTimeStart != Usd_ClipTimesEarliest,
                      /* maxClosed */ false);
            spline = clipSpline.GetTruncated(interval, TsExtrapHeld, TsExtrapHeld);

            // Compute time scaling factors for this timing section
            const double timeScale = (extSectionEnd - extSectionStart)
                                   / (clipSectionEnd - clipSectionStart);
            const double clipSectionStartScaled = timeScale * clipSectionStart;
            const double timeOffset = extSectionStart - clipSectionStartScaled;
            spline = spline.GetTimeScaled(timeScale, timeOffset);

            // Normalize spline. Concatenate relies on the boundary knots of
            // input splines' GetKnots being at exactly the same times. We
            // adjust the desired knot times directly without adjusting values
            // because any discrepancies between knot times are due to slight
            // differences in floating point operation rounding accumulation.
            const TsKnotMap knots = spline.GetKnots();
            TF_VERIFY(knots.size() >= 2);
            TsKnot startKnot = *knots.begin();
            if (startKnot.GetTime() != extSectionStart) {
                spline.RemoveKnot(startKnot.GetTime());
                startKnot.SetTime(extSectionStart);
                spline.SetKnot(startKnot);
            }
            TsKnot endKnot = *knots.rbegin();
            if (endKnot.GetTime() != extSectionEnd) {
                spline.RemoveKnot(endKnot.GetTime());
                endKnot.SetTime(extSectionEnd);
                spline.SetKnot(endKnot);
            }
        }

        splines.push_back(spline);
        if (it->isJumpDiscontinuity) {
            it++;
            extSectionStart = it->externalTime;
            clipSectionStart = it->internalTime;
        } else {
            extSectionStart = extSectionEnd;
            clipSectionStart = clipSectionEnd;
        }
        it++;
    }

    if ((it+1) >= times->end() && endTime > extSectionStart) {
        // Encountered a time section that follows values in `times`, so
        // we need to build out a held section of spline.
        splines.push_back(_MakeHeldSpline(clipSpline, extSectionStart,
                                          endTime, clipSectionStart));
    }

    if (splines.empty()) {
        return false;
    }

    if (splines.size() == 1) {
        *result = splines[0];
        return true;
    }

    *result = TsSpline::Concatenate(splines);
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE

