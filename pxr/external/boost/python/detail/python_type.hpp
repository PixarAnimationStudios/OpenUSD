//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright Nikolay Mladenov 2007.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_PYTHON_TYPE_HPP
#define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_PYTHON_TYPE_HPP

#include "pxr/external/boost/python/converter/registered.hpp"

namespace boost {namespace python {namespace detail{


template <class T> struct python_class : PyObject
{
    typedef python_class<T> this_type;

    typedef T type;

    static void *converter (PyObject *p){
        return p;
    }

    static void register_()
    {
        static bool first_time = true;

        if ( !first_time ) return;

        first_time = false;
        converter::registry::insert(&converter, boost::python::type_id<this_type>(), &converter::registered_pytype_direct<T>::get_pytype);
    }
};


}}} //namespace boost :: python :: detail

#endif //PXR_EXTERNAL_BOOST_PYTHON_DETAIL_PYTHON_TYPE_HPP
