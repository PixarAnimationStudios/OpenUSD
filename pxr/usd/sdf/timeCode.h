//
// Copyright 2019 Pixar
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
