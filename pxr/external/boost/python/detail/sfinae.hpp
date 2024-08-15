//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_SFINAE_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_SFINAE_HPP

# include "pxr/external/boost/python/detail/prefix.hpp"

# if defined(BOOST_NO_SFINAE) && !defined(BOOST_MSVC)
#  define BOOST_PYTHON_NO_SFINAE
# endif

#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_SFINAE_HPP
