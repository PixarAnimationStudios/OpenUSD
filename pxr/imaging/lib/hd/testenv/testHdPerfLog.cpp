#include "pxr/imaging/hd/renderIndex.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>
#include <cmath>

static bool
_IsClose(double a, double b)
{
    return std::abs(a - b) < 0.0000001;
}

void
CounterTest()
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    TfToken foo("foo");
    TfToken bar("bar");

    // Make sure the log is disabled.
    perfLog.Disable();

    // Performance logging is disabled, expect no tracking
    perfLog.IncrementCounter(foo);
    TF_VERIFY(perfLog.GetCounter(foo) == 0);
    perfLog.DecrementCounter(foo);
    TF_VERIFY(perfLog.GetCounter(foo) == 0);
    perfLog.AddCounter(foo, 5);
    TF_VERIFY(perfLog.GetCounter(foo) == 0);
    perfLog.SubtractCounter(foo, 6);
    TF_VERIFY(perfLog.GetCounter(foo) == 0);
    // Macros
    HD_PERF_COUNTER_DECR(foo);
    TF_VERIFY(perfLog.GetCounter(foo) == 0);
    HD_PERF_COUNTER_INCR(foo);
    TF_VERIFY(perfLog.GetCounter(foo) == 0);
    HD_PERF_COUNTER_SET(foo, 42);
    TF_VERIFY(perfLog.GetCounter(foo) == 0);
    HD_PERF_COUNTER_ADD(foo, 5);
    TF_VERIFY(perfLog.GetCounter(foo) == 0);
    HD_PERF_COUNTER_SUBTRACT(foo, 6);
    TF_VERIFY(perfLog.GetCounter(foo) == 0);

    // Enable logging
    perfLog.Enable();
    // Still expect zero
    TF_VERIFY(perfLog.GetCounter(foo) == 0);

    // Incr, Decr, Set
    perfLog.IncrementCounter(foo);
    TF_VERIFY(perfLog.GetCounter(foo) == 1);
    perfLog.DecrementCounter(foo);
    TF_VERIFY(perfLog.GetCounter(foo) == 0);
    perfLog.SetCounter(foo, 42);
    TF_VERIFY(perfLog.GetCounter(foo) == 42);
    perfLog.AddCounter(foo, 5);
    TF_VERIFY(perfLog.GetCounter(foo) == 47);
    perfLog.SubtractCounter(foo, 6);
    TF_VERIFY(perfLog.GetCounter(foo) == 41);

    perfLog.SetCounter(bar, .1);
    TF_VERIFY(_IsClose(perfLog.GetCounter(bar), .1));
    perfLog.IncrementCounter(bar);
    TF_VERIFY(_IsClose(perfLog.GetCounter(bar), 1.1));
    perfLog.DecrementCounter(bar);
    TF_VERIFY(_IsClose(perfLog.GetCounter(bar), 0.1));

    perfLog.SetCounter(foo, 0);
    perfLog.SetCounter(bar, 0);

    // Macros
    HD_PERF_COUNTER_DECR(foo);
    TF_VERIFY(perfLog.GetCounter(foo) == -1);
    HD_PERF_COUNTER_INCR(foo);
    TF_VERIFY(perfLog.GetCounter(foo) == 0);
    HD_PERF_COUNTER_SET(foo, 42);
    TF_VERIFY(perfLog.GetCounter(foo) == 42);
    HD_PERF_COUNTER_DECR(foo);
    TF_VERIFY(perfLog.GetCounter(foo) == 41);
    HD_PERF_COUNTER_INCR(foo);
    TF_VERIFY(perfLog.GetCounter(foo) == 42);
    HD_PERF_COUNTER_ADD(foo, 5);
    TF_VERIFY(perfLog.GetCounter(foo) == 47);
    HD_PERF_COUNTER_SUBTRACT(foo, 6);
    TF_VERIFY(perfLog.GetCounter(foo) == 41);

    HD_PERF_COUNTER_SET(bar, 0.1);
    TF_VERIFY(_IsClose(perfLog.GetCounter(bar), 0.1));
    HD_PERF_COUNTER_DECR(bar);
    TF_VERIFY(_IsClose(perfLog.GetCounter(bar), -0.9));
    HD_PERF_COUNTER_INCR(bar);
    TF_VERIFY(_IsClose(perfLog.GetCounter(bar), 0.1));

    // When the log is disabled, we expect to still be able to read the existing
    // values.
    perfLog.Disable();
    TF_VERIFY(perfLog.GetCounter(foo) == 41);
    perfLog.IncrementCounter(foo);
    TF_VERIFY(perfLog.GetCounter(foo) == 41);
    perfLog.DecrementCounter(foo);
    TF_VERIFY(perfLog.GetCounter(foo) == 41);
    perfLog.SetCounter(foo, 0);
    TF_VERIFY(perfLog.GetCounter(foo) == 41);
    perfLog.AddCounter(foo, 5);
    TF_VERIFY(perfLog.GetCounter(foo) == 41);
    perfLog.SubtractCounter(foo, 6);
    TF_VERIFY(perfLog.GetCounter(foo) == 41);
}

void
CacheTest()
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    TfToken foo("foo");
    TfToken bar("bar");
    SdfPath id("/Some/Path");
    TfTokenVector emptyNames;
    TfTokenVector populatedNames;
    populatedNames.push_back(bar);
    populatedNames.push_back(foo);

    // Make sure the log is disabled.
    perfLog.Disable();
    
    // Performance logging is disabled, expect no tracking.
    TF_VERIFY(perfLog.GetCacheHits(foo) == 0);
    TF_VERIFY(perfLog.GetCacheMisses(foo) == 0);
    TF_VERIFY(perfLog.GetCacheHitRatio(foo) == 0);
    TF_VERIFY(perfLog.GetCacheHits(bar) == 0);
    TF_VERIFY(perfLog.GetCacheMisses(bar) == 0);
    TF_VERIFY(perfLog.GetCacheHitRatio(bar) == 0);
    TF_VERIFY(perfLog.GetCacheNames() == emptyNames);

    // Enable perf logging.
    perfLog.Enable();
    // Nothing should have changed yet.
    TF_VERIFY(perfLog.GetCacheHits(foo) == 0);
    TF_VERIFY(perfLog.GetCacheMisses(foo) == 0);
    TF_VERIFY(perfLog.GetCacheHitRatio(foo) == 0);
    TF_VERIFY(perfLog.GetCacheHits(bar) == 0);
    TF_VERIFY(perfLog.GetCacheMisses(bar) == 0);
    TF_VERIFY(perfLog.GetCacheHitRatio(bar) == 0);
    TF_VERIFY(perfLog.GetCacheNames() == emptyNames);

    perfLog.AddCacheHit(foo, id);
    perfLog.AddCacheHit(foo, id);
    perfLog.AddCacheMiss(foo, id);
    perfLog.AddCacheMiss(foo, id);
    TF_VERIFY(perfLog.GetCacheHits(foo) == 2);
    TF_VERIFY(perfLog.GetCacheMisses(foo) == 2);
    TF_VERIFY(_IsClose(perfLog.GetCacheHitRatio(foo), .5));

    TF_VERIFY(perfLog.GetCacheHits(bar) == 0);
    TF_VERIFY(perfLog.GetCacheMisses(bar) == 0);
    TF_VERIFY(_IsClose(perfLog.GetCacheHitRatio(bar), 0));
    perfLog.AddCacheHit(bar, id);
    perfLog.AddCacheHit(bar, id);
    perfLog.AddCacheHit(bar, id);
    perfLog.AddCacheMiss(bar, id);
    TF_VERIFY(perfLog.GetCacheHits(bar) == 3);
    TF_VERIFY(perfLog.GetCacheMisses(bar) == 1);
    TF_VERIFY(_IsClose(perfLog.GetCacheHitRatio(bar), .75));

    if (perfLog.GetCacheNames() != populatedNames) {
        TfTokenVector names = perfLog.GetCacheNames();
        TF_FOR_ALL(i, names) {
            std::cout << *i << "\n";
        }
    }
    TF_VERIFY(perfLog.GetCacheNames() == populatedNames);
    
    // Make sure the log is disabled.
    perfLog.Disable();
 
    // We still expect to read results, even when disabled.
    TF_VERIFY(perfLog.GetCacheHits(foo) == 2);
    TF_VERIFY(perfLog.GetCacheMisses(foo) == 2);
    TF_VERIFY(_IsClose(perfLog.GetCacheHitRatio(foo), .5));
    TF_VERIFY(perfLog.GetCacheHits(bar) == 3);
    TF_VERIFY(perfLog.GetCacheMisses(bar) == 1);
    TF_VERIFY(_IsClose(perfLog.GetCacheHitRatio(bar), .75));

    if (perfLog.GetCacheNames() != populatedNames) {
        TfTokenVector names = perfLog.GetCacheNames();
        TF_FOR_ALL(i, names) {
            std::cout << *i << "\n";
        }
    }
    TF_VERIFY(perfLog.GetCacheNames() == populatedNames);
}

int main()
{
    TfErrorMark mark;

    CounterTest();
    CacheTest();

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

