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
#ifndef GF_INTERVAL_SET_H
#define GF_INTERVAL_SET_H

/// \file gf/multiInterval.h
/// \ingroup group_gf_BasicMath

#include "pxr/base/gf/interval.h"
#include "pxr/base/gf/api.h"

#include <iosfwd>
#include <set>
#include <vector>

/// \class GfMultiInterval
/// \ingroup group_gf_BasicMath
///
/// GfMultiInterval represents a subset of the real number line as an
/// ordered set of non-intersecting GfIntervals.
///
class GfMultiInterval
{
public:
    typedef std::set<GfInterval> Set;
    typedef Set::const_iterator const_iterator;
    typedef Set::const_iterator iterator;

    /// \name Constructors
    /// @{
    /// Constructs an empty multi-interval.
	GF_API GfMultiInterval() {}
    /// Constructs an multi-interval by copying the given set.
	GF_API GfMultiInterval(const GfMultiInterval &s) : _set(s._set) {}
    /// Constructs an multi-interval with the single given interval.
	GF_API explicit GfMultiInterval(const GfInterval &i);
    /// Constructs an multi-interval containing the given input intervals.
	GF_API explicit GfMultiInterval(const std::vector<GfInterval> &intervals);
    /// @}

	GF_API bool operator==(const GfMultiInterval &that) const { return _set == that._set; }
	GF_API bool operator!=(const GfMultiInterval &that) const { return !(*this == that); }
	GF_API bool operator<(const GfMultiInterval &that) const { return _set < that._set; }
	GF_API bool operator>=(const GfMultiInterval &that) const { return !(*this < that); }
	GF_API bool operator>(const GfMultiInterval &that) const { return (that < *this); }
	GF_API bool operator<=(const GfMultiInterval &that) const { return !(that < *this); }
    

    /// Hash value.
    /// Just a basic hash function, not particularly high quality.
	GF_API size_t Hash() const;

    friend inline size_t hash_value(const GfMultiInterval &mi) {
        return mi.Hash();
    }

    /// \name Accessors
    /// @{

    /// Returns true if the multi-interval is empty.
	GF_API bool IsEmpty() const { return _set.empty(); }

    /// Returns the number of intervals in the set.
	GF_API size_t GetSize() const { return _set.size(); }

    /// Returns an interval bounding the entire multi-interval.
    /// Returns an empty interval if the multi-interval is empty.
	GF_API GfInterval GetBounds() const;

    /// Returns true if the multi-interval contains the given value.
	GF_API bool Contains(double d) const;

    /// Returns true if the multi-interval contains the given interval.
	GF_API bool Contains(const GfInterval & i) const;

    /// Returns true if the multi-interval contains all the intervals in the 
    /// given multi-interval.
	GF_API bool Contains(const GfMultiInterval & s) const;

    /// @}

    /// \name Mutation
    /// @{

    /// Clear the multi-interval.
	GF_API void Clear() { _set.clear(); }

    /// Add the given interval to the multi-interval.
	GF_API void Add( const GfInterval & i );
    /// Add the given multi-interval to the multi-interval.
    /// Sets this object to the union of the two sets.
	GF_API void Add( const GfMultiInterval &s );

    /// Uses the given interval to extend the multi-interval in
    /// the interval arithmetic sense.
	GF_API void ArithmeticAdd( const GfInterval &i );

    /// Remove the given interval from this multi-interval.
	GF_API void Remove( const GfInterval & i );
    /// Remove the given multi-interval from this multi-interval.
	GF_API void Remove( const GfMultiInterval &s );

	GF_API void Intersect( const GfInterval & i );
	GF_API void Intersect( const GfMultiInterval &s );

    /// Return the complement of this set.
	GF_API GfMultiInterval GetComplement() const;

    /// @}

    /// \name Iteration
    /// Only const iterators are returned.  To maintain the invariants of
    /// the multi-interval, changes must be made via the public mutation API.
    /// @{

	GF_API const_iterator begin() const { return _set.begin(); }
	GF_API const_iterator end() const { return _set.end(); }

    /// Returns an iterator identifying the first (lowest) interval whose
    /// minimum value is >= x.  If no such interval exists, returns end().
	GF_API const_iterator lower_bound( double x ) const;

    /// Returns an iterator identifying the first (lowest) interval whose
    /// minimum value is > x.  If no such interval exists, returns end().
	GF_API const_iterator upper_bound( double x ) const;

    /// Returns an iterator identifying the first (loest) interval whose
    /// minimum value is > x.  If no such interval exists, returns end().
	GF_API const_iterator GetNextNonContainingInterval( double x ) const;

    /// Returns an iterator identifying the last (highest) interval whose
    /// maximum value is < x.  If no such interval exists, returns end().
	GF_API const_iterator GetPriorNonContainingInterval( double x ) const;

    /// Returns an iterator identifying the interval that contains x.  If
    /// no interval contains x, then it returns end()
	GF_API const_iterator GetContainingInterval( double x ) const;

    /// @}

    /// Returns the full interval (-inf, inf).
    static GfMultiInterval GetFullInterval() {
        return GfMultiInterval(GfInterval::GetFullInterval());
    }

    /// Swap two multi-intervals
    void swap(GfMultiInterval &other) { _set.swap(other._set); }

private:
    void _AssertInvariants() const;

    Set _set;
};

/// Output a GfMultiInterval
/// \ingroup group_gf_DebuggingOutput
GF_API std::ostream & operator<<(std::ostream &out, const GfMultiInterval &s);

#endif
