//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_MPL2_POP_FRONT_HPP
#define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_MPL2_POP_FRONT_HPP

#include "pxr/external/boost/python/type_list.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail { namespace mpl2 {

template <typename Sequence>
struct pop_front : pop_front<typename Sequence::type>
{
};

template <typename T0, typename... T>
struct pop_front<python::type_list<T0, T...>>
{
    using type = python::type_list<T...>;
};

}}}} // namespace PXR_BOOST_NAMESPACE::python::detail::mpl2

#endif
