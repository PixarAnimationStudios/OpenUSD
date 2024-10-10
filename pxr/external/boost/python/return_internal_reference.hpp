//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_RETURN_INTERNAL_REFERENCE_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_RETURN_INTERNAL_REFERENCE_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/return_internal_reference.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"

# include "pxr/external/boost/python/default_call_policies.hpp"
# include "pxr/external/boost/python/reference_existing_object.hpp"
# include "pxr/external/boost/python/with_custodian_and_ward.hpp"
# include "pxr/external/boost/python/detail/mpl2/if.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { 

namespace detail
{
  template <std::size_t>
  struct return_internal_reference_owner_arg_must_be_greater_than_zero
# if defined(__GNUC__) || defined(__EDG__)
  {}
# endif
  ;
}

template <std::size_t owner_arg = 1, class BasePolicy_ = default_call_policies>
struct return_internal_reference
    : with_custodian_and_ward_postcall<0, owner_arg, BasePolicy_>
{
 private:
    static constexpr bool legal = owner_arg > 0;
 public:
    typedef typename detail::mpl2::if_c<
        legal
        , reference_existing_object
        , detail::return_internal_reference_owner_arg_must_be_greater_than_zero<owner_arg>
    >::type result_converter;
};

}} // namespace PXR_BOOST_NAMESPACE::python

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_RETURN_INTERNAL_REFERENCE_HPP
