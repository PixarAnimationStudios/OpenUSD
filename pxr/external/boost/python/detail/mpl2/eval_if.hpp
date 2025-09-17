//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_MPL2_EVAL_IF_HPP
#define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_MPL2_EVAL_IF_HPP

#include <type_traits>

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail { namespace mpl2 {

template <bool b, typename F1, typename F2>
struct eval_if_c
{
    using type = typename F1::type;
};

template <typename F1, typename F2>
struct eval_if_c<false, F1, F2>
{
    using type = typename F2::type;
};

template <typename C, typename F1, typename F2>
using eval_if = eval_if_c<C::value, F1, F2>;

}}}} // namespace PXR_BOOST_NAMESPACE::python::detail::mpl2

#endif
