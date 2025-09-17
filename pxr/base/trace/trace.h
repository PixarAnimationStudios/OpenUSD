//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TRACE_TRACE_H
#define PXR_BASE_TRACE_TRACE_H

/// \file trace/trace.h

#include "pxr/pxr.h"

#include "pxr/base/trace/api.h"
#include "pxr/base/trace/collector.h"

#include "pxr/base/tf/preprocessorUtilsLite.h"

#include <atomic>

#if !defined(TRACE_DISABLE)

/// Records a timestamp when constructed and a timespan event when destructed,
/// using the name of the function or method as the key.
#define TRACE_FUNCTION() \
        _TRACE_FUNCTION_INSTANCE(__LINE__, __ARCH_FUNCTION__, __ARCH_PRETTY_FUNCTION__)

/// Records a timestamp when constructed and a timespan event when destructed,
/// using \a name as the key.
#define TRACE_SCOPE(name) \
        _TRACE_SCOPE_INSTANCE(__LINE__, name)

/// Records a timestamp when constructed and a timespan event when destructed,
/// using the name of the function concatenated with \a name as the key.
#define TRACE_FUNCTION_SCOPE(name) \
        _TRACE_FUNCTION_SCOPE_INSTANCE( \
            __LINE__, __ARCH_FUNCTION__, __ARCH_PRETTY_FUNCTION__, name)

/// Records a timestamp when constructed, using \a name as the key.
#define TRACE_MARKER(name) \
        _TRACE_MARKER_INSTANCE(__LINE__, name)

/// Records a timestamp when constructed, using \a name as the key.
#define TRACE_MARKER_DYNAMIC(name) \
        _TRACE_MARKER_DYNAMIC_INSTANCE(__LINE__, name)

/// Records a counter \a delta using the \a name as the counter key. The delta can
/// be positive or negative. A positive delta will increment the total counter
/// value, whereas a negative delta will decrement it. The recorded value will
/// be stored at the currently traced scope, and will propagate up to the
/// parent scopes.
#define TRACE_COUNTER_DELTA(name, delta) \
        _TRACE_COUNTER_INSTANCE(__LINE__, name, delta, /* isDelta */ true)

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
        _TRACE_COUNTER_INSTANCE(__LINE__, name, value, /* isDelta */ false)

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
        _TRACE_COUNTER_CODE_INSTANCE(__LINE__, name, code, true)

/// Records a begin event when constructed and an end event when destructed,
/// using name of the function or method and the supplied name as the key. 
/// Unlike TRACE_FUNCTION, the name argument will be evaluated each time this 
/// macro is invoked. This allows for a single TRACE_FUNCTION to track time 
/// under different keys, but incurs greater overhead.
#define TRACE_FUNCTION_DYNAMIC(name) \
        _TRACE_FUNCTION_DYNAMIC_INSTANCE(__LINE__, __ARCH_FUNCTION__, __ARCH_PRETTY_FUNCTION__, name)

/// Records a begin event when constructed and an end event when destructed,
/// using \a name as the key. Unlike TRACE_SCOPE, the name argument will
/// be evaluated each time this macro is invoked. This allows for a single 
/// TRACE_SCOPE to track time under different keys, but incurs greater 
/// overhead.
#define TRACE_SCOPE_DYNAMIC(name) \
        _TRACE_SCOPE_DYNAMIC_INSTANCE(__LINE__, name)


/// These pair a uniquely named TraceScopeHolder with a TraceScopeAuto.
/// Together these will register a TraceScope only the first time the
/// code is executed or if the TraceScope expires.  Otherwise, the held
/// TraceScope will be used to record begin and end events.


#define _TRACE_FUNCTION_INSTANCE(instance, name, prettyName) \
constexpr static PXR_NS::TraceStaticKeyData \
    TF_PP_CAT(TraceKeyData_, instance)(name, prettyName); \
PXR_NS::TraceScopeAuto TF_PP_CAT(TraceScopeAuto_, instance)(\
    TF_PP_CAT(TraceKeyData_, instance));

#define _TRACE_SCOPE_INSTANCE(instance, name) \
constexpr static PXR_NS::TraceStaticKeyData \
    TF_PP_CAT(TraceKeyData_, instance)(name); \
PXR_NS::TraceScopeAuto TF_PP_CAT(TraceScopeAuto_, instance)(\
    TF_PP_CAT(TraceKeyData_, instance));

#define _TRACE_FUNCTION_SCOPE_INSTANCE(instance, name, prettyName, scopeName) \
constexpr static PXR_NS::TraceStaticKeyData \
    TF_PP_CAT(TraceKeyData_, instance)(name, prettyName, scopeName); \
PXR_NS::TraceScopeAuto TF_PP_CAT(TraceScopeAuto_, instance)(\
    TF_PP_CAT(TraceKeyData_, instance));

#define _TRACE_MARKER_INSTANCE(instance, name) \
constexpr static PXR_NS::TraceStaticKeyData \
    TF_PP_CAT(TraceKeyData_, instance)(name); \
    TraceCollector::GetInstance().MarkerEventStatic(TF_PP_CAT(TraceKeyData_, instance));

#define _TRACE_COUNTER_INSTANCE(instance, name, value, isDelta) \
constexpr static PXR_NS::TraceStaticKeyData \
    TF_PP_CAT(TraceKeyData_, instance)(name); \
static PXR_NS::TraceCounterHolder \
    TF_PP_CAT(TraceCounterHolder_, instance) \
    (TF_PP_CAT(TraceKeyData_, instance)); \
TF_PP_CAT(TraceCounterHolder_, instance).Record(value, isDelta);

#define _TRACE_COUNTER_CODE_INSTANCE(instance, name, code, isDelta) \
static PXR_NS::TraceCounterHolder \
    TF_PP_CAT(TraceCounterHolder_, instance)(name); \
if (TF_PP_CAT(TraceCounterHolder_, instance).IsEnabled()) { \
    double value = 0.0; \
    code \
    TF_PP_CAT(TraceCounterHolder_, instance).RecordDelta(value, isDelta); \
}

#define _TRACE_FUNCTION_DYNAMIC_INSTANCE(instance, fnName, fnPrettyName, name) \
PXR_NS::TraceAuto TF_PP_CAT(TraceAuto_, instance)(fnName, fnPrettyName, name)

#define _TRACE_SCOPE_DYNAMIC_INSTANCE(instance, str) \
PXR_NS::TraceAuto TF_PP_CAT(TraceAuto_, instance)(str)

#define _TRACE_MARKER_DYNAMIC_INSTANCE(instance, name) \
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
    }

    /// Constructor that also records scope arguments.
    ///
    template < typename... Args>
    TraceScopeAuto(const TraceStaticKeyData& key, Args&&... args)
        : _key(&key)
        , _intervalTimer(/*start=*/false) {
        if (TraceCollector::IsEnabled()) {
            _intervalTimer.Start();
            TraceCollector
                ::GetInstance().ScopeArgs(std::forward<Args>(args)...);
        }
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
    }
    
private:
    const TraceStaticKeyData* const _key;
    ArchIntervalTimer _intervalTimer;
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
    TraceAuto(const char *funcName, const char *prettyFuncName,
              const std::string &name) 
        : _key(_CreateKeyString(funcName, prettyFuncName, name)) {
        std::atomic_thread_fence(std::memory_order_seq_cst);
        _collector = &TraceCollector::GetInstance();
        _collector->BeginEvent(_key);
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }

    /// Constructor taking a TfToken key.
    ///
    explicit TraceAuto(const TfToken& key)
        : _key(key) {
        std::atomic_thread_fence(std::memory_order_seq_cst);
        _collector = &TraceCollector::GetInstance();
        _collector->BeginEvent(_key);
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }

    /// Constructor taking a string key.
    ///
    explicit TraceAuto(const std::string& key) 
        : TraceAuto(TfToken(key)) {}

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
