//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_RETURN_VALUE_POLICY_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_RETURN_VALUE_POLICY_HPP

# include "pxr/external/boost/python/detail/prefix.hpp"
# include "pxr/external/boost/python/default_call_policies.hpp"

namespace boost { namespace python { 

template <class ResultConverterGenerator, class BasePolicy_ = default_call_policies>
struct return_value_policy : BasePolicy_
{
    typedef ResultConverterGenerator result_converter;
};

}} // namespace boost::python

#endif // PXR_EXTERNAL_BOOST_PYTHON_RETURN_VALUE_POLICY_HPP
