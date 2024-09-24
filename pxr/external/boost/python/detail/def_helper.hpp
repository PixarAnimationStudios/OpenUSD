//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_DEF_HELPER_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_DEF_HELPER_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/def_helper.hpp>
#else

# include "pxr/external/boost/python/args.hpp"
# include "pxr/external/boost/python/detail/indirect_traits.hpp"
# include "pxr/external/boost/python/detail/type_traits.hpp"
# include "pxr/external/boost/python/detail/mpl2/not.hpp"
# include "pxr/external/boost/python/detail/mpl2/and.hpp"
# include "pxr/external/boost/python/detail/mpl2/or.hpp"
# include "pxr/external/boost/python/detail/not_specified.hpp"
# include "pxr/external/boost/python/detail/def_helper_fwd.hpp"

# include <tuple>
# include <utility>

namespace PXR_BOOST_NAMESPACE { namespace python {

struct default_call_policies;

namespace detail
{
  // tuple_extract<Tuple,Predicate>::extract(t) returns the first
  // element of a Tuple whose type E satisfies the given Predicate
  // applied to add_reference<E>. The Predicate must be an MPL
  // metafunction class.
  template <class Tuple, template <typename> class Predicate>
  struct tuple_extract;

  template <class... T, template <typename> class Predicate>
  struct tuple_extract<std::tuple<T...>, Predicate>
  {
      template <class U>
      using match_t = Predicate<typename add_lvalue_reference<U>::type>;

      static constexpr size_t compute_index()
      {
          size_t idx = 0;
          ((match_t<T>::value ? true : (++idx, false)) || ...);
          return idx;
      }

      static constexpr size_t match_index = compute_index();
      static_assert(match_index < sizeof...(T), "No matches for predicate");

      using tuple_type = std::tuple<T...>;
      using result_type = 
          typename std::tuple_element<match_index, tuple_type>::type;

      static result_type extract(tuple_type const& x)
      {
          return std::get<match_index>(x);
      }
  };

  //
  // Specialized extractors for the docstring, keywords, CallPolicies,
  // and default implementation of virtual functions
  //

  template <class T>
  struct doc_extract_pred
      : mpl2::not_<
          mpl2::or_<
              indirect_traits::is_reference_to_class<T>
            , indirect_traits::is_reference_to_member_function_pointer<T>
          >
      >
  {
  };

  template <class Tuple>
  struct doc_extract
      : tuple_extract<Tuple, doc_extract_pred>
  {
  };
  
  template <class Tuple>
  struct keyword_extract
      : tuple_extract<Tuple, is_reference_to_keywords>
  {
  };

  template <class T>
  struct policy_extract_pred
      : mpl2::and_<
          mpl2::not_<std::is_same<not_specified const&, T> >
        , indirect_traits::is_reference_to_class<T>
        , mpl2::not_<is_reference_to_keywords<T> >
      >
  {
  };

  template <class Tuple>
  struct policy_extract
      : tuple_extract<Tuple, policy_extract_pred>
  {
  };

  template <class Tuple>
  struct default_implementation_extract
      : tuple_extract<
          Tuple, indirect_traits::is_reference_to_member_function_pointer
      >
  {
  };

  //
  // A helper class for decoding the optional arguments to def()
  // invocations, which can be supplied in any order and are
  // discriminated by their type properties. The template parameters
  // are expected to be the types of the actual (optional) arguments
  // passed to def().
  //
  template <class T1, class T2, class T3, class T4>
  struct def_helper
  {
      // A tuple type which begins with references to the supplied
      // arguments and ends with actual representatives of the default
      // types.
      typedef std::tuple<
          T1 const&
          , T2 const&
          , T3 const&
          , T4 const&
          , default_call_policies
          , detail::keywords<0>
          , char const*
          , void(not_specified::*)()   // A function pointer type which is never an
                                       // appropriate default implementation
          > all_t;

      // Constructors; these initialize an member of the tuple type
      // shown above.
      def_helper(T1 const& a1) : m_all(a1,m_nil,m_nil,m_nil,{},{},{},{}) {}
      def_helper(T1 const& a1, T2 const& a2) : m_all(a1,a2,m_nil,m_nil,{},{},{},{}) {}
      def_helper(T1 const& a1, T2 const& a2, T3 const& a3) : m_all(a1,a2,a3,m_nil,{},{},{},{}) {}
      def_helper(T1 const& a1, T2 const& a2, T3 const& a3, T4 const& a4) : m_all(a1,a2,a3,a4,{},{},{},{}) {}

   private: // types
      typedef typename default_implementation_extract<all_t>::result_type default_implementation_t;
      
   public: // Constants which can be used for static assertions.

      // Users must not supply a default implementation for non-class
      // methods.
      BOOST_STATIC_CONSTANT(
          bool, has_default_implementation = (
              !is_same<default_implementation_t, void(not_specified::*)()>::value));
      
   public: // Extractor functions which pull the appropriate value out
           // of the tuple
      char const* doc() const
      {
          return doc_extract<all_t>::extract(m_all);
      }
      
      typename keyword_extract<all_t>::result_type keywords() const
      {
          return keyword_extract<all_t>::extract(m_all);
      }
      
      typename policy_extract<all_t>::result_type policies() const
      {
          return policy_extract<all_t>::extract(m_all);
      }

      default_implementation_t default_implementation() const
      {
          return default_implementation_extract<all_t>::extract(m_all);
      }
      
   private: // data members
      all_t m_all; 
      not_specified m_nil; // for filling in not_specified slots
  };
}

}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_DEF_HELPER_HPP
