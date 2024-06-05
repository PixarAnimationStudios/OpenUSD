//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_3_COMPAT_H
#define PXR_BASE_TF_PY_3_COMPAT_H

/// \file tf/py3Compat.h
/// Compatibility code for supporting python 2 and 3

#include "pxr/pxr.h"

#include "pxr/base/tf/pySafePython.h"

PXR_NAMESPACE_OPEN_SCOPE

// Python 3 migrating helpers:
#if PY_MAJOR_VERSION >= 3

// These flags are needed in python 2 only, stub them out in python 3.
// In python 3 this behavior is the default. See PEP-3118
#define TfPy_TPFLAGS_HAVE_NEWBUFFER 0
#define TfPy_TPFLAGS_HAVE_GETCHARBUFFER 0

#define TfPyBytes_Check PyBytes_Check
#define TfPyString_Check(a) (PyBytes_Check(a) || PyUnicode_Check(a))
#define TfPyString_AsString PyUnicode_AsUTF8

// PyInt -> PyLong remapping 
#define TfPyInt_Check PyLong_Check
// Note: Slightly different semantics, the macro does not do any error checking
#define TfPyInt_AS_LONG PyLong_AsLong

// Method and module names that are changed in python 3
#define TfPyIteratorNextMethodName "__next__"
#define TfPyClassMethodFuncName "__func__"
#define TfPyBoolBuiltinFuncName "__bool__"
#define TfPyBuiltinModuleName "builtins"

#else // Python 2

#define TfPy_TPFLAGS_HAVE_NEWBUFFER Py_TPFLAGS_HAVE_NEWBUFFER 
#define TfPy_TPFLAGS_HAVE_GETCHARBUFFER Py_TPFLAGS_HAVE_GETCHARBUFFER 

#define TfPyBytes_Check PyString_Check 
#define TfPyString_Check PyString_Check
#define TfPyString_AsString PyString_AsString

#define TfPyInt_Check PyInt_Check
#define TfPyInt_AS_LONG PyInt_AS_LONG

#define TfPyIteratorNextMethodName "next"
#define TfPyClassMethodFuncName "im_func"
#define TfPyBoolBuiltinFuncName "__nonzero__"
#define TfPyBuiltinModuleName "__builtin__"

#endif

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_3_COMPAT_H
