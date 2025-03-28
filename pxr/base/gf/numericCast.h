//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_GF_NUMERIC_CAST_H
#define PXR_BASE_GF_NUMERIC_CAST_H

#include "pxr/pxr.h"

#include "pxr/base/gf/traits.h"

#include <cmath>
#include <limits>
#include <optional>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

/// Return true if integer \p t compares logically less-than integer \p u in a
/// mathematical sense.  The comparison is safe against non-value-preserving
/// integral conversion.
///
/// This mimics the C++20 std::cmp_less function for comparing integers of
/// different types where negative signed integers always compare less than (and
/// not equal to) unsigned integers.
template <class T, class U>
constexpr bool
GfIntegerCompareLess(T t, U u) noexcept
{
    static_assert(std::is_integral_v<T> && std::is_integral_v<U>);

    if constexpr (std::is_signed_v<T> == std::is_signed_v<U>) {
        return t < u;
    }
    else if constexpr (std::is_signed_v<T>) {
        return t < 0 || std::make_unsigned_t<T>(t) < u;
    }
    else {
        return u >= 0 && t < std::make_unsigned_t<U>(u);
    }
}

enum GfNumericCastFailureType {
    GfNumericCastPosOverflow,  ///< Value too high to convert.
    GfNumericCastNegOverflow,  ///< Value too low to convert.
    GfNumericCastNaN           ///< Value is a floating-point NaN.
};

/// Attempt to convert \p from to a value of type \p To "safely".  From and To
/// must be arithmetic types according to GfIsArithmetic -- either integral or
/// floating-point types (including GfHalf).  Return a std::optional holding the
/// converted value if conversion succeeds, otherwise the empty optional.  The
/// optional out-parameter \p failType can be used to determine why conversion
/// failed if desired.
///
/// What "safely" means depends on the types From and To.  If From and To are
/// both integral types, then \p from can safely convert to To if \p from is in
/// To's range.  For example if \p from is an int32_t and To is uint16_t, then
/// \p from can successfully convert if it is in the range [0, 65535].
///
/// If To is an integral type and From is a floating-point type (including
/// GfHalf), then \p from can safely convert to To if it is neither a NaN nor an
/// infinity, and after truncation to integer its value is in To's range, as
/// above.
///
/// Following boost::numeric_cast's behavior, no range checking is performed
/// converting from integral to floating-point or from floating-point to other
/// floating-point types.  Note that converting an integral value that is out of
/// GfHalf's _finite_ range will produce a +/- inf GfHalf.
///
template <class To, class From>
std::optional<To>
GfNumericCast(From from, GfNumericCastFailureType *failType = nullptr)
{
    static_assert(GfIsArithmetic<From>::value &&
                  GfIsArithmetic<To>::value);

    using FromLimits = std::numeric_limits<From>;
    using ToLimits = std::numeric_limits<To>;

    auto setFail = [&failType](GfNumericCastFailureType ft) {
        if (failType) {
            *failType = ft;
        };
    };

    // int -> int.
    if constexpr (std::is_integral_v<From> &&
                  std::is_integral_v<To>) {
        // Range check integer to integer.
        if (GfIntegerCompareLess(from, ToLimits::min())) {
            setFail(GfNumericCastNegOverflow);
            return {};
        }
        if (GfIntegerCompareLess(ToLimits::max(), from)) {
            setFail(GfNumericCastPosOverflow);
            return {};
        }
        // In-range.
        return static_cast<To>(from);
    }
    // float -> int.
    else if constexpr (GfIsFloatingPoint<From>::value &&
                       std::is_integral_v<To>) {
        // If the floating point value is NaN we cannot convert.
        if (std::isnan(from)) {
            setFail(GfNumericCastNaN);
            return {};
        }
        // If the floating point value is an infinity we cannot convert.
        if (std::isinf(from)) {
            setFail(std::signbit(static_cast<double>(from))
                    ? GfNumericCastNegOverflow
                    : GfNumericCastPosOverflow);
            return {};
        }
        // Otherwise the floating point value must be (when truncated) in the
        // range for the To type.  We do this by mapping the low/high values for
        // To into From, then displacing these away from zero by 1 to account
        // for the truncation, then checking against this range.  Note this
        // works okay for GfHalf whose max is ~65,000 when converting to
        // int32_t, say.  In that case we get a range like (-inf, inf), meaning
        // that all finite halfs are in-range.
        From low = static_cast<From>(ToLimits::lowest()) - static_cast<From>(1);
        From high = static_cast<From>(ToLimits::max()) + static_cast<From>(1);
        
        if (from <= low) {
            setFail(GfNumericCastNegOverflow);
            return {};
        }
        if (from >= high) {
            setFail(GfNumericCastPosOverflow);
            return {};
        }
        // The value is in-range.
        return static_cast<To>(from);
    }
    // float -> float, or float -> int.
    else {
        (void)setFail; // hush compiler.
        
        // No range checking, following boost::numeric_cast.
        return static_cast<To>(from);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_GF_NUMERIC_CAST_H
