//
// Copyright 2018 Pixar
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

#ifndef PXR_BASE_TRACE_CUSTOMCALLBACK_H
#define PXR_BASE_TRACE_CUSTOMCALLBACK_H

#include "pxr/pxr.h"

#include "pxr/base/trace/api.h"
#include "pxr/base/trace/staticKeyData.h"
#include "pxr/base/trace/dynamicKey.h"
#include "pxr/base/arch/hints.h"

// Make TRACE_CUSTOM_CALLBACK the default for downstream library use simplicity
#ifndef TRACE_CUSTOM_CALLBACK
#define TRACE_CUSTOM_CALLBACK 1
#endif

PXR_NAMESPACE_OPEN_SCOPE


////////////////////////////////////////////////////////////////////////////////
/// \class TraceCustomCallback
///
/// A class which allows other Profilers to hook into the trace scoped classes.
/// An external system will need to compile USD with "TRACE_CUSTOM_CALLBACK" defined,
/// then call the \ref TraceCustomCallback::RegisterCallbacks to register function
/// pointers to begin and end callbacks.
/// \ref TraceCustomCallback::UnregisterCallbacks needs to be called to clear the callbacks.
///
struct TraceCustomCallback
{
    typedef void(* BeginStaticKeyFn)(const TraceStaticKeyData& key, void** customData);
    typedef void(* BeginDynamicKeyFn)(const TraceDynamicKey& key, void** customData);
    typedef void(* EndFn)(void** customData);

    /// Creates a new callback, this is used by the TRACE_CUSTOM_CALLBACK_DEFINE to automatically
    /// instanciate a new callback in each dll/so file that uses the TRACE_* macros.
    /// Each dll/so will contain one instance of this \ref TraceCustomCallback named \ref g_traceCustomCallback.
    TRACE_API
    static TraceCustomCallback* CreateNew();

    /// Registers new begin/end callbacks.
    TRACE_API
    static void RegisterCallbacks(BeginStaticKeyFn bs, BeginDynamicKeyFn bd, EndFn e);

    /// Unregister the callbacks.    
    TRACE_API
    static void UnregisterCallbacks();

    TRACE_API
    TraceCustomCallback();
    
    TRACE_API
    ~TraceCustomCallback();
    
    /// Called by the TRACE_ macros when TRACE_CUSTOM_CALLBACK is defined
    inline void BeginStatic(const TraceStaticKeyData& key, void** customData) {
        if (ARCH_UNLIKELY(_BeginS)){
            _BeginS(key, customData);
        }
    }
    
    /// Called by the TRACE_ macros when TRACE_CUSTOM_CALLBACK is defined
    inline void BeginDynamic(const TraceDynamicKey& key, void** customData) {
        if (ARCH_UNLIKELY(_BeginD)){
            _BeginD(key, customData);
        }
    }
    
    /// Called by the TRACE_ macros when TRACE_CUSTOM_CALLBACK is defined
    inline void End(void** customData) {
        if (ARCH_UNLIKELY(_End)){
            _End(customData);
        }
    }

private:

    BeginStaticKeyFn _BeginS = nullptr;
    BeginDynamicKeyFn _BeginD = nullptr;
    EndFn _End = nullptr;

    static BeginStaticKeyFn _currentBeginS;
    static BeginDynamicKeyFn _currentBeginD;
    static EndFn _currentEnd;
};

/// This global needs to be initialised in each dll/so that uses the TRACE_* macros
/// This can be done conveniently by using the TRACE_CUSTOM_CALLBACK_DEFINE macro
extern TraceCustomCallback* g_traceCustomCallback;

PXR_NAMESPACE_CLOSE_SCOPE


#if(TRACE_CUSTOM_CALLBACK)
 
#define TRACE_CUSTOM_CALLBACK_DEFINE \
PXR_INTERNAL_NS::TraceCustomCallback* PXR_INTERNAL_NS::g_traceCustomCallback = \
    PXR_INTERNAL_NS::TraceCustomCallback::CreateNew();

#else // TRACE_CUSTOM_CALLBACK

#define TRACE_CUSTOM_CALLBACK_DEFINE

#endif // TRACE_CUSTOM_CALLBACK

#endif // PXR_BASE_TRACE_CUSTOMCALLBACK_H
