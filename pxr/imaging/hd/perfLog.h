//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_PERF_LOG_H
#define PXR_IMAGING_HD_PERF_LOG_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/debugCodes.h"

#include "pxr/base/trace/trace.h"

#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/token.h"

#include "pxr/base/tf/hashmap.h"

#include <memory>
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE


class SdfPath;
class HdResourceRegistry;

// XXX: it would be nice to move this into Trace or use the existing Trace
// counter mechanism, however we are restricted to TraceLite in the rocks.

//----------------------------------------------------------------------------//
// PERFORMANCE INSTURMENTATION MACROS                                         //
//----------------------------------------------------------------------------//

// Emits a trace scope tagged for the function.
#define HD_TRACE_FUNCTION() TRACE_FUNCTION()
// Emits a trace scope with the specified tag.
#define HD_TRACE_SCOPE(tag) TRACE_SCOPE(tag)

// Adds a cache hit for the given cache name, the id is provided for debugging,
// see HdPerfLog for details.
#define HD_PERF_CACHE_HIT(name, id) \
    HdPerfLog::GetInstance().AddCacheHit(name, id);
#define HD_PERF_CACHE_HIT_TAG(name, id, tag) \
    HdPerfLog::GetInstance().AddCacheHit(name, id, tag);

// Adds a cache miss for the given cache name, the id is provided for debugging,
// see HdPerfLog for details.
#define HD_PERF_CACHE_MISS(name, id) \
    HdPerfLog::GetInstance().AddCacheMiss(name, id);
#define HD_PERF_CACHE_MISS_TAG(name, id, tag) \
    HdPerfLog::GetInstance().AddCacheMiss(name, id, tag);

// Increments/Decrements/Sets/Adds/Subtracts a named performance counter
// see HdPerfLog for details.
#define HD_PERF_COUNTER_INCR(name) \
    HdPerfLog::GetInstance().IncrementCounter(name);
#define HD_PERF_COUNTER_DECR(name) \
    HdPerfLog::GetInstance().DecrementCounter(name);
#define HD_PERF_COUNTER_SET(name, value) \
    HdPerfLog::GetInstance().SetCounter(name, value);
#define HD_PERF_COUNTER_ADD(name, value) \
    HdPerfLog::GetInstance().AddCounter(name, value);
#define HD_PERF_COUNTER_SUBTRACT(name, value) \
    HdPerfLog::GetInstance().SubtractCounter(name, value);

//----------------------------------------------------------------------------//
// PERFORMANCE LOG                                                            //
//----------------------------------------------------------------------------//

/// \class HdPerfLog
///
/// Performance counter monitoring.
///
class HdPerfLog
{
public:
    HD_API
    static HdPerfLog& GetInstance() {
        return TfSingleton<HdPerfLog>::GetInstance();
    }

    /// Tracks a cache hit for the named cache, the id and tag are reported
    /// when debug logging is enabled.
    HD_API
    void AddCacheHit(TfToken const& name,
                     SdfPath const& id,
                     TfToken const& tag=TfToken());

    /// Tracks a cache miss for the named cache, the id and tag are reported
    /// when debug logging is enabled.
    HD_API
    void AddCacheMiss(TfToken const& name,
                      SdfPath const& id,
                      TfToken const& tag=TfToken());

    HD_API
    void ResetCache(TfToken const& name);

    /// Gets the hit ratio (numHits / totalRequests) of a cache performance
    /// counter.
    HD_API
    double GetCacheHitRatio(TfToken const& name);

    /// Gets the number of hit hits for a cache performance counter.
    HD_API
    size_t GetCacheHits(TfToken const& name);

    /// Gets the number of hit misses for a cache performance counter.
    HD_API
    size_t GetCacheMisses(TfToken const& name);

    /// Returns the names of all cache performance counters.
    HD_API
    TfTokenVector GetCacheNames();

    /// Returns a vector of all performance counter names.
    HD_API
    TfTokenVector GetCounterNames();

    /// Increments a named counter by 1.0.
    HD_API
    void IncrementCounter(TfToken const& name);

    /// Decrements a named counter by 1.0.
    HD_API
    void DecrementCounter(TfToken const& name);

    /// Sets the value of a named counter.
    HD_API
    void SetCounter(TfToken const& name, double value);

    /// Adds value to a named counter.
    HD_API
    void AddCounter(TfToken const& name, double value);

    /// Subtracts value to a named counter.
    HD_API
    void SubtractCounter(TfToken const& name, double value);

    /// Returns the current value of a named counter.
    HD_API
    double GetCounter(TfToken const& name);

    /// Reset all conter values to 0.0. 
    /// Note that this doesn't reset cache counters.
    HD_API
    void ResetCounters();

    /// Enable performance logging.
    void Enable() { _enabled = true; }

    /// Disable performance logging.
    void Disable() { _enabled = false; }

    /// Add a resource registry to the tracking.
    HD_API
    void AddResourceRegistry(HdResourceRegistry * resourceRegistry);

    /// Remove Resource Registry from the tracking.
    HD_API
    void RemoveResourceRegistry(HdResourceRegistry * resourceRegistry);

    /// Returns a vector of resource registry.
    HD_API
    std::vector<HdResourceRegistry*> const& GetResourceRegistryVector();

private:
     
    // Don't allow copies
    HdPerfLog(const HdPerfLog &) = delete;
    HdPerfLog &operator=(const HdPerfLog &) = delete;

    friend class TfSingleton<HdPerfLog>;
    HD_API HdPerfLog();
    HD_API ~HdPerfLog();

    // Tracks number of hits and misses and provides some convenience API.
    class _CacheEntry {
    public:
        _CacheEntry() : _hits(0), _misses(0) { }

        void AddHit() {++_hits;}
        size_t GetHits() {return _hits;}

        void AddMiss() {++_misses;}
        size_t GetMisses() {return _misses;}

        size_t GetTotal() {return _hits+_misses;}
        double GetHitRatio() {return (double)_hits / GetTotal();}

        void Reset() { _hits = 0; _misses = 0; }
    private:
        size_t _hits;
        size_t _misses;
    };

    // Cache performance counters.
    typedef TfHashMap<TfToken, _CacheEntry, TfToken::HashFunctor> _CacheMap;
    _CacheMap _cacheMap;

    // Named value counters.
    typedef TfHashMap<TfToken, double, TfToken::HashFunctor> _CounterMap;
    _CounterMap _counterMap;

    // Resource registry vector.
    std::vector<HdResourceRegistry *> _resourceRegistryVector;

    // Enable / disable performance tracking.
    bool _enabled;
    std::mutex _mutex;
    typedef std::lock_guard<std::mutex> _Lock;
};

HD_API_TEMPLATE_CLASS(TfSingleton<HdPerfLog>);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_PERF_LOG_H
