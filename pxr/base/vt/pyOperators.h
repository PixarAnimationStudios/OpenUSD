//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_VT_PY_OPERATORS_H
#define PXR_BASE_VT_PY_OPERATORS_H

#include "pxr/pxr.h"
#include "pxr/base/vt/api.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {
template <class T>
struct _ArrayPyOpHelp {
    static T __add__(T l, T r) { return l + r; }
    static T __sub__(T l, T r) { return l - r; }
    static T __mul__(T l, T r) { return l * r; }
    static T __div__(T l, T r) { return l / r; }
    static T __mod__(T l, T r) { return l % r; }
};

// These operations on bool-arrays are highly questionable, but this preserves
// existing behavior in the name of Hyrum's Law.
template <>
struct _ArrayPyOpHelp<bool> {
    static bool __add__(bool l, bool r) { return l | r; }
    static bool __sub__(bool l, bool r) { return l ^ r; }
    static bool __mul__(bool l, bool r) { return l & r; }
    static bool __div__(bool l, bool r) { return l; }
    static bool __mod__(bool l, bool r) { return false; }
};

} // anon

// -------------------------------------------------------------------------
// Python operator definitions
// -------------------------------------------------------------------------
// These will define the operator to work with tuples and lists from Python.

// base macro called by wrapping layers below for various operators, python
// types (lists and tuples), and special methods
#define VTOPERATOR_WRAP_PYTYPE_BASE(op, method, pytype, isRightVer)          \
    template <typename T> static                                             \
    VtArray<T> method##pytype(VtArray<T> vec, pytype obj) {                  \
        size_t length = len(obj);                                            \
        if (length != vec.size()) {                                          \
            TfPyThrowValueError("Non-conforming inputs for operator "        \
                                #method);                                    \
            return VtArray<T>();                                             \
        }                                                                    \
        VtArray<T> ret(vec.size());                                          \
        for (size_t i = 0; i < length; ++i) {                                \
            if (!extract<T>(obj[i]).check())                                 \
                TfPyThrowValueError("Element is of incorrect type.");        \
            if (isRightVer) {                                                \
                ret[i] = _ArrayPyOpHelp<T>:: op (                            \
                    (T)extract<T>(obj[i]), vec[i]);                          \
            }                                                                \
            else {                                                           \
                ret[i] = _ArrayPyOpHelp<T>:: op (                            \
                    vec[i], (T)extract<T>(obj[i]));                          \
            }                                                                \
        }                                                                    \
        return ret;                                                          \
    }

// wrap Array op pytype
#define VTOPERATOR_WRAP_PYTYPE(op, method, pytype)                          \
    VTOPERATOR_WRAP_PYTYPE_BASE(op, method, pytype, false)

// wrap pytype op Array (for noncommutative ops like subtraction)
#define VTOPERATOR_WRAP_PYTYPE_R(op, method, pytype)                           \
    VTOPERATOR_WRAP_PYTYPE_BASE(op, method, pytype, true) 


// operator that needs a special method plus a reflected special method,
// each defined on tuples and lists
#define VTOPERATOR_WRAP(lmethod,rmethod)                                       \
    VTOPERATOR_WRAP_PYTYPE(lmethod,lmethod,tuple)                              \
    VTOPERATOR_WRAP_PYTYPE(lmethod,lmethod,list)                               \
    VTOPERATOR_WRAP_PYTYPE(lmethod,rmethod,tuple)                              \
    VTOPERATOR_WRAP_PYTYPE(lmethod,rmethod,list)                

// like above, but for non-commutative ops like subtraction
#define VTOPERATOR_WRAP_NONCOMM(lmethod,rmethod)            \
    VTOPERATOR_WRAP_PYTYPE(lmethod,lmethod,tuple)           \
    VTOPERATOR_WRAP_PYTYPE(lmethod,lmethod,list)            \
    VTOPERATOR_WRAP_PYTYPE_R(lmethod,rmethod,tuple)         \
    VTOPERATOR_WRAP_PYTYPE_R(lmethod,rmethod,list)                

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
                      

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_VT_PY_OPERATORS_H
