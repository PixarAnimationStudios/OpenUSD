//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_OBJECT_FORWARD_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_OBJECT_FORWARD_HPP

# include <boost/mpl/if.hpp>
# include <boost/ref.hpp>
# include "pxr/external/boost/python/detail/value_arg.hpp"
# include "pxr/external/boost/python/detail/type_traits.hpp"
# include "pxr/external/boost/python/detail/copy_ctor_mutates_rhs.hpp"
# include <boost/mpl/or.hpp>

namespace boost { namespace python { namespace objects { 

// Very much like boost::reference_wrapper<T>, except that in this
// case T can be a reference already without causing a
// reference-to-reference error.
template <class T>
struct reference_to_value
{
    typedef typename boost::python::detail::add_lvalue_reference<typename
        boost::python::detail::add_const<T>::type>::type reference;
    
    reference_to_value(reference x) : m_value(x) {}
    reference get() const { return m_value; }
 private:
    reference m_value;
};

// A little metaprogram which selects the type to pass through an
// intermediate forwarding function when the destination argument type
// is T.
template <class T>
struct forward
    : mpl::if_<
          mpl::or_<python::detail::copy_ctor_mutates_rhs<T>, boost::python::detail::is_scalar<T> >
        , T
        , reference_to_value<T>
      >
{
};

template<typename T>
struct unforward
{
    typedef typename unwrap_reference<T>::type& type;
};

template<typename T>
struct unforward<reference_to_value<T> >
{
    typedef T type;
};

template <typename T>
struct unforward_cref
  : python::detail::value_arg<
        typename unwrap_reference<T>::type
    >
{
};

template<typename T>
struct unforward_cref<reference_to_value<T> >
  : boost::python::detail::add_lvalue_reference<typename boost::python::detail::add_const<T>::type>
{
};


template <class T>
typename reference_to_value<T>::reference
do_unforward(reference_to_value<T> const& x, int)
{
    return x.get();
}

template <class T>
typename reference_wrapper<T>::type&
do_unforward(reference_wrapper<T> const& x, int)
{
    return x.get();
}

template <class T>
T const& do_unforward(T const& x, ...)
{
    return x;
}

}}} // namespace boost::python::objects

#endif // PXR_EXTERNAL_BOOST_PYTHON_OBJECT_FORWARD_HPP
