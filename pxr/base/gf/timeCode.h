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
    /// A default constructed GfTimeCode has a time of 0.0.
    /// A double value can implicitly cast to GfTimeCode.
    constexpr GfTimeCode(double time = 0.0) noexcept : _time(time) {};

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

    constexpr GfTimeCode operator*(const GfTimeCode &rhs) const noexcept
        { return GfTimeCode(_time * rhs._time); }
    constexpr GfTimeCode operator/(const GfTimeCode &rhs) const noexcept
        { return GfTimeCode(_time / rhs._time); }
    constexpr GfTimeCode operator+(const GfTimeCode &rhs) const noexcept
        { return GfTimeCode(_time + rhs._time); }
    constexpr GfTimeCode operator-(const GfTimeCode &rhs) const noexcept
        { return GfTimeCode(_time - rhs._time); }
    constexpr GfTimeCode operator-() const noexcept
        { return GfTimeCode(-_time); }

    /// Explicit conversion to double
    explicit constexpr operator double() const noexcept {return _time;}

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
/// Binary arithmetic and comparison operators with double valued
/// lefthand side.
/// @{

inline constexpr
GfTimeCode operator*(double time, const GfTimeCode &timeCode) noexcept
    { return GfTimeCode(time) * timeCode; }

inline constexpr
GfTimeCode operator/(double time, const GfTimeCode &timeCode) noexcept
    { return GfTimeCode(time) / timeCode; }

inline constexpr
GfTimeCode operator+(double time, const GfTimeCode &timeCode) noexcept
    { return GfTimeCode(time) + timeCode; }

inline constexpr
GfTimeCode operator-(double time, const GfTimeCode &timeCode) noexcept
    { return GfTimeCode(time) - timeCode; }

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
