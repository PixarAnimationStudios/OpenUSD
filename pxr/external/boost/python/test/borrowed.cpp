//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include "pxr/external/boost/python/detail/wrap_python.hpp"
#include "pxr/external/boost/python/borrowed.hpp"
#include <boost/static_assert.hpp>

using namespace PXR_BOOST_NAMESPACE::python;

template <class T>
void assert_borrowed_ptr(T const&)
{
    BOOST_STATIC_ASSERT(PXR_BOOST_NAMESPACE::python::detail::is_borrowed_ptr<T>::value);
}
    
template <class T>
void assert_not_borrowed_ptr(T const&)
{
    BOOST_STATIC_ASSERT(!PXR_BOOST_NAMESPACE::python::detail::is_borrowed_ptr<T>::value);
}
    
int main()
{
    assert_borrowed_ptr(borrowed((PyObject*)0));
    assert_borrowed_ptr(borrowed((PyTypeObject*)0));
    assert_borrowed_ptr((detail::borrowed<PyObject> const*)0);
    assert_borrowed_ptr((detail::borrowed<PyObject> volatile*)0);
    assert_borrowed_ptr((detail::borrowed<PyObject> const volatile*)0);
    assert_not_borrowed_ptr((PyObject*)0);
    assert_not_borrowed_ptr(0);
    return 0;
}
