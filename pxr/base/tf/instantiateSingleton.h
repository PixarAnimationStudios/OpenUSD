//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/*
 * This header is not meant to be included in a .h file.
 * Complain if we see this header twice through.
 */

#ifdef PXR_BASE_TF_INSTANTIATE_SINGLETON_H
#error This file should only be included once in any given source (.cpp) file.
#endif

#define PXR_BASE_TF_INSTANTIATE_SINGLETON_H

/// \file tf/instantiateSingleton.h
/// \ingroup group_tf_ObjectCreation
/// Manage a single instance of an object.

#include "pxr/pxr.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/arch/demangle.h"

#include <thread>

PXR_NAMESPACE_OPEN_SCOPE

template <class T> std::atomic<T *> TfSingleton<T>::_instance;

template <class T>
void
TfSingleton<T>::SetInstanceConstructed(T &instance)
{
    if (_instance.exchange(&instance) != nullptr) {
        TF_FATAL_ERROR("this function may not be called after "
                       "GetInstance() or another SetInstanceConstructed() "
                       "has completed");
    }
}

template <class T>
T *
TfSingleton<T>::_CreateInstance(std::atomic<T *> &instance)
{
    static std::atomic<bool> isInitializing;
    
    TfAutoMallocTag2 tag("Tf", "TfSingleton::_CreateInstance",
                         "Create Singleton " + ArchGetDemangled<T>());

    // Try to take isInitializing false -> true.  If we do it, then check to see
    // if we don't yet have an instance.  If we don't, then we get to create it.
    // Otherwise we just wait until the instance shows up.
    if (isInitializing.exchange(true) == false) {
        // Do we not yet have an instance?
        if (!instance) {
            // Create it.  The constructor may set instance via
            // SetInstanceConstructed(), so check for that.
            T *newInst = new T;
            
            T *curInst = instance.load();
            if (curInst) {
                if (curInst != newInst) {
                    TF_FATAL_ERROR("race detected setting singleton instance");
                }
            }
            else {
                TF_AXIOM(instance.exchange(newInst) == nullptr);
            }                
        }
        isInitializing = false;
    }
    else {
        while (!instance) {
            std::this_thread::yield();
        }
    }
    
    return instance.load();
}

template <typename T>
void
TfSingleton<T>::DeleteInstance()
{
    // Try to swap out a non-null instance for nullptr -- if we do it, we delete
    // it.
    T *instance = _instance.load();
    while (instance && !_instance.compare_exchange_weak(instance, nullptr)) {
        std::this_thread::yield();
    }
    delete instance;
}

/// Source file definition that a type is being used as a singleton.
///
/// To use a type \c T in conjunction with \c TfSingleton, add
/// TF_INSTANTIATE_SINGLETON(T) in one source file (typically the .cpp) file
/// for class \c T.
///
/// \hideinitializer
#define TF_INSTANTIATE_SINGLETON(T)                               \
    template class PXR_NS_GLOBAL::TfSingleton<T>


PXR_NAMESPACE_CLOSE_SCOPE
