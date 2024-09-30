//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_MPL2_IDENTITY_HPP
#define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_MPL2_IDENTITY_HPP

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail { namespace mpl2 {

template <typename X>
struct identity
{
    using type = X;
};

}}}} // namespace PXR_BOOST_NAMESPACE::python::detail::mpl2

#endif
