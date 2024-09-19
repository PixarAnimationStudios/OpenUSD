//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_OBJECT_OPERATORS_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_OBJECT_OPERATORS_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/object_operators.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"

# include "pxr/external/boost/python/object_core.hpp"
# include "pxr/external/boost/python/call.hpp"
# include <boost/iterator/detail/enable_if.hpp>
# include "pxr/external/boost/python/detail/mpl2/bool.hpp"

# include <boost/iterator/detail/config_def.hpp>

namespace PXR_BOOST_NAMESPACE { namespace python { namespace api {

template <class X>
char is_object_operators_helper(object_operators<X> const*);
    
typedef char (&no_type)[2];
no_type is_object_operators_helper(...);

template <class X> X* make_ptr();

template <class L, class R = L>
struct is_object_operators
{
    enum {
        value 
        = (sizeof(api::is_object_operators_helper(api::make_ptr<L>()))
           + sizeof(api::is_object_operators_helper(api::make_ptr<R>()))
           < 4
        )
    };
    typedef python::detail::mpl2::bool_<value> type;
};

template <class L, class R, class T>
struct enable_binary
  : boost::iterators::enable_if<is_object_operators<L,R>, T>
{};
#  define PXR_BOOST_PYTHON_BINARY_RETURN(T) typename enable_binary<L,R,T>::type

template <class U>
object object_operators<U>::operator()() const
{
    object_cref2 f = *static_cast<U const*>(this);
    return call<object>(f.ptr());
}

template <class U>
template <class A0, class... A>
typename detail::dependent<object, A0>::type
object_operators<U>::operator()(A0 const& a0, A const&... a) const
{
    typedef typename detail::dependent<object, A0>::type obj;
    U const& self = *static_cast<U const*>(this);
    return call<obj>(get_managed_object(self, tag), a0, a...);
}

template <class U>
inline
object_operators<U>::operator bool_type() const
{
    object_cref2 x = *static_cast<U const*>(this);
    int is_true = PyObject_IsTrue(x.ptr());
    if (is_true < 0) throw_error_already_set();
    return is_true ? &object::ptr : 0;
}

template <class U>
inline bool
object_operators<U>::operator!() const
{
    object_cref2 x = *static_cast<U const*>(this);
    int is_true = PyObject_IsTrue(x.ptr());
    if (is_true < 0) throw_error_already_set();
    return !is_true;
}

# define PXR_BOOST_PYTHON_COMPARE_OP(op, opid)                              \
template <class L, class R>                                             \
PXR_BOOST_PYTHON_BINARY_RETURN(object) operator op(L const& l, R const& r)    \
{                                                                       \
    return PyObject_RichCompare(                                    \
        object(l).ptr(), object(r).ptr(), opid);                        \
}
# undef PXR_BOOST_PYTHON_COMPARE_OP
    
# define PXR_BOOST_PYTHON_BINARY_OPERATOR(op)                               \
PXR_BOOST_PYTHON_DECL object operator op(object const& l, object const& r); \
template <class L, class R>                                             \
PXR_BOOST_PYTHON_BINARY_RETURN(object) operator op(L const& l, R const& r)  \
{                                                                       \
    return object(l) op object(r);                                      \
}
PXR_BOOST_PYTHON_BINARY_OPERATOR(>)
PXR_BOOST_PYTHON_BINARY_OPERATOR(>=)
PXR_BOOST_PYTHON_BINARY_OPERATOR(<)
PXR_BOOST_PYTHON_BINARY_OPERATOR(<=)
PXR_BOOST_PYTHON_BINARY_OPERATOR(==)
PXR_BOOST_PYTHON_BINARY_OPERATOR(!=)
PXR_BOOST_PYTHON_BINARY_OPERATOR(+)
PXR_BOOST_PYTHON_BINARY_OPERATOR(-)
PXR_BOOST_PYTHON_BINARY_OPERATOR(*)
PXR_BOOST_PYTHON_BINARY_OPERATOR(/)
PXR_BOOST_PYTHON_BINARY_OPERATOR(%)
PXR_BOOST_PYTHON_BINARY_OPERATOR(<<)
PXR_BOOST_PYTHON_BINARY_OPERATOR(>>)
PXR_BOOST_PYTHON_BINARY_OPERATOR(&)
PXR_BOOST_PYTHON_BINARY_OPERATOR(^)
PXR_BOOST_PYTHON_BINARY_OPERATOR(|)
# undef PXR_BOOST_PYTHON_BINARY_OPERATOR

        
# define PXR_BOOST_PYTHON_INPLACE_OPERATOR(op)                              \
PXR_BOOST_PYTHON_DECL object& operator op(object& l, object const& r);      \
template <class R>                                                      \
object& operator op(object& l, R const& r)                              \
{                                                                       \
    return l op object(r);                                              \
}
PXR_BOOST_PYTHON_INPLACE_OPERATOR(+=)
PXR_BOOST_PYTHON_INPLACE_OPERATOR(-=)
PXR_BOOST_PYTHON_INPLACE_OPERATOR(*=)
PXR_BOOST_PYTHON_INPLACE_OPERATOR(/=)
PXR_BOOST_PYTHON_INPLACE_OPERATOR(%=)
PXR_BOOST_PYTHON_INPLACE_OPERATOR(<<=)
PXR_BOOST_PYTHON_INPLACE_OPERATOR(>>=)
PXR_BOOST_PYTHON_INPLACE_OPERATOR(&=)
PXR_BOOST_PYTHON_INPLACE_OPERATOR(^=)
PXR_BOOST_PYTHON_INPLACE_OPERATOR(|=)
# undef PXR_BOOST_PYTHON_INPLACE_OPERATOR

}}} // namespace PXR_BOOST_NAMESPACE::python

#include <boost/iterator/detail/config_undef.hpp>

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_OBJECT_OPERATORS_HPP
