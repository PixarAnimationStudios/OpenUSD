//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_NULLARY_FUNCTION_ADAPTOR_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_NULLARY_FUNCTION_ADAPTOR_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/nullary_function_adaptor.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail { 

// nullary_function_adaptor -- a class template which ignores its
// arguments and calls a nullary function instead.  Used for building
// error-reporting functions, c.f. pure_virtual
template <class NullaryFunction>
struct nullary_function_adaptor
{
    nullary_function_adaptor(NullaryFunction fn)
      : m_fn(fn)
    {}

    template <class... A>
    void operator()(A const&...) const { m_fn(); }
    
 private:
    NullaryFunction m_fn;
};

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_NULLARY_FUNCTION_ADAPTOR_HPP
