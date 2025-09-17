//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_TIME_CODE_H
#define PXR_USD_SDF_TIME_CODE_H

/// \file sdf/timeCode.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"

#include <algorithm>
#include <functional>
#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE

/// \class SdfTimeCode
///
/// Value type that represents a time code. It's equivalent to a double type
/// value but is used to indicate that this value should be resolved by any
/// time based value resolution.
///
class SdfTimeCode
{
public:
    /// \name Constructors
    /// @{
    ///

    /// Construct a time code with the given time. 
    /// A default constructed SdfTimeCode has a time of 0.0.
    /// A double value can implicitly cast to SdfTimeCode.
    constexpr SdfTimeCode(double time = 0.0) noexcept : _time(time) {};

    /// @}

    ///\name Operators
    /// @{

    constexpr bool operator==(const SdfTimeCode &rhs) const noexcept
        { return _time == rhs._time; }
    constexpr bool operator!=(const SdfTimeCode &rhs) const noexcept
        { return _time != rhs._time; }
    constexpr bool operator<(const SdfTimeCode &rhs) const noexcept
        { return _time < rhs._time; }
    constexpr bool operator>(const SdfTimeCode &rhs) const noexcept
        { return _time > rhs._time; }
    constexpr bool operator<=(const SdfTimeCode &rhs) const noexcept
        { return _time <= rhs._time; }
    constexpr bool operator>=(const SdfTimeCode &rhs) const noexcept
        { return _time >= rhs._time; }

    constexpr SdfTimeCode operator*(const SdfTimeCode &rhs) const noexcept
        { return SdfTimeCode(_time * rhs._time); }
    constexpr SdfTimeCode operator/(const SdfTimeCode &rhs) const noexcept
        { return SdfTimeCode(_time / rhs._time); }
    constexpr SdfTimeCode operator+(const SdfTimeCode &rhs) const noexcept
        { return SdfTimeCode(_time + rhs._time); } 
    constexpr SdfTimeCode operator-(const SdfTimeCode &rhs) const noexcept
        { return SdfTimeCode(_time - rhs._time); }

    /// Explicit conversion to double
    explicit constexpr operator double() const noexcept {return _time;}
    
    /// Hash function
    size_t GetHash() const {
        return std::hash<double>()(_time);
    }

    /// \class Hash
    struct Hash
    {
        size_t operator()(const SdfTimeCode &ap) const {
            return ap.GetHash();
        }
    };

    friend size_t hash_value(const SdfTimeCode &ap) { return ap.GetHash(); }

    /// @}

    /// \name Accessors
    /// @{

    /// Return the time value.
    constexpr double GetValue() const noexcept {
        return _time;
    }

    /// @}

private:
    friend inline void swap(SdfTimeCode &lhs, SdfTimeCode &rhs) {
        std::swap(lhs._time, rhs._time);
    }

    double _time;
};

/// \name Related
/// Binary arithmetic and comparison operators with double valued lefthand side.
/// @{

inline constexpr 
SdfTimeCode operator*(double time, const SdfTimeCode &timeCode) noexcept
    { return SdfTimeCode(time) * timeCode; }

inline constexpr 
SdfTimeCode operator/(double time, const SdfTimeCode &timeCode) noexcept
    { return SdfTimeCode(time) / timeCode; }

inline constexpr 
SdfTimeCode operator+(double time, const SdfTimeCode &timeCode) noexcept
    { return SdfTimeCode(time) + timeCode; }

inline constexpr 
SdfTimeCode operator-(double time, const SdfTimeCode &timeCode) noexcept
    { return SdfTimeCode(time) - timeCode; }

inline constexpr 
bool operator==(double time, const SdfTimeCode &timeCode) noexcept
    { return SdfTimeCode(time) == timeCode; }

inline constexpr 
bool operator!=(double time, const SdfTimeCode &timeCode) noexcept
    { return SdfTimeCode(time) != timeCode; }

inline constexpr 
bool operator<(double time, const SdfTimeCode &timeCode) noexcept
    { return SdfTimeCode(time) < timeCode; }

inline constexpr 
bool operator>(double time, const SdfTimeCode &timeCode) noexcept
    { return SdfTimeCode(time) > timeCode; }

inline constexpr 
bool operator<=(double time, const SdfTimeCode &timeCode) noexcept
    { return SdfTimeCode(time) <= timeCode; }

inline constexpr 
bool operator>=(double time, const SdfTimeCode &timeCode) noexcept
    { return SdfTimeCode(time) >= timeCode; }

/// Stream insertion operator for the string representation of this time code.
SDF_API std::ostream& operator<<(std::ostream& out, const SdfTimeCode& ap);

/// @}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_TIME_CODE_H
