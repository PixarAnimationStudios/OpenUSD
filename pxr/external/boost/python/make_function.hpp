//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_MAKE_FUNCTION_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_MAKE_FUNCTION_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/make_function.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"

# include "pxr/external/boost/python/default_call_policies.hpp"
# include "pxr/external/boost/python/args.hpp"
# include "pxr/external/boost/python/detail/caller.hpp"

# include "pxr/external/boost/python/object/function_object.hpp"

# include <boost/mpl/size.hpp>
# include <boost/mpl/int.hpp>

namespace PXR_BOOST_NAMESPACE { namespace python {

namespace detail
{
  // make_function_aux --
  //
  // These helper functions for make_function (below) do the raw work
  // of constructing a Python object from some invokable entity. See
  // "pxr/external/boost/python/detail/caller.hpp" for more information about how
  // the Sig arguments is used.
  template <class F, class CallPolicies, class Sig>
  object make_function_aux(
      F f                               // An object that can be invoked by detail::invoke()
      , CallPolicies const& p           // CallPolicies to use in the invocation
      , Sig const&                      // An MPL sequence of argument types expected by F
      )
  {
      return objects::function_object(
          detail::caller<F,CallPolicies,Sig>(f, p)
      );
  }

  // As above, except that it accepts argument keywords. NumKeywords
  // is used only for a compile-time assertion to make sure the user
  // doesn't pass more keywords than the function can accept. To
  // disable all checking, pass mpl::int_<0> for NumKeywords.
  template <class F, class CallPolicies, class Sig, class NumKeywords>
  object make_function_aux(
      F f
      , CallPolicies const& p
      , Sig const&
      , detail::keyword_range const& kw // a [begin,end) pair of iterators over keyword names
      , NumKeywords                     // An MPL integral type wrapper: the size of kw
      )
  {
      enum { arity = mpl::size<Sig>::value - 1 };
      
      typedef typename detail::error::more_keywords_than_function_arguments<
          NumKeywords::value, arity
          >::too_many_keywords assertion BOOST_ATTRIBUTE_UNUSED;
    
      return objects::function_object(
          detail::caller<F,CallPolicies,Sig>(f, p)
        , kw);
  }

  //   Helpers for make_function when called with 3 arguments.  These
  //   dispatch functions are used to discriminate between the cases
  //   when the 3rd argument is keywords or when it is a signature.
  //
  // @group {
  template <class F, class CallPolicies, class Keywords>
  object make_function_dispatch(F f, CallPolicies const& policies, Keywords const& kw, detail::mpl2::true_)
  {
      return detail::make_function_aux(
          f
        , policies
        , detail::get_signature(f)
        , kw.range()
        , mpl::int_<Keywords::size>()
      );
  }

  template <class F, class CallPolicies, class Signature>
  object make_function_dispatch(F f, CallPolicies const& policies, Signature const& sig, detail::mpl2::false_)
  {
      return detail::make_function_aux(
          f
        , policies
        , sig
      );
  }
  // }
  
 }

//   These overloaded functions wrap a function or member function
//   pointer as a Python object, using optional CallPolicies,
//   Keywords, and/or Signature.
//
//   @group {
template <class F>
object make_function(F f)
{
    return detail::make_function_aux(
        f,default_call_policies(), detail::get_signature(f));
}

template <class F, class CallPolicies>
object make_function(F f, CallPolicies const& policies)
{
    return detail::make_function_aux(
        f, policies, detail::get_signature(f));
}

template <class F, class CallPolicies, class KeywordsOrSignature>
object make_function(
    F f
  , CallPolicies const& policies
  , KeywordsOrSignature const& keywords_or_signature)
{
    typedef typename
        detail::is_reference_to_keywords<KeywordsOrSignature&>::type
        is_kw;
    
    return detail::make_function_dispatch(
        f
      , policies
      , keywords_or_signature
      , is_kw()
    );
}

template <class F, class CallPolicies, class Keywords, class Signature>
object make_function(
    F f
  , CallPolicies const& policies
  , Keywords const& kw
  , Signature const& sig
 )
{
    return detail::make_function_aux(
          f
        , policies
        , sig
        , kw.range()
        , mpl::int_<Keywords::size>()
      );
}
// }

}} 


#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_MAKE_FUNCTION_HPP
