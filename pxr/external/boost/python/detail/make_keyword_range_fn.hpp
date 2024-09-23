//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_MAKE_KEYWORD_RANGE_FN_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_MAKE_KEYWORD_RANGE_FN_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/make_keyword_range_fn.hpp>
#else

# include "pxr/external/boost/python/make_function.hpp"
# include "pxr/external/boost/python/args_fwd.hpp"

# include "pxr/external/boost/python/object/make_holder.hpp"

# include <boost/mpl/size.hpp>


namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail { 

// Think of this as a version of make_function without a compile-time
// check that the size of kw is no greater than the expected arity of
// F. This version is needed when defining functions with default
// arguments, because compile-time information about the number of
// keywords is missing for all but the initial function definition.
//
// @group make_keyword_range_function {
template <class F, class Policies>
object make_keyword_range_function(
    F f
  , Policies const& policies
  , keyword_range const& kw)
{
    return detail::make_function_aux(
        f, policies, detail::get_signature(f), kw, mpl::int_<0>());
}

template <class F, class Policies, class Signature>
object make_keyword_range_function(
    F f
  , Policies const& policies
  , keyword_range const& kw
  , Signature const& sig)
{
    return detail::make_function_aux(
        f, policies, sig, kw, mpl::int_<0>());
}
// }

// Builds an '__init__' function which inserts the given Holder type
// in a wrapped C++ class instance. ArgList is an MPL type sequence
// describing the C++ argument types to be passed to Holder's
// constructor.
//
// Holder and ArgList are intended to be explicitly specified. 
template <class ArgList, class Arity, class Holder, class CallPolicies>
object make_keyword_range_constructor(
    CallPolicies const& policies        // The CallPolicies with which to invoke the Holder's constructor
    , detail::keyword_range const& kw   // The (possibly empty) set of associated argument keywords
    , Holder* = 0                       
    , ArgList* = 0, Arity* = 0)
{
#if !defined( PXR_BOOST_PYTHON_NO_PY_SIGNATURES) && defined( PXR_BOOST_PYTHON_PY_SIGNATURES_PROPER_INIT_SELF_TYPE)
    python_class<BOOST_DEDUCED_TYPENAME Holder::value_type>::register_();
#endif
    return detail::make_keyword_range_function(
        objects::make_holder<Arity::value>
            ::template apply<Holder,ArgList>::execute
        , policies
        , kw);
}

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_MAKE_KEYWORD_RANGE_FN_HPP
