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
#ifndef TF_CXXCAST_H
#define TF_CXXCAST_H

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
    not std::is_polymorphic<T>::value, Tf_CopyCV<T, void>*>::type
TfCastToMostDerivedType(T* ptr)
{
    return static_cast<Tf_CopyCV<T, void>*>(ptr);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
