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
#ifndef TF_TYPEFUNCTIONS_H
#define TF_TYPEFUNCTIONS_H

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

/// \class TfCopyIfNotReference
/// \ingroup group_tf_Internal
///
/// \c TfCopyIfNotReference<T>::Apply(v) is used to return a pointer to the
/// value \p v.  If \c T is a non-reference type, then the value returned
/// points to newly constructed dynamic space, which the caller must free.
/// Otherwise, the returned value is the address of \p v.
///
template <class T>
struct TfCopyIfNotReference
{
    static T* Apply(T value) {
        return new T(value);
    }
};

template <class T>
struct TfCopyIfNotReference<T&>
{
    static T* Apply(T& value) {
        return &value;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TF_TYPEFUNCTIONS_H
