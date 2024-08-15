//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_SCOPE_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_SCOPE_HPP

# include "pxr/external/boost/python/detail/config.hpp"

namespace boost { namespace python { namespace detail {

void BOOST_PYTHON_DECL scope_setattr_doc(char const* name, object const& obj, char const* doc);

}}} // namespace boost::python::detail

#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_SCOPE_HPP
