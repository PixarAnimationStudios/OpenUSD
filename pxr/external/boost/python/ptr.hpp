#ifndef PXR_EXTERNAL_BOOST_PYTHON_PTR_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_PTR_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/ptr.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"
//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Based on boost/ref.hpp, thus:
//  Copyright (C) 1999, 2000 Jaakko Jarvi (jaakko.jarvi@cs.utu.fi)
//  Copyright (C) 2001 Peter Dimov

# include <boost/config.hpp>
# include "pxr/external/boost/python/detail/mpl2/bool.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python {

template<class Ptr> class pointer_wrapper
{ 
 public:
    typedef Ptr type;
    
    explicit pointer_wrapper(Ptr x): p_(x) {}
    operator Ptr() const { return p_; }
    Ptr get() const { return p_; }
 private:
    Ptr p_;
};

template<class T>
inline pointer_wrapper<T> ptr(T t)
{ 
    return pointer_wrapper<T>(t);
}

template<typename T>
class is_pointer_wrapper
    : public detail::mpl2::false_
{
};

template<typename T>
class is_pointer_wrapper<pointer_wrapper<T> >
    : public detail::mpl2::true_
{
};

template<typename T>
class unwrap_pointer
{
 public:
    typedef T type;
};

template<typename T>
class unwrap_pointer<pointer_wrapper<T> >
{
 public:
    typedef T type;
};

}} // namespace PXR_BOOST_NAMESPACE::python

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif
