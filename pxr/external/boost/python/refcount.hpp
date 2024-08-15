//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_REFCOUNT_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_REFCOUNT_HPP

# include "pxr/external/boost/python/detail/prefix.hpp"
# include "pxr/external/boost/python/cast.hpp"

namespace boost { namespace python { 

template <class T>
inline T* incref(T* p)
{
    Py_INCREF(python::upcast<PyObject>(p));
    return p;
}

template <class T>
inline T* xincref(T* p)
{
    Py_XINCREF(python::upcast<PyObject>(p));
    return p;
}

template <class T>
inline void decref(T* p)
{
    assert( Py_REFCNT(python::upcast<PyObject>(p)) > 0 );
    Py_DECREF(python::upcast<PyObject>(p));
}

template <class T>
inline void xdecref(T* p)
{
    assert( !p || Py_REFCNT(python::upcast<PyObject>(p)) > 0 );
    Py_XDECREF(python::upcast<PyObject>(p));
}

}} // namespace boost::python

#endif // PXR_EXTERNAL_BOOST_PYTHON_REFCOUNT_HPP
