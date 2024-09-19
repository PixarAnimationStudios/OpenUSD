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
#ifndef PXR_EXTERNAL_BOOST_PYTHON_INIT_HPP
#define PXR_EXTERNAL_BOOST_PYTHON_INIT_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/init.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"

#include "pxr/external/boost/python/detail/type_list.hpp"
#include "pxr/external/boost/python/args_fwd.hpp"
#include "pxr/external/boost/python/detail/make_keyword_range_fn.hpp"
#include "pxr/external/boost/python/def_visitor.hpp"

#include "pxr/external/boost/python/detail/mpl2/if.hpp"
#include "pxr/external/boost/python/detail/mpl2/eval_if.hpp"
#include <boost/mpl/size.hpp>
#include <boost/mpl/iterator_range.hpp>
#include <boost/mpl/empty.hpp>
#include <boost/mpl/begin_end.hpp>
#include "pxr/external/boost/python/detail/mpl2/bool.hpp"
#include <boost/mpl/prior.hpp>
#include <boost/mpl/joint_view.hpp>
#include <boost/mpl/back.hpp>

#include "pxr/external/boost/python/detail/type_traits.hpp"

#include <utility>

namespace PXR_BOOST_NAMESPACE { namespace python {

template <class... T>
class init; // forward declaration


template <class... T>
struct optional; // forward declaration

namespace detail
{
  namespace error
  {
    template <int keywords, int init_args>
    struct more_keywords_than_init_arguments
    {
        typedef char too_many_keywords[init_args - keywords >= 0 ? 1 : -1] BOOST_ATTRIBUTE_UNUSED;
    };
  }

  //  is_optional<T>::value
  //
  //      This metaprogram checks if T is an optional
  //

    template <class T>
    struct is_optional
      : detail::mpl2::false_
    {};

    template <class... T>
    struct is_optional<optional<T...> >
      : detail::mpl2::true_
    {};
  

  template <int NDefaults>
  struct define_class_init_helper;

} // namespace detail

template <class DerivedT>
struct init_base : def_visitor<DerivedT>
{
    init_base(char const* doc_, detail::keyword_range const& keywords_)
        : m_doc(doc_), m_keywords(keywords_)
    {}
        
    init_base(char const* doc_)
        : m_doc(doc_)
    {}

    DerivedT const& derived() const
    {
        return *static_cast<DerivedT const*>(this);
    }
    
    char const* doc_string() const
    {
        return m_doc;
    }

    detail::keyword_range const& keywords() const
    {
        return m_keywords;
    }

    static default_call_policies call_policies()
    {
        return default_call_policies();
    }

 private:
    //  visit
    //
    //      Defines a set of n_defaults + 1 constructors for its
    //      class_<...> argument. Each constructor after the first has
    //      one less argument to its right. Example:
    //
    //          init<int, optional<char, long, double> >
    //
    //      Defines:
    //
    //          __init__(int, char, long, double)
    //          __init__(int, char, long)
    //          __init__(int, char)
    //          __init__(int)
    template <class classT>
    void visit(classT& cl) const
    {
        typedef typename DerivedT::signature signature;
        typedef typename DerivedT::n_arguments n_arguments;
        typedef typename DerivedT::n_defaults n_defaults;
    
        detail::define_class_init_helper<n_defaults::value>::apply(
            cl
          , derived().call_policies()
          , signature()
          , n_arguments()
          , derived().doc_string()
          , derived().keywords());
    }
    
    friend class python::def_visitor_access;
    
 private: // data members
    char const* m_doc;
    detail::keyword_range m_keywords;
};

template <class CallPoliciesT, class InitT>
class init_with_call_policies
    : public init_base<init_with_call_policies<CallPoliciesT, InitT> >
{
    typedef init_base<init_with_call_policies<CallPoliciesT, InitT> > base;
 public:
    typedef typename InitT::n_arguments n_arguments;
    typedef typename InitT::n_defaults n_defaults;
    typedef typename InitT::signature signature;

    init_with_call_policies(
        CallPoliciesT const& policies_
        , char const* doc_
        , detail::keyword_range const& keywords
        )
        : base(doc_, keywords)
        , m_policies(policies_)
    {}

    CallPoliciesT const& call_policies() const
    {
        return this->m_policies;
    }
    
 private: // data members
    CallPoliciesT m_policies;
};

//
// drop1<S> is the initial length(S) elements of S
//
namespace detail
{
  template <class S>
  struct drop1
    : mpl::iterator_range<
          typename mpl::begin<S>::type
        , typename mpl::prior<
              typename mpl::end<S>::type
          >::type
      >
  {};
}

template <class... T>
class init : public init_base<init<T...> >
{
    typedef init_base<init<T...> > base;
 public:
    typedef init<T...> self_t;

    init(char const* doc_ = 0)
        : base(doc_)
    {
    }

    template <std::size_t N>
    init(char const* doc_, detail::keywords<N> const& kw)
        : base(doc_, kw.range())
    {
        typedef typename detail::error::more_keywords_than_init_arguments<
            N, n_arguments::value + 1
            >::too_many_keywords assertion BOOST_ATTRIBUTE_UNUSED;
    }

    template <std::size_t N>
    init(detail::keywords<N> const& kw, char const* doc_ = 0)
        : base(doc_, kw.range())
    {
        typedef typename detail::error::more_keywords_than_init_arguments<
            N, n_arguments::value + 1
            >::too_many_keywords assertion BOOST_ATTRIBUTE_UNUSED;
    }

    template <class CallPoliciesT>
    init_with_call_policies<CallPoliciesT, self_t>
    operator[](CallPoliciesT const& policies) const
    {
        return init_with_call_policies<CallPoliciesT, self_t>(
            policies, this->doc_string(), this->keywords());
    }

    typedef detail::type_list<T...> signature_;

    typedef detail::is_optional<
        typename detail::mpl2::eval_if<
            mpl::empty<signature_>
          , detail::mpl2::false_
          , mpl::back<signature_>
        >::type
    > back_is_optional;
    
    typedef typename detail::mpl2::eval_if<
        back_is_optional
      , mpl::back<signature_>
      , mpl::vector0<>
    >::type optional_args;

    typedef typename detail::mpl2::eval_if<
        back_is_optional
      , detail::mpl2::if_<
            mpl::empty<optional_args>
          , detail::drop1<signature_>
          , mpl::joint_view<
                detail::drop1<signature_>
              , optional_args
            >
        >
      , signature_
    >::type signature;

    // TODO: static assert to make sure there are no other optional elements

    // Count the number of default args
    typedef mpl::size<optional_args> n_defaults;
    typedef mpl::size<signature> n_arguments;
};

///////////////////////////////////////////////////////////////////////////////
//
//  optional
//
//      optional<T0...TN>::type returns a typelist.
//
///////////////////////////////////////////////////////////////////////////////
template <class... T>
struct optional
    : detail::type_list<T...>
{
};

namespace detail
{
  template <class ClassT, class CallPoliciesT, class Signature, class NArgs>
  inline void def_init_aux(
      ClassT& cl
      , Signature const&
      , NArgs
      , CallPoliciesT const& policies
      , char const* doc
      , detail::keyword_range const& keywords_
      )
  {
      cl.def(
          "__init__"
        , detail::make_keyword_range_constructor<Signature,NArgs>(
              policies
            , keywords_
            , (typename ClassT::metadata::holder*)0
          )
        , doc
      );
  }

  ///////////////////////////////////////////////////////////////////////////////
  //
  //  define_class_init_helper<N>::apply
  //
  //      General case
  //
  //      Accepts a class_ and an arguments list. Defines a constructor
  //      for the class given the arguments and recursively calls
  //      define_class_init_helper<N-1>::apply with one fewer argument (the
  //      rightmost argument is shaved off)
  //
  ///////////////////////////////////////////////////////////////////////////////
  template <int NDefaults>
  struct define_class_init_helper
  {

      template <class ClassT, class CallPoliciesT, class Signature, class NArgs>
      static void apply(
          ClassT& cl
          , CallPoliciesT const& policies
          , Signature const& args
          , NArgs
          , char const* doc
          , detail::keyword_range keywords)
      {
          detail::def_init_aux(cl, args, NArgs(), policies, doc, keywords);

          if (keywords.second > keywords.first)
              --keywords.second;

          typedef typename mpl::prior<NArgs>::type next_nargs;
          define_class_init_helper<NDefaults-1>::apply(
              cl, policies, Signature(), next_nargs(), doc, keywords);
      }
  };

  ///////////////////////////////////////////////////////////////////////////////
  //
  //  define_class_init_helper<0>::apply
  //
  //      Terminal case
  //
  //      Accepts a class_ and an arguments list. Defines a constructor
  //      for the class given the arguments.
  //
  ///////////////////////////////////////////////////////////////////////////////
  template <>
  struct define_class_init_helper<0> {

      template <class ClassT, class CallPoliciesT, class Signature, class NArgs>
      static void apply(
          ClassT& cl
        , CallPoliciesT const& policies
        , Signature const& args
        , NArgs
        , char const* doc
        , detail::keyword_range const& keywords)
      {
          detail::def_init_aux(cl, args, NArgs(), policies, doc, keywords);
      }
  };
}

}} // namespace PXR_BOOST_NAMESPACE::python

///////////////////////////////////////////////////////////////////////////////
#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_INIT_HPP








