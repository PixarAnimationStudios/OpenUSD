//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright Shreyans Doshi 2017.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_TYPE_TRAITS_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_TYPE_TRAITS_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/type_traits.hpp>
#else

# include <type_traits>

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail {

    using std::alignment_of;
    using std::add_const;
    using std::add_cv;
    using std::add_lvalue_reference;
    using std::add_pointer;

    using std::is_array;
    using std::is_class;
    using std::is_const;
    using std::is_convertible;
    using std::is_enum;
    using std::is_function;
    using std::is_integral;
    using std::is_lvalue_reference;
    using std::is_member_function_pointer;
    using std::is_member_pointer;
    using std::is_pointer;
    using std::is_polymorphic;
    using std::is_reference;
    using std::is_same;
    using std::is_scalar;
    using std::is_union;
    using std::is_void;
    using std::is_volatile;

    using std::remove_reference;
    using std::remove_pointer;
    using std::remove_cv;
    using std::remove_const;

    typedef std::integral_constant<bool, true> true_;
    typedef std::integral_constant<bool, false> false_;

    // This was previously boost::is_base_and_derived, which was once
    // user-facing but now appears to be an undocumented implementation
    // detail in boost/type_traits.
    //
    // The boost trait is *not* equivalent to std::is_base_of. Most
    // critically, boost::is_base_and_derived<T, T>::value is false,
    // while std::is_base_of<T, T>::value is true. We accommodate that
    // difference below.
    //
    // boost::is_base_and_derived also handles inaccessible
    // or ambiguous base classes, whereas std::is_base_of does not.
    // This does not appear to be relevant for this library.
    template <class Base, class Derived>
    using is_base_and_derived = std::bool_constant<
        std::is_base_of_v<Base, Derived> && !std::is_same_v<Base, Derived>
    >;

}}} // namespace PXR_BOOST_NAMESPACE::python::detail


#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif //BOOST_DETAIL_TYPE_TRAITS_HPP
