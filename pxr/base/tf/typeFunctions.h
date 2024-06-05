//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_TYPE_FUNCTIONS_H
#define PXR_BASE_TF_TYPE_FUNCTIONS_H

/// \file tf/typeFunctions.h
/// \ingroup group_tf_Internal

#include "pxr/pxr.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfTypeFunctions
/// \ingroup group_tf_Internal
///
/// Implements assorted functions based on compile-time type information.
///
/// TfTypeFunctions<T>::GetRawPtr(T* tPtr) returns tPtr.  A smart-pointer
/// class, such as \c TfRefPtr, may specialize this function to have different
/// behavior.  Note that for a non-pointer type, this returns the address of
/// the object, which allows one to uniformly apply the -> operator for member
/// function calls.
///
/// TfTypeFunctions<T>::ConstructFromRawPtr(T* tPtr) returns tPtr.
/// Pointer-like objects should specialize this function so that given a raw
/// pointer of type T*, they return a smart pointer pointing to that object
/// (see refPtr.h for an example). Essentially, this is the inverse of
/// TfTypeFunctions<T>::GetRawPtr.
///
template <class T, class ENABLE = void>
struct TfTypeFunctions {
#if 0
    static T* GetRawPtr(T& t) {
        return &t;
    }
#endif
    
    static const T* GetRawPtr(const T& t) {
        return &t;
    }

    static T& ConstructFromRawPtr(T* ptr) { return *ptr; }

    static bool IsNull(const T&) {
        return false;
    }

    static void Class_Object_MUST_Not_Be_Const() { }
    static void Object_CANNOT_Be_a_Pointer() { }
};

template <class T>
struct TfTypeFunctions<T*> {
    static T* GetRawPtr(T* t) {
        return t;
    }

    static T* ConstructFromRawPtr(T* ptr) { return ptr; }

    static bool IsNull(T* ptr) {
        return !ptr;
    }

    static void Class_Object_MUST_Be_Passed_By_Address() { }
    static void Class_Object_MUST_Not_Be_Const() { }
};

template <class T>
struct TfTypeFunctions<const T*> {
    static const T* GetRawPtr(const T* t) {
        return t;
    }

    static bool IsNull(const T* ptr) {
        return !ptr;
    }

    static const T* ConstructFromRawPtr(T* ptr) { return ptr; }
    static void Class_Object_MUST_Be_Passed_By_Address() { }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_TYPE_FUNCTIONS_H
