//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_PURE_VIRTUAL_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_PURE_VIRTUAL_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/pure_virtual.hpp>
#else

# include "pxr/external/boost/python/def_visitor.hpp"
# include "pxr/external/boost/python/default_call_policies.hpp"
# include "pxr/external/boost/python/detail/mpl2/push_front.hpp"
# include "pxr/external/boost/python/detail/mpl2/pop_front.hpp"

# include "pxr/external/boost/python/detail/nullary_function_adaptor.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { 

namespace detail
{
  //
  // @group Helpers for pure_virtual_visitor. {
  //
  
  // Raises a Python RuntimeError reporting that a pure virtual
  // function was called.
  void PXR_BOOST_PYTHON_DECL pure_virtual_called();

  // Replace the two front elements of S with T1 and T2
  template <class S, class T1, class T2>
  struct replace_front2
  {
      // Metafunction forwarding seemed to confound vc6 
      typedef typename detail::mpl2::push_front<
          typename detail::mpl2::push_front<
              typename detail::mpl2::pop_front<
                  typename detail::mpl2::pop_front<
                      S
                  >::type
              >::type
            , T2
          >::type
        , T1
      >::type type;
  };

  // Given an MPL sequence representing a member function [object]
  // signature, returns a new MPL sequence whose return type is
  // replaced by void, and whose first argument is replaced by C&.
  template <class C, class S>
  typename replace_front2<S,void,C&>::type
  error_signature(S)
  {
      typedef typename replace_front2<S,void,C&>::type r;
      return r();
  }

  //
  // } 
  //

  //
  // A def_visitor which defines a method as usual, then adds a
  // corresponding function which raises a "pure virtual called"
  // exception unless it's been overridden.
  //
  template <class PointerToMemberFunction>
  struct pure_virtual_visitor
    : def_visitor<pure_virtual_visitor<PointerToMemberFunction> >
  {
      pure_virtual_visitor(PointerToMemberFunction pmf)
        : m_pmf(pmf)
      {}
      
   private:
      friend class python::def_visitor_access;
      
      template <class C_, class Options>
      void visit(C_& c, char const* name, Options& options) const
      {
          // This should probably be a nicer error message
          static_assert(!Options::has_default_implementation);

          // Add the virtual function dispatcher
          c.def(
              name
            , m_pmf
            , options.doc()
            , options.keywords()
            , options.policies()
          );

          typedef typename C_::metadata::held_type held_type;
          
          // Add the default implementation which raises the exception
          c.def(
              name
            , make_function(
                  detail::nullary_function_adaptor<void(*)()>(pure_virtual_called)
                , default_call_policies()
                , detail::error_signature<held_type>(detail::get_signature(m_pmf))
              )
          );
      }
      
   private: // data members
      PointerToMemberFunction m_pmf;
  };
}

//
// Passed a pointer to member function, generates a def_visitor which
// creates a method that only dispatches to Python if the function has
// been overridden, either in C++ or in Python, raising a "pure
// virtual called" exception otherwise.
//
template <class PointerToMemberFunction>
detail::pure_virtual_visitor<PointerToMemberFunction>
pure_virtual(PointerToMemberFunction pmf)
{
    return detail::pure_virtual_visitor<PointerToMemberFunction>(pmf);
}

}} // namespace PXR_BOOST_NAMESPACE::python

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_PURE_VIRTUAL_HPP
