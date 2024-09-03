//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_AIX_INIT_MODULE_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_AIX_INIT_MODULE_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/aix_init_module.hpp>
#else
# ifdef _AIX
# include "pxr/external/boost/python/detail/prefix.hpp"
# include <cstdio>
# ifdef __KCC
#  include <iostream> // this works around a problem in KCC 4.0f
# endif 

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail { 

extern "C"
{
    typedef PyObject* (*so_load_function)(char*,char*,FILE*);
}

void aix_init_module(so_load_function, char const* name, void (*init_module)());

}}} // namespace PXR_BOOST_NAMESPACE::python::detail
# endif

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_AIX_INIT_MODULE_HPP
