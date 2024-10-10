//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_EXCEPTION_TRANSLATOR_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_EXCEPTION_TRANSLATOR_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/exception_translator.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"

# include "pxr/external/boost/python/type.hpp"
# include "pxr/external/boost/python/detail/translate_exception.hpp"
# include "pxr/external/boost/python/detail/exception_handler.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { 

template <class ExceptionType, class Translate>
void register_exception_translator(Translate translate, type<ExceptionType>* = 0)
{
    // XXX:
    // Avoid ambiguity between std::placeholders and boost.
    // Can be replaced with "using namespace std::placeholders" once
    // boost dependency has been removed.
    using std::placeholders::_1;
    using std::placeholders::_2;
    detail::register_exception_handler(
        std::bind<bool>(detail::translate_exception<ExceptionType,Translate>(), _1, _2, translate)
        );
}

}} // namespace PXR_BOOST_NAMESPACE::python

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_EXCEPTION_TRANSLATOR_HPP
