#ifndef BOOST_PP_IS_ITERATING
//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
# ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_TYPE_LIST_IMPL_HPP
#  define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_TYPE_LIST_IMPL_HPP

#  include "pxr/external/boost/python/detail/type_list.hpp"

#  include <boost/preprocessor/enum_params.hpp>
#  include <boost/preprocessor/enum_params_with_a_default.hpp>
#  include <boost/preprocessor/repetition/enum.hpp>
#  include <boost/preprocessor/comma_if.hpp>
#  include <boost/preprocessor/arithmetic/sub.hpp>
#  include <boost/preprocessor/iterate.hpp>
#  include <boost/preprocessor/repetition/enum_trailing.hpp>

namespace boost { namespace python { namespace detail { 

template <BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(BOOST_PYTHON_LIST_SIZE, class T, mpl::void_)>
struct type_list
    : BOOST_PP_CAT(mpl::vector,BOOST_PYTHON_LIST_SIZE)<BOOST_PP_ENUM_PARAMS_Z(1, BOOST_PYTHON_LIST_SIZE, T)>
{
};

#  define BOOST_PP_ITERATION_PARAMS_1                                                                   \
        (3, (0, BOOST_PP_DEC(BOOST_PYTHON_LIST_SIZE), "pxr/external/boost/python/detail/type_list_impl.hpp"))
#  include BOOST_PP_ITERATE()


}}} // namespace boost::python::detail

# endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_TYPE_LIST_IMPL_HPP

#else // BOOST_PP_IS_ITERATING

# define N BOOST_PP_ITERATION()
# define BOOST_PYTHON_VOID_ARGS BOOST_PP_SUB_D(1,BOOST_PYTHON_LIST_SIZE,N)

template <
    BOOST_PP_ENUM_PARAMS_Z(1, N, class T)
    >
struct type_list<
    BOOST_PP_ENUM_PARAMS_Z(1, N, T)
    BOOST_PP_COMMA_IF(N)
    BOOST_PP_ENUM(
        BOOST_PYTHON_VOID_ARGS, BOOST_PYTHON_FIXED, mpl::void_)
    >
   : BOOST_PP_CAT(mpl::vector,N)<BOOST_PP_ENUM_PARAMS_Z(1, N, T)>
{
};

# undef BOOST_PYTHON_VOID_ARGS
# undef N

#endif // BOOST_PP_IS_ITERATING 
