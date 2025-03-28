//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_MPL2_IF_HPP
#define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_MPL2_IF_HPP

#include <type_traits>

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail { namespace mpl2 {

template <typename C, typename T1, typename T2>
using if_ = std::conditional<C::value, T1, T2>;

template <bool c, typename T1, typename T2>
using if_c = std::conditional<c, T1, T2>;

}}}} // namespace PXR_BOOST_NAMESPACE::python::detail::mpl2

#endif
