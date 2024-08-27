//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_OPERATOR_ID_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_OPERATOR_ID_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/operator_id.hpp>
#else

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail {

enum operator_id
{ 
    op_add, 
    op_sub, 
    op_mul, 
    op_div, 
    op_mod, 
    op_divmod,
    op_pow, 
    op_lshift, 
    op_rshift, 
    op_and, 
    op_xor, 
    op_or, 
    op_neg, 
    op_pos, 
    op_abs, 
    op_invert, 
    op_int, 
    op_long, 
    op_float, 
    op_str,
    op_cmp,
    op_gt,
    op_ge,
    op_lt,
    op_le,
    op_eq,
    op_ne,
    op_iadd,
    op_isub,
    op_imul,
    op_idiv,
    op_imod,
    op_ilshift,
    op_irshift,
    op_iand,
    op_ixor,
    op_ior,
    op_complex,
#if PY_VERSION_HEX >= 0x03000000
    op_bool,
#else
    op_nonzero,
#endif
    op_repr
#if PY_VERSION_HEX >= 0x03000000
    ,op_truediv
#endif
};

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_OPERATOR_ID_HPP
