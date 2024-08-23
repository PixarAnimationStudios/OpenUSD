//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_IS_AUTO_PTR_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_IS_AUTO_PTR_HPP

#include <type_traits>

namespace boost { namespace python { namespace detail { 

template <class T>
struct is_auto_ptr : std::false_type
{
};

}}} // namespace boost::python::detail

#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_IS_AUTO_PTR_HPP
