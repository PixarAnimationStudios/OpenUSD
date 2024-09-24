//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_INTEGER_CAST_HPP
#define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_INTEGER_CAST_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#include <limits>
#include <type_traits>
#include <typeinfo>

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail {

class bad_integer_cast 
    : public std::bad_cast
{
protected:
    bad_integer_cast() = default;
};

class negative_overflow
    : public bad_integer_cast
{
public:
    char const* what() const noexcept override
    { return "bad integer conversion: negative overflow"; }
};

class positive_overflow
    : public bad_integer_cast
{
public:
    char const* what() const noexcept override
    { return "bad integer conversion: positive overflow"; }
};

template <class T, class U>
constexpr bool
integer_compare_less(T t, U u) noexcept
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

template <class To, class From>
To
integer_cast(From from)
{
    static_assert(std::is_integral_v<To> && std::is_integral_v<From>);

    using FromLimits = std::numeric_limits<From>;
    using ToLimits = std::numeric_limits<To>;

    // Range check integer to integer.
    if (integer_compare_less(from, ToLimits::min())) {
        throw negative_overflow();
    }
    if (integer_compare_less(ToLimits::max(), from)) {
        throw positive_overflow();
    }
    // In-range.
    return static_cast<To>(from);
}

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_INTEGER_CAST_HPP
