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
#ifndef PXR_BASE_GF_INTERVAL_H
#define PXR_BASE_GF_INTERVAL_H

/// \file gf/interval.h
/// \ingroup group_gf_BasicMath

#include "pxr/pxr.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/gf/api.h" 

#include <boost/functional/hash.hpp>

#include <float.h>
#include <iosfwd>
#include <limits>

PXR_NAMESPACE_OPEN_SCOPE

/// \class GfInterval
/// \ingroup group_gf_BasicMath
///
/// A basic mathematical interval class.
///
/// Can represent intervals with either open or closed boundary
/// conditions.
///
class GfInterval
{
public:
    /// \name Constructors
    ///@{

    /// Construct an empty open interval, (0,0).
    GfInterval() :
        _min(0.0, false),
        _max(0.0, false)
    {}

    /// Construct a closed interval representing the single point, as [val,val].
    GfInterval(double val) :
        _min(val, true),
        _max(val, true)
    {}
    
    /// Construct an interval with the given arguments.
    GfInterval(double min, double max,
               bool minClosed=true, bool maxClosed=true) :
        _min(min, minClosed),
        _max(max, maxClosed)
    {}

    ///@}

    /// Equality operator.
    bool operator==(const GfInterval &rhs) const {
        return _min == rhs._min && _max == rhs._max;
    }

    /// Inequality operator.
    bool operator!=(const GfInterval &rhs) const {
        return !(*this == rhs);
    }

    /// Less-than operator.
    bool operator<(const GfInterval &rhs) const {
        // Compare min bound
        if (_min != rhs._min)
            return _min < rhs._min;

        // Compare max bound
        if (_max != rhs._max)
            return _max < rhs._max;

        // Equal
        return false;
    }

    /// Hash value.
    /// Just a basic hash function, not particularly high quality.
    size_t Hash() const { return hash_value(*this); }

    friend inline size_t hash_value(GfInterval const &i) {
        size_t h = 0;
        boost::hash_combine(h, i._min);
        boost::hash_combine(h, i._max);
        return h;
    }

    /// Minimum value
    double GetMin() const { return _min.value; }

    /// Maximum value
    double GetMax() const { return _max.value; }

    /// Set minimum value
    void SetMin(double v) { 
        _min = _Bound(v, _min.closed);
    }

    /// Set minimum value and boundary condition
    void SetMin(double v, bool minClosed ) {
        _min = _Bound(v, minClosed);
    }

    /// Set maximum value
    void SetMax(double v) { 
        _max = _Bound(v, _max.closed);
    }

    /// Set maximum value and boundary condition
    void SetMax(double v, bool maxClosed ) {
        _max = _Bound(v, maxClosed);
    }

    /// Minimum boundary condition
    bool IsMinClosed() const { return _min.closed; }

    /// Maximum boundary condition
    bool IsMaxClosed() const { return _max.closed; }

    /// Minimum boundary condition
    bool IsMinOpen() const { return ! _min.closed; }

    /// Maximum boundary condition
    bool IsMaxOpen() const { return ! _max.closed; }

    /// Returns true if the maximum value is finite.
    bool IsMaxFinite() const {
        return (_max.value != -std::numeric_limits<double>::infinity()
            && _max.value !=  std::numeric_limits<double>::infinity());
    }

    /// Returns true if the minimum value is finite.
    bool IsMinFinite() const {
        return (_min.value != -std::numeric_limits<double>::infinity()
            && _min.value !=  std::numeric_limits<double>::infinity());
    }

    /// Returns true if both the maximum and minimum value are finite.
    bool IsFinite() const {
        return IsMaxFinite() && IsMinFinite();
    }

    /// Return true iff the interval is empty.
    bool IsEmpty() const {
        return (_min.value > _max.value) ||
            ((_min.value == _max.value)
             && (! _min.closed || !_max.closed));
    }

    /// Width of the interval.
    /// An empty interval has size 0.
    double GetSize() const {
        return GfMax( 0.0, _max.value - _min.value );
    }

    // For 2x compatibility
    double Size() const { return GetSize(); }

    /// Return true iff the value d is contained in the interval.
    /// An empty interval contains no values.
    bool Contains(double d) const {
        return ((d > _min.value) || (d == _min.value && _min.closed))
           &&  ((d < _max.value) || (d == _max.value && _max.closed));
    }

    // For 2x compatibility
    bool In(double d) const { return Contains(d); }

    /// Return true iff the interval i is entirely contained in the interval.
    /// An empty interval contains no intervals, not even other
    /// empty intervals.
    bool Contains(const GfInterval &i) const {
        return (*this & i) == i;
    }

    /// Return true iff the given interval i intersects this interval.
    bool Intersects(const GfInterval &i) const {
        return !(*this & i).IsEmpty();
    }

    /// \name Math operations
    ///@{

    /// Boolean intersection.
    GfInterval & operator&=(const GfInterval &rhs) {
        if (IsEmpty()) {
            // No change
        } else if (rhs.IsEmpty()) {
            // Intersection is empty
            *this = GfInterval();
        } else {
            // Intersect min edge
            if (_min.value < rhs._min.value)
                _min = rhs._min;
            else if (_min.value == rhs._min.value)
                _min.closed &= rhs._min.closed;

            // Intersect max edge
            if (_max.value > rhs._max.value)
                _max = rhs._max;
            else if (_max.value == rhs._max.value)
                _max.closed &= rhs._max.closed;
        }
        return *this;
    }
    
    /// Returns the interval that bounds the union of this interval and rhs.
    GfInterval & operator|=(const GfInterval &rhs) {
        if (IsEmpty()) {
            *this = rhs;
        } else if (rhs.IsEmpty()) {
            // No change
        } else {
            // Expand min edge
            if (_min.value > rhs._min.value)
                _min = rhs._min;
            else if (_min.value == rhs._min.value)
                _min.closed |= rhs._min.closed;

            // Expand max edge
            if (_max.value < rhs._max.value)
                _max = rhs._max;
            else if (_max.value == rhs._max.value)
                _max.closed |= rhs._max.closed;
        }
        return *this;
    }

    /// Interval addition.
    GfInterval & operator+=(const GfInterval &rhs) {
        if (!rhs.IsEmpty()) {
            _min.value += rhs._min.value;
            _max.value += rhs._max.value;
            _min.closed &= rhs._min.closed;
            _max.closed &= rhs._max.closed;
        }
        return *this;
    }
    
    /// Interval subtraction.
    GfInterval & operator-=(const GfInterval &rhs) {
        return *this += -rhs;
    }
    
    /// Interval unary minus.
    GfInterval operator-() const {
        return GfInterval(-_max.value, -_min.value, _max.closed, _min.closed);
    }
    
    /// Interval multiplication.
    GfInterval & operator*=(const GfInterval &rhs) {
        const _Bound a = _min * rhs._min;
        const _Bound b = _min * rhs._max;
        const _Bound c = _max * rhs._min;
        const _Bound d = _max * rhs._max;
        _max = _Max( _Max(a,b), _Max(c,d) );
        _min = _Min( _Min(a,b), _Min(c,d) );
        return *this;
    }

    /// Greater than operator
    bool operator>(const GfInterval& rhs) {
        // Defined in terms of operator<()
        return rhs < *this;
    }

    /// Less than or equal operator
    bool operator<=(const GfInterval& rhs) {
        // Defined in terms of operator<()
        return !(rhs < *this);
    }

    /// Greater than or equal operator
    bool operator>=(const GfInterval& rhs) {
        // Defined in terms of operator<()
        return !(*this < rhs);
    }

    /// Union operator
    GfInterval operator|(const GfInterval& rhs) const {
        // Defined in terms of operator |=()
        GfInterval tmp(*this);
        tmp |= rhs;
        return tmp;
    }

    /// Intersection operator
    GfInterval operator&(const GfInterval& rhs) const {
        // Defined in terms of operator &=()
        GfInterval tmp(*this);
        tmp &= rhs;
        return tmp;
    }

    /// Addition operator
    GfInterval operator+(const GfInterval& rhs) const {
        // Defined in terms of operator +=()
        GfInterval tmp(*this);
        tmp += rhs;
        return tmp;
    }

    /// Subtraction operator
    GfInterval operator-(const GfInterval& rhs) const {
        // Defined in terms of operator -=()
        GfInterval tmp(*this);
        tmp -= rhs;
        return tmp;
    }

    /// Multiplication operator
    GfInterval operator*(const GfInterval& rhs) const {
        // Defined in terms of operator *=()
        GfInterval tmp(*this);
        tmp *= rhs;
        return tmp;
    }

    ///@}

    /// Returns the full interval (-inf, inf).
    static GfInterval GetFullInterval() {
        return GfInterval( -std::numeric_limits<double>::infinity(),
                            std::numeric_limits<double>::infinity(),
                            false, false );
    }

private:
    // Helper struct to represent interval boundaries.
    struct _Bound {
        // Boundary value.
        double value;
        // Boundary condition.  The boundary value is included in interval
        // only if the boundary is closed.
        bool closed;

        _Bound(double val, bool isClosed) :
            value(val),
            closed(isClosed)
        {
            // Closed boundaries on infinite values do not make sense so
            // force the bound to be open
            if (value == -std::numeric_limits<double>::infinity() ||
                value == std::numeric_limits<double>::infinity()) {
                closed = false;
            }
        }

        bool operator==(const _Bound &rhs) const {
            return value == rhs.value && closed == rhs.closed;
        }

        bool operator!=(const _Bound &rhs) const {
            return !(*this == rhs);
        }

        bool operator<(const _Bound &rhs) const {
            return value < rhs.value || (value == rhs.value && closed && !rhs.closed);
        }

        _Bound & operator=(const _Bound &rhs) {
            value  = rhs.value;
            closed = rhs.closed;
            return *this;
        }
        _Bound operator*(const _Bound &rhs) const {
            return _Bound( value * rhs.value, closed & rhs.closed );
        }
        friend inline size_t hash_value(const _Bound &b) {
            size_t h = 0;
            boost::hash_combine(h, b.value);
            boost::hash_combine(h, b.closed);
            return h;
        }
    };

    // Return the lesser minimum bound, handling boundary conditions.
    inline static const _Bound &
    _Min( const _Bound &a, const _Bound &b ) {
        return (a.value < b.value
            || ((a.value == b.value) && a.closed && !b.closed)) ?
            a : b;
    }

    // Return the greater maximum bound, handling boundary conditions.
    inline static const _Bound &
    _Max( const _Bound &a, const _Bound &b ) {
        return (a.value < b.value
            || ((a.value == b.value) && !a.closed && b.closed)) ?
            b : a;
    }

    /// Data
    _Bound _min, _max;
};

/// Output a GfInterval using the format (x, y).
/// \ingroup group_gf_DebuggingOutput
GF_API std::ostream &operator<<(std::ostream&, const GfInterval&);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_GF_INTERVAL_H 
