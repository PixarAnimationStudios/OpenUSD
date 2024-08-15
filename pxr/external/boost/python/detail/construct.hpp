//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_CONSTRUCT_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_CONSTRUCT_HPP

namespace boost { namespace python { namespace detail { 

template <class T, class Arg>
void construct_pointee(void* storage, Arg& x, T const volatile*)
{
    new (storage) T(x);
}

template <class T, class Arg>
void construct_referent_impl(void* storage, Arg& x, T&(*)())
{
    construct_pointee(storage, x, (T*)0);
}

template <class T, class Arg>
void construct_referent(void* storage, Arg const& x, T(*tag)() = 0)
{
    construct_referent_impl(storage, x, tag);
}

template <class T, class Arg>
void construct_referent(void* storage, Arg& x, T(*tag)() = 0)
{
    construct_referent_impl(storage, x, tag);
}

}}} // namespace boost::python::detail

#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_CONSTRUCT_HPP
