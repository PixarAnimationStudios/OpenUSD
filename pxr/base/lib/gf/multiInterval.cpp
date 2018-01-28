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
#include "pxr/base/gf/multiInterval.h"
#include "pxr/base/gf/ostreamHelpers.h"
#include "pxr/base/tf/tf.h"
#include "pxr/base/tf/type.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<GfMultiInterval>();
}

GfMultiInterval::GfMultiInterval(const GfInterval &i)
{
    Add(i);
}

GfMultiInterval::GfMultiInterval(const std::vector<GfInterval> &intervals)
{
    for (const auto& i : intervals) {
        Add(i);
    }
}

size_t
GfMultiInterval::Hash() const
{
    size_t h = 0;
    for (auto const &i: _set)
        boost::hash_combine(h, i);
    return h;
}

GfInterval
GfMultiInterval::GetBounds() const
{
    if (!_set.empty()) {
        const GfInterval & first = *_set.begin();
        const GfInterval & last = *_set.rbegin();
        return GfInterval(first.GetMin(), last.GetMax(),
                          first.IsMinClosed(), last.IsMaxClosed());
    } else {
        return GfInterval();
    }
}

bool
GfMultiInterval::Contains(double d) const
{
    // Find position of first interval >= [d,d].
    const_iterator i = lower_bound(d);

    // Case 1: i is the first interval after d.
    if (i != end() && i->Contains(d))
        return true;

    // Case 2: the previous interval, (i-1), contains d.
    if (i != begin() && (--i)->Contains(d))
        return true;

    return false;
}

bool
GfMultiInterval::Contains(const GfInterval & a) const
{
    if (a.IsEmpty())
        return false;

    // Find position of first interval >= a.
    const_iterator i = _set.lower_bound(a);

    // Case 1: i is the first interval at-or-after a and contains a.
    if (i != end() && i->Contains(a))
        return true;

    // Case 2: the previous interval, (i-1), contains a.
    if (i != begin() && (--i)->Contains(a))
        return true;

    return false;
}

bool
GfMultiInterval::Contains(const GfMultiInterval & s) const
{
    if (s.IsEmpty()) {
        return false;
    }
    for (const auto& i : s) {
        if (!Contains(i)) {
            return false;
        }
    }
    return true;
}

GfMultiInterval::const_iterator
GfMultiInterval::lower_bound( double x ) const
{
    return _set.lower_bound( GfInterval(x) );
}

GfMultiInterval::const_iterator
GfMultiInterval::upper_bound( double x ) const
{
    return _set.upper_bound( GfInterval(x) );
}

GfMultiInterval::const_iterator
GfMultiInterval::GetNextNonContainingInterval( double x ) const
{
    // We pass the partially open interval (x,x] to the upper_bound
    // function instead of the closed interval [x,x] because of how the
    // less than operator behaves on intervals with the same minimum
    // value. In the case where the multi-interval contains an interval
    // with a closed min of x such as [x,x+1], upper_bound([x,x]) would
    // return [x, x+1] while upper_bound( (x,x] ) would return the
    // interval afterwards.  The latter behavior is what we want because
    // [x,x+1] contains x.
    return _set.upper_bound( GfInterval(x, x, false, true) );
}

GfMultiInterval::const_iterator
GfMultiInterval::GetPriorNonContainingInterval( double x ) const
{
    const_iterator i = _set.lower_bound(x);

    if (i == _set.begin()) {
        // No interval before x.
        return _set.end();
    }
    --i;
    if (!i->Contains(x)) {
        // Found prior non-overlapping interval.
        return i;
    }
    if (i != _set.begin()) {
        // Return prior interval, which we know does not contain x
        // because intervals in a set never overlap.
        --i;
        TF_AXIOM(!i->Contains(x));
        return i;
    } else {
        // No prior intervals left.
        return _set.end();
    }
}

GfMultiInterval::const_iterator
GfMultiInterval::GetContainingInterval( double x ) const
{   
    // Get the next interval that doesn't contain x
    const_iterator i = GetNextNonContainingInterval(x);

    // There are no intervals before the next non-containing interval so the
    // multi-interval does not contain x
    if (i == _set.begin()) {
        return _set.end();
    }

    // The previous interval is the one that would contain x if the interval 
    // set contains x so return it if it does
    --i;
    if (i->Contains(x)) {
        return i;
    }
    return _set.end();
}

void
GfMultiInterval::Add( const GfMultiInterval &intervals )
{
    for (const auto& i : intervals) {
        Add(i);
    }
}

void
GfMultiInterval::Add( const GfInterval & interval )
{
    if (interval.IsEmpty())
        return;

    // Merge  interval with neighboring overlapping intervals.
    GfInterval merged = interval;
    const_iterator i = _set.lower_bound(merged);

    // Merge with subsequent intervals.
    while (i != _set.end() && merged.Intersects(*i)) {
        merged |= *i;
        _set.erase(i++);
    }
    // Intersection doesn't account for when the min of the next interval 
    // matches the max of the merged interval if either of these intervals 
    // has that boundary open.  So we need to merge in the next interval if our
    // end points match up and either of the these end points is closed 
    if (i != _set.end() && merged.GetMax() == (*i).GetMin() &&
        !(merged.IsMaxOpen() && (*i).IsMinOpen()) ) {
        merged |= *i;
        _set.erase(i++);
    }
    // Merge with the preceding interval.
    if (i != _set.begin()) {
        --i;
        // If our merged interval intersects the previous interval or it's min
        // matches the previous interval's max with either one of the ends being
        // close, then merged the previous interval in
        if (merged.Intersects(*i) || 
                (merged.GetMin() == (*i).GetMax() && 
                 !(merged.IsMinOpen() && (*i).IsMaxOpen()))) {
            merged |= *i;
            _set.erase(i);

        }
    }

    // Insert final merged result
    _set.insert(merged);

    if (TF_DEV_BUILD)
        _AssertInvariants();
}

void
GfMultiInterval::Remove( const GfMultiInterval &intervals )
{
    for (const auto& i : intervals) {
        Remove(i);
    }
}

// Remove interval j from interval at iterator i, inserting new intervals
// as necessary.
static void
_RemoveInterval( const GfMultiInterval::Set::iterator i, const GfInterval & j,
                 GfMultiInterval::Set *set )
{
    if (!i->Intersects(j))
        return;

    GfInterval lo( i->GetMin(),         j.GetMin(),
                   i->IsMinClosed(),    !j.IsMinClosed() );
    GfInterval hi( j.GetMax(),          i->GetMax(),
                   !j.IsMaxClosed(), i->IsMaxClosed() );

    if (!lo.IsEmpty())
        set->insert(/* hint */ i, lo);
    if (!hi.IsEmpty())
        set->insert(/* hint */ i, hi);

    set->erase(i);
}

void
GfMultiInterval::Remove( const GfInterval & intervalToRemove )
{
    if (intervalToRemove.IsEmpty())
        return;

    Set::iterator i;

    // Remove from subsequent intersecting intervals.
    i = _set.lower_bound(intervalToRemove);
    while (i != _set.end() && intervalToRemove.Intersects(*i))
        _RemoveInterval( i++, intervalToRemove, &_set );

    // Remove from prior interval.  Need to handle at most one.
    i = _set.upper_bound(intervalToRemove);
    if (i != _set.begin())
        _RemoveInterval( --i, intervalToRemove, &_set );

    if (TF_DEV_BUILD)
        _AssertInvariants();
}

GfMultiInterval
GfMultiInterval::GetComplement() const
{
    GfMultiInterval r;
    GfInterval workingInterval = GfInterval::GetFullInterval();
    for (const auto& i : _set) {
        // Insert interval prior to *i.
        workingInterval.SetMax( i.GetMin(), !i.IsMinClosed() );
        if (!workingInterval.IsEmpty()) {
            r._set.insert(/* hint */ r._set.end(), workingInterval);
        }

        // Set up next interval.
        workingInterval = GfInterval::GetFullInterval();
        workingInterval.SetMin( i.GetMax(), !i.IsMaxClosed() );
    }
    if (!workingInterval.IsEmpty()) {
        r._set.insert(/* hint */ r._set.end(), workingInterval);
    }
    return r;
}

void
GfMultiInterval::Intersect( const GfMultiInterval &intervals )
{
    Remove(intervals.GetComplement());
}

void
GfMultiInterval::Intersect( const GfInterval & i )
{
    Intersect( GfMultiInterval(i) );
}

void
GfMultiInterval::_AssertInvariants() const
{
    const GfInterval *last = 0;

    for (const_iterator i = _set.begin(), iEnd = _set.end(); i != iEnd; ++i)
    {
        // Verify no empty intervals
        TF_AXIOM(!i->IsEmpty());

        // Verify that intervals increase monotonically and do not overlap
        if (last) {
            TF_AXIOM( *last < *i );
            TF_AXIOM( !last->Intersects(*i) );
        }

        last = &(*i);
    }
}

void
GfMultiInterval::ArithmeticAdd( const GfInterval &i )
{
    GfMultiInterval result;
    for (const auto& interval : *this) {
        result.Add(interval + i);
    }

    swap(result);
}

std::ostream &
operator<<(std::ostream &out, const GfMultiInterval &s)
{
    out << "[";
    bool first = true;
    for (const auto& interval : s) {
        if (!first) {
            out << ", ";
        }
        out << Gf_OstreamHelperP(interval);
        first = false;
    }
    out << "]";
    return out;
}

PXR_NAMESPACE_CLOSE_SCOPE
