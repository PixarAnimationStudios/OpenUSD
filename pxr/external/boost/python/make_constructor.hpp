//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_MAKE_CONSTRUCTOR_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_MAKE_CONSTRUCTOR_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/make_constructor.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"

# include "pxr/external/boost/python/default_call_policies.hpp"
# include "pxr/external/boost/python/args.hpp"
# include "pxr/external/boost/python/object_fwd.hpp"

# include "pxr/external/boost/python/object/function_object.hpp"
# include "pxr/external/boost/python/object/make_holder.hpp"
# include "pxr/external/boost/python/object/pointer_holder.hpp"
# include "pxr/external/boost/python/converter/context_result_converter.hpp"

# include "pxr/external/boost/python/detail/caller.hpp"
# include "pxr/external/boost/python/detail/none.hpp"

# include "pxr/external/boost/python/detail/mpl2/size.hpp"
# include "pxr/external/boost/python/detail/mpl2/int.hpp"
# include "pxr/external/boost/python/detail/mpl2/push_front.hpp"
# include "pxr/external/boost/python/detail/mpl2/pop_front.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python {

namespace detail
{
  template <class T>
  struct install_holder : converter::context_result_converter
  {
      install_holder(PyObject* args_)
        : m_self(PyTuple_GetItem(args_, 0)) {}

      PyObject* operator()(T x) const
      {
          dispatch(x, is_pointer<T>());
          return none();
      }

   private:
      template <class U>
      void dispatch(U* x, detail::true_) const
      {
	std::unique_ptr<U> owner(x);
	dispatch(std::move(owner), detail::false_());
      }
      
      template <class Ptr>
      void dispatch(Ptr x, detail::false_) const
      {
          typedef typename pointee<Ptr>::type value_type;
          typedef objects::pointer_holder<Ptr,value_type> holder;
          typedef objects::instance<holder> instance_t;

          void* memory = holder::allocate(this->m_self, offsetof(instance_t, storage), sizeof(holder));
          try {
              (new (memory) holder(std::move(x)))->install(this->m_self);
          }
          catch(...) {
              holder::deallocate(this->m_self, memory);
              throw;
          }
      }
      
      PyObject* m_self;
  };
  
  struct constructor_result_converter
  {
      template <class T>
      struct apply
      {
          typedef install_holder<T> type;
      };
  };

  template <class BaseArgs, class Offset>
  struct offset_args
  {
      offset_args(BaseArgs base_) : base(base_) {}
      BaseArgs base;
  };

  template <int N, class BaseArgs, class Offset>
  inline PyObject* get(detail::mpl2::int_<N>, offset_args<BaseArgs,Offset> const& args_)
  {
      return get(detail::mpl2::int_<(N+Offset::value)>(), args_.base);
  }
  
  template <class BaseArgs, class Offset>
  inline unsigned arity(offset_args<BaseArgs,Offset> const& args_)
  {
      return arity(args_.base) - Offset::value;
  }

  template <class BasePolicy_ = default_call_policies>
  struct constructor_policy : BasePolicy_
  {
      constructor_policy(BasePolicy_ base) : BasePolicy_(base) {}
      
      // If the BasePolicy_ supplied a result converter it would be
      // ignored; issue an error if it's not the default.
      static_assert(
         (is_same<
              typename BasePolicy_::result_converter
            , default_result_converter
          >::value)
        , "MAKE_CONSTRUCTOR_SUPPLIES_ITS_OWN_RESULT_CONVERTER_THAT_WOULD_OVERRIDE_YOURS"
      );
      typedef constructor_result_converter result_converter;
      typedef offset_args<typename BasePolicy_::argument_package, detail::mpl2::int_<1> > argument_package;
  };

  template <class InnerSignature>
  struct outer_constructor_signature
  {
      typedef typename detail::mpl2::pop_front<InnerSignature>::type inner_args;
      typedef typename detail::mpl2::push_front<inner_args,object>::type outer_args;
      typedef typename detail::mpl2::push_front<outer_args,void>::type type;
  };

  // ETI workaround
  template <>
  struct outer_constructor_signature<int>
  {
      typedef int type;
  };
  
  //
  // These helper functions for make_constructor (below) do the raw work
  // of constructing a Python object from some invokable entity. See
  // "pxr/external/boost/python/detail/caller.hpp" for more information about how
  // the Sig arguments is used.
  //
  // @group make_constructor_aux {
  template <class F, class CallPolicies, class Sig>
  object make_constructor_aux(
      F f                             // An object that can be invoked by detail::invoke()
    , CallPolicies const& p           // CallPolicies to use in the invocation
    , Sig const&                      // An MPL sequence of argument types expected by F
  )
  {
      typedef typename outer_constructor_signature<Sig>::type outer_signature;

      typedef constructor_policy<CallPolicies> inner_policy;
      
      return objects::function_object(
          objects::py_function(
              detail::caller<F,inner_policy,Sig>(f, inner_policy(p))
            , outer_signature()
          )
      );
  }
  
  // As above, except that it accepts argument keywords. NumKeywords
  // is used only for a compile-time assertion to make sure the user
  // doesn't pass more keywords than the function can accept. To
  // disable all checking, pass detail::mpl2::int_<0> for NumKeywords.
  template <class F, class CallPolicies, class Sig, class NumKeywords>
  object make_constructor_aux(
      F f
      , CallPolicies const& p
      , Sig const&
      , detail::keyword_range const& kw // a [begin,end) pair of iterators over keyword names
      , NumKeywords                     // An MPL integral type wrapper: the size of kw
      )
  {
      enum { arity = detail::mpl2::size<Sig>::value - 1 };
      
      [[maybe_unused]] typedef typename detail::error::more_keywords_than_function_arguments<
          NumKeywords::value, arity
          >::too_many_keywords assertion;
    
      typedef typename outer_constructor_signature<Sig>::type outer_signature;

      typedef constructor_policy<CallPolicies> inner_policy;
      
      return objects::function_object(
          objects::py_function(
              detail::caller<F,inner_policy,Sig>(f, inner_policy(p))
            , outer_signature()
          )
          , kw
      );
  }
  // }

  //
  //   These dispatch functions are used to discriminate between the
  //   cases when the 3rd argument is keywords or when it is a
  //   signature.
  //
  //   @group Helpers for make_constructor when called with 3 arguments. {
  //
  template <class F, class CallPolicies, class Keywords>
  object make_constructor_dispatch(F f, CallPolicies const& policies, Keywords const& kw, detail::mpl2::true_)
  {
      return detail::make_constructor_aux(
          f
        , policies
        , detail::get_signature(f)
        , kw.range()
        , detail::mpl2::int_<Keywords::size>()
      );
  }

  template <class F, class CallPolicies, class Signature>
  object make_constructor_dispatch(F f, CallPolicies const& policies, Signature const& sig, detail::mpl2::false_)
  {
      return detail::make_constructor_aux(
          f
        , policies
        , sig
      );
  }
  // }
}

//   These overloaded functions wrap a function or member function
//   pointer as a Python object, using optional CallPolicies,
//   Keywords, and/or Signature. @group {
//
template <class F>
object make_constructor(F f)
{
    return detail::make_constructor_aux(
        f,default_call_policies(), detail::get_signature(f));
}

template <class F, class CallPolicies>
object make_constructor(F f, CallPolicies const& policies)
{
    return detail::make_constructor_aux(
        f, policies, detail::get_signature(f));
}

template <class F, class CallPolicies, class KeywordsOrSignature>
object make_constructor(
    F f
  , CallPolicies const& policies
  , KeywordsOrSignature const& keywords_or_signature)
{
    typedef typename
        detail::is_reference_to_keywords<KeywordsOrSignature&>::type
        is_kw;
    
    return detail::make_constructor_dispatch(
        f
      , policies
      , keywords_or_signature
      , is_kw()
    );
}

template <class F, class CallPolicies, class Keywords, class Signature>
object make_constructor(
    F f
  , CallPolicies const& policies
  , Keywords const& kw
  , Signature const& sig
 )
{
    return detail::make_constructor_aux(
          f
        , policies
        , sig
        , kw.range()
        , detail::mpl2::int_<Keywords::size>()
      );
}
// }

}} 


#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_MAKE_CONSTRUCTOR_HPP
