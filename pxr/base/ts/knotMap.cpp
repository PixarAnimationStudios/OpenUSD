//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/knotMap.h"
#include "pxr/base/ts/splineData.h"
#include "pxr/base/ts/knotData.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE


TsKnotMap::TsKnotMap() = default;
TsKnotMap::TsKnotMap(std::initializer_list<TsKnot> knots)
{
    for (const TsKnot &knot : knots)
        insert(knot);
}

TsKnotMap::TsKnotMap(const Ts_SplineData* const data)
{
    // Determine value type.
    const TfType valueType = data->GetValueType();

    // Reserve storage for Knot objects.
    const size_t numKnots = data->times.size();
    _knots.reserve(numKnots);

    // Decide whether to do customData lookups.
    VtDictionary knotCustom;
    const bool haveCustom = !data->customData.empty();

    // Populate Knot objects.
    for (size_t i = 0; i < numKnots; i++)
    {
        if (haveCustom)
        {
            knotCustom = TfMapLookupByValue(
                data->customData, data->times[i], VtDictionary());
        }

        // This incurs a virtual method call per knot.  Could be improved, but
        // header dependencies make it not trivial.
        TsKnot knot(
            data->CloneKnotAtIndex(i),
            valueType,
            std::move(knotCustom));
        _knots.emplace_back(std::move(knot));
    }
}

bool TsKnotMap::operator==(const TsKnotMap &other) const
{
    return _knots == other._knots;
}

bool TsKnotMap::operator!=(const TsKnotMap &other) const
{
    return !(*this == other);
}

TsKnotMap::iterator
TsKnotMap::begin()
{
    return _knots.begin();
}

TsKnotMap::const_iterator
TsKnotMap::begin() const
{
    return _knots.begin();
}

TsKnotMap::const_iterator
TsKnotMap::cbegin() const
{
    return _knots.cbegin();
}

TsKnotMap::iterator
TsKnotMap::end()
{
    return _knots.end();
}

TsKnotMap::const_iterator
TsKnotMap::end() const
{
    return _knots.end();
}

TsKnotMap::const_iterator
TsKnotMap::cend() const
{
    return _knots.cend();
}

TsKnotMap::reverse_iterator
TsKnotMap::rbegin()
{
    return _knots.rbegin();
}

TsKnotMap::const_reverse_iterator
TsKnotMap::rbegin() const
{
    return _knots.rbegin();
}

TsKnotMap::const_reverse_iterator
TsKnotMap::crbegin() const
{
    return _knots.crbegin();
}

TsKnotMap::reverse_iterator
TsKnotMap::rend()
{
    return _knots.rend();
}

TsKnotMap::const_reverse_iterator
TsKnotMap::rend() const
{
    return _knots.rend();
}

TsKnotMap::const_reverse_iterator
TsKnotMap::crend() const
{
    return _knots.crend();
}

size_t TsKnotMap::size() const
{
    return _knots.size();
}

bool TsKnotMap::empty() const
{
    return _knots.empty();
}

void TsKnotMap::reserve(const size_t size)
{
    _knots.reserve(size);
}

void TsKnotMap::clear()
{
    _knots.clear();
}

void TsKnotMap::swap(TsKnotMap &other)
{
    _knots.swap(other._knots);
}

std::pair<TsKnotMap::iterator, bool>
TsKnotMap::insert(const TsKnot &knot)
{
    const iterator lbIt = lower_bound(knot.GetTime());
    if (lbIt != _knots.end() && lbIt->GetTime() == knot.GetTime())
    {
        return std::make_pair(lbIt, false);
    }

    const iterator insIt = _knots.insert(lbIt, knot);
    return std::make_pair(insIt, true);
}

size_t TsKnotMap::erase(const TsTime time)
{
    const iterator lbIt = lower_bound(time);
    if (lbIt == _knots.end() || lbIt->GetTime() != time)
    {
        return 0;
    }

    _knots.erase(lbIt);
    return 1;
}

TsKnotMap::iterator
TsKnotMap::erase(iterator i)
{
    return _knots.erase(i);
}

TsKnotMap::iterator
TsKnotMap::erase(iterator first, iterator last)
{
    return _knots.erase(first, last);
}

TsKnotMap::iterator
TsKnotMap::find(const TsTime time)
{
    const iterator lbIt = lower_bound(time);
    if (lbIt != _knots.end() && lbIt->GetTime() == time)
    {
        return lbIt;
    }

    return _knots.end();
}

TsKnotMap::const_iterator
TsKnotMap::find(const TsTime time) const
{
    const const_iterator lbIt = lower_bound(time);
    if (lbIt != _knots.end() && lbIt->GetTime() == time)
    {
        return lbIt;
    }

    return _knots.end();
}

TsKnotMap::iterator
TsKnotMap::lower_bound(const TsTime time)
{
    return std::partition_point(
        _knots.begin(), _knots.end(),
        [=](const TsKnot &knot)
        { return knot.GetTime() < time; });
}

TsKnotMap::const_iterator
TsKnotMap::lower_bound(const TsTime time) const
{
    return std::partition_point(
        _knots.begin(), _knots.end(),
        [=](const TsKnot &knot)
        { return knot.GetTime() < time; });
}

TsKnotMap::iterator
TsKnotMap::FindClosest(const TsTime time)
{
    const const_iterator it =
        const_cast<const TsKnotMap*>(this)->FindClosest(time);

    return _knots.begin() + (it - _knots.cbegin());
}

TsKnotMap::const_iterator
TsKnotMap::FindClosest(const TsTime time) const
{
    // Do we have any knots?
    if (_knots.empty())
    {
        return _knots.end();
    }

    // Find first knot at or after time.
    const const_iterator lbIt = lower_bound(time);

    // If time is before first knot, return first knot.
    if (lbIt == _knots.begin())
    {
        return lbIt;
    }

    // If time is after last knot, return last knot.
    if (lbIt == _knots.end())
    {
        return lbIt - 1;
    }

    // Return exact matches.
    if (lbIt->GetTime() == time)
    {
        return lbIt;
    }

    // Between knots.  Compare distances.  Ties go to the later knot.
    const const_iterator prevIt = lbIt - 1;
    const TsTime prevGap = time - prevIt->GetTime();
    const TsTime nextGap = lbIt->GetTime() - time;
    return (nextGap > prevGap ? prevIt : lbIt);
}

TfType TsKnotMap::GetValueType() const
{
    if (_knots.empty())
    {
        return TfType();
    }

    return _knots[0].GetValueType();
}

GfInterval TsKnotMap::GetTimeSpan() const
{
    // No knots -> empty interval.
    if (_knots.empty())
    {
        return GfInterval();
    }

    // From first to last time.  Closed at both ends.  If there's only one knot,
    // both times will be the same, giving a zero-width but non-empty interval.
    return GfInterval(_knots.begin()->GetTime(), _knots.rbegin()->GetTime());
}

bool TsKnotMap::HasCurveSegments() const
{
    for (size_t i = 0, end = _knots.size(); i + 1 < end; i++)
    {
        if (_knots[i].GetNextInterpolation() == TsInterpCurve)
        {
            return true;
        }
    }

    return false;
}


PXR_NAMESPACE_CLOSE_SCOPE
