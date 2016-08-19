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
#ifndef TF_DECLARE_PTR_H
#define TF_DECLARE_PTR_H

/// \file tf/declarePtrs.h
/// Standard pointer typedefs.
///
/// This file provides typedefs for standard pointer types.

#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/tf/refPtr.h"
#include <vector>

/// \struct TfDeclarePtrs
/// Templated struct used for type definition macros.
template<typename T> struct TfDeclarePtrs {
    typedef TfWeakPtr< T >              Ptr;
    typedef TfWeakPtr< const T >        ConstPtr;
    typedef std::vector< Ptr >          PtrVector;
    typedef std::vector< ConstPtr >     ConstPtrVector;

    typedef TfRefPtr< T >               RefPtr;
    typedef TfRefPtr< const T >         ConstRefPtr;
    typedef std::vector< RefPtr >       RefPtrVector;
    typedef std::vector< ConstRefPtr >  ConstRefPtrVector;
};

/// Define standard weak pointer types.
///
/// \param type is a class name.
///
/// \c TF_DECLARE_WEAK_PTRS(Class) declares ClassPtr, ClassConstPtr,
/// ClassPtrVector and ClassConstPtrVector.
///
/// \hideinitializer
#define TF_DECLARE_WEAK_PTRS(type)                                      \
    typedef TfDeclarePtrs< class type >::Ptr type##Ptr;                 \
    typedef TfDeclarePtrs< class type >::ConstPtr type##ConstPtr;       \
    typedef TfDeclarePtrs< class type >::PtrVector type##PtrVector;     \
    typedef TfDeclarePtrs< class type >::ConstPtrVector type##ConstPtrVector

/// Define standard ref pointer types.
///
/// \param type is a class name.
///
/// \c TF_DECLARE_REF_PTRS(Class) declares ClassRefPtr and ClassConstRefPtr.
///
/// \hideinitializer
#define TF_DECLARE_REF_PTRS(type)                                       \
    typedef TfDeclarePtrs< class type >::RefPtr type##RefPtr;           \
    typedef TfDeclarePtrs< class type >::ConstRefPtr type##ConstRefPtr; \
    typedef TfDeclarePtrs< class type >::RefPtrVector type##RefPtrVector; \
    typedef TfDeclarePtrs< class type >::ConstRefPtrVector type##ConstRefPtrVector

/// Define standard weak, ref, and vector pointer types.
///
/// \param type is a class name.
///
/// \c TF_DECLARE_WEAK_AND_REF_PTRS(Class) declares ClassPtr, ClassConstPtr,
/// ClassPtrVector, ClassConstPtrVector, ClassRefPtr and ClassConstRefPtr.
///
/// \hideinitializer
#define TF_DECLARE_WEAK_AND_REF_PTRS(type)                              \
    TF_DECLARE_WEAK_PTRS(type);                                         \
    TF_DECLARE_REF_PTRS(type)

#endif
