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
# include <boost/type_traits/is_base_and_derived.hpp>
# include <boost/type_traits/alignment_traits.hpp>
# include <boost/type_traits/has_trivial_copy.hpp>


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

    using boost::is_base_and_derived;
    using boost::type_with_alignment;
    using boost::has_trivial_copy;
}}} // namespace PXR_BOOST_NAMESPACE::python::detail


#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif //BOOST_DETAIL_TYPE_TRAITS_HPP
