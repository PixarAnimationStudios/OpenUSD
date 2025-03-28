//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_DECLARE_PTRS_H
#define PXR_BASE_TF_DECLARE_PTRS_H

/// \file tf/declarePtrs.h
/// Standard pointer typedefs.
///
/// This file provides typedefs for standard pointer types.

#include "pxr/pxr.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/tf/refPtr.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

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

PXR_NAMESPACE_CLOSE_SCOPE

#endif
