//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_TRANSLATE_EXCEPTION_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_TRANSLATE_EXCEPTION_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/translate_exception.hpp>
#else

# include "pxr/external/boost/python/detail/exception_handler.hpp"
# include "pxr/external/boost/python/detail/type_traits.hpp"

# include <boost/call_traits.hpp>

#include <functional>

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail { 

// A ternary function object used to translate C++ exceptions of type
// ExceptionType into Python exceptions by invoking an object of type
// Translate. Typically the translate function will be curried with
// boost::bind().
template <class ExceptionType, class Translate>
struct translate_exception
{
    typedef typename add_lvalue_reference<
        typename add_const<ExceptionType>::type
    >::type exception_cref;
    
    inline bool operator()(
        exception_handler const& handler
      , std::function<void()> const& f
      , typename call_traits<Translate>::param_type translate) const
    {
        try
        {
            return handler(f);
        }
        catch(exception_cref e)
        {
            translate(e);
            return true;
        }
    }
};

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // TRANSLATE_EXCEPTION_DWA2002810_HPP
