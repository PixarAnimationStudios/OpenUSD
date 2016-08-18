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
/*
 * This header is not meant to be included in a .h file.
 * Complain if we see this header twice through.
 */

#ifdef TF_INSTANTIATESINGLETON_H
#error This file should only be included once in any given source (.cpp) file.
#endif

#define TF_INSTANTIATESINGLETON_H

/// \file tf/instantiateSingleton.h
/// \ingroup group_tf_ObjectCreation
/// Manage a single instance of an object.

#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/pragmas.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/arch/demangle.h"
#include <mutex>

template <typename T>
T&
TfSingleton<T>::_CreateInstance()
{
    static std::once_flag once;
    std::call_once(once, [](){
        TfSingleton<T>::_mutex = new std::mutex;
    });

    TfAutoMallocTag2 tag2("Tf", "TfSingleton::_CreateInstance");
    TfAutoMallocTag tag("Create Singleton " + ArchGetDemangled<T>());

    std::lock_guard<std::mutex> lock(*TfSingleton<T>::_mutex);
    if (!TfSingleton<T>::_instance) {
        // T's constructor could cause this to be created and set
        // already, so guard against that.
        ARCH_PRAGMA_MAY_NOT_BE_ALIGNED
        T *inst = new T;
        ARCH_PRAGMA_RESTORE
        if (!TfSingleton<T>::_instance) {
            TfSingleton<T>::_instance = inst;
        }
    }

    return *TfSingleton<T>::_instance;
}

template <typename T>
void
TfSingleton<T>::_DestroyInstance()
{
    std::lock_guard<std::mutex> lock(*TfSingleton<T>::_mutex);
    delete TfSingleton<T>::_instance;
    TfSingleton<T>::_instance = 0;
}

/// Source file definition that a type is being used as a singleton.
///
/// To use a type \c T in conjunction with \c TfSingleton, add
/// TF_INSTANTIATE_SINGLETON(T) in one source file (typically the .cpp) file
/// for class \c T.
///
/// \hideinitializer
#define TF_INSTANTIATE_SINGLETON(T)                               \
    template <> std::mutex* TfSingleton<T>::_mutex = 0;           \
    template <> T* TfSingleton<T>::_instance = 0;                 \
                                                                  \
    template T& TfSingleton< T >::_CreateInstance();              \
    template void TfSingleton< T >::_DestroyInstance()

