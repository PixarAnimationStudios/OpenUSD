#line 1 "C:/src/pxr-usd/pxr/base/trace/trace.h"
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

#ifndef PXR_BASE_TRACE_TRACE_H
#define PXR_BASE_TRACE_TRACE_H

/// \file trace/trace.h

#include "pxr/pxr.h"

#include "pxr/base/trace/api.h"
#include "pxr/base/trace/collector.h"
#include "pxr/base/trace/customCallback.h"

#include "pxr/base/tf/preprocessorUtilsLite.h"

#include <atomic>

#if !defined(TRACE_DISABLE)

/// Records a timestamp when constructed and a timespan event when destructed,
/// using the name of the function or method as the key.
#define TRACE_FUNCTION() \
        _TRACE_FUNCTION_INSTANCE(__FILE__, __LINE__, __ARCH_FUNCTION__, __ARCH_PRETTY_FUNCTION__)

/// Records a timestamp when constructed and a timespan event when destructed,
/// using \a name as the key.
#define TRACE_SCOPE(name) \
        _TRACE_SCOPE_INSTANCE(__FILE__, __LINE__, name)

/// Records a timestamp when constructed and a timespan event when destructed,
/// using the name of the function concatenated with \a name as the key.
#define TRACE_FUNCTION_SCOPE(name) \
        _TRACE_FUNCTION_SCOPE_INSTANCE( \
            __FILE__, __LINE__, __ARCH_FUNCTION__, __ARCH_PRETTY_FUNCTION__, name)

/// Records a timestamp when constructed, using \a name as the key.
#define TRACE_MARKER(name) \
        _TRACE_MARKER_INSTANCE(__FILE__, __LINE__, name)

/// Records a timestamp when constructed, using \a name as the key.
#define TRACE_MARKER_DYNAMIC(name) \
        _TRACE_MARKER_DYNAMIC_INSTANCE(name)

/// Records a counter \a delta using the \a name as the counter key. The delta can
/// be positive or negative. A positive delta will increment the total counter
/// value, whereas a negative delta will decrement it. The recorded value will
/// be stored at the currently traced scope, and will propagate up to the
/// parent scopes.
#define TRACE_COUNTER_DELTA(name, delta) \
        _TRACE_COUNTER_INSTANCE(__FILE__, __LINE__, name, delta, /* isDelta */ true)

/// Records a counter delta using the name as the counter key. Similar to 
/// TRACE_COUNTER_DELTA except that \p name does not need to be a compile time
/// string.
/// \sa TRACE_COUNTER_DELTA
#define TRACE_COUNTER_DELTA_DYNAMIC(name, delta) \
        TraceCollector::GetInstance().RecordCounterDelta(name, delta);

/// Records a counter value using the name as the counter key. The recorded 
/// value will be stored at the currently traced scope, and will propagate up to
/// the parent scopes.
#define TRACE_COUNTER_VALUE(name, value) \
        _TRACE_COUNTER_INSTANCE(__FILE__, __LINE__, name, value, /* isDelta */ false)

/// Records a counter value using the name as the counter key. Similar to 
/// TRACE_COUNTER_VALUE except that \p name does not need to be a compile time
/// string.
/// \sa TRACE_COUNTER_VALUE
#define TRACE_COUNTER_VALUE_DYNAMIC(name, value) \
        TraceCollector::GetInstance().RecordCounterValue(name, value);

/// Records a counter value using the name as the counter key. The value can
/// be positive or negative. A positive value will increment the total counter
/// value, whereas a negative value will decrement it. The recorded value will
/// be stored at the currently traced scope, and will propagate up to the
/// parent scopes.
///
/// This macro provides the same functionality as TRACE_COUNTER_DELTA, but takes
/// a section of code in brackets, which assumes that a value will be
/// assigned to 'value'. The section of code will not be executed, when
/// tracing is turned off, which makes it possible to gather counter values
/// from potentially expensive logic, without incurring an overhead with
/// tracing turned off.
///
/// Usage:
///
/// TRACE_COUNTER_DELTA_CODE("My counter", {
///     value = _ComputeExpensiveCounterValue();
/// })
#define TRACE_COUNTER_DELTA_CODE(name, code) \
        _TRACE_COUNTER_CODE_INSTANCE(__FILE__, __LINE__, name, code, true)

/// Records a begin event when constructed and an end event when destructed,
/// using name of the function or method and the supplied name as the key. 
/// Unlike TRACE_FUNCTION, the name argument will be evaluated each time this 
/// macro is invoked. This allows for a single TRACE_FUNCTION to track time 
/// under different keys, but incurs greater overhead.
#define TRACE_FUNCTION_DYNAMIC(name) \
        _TRACE_FUNCTION_DYNAMIC_INSTANCE(__FILE__, __LINE__, __ARCH_FUNCTION__, __ARCH_PRETTY_FUNCTION__, name)

/// Records a begin event when constructed and an end event when destructed,
/// using \a name as the key. Unlike TRACE_SCOPE, the name argument will
/// be evaluated each time this macro is invoked. This allows for a single 
/// TRACE_SCOPE to track time under different keys, but incurs greater 
/// overhead.
#define TRACE_SCOPE_DYNAMIC(name) \
        _TRACE_SCOPE_DYNAMIC_INSTANCE(__FILE__, __LINE__, name)


/// These pair a uniquely named TraceScopeHolder with a TraceScopeAuto.
/// Together these will register a TraceScope only the first time the
/// code is executed or if the TraceScope expires.  Otherwise, the held
/// TraceScope will be used to record begin and end events.

#define _TRACE_FUNCTION_INSTANCE(file, line, name, prettyName) \
constexpr static PXR_NS::TraceStaticKeyData \
    TF_PP_CAT(TraceKeyData_, line)(file, line, name, prettyName); \
PXR_NS::TraceScopeAuto TF_PP_CAT(TraceScopeAuto_, line)(\
    TF_PP_CAT(TraceKeyData_, line));

#define _TRACE_SCOPE_INSTANCE(file, line, name) \
constexpr static PXR_NS::TraceStaticKeyData \
    TF_PP_CAT(TraceKeyData_, line)(file, line, name); \
PXR_NS::TraceScopeAuto TF_PP_CAT(TraceScopeAuto_, line)(\
    TF_PP_CAT(TraceKeyData_, line));

#define _TRACE_FUNCTION_SCOPE_INSTANCE(file, line, name, prettyName, scopeName) \
constexpr static PXR_NS::TraceStaticKeyData \
    TF_PP_CAT(TraceKeyData_, line)(file, line, name, prettyName, scopeName); \
PXR_NS::TraceScopeAuto TF_PP_CAT(TraceScopeAuto_, line)(\
    TF_PP_CAT(TraceKeyData_, line));

#define _TRACE_MARKER_INSTANCE(file, line, name) \
constexpr static PXR_NS::TraceStaticKeyData \
    TF_PP_CAT(TraceKeyData_, line)(file, line, name); \
    TraceCollector::GetInstance().MarkerEventStatic(TF_PP_CAT(TraceKeyData_, line));

#define _TRACE_COUNTER_INSTANCE(file, line, name, value, isDelta) \
constexpr static PXR_NS::TraceStaticKeyData \
    TF_PP_CAT(TraceKeyData_, line)(file, line, name); \
static PXR_NS::TraceCounterHolder \
    TF_PP_CAT(TraceCounterHolder_, line) \
    (TF_PP_CAT(TraceKeyData_, line)); \
TF_PP_CAT(TraceCounterHolder_, line).Record(value, isDelta);

#define _TRACE_COUNTER_CODE_INSTANCE(file, line, name, code, isDelta) \
static PXR_NS::TraceCounterHolder \
    TF_PP_CAT(TraceCounterHolder_, line)(file, line, name); \
if (TF_PP_CAT(TraceCounterHolder_, line).IsEnabled()) { \
    double value = 0.0; \
    code \
    TF_PP_CAT(TraceCounterHolder_, line).RecordDelta(value, isDelta); \
}

#define _TRACE_FUNCTION_DYNAMIC_INSTANCE(file, line, fnName, fnPrettyName, name) \
PXR_NS::TraceAuto TF_PP_CAT(TraceAuto_, line)(file, line, fnName, fnPrettyName, name)

#define _TRACE_SCOPE_DYNAMIC_INSTANCE(file, line, str) \
PXR_NS::TraceAuto TF_PP_CAT(TraceAuto_, line)(file, line, str)

#define _TRACE_MARKER_DYNAMIC_INSTANCE(name) \
    TraceCollector::GetInstance().MarkerEvent(name);

#else // TRACE_DISABLE

#define TRACE_FUNCTION()
#define TRACE_FUNCTION_DYNAMIC(name)
#define TRACE_SCOPE(name)
#define TRACE_SCOPE_DYNAMIC(name)
#define TRACE_FUNCTION_SCOPE(name)
#define TRACE_MARKER(name)
#define TRACE_MARKER_DYNAMIC(name)

#endif // TRACE_DISABLE

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
/// \class TraceScopeAuto
///
/// A class which records a timestamp when it is created and a
/// scope event when it is destructed.
///
class TraceScopeAuto {
public:
    /// Constructor for TRACE_FUNCTION macro.
    ///
    explicit TraceScopeAuto(const TraceStaticKeyData& key) noexcept
        : _key(&key)
        , _intervalTimer(/*start=*/TraceCollector::IsEnabled()) {
#if(TRACE_CUSTOM_CALLBACK)
        if (ARCH_UNLIKELY(g_traceCustomCallback)) {
            g_traceCustomCallback->BeginStatic(key, &_userData);
        }
#endif // TRACE_CUSTOM_CALLBACK
    }

    /// Constructor that also records scope arguments.
    ///
    template < typename... Args>
    TraceScopeAuto(const TraceStaticKeyData& key, Args&&... args)
        : TraceScopeAuto(key) {
        if (ARCH_UNLIKELY(_key)) {
            TraceCollector::GetInstance().ScopeArgs(std::forward<Args>(args)...);
        }
#if(TRACE_CUSTOM_CALLBACK)
        if (ARCH_UNLIKELY(g_traceCustomCallback)) {
            g_traceCustomCallback->BeginStatic(key, &_userData);
        }
#endif // TRACE_CUSTOM_CALLBACK
    }

    /// Destructor.
    ///
    ~TraceScopeAuto() noexcept {
        if (_intervalTimer.IsStarted()) {
            TraceCollector::TimeStamp stopTicks =
                _intervalTimer.GetCurrentTicks();
            TraceCollector::Scope(
                *_key, _intervalTimer.GetStartTicks(), stopTicks);
        }
#if(TRACE_CUSTOM_CALLBACK)
        if (ARCH_UNLIKELY(g_traceCustomCallback)) {
            g_traceCustomCallback->End(&_userData);
        }
#endif // TRACE_CUSTOM_CALLBACK
    }
    
private:
    const TraceStaticKeyData* const _key;
    ArchIntervalTimer _intervalTimer;
#if(TRACE_CUSTOM_CALLBACK)
    void* _userData = nullptr;
#endif // TRACE_CUSTOM_CALLBACK
};

////////////////////////////////////////////////////////////////////////////////
/// \class TraceAuto
///
/// A class which records a begin event when it is constructed, and a matching
/// end event when it is destructed.  It is intended to help ensure begin-end
/// pairing correctness when the begin-end pairing can be expressed by automatic
/// variable lifetime.
/// 
/// The TRACE_FUNCTION() macro may be even more convenient in some
/// circumstances.
///
struct TraceAuto {
    /// Constructor taking function name, pretty function name and a scope name.
    ///
    TraceAuto(const char* file, int line, const char *funcName, const char *prettyFuncName,
              const std::string &name) 
        : _key(file, line, _CreateKeyString(funcName, prettyFuncName, name)) {
        std::atomic_thread_fence(std::memory_order_seq_cst);
        _collector = &TraceCollector::GetInstance();
        _collector->BeginEvent(_key);
        std::atomic_thread_fence(std::memory_order_seq_cst);
#if(TRACE_CUSTOM_CALLBACK)
        if (ARCH_UNLIKELY(g_traceCustomCallback)) {
            g_traceCustomCallback->BeginDynamic(_key, &_userData);
        }
#endif // TRACE_CUSTOM_CALLBACK
    }

    /// Constructor taking a TfToken key.
    ///
    explicit TraceAuto(const char* file, int line, const TfToken& key)
        : _key(file, line, key) {
        std::atomic_thread_fence(std::memory_order_seq_cst);
        _collector = &TraceCollector::GetInstance();
        _collector->BeginEvent(_key);
        std::atomic_thread_fence(std::memory_order_seq_cst);
#if(TRACE_CUSTOM_CALLBACK)
        if (ARCH_UNLIKELY(g_traceCustomCallback)) {
            g_traceCustomCallback->BeginDynamic(_key, &_userData);
        }
#endif // TRACE_CUSTOM_CALLBACK
    }

    /// Constructor taking a string key.
    ///
    explicit TraceAuto(const char* file, int line, const std::string& key) 
        : TraceAuto(file, line, TfToken(key)) {}

    // Non-copyable
    //
    TraceAuto(const TraceAuto &) = delete;
    TraceAuto& operator=(const TraceAuto &) = delete;

    // Non-movable
    //
    TraceAuto(TraceAuto &&) = delete;
    TraceAuto& operator=(TraceAuto &&) = delete;

    /// Destructor.
    ///
    ~TraceAuto() {
        std::atomic_thread_fence(std::memory_order_seq_cst);
        _collector->EndEvent(_key);
        std::atomic_thread_fence(std::memory_order_seq_cst);
#if(TRACE_CUSTOM_CALLBACK)
        if (ARCH_UNLIKELY(g_traceCustomCallback)) {
            g_traceCustomCallback->End(&_userData);
        }
#endif // TRACE_CUSTOM_CALLBACK
    }

private:
    static std::string _CreateKeyString(
        const char *funcName,
        const char *prettyFuncName,
        const std::string &name) {
        std::string key = ArchGetPrettierFunctionName(funcName, prettyFuncName);
        key += " [";
        key += name;
        key += "]";
        return key;
    }

    TraceCollector* _collector;
    TraceDynamicKey _key;
#if(TRACE_CUSTOM_CALLBACK)
    void* _userData = nullptr;
#endif // TRACE_CUSTOM_CALLBACK
};

////////////////////////////////////////////////////////////////////////////////
/// \class TraceCounterHolder
///
/// Holds on to a counter key, as well as the global
/// collector for fast lookup.
///
class TraceCounterHolder {
public:
    /// Constructor used by TRACE_COUNTER_* macro.
    ///
    explicit TraceCounterHolder(const TraceKey& key) 
        : _key(key) {}

    /// Returns whether the TraceCollector is enabled or not.
    ///
    bool IsEnabled() const {
        return TraceCollector::IsEnabled();
    }

    /// Records a counter delta \p value if the TraceCollector is enabled.
    ///
    void Record(double value, bool delta) {
        if (delta) {
            TraceCollector::GetInstance().RecordCounterDelta(_key, value);
        } else {
            TraceCollector::GetInstance().RecordCounterValue(_key, value);
        }
    }

private:
    TraceKey _key;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TRACE_TRACE_H
