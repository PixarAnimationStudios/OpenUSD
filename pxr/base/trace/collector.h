//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TRACE_COLLECTOR_H
#define PXR_BASE_TRACE_COLLECTOR_H

#include "pxr/pxr.h"

#include "pxr/base/trace/api.h"
#include "pxr/base/trace/concurrentList.h"
#include "pxr/base/trace/collection.h"
#include "pxr/base/trace/event.h"
#include "pxr/base/trace/key.h"
#include "pxr/base/trace/threads.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/mallocTag.h"

#include "pxr/base/tf/pyTracing.h"

#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/base/tf/weakPtr.h"

#include "pxr/base/arch/pragmas.h"

#include <atomic>
#include <string>
#include <vector>

#include <tbb/spin_mutex.h>

PXR_NAMESPACE_OPEN_SCOPE

class TraceScopeHolder;
 
TF_DECLARE_WEAK_PTRS(TraceCollector);
TF_DECLARE_WEAK_AND_REF_PTRS(TraceScope);


////////////////////////////////////////////////////////////////////////////////
/// \class TraceCollector
///
/// This is a singleton class that records TraceEvent instances and populates 
/// TraceCollection instances.
///
/// All public methods of TraceCollector are safe to call from any thread.
class TraceCollector : public TfWeakBase {
public:

    TF_MALLOC_TAG_NEW("Trace", "TraceCollector");

    using This = TraceCollector;
    using ThisPtr = TraceCollectorPtr;

    using TimeStamp = TraceEvent::TimeStamp;

    using Key = TraceDynamicKey;

    /// Returns the singleton instance.
    TRACE_API static TraceCollector& GetInstance() {
         return TfSingleton<TraceCollector>::GetInstance();
    }
    
    TRACE_API ~TraceCollector();

    ///  Enables or disables collection of events for DefaultCategory.
    TRACE_API void SetEnabled(bool isEnabled);

    ///  Returns whether collection of events is enabled for DefaultCategory.
    static bool IsEnabled() {
        return (_isEnabled.load(std::memory_order_acquire) == 1);
    }

    /// Default Trace category which corresponds to events stored for TRACE_
    /// macros.
    struct DefaultCategory {
        /// Returns TraceCategory::Default.
        static constexpr TraceCategoryId GetId() { return TraceCategory::Default;}
        /// Returns the result of TraceCollector::IsEnabled.
        static bool IsEnabled() { return TraceCollector::IsEnabled(); }
    };

#ifdef PXR_PYTHON_SUPPORT_ENABLED
    /// Returns whether automatic tracing of all python scopes is enabled.
    bool IsPythonTracingEnabled() const {
        return _isPythonTracingEnabled.load(std::memory_order_acquire) != 0;
    }

    /// Set whether automatic tracing of all python scopes is enabled.
    TRACE_API void SetPythonTracingEnabled(bool enabled);
#endif // PXR_PYTHON_SUPPORT_ENABLED

    /// Return the overhead cost to measure a scope.
    TRACE_API TimeStamp GetScopeOverhead() const;
    
    /// Clear all pending events from the collector. No TraceCollection will be 
    /// made for these events.
    TRACE_API void Clear();

    /// \name Event Recording
    /// @{

    /// Record a begin event with \a key if \p Category is enabled.
    /// A matching end event is expected some time in the future.
    ///
    /// If the key is known at compile time \c BeginScope and \c Scope methods 
    /// are preferred because they have lower overhead.
    /// \returns The TimeStamp of the TraceEvent or 0 if the collector is 
    /// disabled.
    /// \sa BeginScope \sa Scope
    template <typename Category = DefaultCategory>
    TimeStamp BeginEvent(const Key& key) {
        if (ARCH_LIKELY(!Category::IsEnabled())) {
            return 0;
        } 
        return _BeginEvent(key, Category::GetId());
    }

    /// Record a begin event with \a key at a specified time if \p Category is 
    /// enabled.
    /// This version of the method allows the passing of a specific number of
    /// elapsed milliseconds, \a ms, to use for this event.
    /// This method is used for testing and debugging code.
    template <typename Category = DefaultCategory>
    void BeginEventAtTime(const Key& key, double ms) {
      if (ARCH_LIKELY(!Category::IsEnabled())) {
          return;
      }
      _BeginEventAtTime(key, ms, Category::GetId());
    }

    /// Record an end event with \a key if \p Category is enabled.
    /// A matching begin event must have preceded this end event.
    ///
    /// If the key is known at compile time EndScope and Scope methods are
    /// preferred because they have lower overhead.
    /// \returns The TimeStamp of the TraceEvent or 0 if the collector is 
    /// disabled.
    /// \sa EndScope \sa Scope
    template <typename Category = DefaultCategory>
    TimeStamp EndEvent(const Key& key) {
        if (ARCH_LIKELY(!Category::IsEnabled())) {
            return 0;
        }
        return _EndEvent(key, Category::GetId());
    }

    /// Record an end event with \a key at a specified time if \p Category is
    /// enabled.
    /// This version of the method allows the passing of a specific number of
    /// elapsed milliseconds, \a ms, to use for this event.
    /// This method is used for testing and debugging code.
    template <typename Category = DefaultCategory>
    void EndEventAtTime(const Key& key, double ms) {
        if (ARCH_LIKELY(!Category::IsEnabled())) {
            return;
        }
        _EndEventAtTime(key, ms, Category::GetId());
    }


    /// Record a marker event with \a key if \p Category is enabled.
    /// Unlike begin/end, there is no matching event for marker events
    ///

    template <typename Category = DefaultCategory>
    TimeStamp MarkerEvent(const Key& key) {
        if (ARCH_LIKELY(!Category::IsEnabled())) {
            return 0;
        } 
        return _MarkerEvent(key, Category::GetId());
    }

    /// Record a marker event with \a key at a specified time if \p Category is 
    /// enabled.
    /// This version of the method allows the passing of a specific number of
    /// elapsed milliseconds, \a ms, to use for this event.
    /// This method is used for testing and debugging code.
    template <typename Category = DefaultCategory>
    void MarkerEventAtTime(const Key& key, double ms) {
      if (ARCH_LIKELY(!Category::IsEnabled())) {
          return;
      }
      _MarkerEventAtTime(key, ms, Category::GetId());
    }

    /// Record a begin event for a scope described by \a key if \p Category is
    /// enabled.
    /// It is more efficient to use the \c Scope method than to call both
    /// \c BeginScope and \c EndScope.
    /// \sa EndScope \sa Scope
    template <typename Category = DefaultCategory>
    void BeginScope(const TraceKey& _key) {
        if (ARCH_LIKELY(!Category::IsEnabled()))
            return;

        _BeginScope(_key, Category::GetId());
    }

    /// Record a begin event for a scope described by \a key and a specified
    /// category and store data arguments if \p Category is enabled.
    /// The variadic arguments \a args must be an even number of parameters in 
    /// the form TraceKey, Value.
    /// \sa EndScope \sa Scope \sa StoreData
    template <typename Category, typename... Args>
    void BeginScope(
        const TraceKey& key, Args&&... args) {
        static_assert( sizeof...(Args) %2 == 0, 
            "Data arguments must come in pairs");

        if (ARCH_LIKELY(!Category::IsEnabled()))
            return;

        _PerThreadData *threadData = _GetThreadData();
        threadData->BeginScope(key, Category::GetId());
        _StoreDataRec(threadData, Category::GetId(), std::forward<Args>(args)...);
    }

    /// Record a begin event for a scope described by \a key and store data 
    /// arguments if \p Category is enabled. The variadic arguments \a args must
    /// be an even number of parameters in the form TraceKey, Value.
    /// \sa EndScope \sa Scope \sa StoreData
    template <typename... Args>
    void BeginScope(const TraceKey& key, Args&&... args) {
        static_assert( sizeof...(Args) %2 == 0, 
            "Data arguments must come in pairs");

        // Explicitly cast to TraceCategoryId so overload resolution choose the
        // version with a category arguement.
        BeginScope<DefaultCategory>(key, 
            std::forward<Args>(args)...);
    }

    /// Record an end event described by  \a key if \p Category is enabled.
    /// It is more efficient to use the \c Scope method than to call both
    /// \c BeginScope and \c EndScope.
    /// \sa BeginScope \sa Scope
    template <typename Category = DefaultCategory>
    void EndScope(const TraceKey& key) {
        if (ARCH_LIKELY(!Category::IsEnabled()))
            return;

        _EndScope(key, Category::GetId());
    }

    /// Record a scope event described by \a key that started at \a start for
    /// the DefaultCategory.
    ///
    /// This method is used by the TRACE_FUNCTION, TRACE_SCOPE and
    /// TRACE_FUNCTION_SCOPE macros.
    /// \sa BeginScope \sa EndScope
    TRACE_API
    static void
    Scope(const TraceKey& key, TimeStamp start, TimeStamp stop) noexcept;

    /// Record a scope event described by \a key that started at \a start if 
    /// \p Category is enabled.
    ///
    /// This method is used by the TRACE_FUNCTION, TRACE_SCOPE and
    /// TRACE_FUNCTION_SCOPE macros.
    /// \sa BeginScope \sa EndScope
    template <typename Category = DefaultCategory>
    void Scope(const TraceKey& key, TimeStamp start, TimeStamp stop) {
        if (ARCH_LIKELY(!Category::IsEnabled()))
            return;
        _PerThreadData *threadData = _GetThreadData();
        threadData->EmplaceEvent(
            TraceEvent::Timespan, key, start, stop, Category::GetId());
    }
    
    /// Record multiple data events with category \a cat if \p Category is 
    /// enabled.
    /// \sa StoreData
    template <typename Category, typename... Args>
    void ScopeArgs(Args&&... args) {
        static_assert( sizeof...(Args) %2 == 0, 
            "Data arguments must come in pairs");

        if (ARCH_LIKELY(!Category::IsEnabled()))
            return;

        _PerThreadData *threadData = _GetThreadData();
        _StoreDataRec(threadData, Category::GetId(), std::forward<Args>(args)...);
    }

    /// Record multiple data events with the default category if collection of 
    /// events is enabled.
    /// The variadic arguments \a args must be an even number of parameters in 
    /// the form TraceKey, Value. It is more efficient to use this method to 
    /// store multiple data items than to use multiple calls to \c StoreData.
    /// \sa StoreData
    template <typename... Args>
    void ScopeArgs(Args&&... args) {
        static_assert( sizeof...(Args) %2 == 0, 
            "Data arguments must come in pairs");

        ScopeArgs<DefaultCategory>(std::forward<Args>(args)...);
    }


    /// Record a scope event described by \a key that started at \a start if 
    /// \p Category is enabled.
    ///
    /// This method is used by the TRACE_FUNCTION, TRACE_SCOPE and
    /// TRACE_FUNCTION_SCOPE macros.
    /// \sa BeginScope \sa EndScope
    template <typename Category = DefaultCategory>
    void MarkerEventStatic(const TraceKey& key) {
        if (ARCH_LIKELY(!Category::IsEnabled()))
            return;

        _PerThreadData *threadData = _GetThreadData();
        threadData->EmplaceEvent(
            TraceEvent::Marker, key, Category::GetId());
    }

    /// Record a data event with the given \a key and \a value if \p Category is
    /// enabled. \a value may be  of any type which a TraceEvent can
    /// be constructed from (bool, int, std::string, uint64, double).
    /// \sa ScopeArgs
    template <typename Category = DefaultCategory, typename T>
    void StoreData(const TraceKey &key, const T& value) {
        if (ARCH_UNLIKELY(Category::IsEnabled())) {
            _StoreData(_GetThreadData(), key, Category::GetId(), value);
        }
    }

    /// Record a counter \a delta for a name \a key if \p Category is enabled.
    template <typename Category = DefaultCategory>
    void RecordCounterDelta(const TraceKey &key, 
                            double delta) {
        // Only record counter values if the collector is enabled.
        if (ARCH_UNLIKELY(Category::IsEnabled())) {
            _PerThreadData *threadData = _GetThreadData();
            threadData->EmplaceEvent(
                TraceEvent::CounterDelta, key, delta, Category::GetId());
        }
    }

    /// Record a counter \a delta for a name \a key if \p Category is enabled.
    template <typename Category = DefaultCategory>
    void RecordCounterDelta(const Key &key, double delta) {
        if (ARCH_UNLIKELY(Category::IsEnabled())) {
            _PerThreadData *threadData = _GetThreadData();
            threadData->CounterDelta(key, delta, Category::GetId());
        }
    }

    /// Record a counter \a value for a name \a key if \p Category is enabled.
    template <typename Category = DefaultCategory>
    void RecordCounterValue(const TraceKey &key, double value) {
        // Only record counter values if the collector is enabled.
        if (ARCH_UNLIKELY(Category::IsEnabled())) {
            _PerThreadData *threadData = _GetThreadData();
            threadData->EmplaceEvent(
                TraceEvent::CounterValue, key, value, Category::GetId());
        }
    }

    /// Record a counter \a value for a name \a key and delta \a value if 
    /// \p Category is enabled.
    template <typename Category = DefaultCategory>
    void RecordCounterValue(const Key &key, double value) {

        if (ARCH_UNLIKELY(Category::IsEnabled())) {
            _PerThreadData *threadData = _GetThreadData();
            threadData->CounterValue(key, value, Category::GetId());
        }
    }

    /// @}

    ///  Return the label associated with this collector.
    const std::string& GetLabel() {
        return _label;
    }

    /// Produces a TraceCollection from all the events that recorded in the 
    /// collector and issues a TraceCollectionAvailable notice. Note that
    /// creating a collection restarts tracing, i.e. events contained in this
    /// collection will not be present in subsequent collections.
    TRACE_API void CreateCollection();

private:

    TraceCollector();

    friend class TfSingleton<TraceCollector>;

    class _PerThreadData;

    // Return a pointer to existing per-thread data or create one if none
    // exists.
    TRACE_API _PerThreadData* _GetThreadData() noexcept;

    TRACE_API TimeStamp _BeginEvent(const Key& key, TraceCategoryId cat);

    TRACE_API void _BeginEventAtTime(
        const Key& key, double ms, TraceCategoryId cat);

    TRACE_API TimeStamp _EndEvent(const Key& key, TraceCategoryId cat);

    TRACE_API void _EndEventAtTime(
         const Key& key, double ms, TraceCategoryId cat);

    TRACE_API TimeStamp _MarkerEvent(const Key& key, TraceCategoryId cat);

    TRACE_API void _MarkerEventAtTime(
        const Key& key, double ms, TraceCategoryId cat);

    // This is the fast execution path called from the TRACE_FUNCTION
    // and TRACE_SCOPE macros
    void _BeginScope(const TraceKey& key, TraceCategoryId cat)
    {
        // Note we're not calling _NewEvent, don't need to cache key
        _PerThreadData *threadData = _GetThreadData();
        threadData->BeginScope(key, cat);
    }

    // This is the fast execution path called from the TRACE_FUNCTION
    // and TRACE_SCOPE macros    
    TRACE_API void _EndScope(const TraceKey& key, TraceCategoryId cat);

    TRACE_API void _MeasureScopeOverhead();

#ifdef PXR_PYTHON_SUPPORT_ENABLED
    // Callback function registered as a python tracing function.
    void _PyTracingCallback(const TfPyTraceInfo &info);
#endif // PXR_PYTHON_SUPPORT_ENABLED

    // Implementation for small data that can stored inlined with the event.
    template <typename T,
        typename std::enable_if<
            sizeof(T) <= sizeof(uintptr_t) 
            && !std::is_pointer<T>::value , int>::type = 0>
    void _StoreData(_PerThreadData* threadData, const TraceKey &key,
        TraceCategoryId cat, const T& value) {
        threadData->StoreData(key, value, cat);
    }

    // Implementation for data that must be stored outside of the events.
    template <typename T,
        typename std::enable_if<
            (sizeof(T) > sizeof(uintptr_t))
            && !std::is_pointer<T>::value, int>::type = 0>
    void _StoreData(_PerThreadData* threadData, const TraceKey &key,
        TraceCategoryId cat,  const T& value) {
        threadData->StoreLargeData(key, value, cat);
    }

    // Specialization for c string
    void _StoreData(
        _PerThreadData* threadData, 
        const TraceKey &key, 
        TraceCategoryId cat, 
        const char* value) {
        threadData->StoreLargeData(key, value, cat);
    }

    // Specialization for std::string
    void _StoreData(
        _PerThreadData* threadData,
        const TraceKey &key,
        TraceCategoryId cat, 
        const std::string& value) {
        threadData->StoreLargeData(key, value.c_str(), cat);
    }

    // Variadic version to store multiple data events in one function call.
    template <typename K, typename T, typename... Args>
    void _StoreDataRec(
        _PerThreadData* threadData, TraceCategoryId cat, K&& key, 
        const T& value, Args&&... args) {
        _StoreData(threadData, std::forward<K>(key), cat, value);
        _StoreDataRec(threadData, cat, std::forward<Args>(args)...);
    }

    // Base case to terminate template recursion
    void _StoreDataRec(_PerThreadData* threadData, TraceCategoryId cat) {}


    // Thread-local storage, accessed via _GetThreadData()
    //
    class _PerThreadData {
        public:
            using EventList = TraceCollection::EventList;

            _PerThreadData();
            ~_PerThreadData();

            const TraceThreadId& GetThreadId() const {
                return _threadIndex;
            }
            TimeStamp BeginEvent(const Key& key, TraceCategoryId cat);
            TimeStamp EndEvent(const Key& key, TraceCategoryId cat);
            TimeStamp MarkerEvent(const Key& key, TraceCategoryId cat);

            // Debug Methods
            void BeginEventAtTime(
                const Key& key, double ms, TraceCategoryId cat);
            void EndEventAtTime(const Key& key, double ms, TraceCategoryId cat);
            void MarkerEventAtTime(const Key& key, double ms, TraceCategoryId cat);

            void BeginScope(const TraceKey& key, TraceCategoryId cat) {
                AtomicRef lock(_writing);
                _BeginScope(key, cat);
            }

            void EndScope(const TraceKey& key, TraceCategoryId cat) {
                AtomicRef lock(_writing);
                _EndScope(key, cat);
            }

            TRACE_API void CounterDelta(
                const Key&, double value, TraceCategoryId cat);

            TRACE_API void CounterValue(
                const Key&, double value, TraceCategoryId cat);

            template <typename T>
            void StoreData(
                const TraceKey& key, const T& data, TraceCategoryId cat) {
                AtomicRef lock(_writing);
                _events.load(std::memory_order_acquire)->EmplaceBack(
                    TraceEvent::Data, key, data, cat);
            }

            template <typename T>
            void StoreLargeData(
                const TraceKey& key, const T& data, TraceCategoryId cat) {
                AtomicRef lock(_writing);
                EventList* events = _events.load(std::memory_order_acquire);
                const auto* cached = events->StoreData(data);
                events->EmplaceBack(TraceEvent::Data, key, cached, cat);
            }

            template <typename... Args>
            void EmplaceEvent(Args&&... args) {
                AtomicRef lock(_writing);
                _events.load(std::memory_order_acquire)->EmplaceBack(
                    std::forward<Args>(args)...);
            }

#ifdef PXR_PYTHON_SUPPORT_ENABLED
            void PushPyScope(const Key& key, bool enabled);
            void PopPyScope(bool enabled);
#endif // PXR_PYTHON_SUPPORT_ENABLED

            // These methods can be called from threads at the same time as the 
            // other methods.
            std::unique_ptr<EventList> GetCollectionData();
            void Clear();

        private:
            void _BeginScope(const TraceKey& key, TraceCategoryId cat) {
                _events.load(std::memory_order_acquire)->EmplaceBack(
                    TraceEvent::Begin, key, cat);
            }

            void _EndScope(const TraceKey& key, TraceCategoryId cat);

            // Flag to let other threads know that the list is being written to.
            mutable std::atomic<bool> _writing;
            std::atomic<EventList*> _events;

            class AtomicRef {
            public:
                AtomicRef(std::atomic<bool>& b) : _bool(b) {
                    _bool.store(true, std::memory_order_release);
                }
                ~AtomicRef() {
                    _bool.store(false, std::memory_order_release);
                }
            private:
                std::atomic<bool>& _bool;
            };

            // An integer that is unique for each thread launched by any
            // threadDispatcher.  Each time a thread is Start-ed it get's
            // a new id.
            //
            TraceThreadId _threadIndex;

            // When auto-tracing python frames, this stores the stack of scopes.
            struct PyScope {
                Key key;
            };
            std::vector<PyScope> _pyScopes;
    };

    TRACE_API static std::atomic<int> _isEnabled;

    // A list with one _PerThreadData per thread.
    TraceConcurrentList<_PerThreadData> _allPerThreadData;

    std::string _label;

    TimeStamp _measuredScopeOverhead;

    // These members are unused if Python support is disabled. However, we
    // leave them in place and just mark them unused to provide ABI
    // compatibility between USD builds with and without Python enabled.
#ifndef PXR_PYTHON_SUPPORT_ENABLED    
    ARCH_PRAGMA_PUSH
    ARCH_PRAGMA_UNUSED_PRIVATE_FIELD
#endif
    std::atomic<int> _isPythonTracingEnabled;
    TfPyTraceFnId _pyTraceFnId;
#ifndef PXR_PYTHON_SUPPORT_ENABLED
    ARCH_PRAGMA_POP
#endif
};
 
TRACE_API_TEMPLATE_CLASS(TfSingleton<TraceCollector>);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TRACE_COLLECTOR_H
