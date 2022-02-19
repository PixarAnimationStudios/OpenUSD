//
// Copyright 2016 Pixar
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
#include "pxr/pxr.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/clip.h"
#include "pxr/usd/usd/interpolators.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverScopedCache.h"
#include "pxr/usd/ar/resolverContextBinder.h"

#include "pxr/usd/pcp/layerStack.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/layerUtils.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/usd/usd/tokens.h"
#include "pxr/usd/usd/usdaFileFormat.h"

#include "pxr/base/gf/interval.h"
#include "pxr/base/tf/stringUtils.h"

#include <boost/optional.hpp>

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
    , sourceLayerIndex(clipSourceLayerIndex)
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
    if (TF_VERIFY(sourceLayerIndex < sourceLayerStack->GetLayers().size())) {
        const ArResolverContextBinder binder(
            sourceLayerStack->GetIdentifier().pathResolverContext);
        _layer = SdfLayer::FindRelativeToLayer(
            sourceLayerStack->GetLayers()[sourceLayerIndex],
            assetPath.GetAssetPath());
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
    const SdfPath clipPath = _TranslatePathToClip(path);
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

    boost::optional<ExternalTime> translatedLower, translatedUpper;
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
                translated.reset(
                    this->_TranslateTimeToExternal(timeInClip, i1, i2));
            } else {
                const bool lowerUpperMatch = (lowerInClip == upperInClip);
                if (lowerUpperMatch && time == map1.externalTime) {
                    translated.reset(map1.externalTime);
                } else if (lowerUpperMatch && time == map2.externalTime) {
                    translated.reset(map2.externalTime);
                } else {
                    if (translatingLower) {
                        translated.reset(map1.externalTime);
                    } else {
                        translated.reset(map2.externalTime);
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
            translatedLower.reset(times->front().externalTime);
        }
        else if (lowerInClip > times->back().internalTime) {
            translatedLower.reset(times->back().externalTime);
        }

        if (upperInClip < times->front().internalTime) {
            translatedUpper.reset(times->front().externalTime);
        }
        else if (upperInClip > times->back().internalTime) {
            translatedUpper.reset(times->back().externalTime);
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
        _GetLayerForClip()->ListTimeSamplesForPath(_TranslatePathToClip(path));
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
    return _GetLayerForClip()->HasField(_TranslatePathToClip(path), field);
}

bool
Usd_Clip::HasAuthoredTimeSamples(const SdfPath& path) const
{
    return _GetLayerForClip()->GetNumTimeSamplesForPath(
        _TranslatePathToClip(path)) > 0;    
}

bool
Usd_Clip::IsBlocked(const SdfPath& path, ExternalTime time) const
{
    SdfAbstractDataTypedValue<SdfValueBlock> blockValue(nullptr);
    if (_GetLayerForClip()->QueryTimeSample(
            path, _TranslateTimeToInternal(time), 
            (SdfAbstractDataValue*)&blockValue)
        && blockValue.isValueBlock) {
        return true;
    }
    return false;
}

SdfPath
Usd_Clip::_TranslatePathToClip(const SdfPath& path) const
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
Usd_Clip::_TranslateTimeToInternal(ExternalTime extTime) const
{
    size_t i1, i2;
    if (!_GetBracketingTimeSegment(*times, extTime, &i1, &i2)) {
        return extTime;
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
    if (m2.isJumpDiscontinuity) {
        TF_VERIFY(i2 + 1 < times->size());
        const TimeMapping& m3 = (*times)[i2 + 1];
        return _TranslateTimeToInternalHelper(
            extTime, m1, TimeMapping(m3.externalTime, m2.internalTime));
    }

    return _TranslateTimeToInternalHelper(extTime, m1, m2);
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

SdfPropertySpecHandle
Usd_Clip::GetPropertyAtPath(const SdfPath &path) const
{
    return _GetLayerForClip()->GetPropertyAtPath(_TranslatePathToClip(path));
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

    if (TF_VERIFY(sourceLayerIndex < sourceLayerStack->GetLayers().size())) {
        const ArResolverContextBinder binder(
            sourceLayerStack->GetIdentifier().pathResolverContext);
        layer = SdfLayer::FindOrOpenRelativeToLayer(
            sourceLayerStack->GetLayers()[sourceLayerIndex],
            assetPath.GetAssetPath());
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
                     UsdUsdaFileFormatTokens->Id.GetText()));
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

// SdfTimeCode values from clips need to be converted from internal time to
// external time. We treat time code values as relative to the internal time
// to convert to external.
inline
void 
_ConvertValueForTime(const Usd_Clip::ExternalTime &extTime, 
                     const Usd_Clip::InternalTime &intTime,
                     SdfTimeCode *value)
{
    *value = *value + (extTime - intTime);
}

// Similarly we convert arrays of SdfTimeCodes.
inline
void 
_ConvertValueForTime(const Usd_Clip::ExternalTime &extTime, 
                     const Usd_Clip::InternalTime &intTime,
                     VtArray<SdfTimeCode> *value)
{
    for (size_t i = 0; i < value->size(); ++i) {
        _ConvertValueForTime(extTime, intTime, &(*value)[i]);
    }
}

// Helpers for accessing the typed value from type erased values, needed for
// converting SdfTimeCodes.
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

// For type erased values, we need to convert them if they hold SdfTimeCode 
// based types.
template <class Storage>
inline
void
_ConvertTypeErasedValueForTime(const Usd_Clip::ExternalTime &extTime, 
                               const Usd_Clip::InternalTime &intTime,
                               Storage *value)
{
    if (_IsHolding<SdfTimeCode>(*value)) {
        SdfTimeCode rawVal;
        _UncheckedSwap(value, rawVal);
        _ConvertValueForTime(extTime, intTime, &rawVal);
        _UncheckedSwap(value, rawVal);
    } else if (_IsHolding<VtArray<SdfTimeCode>>(*value)) {
        VtArray<SdfTimeCode> rawVal;
        _UncheckedSwap(value, rawVal);
        _ConvertValueForTime(extTime, intTime, &rawVal);
        _UncheckedSwap(value, rawVal);
    }
}

void 
_ConvertValueForTime(const Usd_Clip::ExternalTime &extTime, 
                     const Usd_Clip::InternalTime &intTime,
                     VtValue *value)
{
    _ConvertTypeErasedValueForTime(extTime, intTime, value);
}

void 
_ConvertValueForTime(const Usd_Clip::ExternalTime &extTime, 
                     const Usd_Clip::InternalTime &intTime,
                     SdfAbstractDataValue *value)
{
    _ConvertTypeErasedValueForTime(extTime, intTime, value);
}

// Fallback no-op default for the rest of the value types; there is no time 
// conversion necessary for non-timecode types.
template <class T>
inline
void _ConvertValueForTime(const Usd_Clip::ExternalTime &extTime, 
                          const Usd_Clip::InternalTime &intTime,
                          T *value)
{
}

template <class T>
static bool
_Interpolate(
    const SdfLayerRefPtr& clip, const SdfPath &clipPath,
    Usd_Clip::InternalTime clipTime, Usd_InterpolatorBase* interpolator,
    T* value)
{
    double lowerInClip, upperInClip;
    if (clip->GetBracketingTimeSamplesForPath(
            clipPath, clipTime, &lowerInClip, &upperInClip)) {
            
        return Usd_GetOrInterpolateValue(
            clip, clipPath, clipTime, lowerInClip, upperInClip,
            interpolator, value);
    }

    return false;
}

}; // End anonymous namespace

template <class T>
bool 
Usd_Clip::QueryTimeSample(
    const SdfPath& path, ExternalTime time, 
    Usd_InterpolatorBase* interpolator, T* value) const
{
    const SdfPath clipPath = _TranslatePathToClip(path);
    const InternalTime clipTime = _TranslateTimeToInternal(time);
    const SdfLayerRefPtr& clip = _GetLayerForClip();

    if (!clip->QueryTimeSample(clipPath, clipTime, value)) {
        // See comment in Usd_Clip::GetBracketingTimeSamples.
        if (!_Interpolate(clip, clipPath, clipTime, interpolator, value)) {
            return false;
        }
    }

    // Convert values containing SdfTimeCodes if necessary.
    _ConvertValueForTime(time, clipTime, value);
    return true;
}

#define _INSTANTIATE_QUERY_TIME_SAMPLE(r, unused, elem)         \
    template bool Usd_Clip::QueryTimeSample(                    \
        const SdfPath&, Usd_Clip::ExternalTime,                 \
        Usd_InterpolatorBase*,                                  \
        SDF_VALUE_CPP_TYPE(elem)*) const;                       \
    template bool Usd_Clip::QueryTimeSample(                    \
        const SdfPath&, Usd_Clip::ExternalTime,                 \
        Usd_InterpolatorBase*,                                  \
        SDF_VALUE_CPP_ARRAY_TYPE(elem)*) const;

BOOST_PP_SEQ_FOR_EACH(_INSTANTIATE_QUERY_TIME_SAMPLE, ~, SDF_VALUE_TYPES)
#undef _INSTANTIATE_QUERY_TIME_SAMPLE

template bool Usd_Clip::QueryTimeSample(
    const SdfPath&, Usd_Clip::ExternalTime,
    Usd_InterpolatorBase*,
    SdfAbstractDataValue*) const;

template bool Usd_Clip::QueryTimeSample(
    const SdfPath&, Usd_Clip::ExternalTime,
    Usd_InterpolatorBase*,
    VtValue*) const;

PXR_NAMESPACE_CLOSE_SCOPE

