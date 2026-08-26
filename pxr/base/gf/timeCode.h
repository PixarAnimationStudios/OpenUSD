//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_GF_TIME_CODE_H
#define PXR_BASE_GF_TIME_CODE_H

/// \file gf/timeCode.h

#include "pxr/pxr.h"
#include "pxr/base/gf/api.h"
#include "pxr/base/gf/duration.h"

#include <algorithm>
#include <functional>
#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE

/// \class GfTimeCode
///
/// Value type that represents a time code. It's equivalent to a double type
/// value but is used to indicate that this value should be resolved by any
/// time based value resolution.
///
class GfTimeCode
{
public:
    /// \name Constructors
    /// @{
    ///

    /// Construct a time code with the given time.
    ///
    /// A default constructed GfTimeCode has a time of 0.0.
    /// A double value can implicitly cast to GfTimeCode.
    constexpr GfTimeCode(double time = 0.0) noexcept
    : _time(time)
    {};

    /// Explicitly construct a time code from a duration.
    ///
    /// This is equivalent to (GfTimeCode(0) + duration).
    explicit constexpr GfTimeCode(GfDuration duration) noexcept
    : GfTimeCode(duration.GetValue())
    {}

    /// @}

    ///\name Operators
    /// @{

    constexpr bool operator==(const GfTimeCode &rhs) const noexcept
        { return _time == rhs._time; }
    constexpr bool operator!=(const GfTimeCode &rhs) const noexcept
        { return _time != rhs._time; }
    constexpr bool operator<(const GfTimeCode &rhs) const noexcept
        { return _time < rhs._time; }
    constexpr bool operator>(const GfTimeCode &rhs) const noexcept
        { return _time > rhs._time; }
    constexpr bool operator<=(const GfTimeCode &rhs) const noexcept
        { return _time <= rhs._time; }
    constexpr bool operator>=(const GfTimeCode &rhs) const noexcept
        { return _time >= rhs._time; }

    /// Scale time code values by doubles
    /// @{
    friend constexpr
    GfTimeCode operator*(const GfTimeCode& t,
                         const double amount) noexcept
        { return GfTimeCode(t._time * amount); }
    friend constexpr
    GfTimeCode operator*(const double amount,
                         const GfTimeCode& t) noexcept
        { return GfTimeCode(amount * t._time); }
    friend constexpr
    GfTimeCode operator/(const GfTimeCode& t,
                         const double amount) noexcept
        { return GfTimeCode(t._time / amount); }

    /// @}
    ///
    /// Ratio of two time codes is a unitless double scale factor
    constexpr double operator/(const GfTimeCode &rhs) const noexcept
        { return _time / rhs._time; }

    /// Sum time codes.
    ///
    /// This is questionable mathematically but it is needed to conveniently
    /// compute an average time code, like say a midpoint time.
    friend constexpr
    GfTimeCode operator+(const GfTimeCode& lhs,
                         const GfTimeCode& rhs) noexcept
        { return GfTimeCode(lhs._time + rhs._time); }

    /// Compute a GfDuration offset between two time codes.
    constexpr GfDuration operator-(const GfTimeCode &rhs) const noexcept
        { return GfDuration(_time - rhs._time); }

    /// Offset a time code value.
    friend constexpr
    GfTimeCode operator+(const GfTimeCode& lhs,
                         const GfDuration& rhs) noexcept
        { return GfTimeCode(lhs._time + rhs.GetValue()); }
    /// \overload
    friend constexpr
    GfTimeCode operator+(const GfDuration& lhs,
                         const GfTimeCode& rhs) noexcept
        { return GfTimeCode(lhs.GetValue() + rhs._time); }
    /// \overload
    friend constexpr
    GfTimeCode operator+(const GfTimeCode& lhs,
                         const double rhs) noexcept
        { return GfTimeCode(lhs._time + rhs); }
    /// \overload
    friend constexpr
    GfTimeCode operator+(const double lhs,
                         const GfTimeCode& rhs) noexcept
        { return GfTimeCode(lhs + rhs._time); }

    /// Offset a time code value.
    friend constexpr
    GfTimeCode operator-(const GfTimeCode& lhs,
                         const GfDuration& rhs) noexcept
        { return GfTimeCode(lhs._time - rhs.GetValue()); }
    /// \overload
    friend constexpr
    GfTimeCode operator-(const GfDuration& lhs,
                         const GfTimeCode& rhs) noexcept
        { return GfTimeCode(lhs.GetValue() - rhs._time); }
    /// \overload
    friend constexpr
    GfTimeCode operator-(const GfTimeCode& lhs,
                         const double rhs) noexcept
        { return GfTimeCode(lhs._time - rhs); }
    /// \overload
    friend constexpr
    GfTimeCode operator-(const double lhs,
                         const GfTimeCode& rhs) noexcept
        { return GfTimeCode(lhs - rhs._time); }

    /// Negate a time code. Equivalent to scaling by -1.
    constexpr GfTimeCode operator-() const noexcept
        { return GfTimeCode(-_time); }

    /// Explicit conversion to double
    explicit constexpr operator double() const noexcept {return _time;}

    /// Explicit conversion to GfDuration
    ///
    /// This is equivalent to *this - GfTimeCode(0)
    explicit constexpr operator GfDuration() const noexcept
    {
        return GfDuration(_time);
    }

    /// Hash function
    size_t GetHash() const {
        return std::hash<double>()(_time);
    }

    /// \class Hash
    struct Hash
    {
        size_t operator()(const GfTimeCode &ap) const {
            return ap.GetHash();
        }
    };

    friend size_t hash_value(const GfTimeCode &ap) { return ap.GetHash(); }

    /// Enable TfHash for GfTimeCode
    template <class HashState>
    friend void TfHashAppend(HashState &h, const GfTimeCode& t)
    {
        // Delegate to TfHashAppend(HashState, double)
        TfHashAppend(h, t._time);
    }

    /// @}

    /// \name Accessors
    /// @{

    /// Return the time value.
    constexpr double GetValue() const noexcept {
        return _time;
    }

    /// @}

private:
    friend inline void swap(GfTimeCode &lhs, GfTimeCode &rhs) {
        std::swap(lhs._time, rhs._time);
    }

    double _time;
};

/// \name Related
/// Binary arithmetic and comparison operators with double or GfDuration valued
/// lefthand side.
/// @{
inline constexpr
bool operator==(double time, const GfTimeCode &timeCode) noexcept
    { return GfTimeCode(time) == timeCode; }

inline constexpr
bool operator!=(double time, const GfTimeCode &timeCode) noexcept
    { return GfTimeCode(time) != timeCode; }

inline constexpr
bool operator<(double time, const GfTimeCode &timeCode) noexcept
    { return GfTimeCode(time) < timeCode; }

inline constexpr
bool operator>(double time, const GfTimeCode &timeCode) noexcept
    { return GfTimeCode(time) > timeCode; }

inline constexpr
bool operator<=(double time, const GfTimeCode &timeCode) noexcept
    { return GfTimeCode(time) <= timeCode; }

inline constexpr
bool operator>=(double time, const GfTimeCode &timeCode) noexcept
    { return GfTimeCode(time) >= timeCode; }

/// Stream insertion operator for the string representation of this
/// time code.
GF_API std::ostream& operator<<(std::ostream& out, const GfTimeCode& ap);

/// @}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_GF_TIME_CODE_H
