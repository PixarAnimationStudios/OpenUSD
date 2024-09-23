//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_MPL2_OR_HPP
#define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_MPL2_OR_HPP

#include <type_traits>

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail { namespace mpl2 {

template <typename ...F>
using or_ = std::disjunction<F...>;

}}}} // namespace PXR_BOOST_NAMESPACE::python::detail::mpl2

#endif
