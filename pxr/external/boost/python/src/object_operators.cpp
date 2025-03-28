//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "pxr/external/boost/python/object_operators.hpp"
#include "pxr/external/boost/python/detail/raw_pyobject.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { namespace api {

# define PXR_BOOST_PYTHON_COMPARE_OP(op, opid)                              \
PXR_BOOST_PYTHON_DECL object operator op(object const& l, object const& r)  \
{                                                                       \
    return object(                                                      \
        detail::new_reference(                                          \
            PyObject_RichCompare(                                       \
                l.ptr(), r.ptr(), opid))                                \
            );                                                          \
}
PXR_BOOST_PYTHON_COMPARE_OP(>, Py_GT)
PXR_BOOST_PYTHON_COMPARE_OP(>=, Py_GE)
PXR_BOOST_PYTHON_COMPARE_OP(<, Py_LT)
PXR_BOOST_PYTHON_COMPARE_OP(<=, Py_LE)
PXR_BOOST_PYTHON_COMPARE_OP(==, Py_EQ)
PXR_BOOST_PYTHON_COMPARE_OP(!=, Py_NE)
# undef PXR_BOOST_PYTHON_COMPARE_OP
    

#define PXR_BOOST_PYTHON_BINARY_OPERATOR(op, name)                          \
PXR_BOOST_PYTHON_DECL object operator op(object const& l, object const& r)  \
{                                                                       \
    return object(                                                      \
        detail::new_reference(                                          \
            PyNumber_##name(l.ptr(), r.ptr()))                          \
        );                                                              \
}

PXR_BOOST_PYTHON_BINARY_OPERATOR(+, Add)
PXR_BOOST_PYTHON_BINARY_OPERATOR(-, Subtract)
PXR_BOOST_PYTHON_BINARY_OPERATOR(*, Multiply)
#if PY_VERSION_HEX >= 0x03000000
// We choose FloorDivide instead of TrueDivide to keep the semantic
// conform with C/C++'s '/' operator
PXR_BOOST_PYTHON_BINARY_OPERATOR(/, FloorDivide)
#else
PXR_BOOST_PYTHON_BINARY_OPERATOR(/, Divide)
#endif
PXR_BOOST_PYTHON_BINARY_OPERATOR(%, Remainder)
PXR_BOOST_PYTHON_BINARY_OPERATOR(<<, Lshift)
PXR_BOOST_PYTHON_BINARY_OPERATOR(>>, Rshift)
PXR_BOOST_PYTHON_BINARY_OPERATOR(&, And)
PXR_BOOST_PYTHON_BINARY_OPERATOR(^, Xor)
PXR_BOOST_PYTHON_BINARY_OPERATOR(|, Or)
#undef PXR_BOOST_PYTHON_BINARY_OPERATOR

#define PXR_BOOST_PYTHON_INPLACE_OPERATOR(op, name)                         \
PXR_BOOST_PYTHON_DECL object& operator op##=(object& l, object const& r)    \
{                                                                       \
    return l = object(                                                  \
        (detail::new_reference)                                         \
            PyNumber_InPlace##name(l.ptr(), r.ptr()));                  \
}
    
PXR_BOOST_PYTHON_INPLACE_OPERATOR(+, Add)
PXR_BOOST_PYTHON_INPLACE_OPERATOR(-, Subtract)
PXR_BOOST_PYTHON_INPLACE_OPERATOR(*, Multiply)
#if PY_VERSION_HEX >= 0x03000000
// Same reason as above for choosing FloorDivide instead of TrueDivide
PXR_BOOST_PYTHON_INPLACE_OPERATOR(/, FloorDivide)
#else
PXR_BOOST_PYTHON_INPLACE_OPERATOR(/, Divide)
#endif
PXR_BOOST_PYTHON_INPLACE_OPERATOR(%, Remainder)
PXR_BOOST_PYTHON_INPLACE_OPERATOR(<<, Lshift)
PXR_BOOST_PYTHON_INPLACE_OPERATOR(>>, Rshift)
PXR_BOOST_PYTHON_INPLACE_OPERATOR(&, And)
PXR_BOOST_PYTHON_INPLACE_OPERATOR(^, Xor)
PXR_BOOST_PYTHON_INPLACE_OPERATOR(|, Or)
#undef PXR_BOOST_PYTHON_INPLACE_OPERATOR

object::object(handle<> const& x)
     : object_base(python::incref(python::expect_non_null(x.get())))
{}

}}} // namespace PXR_BOOST_NAMESPACE::python
