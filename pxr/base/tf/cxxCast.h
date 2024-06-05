//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_CXX_CAST_H
#define PXR_BASE_TF_CXX_CAST_H

/// \file tf/cxxCast.h
/// C++ Cast Utilities.

#ifndef __cplusplus
#error This include file can only be included in C++ programs.
#endif

#include "pxr/pxr.h"
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

template <class Src, class Dst>
using Tf_CopyConst =
    typename std::conditional<std::is_const<Src>::value,
                              typename std::add_const<Dst>::type, Dst>::type;

template <class Src, class Dst>
using Tf_CopyVolatile =
    typename std::conditional<std::is_volatile<Src>::value,
                              typename std::add_volatile<Dst>::type, Dst>::type;

template <class Src, class Dst>
using Tf_CopyCV = Tf_CopyConst<Src, Tf_CopyVolatile<Src, Dst>>;

/// Return a pointer to the most-derived object.
///
/// A \c dynamic_cast to \c void* is legal only for pointers to polymorphic
/// objects. This function returns the original pointer for non-polymorphic
/// types, and a pointer to the most-derived type of the object.
/// Said differently, given a pointer of static type \c B*, and given that
/// the object really points to an object of type \c D*, this function
/// returns the address of the object considered as a \c D*; however, for
/// non-polymorphic objects, the actual type of an object is taken to be \c B,
/// since one cannot prove that that the type is actually different.
///
/// \warning This function is public, but should be used sparingly (or not all).
template <typename T>
inline typename std::enable_if<
    std::is_polymorphic<T>::value, Tf_CopyCV<T, void>*>::type
TfCastToMostDerivedType(T* ptr)
{
    return dynamic_cast<Tf_CopyCV<T, void>*>(ptr);
}

template <typename T>
inline typename std::enable_if<
    !std::is_polymorphic<T>::value, Tf_CopyCV<T, void>*>::type
TfCastToMostDerivedType(T* ptr)
{
    return static_cast<Tf_CopyCV<T, void>*>(ptr);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
