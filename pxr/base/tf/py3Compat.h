//
// Copyright 2020 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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

#define TfPyInt_Check PyInt_Check
#define TfPyInt_AS_LONG PyInt_AS_LONG

#define TfPyIteratorNextMethodName "next"
#define TfPyClassMethodFuncName "im_func"
#define TfPyBoolBuiltinFuncName "__nonzero__"
#define TfPyBuiltinModuleName "__builtin__"

#define TfPyString_AsString PyString_AsString
#define TfPyString_Check PyString_Check

#endif

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_3_COMPAT_H

