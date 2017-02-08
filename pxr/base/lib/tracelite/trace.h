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
#ifndef TRACELITE_TRACE_H
#define TRACELITE_TRACE_H

/// \file tracelite/trace.h
/// Low-level trace handler facility.
///
/// This library implements a stub-tracing system, with the actual code doing
/// the tracing registered as callback functions, via \c
/// TraceliteSetFunctions().

#include <stddef.h>

#include "pxr/pxr.h"
#include "pxr/base/arch/functionLite.h"
#include "pxr/base/arch/hints.h"

#include <atomic>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// Forward declaration to type in lib/Trace (in place of void*)
class TraceScopeHolder;

#define _TRACELITE_JOIN(x, y) _TRACELITE_JOIN2(x, y)
#define _TRACELITE_JOIN2(x, y) x ## y

/// Typedef for "initialize" trace function.
typedef void (*TraceliteInitializeFunction)(
    std::atomic<TraceScopeHolder*>*, const std::string*,
    char const*, char const*);

/// Typedef for "begin" trace function.
typedef void (*TraceliteBeginFunction)(void*, void*);

/// Typedef for "end" trace function.
typedef void (*TraceliteEndFunction)(void*);

/// Size of available "stack" data.
/// \hideinitializer
#define TRACELITE_STACKDATA_SIZE  (sizeof(size_t) + 2*sizeof(void*))

/// Register begin/end trace callbacks.
///
/// The begin/end functions/initialize functions are called in the following
/// sequence:
///
/// \code
///   static void* siteData = NULL;
///   if (!siteData) (*initializeFunction)(&siteData, keyStr1, keyStr2);
///   (*beginFunction)(stackData, siteData)
///
///   // ...code to be traced...
///
///   (*endFunction)(stackData)
/// \endcode
///
/// The argument stackData is a pointer to data on the thread's stack, with
/// pointer alignment and size of at least \c TRACELITE_STACKDATA_SIZE. The
/// arguments keyStr1 and keyStr2 are char const* pointers with data that
/// describes the site being initialized.
///
/// Until \c TraceliteSetFunctions() is called, the functions called above are
/// no-op functions (and in particular, the initialize function called will
/// not modify siteData).
///
/// After calling \c TraceliteSetFunctions(), the initialize function will be
/// called; however, one must still call TraceliteEnable(true) to activate the
/// begin/end functions.
///
/// This call is not thread-safe (the simplest use is to only call it from the
/// main thread). 
void TraceliteSetFunctions(TraceliteInitializeFunction initializeFunction,
			   TraceliteBeginFunction beginFunction,
			   TraceliteEndFunction endFunction);

/// Enable the begin/end trace callbacks.
///
/// Calling this function before calling \c TraceliteSetFunctions() is
/// silently ignored. After that, each call to this function with true
/// increments a counter; each call with false decrements a counter.  As long
/// as the counter is positive, the begin/end functions registered by \c
/// TraceliteSetFunctions() are active.
///
/// The value returned is the count (including the effects of this call);
/// thus, if the call returns positive, the begin/end trace callbacks are
/// enabled.
///
/// This call is not thread-safe (the simplest use is to only call it from the
/// main thread). 
int TraceliteEnable(bool state);

class Tracelite_ScopeAuto {
public:
    Tracelite_ScopeAuto(std::atomic<TraceScopeHolder*>* siteData,
                        const std::string& key) {
        _wasActive = false;
        if (ARCH_UNLIKELY(Tracelite_ScopeAuto::_active)) {
            _wasActive = true;
            if (ARCH_UNLIKELY(!*siteData))
                _Initialize(siteData, key);

            (*_beginFunction)(_space, *siteData);
        }
    }

    Tracelite_ScopeAuto(std::atomic<TraceScopeHolder*>* siteData,
                        char const* key1, char const* key2 = NULL) {
	_wasActive = false;
	if (ARCH_UNLIKELY(Tracelite_ScopeAuto::_active)) {
	    _wasActive = true;
	    if (ARCH_UNLIKELY(!*siteData))
		_Initialize(siteData, key1, key2);

	    (*_beginFunction)(_space, *siteData);
	}
    }
    
    ~Tracelite_ScopeAuto() {
	if (ARCH_UNLIKELY(_wasActive)) {
	    (*_endFunction)(_space);
	}
    }
    
    
private:
    static void _Initialize(std::atomic<TraceScopeHolder*>* siteData,
                            const std::string& key);
    static void _Initialize(std::atomic<TraceScopeHolder*>* siteData,
                            char const* key1, char const* key2);

    friend int TraceliteEnable(bool);
    friend void TraceliteSetFunctions(TraceliteInitializeFunction initializeFunction,
				      TraceliteBeginFunction beginFunction,
				      TraceliteEndFunction endFunction);
    union {
	void* _autoData;
	unsigned char _space[TRACELITE_STACKDATA_SIZE];
    };

    static TraceliteBeginFunction _beginFunction;
    static TraceliteEndFunction _endFunction;
    static bool _active;
    bool _wasActive;
};

#undef TRACE_SCOPE
#undef TRACE_FUNCTION

#define TRACE_SCOPE(name) _TRACELITE_SCOPE(__LINE__, name)
#define TRACE_FUNCTION()  _TRACELITE_FUNCTION(__LINE__, __ARCH_FUNCTION__, __ARCH_PRETTY_FUNCTION__)

#define _TRACELITE_FUNCTION(instance, name, prettyName)			    \
    static std::atomic<TraceScopeHolder*>                                   \
        _TRACELITE_JOIN(_tracelite_site, instance);                         \
    Tracelite_ScopeAuto _TRACELITE_JOIN(_traceliteScopeAuto_, instance)(    \
			&_TRACELITE_JOIN(_tracelite_site, instance), name, prettyName)

#define _TRACELITE_SCOPE(instance, name)				    \
    static std::atomic<TraceScopeHolder*>                                   \
        _TRACELITE_JOIN(_tracelite_site, instance);                         \
    Tracelite_ScopeAuto _TRACELITE_JOIN(_traceliteScopeAuto_, instance)(    \
			&_TRACELITE_JOIN(_tracelite_site, instance), name)

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TRACELITE_TRACE_H
