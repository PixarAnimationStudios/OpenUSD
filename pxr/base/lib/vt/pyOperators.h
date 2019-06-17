//
// Copyright 2017 Pixar
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

#include "pxr/pxr.h"
#include "pxr/base/vt/api.h"

// See the description in vt/operators.h for a better understanding regarding
// the purpose and usage of macros defined below.

PXR_NAMESPACE_OPEN_SCOPE

// -------------------------------------------------------------------------
// Python operator definitions
// -------------------------------------------------------------------------
// These will define the operator to work with tuples and lists from Python.

// base macro called by wrapping layers below for various operators, python
// types (lists and tuples), and special methods
#define VTOPERATOR_WRAP_PYTYPE_BASE(op,method,pytype,rettype,expr)           \
    template <typename T> static                                             \
    VtArray<rettype> method##pytype(VtArray<T> vec, pytype obj)              \
    {                                                                        \
        size_t length = len(obj);                                            \
        if (length != vec.size()) {                                          \
            TfPyThrowValueError("Non-conforming inputs for operator " #op);  \
            return VtArray<T>();                                             \
        }                                                                    \
        VtArray<rettype> ret(vec.size());                                    \
        for (size_t i = 0; i < length; ++i) {                                \
            if (!extract<T>(obj[i]).check())                                 \
                TfPyThrowValueError("Element is of incorrect type.");        \
            ret[i] = expr;                                                   \
        }                                                                    \
        return ret;                                                          \
    }

// wrap Array op pytype
#define VTOPERATOR_WRAP_PYTYPE(op,lmethod,tuple,T)                             \
    VTOPERATOR_WRAP_PYTYPE_BASE(op,lmethod,tuple,T,                            \
                                (vec[i] op (T)extract<T>(obj[i])) )

// wrap pytype op Array (for noncommutative ops like subtraction)
#define VTOPERATOR_WRAP_PYTYPE_R(op,lmethod,tuple,T)                           \
    VTOPERATOR_WRAP_PYTYPE_BASE(op,lmethod,tuple,T,                            \
                                ((T)extract<T>(obj[i]) op vec[i]) )


// operator that needs a special method plus a reflected special method,
// each defined on tuples and lists
#define VTOPERATOR_WRAP(op,lmethod,rmethod)                 \
    VTOPERATOR_WRAP_PYTYPE(op,lmethod,tuple,T)              \
    VTOPERATOR_WRAP_PYTYPE(op,lmethod,list,T)               \
    VTOPERATOR_WRAP_PYTYPE(op,rmethod,tuple,T)              \
    VTOPERATOR_WRAP_PYTYPE(op,rmethod,list,T)                

// like above, but for non-commutative ops like subtraction
#define VTOPERATOR_WRAP_NONCOMM(op,lmethod,rmethod)         \
    VTOPERATOR_WRAP_PYTYPE(op,lmethod,tuple,T)              \
    VTOPERATOR_WRAP_PYTYPE(op,lmethod,list,T)               \
    VTOPERATOR_WRAP_PYTYPE_R(op,rmethod,tuple,T)            \
    VTOPERATOR_WRAP_PYTYPE_R(op,rmethod,list,T)                

// to be used to actually declare the wrapping with def() on the class
#define VTOPERATOR_WRAPDECLARE_BASE(op,method,rettype)      \
    .def(self op self)                                      \
    .def(self op Type())                                    \
    .def(Type() op self)                                    \
    .def(#method,method##tuple<rettype>)                    \
    .def(#method,method##list<rettype>)                      

#define VTOPERATOR_WRAPDECLARE(op,lmethod,rmethod)          \
    VTOPERATOR_WRAPDECLARE_BASE(op,lmethod,Type)            \
    .def(#rmethod,rmethod##tuple<Type>)                     \
    .def(#rmethod,rmethod##list<Type>)                

// to be used for wrapping conditional functions that return bool arrays
// (i.e. Equal, etc)
#define VTOPERATOR_WRAP_PYTYPE_BOOL_BASE(func,arg1,arg2,expr)         \
    template <typename T> static                                      \
    VtArray<bool> Vt##func(arg1, arg2)                                \
    {                                                                 \
        size_t length = len(obj);                                     \
        if (length != vec.size()) {                                   \
            TfPyThrowValueError("Non-conforming inputs for " #func);  \
            return VtArray<bool>();                                   \
        }                                                             \
        VtArray<bool> ret(vec.size());                                \
        for (size_t i = 0; i < length; ++i) {                         \
            if (!extract<T>(obj[i]).check())                          \
                TfPyThrowValueError("Element is of incorrect type."); \
            ret[i] = expr;                                            \
        }                                                             \
        return ret;                                                   \
    }

// array OP pytype
// pytype OP array
#define VTOPERATOR_WRAP_PYTYPE_BOOL(func,pytype,op)         \
        VTOPERATOR_WRAP_PYTYPE_BOOL_BASE(func,              \
            VtArray<T> const &vec, pytype const &obj,       \
            (vec[i] op (T)extract<T>(obj[i])) )             \
        VTOPERATOR_WRAP_PYTYPE_BOOL_BASE(func,              \
            pytype const &obj,VtArray<T> const &vec,        \
            ((T)extract<T>(obj[i]) op vec[i]) )             

#define VTOPERATOR_WRAP_BOOL(func,op)                       \
        VTOPERATOR_WRAP_PYTYPE_BOOL(func,list,op)           \
        VTOPERATOR_WRAP_PYTYPE_BOOL(func,tuple,op)          

// to be used to actually declare the wrapping with def() on the class
#define VTOPERATOR_WRAPDECLARE_BOOL(func)                   \
       def(#func,(VtArray<bool> (*)                         \
            (VtArray<Type> const &,VtArray<Type> const &))  \
            Vt##func<Type>);                                \
        def(#func,(VtArray<bool> (*)                        \
            (Type const &,VtArray<Type> const &))           \
            Vt##func<Type>);                                \
        def(#func,(VtArray<bool> (*)                        \
            (VtArray<Type> const &,Type const &))           \
            Vt##func<Type>);                                \
        def(#func,(VtArray<bool> (*)                        \
            (VtArray<Type> const &,tuple const &))          \
            Vt##func<Type>);                                \
        def(#func,(VtArray<bool> (*)                        \
            (tuple const &,VtArray<Type> const &))          \
            Vt##func<Type>);                                \
        def(#func,(VtArray<bool> (*)                        \
            (VtArray<Type> const &,list const &))           \
            Vt##func<Type>);                                \
        def(#func,(VtArray<bool> (*)                        \
            (list const &,VtArray<Type> const &))           \
            Vt##func<Type>);                                

PXR_NAMESPACE_CLOSE_SCOPE
