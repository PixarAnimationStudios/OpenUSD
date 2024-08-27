#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_BORROWED_PTR_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_BORROWED_PTR_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/borrowed_ptr.hpp>
#else
//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

# include <boost/config.hpp>
# include <boost/type.hpp>
# include <boost/mpl/if.hpp>
# include "pxr/external/boost/python/detail/type_traits.hpp"
# include "pxr/external/boost/python/tag.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail {

template<class T> class borrowed
{ 
    typedef T type;
};

template<typename T>
struct is_borrowed_ptr
{
    BOOST_STATIC_CONSTANT(bool, value = false); 
};

#  if !defined(__MWERKS__) || __MWERKS__ > 0x3000
template<typename T>
struct is_borrowed_ptr<borrowed<T>*>
{
    BOOST_STATIC_CONSTANT(bool, value = true);
};

template<typename T>
struct is_borrowed_ptr<borrowed<T> const*>
{
    BOOST_STATIC_CONSTANT(bool, value = true);
};

template<typename T>
struct is_borrowed_ptr<borrowed<T> volatile*>
{
    BOOST_STATIC_CONSTANT(bool, value = true);
};

template<typename T>
struct is_borrowed_ptr<borrowed<T> const volatile*>
{
    BOOST_STATIC_CONSTANT(bool, value = true);
};
#  else
template<typename T>
struct is_borrowed
{
    BOOST_STATIC_CONSTANT(bool, value = false);
};
template<typename T>
struct is_borrowed<borrowed<T> >
{
    BOOST_STATIC_CONSTANT(bool, value = true);
};
template<typename T>
struct is_borrowed_ptr<T*>
    : is_borrowed<typename remove_cv<T>::type>
{
};
#  endif 


}

template <class T>
inline T* get_managed_object(detail::borrowed<T> const volatile* p, tag_t)
{
    return (T*)p;
}

}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // #ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_BORROWED_PTR_HPP
