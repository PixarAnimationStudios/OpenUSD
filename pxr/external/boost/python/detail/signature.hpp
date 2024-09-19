//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

# ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_SIGNATURE_HPP
#  define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_SIGNATURE_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/signature.hpp>
#else

#  include "pxr/external/boost/python/type_id.hpp"

#  include "pxr/external/boost/python/detail/preprocessor.hpp"
#  include "pxr/external/boost/python/detail/indirect_traits.hpp"
#  include "pxr/external/boost/python/converter/pytype_function.hpp"

#  include <boost/mpl/at.hpp>
#  include <boost/mpl/size.hpp>

#  include <utility>

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail { 

struct signature_element
{
    char const* basename;
    converter::pytype_function pytype_f;
    bool lvalue;
};

struct py_func_sig_info
{
    signature_element const *signature;
    signature_element const *ret;
};

template <class Idxs> struct signature_arity;

// A metafunction returning the base class used for
//
//   signature<class F, class CallPolicies, class Sig>.
//
template <class Sig>
struct signature_base_select
{
    enum { arity = mpl::size<Sig>::value - 1 };
    typedef typename signature_arity<
        std::make_index_sequence<arity+1>>::template impl<Sig> type;
};

template <class Sig>
struct signature
    : signature_base_select<Sig>::type
{
};

template <size_t... N>
struct signature_arity<std::index_sequence<N...>>
{
    template <class Sig>
    struct impl
    {
        static signature_element const* elements()
        {
            static signature_element const result[sizeof...(N)+1] = {
                
#ifndef PXR_BOOST_PYTHON_NO_PY_SIGNATURES
                {
                  type_id<BOOST_DEDUCED_TYPENAME mpl::at_c<Sig,N>::type>().name()
                  , &converter::expected_pytype_for_arg<BOOST_DEDUCED_TYPENAME mpl::at_c<Sig,N>::type>::get_pytype
                  , indirect_traits::is_reference_to_non_const<BOOST_DEDUCED_TYPENAME mpl::at_c<Sig,N>::type>::value
                }...,
#else
                {
                  type_id<BOOST_DEDUCED_TYPENAME mpl::at_c<Sig,N>::type>().name()
                  , 0
                  , indirect_traits::is_reference_to_non_const<BOOST_DEDUCED_TYPENAME mpl::at_c<Sig,N>::type>::value
                },
#endif
                {0,0,0}
            };
            return result;
        }
    };
};

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
# endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_SIGNATURE_HPP
