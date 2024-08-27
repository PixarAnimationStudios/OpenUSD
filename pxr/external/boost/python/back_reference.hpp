//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_BACK_REFERENCE_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_BACK_REFERENCE_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/back_reference.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"

# include "pxr/external/boost/python/object_fwd.hpp"
# include "pxr/external/boost/python/detail/dependent.hpp"
# include "pxr/external/boost/python/detail/raw_pyobject.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { 

template <class T>
struct back_reference
{
 private: // types
    typedef typename detail::dependent<object,T>::type source_t;
 public:
    typedef T type;
    
    back_reference(PyObject*, T);
    source_t const& source() const;
    T get() const;
 private:
    source_t m_source;
    T m_value;
};

template<typename T>
class is_back_reference
{
 public:
    BOOST_STATIC_CONSTANT(bool, value = false); 
};

template<typename T>
class is_back_reference<back_reference<T> >
{
 public:
    BOOST_STATIC_CONSTANT(bool, value = true);
};


//
// implementations
//
template <class T>
back_reference<T>::back_reference(PyObject* p, T x)
    : m_source(detail::borrowed_reference(p))
      , m_value(x)
{
}

template <class T>
typename back_reference<T>::source_t const& back_reference<T>::source() const
{
    return m_source;
}

template <class T>
T back_reference<T>::get() const
{
    return m_value;
}

}} // namespace PXR_BOOST_NAMESPACE::python

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_BACK_REFERENCE_HPP
