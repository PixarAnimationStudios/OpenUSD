//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_PREPROCESSOR_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_PREPROCESSOR_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/preprocessor.hpp>
#else

# include <boost/preprocessor/cat.hpp>
# include <boost/preprocessor/comma_if.hpp>
# include <boost/preprocessor/repeat.hpp>
# include <boost/preprocessor/tuple/elem.hpp>

// stuff that should be in the preprocessor library

# define PXR_BOOST_PYTHON_APPLY(x) BOOST_PP_CAT(PXR_BOOST_PYTHON_APPLY_, x)

# define PXR_BOOST_PYTHON_APPLY_PXR_BOOST_PYTHON_ITEM(v) v
# define PXR_BOOST_PYTHON_APPLY_PXR_BOOST_PYTHON_NIL

// cv-qualifiers

# if !defined(__MWERKS__) || __MWERKS__ > 0x2407
#  define PXR_BOOST_PYTHON_CV_COUNT 4
# else
#  define PXR_BOOST_PYTHON_CV_COUNT 1
# endif

# ifndef PXR_BOOST_PYTHON_MAX_ARITY
#  define PXR_BOOST_PYTHON_MAX_ARITY 15
# endif

# ifndef PXR_BOOST_PYTHON_MAX_BASES
#  define PXR_BOOST_PYTHON_MAX_BASES 10
# endif 

# define PXR_BOOST_PYTHON_CV_QUALIFIER(i)                          \
    PXR_BOOST_PYTHON_APPLY(                                        \
        BOOST_PP_TUPLE_ELEM(4, i, PXR_BOOST_PYTHON_CV_QUALIFIER_I) \
    )

# define PXR_BOOST_PYTHON_CV_QUALIFIER_I      \
    (                                     \
        PXR_BOOST_PYTHON_NIL,                 \
        PXR_BOOST_PYTHON_ITEM(const),         \
        PXR_BOOST_PYTHON_ITEM(volatile),      \
        PXR_BOOST_PYTHON_ITEM(const volatile) \
    )

// enumerators
# define PXR_BOOST_PYTHON_UNARY_ENUM(c, text) BOOST_PP_REPEAT(c, PXR_BOOST_PYTHON_UNARY_ENUM_I, text)
# define PXR_BOOST_PYTHON_UNARY_ENUM_I(z, n, text) BOOST_PP_COMMA_IF(n) text ## n

# define PXR_BOOST_PYTHON_BINARY_ENUM(c, a, b) BOOST_PP_REPEAT(c, PXR_BOOST_PYTHON_BINARY_ENUM_I, (a, b))
# define PXR_BOOST_PYTHON_BINARY_ENUM_I(z, n, _) BOOST_PP_COMMA_IF(n) BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(2, 0, _), n) BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(2, 1, _), n)

# define PXR_BOOST_PYTHON_ENUM_WITH_DEFAULT(c, text, def) BOOST_PP_REPEAT(c, PXR_BOOST_PYTHON_ENUM_WITH_DEFAULT_I, (text, def))
# define PXR_BOOST_PYTHON_ENUM_WITH_DEFAULT_I(z, n, _) BOOST_PP_COMMA_IF(n) BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(2, 0, _), n) = BOOST_PP_TUPLE_ELEM(2, 1, _)

// fixed text (no commas)
# define PXR_BOOST_PYTHON_FIXED(z, n, text) text

// flags
# define PXR_BOOST_PYTHON_FUNCTION_POINTER 0x0001
# define PXR_BOOST_PYTHON_POINTER_TO_MEMBER 0x0002

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_PREPROCESSOR_HPP
