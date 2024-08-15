//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_UNWRAP_WRAPPER_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_UNWRAP_WRAPPER_HPP

# include "pxr/external/boost/python/detail/prefix.hpp"
# include "pxr/external/boost/python/detail/is_wrapper.hpp"
# include <boost/mpl/eval_if.hpp>
# include <boost/mpl/identity.hpp>

namespace boost { namespace python { namespace detail { 

template <class T>
struct unwrap_wrapper_helper
{
    typedef typename T::_wrapper_wrapped_type_ type;
};

template <class T>
struct unwrap_wrapper_
  : mpl::eval_if<is_wrapper<T>,unwrap_wrapper_helper<T>,mpl::identity<T> >
{};

template <class T>
typename unwrap_wrapper_<T>::type*
unwrap_wrapper(T*)
{
    return 0;
}

}}} // namespace boost::python::detail

#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_UNWRAP_WRAPPER_HPP
