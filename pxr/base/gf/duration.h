//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_GF_DURATION_H
#define PXR_BASE_GF_DURATION_H

/// \file gf/duration.h

#include "pxr/pxr.h"
#include "pxr/base/gf/api.h"

#include <functional>
#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE

/// \class GfDuration
///
/// Value type that represents a duration measured in time codes. It
/// is equivalent to a double type value but is used to indicate that
/// this value should be resolved by any time based value resolution.
///
class GfDuration
{
public:
    /// \name Constructors
    /// @{
    ///

    /// Construct a duration with the given elapsed time.
    /// A default constructed GfDuration has a duration of 0.0.
    /// A double value can implicitly cast to GfDuration.
    constexpr GfDuration(double duration = 0.0) noexcept : _duration(duration) {};

    /// @}

    ///\name Operators
    /// @{

    /// Comparison operators
    friend constexpr bool operator==(const GfDuration &lhs,
                                     const GfDuration &rhs) noexcept
        { return lhs._duration == rhs._duration; }
    friend constexpr bool operator!=(const GfDuration &lhs,
                                     const GfDuration &rhs) noexcept
        { return lhs._duration != rhs._duration; }
    friend constexpr bool operator<(const GfDuration &lhs,
                                    const GfDuration &rhs) noexcept
        { return lhs._duration < rhs._duration; }
    friend constexpr bool operator>(const GfDuration &lhs,
                                    const GfDuration &rhs) noexcept
        { return lhs._duration > rhs._duration; }
    friend constexpr bool operator<=(const GfDuration &lhs,
                                     const GfDuration &rhs) noexcept
        { return lhs._duration <= rhs._duration; }
    friend constexpr bool operator>=(const GfDuration &lhs,
                                     const GfDuration &rhs) noexcept
        { return lhs._duration >= rhs._duration; }

    /// Scale duration values by doubles
    /// @{
    friend constexpr
    GfDuration operator*(const GfDuration& d,
                         const double amount) noexcept
        { return GfDuration(d._duration * amount); }
    friend constexpr
    GfDuration operator*(const double amount,
                         const GfDuration& d) noexcept
        { return GfDuration(amount * d._duration); }
    friend constexpr
    GfDuration operator/(const GfDuration& d,
                         const double amount) noexcept
        { return GfDuration(d._duration / amount); }
    /// @}

    /// Ratio of two durations is a unitless double scale factor
    constexpr double operator/(const GfDuration &rhs) const noexcept
        { return (_duration / rhs._duration); }

    /// Sum, difference, and negation of durations are durations.
    /// @{
    friend constexpr
    GfDuration operator+(const GfDuration& lhs,
                         const GfDuration& rhs) noexcept
        { return GfDuration(lhs._duration + rhs._duration); }
    friend constexpr
    GfDuration operator-(const GfDuration& lhs,
                         const GfDuration& rhs) noexcept
        { return GfDuration(lhs._duration - rhs._duration); }
    constexpr GfDuration operator-() const noexcept
        { return GfDuration(-_duration); }
    /// @}

    /// Explicit conversion to double
    explicit constexpr operator double() const noexcept {return _duration;}

    /// Hash operations
    /// @{
    size_t GetHash() const {
        return std::hash<double>()(_duration);
    }

    /// \class Hash
    struct Hash
    {
        size_t operator()(const GfDuration& ap) const {
            return ap.GetHash();
        }
    };

    friend size_t hash_value(const GfDuration& ap) { return ap.GetHash(); }

    /// Enable TfHash for GfDuration
    template <class HashState>
    friend void TfHashAppend(HashState &h, const GfDuration& d)
    {
        // Delegate to TfHashAppend(HashState, double)
        TfHashAppend(h, d._duration);
    }

    /// @}
    /// @}

    /// \name Accessors
    /// @{

    /// Return the duration value.
    constexpr double GetValue() const noexcept {
        return _duration;
    }

    /// @}

private:
    friend inline void swap(GfDuration &lhs, GfDuration &rhs) {
        std::swap(lhs._duration, rhs._duration);
    }

    double _duration;
};


/// Stream insertion operator for the string representation of this
/// duration code.
GF_API std::ostream& operator<<(std::ostream& out, const GfDuration& duration);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_GF_DURATION_H
