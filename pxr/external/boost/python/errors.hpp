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

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/errors.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"
#include <functional>

namespace PXR_BOOST_NAMESPACE { namespace python {

struct PXR_BOOST_PYTHON_DECL error_already_set
{
  virtual ~error_already_set();
};

// Handles exceptions caught just before returning to Python code.
// Returns true iff an exception was caught.
PXR_BOOST_PYTHON_DECL bool handle_exception_impl(std::function<void()>);

template <class T>
bool handle_exception(T f)
{
    return handle_exception_impl(std::function<void()>(std::ref(f)));
}

namespace detail { inline void rethrow() { throw; } }

inline void handle_exception()
{
    handle_exception(detail::rethrow);
}

PXR_BOOST_PYTHON_DECL void throw_error_already_set();

template <class T>
inline T* expect_non_null(T* x)
{
    if (x == 0)
        throw_error_already_set();
    return x;
}

// Return source if it is an instance of pytype; throw an appropriate
// exception otherwise.
PXR_BOOST_PYTHON_DECL PyObject* pytype_check(PyTypeObject* pytype, PyObject* source);

}} // namespace PXR_BOOST_NAMESPACE::python

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_ERRORS_HPP
