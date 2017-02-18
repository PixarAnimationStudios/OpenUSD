//
// Copyright 2016 Pixar
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
#ifndef VT_OPERATORS_H
#define VT_OPERATORS_H

#include "pxr/pxr.h"
#include "pxr/base/vt/api.h"

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>

//
// Operators.h
//
// Define macros to make it easier to create and wrap operators that work
// with multiple types.  In general, we want most operators to allow
// array OP array
// array OP scalar
// scalar OP array
// array OP tuple
// tuple OP array
// array OP list
// list OP array
//
// where the operators work element-by-element.  For arrays, this means they
// need to be the same size and shape.   For tuples and lists, this means
// the array shape is ignored, and the array and tuple/list need to contain the
// same number of elements.  For scalars, this means the same scalar value is
// used for every array element.  Naturally we require that the types held in
// the array, tuple, list, and scalar are all the same (i.e no multiplying
// a VtDoubleArray by a GfVec3d).
//
// However, we do have special cases for
// array OP double
// double OP array
// for some OPs, for cases where you may want to do element-by-element
// operations with a double when the array type is *not* double (e.g. Vec3d).
// This is done in Core/Anim, for example, for spline calculations on the
// underlying data type.
//
// We define macros because this is so repetitive, and because they need to be
// defined in different places (the array operators on the class, the array and
// scalar operators as free functions, and the tuple/list operators in the
// Python wrapping of course).  Having them here allows us to put them all
// in one place for better editability.

// -------------------------------------------------------------------------
// C++ operator definitions
// -------------------------------------------------------------------------
// These will be callable from C++, and define operations between arrays
// and between arrays and scalars.

PXR_NAMESPACE_OPEN_SCOPE

// Operations on arrays
// These go into the class definition for VtArray
#define VTOPERATOR_CPPARRAY(op)                                                \
    VtArray operator op (VtArray const &other) const {                         \
        /* accept empty vecs */                                                \
        if ((size()!=0 && other.size()!=0) &&                                  \
            (size() != other.size() /*||                                       \
             GetShape() != other.GetShape()*/)) {                                \
            TF_CODING_ERROR("Non-conforming inputs for operator %s",#op);      \
            return VtArray();                                                  \
        }                                                                      \
        /* promote empty vecs to vecs of zeros */                              \
        const bool thisEmpty = size() == 0,                                    \
            otherEmpty = other.size() == 0;                                    \
        VtArray ret(thisEmpty ? other.size() : size());                        \
        ElementType zero = VtZero<ElementType>();                              \
        for (size_t i = 0, n = ret.size(); i != n; ++i) {                      \
            ret[i] = (thisEmpty ? zero : (*this)[i]) op                        \
                (otherEmpty ? zero : other[i]);                                \
        }                                                                      \
        return ret;                                                            \
    }

#define VTOPERATOR_CPPARRAY_UNARY(op)                              \
    VtArray operator op () const {                                 \
        VtArray ret(size());                                       \
        for (size_t i = 0, sz = ret.size(); i != sz; ++i) {        \
            ret[i] = op (*this)[i];                                \
        }                                                          \
        return ret;                                                \
    }

// Operations on scalars and arrays
// These are free functions defined in Array.h
#define VTOPERATOR_CPPSCALAR_TYPE(op,arraytype,scalartype,rettype)      \
    template<typename arraytype>                                        \
    VtArray<ElemType>                                                   \
    operator op (scalartype const &scalar,                              \
                 VtArray<arraytype> const &vec) {                       \
        VtArray<rettype> ret(vec.size());                               \
        for (size_t i = 0; i<vec.size(); ++i)                           \
            ret[i] = scalar op vec[i];                                  \
        return ret;                                                     \
    }                                                                   \
    template<typename arraytype>                                        \
    VtArray<ElemType>                                                   \
    operator op (VtArray<arraytype> const &vec,                         \
                 scalartype const &scalar) {                            \
        VtArray<rettype> ret(vec.size());                               \
        for (size_t i = 0; i<vec.size(); ++i)                           \
            ret[i] = vec[i] op scalar;                                  \
        return ret;                                                     \
    } 

#define VTOPERATOR_CPPSCALAR(op)                                        \
    VTOPERATOR_CPPSCALAR_TYPE(op,ElemType,ElemType,ElemType)

// define special-case operators on arrays and doubles - except if the array
// holds doubles, in which case we already defined the operator (with
// VTOPERATOR_CPPSCALAR above) so we can't do it again!
#define VTOPERATOR_CPPSCALAR_DOUBLE(op)                                        \
    template<typename ElemType>                                                \
    typename boost::disable_if<boost::is_same<ElemType, double>,               \
                               VtArray<ElemType> >::type                       \
    operator op (double const &scalar,                                         \
                 VtArray<ElemType> const &vec) {                               \
        VtArray<ElemType> ret(vec.size());                                     \
        for (size_t i = 0; i<vec.size(); ++i)                                  \
            ret[i] = scalar op vec[i];                                         \
        return ret;                                                            \
    }                                                                          \
    template<typename ElemType>                                                \
    typename boost::disable_if<boost::is_same<ElemType, double>,               \
                               VtArray<ElemType> >::type                       \
    operator op (VtArray<ElemType> const &vec,                                 \
                 double const &scalar) {                                       \
        VtArray<ElemType> ret(vec.size());                                     \
        for (size_t i = 0; i<vec.size(); ++i)                                  \
            ret[i] = vec[i] op scalar;                                         \
        return ret;                                                            \
    } 

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
        for (int i = 0; i < length; ++i) {                                   \
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
        for (int i = 0; i < length; ++i) {                            \
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

struct Vt_Reserved;

VT_API bool
Vt_ArrayStackCheck(size_t size, const Vt_Reserved* reserved);
VT_API bool
Vt_ArrayCompareSize(size_t aSize, const Vt_Reserved* aReserved,
                    size_t bSize, const Vt_Reserved* bReserved);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // VT_OPERATORS_H
