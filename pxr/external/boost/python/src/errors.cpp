//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef PXR_BOOST_PYTHON_SOURCE
# define PXR_BOOST_PYTHON_SOURCE
#endif

#include "pxr/external/boost/python/errors.hpp"
#include <boost/cast.hpp>
#include "pxr/external/boost/python/detail/exception_handler.hpp"
#include <exception>
#include <new>
#include <stdexcept>

namespace PXR_BOOST_NAMESPACE { namespace python {

error_already_set::~error_already_set() {}

// IMPORTANT: this function may only be called from within a catch block!
PXR_BOOST_PYTHON_DECL bool handle_exception_impl(std::function<void()> f)
{
    try
    {
        if (detail::exception_handler::chain)
            return detail::exception_handler::chain->handle(f);
        f();
        return false;
    }
    catch(const PXR_BOOST_NAMESPACE::python::error_already_set&)
    {
        // The python error reporting has already been handled.
    }
    catch(const std::bad_alloc&)
    {
        PyErr_NoMemory();
    }
    catch(const bad_numeric_cast& x)
    {
        PyErr_SetString(PyExc_OverflowError, x.what());
    }
    catch(const std::out_of_range& x)
    {
        PyErr_SetString(PyExc_IndexError, x.what());
    }
    catch(const std::invalid_argument& x)
    {
        PyErr_SetString(PyExc_ValueError, x.what());
    }
    catch(const std::exception& x)
    {
        PyErr_SetString(PyExc_RuntimeError, x.what());
    }
    catch(...)
    {
        PyErr_SetString(PyExc_RuntimeError, "unidentifiable C++ exception");
    }
    return true;
}

void PXR_BOOST_PYTHON_DECL throw_error_already_set()
{
    throw error_already_set();
}

namespace detail {

bool exception_handler::operator()(std::function<void()> const& f) const
{
    if (m_next)
    {
        return m_next->handle(f);
    }
    else
    {
        f();
        return false;
    }
}

exception_handler::exception_handler(handler_function const& impl)
    : m_impl(impl)
    , m_next(0)
{
    if (chain != 0)
        tail->m_next = this;
    else
        chain = this;
    tail = this;
}

exception_handler* exception_handler::chain;
exception_handler* exception_handler::tail;

PXR_BOOST_PYTHON_DECL void register_exception_handler(handler_function const& f)
{
    // the constructor links the new object into a handler chain, so
    // this object isn't actaully leaked (until, of course, the
    // interpreter exits).
    new exception_handler(f);
}

} // namespace PXR_BOOST_NAMESPACE::python::detail

}} // namespace PXR_BOOST_NAMESPACE::python


