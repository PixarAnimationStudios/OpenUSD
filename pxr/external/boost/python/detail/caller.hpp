//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

# ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_CALLER_HPP
#  define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_CALLER_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/caller.hpp>
#else

#  include "pxr/external/boost/python/type_id.hpp"
#  include "pxr/external/boost/python/handle.hpp"

#  include "pxr/external/boost/python/detail/indirect_traits.hpp"
#  include "pxr/external/boost/python/detail/invoke.hpp"
#  include "pxr/external/boost/python/detail/signature.hpp"
#  include "pxr/external/boost/python/detail/preprocessor.hpp"
#  include "pxr/external/boost/python/detail/type_traits.hpp"

#  include "pxr/external/boost/python/arg_from_python.hpp"
#  include "pxr/external/boost/python/converter/context_result_converter.hpp"
#  include "pxr/external/boost/python/converter/builtin_converters.hpp"

#  include "pxr/external/boost/python/detail/mpl2/front.hpp"
#  include "pxr/external/boost/python/detail/mpl2/eval_if.hpp"
#  include "pxr/external/boost/python/detail/mpl2/identity.hpp"
#  include "pxr/external/boost/python/detail/mpl2/size.hpp"
#  include "pxr/external/boost/python/detail/mpl2/at.hpp"
#  include "pxr/external/boost/python/detail/mpl2/int.hpp"

#  include <type_traits>

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail { 

// Helper for storing a function object and associated call policies,
// applying the empty base class optimization for the call policies
// if possible.
template <
    class Policies, 
    // Apply EBCO if Policies is empty and not final (since we're going
    // to derive from it in the specialization below)
    bool EBCO = std::is_empty_v<Policies> && !std::is_final_v<Policies>
>
struct policies_holder
{
    policies_holder(Policies const& p) : m_p(p) { }
    Policies& second() { return m_p; }
    Policies const& second() const { return m_p; }

private:
    Policies m_p;
};

template <class Policies>
struct policies_holder<Policies, true>
    : protected Policies
{
    policies_holder(Policies const& p) : Policies(p) { }
    Policies& second() { return *this; }
    Policies const& second() const { return *this; }
};

template <class F, class Policies>
struct function_and_policies : policies_holder<Policies>
{
    function_and_policies(F const& f, Policies const& p)
        : policies_holder<Policies>(p), m_f(f) { }

    F& first() { return m_f; }
    F const& first() const { return m_f; }

private:
    F m_f;
};

template <int N>
inline PyObject* get(detail::mpl2::int_<N>, PyObject* const& args_)
{
    return PyTuple_GET_ITEM(args_,N);
}

inline Py_ssize_t arity(PyObject* const& args_)
{
    return PyTuple_GET_SIZE(args_);
}

// This "result converter" is really just used as
// a dispatch tag to invoke(...), selecting the appropriate
// implementation
typedef int void_result_to_python;

// Given a model of CallPolicies and a C++ result type, this
// metafunction selects the appropriate converter to use for
// converting the result to python.
template <class Policies, class Result>
struct select_result_converter
  : mpl2::eval_if<
        is_same<Result,void>
      , detail::mpl2::identity<void_result_to_python>
      , typename Policies::result_converter::template apply<Result>
    >
{
};

template <class ArgPackage, class ResultConverter>
inline ResultConverter create_result_converter(
    ArgPackage const& args_
  , ResultConverter*
  , converter::context_result_converter*
)
{
    return ResultConverter(args_);
}
    
template <class ArgPackage, class ResultConverter>
inline ResultConverter create_result_converter(
    ArgPackage const&
  , ResultConverter*
  , ...
)
{
    return ResultConverter();
}

#ifndef PXR_BOOST_PYTHON_NO_PY_SIGNATURES
template <class ResultConverter>
struct converter_target_type 
{
    static PyTypeObject const *get_pytype()
    {
        return create_result_converter((PyObject*)0, (ResultConverter *)0, (ResultConverter *)0).get_pytype();
    }
};

template < >
struct converter_target_type <void_result_to_python >
{
    static PyTypeObject const *get_pytype()
    {
        return 0;
    }
};

// Generation of ret moved from caller_arity<N>::impl::signature to here due to "feature" in MSVC 15.7.2 with /O2
// which left the ret uninitialized and caused segfaults in Python interpreter.
template<class Policies, class Sig> const signature_element* get_ret()
{
    typedef typename Policies::template extract_return_type<Sig>::type rtype;
    typedef typename select_result_converter<Policies, rtype>::type result_converter;

    static const signature_element ret = {
        (detail::is_void<rtype>::value ? "void" : type_id<rtype>().name())
        , &detail::converter_target_type<result_converter>::get_pytype
        , indirect_traits::is_reference_to_non_const<rtype>::value 
    };

    return &ret;
}

#endif

    
template <class Idxs> struct caller_arity;

template <class F, class CallPolicies, class Sig>
struct caller;


// A metafunction returning the base class used for caller<class F,
// class ConverterGenerators, class CallPolicies, class Sig>.
template <class F, class CallPolicies, class Sig>
struct caller_base_select
{
    enum { arity = detail::mpl2::size<Sig>::value - 1 };
    typedef typename caller_arity<
        std::make_index_sequence<arity>>::template impl<F,CallPolicies,Sig> type;
};

// A function object type which wraps C++ objects as Python callable
// objects.
//
// Template Arguments:
//
//   F -
//      the C++ `function object' that will be called. Might
//      actually be any data for which an appropriate invoke_tag() can
//      be generated. invoke(...) takes care of the actual invocation syntax.
//
//   CallPolicies -
//      The precall, postcall, and what kind of resultconverter to
//      generate for mpl::front<Sig>::type
//
//   Sig -
//      The `intended signature' of the function. An MPL sequence
//      beginning with a result type and continuing with a list of
//      argument types.
template <class F, class CallPolicies, class Sig>
struct caller
    : caller_base_select<F,CallPolicies,Sig>::type
{
    typedef typename caller_base_select<
        F,CallPolicies,Sig
        >::type base;

    typedef PyObject* result_type;
    
    caller(F f, CallPolicies p) : base(f,p) {}

};

template <size_t ...N>
struct caller_arity<std::index_sequence<N...>>
{
    template <class F, class Policies, class Sig>
    struct impl
    {
        impl(F f, Policies p) : m_data(f,p) {}

        PyObject* operator()(PyObject* args_, PyObject*) // eliminate
                                                         // this
                                                         // trailing
                                                         // keyword dict
        {
            typedef typename detail::mpl2::front<Sig>::type result_t;
            typedef typename select_result_converter<Policies, result_t>::type result_converter;
            typedef typename Policies::argument_package argument_package;
            
            argument_package inner_args(args_);

            // N... is a 0-based sequence of indexes corresponding to the
            // expected arguments in args_. However, this must be offset by
            // 1 to retrieve the corresponding argument type from Sig, since
            // that type sequence begins with an additional entry representing
            // the function's return type.
            using arg_from_python_tuple = std::tuple<
                arg_from_python<typename detail::mpl2::at_c<Sig, N+1>::type>...
            >;
            arg_from_python_tuple t{ get(detail::mpl2::int_<N>(), inner_args)... };

            if ( (... || !std::get<N>(t).convertible()) ) {
                return 0;
            }            

            // all converters have been checked. Now we can do the
            // precall part of the policy
            if (!m_data.second().precall(inner_args))
                return 0;

            PyObject* result = detail::invoke(
                detail::invoke_tag<result_t,F>()
              , create_result_converter(args_, (result_converter*)0, (result_converter*)0)
              , m_data.first()
              , std::get<N>(t)...
            );
            
            return m_data.second().postcall(inner_args, result);
        }

        static unsigned min_arity() { return sizeof...(N); }
        
        static py_func_sig_info  signature()
        {
            const signature_element * sig = detail::signature<Sig>::elements();
#ifndef PXR_BOOST_PYTHON_NO_PY_SIGNATURES
            // MSVC 15.7.2, when compiling to /O2 left the static const signature_element ret, 
            // originally defined here, uninitialized. This in turn led to SegFault in Python interpreter.
            // Issue is resolved by moving the generation of ret to separate function in detail namespace (see above).
            const signature_element * ret = detail::get_ret<Policies, Sig>();

            py_func_sig_info res = {sig, ret };
#else
            py_func_sig_info res = {sig, sig };
#endif

            return  res;
        }
     private:
        function_and_policies<F,Policies> m_data;
    };
};

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
# endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_CALLER_HPP
