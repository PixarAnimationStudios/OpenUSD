//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_FORCE_INSTANTIATE_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_FORCE_INSTANTIATE_HPP

namespace boost { namespace python { namespace detail { 

// Allows us to force the argument to be instantiated without
// incurring unused variable warnings

template <class T>
inline void force_instantiate(T const&) {}

}}} // namespace boost::python::detail

#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_FORCE_INSTANTIATE_HPP
