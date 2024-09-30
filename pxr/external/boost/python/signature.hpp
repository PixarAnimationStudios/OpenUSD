///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002, Joel de Guzman, 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
///////////////////////////////////////////////////////////////////////////////
# ifndef PXR_EXTERNAL_BOOST_PYTHON_SIGNATURE_HPP
#  define PXR_EXTERNAL_BOOST_PYTHON_SIGNATURE_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/signature.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"

# include "pxr/external/boost/python/detail/mpl2/if.hpp"

#  include "pxr/external/boost/python/detail/preprocessor.hpp"
#  include "pxr/external/boost/python/detail/type_traits.hpp"
#  include "pxr/external/boost/python/type_list.hpp"

///////////////////////////////////////////////////////////////////////////////
namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail {

// A metafunction returning C1 if C1 is derived from C2, and C2
// otherwise
template <class C1, class C2>
struct most_derived
{
    typedef typename detail::mpl2::if_<
        detail::is_convertible<C1*,C2*>
      , C1
      , C2
    >::type type;
};

//  The following macros generate expansions for::
//
//      template <class RT, class T0... class TN>
//      inline mpl::vector<RT, T0...TN>
//      get_signature(RT(*)(T0...TN), void* = 0)
//      {
//          return mpl::list<RT, T0...TN>();
//      }
//
//   And, for an appropriate assortment of cv-qualifications::
//
//      template <class RT, class ClassT, class T0... class TN>
//      inline mpl::vector<RT, ClassT&, T0...TN>
//      get_signature(RT(ClassT::*)(T0...TN) cv))
//      {
//          return mpl::list<RT, ClassT&, T0...TN>();
//      }
//
//      template <class Target, class RT, class ClassT, class T0... class TN>
//      inline mpl::vector<
//          RT
//        , typename most_derived<Target, ClassT>::type&
//        , T0...TN
//      >
//      get_signature(RT(ClassT::*)(T0...TN) cv), Target*)
//      {
//          return mpl::list<RT, ClassT&, T0...TN>();
//      }
//
//  NOTE: 
//  This code previously supported functions with non-default calling
//  conventions. This was dropped since they were Microsoft-specific
//  extensions for x86 platforms, which we don't support.
//
//  There are two forms for invoking get_signature::
//
//      get_signature(f)
//
//  and ::
//
//      get_signature(f,(Target*)0)
//
//  These functions extract the return type, class (for member
//  functions) and arguments of the input signature and stuff them in
//  an mpl type sequence (the calling convention is dropped).
//  Note that cv-qualification is dropped from
//  the "hidden this" argument of member functions; that is a
//  necessary sacrifice to ensure that an lvalue from_python converter
//  is used.  A pointer is not used so that None will be rejected for
//  overload resolution.
//
//  The second form of get_signature essentially downcasts the "hidden
//  this" argument of member functions to Target, because the function
//  may actually be a member of a base class which is not wrapped, and
//  in that case conversion from python would fail.
//
//
// @group {

// 'default' calling convention

template <class RT, class... T>
inline python::type_list<RT, T...>
get_signature(RT(*)(T...), void* = 0)
{
    return python::type_list<RT, T...>();
}

#define PXR_BOOST_PYTHON_GET_SIGNATURE_MEMBERS(Q, ...)             \
template <class RT, class ClassT, class... T>                      \
inline python::type_list<RT, ClassT&, T...>                        \
get_signature(RT(ClassT::*)(T...) Q)                               \
{                                                                  \
    return python::type_list<RT, ClassT&, T...>();                 \
}                                                                  \
                                                                   \
template <                                                         \
    class Target                                                   \
  , class RT                                                       \
  , class ClassT                                                   \
  , class... T                                                     \
>                                                                  \
inline python::type_list<                                          \
    RT                                                             \
  , typename most_derived<Target, ClassT>::type&                   \
  , T...                                                           \
>                                                                  \
get_signature(                                                     \
    RT(ClassT::*)(T...) Q                                          \
  , Target*                                                        \
)                                                                  \
{                                                                  \
    return python::type_list<                                      \
        RT                                                         \
      , typename most_derived<Target, ClassT>::type&               \
      , T...                                                       \
    >();                                                           \
}

PXR_BOOST_PYTHON_APPLY_QUALIFIERS(PXR_BOOST_PYTHON_GET_SIGNATURE_MEMBERS);

#undef PXR_BOOST_PYTHON_GET_SIGNATURE_MEMBERS

// }

}}} // namespace PXR_BOOST_NAMESPACE::python::detail


#endif // PXR_USE_INTERNAL_BOOST_PYTHON
# endif // PXR_EXTERNAL_BOOST_PYTHON_SIGNATURE_HPP
