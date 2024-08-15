//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
//  (C) Copyright David Abrahams 2000.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//  The author gratefully acknowleges the support of Dragon Systems, Inc., in
//  producing this work.

#ifndef PXR_EXTERNAL_BOOST_PYTHON_ERRORS_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_ERRORS_HPP

# include "pxr/external/boost/python/detail/prefix.hpp"
# include <boost/function/function0.hpp>

namespace boost { namespace python {

struct BOOST_PYTHON_DECL error_already_set
{
  virtual ~error_already_set();
};

// Handles exceptions caught just before returning to Python code.
// Returns true iff an exception was caught.
BOOST_PYTHON_DECL bool handle_exception_impl(function0<void>);

template <class T>
bool handle_exception(T f)
{
    return handle_exception_impl(function0<void>(boost::ref(f)));
}

namespace detail { inline void rethrow() { throw; } }

inline void handle_exception()
{
    handle_exception(detail::rethrow);
}

BOOST_PYTHON_DECL void throw_error_already_set();

template <class T>
inline T* expect_non_null(T* x)
{
    if (x == 0)
        throw_error_already_set();
    return x;
}

// Return source if it is an instance of pytype; throw an appropriate
// exception otherwise.
BOOST_PYTHON_DECL PyObject* pytype_check(PyTypeObject* pytype, PyObject* source);

}} // namespace boost::python

#endif // PXR_EXTERNAL_BOOST_PYTHON_ERRORS_HPP
