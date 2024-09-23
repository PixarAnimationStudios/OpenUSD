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
# include <boost/preprocessor/iteration/local.hpp>
# include <boost/preprocessor/facilities/intercept.hpp>
# include <boost/preprocessor/repetition/enum_params.hpp>
# include <boost/preprocessor/repetition/enum_binary_params.hpp>

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

    void operator()() const { m_fn(); }

# define BOOST_PP_LOCAL_MACRO(i)                                            \
    template <BOOST_PP_ENUM_PARAMS_Z(1, i, class A)>                        \
    void operator()(                                                        \
        BOOST_PP_ENUM_BINARY_PARAMS_Z(1, i, A, const& BOOST_PP_INTERCEPT)   \
    ) const                                                                 \
    {                                                                       \
        m_fn();                                                             \
    }

# define BOOST_PP_LOCAL_LIMITS (1, PXR_BOOST_PYTHON_MAX_ARITY)
# include BOOST_PP_LOCAL_ITERATE()
    
 private:
    NullaryFunction m_fn;
};

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_NULLARY_FUNCTION_ADAPTOR_HPP
