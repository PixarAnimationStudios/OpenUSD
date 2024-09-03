//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_EXCEPTION_HANDLER_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_EXCEPTION_HANDLER_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/exception_handler.hpp>
#else

# include "pxr/external/boost/python/detail/config.hpp"
# include <boost/function/function0.hpp>
# include <boost/function/function2.hpp>

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail {

struct exception_handler;

typedef function2<bool, exception_handler const&, function0<void> const&> handler_function;

struct PXR_BOOST_PYTHON_DECL exception_handler
{
 private: // types
    
 public:
    explicit exception_handler(handler_function const& impl);

    inline bool handle(function0<void> const& f) const;
    
    bool operator()(function0<void> const& f) const;
 
    static exception_handler* chain;
    
 private:
    static exception_handler* tail;
    
    handler_function m_impl;
    exception_handler* m_next;
};


inline bool exception_handler::handle(function0<void> const& f) const
{
    return this->m_impl(*this, f);
}

PXR_BOOST_PYTHON_DECL void register_exception_handler(handler_function const& f);

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_EXCEPTION_HANDLER_HPP
