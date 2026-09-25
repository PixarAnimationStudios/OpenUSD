//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/mallocTag.h"

#include "pxr/base/tf/bigRWMutex.h"
#include "pxr/base/tf/buffer.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/pointerAndBits.h"
#include "pxr/base/tf/pxrTslExt.h"
#include "pxr/base/tf/pxrTslRobinMap/robin_map.h"
#include "pxr/base/tf/spinMutex.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/tf.h"

#include "pxr/base/arch/align.h"
#include "pxr/base/arch/attributes.h"
#include "pxr/base/arch/debugger.h"
#include "pxr/base/arch/hash.h"
#include "pxr/base/arch/mallocHook.h"
#include "pxr/base/arch/math.h"
#include "pxr/base/arch/prefetch.h"
#include "pxr/base/arch/stackTrace.h"
#include "pxr/base/arch/threads.h"
#include "pxr/base/arch/virtualMemory.h"

#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_unordered_set.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <iostream>
#include <istream>
#include <mutex>
#include <ostream>
#include <regex>
#include <stack>
#include <string>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <vector>

using std::map;
using std::make_pair;
using std::pair;
using std::string;
using std::vector;

// TfMallocTag Implementation
// ============================================================================
//
// This file implements the malloc/free interception and accounting behind
// TfMallocTag.  For what the feature does and how to use it, see
// mallocTagOverview.dox.
//
// This documentation block covers the design goal, the observations the design
// rests on, the event pipeline, and the invariants and assumptions that no
// single function owns.  Everything else including the correctness arguments
// for individual mechanisms lives next to the code it constrains.
//
// The Problem
// ----------------------------------------------------------------------------
//
// TfMallocTag intercepts every malloc and free in the process and attributes
// each live allocation to a path through a tree of user-pushed tags.  The
// interception is unconditional, so the cost of the bookkeeping directly
// impacts application code, often on very hot paths.  The design goal is
// therefore narrow and specific: make the work done on application threads as
// close to nothing as possible, and pay elsewhere to achieve it.
//
// Why a Non-Intrusive Approach
// ----------------------------------------------------------------------------
//
// Pixar once commissioned a variant of ptmalloc3 from its author that let us
// store a 24-bit call-site index in unused bits of each allocation's control
// block.  That is a structurally cheaper mechanism than anything here, and not
// by a small factor.  The call-site and the allocation size were located
// adjacent to the allocation itself, making accounting nearly trivially free.
//
// As Menv30/Presto replaced Menv2x, user sessions ran longer, switching between
// shots and assets without restarting.  These exposed ptmalloc3's poor heap
// fragmentation behavior over time, with sessions eventually running out of
// memory despite the application calling free() appropriately.  jemalloc, which
// was designed for high performance long-running server workloads proved to be
// both faster and better at handling extended sessions, so it replaced
// ptmalloc3.  We rewrote TfMallocTag to be allocator agnostic not only to
// support jemalloc but also any possible future allocator we might adopt.
//
// Pixar's internal pxmalloc wrapper library hosts jemalloc and provides the old
// glibc-style hooks that this file employs via ArchMallocHook.
//
// Key Observations
// ----------------------------------------------------------------------------
//
// Most allocations are transient.  Under typical workloads the majority of heap
// allocations are freed soon after they are made, never contributing to
// steady-state memory use.  If a transient allocation and its free can be
// recognized and discarded before any shared state is touched, then the entire
// cost of tracking it is a handful of thread-local instructions and no added
// memory traffic.
//
// Moving CPU cycles off the application thread is nearly always a win; moving
// memory traffic is not.  Our workstations have many cores, and user-facing
// workloads rarely saturate all of them.  CPU work handed to a dedicated
// low-priority background thread therefore is a near pure benefit to the
// application thread in wall-clock terms.  But cores are not the only shared
// resource; cache lines and memory bandwidth cannot be cleanly handed off
// without impacting the application.  So the guiding principle is to move
// logic/instructions off the application threads, but try to minimize shifting
// memory traffic.  Canceling transient allocations on the application threads,
// for example, costs a little CPU there but more than pays for itself in the
// resulting saved memory traffic that would be paid handing those alloc & free
// events to a background thread.
//
// For any piece of information the system needs, there is a latest pipeline
// stage at which it could be produced and the stages get cheaper as you go
// because they run at decreasing frequencies.  An allocating application thread
// runs hundreds of millions of times.  The background consolidator thread runs
// per event buffer spill and on a 10s-of-milliseconds sampling interval.
// Report generation runs only when a caller requests it.  It pays to defer
// computing something until the last possible moment.
//
// The Pipeline
// ----------------------------------------------------------------------------
//
// The observations suggest a three-stage pipeline.
//
// 1. Record events & cancel transients.  This is the work done by application
//    threads during malloc & free calls.  Each thread appends alloc and free
//    events to its own fixed-capacity buffer.  If a free cancels a recent alloc
//    both events are discarded.  There is no cross-thread coordination in the
//    common case, keeping the hot path cheap.
//
// 2. Spill and consolidate.  When a thread's buffer fills, it hands the whole
//    buffer off to global state and takes an empty one in exchange.  That is
//    the end of the application thread's involvement in this stage.  The
//    background consolidator thread takes and merges event buffers into a
//    persistent baseline: the set of allocations currently believed live, keyed
//    by address.  The consolidator also drains thread event buffers
//    opportunistically at a regular sampling interval to ensure high-water
//    estimate accuracy.
//
// 3. Report.  Report generation drains and consolidates all event buffers from
//    all threads, then builds the report from the consolidated baseline.
//
// Everything of note that application threads do lives in stage 1.  Stage 2 is
// a couple of pointer swaps for an application thread; all the expensive work
// belongs to the background consolidator thread.
//
//
// The Cancellation Condition
// ----------------------------------------------------------------------------
//
// As mentioned, canceling transient allocations locally is a significant win
// with respect to memory traffic.  Doing this correctly relies on the
// "cancellation condition".  This is a part of the design worth understanding.
//
// The question we must answer is: when a thread sees a free for a recently
// allocated address, can it be certain the free truly matches the allocation?
// If so, both events cancel and can be discarded.
//
// Matching addresses alone is not enough.  Consider thread 1 allocating under
// tag t1 and later seeing a free for the same address:
//
//   thread 1: ----- A_t1 -------------------------- F -----
//
// It is tempting to cancel the pair.  But suppose thread 2 sees:
//
//   thread 2: ------------- F ---------- A_t2 -------------
//
// for the same address.  This happens if thread 1 hands the pointer to thread
// 2, thread 2 frees it, then does another allocation A_t2 and the allocator
// reuses the same address.  Thread 2 then hands this pointer back to thread 1
// which frees it.  Thread 1's F appears to match its A_t1 but it does not.
// A_t1 is dead, freed by thread 2.  The free is for A_t2 which thread 1 never
// saw.  If thread 1 cancels A_t1 here, A_t2 is incorrectly treated as
// persistently live.  Thread 1 must rule out this possibility to correctly
// cancel the pair.
//
// The solution is to count frees by address.  Every free increments a global
// atomic counter associated with its address, the "free count".  Every event
// (both alloc and free) records the current free count when it occurred.
//
// The key invariant is: if the free count for an address has not changed since
// an alloc event was recorded, then that address cannot have been freed in the
// interim by any thread.  If it was, it would have incremented the free count.
// So when a free arrives and the thread has a prior alloc event showing the
// same free count, it is a definitive match and both events may be discarded.
//
// It is impractical to record a free count for literally every address, so
// instead we use a fixed-size table of free counters and assign addresses to
// counters by hashing.  This means unrelated addresses share counters, and a
// free increments the counter for all hash-equivalent addresses.  The
// consequence is simply that some valid cancellations are missed and left to
// the consolidator thread to deal with later.  This is a missed optimization,
// not a correctness problem.
//
// Cancellation Must Not Orphan an Earlier Event
// ----------------------------------------------------------------------------
//
// Cancellation removes an alloc/free pair from a thread's buffer.  For it to
// stay correct, no event that the baseline or a later cancellation still needs
// may be discarded early.  In particular, a free event is the only record that
// an address died, and the baseline relies on it to supersede an earlier alloc
// of that same address (see IsNewer()).
//
// Suppose a thread's buffer holds an alloc A and a later free F for one address
// that did not cancel each other, and the address is then reused by a new alloc
// A'.  It is tempting to have A' overwrite F in place, on the reasoning that
// the newest event for an address makes older ones obsolete.  But if A'
// overwrites F and is itself later canceled, F is gone and nothing remains to
// supersede A, which is then reported live for the rest of the process.
//
// To prevent this, alloc events are only ever appended, never overwritten by
// others.  Appending can leave several events for one address in the buffer,
// which is harmless because cancellation defends itself:
// _FindCancellableEventForAddr() only ever returns the event provably newest
// for its address, so the older duplicates are inert.  See
// TestCancelDoesNotOrphanEarlierAlloc().
//
// Assumption: TfMallocTag Never Frees Memory It Did Not Allocate
// ----------------------------------------------------------------------------
//
// This is a whole-program property that no single site enforces, so it is
// stated here.  It is essential for correct cancellation.
//
// Cancellation is sound because any free that could have released an address to
// another thread bumped its free count.  However, allocs and frees performed
// while tracking is disabled do not record events.  These are for allocations
// related to TfMallocTag's own bookkeeping and everything the consolidator
// thread does, for example.  The frees still bump their free count, so they
// cannot cause a wrong cancellation, but since they record no event, if such a
// free ever released an address that _had_ been recorded, the recorded alloc
// would have nothing to retire it and would be treated as live forever.
//
// What makes it safe is that the disabled windows only ever cover TfMallocTag
// freeing its own allocations -- buffers, nodes, tables, the consolidator's
// maps -- all of which were also allocated while disabled and so were never
// recorded.  Nothing in the code checks this though.
//
// TfMallocTag does not currently have a way to support taking ownership of or
// freeing memory that application code allocated.  If a need for this arises,
// TfMallocTag will need to carefully ensure that all such frees are performed
// outside a thread's tracking-disabled windows for correct event recording and
// accounting.
//
// Reentrancy
// ----------------------------------------------------------------------------
//
// TfMallocTag allocates in order to do its job.  Every such allocation must be
// prevented from recursing back into tracking.  Tracking is disabled per-thread
// around any of the system's own heap use, and the interception wrappers check
// that flag before doing anything else.  See _TemporaryDisabler.
//
// A consequence, since it may look alarming otherwise: a thread holding its own
// buffer lock may safely allocate, precisely because the disabled flag stops a
// recursive call before it tries to take the lock again.
//
// Sessions and the Epoch
// ----------------------------------------------------------------------------
//
// TfMallocTag can be shut down and reinitialized, and cleared in place.  Both
// begin a new session, and data must not leak across the boundary: buffers in
// flight, buffers queued for consolidation, and a consolidation already in
// progress all predate the new session.
//
// A single monotonic epoch counter marks the boundary.  Every thread carries
// the epoch it is recording for, and every path that would move events toward
// the baseline re-checks the epoch and discards on a mismatch.  The counter
// advances in exactly one place, _ClearAll().
//
// Lock Order
// ----------------------------------------------------------------------------
//
// Acquired outer to inner.  No path acquires them in any other order:
//
//     consolidator mutex
//     thread list  ->  per-thread  ->  spilled/recycled buffer pool
//     per-thread   ->  tagging state (read)
//     tagging state (write)  ->  baseline
//
// The baseline lock is otherwise a leaf.  In particular nothing holds it while
// acquiring the buffer pool lock, which is why drained buffers are recycled
// after releasing it rather than while holding it.
//
// The consolidator mutex comes first because stopping the consolidator must
// happen before any other lock is taken: a demand consolidation holds the
// thread list lock and every per-thread lock, so a stop requested while holding
// those would wait forever on a thread that is itself waiting on us.
//
// Known Limitations
// ----------------------------------------------------------------------------
//
// Unresolved issues in the current implementation as it stands:
//
// - Thread data lifetime.  _ThreadData is intentionally never destroyed (see
// TfMallocTag::Tls), so _UnregisterThread never runs in practice and the thread
// list grows monotonically with every thread the process has ever created.
// That is a slow leak, and it makes demand consolidation walk and lock dead
// entries forever.  Fixing this is nontrivial.  Simply letting them destruct
// introduces teardown-ordering hazards.
//
// - CallTree includes nodes with nothing live.  _BuildPathNodeChildrenTable()
// walks every path node that has ever existed, so _BuildTree() creates a
// CallTree::PathNode -- each carrying a std::string and a std::vector -- for
// all of them, which dominates the cost of a report on large workloads.  Only
// nodes the baseline mentions plus their ancestors, can carry bytes.  Pruning
// to those changes the structure callers receive rather than merely what is
// printed, and callers do rely on zero-byte nodes being present, so this cannot
// be done as an internal optimization.  Note also that the two report paths
// already disagree about zero-byte nodes: GetPrettyPrintString() does not prune
// them while Report() does.
//
// - GetMaxTotalBytes() is sampled, not continuous, but the sampling is driven
// two ways so that it tracks peaks closely.  First, the consolidator is woken
// on every spill (see _SpillThreadBuffer), so an actively-allocating thread has
// its total reflected within one buffer-fill of the background thread keeping
// up. Second, the consolidator also performs periodic passes
// (PXR_TF_MALLOC_TAG_CONSOLIDATE_PERIOD_MS) to drain all threads.  A thread
// that fills less than one buffer and then stalls or goes idle never spills, so
// without this its share of a co-live peak reaches the baseline only at the
// next demand read -- by which time its co-live partners may have freed and
// drained.  The periodic drain walks all threads and opportunistically takes
// each one's current buffer, so those events join the running baseline while
// the peak they belong to is still live.
//
// The intra-batch peak is an estimate.  Events are consumed per buffer, and
// only within a buffer are they in true time order; across buffers the order is
// arbitrary, so a free that really fell between two allocs in another buffer is
// applied after both and the running total can momentarily exceed any real
// instant.  The error is bounded by a single batch's event volume.
//
// The remaining true limit is a co-live peak that both forms and dissolves
// within a single sample period.  Noted with API doc in the header.
//
// - Report column label does not match its meaning.  Tf_MallocPathNode's live
// allocation count and CallTree::PathNode::nAllocations both mean "live
// allocations at this node", but the pretty-printed report labels the column
// "samples" -- and CallTree::LoadReport() parses that literal string, so the
// label cannot be changed without breaking every previously-saved report.
// Renaming the public field would likewise break API.  Fixing either means
// revising the report format.
//

PXR_NAMESPACE_OPEN_SCOPE

// Set to true to compile-in debug output support.
#define DEBUG_MALLOC_TAG false

TF_CONDITIONALLY_COMPILE_TIME_ENABLED_DEBUG_CODES(
    DEBUG_MALLOC_TAG,
    TF_MALLOC_TAG
    );

#if DEBUG_MALLOC_TAG
// TfMallocTag can be engaged super early, before main or before or during
// diagnostic mgr initialization, and trying to issue diagnostics at those times
// can run into infinite recursion or deadlocks, so we provide a bare-bones
// solution here for debugging TfMallocTag.
#define DBG_AXIOM(...)                                      \
    if (!(__VA_ARGS__)) {                                   \
        fprintf(stderr, "Failed: %s\n", #__VA_ARGS__);      \
        abort();                                            \
    }
#else
#define DBG_AXIOM(...)
#endif // DEBUG_MALLOC_TAG

// Forward decl.
struct Tf_MallocGlobalData;

////////////////////////////////////////////////////////////////////////
// Constants and configuration

namespace {

// The max number of captured unique malloc stacks printed in a report.
static constexpr size_t _MaxReportedMallocStacks = 100;

// The max number of call stack frames stored when malloc stack capturing is
// enabled.  Note that two malloc stacks are considered identical if all their
// frames up to this depth are matching (the uncaptured parts of the stacks can
// still differ).
static constexpr size_t _MaxMallocStackDepth = 64;

// The number of top stack frames to ignore when saving frames for a malloc
// stack.  This cuts out the _MallocWrapper and its callees which are not of
// interest to users.
static constexpr size_t _IgnoreStackFramesCount = 3;


static constexpr size_t _EventSize = 4 * sizeof(void *); // asserted below.

// Capacity of a single _EventBuf.  Use a smaller size when debugging to make
// printouts more tractable.
static constexpr size_t EventBufferCapacity = DEBUG_MALLOC_TAG ? 512 : 8192;

// The max size of the recycled event-buffer pool, in MB, converted to a buffer
// count below.  Note this bounds only the recycle pool.  Each live thread owns
// one buffer it is currently filling, and the spill queue
// (Tf_MallocGlobalData::_threadBuffers) can hold buffers awaiting
// consolidation, so under load, total event-buffer memory is not bounded by
// this setting.  Rather, it bounds the steady-state minimum event buffer memory
// under low load.
TF_DEFINE_ENV_SETTING(
    PXR_TF_MALLOC_TAG_EVENT_BUFFERS_MB, 16,
    "The amount of memory TfMallocTag keeps as a pool of recycled event "
    "collection buffers, reused to absorb spills without malloc/free churn.");
static size_t _GetFreeBufferPoolCap()
{
    static size_t numBuffers = []() {
        size_t requested = TfGetEnvSetting(PXR_TF_MALLOC_TAG_EVENT_BUFFERS_MB);
        if (requested == 0) {
            fprintf(stderr, "TfMallocTag set to use *unlimited* memory "
                    "for event buffers\n");
            return size_t(-1);
        }
        if (requested > 100ull * 1024) {
            fprintf(stderr, "PXR_TF_MALLOC_TAG_EVENT_BUFFERS_MB clamped to "
                    "100 GB -- set to zero to use unlimited memory\n");
            requested = 100ull * 1024;
        }
        return (requested * 1024 * 1024) / (EventBufferCapacity * _EventSize);
    }();
    return numBuffers;
}

// Period between opportunistic wakes of the background consolidator, in
// milliseconds.  On each wake with no spill or demand pending, the worker walks
// all threads and try-lock/drains each one's current buffer (see
// _DrainMode::Periodic), so a slowly-allocating or stalled thread's events join
// the running baseline while the high-water peak they belong to is still live,
// rather than waiting for the next demand read.  A value of 0 disables periodic
// wake.  The default is a prime number to try to avoid resonance with other
// periodic application work.
TF_DEFINE_ENV_SETTING(
    PXR_TF_MALLOC_TAG_CONSOLIDATE_PERIOD_MS, 23,
    "Period in milliseconds between opportunistic wakes of the TfMallocTag "
    "background consolidator, which drains all threads so malloc peak "
    "accounting stays close to the true high-water.  Set to 0 to disable.");
static size_t _GetConsolidatePeriodMs()
{
    static size_t periodMs =
        TfGetEnvSetting(PXR_TF_MALLOC_TAG_CONSOLIDATE_PERIOD_MS);
    return periodMs;
}

// Size of the global path node bucket directory (see _PathNodeTable).  This is
// the second user-facing knob.
//
// It is an env setting rather than a compile-time constant unlike
// _ImmortalNameSiteTable::Log2Capacity, which it otherwise resembles.  This is
// because the two tables are pressured by different things.  The immortal-name
// table is sized by the number of source sites naming a distinct immortal
// string, ~1,000 at time of writing, and bounded by what is written in the
// code, so "raise the constant and rebuild" is a reasonable answer to overflow.
//
// The path node count, by contrast, is sized by workload.  A pathological run
// is plausible here, and a runtime setting accommodates it without rebuilding.
//
// The default is 128 MB of address space, which is 2^24 buckets at 8 bytes
// each.  Resident memory is lower than that under light loads: the directory is
// reserved and committed rather than heap-allocated, so a page becomes resident
// only when a bucket on it is touched, and a run that creates few nodes touches
// few buckets.
//
// If a pathological run fills this table the consequence is just somewhat
// degraded performance.  TfMallocTag warns when this happens so you can re-run
// with an increased number.
TF_DEFINE_ENV_SETTING(
    PXR_TF_MALLOC_TAG_PATH_NODE_TABLE_MB, 128,
    "The amount of address space TfMallocTag should use for the hash table "
    "mapping a (parent, call site) pair to its path node.  Rounded down to a "
    "power of two number of buckets.  Raise this to improve performance if "
    "TfMallocTag warns that the table is overloaded.");

// Log2 of the number of buckets in the path node directory.  Derived once from
// the env setting above and clamped to a sane range: the floor keeps a tiny
// setting from degenerating into a linked list, and the ceiling keeps a typo
// from asking for a terabyte of address space.
static unsigned _GetPathNodeTableLog2Buckets()
{
    static unsigned log2Buckets = []() {
        constexpr unsigned MinLog2 = 10;  //   1 K buckets,   8 KB.
        constexpr unsigned MaxLog2 = 34;  //  16 G buckets, 128 GB.
        // Widen before scaling: the setting is an int, and anything past 2047
        // MB would overflow.  A negative or zero value clamps to MinLog2 below.
        const size_t requestedMB = static_cast<size_t>(
            std::max(0, TfGetEnvSetting(PXR_TF_MALLOC_TAG_PATH_NODE_TABLE_MB)));
        const size_t requestedBuckets =
            (requestedMB * 1024 * 1024) / sizeof(void *);
        unsigned log2 = 0;
        while ((size_t(1) << (log2 + 1)) <= requestedBuckets) {
            ++log2;
        }
        if (log2 < MinLog2) {
            log2 = MinLog2;
        }
        else if (log2 > MaxLog2) {
            log2 = MaxLog2;
        }
        return log2;
    }();
    return log2Buckets;
}

// Number of recent events Free() examines for a cancellable alloc.  Must be a
// power-of-two (index arithmetic wraps by masking) and a whole number of SWAR
// (SIMD-within-a-register) words (the window is searched eight one-byte tags at
// a time -- see _FindCancellableEventForAddr).
//
// 16 is measured, not assumed.  Both 8 and 32 were evaluated and performed
// worse on tested workloads: each SWAR word costs about 11 instructions on
// every free, but a cancellation is worth roughly 50 instructions of downstream
// work plus its share of memory traffic, so the second word earns far more than
// it costs while a third earns almost nothing.  Slots 8-15 alone were measured
// to carry about 25 percentage points of discard rate.  Measure several
// real-world workloads carefully before changing this.
static constexpr unsigned LookBackWindowSize = 16;

// Number of 64-bit words the look-back window's tags occupy.
static constexpr unsigned NumLookBackWords =
    LookBackWindowSize / sizeof(uint64_t);

static_assert(LookBackWindowSize % sizeof(uint64_t) == 0);
static_assert((LookBackWindowSize & (LookBackWindowSize-1)) == 0);

// Bits for freeCount tables.
static constexpr int FreeCountTableBits = 10;
static constexpr int FreeCountTableSize = 1 << FreeCountTableBits;

////////////////////////////////////////////////////////////////////////
// Hash utils.

// Knuth-style multiplicative hash constant (prime near 2^64 / phi).
static constexpr size_t _HashMultiplier = 11400714819323198549ULL;

// Multiplicative mix of a raw integer.  The top bits of the product are the
// best-distributed, so callers take the top N bits via >> (bits - N) to get an
// index in [0, 2^N).
static inline size_t
_HashMix(size_t v)
{
    return v * _HashMultiplier;
}

// Return the raw multiplicative hash of a pointer.  All address hashing in this
// file is derived from this single function by right-shifting the result to the
// desired bit width.
static inline size_t
_HashAddr(void const *p)
{
    return _HashMix(reinterpret_cast<uintptr_t>(p));
}

struct _HashAddrObj
{
    inline size_t operator()(void const *p) const {
        return _HashAddr(p);
    }
};

// Open-address pointer-keyed hash tables.
template <class KeyPtr, class Value>
using _PtrRobinMap = pxr_tsl::robin_map<
    KeyPtr, Value, _HashAddrObj,
    std::equal_to<KeyPtr>,
    std::allocator<std::pair<KeyPtr, Value>>,
    /*StoreHash=*/false,
    /*GrowthPolicy=*/high_bits_power_of_two_growth_policy<2>>;

} // anon

////////////////////////////////////////////////////////////////////////
// Call site and path node infrastructure

// There is a different call-site object associated with each different tag
// string used to construct a TfAutoMallocTag.  Call-site objects are uniqued
// and immortal once created, and are intentionally leaked -- so there is no
// destructor, and `name` must outlive the program.  Tf_GetOrCreateCallSite() is
// what guarantees that; nothing else should construct one of these.
struct Tf_MallocCallSite
{
    explicit Tf_MallocCallSite(const char *name);

    char const *_name;

    static constexpr unsigned _TraceFlag = 1u;
    static constexpr unsigned _DebugFlag = 1u << 1;

    // If _TraceFlag bit is set, then capture a stack trace when allocating at
    // this site.  If _DebugFlag bit is set, then invoke the debugger trap when
    // allocating or freeing at this site.
    //
    // Writes happen only under the _taggingStateMutex write lock, but Alloc()
    // reads this with no lock at all, so it must be atomic.  Relaxed is
    // sufficient: the flags are independent of each other and of any other
    // state, so there is nothing to order against.  An allocation racing a
    // SetDebugMatchList()/SetCapturedMallocStacksMatchList() call may observe
    // either the old or the new value, which is the same latitude those calls
    // already have with respect to concurrently running allocations.  Relaxed
    // loads compile to a plain load, so this costs nothing on the hot path.
    std::atomic<unsigned> _flags;

    // Set or clear `flag`.  Callers must hold the _taggingStateMutex write
    // lock, which is what makes the non-atomic read-modify-write below safe;
    // the individual load and store are relaxed to match Alloc()'s read.
    void _SetFlag(unsigned flag, bool set) {
        const unsigned cur = _flags.load(std::memory_order_relaxed);
        _flags.store(set ? (cur | flag) : (cur & ~flag),
                     std::memory_order_relaxed);
    }
};

using Tf_PathNodeChildrenTable = _PtrRobinMap<
    Tf_MallocPathNode const *, std::vector<Tf_MallocPathNode const *>>;

// Per-node live totals for one report, accumulated from the baseline.  These
// used to be counters on the node itself; see the note on Tf_MallocPathNode.  A
// node absent from this table has no live allocations billed directly to it.
struct Tf_PathNodeCounts
{
    size_t nBytes       = 0;
    size_t nAllocations = 0;
};

using Tf_PathNodeCountsTable =
    _PtrRobinMap<Tf_MallocPathNode const *, Tf_PathNodeCounts>;

//
// Each node describes a sequence (i.e. path) of call sites.
// Recursively-encountered sites in a path can be detected by walking _parents.
//
// A path node carries no accounting of its own: live byte and allocation counts
// live in side maps built per report (see _BuildCallTree()).  It carries no
// child links either.  Lookup is a hash of (parent, site) into the global
// _PathNodeTable, so the only link is that table's collision chain, and the
// parent -> children relation is recovered per report by scanning.
//
// It also does not carry anything about repeated sites.  Whether a node's call
// site also appears among its ancestors is asked only by report building and
// only under skipRepeated so _BuildTree() derives it while descending for
// nothing -- so it is not worth O(depth) of pointer chasing per node on an
// allocating thread.
//
// What remains is entirely immutable except for _nextInBucket, which is written
// once before the node is published and never again.
struct Tf_MallocPathNode
{
    explicit Tf_MallocPathNode(Tf_MallocCallSite *callSite,
                               Tf_MallocPathNode const *parent)
        : _callSite(callSite)
        , _parent(parent)
    {
    }

    TfMallocTag::CallTree::PathNode
    _BuildTree(Tf_PathNodeChildrenTable const &nodeChildren,
               Tf_PathNodeCountsTable const &nodeCounts,
               bool skipRepeated) const;

    Tf_MallocPathNode const *_GetParent() const {
        return _parent;
    }

    Tf_MallocCallSite * const _callSite;

    // A node is never reparented, so this is write-once.
    Tf_MallocPathNode const * const _parent;

    // Next node in this node's _PathNodeTable collision chain.  Written once,
    // before the CAS that publishes this node into its bucket, and never again
    // -- the table only ever prepends, so a node's suffix of the chain is
    // stable from the moment it becomes reachable.
    std::atomic<Tf_MallocPathNode *> _nextInBucket { nullptr };
};

// Three pointers -- worth asserting: some workloads create millions of these,
// so an added field is a multi-megabyte regression, not noise.
static_assert(sizeof(Tf_MallocPathNode) == 3 * sizeof(void *));

namespace {

struct _HashEqCStr
{
    inline bool equal(char const *l, char const *r) const {
        return !strcmp(l, r);
    }
    inline size_t hash(char const *k) const {
        return TfHashCString()(k);
    }
};

using Tf_MallocCallSiteTable =
    tbb::concurrent_hash_map<
    const char*, struct Tf_MallocCallSite *, _HashEqCStr>;

static inline
Tf_MallocCallSite *
Tf_GetOrCreateCallSite(Tf_MallocCallSiteTable* table,
                       const char* name,
                       bool nameIsImmortal) {

    // Callsites persist after first insertion, so optimistically assume
    // presence.
    {
        Tf_MallocCallSiteTable::const_accessor acc;
        if (table->find(acc, name)) {
            return acc->second;
        }
    }

    // A site and its table entry are immortal, so neither may borrow a name
    // that isn't.  When nameIsImmortal the characters already are, so we adopt
    // them; for anything else we take a copy.  Held here rather than inside the
    // site so that losing the race below frees the copy instead of leaking it
    // -- Tf_MallocCallSite has no destructor, by design.
    struct _Freer { void operator()(char *p) const { free(p); } };
    std::unique_ptr<char, _Freer> nameCopy;
    char const *siteName = name;
    if (!nameIsImmortal) {
        nameCopy.reset(strdup(name));
        siteName = nameCopy.get();
    }

    // New up a site and attempt to insert it.  If we lose the race we drop the
    // one we created.
    auto newSite = std::make_unique<Tf_MallocCallSite>(siteName);

    // Key on the site's own name.
    Tf_MallocCallSiteTable::accessor acc;
    if (table->emplace(acc, newSite->_name, newSite.get())) {
        // We emplaced the new site, so release it and its name from the
        // unique_ptrs -- both are the table's now, and immortal.
        nameCopy.release();
        return newSite.release();
    }
    else {
        // We lost the race: this site has been created in the meantime.  Just
        // return the table's pointer, and let the unique_ptrs dispose of the
        // site and name we created.
        return acc->second;
    }
}


// Bump allocator for path nodes.
//
// Nodes are immortal -- nothing ever destroys or frees one -- so a
// general-purpose allocator is just overhead here.  Some workloads create
// millions of nodes.
//
// Chunks are never freed and slots are never reused.  That is what lets a
// thread which loses the _PathNodeTable publish race simply abandon the node it
// built: the waste is one slot per lost race, and races are rare.
class _PathNodeArena
{
public:
    // Return uninitialized, suitably aligned storage for one node.
    void *Allocate() {
        while (true) {
            _Chunk *chunk = _current.load(std::memory_order_acquire);
            if (chunk) {
                const size_t i =
                    chunk->nextSlot.fetch_add(1, std::memory_order_relaxed);
                if (i < NodesPerChunk) {
                    return chunk->GetSlot(i);
                }
                // Exhausted.  Note the counter keeps climbing past the end
                // while other threads discover this too; that is harmless, it
                // is only ever compared against NodesPerChunk.
            }
            // Exactly one thread installs the replacement chunk; everyone else
            // loops and retries against whatever is current by then.
            std::lock_guard<std::mutex> lock(_mutex);
            if (_current.load(std::memory_order_relaxed) == chunk) {
                _Chunk *newChunk = new _Chunk;
                _numChunks.fetch_add(1, std::memory_order_relaxed);
                _current.store(newChunk, std::memory_order_release);
            }
        }
    }

    // Total bytes of node storage committed, for the diagnostic report.
    size_t GetNumBytes() const {
        return _numChunks.load(std::memory_order_relaxed) * sizeof(_Chunk);
    }

private:
    // 64K nodes per chunk -- 1.5 MB at the current three-pointer node size.
    static constexpr size_t NodesPerChunk = 64 * 1024;

    struct _Chunk {
        void *GetSlot(size_t i) {
            return storage + i * sizeof(Tf_MallocPathNode);
        }
        std::atomic<size_t> nextSlot {0};
        alignas(Tf_MallocPathNode)
            char storage[NodesPerChunk * sizeof(Tf_MallocPathNode)];
    };

    std::atomic<_Chunk *> _current   {nullptr};
    std::atomic<size_t>   _numChunks {0};
    // A real mutex rather than a spin lock: the thread holding it does a
    // multi-megabyte allocation.
    std::mutex _mutex;
};

// Nodes are process-global and immortal, so the arena is too.  Its members are
// all constant-initialized, so there is no static initialization order hazard.
static _PathNodeArena _pathNodeArena;

// The hot paths two quantities derived from an address -- a free-count table
// index and a look-back window tag.  We cut them from distinct bit ranges of a
// single _HashAddr() product, although they could be from the same range --
// independence is not required between these values.  A multiplicative hash's
// best-distributed bits are at the top, so the index takes the topmost
// FreeCountTableBits and the tag the eight below them.
static constexpr unsigned _FreeCountIdxShift =
    sizeof(uintptr_t) * 8 - FreeCountTableBits;
static constexpr unsigned _AddrTagShift = _FreeCountIdxShift - 8;

static inline size_t
_FreeCountIdxFromHash(size_t hash)
{
    return hash >> _FreeCountIdxShift;
}

// Return the one-byte look-back window tag for a hashed address.  Zero is
// reserved to mean "empty slot", so it is folded onto one.  That makes the tag
// value 1 twice as likely as any other, which costs nothing measurable: a tag's
// only job is to reject non-matching slots cheaply, and the caller confirms
// every candidate against the real address anyway.
static inline uint8_t
_AddrTagFromHash(size_t hash)
{
    const uint8_t tag = static_cast<uint8_t>(hash >> _AddrTagShift);
    return static_cast<uint8_t>(tag + (tag == 0));
}

static size_t
_FreeCountIdx(void const *p)
{
    return _FreeCountIdxFromHash(_HashAddr(p));
}

// Hash a (parent, site) pair, for _PathNodeTable.
//
// XOR-ing the two _HashAddr products directly is not good enough.  A
// multiplicative hash puts its best-distributed bits at the top of the product,
// so the two operands would contribute their good and bad bits to the same
// positions and the bad ones would survive.  Both inputs also carry a lot of
// shared structure -- every parent comes from the same arena and most sites
// come from the same module -- so there is real correlation to destroy.
//
// So: shift one product down to bring its good bits into the low half, XOR,
// then re-multiply so that every input bit has had a chance to reach the top.
// The caller takes the top bits, as everywhere else in this file.
//
// This has been checked rather than assumed, which is worth knowing before
// anyone "improves" it.  At one workload's mean occupancy of 0.260, an ideal
// uniform hash leaves a Poisson(0.260) distribution of chain lengths.
// Conditioned on non-empty, that predicts 87.6% / 11.39% / 1.07% of occupied
// buckets holding 1 / 2 / 3-4 nodes, and 128 buckets holding exactly 5.  The
// measured figures were 87.5% / 11.4% / 1.1% and 129.  So this behaves like a
// random oracle on real (parent, site) pairs, to about 1% including the tail.
// GetPerfStats() prints the occupancy distribution; recheck it against Poisson
// if you change this.
static inline size_t
_HashParentSite(void const *parent, void const *site)
{
    // Half the word, so this stays defined on 32-bit targets.
    constexpr unsigned halfShift = sizeof(size_t) * 4;
    return _HashMix(_HashAddr(parent) ^ (_HashAddr(site) >> halfShift));
}

// The global map from (parent, call site) to path node.  This is the structure
// the tag stack resolves against, and the hottest table in the file after
// _ImmortalNameSiteTable.
//
// Properties this relies on:
//
// - Authoritative for existence, not an accelerator.  An empty bucket, or a
//   chain that ends, proves the (parent, site) pair is absent.
//
// - Insert-only, and prepend-only.  Nothing is ever erased, moved, or rehashed.
//   That is what makes a chain safe to walk with no lock: a node's suffix of
//   its chain is fixed once the node is published, so a concurrent insert can
//   only add elements in front of where a walker already is, and a walker that
//   misses one simply retries.  It is also why a lost publish race costs only a
//   rescan of the new prefix rather than the whole chain.
//
// - Never resized.  There is no growth protocol at all -- the table is sized
//   once and left alone.  A design that has to change representation would need
//   significant complex machinery.  Overflow is handled by warning and
//   degrading (longer chains), never by rehashing.  The escape hatch is an env
//   setting: PXR_TF_MALLOC_TAG_PATH_NODE_TABLE_MB.
//
// The table takes no lock, neither to scan nor to insert.  Note in particular
// that this is safe against a concurrent _BuildCallTree().  A report that
// misses a node created underneath it just omits that node from one snapshot --
// and a node that new can only have bytes at all if a spill consolidation
// landed its first allocation within the same window, so this is a tiny
// transient undercount in a snapshot that is inherently approximate.
class _PathNodeTable
{
public:
    // Reserve the bucket directory.  Called once, from the first-time branch
    // of TfMallocTag::Initialize(), before any thread can resolve a tag; a
    // second call after Shutdown()/Initialize() is a no-op, since nodes are
    // immortal and outlive a session.
    void Initialize() {
        if (_buckets) {
            return;
        }
        const unsigned log2Buckets = _GetPathNodeTableLog2Buckets();
        _shift = sizeof(size_t) * 8 - log2Buckets;
        _numBuckets = size_t(1) << log2Buckets;

        // Reserved and committed directly rather than malloc-allocated.  Pages
        // become resident only as buckets are touched.  Neither reserving nor
        // committing writes to the range -- on POSIX they are mmap(PROT_NONE)
        // and mprotect(PROT_READ|PROT_WRITE) -- so the whole 128 MB can be
        // committed here in one call, and pages arrive zero-filled on first
        // touch.
        //
        // Reinterpreting zeroed bytes as std::atomic<T *> is the same bargain
        // this file already makes for _mallocHook: on every platform we
        // support, an all-zero object representation is a null pointer with no
        // lock word alongside it.
        static_assert(sizeof(std::atomic<Tf_MallocPathNode *>) ==
                      sizeof(Tf_MallocPathNode *));
        const size_t numBytes =
            _numBuckets * sizeof(std::atomic<Tf_MallocPathNode *>);
        void *mem = ArchReserveVirtualMemory(numBytes);
        if (mem && !ArchCommitVirtualMemoryRange(mem, numBytes)) {
            ArchFreeVirtualMemory(mem, numBytes);
            mem = nullptr;
        }
        if (!mem) {
            TF_FATAL_ERROR("TfMallocTag could not reserve a %zu MB path node "
                           "table; decrease "
                           "PXR_TF_MALLOC_TAG_PATH_NODE_TABLE_MB",
                           numBytes / (1024 * 1024));
        }
        _buckets = static_cast<std::atomic<Tf_MallocPathNode *> *>(mem);
    }

    // Return the node for `site` under `parent`, creating and publishing it if
    // this is the first time that pair has been seen.  Accumulates the number
    // of chain links traversed into `numSteps` for the diagnostic stats; this
    // is the number to watch if the table ever needs a bigger directory.
    Tf_MallocPathNode *
    GetOrCreate(Tf_MallocPathNode *parent, Tf_MallocCallSite *site,
                uint64_t *numSteps)
    {
        std::atomic<Tf_MallocPathNode *> &bucket =
            _buckets[_HashParentSite(parent, site) >> _shift];

        // `scanned` is the chain suffix already proven not to hold our key.  It
        // starts null (nothing proven) and advances to whatever head we last
        // examined, so a lost publish race rescans only what was prepended in
        // the meantime.  Prepend-only is what makes that sound.
        Tf_MallocPathNode *scanned = nullptr;
        Tf_MallocPathNode *head = bucket.load(std::memory_order_acquire);
        Tf_MallocPathNode *candidate = nullptr;

        while (true) {
            // The chain links are read relaxed, which deserves an argument.
            // The acquire above pairs with the release CAS that published
            // `head`, and that CAS came after the publisher's own acquire load
            // of whatever it linked to, which in turn came after that node was
            // published.  So seeing `head` puts us after the construction of
            // every node behind it, transitively, and no per-link acquire is
            // needed to read their fields.
            for (Tf_MallocPathNode *n = head; n != scanned;
                 n = n->_nextInBucket.load(std::memory_order_relaxed)) {
                ++*numSteps;
                if (n->_callSite == site && n->_GetParent() == parent) {
                    // Found.  If we speculatively built a node, abandon it --
                    // arena slots are never reclaimed, so this leaks one slot.
                    return n;
                }
            }
            scanned = head;

            if (!candidate) {
                candidate = new (_pathNodeArena.Allocate())
                    Tf_MallocPathNode(site, parent);
            }
            candidate->_nextInBucket.store(head, std::memory_order_relaxed);
            if (bucket.compare_exchange_weak(head, candidate,
                                             std::memory_order_release,
                                             std::memory_order_acquire)) {
                _PublishNewNode(candidate);
                return candidate;
            }
            // The failed exchange refreshed `head`; loop and rescan the prefix.
        }
    }

    // Call `fn(node)` once per published node, in creation order.
    //
    template <class Fn>
    void ForEachNode(Fn const &fn) const {
        // Load the count first.  Anything appended after this point belongs to
        // a node created during the walk, which a report is already documented
        // to be allowed to miss.
        //
        // Note the guarantee that a visited node is fully constructed comes
        // from the per-slot acquire below pairing with _PublishNewNode()'s
        // release store, not from this load: the count is bumped before the
        // slot is stored, so a slot below the count may still be null.
        const size_t numNodes = _numNodes.load(std::memory_order_acquire);
        // The chunk bound is derived from this snapshot rather than read from
        // _numNodeChunks, which is a diagnostic count of chunks created and
        // skews from the node count in both directions: a chunk is created
        // after the count is bumped, so it can lag, and a read taken after ours
        // can run ahead.  Both break something -- a bound short of the snapshot
        // drops published nodes, and one past it underflows the unsigned `end`
        // below.  A chunk the bound covers but no one has created yet is caught
        // by the null check.
        const size_t numChunks =
            std::min(MaxIndexChunks,
                     (numNodes + NodesPerIndexChunk - 1) /
                     NodesPerIndexChunk);
        for (size_t c = 0; c != numChunks; ++c) {
            _NodeChunk *chunk = _nodeChunks[c].load(std::memory_order_acquire);
            if (!chunk) {
                // A later chunk can exist while this one does not: the thread
                // that claimed this chunk's first index can still be allocating
                // while other threads claim indices past it.  Skipping rather
                // than stopping bounds the loss to this chunk.
                continue;
            }
            const size_t end =
                std::min(NodesPerIndexChunk,
                         numNodes - c * NodesPerIndexChunk);
            for (size_t i = 0; i != end; ++i) {
                if (Tf_MallocPathNode *node =
                    chunk->nodes[i].load(std::memory_order_acquire)) {
                    fn(node);
                }
                // A null slot is an index claimed by a thread whose store has
                // not landed yet.  Skipping it is the same latitude as the
                // count snapshot above: that node is one created during our
                // walk.
            }
        }
    }

    // The bucket `node` hashes to.  For the occupancy histogram in
    // _GetTreeStats(), which derives the table's shape from the nodes rather
    // than by sweeping -- see ForEachNode().
    size_t GetBucketIndex(Tf_MallocPathNode const *node) const {
        return _HashParentSite(node->_GetParent(), node->_callSite) >> _shift;
    }

    // Number of nodes published here.  Excludes the root, which is created
    // directly rather than looked up, since it has no (parent, site) key.
    size_t GetNumNodes() const {
        return _numNodes.load(std::memory_order_relaxed);
    }

    size_t GetNumBuckets() const {
        return _numBuckets;
    }

    // Directory bytes, i.e. address space.  Excludes the node index, which is
    // reported with the arena it parallels.
    size_t GetNumBytes() const {
        return _numBuckets * sizeof(std::atomic<Tf_MallocPathNode *>);
    }

    // Bytes committed by the node index.
    size_t GetNodeIndexNumBytes() const {
        return _numNodeChunks.load(std::memory_order_relaxed) *
            sizeof(_NodeChunk);
    }

private:
    // Mean chain length at which we start warning.  Lookup degrades linearly
    // past this, so it is a "you are losing performance" threshold, not a
    // correctness one -- the table keeps working at any load.
    static constexpr size_t _WarnMeanChainLength = 4;

    // Claim the next index in the node enumeration order, store `node` there,
    // and check the load factor.
    //
    // The index is the reason reports do not have to sweep the directory; see
    // ForEachNode().  It is a separate structure from _PathNodeArena rather
    // than a reuse of it because the arena reserves a slot with fetch_add
    // before the node's constructor runs, so arena order cannot distinguish a
    // live node from an uninitialized slot.  Here the store happens after
    // construction and after the node is published into its bucket, so a
    // non-null slot is always a fully built node.
    void _PublishNewNode(Tf_MallocPathNode *node) {
        const size_t idx = _numNodes.fetch_add(1, std::memory_order_relaxed);
        const size_t n = idx + 1;
        if (_NodeChunk *chunk =
            _GetOrCreateNodeChunk(idx / NodesPerIndexChunk)) {
            chunk->nodes[idx % NodesPerIndexChunk].store(
                node, std::memory_order_release);
        }
        if (ARCH_UNLIKELY(n == _numBuckets * _WarnMeanChainLength)) {
            // Exactly one thread sees the crossing, so no once-flag is needed.
            fprintf(stderr,
                    "TfMallocTag: the path node table is overloaded (%zu nodes "
                    "in %zu buckets, mean chain length %zu).  Tag resolution "
                    "will slow down linearly from here.  Raise "
                    "PXR_TF_MALLOC_TAG_PATH_NODE_TABLE_MB (currently %d) to "
                    "restore full speed.\n",
                    n, _numBuckets, _WarnMeanChainLength,
                    (int)TfGetEnvSetting(
                        PXR_TF_MALLOC_TAG_PATH_NODE_TABLE_MB));
        }
    }

    // Enumeration index storage.  Chunked so that appending never reallocates
    // and so a walk needs no lock: 64K pointers per chunk is 512 KB, and 4,096
    // chunks covers 268M nodes, which is 60x the heaviest workload measured.
    // Past that we simply stop indexing -- a node missing from the index is
    // missing from reports, which is a degraded diagnostic rather than a broken
    // program, and is the same tradeoff _ImmortalNameSiteTable makes when it
    // fills.
    static constexpr size_t NodesPerIndexChunk = 64 * 1024;
    static constexpr size_t MaxIndexChunks     = 4096;

    struct _NodeChunk {
        std::atomic<Tf_MallocPathNode *> nodes[NodesPerIndexChunk];
    };

    _NodeChunk *_GetOrCreateNodeChunk(size_t c) {
        if (ARCH_UNLIKELY(c >= MaxIndexChunks)) {
            return nullptr;
        }
        if (_NodeChunk *chunk =
            _nodeChunks[c].load(std::memory_order_acquire)) {
            return chunk;
        }
        // Similar to _PathNodeArena's chunk installation, and a real mutex for
        // the same reason: the thread holding it is doing a half-megabyte
        // zero-initializing allocation, too long to spin behind.
        std::lock_guard<std::mutex> lock(_nodeChunkMutex);
        if (_NodeChunk *chunk =
            _nodeChunks[c].load(std::memory_order_relaxed)) {
            return chunk;
        }
        _NodeChunk *chunk = new _NodeChunk();  // value-init zeroes the slots
        _numNodeChunks.fetch_add(1, std::memory_order_relaxed);
        _nodeChunks[c].store(chunk, std::memory_order_release);
        return chunk;
    }

    // Set once by Initialize() before any thread can reach GetOrCreate(), and
    // never written again, so these need no synchronization of their own: the
    // release store of _initState that publishes the whole system covers them.
    std::atomic<Tf_MallocPathNode *> *_buckets = nullptr;
    size_t   _numBuckets = 0;
    unsigned _shift = 0;

    // Doubles as the node count and as the next index in the enumeration order.
    std::atomic<size_t>       _numNodes {0};

    std::atomic<_NodeChunk *> _nodeChunks[MaxIndexChunks] = {};
    std::atomic<size_t>       _numNodeChunks {0};
    std::mutex                _nodeChunkMutex;
};

// Like the arena, the table is process-global and outlives any one session.
// Its members are all constant-initialized, so there is no static
// initialization order hazard.
static _PathNodeTable _pathNodeTable;

// The hot path for resolving a tag name to its Tf_MallocCallSite.
//
// Nearly all tag traffic comes from the fixed number of string-literal or
// eternal-string tags in the code, and such a string's address is stable for
// the life of the module and unique to that string.  So we can key on the
// address and never touch the string at all: a probe is one multiplicative hash
// and one pointer compare.  Misses fall back to the content-keyed table
// (Tf_MallocCallSiteTable), which is the only structure that has to hold every
// site -- including the many thousands generated by the handful of call sites
// that pass dynamic names.
//
// Properties this relies on:
//
// - Only immortal addresses may be inserted.  Callers pass nameIsImmortal and
//   must skip this table entirely when it is false.  Exactly two kinds of name
//   set it, both admitted in TfMallocTag::Auto (mallocTag.h) -- read the
//   comment at each before widening what reaches this table:
//
//     - A const char array lvalue, per Auto::_IsImmortalCharArray.
//     - A TfEternalString, which carries the guarantee in its own type.
//
//   (Two translation units may hold distinct addresses for the same characters,
//   and so may a literal and a TfEternalString naming the same content.  That
//   is harmless -- they become two entries resolving to the one site the
//   content table deduplicated.)
//
// - Insert-only, never erased.  That lets a probe stop at the first empty slot.
//   With no erasure an empty slot proves the name was never inserted.
//
// - Fixed capacity, with a fallback rather than a grow.  A lock-free resize is
//   not worth its complexity for a table that holds a couple thousand entries
//   in Capacity slots.  If a probe chain fills up we warn once and let that
//   name go through the content-dedup table forever -- slower, but correct.  As
//   a developer-facing tool, a warning telling the user to raise Capacity and
//   rebuild is an acceptable answer.
//
// Note this table is deliberately not protected by a mutex.  It is ours alone,
// nothing iterates it, and every mutation is a CAS, so lookups are entirely
// lock-free.
class _ImmortalNameSiteTable
{
public:
    // Number of slots.  Must be a power of two.  Raise this (and rebuild) if
    // the warning from _WarnFull() fires.  At 16 bytes per slot this is 128 KB.
    static constexpr size_t Log2Capacity = 13;
    static constexpr size_t Capacity     = size_t(1) << Log2Capacity;

    // Probe chain length before we give up and treat the table as full.  At the
    // expected load factor (a couple thousand entries in 8192 slots) running
    // out of eight consecutive occupied slots is unlikely.
    static constexpr unsigned MaxProbes = 8;

    // Return the site for `name`, or null if it is not present.  Null is always
    // a safe answer: the caller resolves through the content table instead.
    Tf_MallocCallSite *Find(char const *name) const {
        size_t idx = _Index(name);
        for (unsigned i = 0; i != MaxProbes; ++i, idx = _NextProbe(idx)) {
            _Slot const &slot = _slots[idx];
            char const *slotName = slot.name.load(std::memory_order_acquire);
            if (!slotName) {
                // Empty slot ends the chain -- see "Insert-only" above.
                return nullptr;
            }
            if (slotName == name) {
                // May be null if a concurrent inserter has claimed the slot but
                // not yet published the site.  Returning null just sends the
                // caller down the content path, which yields the same site.
                return slot.site.load(std::memory_order_acquire);
            }
        }
        return nullptr;
    }

    // Associate `name` with `site`.  A no-op if `name` is already present or if
    // its probe chain is full.  `site` must already be published in the content
    // table, so that a racing Find() that returns null still resolves
    // correctly.
    void Insert(char const *name, Tf_MallocCallSite *site) {
        size_t idx = _Index(name);
        for (unsigned i = 0; i != MaxProbes; ++i, idx = _NextProbe(idx)) {
            _Slot &slot = _slots[idx];
            char const *slotName = slot.name.load(std::memory_order_acquire);
            if (slotName == name) {
                // Already ours, or a racer claimed it for this same name and is
                // about to publish the site -- it has no early exit between its
                // CAS and its store, so we can just return.
                return;
            }
            if (!slotName) {
                char const *expected = nullptr;
                if (slot.name.compare_exchange_strong(
                        expected, name,
                        std::memory_order_release,
                        std::memory_order_acquire)) {
                    // We own the slot; publish the site.  The release here
                    // pairs with the acquire in Find() so that a reader which
                    // sees the site also sees a fully constructed
                    // Tf_MallocCallSite.
                    slot.site.store(site, std::memory_order_release);
                    _numEntries.fetch_add(1, std::memory_order_relaxed);
                    return;
                }
                if (expected == name) {
                    return; // Lost the race to another insert of the same name.
                }
                // Slot went to a different name; keep probing.
            }
        }
        _WarnFull();
    }

    size_t GetNumEntries() const {
        return _numEntries.load(std::memory_order_relaxed);
    }

private:
    struct _Slot {
        std::atomic<char const *>        name {nullptr};
        std::atomic<Tf_MallocCallSite *> site {nullptr};
    };

    static size_t _Index(char const *name) {
        return _HashAddr(name) >> (sizeof(size_t) * 8 - Log2Capacity);
    }

    // Step to the next slot in a probe chain, wrapping at the end of the table.
    static size_t _NextProbe(size_t idx) {
        return (idx + 1) & (Capacity - 1);
    }

    // Warn once.  This can only fire long after startup -- it takes Capacity
    // distinct immortal-name tags to get here.
    void _WarnFull() {
        if (!_warned.exchange(true, std::memory_order_relaxed)) {
            fprintf(stderr,
                    "TfMallocTag: the immortal-name call-site table is full "
                    "(%zu entries in %zu slots).  Additional immortal-name "
                    "tags will resolve through the slower content-keyed "
                    "table.  To restore full speed, increase "
                    "_ImmortalNameSiteTable::Log2Capacity in mallocTag.cpp and "
                    "rebuild.\n",
                    GetNumEntries(), Capacity);
        }
    }

    _Slot _slots[Capacity];
    std::atomic<size_t> _numEntries {0};
    std::atomic<bool>   _warned {false};
};

////////////////////////////////////////////////////////////////////////
// Event and buffer infrastructure

// A captured stack trace -- a sequence of return address frames.  Stored by
// value in the global _StackTraceTable, which deduplicates traces by content.
// Pointers into the table are stable for the lifetime of the table since it is
// append-only.
struct _StackTrace {
    std::vector<uintptr_t> frames;

    bool operator==(_StackTrace const &other) const {
        return frames == other.frames;
    }
};

struct _StackTraceHash {
    size_t operator()(_StackTrace const &t) const {
        return ArchHash(
            reinterpret_cast<char const *>(t.frames.data()),
            sizeof(uintptr_t) * t.frames.size());
    }
};

// Global dedup table for stack traces.  Each unique sequence of frames is
// stored exactly once.  Entries are never removed -- the table grows
// monotonically and stale entries (for addresses no longer live) are harmless.
// A warning is emitted if the table grows beyond a threshold, since that may
// indicate an unexpectedly large number of unique call sites with stack trace
// capturing enabled.
struct _StackTraceTable {
    // Return a stable pointer to the canonical _StackTrace for `frames`,
    // inserting a new entry if none exists.
    _StackTrace const *GetOrCreate(std::vector<uintptr_t> &&frames) {
        auto result = _table.insert(_StackTrace { std::move(frames) });
        if (result.second &&
            ARCH_UNLIKELY(_table.size() > _WarnThreshold)) {
            fprintf(stderr,
                    "TfMallocTag stack trace table has grown to %zu unique "
                    "entries; stale traces may be accumulating.\n",
                    _table.size());
        }
        return std::addressof(*result.first);
    }

    size_t size() const {
        return _table.size();
    }

private:
    static constexpr size_t _WarnThreshold = 1000000;

    tbb::concurrent_unordered_set<_StackTrace, _StackTraceHash> _table;
};

// Cache-line align to avoid false-sharing.
struct alignas(ARCH_CACHE_LINE_SIZE) _AtomicCounter {
    std::atomic<int64_t> value {0};
};
static _AtomicCounter _globalFreeCount[FreeCountTableSize];

// Enum describing whether allocations are being tagged in an associated
// thread.
enum _TaggingState {
    _TaggingEnabled,   // Allocations are being tagged
    _TaggingDisabled,  // Allocations are not being tagged
};

// A Tf_MallocPathNode pointer and flags.
using _PathNodeAndFlags = TfPointerAndBits<Tf_MallocPathNode>;

// The last-known state of one address in the baseline: either an alloc or a
// free tombstone.  An alloc has a non-null `_nodeAndFlags` and a non-zero
// `size` (unless TfMallocTag is Paused); a free tombstone has a null
// `_nodeAndFlags` and zero `size`.  The bit of `_nodeAndFlags` is the trace
// bit, indicating that a stack trace was captured for this allocation and is
// present in the baseline trace map.
//
struct _BaselineEvent
{
    bool IsAlloc() const {
        return _nodeAndFlags.GetLiteral();
    }

    bool IsFree() const {
        return !_nodeAndFlags.GetLiteral();
    }

    bool HasTrace() const {
        return _nodeAndFlags.BitsAs<bool>();
    }

    Tf_MallocPathNode *GetNode() const {
        return _nodeAndFlags.Get();
    }

    // Temporal ordering of two events for the same address.  Equal free counts
    // need a tie-break, and the axiom below is what makes the answer
    // unambiguous rather than a guess, so it is worth the argument:
    //
    // Two events for one address with the same globalFreeCount must be a free
    // followed by an alloc with the allocator reusing the address.  Alloc then
    // alloc is impossible, since with no intervening free the first allocation
    // was never released.  Free then free is impossible, since a free
    // increments the count, so the second would be greater.  Alloc then free is
    // impossible for the same reason.  Only reuse remains, so the alloc is the
    // newer of the two.
    friend bool IsNewer(_BaselineEvent const &l, _BaselineEvent const &r) {
        if (l.globalFreeCount != r.globalFreeCount) {
            return l.globalFreeCount > r.globalFreeCount;
        }
        TF_DEV_AXIOM(l.IsAlloc() ^ r.IsAlloc());
        return l.IsAlloc();
    }

    size_t             size            = 0;
    _PathNodeAndFlags  _nodeAndFlags   = {};
    int64_t            globalFreeCount = 0;
};

// An event representing either a malloc or a free.  This derives _BaselineEvent
// and adds the address for use in per-thread event buffers.  The _BaselineEvent
// lacks an address because those live in a table keyed by address.
//
struct _Event : _BaselineEvent
{
    void const *addr = nullptr;

    // Note that the inherited `globalFreeCount` does double duty.  The baseline
    // uses it to order events for one address (see IsNewer()), and Free() uses
    // it for the cancellation condition: if the global count for this address's
    // bucket has not changed since an alloc was recorded, then no free of a
    // hash-equivalent address has happened, so a free arriving now must match
    // that alloc's.  See _HaveAllFreesSince().
};

// XXX: Uncomment once USD-9836 is fixed -- this doesn't hold on 32-bit wasm.
//static_assert(sizeof(_Event) == _EventSize);

using _EventBuf = Tf_Buffer<_Event, EventBufferCapacity>;

#if DEBUG_MALLOC_TAG
std::ostream &
operator<<(std::ostream &out, _Event const &e)
{
    char const *site =
        e.GetNode() ? e.GetNode()->_callSite->_name : "<null>";
    char const *parSite =
        e.GetNode() && e.GetNode()->_GetParent()
        ? e.GetNode()->_GetParent()->_callSite->_name : "<null>";

    if (e.IsAlloc()) {
        return out << TfStringPrintf(
            "alloc addr:%p, sz:%zd tr:%d gfc:%zu:%zd from %s -> %s",
            e.addr, e.size, e.HasTrace(), _FreeCountIdx(e.addr),
            e.globalFreeCount, parSite, site);
    }
    else {
        return out << TfStringPrintf(
            "free  addr:%p, gfc:%zu:%zd",
            e.addr, _FreeCountIdx(e.addr), e.globalFreeCount);
    }
}
#endif // DEBUG_MALLOC_TAG

#if DEBUG_MALLOC_TAG
std::ostream &
operator<<(std::ostream &out, _EventBuf const &ev)
{
    out << "--\n";
    for (_Event const &e: ev) {
        out << e << '\n';
    }
    out << "--" << std::endl;
    return out;    
}
#endif // DEBUG_MALLOC_TAG

using _TraceMap =
    std::unordered_map<void const*, _StackTrace const *, _HashAddrObj>;

// A unit of per-thread malloc tracking data.  The `traceMap` is a lookup table
// for stack traces associated with alloc events that have `HasTrace()` set.
// Entries in `traceMap` may be stale (e.g. for addresses that have since been
// freed or replaced) -- they are culled naturally during merge by only pulling
// entries for events with `HasTrace()` set.  `traceMap` is bounded in size by
// `EventBufferCapacity` since there can be at most that many events with the
// trace bit set at any one time.
//
// Allocation contract: `events` is default-constructed *unallocated* (see
// Tf_Buffer::deferAllocation) and Reset() does not allocate, so a default-
// constructed or moved-from _ThreadBuffer cannot be appended to.  Spilling
// moves a thread's buffer away wholesale, which leaves the thread's buffer
// unallocated, so every point that takes the array away must install a
// replacement before the thread can append again.  Buffers are allocated only
// via Tf_MallocGlobalData::_TakeFreeBuffer(), which recycles from _freeBuffers
// when it can, or via _PopFreeBufferLocked() plus an explicit
// events.allocate(); drained buffers go back via _PutFreeBuffer().
struct _ThreadBuffer
{
    // Empty the buffer for reuse.  Does not allocate, so this cannot bring an
    // unallocated buffer into service -- use _TakeFreeBuffer() for that.
    void Reset() {
        events.clear();
        traceMap.clear();
    }

    // Return true if this buffer owns an array and may be appended to.
    bool IsAllocated() const {
        return events.data() != nullptr;
    }

    _EventBuf events { _EventBuf::deferAllocation };
    _TraceMap traceMap;
};

////////////////////////////////////////////////////////////////////////
// Stats infrastructure

// Bucketing for the small-count histograms in _GetPerfStats(): fan-out per
// node, _PathNodeTable chain length per resolve, and bucket occupancy.  Shared
// so they can be compared directly against each other.
static constexpr size_t NumCountBuckets = 9;

static size_t
_CountBucket(uint64_t n)
{
    if (n <= 2)  { return static_cast<size_t>(n); }  // 0, 1, 2
    if (n <= 4)  { return 3; }
    if (n <= 8)  { return 4; }
    if (n <= 16) { return 5; }
    if (n <= 32) { return 6; }
    if (n <= 64) { return 7; }
    return 8;
}

// The first bucket that counts as a "long" chain (9 or more steps).  Used to
// report what share of all path node lookup work happens in overloaded buckets,
// which is the signal that the table wants a bigger directory.
static constexpr size_t FirstLongCountBucket = 5;

static char const *
_CountBucketLabel(size_t i)
{
    static constexpr char const *labels[NumCountBuckets] = {
        "0", "1", "2", "3-4", "5-8", "9-16", "17-32", "33-64", "65+"
    };
    return labels[i];
}

// Plain-data snapshot of per-thread diagnostic counters, used for aggregation
// in _GetPerfStats().  See _AtomicThreadStats.
struct _ThreadStats {
    uint64_t allocsTotal         = 0; // total allocs seen
    uint64_t freesTotal          = 0; // total frees seen
    uint64_t lookBackDiscards    = 0; // look-back alloc/free events discarded
    uint64_t spillsToGlobal      = 0; // number of times spilled to global

    uint64_t emptyTags           = 0; // number of tags containing no allocs
    uint64_t totalTags           = 0; // number of tags pushed
    uint64_t pathNodeGhostHits   = 0; // hits on popped entries above stack top
    uint64_t pathNodeCacheMisses = 0; // stack entries resolved the slow way
    uint64_t chainStepsTotal     = 0; // table chain links traversed doing so

    uint64_t siteFastHits        = 0; // name resolved by immortal address
    uint64_t siteSlowLookups     = 0; // name resolved via the content table

    // Per-session deepest tag stack depth, maintained on the push path.
    uint64_t maxStackDepth       = 0;

    // Distribution of _PathNodeTable chain length over resolves: how many
    // resolves fell in each length bucket, and how many steps those resolves
    // accounted for.
    //
    // The step sums are the point, because a mean is not enough.  A mean
    // steps-per-resolve cannot distinguish "every probe is slightly long" --
    // which means the directory is simply too small -- from "almost every probe
    // is trivial but a few are enormous", which means the hash is clustering on
    // some pathological input.  Those are completely different profiles, and
    // only the distribution tells them apart.  Read this alongside the bucket
    // occupancy histogram, which measures the table's shape rather than the
    // traffic over it.
    uint64_t chainResolves[NumCountBuckets] = {};
    uint64_t chainSteps[NumCountBuckets]    = {};
};

// Live per-thread diagnostic counters.  Many fields are written on the hot
// push/pop path without the thread's _mutex, and read by _GetPerfStats() under
// that lock.  All fields are std::atomic with relaxed ordering: the atomicity
// eliminates torn reads, and no cross-field ordering is needed since these are
// independent diagnostics.
//
// Use Snapshot() to produce a plain _ThreadStats for aggregation.
struct _AtomicThreadStats {
    static constexpr auto relaxed = std::memory_order_relaxed;
    
    std::atomic<uint64_t> allocsTotal         {0};
    std::atomic<uint64_t> freesTotal          {0};
    std::atomic<uint64_t> lookBackDiscards    {0};
    std::atomic<uint64_t> spillsToGlobal      {0};

    std::atomic<uint64_t> emptyTags           {0};
    std::atomic<uint64_t> totalTags           {0};
    std::atomic<uint64_t> pathNodeGhostHits   {0};
    std::atomic<uint64_t> pathNodeCacheMisses {0};
    std::atomic<uint64_t> chainStepsTotal     {0};

    std::atomic<uint64_t> siteFastHits        {0};
    std::atomic<uint64_t> siteSlowLookups     {0};

    std::atomic<uint64_t> maxStackDepth       {0};

    std::atomic<uint64_t> chainResolves[NumCountBuckets] = {};
    std::atomic<uint64_t> chainSteps[NumCountBuckets]    = {};

    _ThreadStats Snapshot() const {
        _ThreadStats s;
        s.allocsTotal          = allocsTotal        .load(relaxed);
        s.freesTotal           = freesTotal         .load(relaxed);
        s.lookBackDiscards     = lookBackDiscards   .load(relaxed);
        s.spillsToGlobal       = spillsToGlobal     .load(relaxed);
        s.emptyTags            = emptyTags          .load(relaxed);
        s.totalTags            = totalTags          .load(relaxed);
        s.pathNodeGhostHits    = pathNodeGhostHits  .load(relaxed);
        s.pathNodeCacheMisses  = pathNodeCacheMisses.load(relaxed);
        s.chainStepsTotal      = chainStepsTotal    .load(relaxed);
        s.siteFastHits         = siteFastHits       .load(relaxed);
        s.siteSlowLookups      = siteSlowLookups    .load(relaxed);
        s.maxStackDepth        = maxStackDepth      .load(relaxed);
        for (size_t i = 0; i != NumCountBuckets; ++i) {
            s.chainResolves[i] = chainResolves[i]   .load(relaxed);
            s.chainSteps[i]    = chainSteps[i]      .load(relaxed);
        }
        return s;
    }

    void Reset() {
        allocsTotal         .store(0, relaxed);
        freesTotal          .store(0, relaxed);
        lookBackDiscards    .store(0, relaxed);
        spillsToGlobal      .store(0, relaxed);
        emptyTags           .store(0, relaxed);
        totalTags           .store(0, relaxed);
        pathNodeGhostHits   .store(0, relaxed);
        pathNodeCacheMisses .store(0, relaxed);
        chainStepsTotal     .store(0, relaxed);
        siteFastHits        .store(0, relaxed);
        siteSlowLookups     .store(0, relaxed);
        maxStackDepth       .store(0, relaxed);
        for (size_t i = 0; i != NumCountBuckets; ++i) {
            chainResolves[i].store(0, relaxed);
            chainSteps[i]   .store(0, relaxed);
        }
    }
};

// Single-writer relaxed increment and add for _AtomicThreadStats fields.  These
// counters are only ever written by the owning thread, so a plain
// load-add-store is correct and avoids the locked RMW that fetch_add would emit
// on x86.
static inline void
_RelaxedAdd(std::atomic<uint64_t> &a, uint64_t n)
{
    a.store(a.load(std::memory_order_relaxed) + n, std::memory_order_relaxed);
}
static inline void
_RelaxedInc(std::atomic<uint64_t> &a)
{
    _RelaxedAdd(a, 1);
}

struct _GlobalStats {
    // Consolidation passes, broken down by what drove each one.  Counted per
    // actual _ConsolidateImpl() run (only the worker thread calls it), so
    // periodic timer wakes are included -- a periodic pass that finds every
    // buffer empty still counts as a pass with zero buffers.  A pass discarded
    // on an epoch race with Clear()/Shutdown() is not counted.
    //   spill    -- due to a buffer spill.
    //   periodic -- due to the wake timer (backstop for non-spilling threads).
    //   demand   -- forced by a query / report request.
    std::atomic<uint64_t> spillConsolidations    {0};
    std::atomic<uint64_t> periodicConsolidations {0};
    std::atomic<uint64_t> demandConsolidations   {0};
    // Buffers processed by consolidation, attributed to the pass that drained
    // them.  Note this is not the same as spillsToGlobal (the producer-side
    // count of buffers handed off): a spilled buffer is drained by whichever
    // pass scoops the queue first, which could also be periodic or demand.
    std::atomic<uint64_t> spillBuffers    {0};
    std::atomic<uint64_t> periodicBuffers {0};
    std::atomic<uint64_t> demandBuffers   {0};

    void Reset() {
        spillConsolidations = periodicConsolidations = demandConsolidations = 0;
        spillBuffers = periodicBuffers = demandBuffers = 0;
    }
};

} // anon

////////////////////////////////////////////////////////////////////////
// Global statics and debug/match utilities

static ArchMallocHook       _mallocHook; // zero-initialized POD
static Tf_MallocGlobalData* _mallocGlobalData = nullptr;

std::atomic<TfMallocTag::_InitState>
TfMallocTag::_initState { TfMallocTag::_NotInitialized };

static bool Tf_MatchesMallocTagDebugName(const string& name);
static bool Tf_MatchesMallocTagTraceName(const string& name);
static void Tf_MallocTagDebugHook(const void* ptr, size_t size) ARCH_NOINLINE;

static void Tf_MallocTagDebugHook(const void* ptr, size_t size)
{
    // Clients don't call this directly so the debugger can conveniently
    // see the pointer and size in the stack.
    ARCH_DEBUGGER_TRAP;
}

static inline size_t
Tf_GetMallocBlockSize(void* ptr, size_t requestedSize)
{
    // The allocator-agnostic implementation keeps track of the exact memory
    // block sizes requested by consumers. This ignores allocator-specific
    // overhead, such as alignment, associated metadata, etc. We believe this is
    // the right thing to be measuring, as malloc tags are intended to allow
    // consumers to bill memory requests to their originating subsystem.
    //
    // Uncomment the following line to enable tracking of 'actual' block sizes.
    // Be sure that the allocator in use provides this function! If not, this
    // will call the default glibc implementation, which will likely return the
    // wrong value (unless you're using the glibc allocator).

    // return malloc_usable_size(ptr);
    return requestedSize;
}

/*
 * Utility for checking a const char* against a table of match strings.  Each
 * string is tested against each item in the table in order.  Each item can
 * either allow or deny the string, with later entries overriding earlier
 * results.  Match strings can end in '*' to wildcard the suffix and can start
 * with '-' to deny or '+' or nothing to allow.
 *
 * Match strings are concatenated into lists using commas, newlines or tabs.
 * Spaces are not delimiters but they are trimmed from each end.
 */
class Tf_MallocTagStringMatchTable {
public:
    Tf_MallocTagStringMatchTable();
    explicit Tf_MallocTagStringMatchTable(const std::string& matchList);

    // Replace the list of matches.
    void SetMatchList(const std::string& matchList);

    // Return \c true iff \p s matches the most recently set match list.
    bool Match(const char* s) const;

private:
    struct _MatchString {
        _MatchString(const std::string&);

        std::string str;    // String to match.
        bool allow;         // New result if str matches.
        bool wildcard;      // str has a suffix wildcard.
    };
    std::vector<_MatchString> _matchStrings;
};

Tf_MallocTagStringMatchTable::_MatchString::_MatchString(const std::string& s) :
    str(s),
    allow(true),
    wildcard(false)
{
    if (!str.empty()) {
        if (str[str.size() - 1] == '*') {
            wildcard = true;
            str.resize(str.size() - 1);
        }
        if (!str.empty()) {
            if (str[0] == '-') {
                allow = false;
                str.erase(0, 1);
            }
            else if (str[0] == '+') {
                str.erase(0, 1);
            }
        }
    }
}

Tf_MallocTagStringMatchTable::Tf_MallocTagStringMatchTable()
{
    // Do nothing
}

Tf_MallocTagStringMatchTable::Tf_MallocTagStringMatchTable(
    const std::string& matchList)
{
    SetMatchList(matchList);
}

void
Tf_MallocTagStringMatchTable::SetMatchList(const std::string& matchList)
{
    _matchStrings.clear();
    std::vector<std::string> items = TfStringTokenize(matchList, ",\t\n");
    for (std::string const &item: items) {
        _matchStrings.push_back(_MatchString(TfStringTrim(item, " ")));
    }
}

bool
Tf_MallocTagStringMatchTable::Match(const char* s) const
{
    // The last match defines the overall result.  If the last match had
    // a '-' prefix then we don't match, otherwise we do.
    TF_REVERSE_FOR_ALL(i, _matchStrings) {
        if (i->wildcard) {
            // Check prefix match.
            const char* m = i->str.c_str();
            while (*m && *m == *s) {
                ++m, ++s;
            }
            if (*m != '\0') {
                continue;
            }
        }
        else {
            // Check exact match.
            if (i->str != s) {
                continue;
            }
        }

        // Matched.
        return i->allow;
    }

    // No match.
    return false;
}

////////////////////////////////////////////////////////////////////////
// Global data singleton (Tf_MallocGlobalData)

// This is a singleton.  Because access to this structure is gated by checks to
// TfMallocTag::IsInitialized(), we forego the usual TfSingleton pattern and
// just use a single static-scoped pointer (_mallocGlobalData) to point to the
// singleton instance.
//
// The member data in this class is guarded by a _mutex member variable.
// However, the way this works is a bit different from ordinary mutex-protected
// data.
//
// Since TfMallocTag intercepts all malloc/free routines, it is important for it
// to be as fast and thread-scalable as possible in order to provide good user
// experience when tagging is enabled.  To that end, the data structures in this
// class are mostly concurrent containers and atomics.  This way different
// threads can concurrently modify these without blocking each other.
//
// To support queries of the entire malloc tags state to generate reports
// (e.g. TfMallocTag::GetCallTree()) or to modify global malloc tags behavior
// (e.g. TfMallocTag::SetCapturedMallocStacksMatchList()) concurrently with
// other threads doing malloc/free, we must have a way to halt all other reading
// or mutation of the global state.  Only then can we do concurrency-unsafe
// operations with the concurrent containers, like iterate over them, and
// present a consistent result to callers.
//
// We do this by employing a readers-writer lock (in _mutex).  Ordinary
// operations that read or modify the concurrent data structures and atomics in
// thread-safe ways (such as during malloc/free handling, and tag push/pop) take
// a "read" (or "shared") lock on _mutex: they can proceed concurrently
// relatively freely.  Operations that read or modify the concurrent data
// structures and atomics in a thread-unsafe way (such as iterating the data for
// report generation, or modifying the stack capture match rules or debug match
// rules) take a "write" (or "exclusive") lock on _mutex blocking all other
// access until they complete.
struct Tf_MallocGlobalData
{
    using _ThreadData = TfMallocTag::_ThreadData;
        
    // Resolve `name` to its call site, creating the site if this is the first
    // time it has been seen.
    //
    // For an immortal name this is typically a single lock-free probe of
    // _immortalNameSites; see _ImmortalNameSiteTable for why keying on the
    // address is both sound and much cheaper than keying on the characters.
    // Everything else -- first sight of an immortal name, and every lookup of a
    // dynamically-built name -- goes through the content-keyed _callSiteTable.
    //
    // The read lock covers only that cold path, and only because _callSiteTable
    // is a tbb container that _SetTraceNames() and _SetDebugNames() iterate
    // under the write lock.  Those two iterations are the sole reason the lock
    // still exists.  _BuildCallTree() also takes the write lock, but only so a
    // report cannot race that iteration; it does not iterate the table itself.
    // `stats` may be null for callers with no thread to charge (Initialize()).
    Tf_MallocCallSite *
    _GetOrCreateCallSite(const char *name,
                         bool nameIsImmortal,
                         _AtomicThreadStats *stats = nullptr) {
        if (nameIsImmortal) {
            if (Tf_MallocCallSite *site = _immortalNameSites.Find(name)) {
                if (stats) { _RelaxedInc(stats->siteFastHits); }
                return site;
            }
        }

        if (stats) {
            _RelaxedInc(stats->siteSlowLookups);
        }
        
        Tf_MallocCallSite *site = nullptr;
        {
            TfBigRWMutex::ScopedLock lock(_taggingStateMutex, /*write=*/false);
            site = Tf_GetOrCreateCallSite(
                &_callSiteTable, name, nameIsImmortal);
        }

        // Promote immortal names so the next lookup takes the fast path.  Done
        // after releasing the lock: Insert() takes none, and the site is
        // already published in _callSiteTable, which is what makes a racing
        // Find() that returns null harmless.
        if (nameIsImmortal) {
            _immortalNameSites.Insert(name, site);
        }
        return site;
    }

    // Capture the current call stack and return a stable pointer to the
    // canonical dedup'd _StackTrace entry for it in the global table.
    _StackTrace const *_GetOrCreateStackTrace() {
        std::vector<uintptr_t> frames;
        _GetStackTrace(_IgnoreStackFramesCount, &frames);
        return _stackTraceTable.GetOrCreate(std::move(frames));
    }    
    
    size_t _GetTotalBytes();
    size_t _GetMaxTotalBytes();

    void _RegisterThread(_ThreadData *td);
    void _UnregisterThread(_ThreadData *td);

    // Per-thread data calls this to spill when it fills its buffer, passing a
    // lock on its own _mutex, which this function releases.
    void _SpillThreadBuffer(_ThreadData *td, TfSpinMutex::ScopedLock tdLock);

    // Return an allocated, empty _ThreadBuffer, recycled from _freeBuffers if
    // one is available and freshly allocated otherwise.  Takes
    // _threadBuffersMutex briefly; any allocation happens outside that lock.
    // The caller must not already hold _threadBuffersMutex.
    _ThreadBuffer _TakeFreeBuffer();

    // Pop a buffer off _freeBuffers, or return a default-constructed one if the
    // pool is empty.  Caller must hold _threadBuffersMutex.
    //
    // Unlike _TakeFreeBuffer(), the result is not guaranteed to be allocated --
    // it is the caller's job to call events.allocate() on it, and the point of
    // this member function is that the caller can do so after dropping
    // _threadBuffersMutex rather than allocating underneath it.
    _ThreadBuffer _PopFreeBufferLocked();

    // Return a drained buffer to _freeBuffers for reuse, or drop it if the pool
    // is already at capacity.  Takes _threadBuffersMutex briefly; if the buffer
    // is dropped its array is freed outside that lock.  The caller must not
    // already hold _threadBuffersMutex.
    void _PutFreeBuffer(_ThreadBuffer &&buf);

    // Called to generate a call tree report or to field queries.  Drains all
    // thread buffers simultaneously and consolidates them to the baseline.
    void _DemandConsolidate();

    // Take all thread locks, discard all in-flight buffers, discard the spill
    // queue, stop the consolidator thread, discard the baseline and reset
    // totals.  If restartConsolidator is true, restart the periodic-wake worker
    // once the reset is done (the session continues, as after Clear()); pass
    // false when tearing down (Shutdown()), so it stays stopped.
    void _ClearAll(bool restartConsolidator);

    // Start the periodic-wake consolidator if enabled and not already running,
    // so the worker stays alive for the whole initialized lifetime and its wake
    // timer keeps sampling high-water peaks even when nothing is spilling.
    // Called from TfMallocTag::Initialize and after a Clear().  A no-op when
    // the period is zero (periodic wake disabled), which preserves lazy start.
    void _EnsureConsolidatorRunning() {
        if (_GetConsolidatePeriodMs()) {
            _consolidator.EnsureRunning();
        }
    }

    // Called by TfMallocTag::Initialize on non-first-time initializations to
    // bring thread states up to the current epoch and reset their buffers.
    void _ReinitializeThreads();

    // Return a report of diagnostic performance information.
    std::string _GetPerfStats(bool showPerThread);

    // A diagnostic snapshot of the path node tree's shape and of the health of
    // the table it lives in, for _GetPerfStats().
    //
    // Everything here is reported as a histogram rather than a mean, because
    // every distribution involved is known to be very wide and it's easy to
    // draw a wrong conclusion from a mean taken over one of them.
    struct _TreeStats {
        uint64_t numNodes  = 0;
        uint64_t maxFanOut = 0;
        uint64_t maxDepth  = 0;
        // Bucketed by _CountBucket(), the same bucketing the chain-length
        // histograms use, so all three can be read against each other.
        uint64_t fanOutHistogram[NumCountBuckets] = {};

        // Sums over all nodes of fan-out and fan-out squared.  It describes the
        // shape of the tag tree -- how bushy the workload's tagging is.
        uint64_t fanOutSum   = 0;
        uint64_t fanOutSqSum = 0;

        // _PathNodeTable bucket occupancy, as distinct from the per-resolve
        // chain length in _ThreadStats.  Occupancy describes the table; chain
        // length describes the traffic over it.  They differ in the case that
        // matters: a few pathologically long chains that nothing ever looks up
        // cost nothing, while one long chain on a hot key costs a great deal,
        // and having both distinguishes those.
        uint64_t occupancyHistogram[NumCountBuckets] = {};
        uint64_t maxChainLength = 0;
    };

    // Summarize the shape of the path node tree and the health of the table it
    // lives in.  Takes no lock: nodes are immortal and the table is
    // insert-only, so the only imprecision is possibly missing a node created
    // during the scan.  That is fine here -- these are diagnostics.
    _TreeStats _GetTreeStats() const;

    void _BuildCallTree(TfMallocTag::CallTree* tree, bool skipRepeated);

    // Build the parent -> children map over every path node, by scanning
    // _PathNodeTable.
    Tf_PathNodeChildrenTable _BuildPathNodeChildrenTable() const;

    void _GetStackTrace(size_t skipFrames, std::vector<uintptr_t>* stack);

    void _SetTraceNames(const std::string& matchList);
    bool _MatchesTraceName(const std::string& name);

    void _BuildUniqueMallocStacks(TfMallocTag::CallTree* tree);

    void _SetDebugNames(const std::string& matchList);
    bool _MatchesDebugName(const std::string& name);

    // Background consolidator
    struct _Consolidator {

        // How a single _ConsolidateImpl() pass drains thread buffers.  All
        // three modes drain the spill queue and publish to the baseline; they
        // differ only in how they treat threads' current (unspilled) buffers
        // and whether they may purge free tombstones.
        //
        //   Spill:    Woken by a buffer spill.  Drains the spill queue only;
        //             does not touch threads' current buffers.  No purge.
        //   Periodic: Woken by the wake timer.  Walks all threads and
        //             try-lock/drains each current buffer that is non-empty,
        //             skipping any thread whose lock is held.  Incremental
        //             (locks taken one at a time), so no purge.
        //   Demand:   Stop-the-world.  Holds every thread lock at once, drains
        //             every current buffer in one atomic snapshot, then purges
        //             the tombstones (only valid because the drain is fully
        //             synchronous).
        enum class _DrainMode { Spill, Periodic, Demand };

        // Try to process spilled event buffers.  It is not guaranteed that
        // spilled buffers have been processed when this function returns, if
        // the mutex was held by another client or an intervening Stop() request
        // canceled.  Does not block/wait if the mutex is already locked by
        // another.  If the mutex is acquired, starts the worker thread if
        // necessary.  Return true if the mutex was acquired and the worker
        // thread was notified.
        bool TrySpillConsolidate();

        // Simultaneously drain all event buffers.  It is guaranteed that at
        // least one demand consolidation is performed before this function
        // returns.  Starts the worker thread if necessary.
        void DemandConsolidate();

        // Ensure that all in-flight DemandConsolidate() requests complete, then
        // stop the worker thread.  Note that concurrent spill or
        // demand-consolidate requests could have restarted the thread by the
        // time Stop() returns.
        void Stop();

        // Start the worker thread if it is not already running.  Unlike
        // TrySpillConsolidate/DemandConsolidate, this requests no work -- it
        // just ensures the worker is alive so its periodic wake timer stays
        // armed.  Idempotent.
        void EnsureRunning() {
            std::unique_lock<std::mutex> lock(_mutex);
            _EnsureRunning(lock);
        }

    private:
        // Caller must hold _mutex.
        void _EnsureRunning(std::unique_lock<std::mutex> &lock);

        // Fields requests to spill, demand-consolidate, and stop.
        void _ThreadTask();

        void _ConsolidateImpl(_DrainMode mode);
        
        std::mutex               _mutex;
        std::condition_variable  _cv;
        std::thread              _thread;
        
        bool                     _stopRequested    = false;
        bool                     _stopped          = true;
        bool                     _spilled          = false;
        size_t                   _demandsMade      = 0;
        size_t                   _demandsCompleted = 0;
    };
    
    // Guards _callSiteTable only.  _pathNodeTable takes no lock at all -- see
    // the note there.
    //
    // Read-locked by the cold content-keyed path of _GetOrCreateCallSite().
    // Write-locked by _SetTraceNames() and _SetDebugNames(), which iterate the
    // table, and by _BuildCallTree(), which does not iterate it but must not
    // race those that do.  Node creation is deliberately not excluded; see
    // _BuildCallTree().
    TfBigRWMutex _taggingStateMutex;

    // Protects _threadDataHead.  Taken by _RegisterThread, _UnregisterThread,
    // and the snapshot phase of _ConsolidateImpl.
    TfSpinMutex _threadListMutex;
    
    // A linked list of all the _ThreadData instances.  Protected by
    // _threadListMutex.
    _ThreadData *_threadDataHead = nullptr;

    // The root of the path node tree.
    Tf_MallocPathNode *_rootNode = nullptr;

    // Totals published-to by demand consolidations.
    std::atomic<int64_t> _totalBytes     {0};
    std::atomic<int64_t> _maxTotalBytes  {0};
    std::atomic<int64_t> _liveAllocCount {0};

    // Threads spill their _ThreadBuffer here when full.  Protected by
    // _threadBuffersMutex.
    std::vector<_ThreadBuffer> _threadBuffers;

    // Pool of empty, allocated buffers recycled from consolidation and handed
    // back out by _TakeFreeBuffer().  This exists to avoid of malloc/free
    // churn: without it, every spill would free one EventBufferCapacity-sized
    // array and allocate another.  Protected by _threadBuffersMutex.
    std::vector<_ThreadBuffer> _freeBuffers;

    TfSpinMutex _threadBuffersMutex;

    // The persistent baseline of live allocations, updated by each
    // consolidation.  Protected by _baselineMutex.
    using _BaselineTable = _PtrRobinMap<void const *, _BaselineEvent>;

    // Protects baseline state.
    TfSpinMutex     _baselineMutex;
    _BaselineTable  _baselineEvents;
    _TraceMap       _baselineTraceMap;
    _Consolidator   _consolidator; // worker thread, populates _baselineEvents
                                   // and _baselineTraceMap.

    // Address-keyed fast path in front of _callSiteTable, for tag names whose
    // characters are immortal.  Lock-free; see _ImmortalNameSiteTable.
    _ImmortalNameSiteTable       _immortalNameSites;
    Tf_MallocCallSiteTable       _callSiteTable;
    _StackTraceTable             _stackTraceTable;
    Tf_MallocTagStringMatchTable _debugMatchTable;
    Tf_MallocTagStringMatchTable _traceMatchTable;
    std::atomic<int>             _globalPauseCount {0};
                                             // Count of threads that
                                             // have _globalPauseDepth > 0.
    
    std::atomic<int64_t>         _epoch {0}; // Represents the current epoch of
                                             // data collection if tagging is
                                             // initialized or the next epoch if
                                             // tagging is not initialized.
                                             // Incremented every Shutdown().
    _GlobalStats                 _stats {0};
};


////////////////////////////////////////////////////////////////////////
// TLS utilities (_ThreadData, Tls, _TemporaryDisabler)

// Per-thread data for TfMallocTag.
struct TfMallocTag::_ThreadData {
    _ThreadData();
    ~_ThreadData();

    _ThreadData(const _ThreadData &) = delete;
    _ThreadData& operator=(const _ThreadData &) = delete;

    // Call once after construction.
    inline void Initialize() {
#if DEBUG_MALLOC_TAG
        _log.resize(EventBufferCapacity);
#endif
        // Bring our buffer into service.  Note that _taggingState is still
        // _TaggingDisabled at this point, so the allocation this may do is not
        // itself tracked and cannot recurse back into us.  This is also why
        // _taggingState is enabled only as the last step.
        _buffer = _mallocGlobalData->_TakeFreeBuffer();
        _taggingState = _TaggingEnabled;
    }
    
    inline bool TaggingEnabled() const {
        return _taggingState == _TaggingEnabled;
    }

    inline void Push(Tf_MallocPathNode *node) {
        _PushEntry({nullptr, node});
    }

    inline void PushNewChild(char const *name, bool nameIsImmortal=false);

    // Pop does not truncate the stack vector -- the entry above _stackTop
    // becomes a ghost, available for fast re-push detection.
    //
    // Deliberately unchecked: the caller must guarantee a matching push.  This
    // is the tag-scope exit path, so it must stay as close to a single
    // decrement as possible.  Both callers satisfy the precondition by
    // construction:
    //
    //   - TfMallocTag::Auto counts _nTags from the pushes _Begin() actually
    //     accepted, so it pops exactly what it pushed.  Note that Clear() and
    //     Shutdown() do not touch _nodeStack or _stackTop, so a session
    //     boundary in the middle of a tag scope cannot unbalance it either.
    //   - StackOverride pushes and pops exactly one entry.
    //
    // The manual TfMallocTag::Pop() API has no such guarantee and must go
    // through PopChecked() instead.
    inline void Pop() {
        if (!_nodeStack[_stackTop].node) {
            _RelaxedInc(_stats.emptyTags);
        }
        --_stackTop;
    }

    // Pop for the manual TfMallocTag::Push()/Pop() API, which -- unlike Auto --
    // can be called unbalanced.  Reachable ways to get here with an empty
    // stack: Push("") does not push (an empty name is ignored) but Pop() always
    // pops, and a Push()/Pop() pair straddling Initialize() is likewise
    // unbalanced since the Push() half was a no-op.  Report and do nothing
    // rather than run _stackTop negative, which would index _nodeStack out of
    // bounds here and then hand Alloc() a null path node.
    inline void PopChecked() {
        if (ARCH_UNLIKELY(_stackTop < 0)) {
            TF_CODING_ERROR("TfMallocTag::Pop() with no matching Push()");
            return;
        }
        Pop();
    }

    // Return _nodeStack.back() if not empty, else nullptr.
    inline Tf_MallocPathNode *GetStackTopNode() const {
        return _stackTop < 0 ? nullptr : _nodeStack[_stackTop].node;
    }

    // Commit all lazy pushes up to the stack top.
    Tf_MallocPathNode *EnsureCurrentPathNode(bool topNameIsImmortal=true);

    // Main entry for recording an allocation event.
    void Alloc(void const *addr, size_t size);

    // Main entry for recording a free event.
    void Free(void const *addr);

    inline void PauseLocal() {
        ++_localPauseDepth;
    }

    inline void UnpauseLocal() {
        --_localPauseDepth;
    }

    inline void PauseGlobal() {
        if (++_globalPauseDepth == 1) {
            _mallocGlobalData->_globalPauseCount.fetch_add(
                1, std::memory_order_relaxed);
        }
    }
    
    inline void UnpauseGlobal() {
        if (--_globalPauseDepth == 0) {
            _mallocGlobalData->_globalPauseCount.fetch_sub(
                1, std::memory_order_relaxed);
        }
    }

    // Clear the event buffer and the look-back window together -- the ring
    // mirrors the buffer tail, so the two must always be reset as a unit.
    // Emptying the tags is what keeps _FindCancellableEventForAddr() from
    // confirming a candidate against a buffer element that no longer exists.
    //
    // This does not allocate, so it cannot bring an unallocated buffer into
    // service; callers that have moved the buffer's array away must install a
    // replacement (see _SpillThreadBuffer).
    inline void ResetBuffer() {
        _buffer.Reset();
        std::fill(
            std::begin(_lookBackTags), std::end(_lookBackTags), _EmptyTag);
        _lookBackRingEnd = 0;
    }

private:

    inline _Event const *_GetRecent(unsigned backIndex) const {
        return &_buffer.events[_buffer.events.size()-1-backIndex];
    }

    // Drop the event at `backIndex` by moving the buffer's last event into its
    // place and popping, mirroring the same move in the ring.
    //
    // This keeps the window consistent with the buffer.  The window's address
    // set afterward is exactly the old set minus the canceled address: the back
    // slot's entry moves to the canceled slot and the back slot is nulled, so
    // nothing is duplicated.  The slot-to-backIndex mapping also stays
    // consistent -- every surviving element at old backIndex k sits at k-1
    // afterward, and since _lookBackRingEnd also steps back by one, the ring
    // yields k-1 for it too.  That holds uniformly for k above and below
    // `backIndex`, so no fixup or rescan is needed.
    //
    // Note the window does shrink to 15 covered entries when this happens: the
    // element that becomes 16th-from-back has no ring entry.  That costs a
    // missed cancellation opportunity at worst, never correctness.
    inline void _CancelRecent(unsigned backIndex) {
        // Relies on LookBackWindowSize dividing 2^32 so that the unsigned
        // wraparound at _lookBackRingEnd == 0 lands on the last slot.
        static_assert((1ull << 32) % LookBackWindowSize == 0);
        unsigned lookBackBackIdx = (_lookBackRingEnd-1) % LookBackWindowSize;
        _lookBackTags[(_lookBackRingEnd-1-backIndex) % LookBackWindowSize] =
            _lookBackTags[lookBackBackIdx];
        _lookBackTags[lookBackBackIdx] = _EmptyTag;
        _lookBackRingEnd = lookBackBackIdx;
        const size_t bufIndex = _buffer.events.size() - 1 - backIndex;
        _buffer.events[bufIndex] = _buffer.events.back();
        _buffer.events.pop_back();
    }

    // There is deliberately no _ReplaceRecent().  Alloc() used to overwrite a
    // matching window event in place rather than appending, in order to
    // maintain a one-event-per-address invariant over the window.  That
    // mechanism is gone, and the reason is worth reading before anyone
    // reintroduces it: overwriting a free event destroys the only record that
    // an address died, and if the alloc that overwrote it is later canceled,
    // nothing is left to kill the even earlier alloc of that same address.  See
    // "Invariant: Cancellation Must Not Orphan an Earlier Event" at the top of
    // this file.
    inline _Event *_AppendEvent(_Event &&ev, uint8_t tag) {
        _lookBackTags[_lookBackRingEnd++] = tag;
        _lookBackRingEnd %= LookBackWindowSize;
        return &_buffer.events.emplace_back(std::move(ev));
    }

    inline _Event _MakeAllocEvent(void const *addr, size_t freeCountIdx,
                                  size_t size, Tf_MallocPathNode *node) const {
        const int64_t gfl = _globalFreeCount[freeCountIdx].value.load(
            std::memory_order_relaxed);
        return { size, _PathNodeAndFlags(node), gfl, addr };
    }

    inline _Event _MakeFreeEvent(void const *addr, size_t freeCountIdx) {
        const int64_t gfl = ++_globalFreeCount[freeCountIdx].value;
        return { /*size=*/0, /*nodeAndFlags=*/{}, gfl, addr };
    }

    // Return a backward-counting index for a window event that this free can
    // cancel: an alloc event for `addr` whose recorded free count still stands.
    // The index is zero-based from the last element -- 0 is the last element, 1
    // the second last, and so on -- and -1 means there is none.
    //
    // Eligibility is folded into the search rather than applied to its result,
    // and that is critical in both directions.
    //
    // The window may hold several events for one address.  Slot order says
    // nothing about which is newest.  The ring wraps, and _CancelRecent() swaps
    // the buffer's last event into the vacated slot, so buffer position does
    // not track temporal order either.
    //
    // At most one event can qualify, so returning the first that does is
    // unambiguous rather than a guess.  Suppose two allocs for one address both
    // carry the still-current count.  The address must have been freed between
    // them for the allocator to hand it out twice, and that free either bumped
    // the count -- leaving the later alloc with a higher one, a contradiction
    // -- or was itself canceled, which removes the earlier alloc from the
    // buffer, so it is not there to be found.
    //
    // The one that qualifies is the newest event for that address in the whole
    // buffer, not merely in the window, which is what makes cancelling it safe.
    // The free count only ever increases, so any event recorded later carries a
    // count at least as high, and no higher than the current one -- hence
    // exactly equal, hence also qualifying, which the previous paragraph rules
    // out.  A later free is excluded too, since a free stores its
    // post-increment value and so would also exceed the current count.
    //
    // Why tags rather than addresses: a one-byte hash per slot puts the whole
    // window in two 64-bit words, so it can be compared eight slots at a time
    // with ordinary integer arithmetic -- no intrinsics and no per-platform
    // code.
    //
    // A tag match is only a candidate and must be confirmed.  Tags collide at
    // about 1/256 per slot, so a few percent of searches turn up a hit that
    // fails confirmation, and confirmation reads a buffer event the caller is
    // about to touch anyway.  The address, the alloc bit and the free count all
    // live in the same event, so testing all three costs no extra memory
    // traffic.
    inline int
    _FindCancellableEventForAddr(void const *addr, uint8_t tag,
                                 size_t freeCountIdx) const {
        // Find which bytes in each 8-byte `word` of _lookBackTags equal `tag`,
        // using only ordinary integer arithmetic SWAR-style (no intrinsics and
        // no per-platform code).
        //
        // Broadcasting `tag` to every byte and xoring with `word` turns the
        // question into a zero-byte search: a byte of `x` is zero if it matched
        // `tag` and nonzero otherwise.  The difficulty is that a 64-bit add or
        // subtract carries across byte boundaries.  So the expression below is
        // arranged so no carry can leave a byte.  Taking a single byte `v` of
        // `x`, with the columns in the order the steps apply:
        //
        //       v    v & 0x7f    + 0x7f     | v      ~     & 0x80
        // ----------------------------------------------------------------
        //     0x00     0x00       0x7f      0x7f    0x80    0x80  <- match
        //     0x01     0x01       0x80      0x81    0x7e    0x00
        //     0x80     0x00       0x7f      0xff    0x00    0x00
        //     0xff     0x7f       0xfe      0xff    0x00    0x00
        //
        // Masking to seven bits bounds each byte's sum at 0x7f + 0x7f = 0xfe,
        // so the add can't carry, and the sum's high bit comes out clear only
        // when the low seven bits of `v` were all zero.  The `| v` restores the
        // bit the mask discarded, so the high bit is clear only when `v` was
        // zero in all eight bits.  Complementing and masking with Highs keeps
        // just those per-byte high bits, yielding 0x80 in each matching byte
        // and 0x00 in every other.
        static constexpr uint64_t Ones  = 0x0101010101010101ull;
        static constexpr uint64_t Lows  = 0x7f7f7f7f7f7f7f7full;
        static constexpr uint64_t Highs = 0x8080808080808080ull;

        const uint64_t bcast = uint64_t(tag) * Ones;

        uint64_t match[NumLookBackWords];
        uint64_t any = 0;
        for (unsigned w = 0; w != NumLookBackWords; ++w) {
            uint64_t word;
            memcpy(&word, &_lookBackTags[w * sizeof(uint64_t)], sizeof(word));
            const uint64_t x = word ^ bcast;
            any |= (match[w] = ~(((x & Lows) + Lows) | x) & Highs);
        }
        if (!any) {
            return -1;
        }

        // Confirm the candidates.
        //
        // _GetRecent() below is unchecked, so it is worth stating why it cannot
        // run off the buffer.  The occupied slots always hold exactly the
        // backward indices 0..k-1, contiguously, where k is the number of
        // occupied slots and k <= the buffer's size.  Appending adds index 0
        // and shifts the rest up; _CancelRecent() removes one and shifts the
        // rest down, decrementing the buffer size in step; ResetBuffer()
        // empties both together.  So every occupied slot names a live element.
        // A candidate can only arise from an occupied slot, because empty slots
        // hold _EmptyTag and _AddrTagFromHash() never returns it -- which is
        // the reason that value is reserved.
        //
        // Regarding reading the free count here: a racing bump to this counter
        // can only come from a different hash-equivalent address, not from
        // `addr` itself, because this thread has not yet actually called the
        // underlying free(addr) to release the memory (see the malloc/free
        // interception hooks recording-order invariant). A race therefore
        // produces only a missed cancellation, never a wrong one.
        const int64_t curFreeCount = _globalFreeCount[freeCountIdx].value.load(
            std::memory_order_relaxed);
        for (unsigned w = 0; w != NumLookBackWords; ++w) {
            for (uint64_t m = match[w]; m; m &= m - 1) {
                // Lane l of word w is slot 8*w + l.  Little-endian byte order
                // is assumed, so the lowest set bit of the mask is the
                // lowest-numbered matching lane.
                const unsigned slot = w * unsigned(sizeof(uint64_t)) +
                    (unsigned(ArchCountTrailingZeros(m)) / 8u);
                // Zero-based backward index: the last-written slot is
                // _lookBackRingEnd-1, so slot's distance from it is
                // (_lookBackRingEnd-1-slot) mod window.  No slot needs to be
                // rejected here -- empty slots hold _EmptyTag and never match,
                // so every candidate names a live element.
                const unsigned backIndex =
                    (_lookBackRingEnd - 1 - slot) % LookBackWindowSize;
                _Event const *ev = _GetRecent(backIndex);
                if (ev->addr == addr && ev->IsAlloc() &&
                    _HaveAllFreesSince(*ev, curFreeCount)) {
                    return int(backIndex);
                }
            }
        }
        return -1;
    }

    // Return true if this thread has seen all the free events that transpired
    // between `event` and now.  That is, no free of a hash-equivalent address
    // has been issued by anyone since `event` was recorded.
    //
    // The test is simply that the global free count for the bucket has not
    // moved.  Every free that can release an address bumps that counter --
    // including frees TfMallocTag does not record, taken while tracking is
    // disabled for its own bookkeeping -- so an unchanged counter proves no
    // such free happened, and a free arriving now for an address this thread
    // allocated must therefore be that allocation's own.
    //
    // `curFreeCount` is the bucket's current count, which
    // _FindCancellableEventForAddr() loads once and tests against every
    // candidate.
    static bool
    _HaveAllFreesSince(_Event const &event, int64_t curFreeCount) {
        return event.globalFreeCount == curFreeCount;
    }

    // Checking for pause runs on every allocation, so it is worth being exact
    // about what has to be tested.
    //
    // _globalPauseDepth is deliberately not tested here.  It counts this
    // thread's own nesting of all-threads pauses, and PauseGlobal() bumps the
    // shared _globalPauseCount on this thread's 0 -> 1 transition while
    // UnpauseGlobal() drops it on the 1 -> 0.  So a non-zero _globalPauseDepth
    // always implies a non-zero _globalPauseCount, and the second test below
    // already covers it.
    inline bool _IsPaused() const {
        return
            _localPauseDepth > 0 ||
            _mallocGlobalData->_globalPauseCount.load(
                std::memory_order_relaxed) > 0;
    }

    ////////////////////////////////////////////////////////////////////////
    // Tag stack.

    struct _StackEntry {
        // The site-name.  When pushing an immortal name, we push only this
        // field and only fetch the `node` lazily on the first alloc, since we
        // often push nodes and then never alloc before popping.
        char const        *name = nullptr;

        // This entry's node.  May be null.
        Tf_MallocPathNode *node = nullptr;
    };

    // Write 'entry' at _stackTop+1 and advance _stackTop.  Any entries beyond
    // the new top are ghosts -- they may be cheaply reclaimed by a future push
    // at the same depth if they match.
    void _PushEntry(_StackEntry entry) {
        _RelaxedInc(_stats.totalTags);
        const int newTop = _stackTop + 1;
        if (newTop < (int)_nodeStack.size()) {
            _nodeStack[newTop] = entry;
        }
        else {
            _nodeStack.push_back(entry);
            _stats.maxStackDepth.store(newTop + 1, std::memory_order_relaxed);
        }
        _stackTop = newTop;
    }

    // Full fallback: look up or create the call site and child path node via
    // the global tables.  In the common case -- an immortal-name tag whose site
    // has been seen before -- this takes no lock at all: an address-keyed probe
    // for the site, then a hash probe of _PathNodeTable for the node.
    Tf_MallocPathNode *
    _GlobalLookupChild(Tf_MallocPathNode *parent,
                       char const *name,
                       bool nameIsImmortal) {
        DBG_AXIOM(name && name[0]); // Caller-checked.

        Tf_MallocCallSite *site = _mallocGlobalData->
            _GetOrCreateCallSite(name, nameIsImmortal, &_stats);

        // Count this probe's chain length separately so we can bucket it; the
        // running total is derived rather than accumulated in place.
        uint64_t steps = 0;
        Tf_MallocPathNode *node =
            _pathNodeTable.GetOrCreate(parent, site, &steps);
        _RelaxedAdd(_stats.chainStepsTotal, steps);
        const size_t bucket = _CountBucket(steps);
        _RelaxedInc(_stats.chainResolves[bucket]);
        _RelaxedAdd(_stats.chainSteps[bucket], steps);
        return node;
    }

#if DEBUG_MALLOC_TAG
#define _DebugLog(...) _DebugLogImpl(__VA_ARGS__)
    void _DebugLogImpl(_Event const &, char const *);
    void _DebugPrintLog() const;
    void _DebugCheckWindow() const;
#else
#define _DebugLog(...)
    constexpr void _DebugPrintLog() const {}
    constexpr void _DebugCheckWindow() const {}
#endif
    
    ////////////////////////////////////////////////////////////////////////
    friend struct Tf_MallocGlobalData;
    friend struct _TemporaryDisabler;

#if DEBUG_MALLOC_TAG
    std::vector<std::pair<_Event, char const *>> _log;
    size_t _logEnd = 0;
#endif // DEBUG_MALLOC_TAG
    
    TfSpinMutex        _mutex;  // protects _buffer, which the global data will
                                // sometimes consume.
    _AtomicThreadStats _stats;
    _ThreadBuffer      _buffer;

    // Stack of active path nodes.  Entries above _stackTop are ghosts -- they
    // are left in place so that a matching re-push can be detected without any
    // cache or table access.
    std::vector<_StackEntry> _nodeStack;
    int                      _stackTop     = -1;
    _TaggingState            _taggingState = _TaggingDisabled;

    // A ring-buffer of one-byte tags, one per event in the buffer tail, used to
    // find a recent matching event quickly for possible cancellation.  Slot i
    // mirrors the buffer element at zero-based backward index (_lookBackRingEnd
    // - 1 - i) mod LookBackWindowSize; _EmptyTag means the slot mirrors
    // nothing.  See _FindCancellableEventForAddr() for why this holds tags
    // rather than the addresses themselves, and note the whole window is 16
    // bytes -- it shares a cache line with the fields around it.
    static constexpr uint8_t  _EmptyTag                         = 0;
    alignas(uint64_t) uint8_t _lookBackTags[LookBackWindowSize] = {};
    unsigned                  _lookBackRingEnd                  = 0;

    int _localPauseDepth  = 0;  // PauseControl this-thread pause depth.
    int _globalPauseDepth = 0;  // PauseControl thread-local count of
                                // all-threads pause depth.

    int64_t _epoch = 0; // The last known _mallocGlobalData _epoch.  This is
                        // used to discard any buffers spilled to the global
                        // data across Shutdown and re-Initialize events.

    _ThreadData *_next = nullptr; // The _mallocGlobalData strings these
                                  // together so it can iterate them.
};

TfMallocTag::_ThreadData::_ThreadData()
{
    TF_AXIOM(_mallocGlobalData);
    _mallocGlobalData->_RegisterThread(this);
}

TfMallocTag::_ThreadData::~_ThreadData()
{
    TF_AXIOM(_mallocGlobalData);
    _taggingState = _TaggingDisabled;
    _mallocGlobalData->_UnregisterThread(this);
}

void
TfMallocTag::_ThreadData::Alloc(void const *addr, size_t size)
{
    // Prefetch the global free count line while we get ready.  The look-back
    // window needs no prefetch: it is 16 bytes sharing a line with the fields
    // around it, which this thread touches on every event.
    const size_t addrHash = _HashAddr(addr);
    const size_t freeCountIdx = _FreeCountIdxFromHash(addrHash);
    const uint8_t addrTag = _AddrTagFromHash(addrHash);
    ArchPrefetchRead(&_globalFreeCount[freeCountIdx]);

    TfSpinMutex::ScopedLock lock(_mutex);
    TF_DEV_AXIOM(_taggingState == _TaggingDisabled);

    _RelaxedInc(_stats.allocsTotal);

    // If collection is paused, force size to 0 so this allocation doesn't count
    // toward memory usage.
    if (_IsPaused()) {
        size = 0;
    }

    // Capture the current path node and its flags.  The read is deliberately
    // relaxed and unsynchronized -- see the comment on
    // Tf_MallocCallSite::_flags.
    Tf_MallocPathNode * const node = EnsureCurrentPathNode();
    const unsigned flags =
        node->_callSite->_flags.load(std::memory_order_relaxed);

    // Just append.  Alloc() consults the look-back window for nothing at all,
    // which is the cheapest it can possibly be -- and it is also the correct
    // thing, not merely the fast thing.
    //
    // Do not add a window search here that overwrites a matching event instead
    // of appending.  Overwriting a stale alloc would be harmless, but
    // overwriting a free destroys the only record that the address died -- and
    // if this alloc is then canceled, which is the common case, nothing remains
    // to retire an earlier alloc of the same address, which can stay
    // incorrectly billed to its tag indefinitely.  See "Invariant: Cancellation
    // Must Not Orphan an Earlier Event" at the top of this file, and
    // TestCancelDoesNotOrphanEarlierAlloc() in the test.
    _Event *bufEvent = _AppendEvent(
        _MakeAllocEvent(addr, freeCountIdx, size, node), addrTag);
    _DebugLog(*bufEvent, "append");
    _DebugCheckWindow();

    // Take a stack trace if the site's flag is set.
    if (ARCH_UNLIKELY(flags & Tf_MallocCallSite::_TraceFlag)) {
        _buffer.traceMap[addr] = _mallocGlobalData->_GetOrCreateStackTrace();
        bufEvent->_nodeAndFlags.SetBits(true);
    }

    // Spill the buffer if full.  Bump the stat first: _SpillThreadBuffer()
    // consumes our lock and releases it, and _stats is read by _GetPerfStats()
    // under this same lock.
    if (ARCH_UNLIKELY(_buffer.events.full())) {
        _RelaxedInc(_stats.spillsToGlobal);
        _mallocGlobalData->_SpillThreadBuffer(this, std::move(lock));
    }

    // NOTE: lock may no longer be acquired here, if we called
    // _SpillThreadBuffer above.
    
    // Invoke the debug hook if this call site has debugging enabled.  Note: we
    // only hook the alloc side in this implementation; free-side hooking would
    // require a global address->node table to recover the path node at free
    // time.
    if (ARCH_UNLIKELY(flags & Tf_MallocCallSite::_DebugFlag)) {
        Tf_MallocTagDebugHook(addr, size);
    }
}

void
TfMallocTag::_ThreadData::Free(void const *addr)
{
    // Prefetch the global free count line while we look for recent matching
    // events.  The look-back window needs no prefetch: it is 16 bytes sharing a
    // line with the fields around it, which this thread touches on every event.
    const size_t addrHash = _HashAddr(addr);
    const size_t freeCountIdx = _FreeCountIdxFromHash(addrHash);
    const uint8_t addrTag = _AddrTagFromHash(addrHash);
    ArchPrefetchWrite(&_globalFreeCount[freeCountIdx]);

    TfSpinMutex::ScopedLock lock(_mutex);
    TF_DEV_AXIOM(_taggingState == _TaggingDisabled);

    _RelaxedInc(_stats.freesTotal);

    // If the window holds an alloc this free definitively matches, drop the
    // pair and record nothing at all: move the buffer's last event into the
    // vacated slot and pop.  This is where the great majority of all events
    // die in typical workloads.
    if (const int bufBackIndex =
            _FindCancellableEventForAddr(addr, addrTag, freeCountIdx);
        bufBackIndex >= 0) {

        _CancelRecent(unsigned(bufBackIndex));
        _RelaxedAdd(_stats.lookBackDiscards, 2); // one each for alloc & free.

        _DebugLog({
                0, {}, _globalFreeCount[freeCountIdx].value.load(), addr
            }, "cancel");
        _DebugCheckWindow();
    }
    else {
        // Otherwise record the free.  Append even when the window already holds
        // another event for this address, and do not be tempted to overwrite it
        // on the reasoning that an older event for an address is obsolete once
        // a newer one exists.  That holds for the baseline, which keys by
        // address and keeps whichever event is newer, but not for the buffer,
        // where an event may still be needed to retire something a cancellation
        // would otherwise orphan.
        _AppendEvent(_MakeFreeEvent(addr, freeCountIdx), addrTag);
        _DebugLog(_buffer.events.back(), "append");
        _DebugCheckWindow();
        // Bump the stat before spilling -- see the note in Alloc().
        if (ARCH_UNLIKELY(_buffer.events.full())) {
            _RelaxedInc(_stats.spillsToGlobal);
            _mallocGlobalData->_SpillThreadBuffer(this, std::move(lock));
        }
    }
}

inline Tf_MallocPathNode *
TfMallocTag::_ThreadData::EnsureCurrentPathNode(bool topNameIsImmortal)
{
    // The tail of the _nodeStack may have lazily-pushed entries that need to be
    // resolved to true node pointers via the global path node table.

    // No entries, just root.
    if (_stackTop < 0) {
        return _mallocGlobalData->_rootNode;
    }
    // Top already has a node.
    if (_nodeStack[_stackTop].node) {
        return _nodeStack[_stackTop].node;
    }

    // Search backward to find the first entry with a node.
    int idx = _stackTop;
    for (; idx >= 0; --idx) {
        if (_nodeStack[idx].node) {
            break;
        }
    }

    ++idx;

    _StackEntry *cur         = &_nodeStack[idx];
    _StackEntry *top         = &_nodeStack[_stackTop];
    _StackEntry *notImmortal = topNameIsImmortal ? nullptr : top;

    // Handle 0th element specially.
    if (idx == 0) {
        _RelaxedInc(_stats.pathNodeCacheMisses);
        cur->node = _GlobalLookupChild(_mallocGlobalData->_rootNode,
                                       cur->name,
                                       cur != notImmortal);
        cur->name = nullptr;
        ++cur;
    }
    // Do the rest up to the top.
    for (_StackEntry *par = cur - 1; cur <= top; ++par, ++cur) {
        // Complete the entry for cur with par.
        _RelaxedInc(_stats.pathNodeCacheMisses);
        cur->node = _GlobalLookupChild(par->node, cur->name,
                                       cur != notImmortal);
        cur->name = nullptr;
    }

    TF_DEV_AXIOM(_nodeStack[_stackTop].node);
    TF_DEV_AXIOM(!_nodeStack[_stackTop].name);

    return _nodeStack[_stackTop].node;
}

inline void
TfMallocTag::_ThreadData::PushNewChild(char const *name, bool nameIsImmortal)
{
    Tf_MallocPathNode *curNode =
        _stackTop < 0 ? nullptr : _nodeStack[_stackTop].node;

    // Fast-path / lazy push for immortal names.
    if (nameIsImmortal) {
        if (!curNode) {
            // Lazy push -- resolved if needed in Alloc().
            _PushEntry({ name });
            return;
        }

        // Ghost check.  This is a push shortcut: the entry just above the top
        // is whatever was popped there last, so an immediate re-push of the
        // same tag at the same depth is a pointer compare and a decrement, with
        // no table access at all.
        //
        // The compare is on the address, so it is a conservative test: the site
        // adopted whichever immortal address first created it, and a re-push of
        // the same characters at a different immortal address misses here
        // forever.  Two translation units' literals for one string already
        // behave this way, and so do a literal and a TfEternalString naming the
        // same content.  The cost of a miss is one lazy push and two table
        // probes, and a content compare here is the exact work the
        // address-keyed fast path exists to avoid.
        const int ghostIdx = _stackTop + 1;
        if (ghostIdx < (int)_nodeStack.size()) {
            Tf_MallocPathNode *ghostNode = _nodeStack[ghostIdx].node;
            if (ghostNode &&
                ghostNode->_GetParent() == curNode &&
                ghostNode->_callSite->_name == name) {
                // Exact re-push match.
                ++_stackTop;
                _RelaxedInc(_stats.pathNodeGhostHits);
                return;
            }
        }
        // Just lazy-push and return.
        _PushEntry({ name });
        return;
    }

    // Otherwise, the given site name is not immortal, so we'll push on a "fake"
    // lazy entry, but then immediately pull up all the parent nodes here and
    // create the child while we have the string available.
    // EnsureCurrentPathNode() nulls out the stack entrys' names.  We do have to
    // pass `topNameIsImmortal=false` so that in case it's a new node we don't
    // accidentally adopt the pointer as the canonical name.
    _PushEntry({ name });
    EnsureCurrentPathNode(/*topNameIsImmortal=*/false);
}

#if DEBUG_MALLOC_TAG

void
TfMallocTag::_ThreadData::_DebugLogImpl(_Event const &e, char const *note)
{
    _log[_logEnd] = { e, note };
    _logEnd = (_logEnd + 1) % _log.size();
}

void
TfMallocTag::_ThreadData::_DebugPrintLog() const
{
    size_t i = _logEnd;
    bool logged = false;
    do {
        if (_log[i].first.addr) {
            std::cout << _log[i].first << " - " << _log[i].second << '\n';
            logged = true;
        }
        i = (i + 1) % _log.size();
    } while (i != _logEnd);
    if (logged) {
        std::cout << std::flush;
    }
}

// Check the property that makes _FindCancellableEventForAddr()'s unchecked
// _GetRecent() safe: the occupied slots carry exactly the backward indices
// 0..k-1, contiguously, where k is the number of occupied slots, and k never
// exceeds the buffer's size.  Every occupied slot therefore names a live buffer
// element.
void
TfMallocTag::_ThreadData::_DebugCheckWindow() const
{
    unsigned occupied = 0;
    bool seen[LookBackWindowSize] = {};
    for (unsigned i = 0; i != LookBackWindowSize; ++i) {
        if (_lookBackTags[i] == _EmptyTag) {
            continue;
        }
        ++occupied;
        // Zero-based backward index, matching the search path: the last-written
        // slot is _lookBackRingEnd-1, so the oldest slot maps to
        // LookBackWindowSize-1 with no wrap onto the not-found sentinel.
        unsigned backIndex = (_lookBackRingEnd - 1 - i) % LookBackWindowSize;
        if (backIndex >= _buffer.events.size()) {
            printf("Look-back slot %u has backIndex %u past buffer size %zu\n",
                   i, backIndex, _buffer.events.size());
            goto fail;
        }
        if (seen[backIndex]) {
            printf("Look-back backIndex %u occupied twice\n", backIndex);
            goto fail;
        }
        seen[backIndex] = true;
    }
    for (unsigned b = 0; b != occupied; ++b) {
        if (!seen[b]) {
            printf("Look-back backIndex %u missing; %u slots occupied\n",
                   b, occupied);
            goto fail;
        }
    }
    return;

fail:
    printf("Event Log:\n");
    _DebugPrintLog();
    printf("Event Buffer:\n");
    std::cout << _buffer.events;
    DBG_AXIOM(false && "look-back window index invariant");
}

#endif // DEBUG_MALLOC_TAG

class TfMallocTag::Tls {
public:
    static
    TfMallocTag::_ThreadData &Find() {
        // This thread_local must be placed in static TLS to prevent reentry.
        // Starting in glibc 2.25, dynamic TLS allocation uses malloc.  Making
        // this allocation after malloc tags have been initialized results in
        // infinite recursion.
        static thread_local _ThreadData* data = nullptr;
        if (ARCH_LIKELY(data)) {
            return *data;
        }
        // This weirdness is so we don't reenter malloc tags and don't call the
        // destructor of _ThreadData when the thread is exiting.  We can't do
        // the latter because we don't know in what order objects will be
        // destroyed and objects destroyed after the _ThreadData may do heap
        // (de)allocation, which requires the _ThreadData object.  We leak the
        // heap allocated blocks in the _ThreadData.
        void *dataBuffer = _mallocHook.IsInitialized()
            ? _mallocHook.Memalign(alignof(_ThreadData), sizeof(_ThreadData))
            : ArchAlignedAlloc(alignof(_ThreadData), sizeof(_ThreadData));
        data = new (dataBuffer) _ThreadData();
        // This has to be after `data` is assigned to, to prevent recursion.
        data->Initialize();
        return *data;
    }
};

// Helper to temporarily disable tagging operations, so that TfMallocTag
// facilities can use the heap for bookkeeping without recursively invoking
// itself.  Note that instances of these classes do not nest!  The reason is
// that we expect disabling to be done in very specific, carefully considered
// places, not willy-nilly, and not within any recursive contexts.
struct _TemporaryDisabler {
public:
    explicit _TemporaryDisabler(TfMallocTag::_ThreadData *threadData = nullptr)
        : _tls(threadData ? *threadData : TfMallocTag::Tls::Find()) {
        TF_DEV_AXIOM(_tls._taggingState == _TaggingEnabled);
        _tls._taggingState = _TaggingDisabled;
    }
        
    ~_TemporaryDisabler() {
        _tls._taggingState = _TaggingEnabled;
    }

private:
    TfMallocTag::_ThreadData &_tls;
};


////////////////////////////////////////////////////////////////////////
// Tf_MallocGlobalData member function bodies

// Caller must hold _mutex.
void
Tf_MallocGlobalData::_Consolidator
::_EnsureRunning(std::unique_lock<std::mutex> &lock) {
    while (_stopped || _stopRequested) {
        if (_stopped) {
            // Restart the thread.
            if (_thread.joinable()) {
                _thread.join();
            }
            _stopped = _stopRequested = false;
            _thread = std::thread(&_Consolidator::_ThreadTask, this);
            break;
        }
        if (_stopRequested) {
            // Wait for completion or another restarter & retry.
            _cv.wait(lock, [&]() {
                return _stopped || !_stopRequested;
            });
        }
    }
}

void
Tf_MallocGlobalData::_Consolidator::_ThreadTask()
{
    // Permanently disable tagging for the consolidator thread.  A temporary
    // disabler doesn't work here, because it is destroyed on join() before the
    // std::thread cleans up its state, and we can deadlock trying to record the
    // frees that happen in thread teardown that we don't care about.
    TfMallocTag::Tls::Find()._taggingState = _TaggingDisabled;
    ArchSetThisThreadName("TfMallocTag");
    ArchSetThisThreadPriority(ArchThreadPriorityLow);

    // Take the lock and wait for instructions.  With a nonzero period we also
    // wake on a timer, so a thread that fills less than one buffer and then
    // stalls still gets drained (see _DrainMode::Periodic).  A zero period
    // disables that and reverts to an indefinite wait.
    //
    // The periodic drain is driven off a wall-clock deadline (nextPeriodic),
    // not off "did the wait time out".  Under a steady spill stream the wait
    // never times out -- pred is almost always already true -- so a timeout
    // trigger would let busy threads starve coverage of stalled ones.  The
    // deadline fires on cadence regardless.
    const auto period = std::chrono::milliseconds(_GetConsolidatePeriodMs());
    auto nextPeriodic = std::chrono::steady_clock::now() + period;
    std::unique_lock<std::mutex> lock(_mutex);
    while (true) {
        auto pred = [&]() {
            return _stopRequested || _spilled ||
                _demandsMade != _demandsCompleted;
        };
        // Wake on a spill/demand/stop, or (with a nonzero period) at the next
        // periodic deadline.  wait_until re-checks pred on spurious wakeups.
        if (period.count()) {
            _cv.wait_until(lock, nextPeriodic, pred);
        }
        else {
            _cv.wait(lock, pred);
        }

        // Phase 1: drain spills, and on cadence a periodic all-thread pass.  A
        // periodic pass also scoops the spill queue, so choosing it over a
        // spill pass never neglects pending spills.  Yields to demands and
        // stop.  pred is true whenever _stopRequested is set, and the loop
        // exits on it, so no periodic runs during shutdown.
        while (!_stopRequested && _demandsMade == _demandsCompleted) {
            const bool periodicDue = period.count() &&
                std::chrono::steady_clock::now() >= nextPeriodic;
            if (!_spilled && !periodicDue) {
                break;  // nothing to drain right now
            }
            _spilled = false;
            lock.unlock();
            _ConsolidateImpl(periodicDue
                             ? _DrainMode::Periodic
                             : _DrainMode::Spill);
            lock.lock();
            if (periodicDue) {
                nextPeriodic = std::chrono::steady_clock::now() + period;
            }
        }

        // Phase 2: drain demands (implicitly covers any remaining spills).
        // Outer loop ensures we don't exit with demands that arrived during
        // notify.
        while (_demandsMade != _demandsCompleted) {
            size_t numCompleted = 0;
            while (_demandsMade != _demandsCompleted) {
                // Clear spill before unlock -- demand work covers it.  If a new
                // spill arrives during _DemandConsolidate() it will reset the
                // flag.
                _spilled = false;
                size_t demandedBefore = _demandsMade;
                lock.unlock();
                _ConsolidateImpl(_DrainMode::Demand);
                lock.lock();
                numCompleted += demandedBefore - _demandsCompleted;
                _demandsCompleted = demandedBefore;
            }
            lock.unlock();
            if (numCompleted == 1) {
                _cv.notify_one();
            }
            else {
                _cv.notify_all();
            }
            lock.lock();
            // New demands may have arrived while unlocked for notify -- outer
            // loop re-checks before proceeding to stop.
        }
        
        // Phase 3: stop if requested.  All demands are processed, but there may
        // be pending spills.  That's okay, they'll either be handled on a
        // restart (or deleted due to shutdown/clear).
        if (_stopRequested) {
            _stopped = true;
            lock.unlock();
            _cv.notify_all();
            break;
        }
    }
}

void
Tf_MallocGlobalData::_Consolidator::_ConsolidateImpl(_DrainMode mode)
{
    // Only invoked by the _consolidator thread.
    Tf_MallocGlobalData *gd = _mallocGlobalData;

    // Note the epoch we are collecting for.  If it advances before we reach the
    // baseline, a Clear() or Shutdown() intervened and everything we drained
    // belongs to a session that no longer exists, so we drop it.
    const int64_t startEpoch = gd->_epoch.load(std::memory_order_relaxed);

    std::vector<_ThreadBuffer> threadBuffers;

    // For a demand consolidation, take all the thread locks (stop-the-world) so
    // we can drain all thread buffers.
    //
    // The locks must be held simultaneously, not one at a time, and that is the
    // reason for the vector below.  The baseline concludes an allocation is
    // live because it has seen an alloc and no later free, which is valid only
    // if the events reaching it are complete for the addresses involved.
    // Drained sequentially, a thread already drained could free an address
    // whose alloc is captured from a thread drained later; that free would be
    // missed and the allocation would be reported live forever.  Holding every
    // lock at once makes the drain a single synchronous snapshot, which is also
    // the condition that lets the tombstone purge below work.
    //
    // A Periodic drain (below) deliberately does not do this: it takes the
    // thread locks one at a time, accepting the two-sided in-flight staleness
    // that retained tombstones + IsNewer already tolerate, and in exchange
    // never stalls a producer.  It therefore must not purge.
    TfSpinMutex::ScopedLock threadListLock;
    std::vector<TfSpinMutex::ScopedLock> threadLocks;
    if (mode == _DrainMode::Demand) {
        // Take the thread list lock first, then each thread lock.
        threadListLock.Acquire(gd->_threadListMutex);
        threadLocks.reserve(128);
        for (_ThreadData *td = gd->_threadDataHead; td; td = td->_next) {
            threadLocks.emplace_back(td->_mutex);
        }
    }

    // In every mode, drain spilled _threadBuffers.
    {
        TfSpinMutex::ScopedLock buffersLock(gd->_threadBuffersMutex);
        threadBuffers = std::move(gd->_threadBuffers);
        gd->_threadBuffers.clear();
    }

    // Drain the threads' current buffers.  Each thread we take a buffer from
    // gets a replacement in the same step: we still hold its lock, but it may
    // append the instant we release it, so it must never be left unallocated.
    //
    // _TakeFreeBuffer() will allocate if the recycle pool has run dry, which
    // means allocating with the world stopped.  That is confined to demand
    // consolidation, which is already documented as a heavyweight operation,
    // and in steady state the pool is warm.  Pre-staging replacements before
    // taking the thread locks would remove it if it ever shows up in a profile.
    if (mode == _DrainMode::Demand) {
        for (_ThreadData *td = gd->_threadDataHead; td; td = td->_next) {
            if (!td->_buffer.events.empty()) {
                threadBuffers.push_back(
                    std::exchange(td->_buffer, gd->_TakeFreeBuffer()));
                td->ResetBuffer();
                TF_DEV_AXIOM(td->_buffer.IsAllocated());
            }
        }
        // Now drop all thread locks and the list lock.
        threadLocks.clear();
        threadListLock.Release();
    }
    else if (mode == _DrainMode::Periodic) {
        // Opportunistically drain each thread's current buffer.  Hold the
        // thread list lock for the walk (so the _next traversal is safe against
        // an unregister), but take each thread's lock only if it is free right
        // now: a busy producer is skipped and picked up on a later wake rather
        // than stalled.  Lock order is _threadListMutex -> td->_mutex ->
        // _threadBuffersMutex (via _TakeFreeBuffer), the same order Demand and
        // _UnregisterThread use.
        //
        // All buffers collected here join the one threadBuffers vector, so the
        // co-live set is consolidated together and localMaxDelta sees it in a
        // single pass.
        //
        // Keep one allocated empty buffer staged ahead of the producer's lock,
        // so the only work done while holding td->_mutex is the buffer swap and
        // ResetBuffer -- never a _threadBuffersMutex acquire (contended by
        // every spilling thread) nor an allocation.  The spare is refilled
        // after the lock is released, as _SpillThreadBuffer() does with its
        // replacement.
        _ThreadBuffer spare = gd->_TakeFreeBuffer();
        threadListLock.Acquire(gd->_threadListMutex);
        for (_ThreadData *td = gd->_threadDataHead; td; td = td->_next) {
            TfSpinMutex::ScopedLock lock(td->_mutex, TfSpinMutex::deferAcquire);
            if (!lock.TryAcquire() || td->_buffer.events.empty()) {
                continue;
            }
            threadBuffers.push_back(
                std::exchange(td->_buffer, std::move(spare)));
            td->ResetBuffer();
            TF_DEV_AXIOM(td->_buffer.IsAllocated());
            lock.Release();
            // Refill the spare off the producer's lock, ready for the next
            // thread we drain.  We still hold _threadListMutex, which is outer
            // to _threadBuffersMutex, so the lock order is preserved.
            spare = gd->_TakeFreeBuffer();
        }
        threadListLock.Release();
        // Return the unused staged buffer to the recycle pool.
        gd->_PutFreeBuffer(std::move(spare));
    }

    // Now push buffer events to the big baseline table.
    //
    // We check the epoch under the baseline lock rather than up front: if we
    // hold the lock and still see startEpoch, then either _ClearAll() has not
    // bumped yet -- in which case it will clear the baseline after us and our
    // work is harmlessly discarded -- or it has, and we see the new value.
    //
    // On a mismatch we drop everything we drained.  We cannot tell which events
    // predate the boundary and which follow it, and over-discarding a few that
    // raced it is consistent with Clear()'s documented semantics.
    {
        TfSpinMutex::ScopedLock baselineLock(gd->_baselineMutex);
        if (gd->_epoch.load(std::memory_order_relaxed) == startEpoch) {

            int64_t localBytesDelta = 0;
            int64_t localAllocDelta = 0;
            // Peak the running byte delta reaches while consuming this batch,
            // used to drive the high-water below.  Starts at 0 so the peak is
            // never below the batch-entry total.
            int64_t localMaxDelta = 0;

            for (_ThreadBuffer const &buf: threadBuffers) {
                for (_Event const &event: buf.events) {
                    // Try to insert in the table.  If present, replace if event
                    // is newer.
                    auto [iter, inserted] = gd->_baselineEvents.insert(
                        { event.addr, _BaselineEvent { event } });
                    if (inserted) {
                        // New baseline entry: register trace if present.
                        if (ARCH_UNLIKELY(event.HasTrace())) {
                            auto traceIt = buf.traceMap.find(event.addr);
                            TF_DEV_AXIOM(traceIt != buf.traceMap.end());
                            gd->_baselineTraceMap[event.addr] = traceIt->second;
                        }
                        // Update incremental totals.
                        if (event.IsAlloc()) {
                            localBytesDelta += event.size;
                            localMaxDelta =
                                std::max(localMaxDelta, localBytesDelta);
                            ++localAllocDelta;
                        }
                        // Free events have zero size and zero alloc
                        // contribution.  They are retained as tombstones -- see
                        // the note on the purge below.
                    }
                    else {
#if DEBUG_MALLOC_TAG
                        if (iter->second.globalFreeCount ==
                            event.globalFreeCount) {
                            // Two events for the same address with the same
                            // free count cannot both be allocs or frees.
                            DBG_AXIOM(iter->second.IsAlloc() ^ event.IsAlloc());
                        }
#endif // DEBUG_MALLOC_TAG
                        if (IsNewer(event, iter->second)) {
                            // Evict stale trace for the old entry.
                            if (ARCH_UNLIKELY(iter->second.HasTrace())) {
                                gd->_baselineTraceMap.erase(event.addr);
                            }
                            // Register trace for the incoming entry.
                            if (ARCH_UNLIKELY(event.HasTrace())) {
                                auto traceIt = buf.traceMap.find(event.addr);
                                TF_DEV_AXIOM(traceIt != buf.traceMap.end());
                                gd->_baselineTraceMap[event.addr] =
                                    traceIt->second;
                            }
                            // Update incremental totals for the transition.
                            //   old=alloc, new=alloc: addr reuse, net bytes.
                            //   old=alloc, new=free:  one alloc gone.
                            //   old=free,  new=alloc: one alloc gained.
                            //   old=free,  new=free:  no change.
                            const bool oldAlloc = iter->second.IsAlloc();
                            const bool newAlloc = event.IsAlloc();
                            localBytesDelta +=
                                (newAlloc ? (int64_t)event.size : 0) -
                                (oldAlloc ? (int64_t)iter->second.size : 0);
                            localMaxDelta =
                                std::max(localMaxDelta, localBytesDelta);
                            localAllocDelta += (int)newAlloc - (int)oldAlloc;
                            iter.value() = event;
                        }
                    }
                }
            }

            // Flush incremental totals to global atomics under the baseline
            // lock we already hold.
            //
            // Plain read-modify-writes rather than fetch_add: every writer of
            // these three fields holds _baselineMutex, and this flush and
            // _ClearAll()'s reset are the only writers.  They are atomic so
            // that readers -- GetTotalBytes(), GetMaxTotalBytes() and the perf
            // report -- can load them without taking the lock.  Adding a writer
            // outside the lock would make these have to be real RMWs.
            const int64_t baseTotal =
                gd->_totalBytes.load(std::memory_order_relaxed);
            const int64_t newTotal = baseTotal + localBytesDelta;
            gd->_totalBytes = newTotal;
            // Drive the high-water from the peak the running total reached
            // *within* this batch (baseTotal + localMaxDelta), not merely its
            // end value (newTotal).  A batch that allocates and then frees
            // inside one consolidation would otherwise net out and hide the
            // peak.  localMaxDelta >= localBytesDelta, so this also covers the
            // end-of-batch value the old code used.
            const int64_t maxTotal = baseTotal + localMaxDelta;
            gd->_maxTotalBytes =
                std::max<int64_t>(gd->_maxTotalBytes.load(), maxTotal);
            gd->_liveAllocCount =
                gd->_liveAllocCount.load(std::memory_order_relaxed) +
                localAllocDelta;

            // Attribute this pass and the buffers it drained to the mode that
            // drove it.  Single writer (the worker thread), under the baseline
            // lock we already hold.
            switch (mode) {
            case _DrainMode::Spill:
                ++gd->_stats.spillConsolidations;
                gd->_stats.spillBuffers += threadBuffers.size();
                break;
            case _DrainMode::Periodic:
                ++gd->_stats.periodicConsolidations;
                gd->_stats.periodicBuffers += threadBuffers.size();
                break;
            case _DrainMode::Demand:
                ++gd->_stats.demandConsolidations;
                gd->_stats.demandBuffers += threadBuffers.size();
                break;
            }

            // If this is a demand consolidate, erase the free tombstones.
            //
            // Tombstones exist because spilled buffers are consolidated in
            // arbitrary relative order, so a free can reach the baseline before
            // the alloc it cancels (that alloc still sitting in another
            // thread's buffer or a later spill).  Retaining the free lets
            // IsNewer() reject the stale alloc when it does arrive.
            //
            // Purging them is safe here, and *only* here, because a demand
            // consolidation is a true stop-the-world barrier: it drained every
            // thread buffer and the whole spill queue with all thread locks
            // held, so every event recorded before the drain is in this batch,
            // and any alloc arriving later necessarily carries a
            // globalFreeCount >= the purged free's.
            //
            // This is exactly why the Spill and Periodic modes must not purge:
            // their drains are incremental (Spill takes no thread lock;
            // Periodic takes them one at a time), so an alloc that cancels a
            // retained free may still be sitting in an undrained buffer.  The
            // tombstone must survive to reject it.  Periodic drains only ever
            // add events; the tombstones they leave are collected by the next
            // demand barrier, whose atomic snapshot makes the purge valid.
            if (mode == _DrainMode::Demand) {
                for (auto iter = gd->_baselineEvents.begin();
                     iter != gd->_baselineEvents.end(); /*advance in body*/) {
                    iter = iter->second.IsFree()
                        ? gd->_baselineEvents.erase(iter)
                        : std::next(iter);
                }
            }
        }
    }

    // Hand the drained buffers back for reuse.  Done after releasing the
    // baseline lock: _PutFreeBuffer() takes _threadBuffersMutex, and nesting
    // that under _baselineMutex would introduce a lock-order edge that exists
    // nowhere else in this file.
    for (_ThreadBuffer &buf: threadBuffers) {
        gd->_PutFreeBuffer(std::move(buf));
    }
}

bool
Tf_MallocGlobalData::_Consolidator::TrySpillConsolidate()
{
    std::unique_lock<std::mutex> lock(_mutex, std::try_to_lock);
    if (lock.owns_lock()) {
        _EnsureRunning(lock);
        _spilled = true;
        lock.unlock();
        _cv.notify_one();
        return true;
    }
    return false;
}

void
Tf_MallocGlobalData::_Consolidator::DemandConsolidate()
{
    size_t ourDemand = 0;
    std::unique_lock<std::mutex> lock(_mutex);
    _EnsureRunning(lock);
    ourDemand = ++_demandsMade;    
    lock.unlock();
    _cv.notify_one();
    lock.lock();
    _cv.wait(lock, [&]() { return _demandsCompleted >= ourDemand; });
}

void
Tf_MallocGlobalData::_Consolidator::Stop()
{
    std::unique_lock<std::mutex> lock(_mutex);
    while (!_stopped) {
        if (!_stopRequested) {
            _stopRequested = true;
            lock.unlock();
            _cv.notify_one();
            lock.lock();
        }
        _cv.wait(lock, [&](){ return _stopped || !_stopRequested; });
    }
    if (_thread.joinable()) {
        _thread.join();
    }
}

void
Tf_MallocGlobalData::_RegisterThread(_ThreadData *td)
{
    // Do no allocations here.
    TfSpinMutex::ScopedLock lock(_threadListMutex);
    td->_epoch = _epoch.load(std::memory_order_relaxed);
    td->_next = _threadDataHead;
    _threadDataHead = td;
}

void
Tf_MallocGlobalData::_UnregisterThread(_ThreadData *td)
{
    // Drain the departing thread's buffer before removing it from the list, so
    // no events are lost.  We hold _threadListMutex while doing this to prevent
    // a concurrent consolidation from also trying to drain this thread.
    //
    // No replacement buffer is installed: td is on its way out and will never
    // append again.  As in _SpillThreadBuffer(), we only keep the events if
    // they belong to the current epoch.
    TfSpinMutex::ScopedLock threadListLock(_threadListMutex);
    {
        TfSpinMutex::ScopedLock threadLock(td->_mutex);
        if (!td->_buffer.events.empty() &&
            td->_epoch == _epoch.load(std::memory_order_relaxed)) {
            TfSpinMutex::ScopedLock buffersLock(_threadBuffersMutex);
            _threadBuffers.push_back(std::move(td->_buffer));
        }
    }
    TfMallocTag::_ThreadData **cur = &_threadDataHead;
    while (*cur && *cur != td) {
        cur = &(*cur)->_next;
    }
    if (TF_VERIFY(*cur)) {
        *cur = (*cur)->_next;
    }
}

_ThreadBuffer
Tf_MallocGlobalData::_PopFreeBufferLocked()
{
    _ThreadBuffer buf;
    if (!_freeBuffers.empty()) {
        buf = std::move(_freeBuffers.back());
        _freeBuffers.pop_back();
    }
    return buf;
}

_ThreadBuffer
Tf_MallocGlobalData::_TakeFreeBuffer()
{
    _ThreadBuffer buf;
    {
        TfSpinMutex::ScopedLock lock(_threadBuffersMutex);
        buf = _PopFreeBufferLocked();
    }
    // A no-op if we recycled one above.  Otherwise this allocates, deliberately
    // outside _threadBuffersMutex.
    buf.events.allocate();
    TF_DEV_AXIOM(buf.IsAllocated() && buf.events.empty());
    return buf;
}

void
Tf_MallocGlobalData::_PutFreeBuffer(_ThreadBuffer &&buf)
{
    if (!buf.IsAllocated()) {
        return; // Nothing worth keeping.
    }
    buf.Reset();
    {
        TfSpinMutex::ScopedLock lock(_threadBuffersMutex);
        if (_freeBuffers.size() < _GetFreeBufferPoolCap()) {
            _freeBuffers.push_back(std::move(buf));
        }
    }
    // If we did not keep it, `buf` still owns its array and frees it here, on
    // the way out and outside the lock.
}

void
Tf_MallocGlobalData::_SpillThreadBuffer(_ThreadData *td,
                                        TfSpinMutex::ScopedLock tdLock)
{
    bool tryConsolidate = false;
    TF_DEV_AXIOM(!td->_buffer.events.empty());
    // Only transfer the buffer if it is from the same epoch.  Otherwise we
    // discard it -- in which case td keeps its own (still allocated) buffer and
    // just clears it below.
    if (td->_epoch == _epoch.load(std::memory_order_relaxed)) {
        TfSpinMutex::ScopedLock buffersLock(_threadBuffersMutex);
        // Transfer the events, handing td a recycled buffer in the same step.
        // If the pool was dry the replacement comes back unallocated; we
        // allocate it below, after dropping this lock, so that we never
        // allocate underneath a spin mutex that every spilling thread contends.
        // That is safe because we hold tdLock for the whole function, so td
        // cannot append to the buffer in the interim.
        _threadBuffers.push_back(
            std::exchange(td->_buffer, _PopFreeBufferLocked()));
        // Wake the consolidator on every spill, so the high-water total is
        // sampled as close to each buffer-fill as the background thread can
        // manage.  TrySpillConsolidate() self-throttles: its try_lock fails
        // fast when a consolidation is already running, so under heavy load
        // spills coalesce into the consolidator's throughput rather than
        // forcing a separate consolidation per buffer.
        tryConsolidate = true;
    }
    // Prepare the thread's buffer for more logging.  allocate() is a no-op
    // except on the pool-was-dry path above, and on the epoch-mismatch path td
    // still owns the array it came in with.
    td->ResetBuffer();
    td->_buffer.events.allocate();
    TF_DEV_AXIOM(td->_buffer.IsAllocated());

    // Release the thread's lock now.
    tdLock.Release();

    // If we spilled we try to start a consolidate.  But we skip it without
    // waiting on _mutex if another consolidate is already in progress.
    if (tryConsolidate) {
        _consolidator.TrySpillConsolidate();
    }
}

void
Tf_MallocGlobalData::_DemandConsolidate()
{
    // The demand pass (or passes) is counted by the worker in _ConsolidateImpl.
    _consolidator.DemandConsolidate();
}

void
Tf_MallocGlobalData::_ClearAll(bool restartConsolidator)
{
    // Stop the consolidator first, before taking any locks.  A demand
    // consolidation acquires _threadListMutex and every per-thread mutex, so
    // stopping it while we hold those would deadlock: Stop() waits for the
    // worker to exit, and the worker waits for the locks we are holding.
    //
    // Stop() joins, so no consolidation is in flight once it returns.  Another
    // thread can restart the worker underneath us (via GetTotalBytes(), say);
    // that is handled by the epoch check in _ConsolidateImpl().
    _consolidator.Stop();

    // Open a new epoch and stamp every thread with it.  This is the single
    // point where the epoch advances, so both Clear() and Shutdown() get the
    // same protection: any buffer a thread spills from the old epoch -- or one
    // a restarted consolidator drains mid-flight -- sees the mismatch and is
    // discarded rather than reaching the new session's baseline.
    {
        TfSpinMutex::ScopedLock threadListLock(_threadListMutex);

        const int64_t newEpoch =
            _epoch.fetch_add(1, std::memory_order_relaxed) + 1;

        // Take all the thread locks simultaneously, so no thread is midway
        // through recording an event while we reset its state.
        std::vector<TfSpinMutex::ScopedLock> threadLocks;
        threadLocks.reserve(128);
        for (_ThreadData *td = _threadDataHead; td; td = td->_next) {
            threadLocks.emplace_back(td->_mutex);
        }

        // Discard all in-flight thread buffers.  Note that ResetBuffer() does
        // not allocate and each thread still owns its own array here (we never
        // moved it away), so this does no allocation with the world stopped.
        for (_ThreadData *td = _threadDataHead; td; td = td->_next) {
            td->ResetBuffer();
            td->_stats.Reset();
            td->_epoch = newEpoch;
        }
    }

    // Discard the buffer spill queue and the recycled buffer pool.
    {
        TfSpinMutex::ScopedLock buffersLock(_threadBuffersMutex);
        TfReset(_threadBuffers);
        TfReset(_freeBuffers);
    }

    // Discard baseline and reset totals.
    {
        TfSpinMutex::ScopedLock baselineLock(_baselineMutex);
        _baselineEvents = {};
        _baselineTraceMap = {};
        _totalBytes = 0;
        _maxTotalBytes = 0;
        _liveAllocCount = 0;
    }

    // Reset global stats.
    _stats.Reset();

    // Restart the periodic-wake worker unless we are tearing down.  _ClearAll
    // stopped it above (Stop() must precede taking the thread locks, or its
    // join would deadlock against the worker's own demand drain), so a Clear()
    // that did not restart it would leave the worker dormant -- and a process
    // that then held a co-live peak without spilling would miss it, exactly the
    // gap the periodic wake exists to close.  Done here, after every lock scope
    // above has closed, so the restart takes only the consolidator's own mutex.
    if (restartConsolidator) {
        _EnsureConsolidatorRunning();
    }
}

void
Tf_MallocGlobalData::_ReinitializeThreads()
{
    // Epoch stamping is _ClearAll()'s job; by the time we get here every
    // existing thread already carries the current epoch, and any thread that
    // registered since has picked it up in _RegisterThread().
    //
    // What is left to do is close one narrow gap.  A thread that had already
    // passed the IsInitialized() gate when Shutdown() flipped the state may
    // have appended events to its buffer afterward -- stamped with the new
    // epoch, and so indistinguishable from real events once the new session
    // opens.  Those belong to no session at all, so clear the buffers once more
    // here, immediately before we go live.
    //
    // We hold each thread's _mutex individually rather than simultaneously: we
    // need no consistent snapshot, just a safe reset of each thread in turn.
    TfSpinMutex::ScopedLock lock(_threadListMutex);
    for (_ThreadData *td = _threadDataHead; td; td = td->_next) {
        TfSpinMutex::ScopedLock tdLock(td->_mutex);
        td->ResetBuffer();
    }
}

size_t
Tf_MallocGlobalData::_GetTotalBytes()
{
    // Note: this triggers a full stop-the-world consolidation and is not a
    // lightweight call.  Callers that need frequent polling of memory usage
    // should be aware of this cost.
    _TemporaryDisabler disable;
    _DemandConsolidate();
    return _totalBytes.load();
}

size_t
Tf_MallocGlobalData::_GetMaxTotalBytes()
{
    // Note: this triggers a full stop-the-world consolidation and is not a
    // lightweight call.  See _GetTotalBytes().
    _TemporaryDisabler disable;
    _DemandConsolidate();
    return _maxTotalBytes.load();
}

////////////////////////////////////////////////////////////////////////
// Call tree construction

// Sum each tree node's direct bytes into a total per call-site name.  Operates
// on the built CallTree rather than on path nodes so that skipRepeated's
// folding of repeated nodes into their parents is reflected, which callers
// depend on.
static void
_AccumulateCallSiteBytes(TfMallocTag::CallTree::PathNode const &node,
                         std::unordered_map<std::string, size_t> *byName)
{
    (*byName)[node.siteName] += node.nBytesDirect;
    for (TfMallocTag::CallTree::PathNode const &child: node.children) {
        _AccumulateCallSiteBytes(child, byName);
    }
}

void
Tf_MallocGlobalData::_BuildCallTree(
    TfMallocTag::CallTree* tree, bool skipRepeated)
{
    _TemporaryDisabler disable;

    // Consolidate first, without holding the write lock.  This is the expensive
    // step and we want push/pop to be free to proceed throughout.
    _DemandConsolidate();

    // The write lock is still taken for the _callSiteTable iteration in
    // _Set{Trace,Debug}Names(), which we must not race.  Note that node
    // creation may proceed underneath us; see the note on _PathNodeTable about
    // why a report tolerates that.
    TfBigRWMutex::ScopedLock taggingStateLock(_taggingStateMutex);

    // Accumulate live totals per node and per call-site name from the baseline.
    // These are per-report side tables rather than counters on the nodes and
    // sites themselves, which is what lets this run without a reset pass.
    Tf_PathNodeCountsTable nodeCounts;
    {
        TfSpinMutex::ScopedLock baselineLock(_baselineMutex);
        for (auto const &p: _baselineEvents) {
            // Skip free tombstones.  They carry a null node, so this guard is
            // what keeps us from dereferencing it.  Note that the
            // _DemandConsolidate() above purges tombstones, but only under the
            // baseline lock at the end of its pass -- a spill consolidation can
            // land more of them in the window before we take the lock here.
            if (p.second.IsFree()) {
                continue;
            }
            Tf_PathNodeCounts &counts = nodeCounts[p.second.GetNode()];
            counts.nBytes += p.second.size;
            counts.nAllocations += 1;
        }
    }

    // Build the snapshot call tree.  Nodes absent from nodeCounts have no live
    // allocations billed directly to them and come out with zero counts.
    tree->root = _rootNode->_BuildTree(
        _BuildPathNodeChildrenTable(), nodeCounts, skipRepeated);

    // Accumulate per-site totals from the built tree, not from the baseline.
    // That matters: with skipRepeated the tree folds a repeated node's direct
    // bytes into its parent, so a site's total legitimately differs between the
    // two settings.
    {
        std::unordered_map<std::string, size_t> siteBytes;
        _AccumulateCallSiteBytes(tree->root, &siteBytes);
        tree->callSites.clear();
        tree->callSites.reserve(siteBytes.size());
        for (auto const &nameAndBytes: siteBytes) {
            tree->callSites.push_back(TfMallocTag::CallTree::CallSite {
                    nameAndBytes.first, nameAndBytes.second
                });
        }
    }

    {
        TfSpinMutex::ScopedLock baselineLock(_baselineMutex);
        _BuildUniqueMallocStacks(tree);
    }
}

Tf_PathNodeChildrenTable
Tf_MallocGlobalData::_BuildPathNodeChildrenTable() const
{
    // Walk the whole path node tree, mapping each node to its children --
    // including nodes with no live allocations, which appear in the resulting
    // CallTree with zero counts.
    //
    // It is tempting to visit only the nodes the baseline mentions plus their
    // ancestors.  That is the subtree a report ends up displaying, since
    // _ReportMallocNode() prunes zero-byte branches, and it would avoid
    // materializing a CallTree::PathNode -- each with a std::string and a
    // std::vector -- for possibly several million nodes.  It was tried and
    // reverted: it changes the structure callers receive, not merely what gets
    // printed, and callers legitimately rely on a zero-byte node being present.
    // The unit tests catch it by reading a node's nBytes as a baseline before
    // allocating under it.
    //
    // Doing it deliberately means first deciding what CallTree promises about
    // zero-byte nodes, and note the two report paths already disagree:
    // GetPrettyPrintString(TREE) does not prune them while Report() does.
    // Picking one behavior would settle both questions at once.
    //
    // Every node except the root is in the enumeration index, and every node
    // except the root has a parent, so one pass over it builds the whole
    // relation.  This is O(nodes).
    Tf_PathNodeChildrenTable result;
    result.reserve(_pathNodeTable.GetNumNodes());
    _pathNodeTable.ForEachNode([&result](Tf_MallocPathNode *node) {
        result[node->_GetParent()].push_back(node);
    });

    // Sort each child list so a report is reproducible run to run: enumeration
    // order is creation order, which is race-determined.  _ReportMallocNode()
    // sorts by name for output but _PrintMallocNode() does not, so do it here.
    for (auto parentAndChildrenIter = result.begin(), end = result.end();
         parentAndChildrenIter != end; ++parentAndChildrenIter) {
        std::vector<Tf_MallocPathNode const *> &children =
            parentAndChildrenIter.value();
        std::sort(children.begin(), children.end(),
                  [](Tf_MallocPathNode const *l, Tf_MallocPathNode const *r) {
                      const int cmp = strcmp(l->_callSite->_name,
                                             r->_callSite->_name);
                      return cmp != 0 ? cmp < 0 : l < r;
                  });
    }
    return result;
}

Tf_MallocGlobalData::_TreeStats
Tf_MallocGlobalData::_GetTreeStats() const
{
    _TreeStats st;
    if (!_rootNode) {
        return st;
    }

    // One pass over the enumeration index gives us the node count, each node's
    // child count, and each node's bucket.  Depth needs the parent chain, so it
    // is memoized in the same map rather than walked per node, which would be
    // O(nodes * depth) at a depth of ~400.
    //
    // This map is transiently large -- millions of entries on common workloads
    // -- which is acceptable for a diagnostic that is already the heaviest
    // thing in GetPerfStats().
    struct _NodeInfo {
        uint64_t numChildren = 0;
        uint64_t depth       = 0;
        bool     depthKnown  = false;
    };
    _PtrRobinMap<Tf_MallocPathNode const *, _NodeInfo> info;
    info.reserve(_pathNodeTable.GetNumNodes() + 1);

    // The root is in neither the table nor the index (it has no (parent, site)
    // key), so seed it.
    info[_rootNode] = _NodeInfo { 0, 0, /*depthKnown=*/true };

    // Bucket occupancy is derived from the nodes, by hashing each back to its
    // bucket, rather than by sweeping the directory.
    pxr_tsl::robin_map<size_t, uint64_t> bucketCounts;
    bucketCounts.reserve(_pathNodeTable.GetNumNodes());

    _pathNodeTable.ForEachNode(
        [&info, &bucketCounts](Tf_MallocPathNode *node) {
            // Make sure this node has an entry even if it has no children, so
            // that the passes below see every node.  insert() leaves an
            // existing entry alone, so this cannot clobber a count already
            // accumulated from a child seen earlier.
            info.insert({node, _NodeInfo {}});
            ++info[node->_GetParent()].numChildren;
            ++bucketCounts[_pathNodeTable.GetBucketIndex(node)];
        });

    for (auto iter = bucketCounts.begin(), end = bucketCounts.end();
         iter != end; ++iter) {
        const uint64_t chainLength = iter->second;
        ++st.occupancyHistogram[_CountBucket(chainLength)];
        st.maxChainLength = std::max(st.maxChainLength, chainLength);
    }

    // Fill in depths, walking up to the nearest ancestor with a known depth and
    // then back down.  Amortized O(1) per node across the whole pass.  Note
    // this mutates values but never inserts, so the outer iterator stays valid.
    // Every node reached by walking _parent is guaranteed to be in the map:
    // each is the parent of something, and the root was seeded above.
    std::vector<Tf_MallocPathNode const *> pending;
    for (auto iter = info.begin(), end = info.end(); iter != end; ++iter) {
        pending.clear();
        Tf_MallocPathNode const *node = iter->first;
        auto found = info.find(node);
        while (found != info.end() && !found->second.depthKnown) {
            pending.push_back(node);
            node = node->_GetParent();
            found = node ? info.find(node) : info.end();
        }
        // Walking up always reaches a node with a known depth.  Fall back to
        // zero rather than dereference end() if that ever stops being true.
        uint64_t depth = (found != info.end()) ? found->second.depth : 0;
        for (auto revIter = pending.rbegin(), revEnd = pending.rend();
             revIter != revEnd; ++revIter) {
            _NodeInfo &cur = info.find(*revIter).value();
            cur.depth = ++depth;
            cur.depthKnown = true;
        }
    }

    for (auto iter = info.begin(), end = info.end(); iter != end; ++iter) {
        const uint64_t fanOut = iter->second.numChildren;
        ++st.numNodes;
        st.maxDepth = std::max(st.maxDepth, iter->second.depth);
        st.maxFanOut = std::max(st.maxFanOut, fanOut);
        st.fanOutSum += fanOut;
        st.fanOutSqSum += fanOut * fanOut;
        ++st.fanOutHistogram[_CountBucket(fanOut)];
    }
    return st;
}

////////////////////////////////////////////////////////////////////////
// Stack trace and debug support

void
Tf_MallocGlobalData::_GetStackTrace(
    size_t skipFrames,
    std::vector<uintptr_t>* stack)
{
    uintptr_t buf[_MaxMallocStackDepth];
    
    // Get the stack trace.
    size_t numFrames =
        ArchGetStackFrames(_MaxMallocStackDepth, skipFrames, buf);

    // Copy into stack, reserving exactly enough space.
    stack->assign(buf, buf + numFrames);
}

void
Tf_MallocGlobalData::_SetTraceNames(const std::string& matchList)
{
    _TemporaryDisabler disable;

    _traceMatchTable.SetMatchList(matchList);

    // Update trace flag on every existing call site.
    for (auto const &p: _callSiteTable) {
        p.second->_SetFlag(Tf_MallocCallSite::_TraceFlag,
                           _traceMatchTable.Match(p.second->_name));
    }
}

bool
Tf_MallocGlobalData::_MatchesTraceName(const std::string& name)
{
    return _traceMatchTable.Match(name.c_str());
}

static bool Tf_MatchesMallocTagTraceName(const string& name)
{
    return _mallocGlobalData->_MatchesTraceName(name);
}

void
Tf_MallocGlobalData::_SetDebugNames(const std::string& matchList)
{
    _TemporaryDisabler disable;

    _debugMatchTable.SetMatchList(matchList);

    // Update debug flag on every existing call site.
    for (auto const &p: _callSiteTable) {
        p.second->_SetFlag(Tf_MallocCallSite::_DebugFlag,
                           _debugMatchTable.Match(p.second->_name));
    }
}

bool
Tf_MallocGlobalData::_MatchesDebugName(const std::string& name)
{
    return _debugMatchTable.Match(name.c_str());
}

static bool Tf_MatchesMallocTagDebugName(const string& name)
{
    return _mallocGlobalData->_MatchesDebugName(name);
}

Tf_MallocCallSite::Tf_MallocCallSite(const char *name)
    : _name(name)
    , _flags(
        (Tf_MatchesMallocTagDebugName(name) ? _DebugFlag : 0) |
        (Tf_MatchesMallocTagTraceName(name) ? _TraceFlag : 0))
{
}

////////////////////////////////////////////////////////////////////////
// Stats reporting

std::string
Tf_MallocGlobalData::_GetPerfStats(bool includePerThread)
{
    _TemporaryDisabler disable;

    // Collect per-thread stats snapshots while holding the thread list mutex.
    std::vector<_ThreadStats> threadStats;
    {
        TfSpinMutex::ScopedLock lock(_threadListMutex);
        for (_ThreadData *td = _threadDataHead; td; td = td->_next) {
            TfSpinMutex::ScopedLock tdLock(td->_mutex);
            threadStats.push_back(td->_stats.Snapshot());
        }
    }

    _GlobalStats const &gs = _stats;

    // Shape of the path node tree, and the size of the live baseline.
    const _TreeStats tree = _GetTreeStats();
    const size_t numCallSites = _callSiteTable.size();
    const size_t numImmortalNameSites = _immortalNameSites.GetNumEntries();
    size_t numBaselineEntries = 0;
    {
        TfSpinMutex::ScopedLock baselineLock(_baselineMutex);
        numBaselineEntries = _baselineEvents.size();
    }

    // Aggregate thread stats.
    _ThreadStats agg;
    for (_ThreadStats const &ts : threadStats) {
        agg.allocsTotal          += ts.allocsTotal;
        agg.freesTotal           += ts.freesTotal;
        agg.lookBackDiscards     += ts.lookBackDiscards;
        agg.spillsToGlobal       += ts.spillsToGlobal;
        agg.totalTags            += ts.totalTags;
        agg.emptyTags            += ts.emptyTags;
        agg.siteFastHits         += ts.siteFastHits;
        agg.siteSlowLookups      += ts.siteSlowLookups;
        agg.pathNodeGhostHits    += ts.pathNodeGhostHits;
        agg.pathNodeCacheMisses  += ts.pathNodeCacheMisses;
        agg.chainStepsTotal      += ts.chainStepsTotal;
        agg.maxStackDepth =
            std::max(agg.maxStackDepth, ts.maxStackDepth);
        for (size_t i = 0; i != NumCountBuckets; ++i) {
            agg.chainResolves[i] += ts.chainResolves[i];
            agg.chainSteps[i]    += ts.chainSteps[i];
        }
    }

    ////////////////////////////////////////////////////////////////////////
    // Helpers.
    auto pct = [](uint64_t num, uint64_t denom) -> std::string {
        if (denom == 0) { return "   n/a"; }
        return TfStringPrintf("%5.1f%%", 100.0 * num / denom);
    };

    // Compute descriptive statistics for a vector of doubles.
    struct DescStats {
        double mean   = 0.0;
        double median = 0.0;
        double stddev = 0.0;
        double min    = 0.0;
        double max    = 0.0;
        size_t n      = 0;
    };

    auto computeStats = [](std::vector<double> v) -> DescStats {
        DescStats s;
        s.n = v.size();
        if (s.n == 0) { return s; }
        std::sort(v.begin(), v.end());
        s.min = v.front();
        s.max = v.back();
        s.median = (s.n % 2 == 0)
            ? 0.5 * (v[s.n/2 - 1] + v[s.n/2])
            : v[s.n/2];
        double sum = 0.0;
        for (double x : v) { sum += x; }
        s.mean = sum / s.n;
        double var = 0.0;
        for (double x : v) { var += (x - s.mean) * (x - s.mean); }
        s.stddev = std::sqrt(var / s.n);
        return s;
    };

    // Collect per-thread vectors for descriptive stats.  Only include threads
    // with meaningful activity.
    std::vector<double> threadEvents, threadFastPct, threadSpills;
    for (_ThreadStats const &ts : threadStats) {
        const uint64_t eventsTotal = ts.allocsTotal + ts.freesTotal;
        if (eventsTotal == 0) { continue; }
        threadEvents.push_back(double(eventsTotal));
        threadFastPct.push_back(100.0 * ts.lookBackDiscards / eventsTotal);
        threadSpills.push_back(double(ts.spillsToGlobal));
    }

    std::string out;
    out.reserve(4096);

    auto line = TfOverloads {
        [&out]() { out += '\n'; },
        [&out](const char *fmt, ...) ARCH_PRINTF_FUNCTION(2, 3) {
            va_list ap;
            va_start(ap, fmt);
            out += TfVStringPrintf(fmt, ap);
            va_end(ap);
            out += '\n';
        }
    };

    ////////////////////////////////////////////////////////////////////////
    // Header.
    line("// TfMallocTag Performance Statistics //////////////////////////");
    line();

    ////////////////////////////////////////////////////////////////////////
    // Event pipeline.
    const uint64_t eventsTotal = agg.allocsTotal + agg.freesTotal;
    const uint64_t reachedGlobal = eventsTotal - agg.lookBackDiscards;

    line("Event Pipeline  (%zu threads)", threadStats.size());
    line("  Generated:               %10" PRIu64, eventsTotal);
    line("  Look-back discarded:     %10" PRIu64 "  (%s)",
         agg.lookBackDiscards, pct(agg.lookBackDiscards, eventsTotal).c_str());
    // Everything the look-back window did not kill is spilled toward the
    // baseline.
    line("  Reached consolidation:   %10" PRIu64 "  (%s)",
         reachedGlobal, pct(reachedGlobal, eventsTotal).c_str());
    line("  Live allocs (current):   %10" PRId64 "  %10zu bytes",
         _mallocGlobalData->_liveAllocCount.load(),
         (size_t)std::max(int64_t(0), _mallocGlobalData->_totalBytes.load()));
    line("  Baseline entries:        %10zu", numBaselineEntries);
    line();

    ////////////////////////////////////////////////////////////////////////
    // Consolidation.
    const uint64_t spillPasses    = gs.spillConsolidations.load();
    const uint64_t periodicPasses = gs.periodicConsolidations.load();
    const uint64_t demandPasses   = gs.demandConsolidations.load();
    const uint64_t totalPasses    = spillPasses + periodicPasses + demandPasses;

    const uint64_t spillBufs    = gs.spillBuffers.load();
    const uint64_t periodicBufs = gs.periodicBuffers.load();
    const uint64_t demandBufs   = gs.demandBuffers.load();
    const uint64_t totalBufs    = spillBufs + periodicBufs + demandBufs;

    line("Consolidation");
    line("  Passes:                  %10" PRIu64
         "  (spill: %" PRIu64 ", periodic: %" PRIu64 ", demand: %" PRIu64 ")",
         totalPasses, spillPasses, periodicPasses, demandPasses);
    line("  Buffers consolidated:    %10" PRIu64
         "  (spill: %" PRIu64 ", periodic: %" PRIu64 ", demand: %" PRIu64 ")",
         totalBufs, spillBufs, periodicBufs, demandBufs);
    // Producer-side count of buffers handed off to the spill queue.  Differs
    // from the spill column above: a spilled buffer is drained by whichever
    // pass scoops the queue, which may be periodic or demand.
    line("  Spills to global:        %10" PRIu64, agg.spillsToGlobal);
    line();

    ////////////////////////////////////////////////////////////////////////
    // Tag resolution.
    const uint64_t siteTotal = agg.siteFastHits + agg.siteSlowLookups;
    const uint64_t pnTotal = agg.totalTags;

    line("Tag Resolution");
    line("  Site fast hits : %10" PRIu64 "    rate: %s  "
         "(immortal-name address table, lock-free)",
         agg.siteFastHits, pct(agg.siteFastHits, siteTotal).c_str());
    line("  Site slow looks: %10" PRIu64 "    rate: %s  "
         "(content table, under read lock)",
         agg.siteSlowLookups, pct(agg.siteSlowLookups, siteTotal).c_str());
    line("  Immortal names : %10zu entries of %zu slots  (%s full)",
         numImmortalNameSites, _ImmortalNameSiteTable::Capacity,
         pct(numImmortalNameSites, _ImmortalNameSiteTable::Capacity).c_str());
    line("  PathNode G hits: %10" PRIu64 "    rate: %s",
         agg.pathNodeGhostHits, pct(agg.pathNodeGhostHits, pnTotal).c_str());
    line("  PathNode misses: %10" PRIu64 "    rate: %s",
         agg.pathNodeCacheMisses,
         pct(agg.pathNodeCacheMisses, pnTotal).c_str());
    line("  Empty Tags     : %10" PRIu64 "    rate: %s",
         agg.emptyTags, pct(agg.emptyTags, pnTotal).c_str());
    line("  Total Tags     : %10" PRIu64, pnTotal);

    line();

    ////////////////////////////////////////////////////////////////////////
    // Tag tree shape and path node table health.
    //
    // The fan-out numbers describe the workload's tagging shape.  The numbers
    // that describe cost are the two chain distributions.  Read them together:
    // occupancy is the table's shape, chain length per resolve is the traffic
    // over it, and it is entirely possible to have a few awful buckets that
    // nothing ever looks up.
    {
        line("Tag Tree");
        line("  Path nodes:              %10" PRIu64 "  %10zu KB arena",
             tree.numNodes, _pathNodeArena.GetNumBytes() / 1024);
        line("  Call sites:              %10zu", numCallSites);
        line("  Nodes per site:          %10.2f",
             numCallSites ? double(tree.numNodes) / double(numCallSites) : 0.0);
        line("  Max tree depth:          %10" PRIu64, tree.maxDepth);
        line("  Max tag stack depth:     %10" PRIu64, agg.maxStackDepth);
        line("  Max fan-out:             %10" PRIu64, tree.maxFanOut);
        line();

        line("  Path node table:");
        line("    Buckets:               %10zu  %10zu KB directory",
             _pathNodeTable.GetNumBuckets(),
             _pathNodeTable.GetNumBytes() / 1024);
        line("    Node index:            %10zu  %10zu KB",
             _pathNodeTable.GetNumNodes(),
             _pathNodeTable.GetNodeIndexNumBytes() / 1024);
        line("    Mean occupancy:        %10.3f  (nodes per bucket)",
             _pathNodeTable.GetNumBuckets()
             ? double(_pathNodeTable.GetNumNodes()) /
               double(_pathNodeTable.GetNumBuckets()) : 0.0);
        line("    Longest chain:         %10" PRIu64, tree.maxChainLength);
        line("    Resolves:              %10" PRIu64, agg.pathNodeCacheMisses);
        line("    Chain steps:           %10" PRIu64, agg.chainStepsTotal);
        if (agg.pathNodeCacheMisses) {
            line("    Steps per resolve:     %10.2f",
                 double(agg.chainStepsTotal) /
                 double(agg.pathNodeCacheMisses));
        }
        else {
            line("    Steps per resolve:     %10s", "n/a");
        }
        line();

        uint64_t maxBucket = 0;
        for (uint64_t n : tree.fanOutHistogram) {
            maxBucket = std::max(maxBucket, n);
        }
        line("  Fan-out distribution over %" PRIu64 " nodes:", tree.numNodes);
        for (size_t i = 0; i != NumCountBuckets; ++i) {
            const uint64_t n = tree.fanOutHistogram[i];
            if (!n) {
                continue;
            }
            // Bar scaled to the largest bucket, 40 columns full scale.
            const size_t bar = maxBucket ? size_t(40.0 * n / maxBucket) : 0;
            line("    %-6s %10" PRIu64 "  (%s)  %s",
                 _CountBucketLabel(i), n,
                 pct(n, tree.numNodes).c_str(),
                 std::string(bar, '#').c_str());
        }
        line();

        // Table shape: how many buckets hold how many nodes.  A healthy table
        // is almost entirely in the 0 and 1 buckets.  Anything in the long
        // buckets means either the directory is too small (which the mean
        // occupancy above will also show) or the hash is clustering (which it
        // will not).  Note bucket 0 is never populated: _GetTreeStats() only
        // visits buckets that hold at least one node, so every entry here is an
        // occupied bucket and the total is the number of buckets in use.
        uint64_t occupiedBuckets = 0;
        uint64_t maxOccupancy = 0;
        for (uint64_t n : tree.occupancyHistogram) {
            occupiedBuckets += n;
            maxOccupancy = std::max(maxOccupancy, n);
        }
        line("  Bucket occupancy over %" PRIu64 " occupied of %zu buckets:",
             occupiedBuckets, _pathNodeTable.GetNumBuckets());
        for (size_t i = 0; i != NumCountBuckets; ++i) {
            const uint64_t n = tree.occupancyHistogram[i];
            if (!n) {
                continue;
            }
            const size_t bar =
                maxOccupancy ? size_t(24.0 * n / maxOccupancy) : 0;
            line("    %-6s %12" PRIu64 "  (%s)  %s",
                 _CountBucketLabel(i), n,
                 pct(n, occupiedBuckets).c_str(),
                 std::string(bar, '#').c_str());
        }
        line();

        // Traffic: how long the chains that resolves actually walked were.  The
        // "steps" column is the decisive one -- see the note on
        // _ThreadStats::chainResolves for why a mean is not enough.
        uint64_t longSteps = 0;
        for (size_t i = FirstLongCountBucket; i != NumCountBuckets; ++i) {
            longSteps += agg.chainSteps[i];
        }
        uint64_t maxChainSteps = 0;
        for (uint64_t n : agg.chainSteps) {
            maxChainSteps = std::max(maxChainSteps, n);
        }
        line("  Chain length distribution over resolves "
             "(%s of steps are in chains of 9+):",
             pct(longSteps, agg.chainStepsTotal).c_str());
        line("    %-6s %12s  %12s  %7s",
             "steps", "resolves", "steps", "%steps");
        for (size_t i = 0; i != NumCountBuckets; ++i) {
            if (!agg.chainResolves[i]) {
                continue;
            }
            const size_t bar = maxChainSteps
                ? size_t(24.0 * agg.chainSteps[i] / maxChainSteps) : 0;
            line("    %-6s %12" PRIu64 "  %12" PRIu64 "  %7s  %s",
                 _CountBucketLabel(i),
                 agg.chainResolves[i], agg.chainSteps[i],
                 pct(agg.chainSteps[i], agg.chainStepsTotal).c_str(),
                 std::string(bar, '#').c_str());
        }
    }
    line();

    ////////////////////////////////////////////////////////////////////////
    // Thread distribution (descriptive stats).
    {
        auto es = computeStats(threadEvents);
        auto fp = computeStats(threadFastPct);
        auto sp = computeStats(threadSpills);

        const char *hdr  = "  %-18s  %12s  %12s  %12s  %12s  %12s";
        const char *row  = "  %-18s  %12.1f  %12.1f  %12.1f  %12.1f  %12.1f";
        const char *rpct =
            "  %-18s  %12.1f%%  %11.1f%%  %11.1f%%  %11.1f%%  %11.1f%%";

        line("Thread Distribution  (%zu active of %zu total)",
             threadEvents.size(), threadStats.size());
        line(hdr, "", "mean", "median", "stddev", "min", "max");
        line(hdr, "", "------------", "------------",
             "------------", "------------", "------------");
        line(row,  "Events/thread:",
             es.mean, es.median, es.stddev, es.min, es.max);
        line(rpct, "Fast-discard %:",
             fp.mean, fp.median, fp.stddev, fp.min, fp.max);
        line(row,  "Spills:",
             sp.mean, sp.median, sp.stddev, sp.min, sp.max);
        line();

        // Top 5 threads by event count.
        std::vector<std::pair<uint64_t,size_t>> byEvents;
        for (size_t i = 0; i != threadStats.size(); ++i) {
            if (threadStats[i].allocsTotal + threadStats[i].freesTotal > 0) {
                byEvents.emplace_back(
                    threadStats[i].allocsTotal + threadStats[i].freesTotal, i);
            }
        }
        std::sort(byEvents.rbegin(), byEvents.rend());
        line("Top threads by events:");
        const size_t topN = std::min(byEvents.size(), size_t(5));
        for (size_t i = 0; i != topN; ++i) {
            auto const &[ev, idx] = byEvents[i];
            line("  #%-3zu  thread %3zu:  %10" PRIu64 "  (%s of total)",
                i+1, idx, ev,
                pct(ev, agg.allocsTotal + agg.freesTotal).c_str());
        }
    }
    line();

    ////////////////////////////////////////////////////////////////////////
    // Per-thread table (optional).
    if (includePerThread) {
        line("Per-Thread Detail:");
        line("  %6s  %12s  %10s  %6s  %8s  %10s  %8s  %6s",
             "Thread", "Events", "LookBack%", "Spills",
             "Resolves", "Steps/Rslv", "SiteFast%", "Depth");
        line("  %6s  %12s  %10s  %6s  %8s  %10s  %8s  %6s",
             "------", "------------", "----------", "------",
             "--------", "----------", "--------", "------");
        for (size_t i = 0; i != threadStats.size(); ++i) {
            _ThreadStats const &ts = threadStats[i];
            if (ts.allocsTotal + ts.freesTotal == 0) {
                continue;
            }
            const std::string stepsPer = ts.pathNodeCacheMisses
                ? TfStringPrintf("%10.2f", double(ts.chainStepsTotal) /
                                           double(ts.pathNodeCacheMisses))
                : std::string("       n/a");
            line("  %6zu  %12" PRIu64 "  %10s  %6" PRIu64
                 "  %8" PRIu64 "  %10s  %8s  %6" PRIu64,
                i,
                ts.allocsTotal + ts.freesTotal,
                pct(ts.lookBackDiscards,
                    ts.allocsTotal + ts.freesTotal).c_str(),
                ts.spillsToGlobal,
                ts.pathNodeCacheMisses,
                stepsPer.c_str(),
                pct(ts.siteFastHits,
                    ts.siteFastHits + ts.siteSlowLookups).c_str(),
                ts.maxStackDepth);
        }
        line();
    }

    ////////////////////////////////////////////////////////////////////////
    // Configuration.
    line("Configuration:");
    line("  EventBufferCapacity              = %-15zu  "
         "ImmortalNameSlots = %zu",
         EventBufferCapacity,
         _ImmortalNameSiteTable::Capacity);
    line("  LookBackWindowSize               = %-15d  "
         "FreeCountTableSize = %d (~%zu KB)",
         LookBackWindowSize,
         FreeCountTableSize,
         sizeof(_globalFreeCount) / 1024);
    size_t freeBufferPoolCap = _GetFreeBufferPoolCap();
    if (freeBufferPoolCap == size_t(-1)) { // (practically) unlimited
        line("  FreeBufferPoolCap                = <unlimited>");
    }
    else {
        line("  FreeBufferPoolCap                = %zu (~%zu MB)",
             freeBufferPoolCap,
             (freeBufferPoolCap *
              EventBufferCapacity * _EventSize) / (1024 * 1024));
    }
    line("  PathNodeTableBuckets             = %-15zu  "
         "(~%zu MB directory)",
         _pathNodeTable.GetNumBuckets(),
         _pathNodeTable.GetNumBytes() / (1024 * 1024));

    return out;
}

////////////////////////////////////////////////////////////////////////
// Malloc/free interception hooks
//
// Invariant: recording order at the interception point.
//
// An alloc event must be recorded after the memory is obtained, and a free
// event before it is released.  Both wrappers below are written that way
// deliberately, and the ordering is enforced by nothing but the order of the
// statements -- so do not reorder them, and in particular do not move a free's
// bookkeeping after the release to shorten the interval where the block is
// still held.
//
// The reason is that the allocator may hand a just-freed address straight back
// out, possibly to another thread.  If a free were recorded after the release,
// that other thread could record its alloc of the same address first, and the
// two events would be ordered backwards -- making a dead allocation look live,
// or a live one look dead.  Recording the free first closes the window: any
// later alloc of that address is guaranteed to be sequenced after it.

void*
TfMallocTag::_MallocWrapper(size_t nBytes, const void*)
{
    void* ptr = _mallocHook.Malloc(nBytes);
    if (!IsInitialized()) {
        return ptr;
    }

    _ThreadData &td = Tls::Find();
    
    if (!td.TaggingEnabled() || ARCH_UNLIKELY(!ptr)) {
        return ptr;
    }

    _TemporaryDisabler disable(&td);
    td.Alloc(ptr, Tf_GetMallocBlockSize(ptr, nBytes));
    return ptr;
}

void*
TfMallocTag::_ReallocWrapper(void* oldPtr, size_t nBytes, const void*)
{
    /*
     * If ptr is NULL, we want to make sure we don't double count, because a
     * call to _mallocHook.Realloc(ptr, nBytes) could call through to our
     * malloc.  To avoid this, we'll explicitly short-circuit ourselves rather
     * than trust that the malloc library will do it.
     */
    if (!oldPtr) {
        return _MallocWrapper(nBytes, nullptr);
    }

    if (!IsInitialized()) {
        // Deliberately *not* bumping _globalFreeCount here.  See the note in
        // _FreeWrapper() -- no event pair can straddle an uninitialized period,
        // and this path must stay free of atomics.
        return _mallocHook.Realloc(oldPtr, nBytes);
    }

    _ThreadData &td = Tls::Find();

    // If tagging is explicitly disabled, just do the realloc and skip
    // everything else. This avoids a deadlock if we get here while updating
    // the global call site and path node tables.
    if (!td.TaggingEnabled()) {
        // We are not recording this free, but it must still be counted -- see
        // the note in _FreeWrapper().
        ++_globalFreeCount[_FreeCountIdx(oldPtr)].value;
        return _mallocHook.Realloc(oldPtr, nBytes);
    }

    _TemporaryDisabler disable(&td);

    // The free must be recorded before the realloc, not after: realloc
    // releases oldPtr internally, so another thread can receive and record
    // that address before we run again.
    td.Free(oldPtr);

    void *newPtr = _mallocHook.Realloc(oldPtr, nBytes);

    if (ARCH_UNLIKELY(!newPtr)) {
        // Realloc failed -- oldPtr is still live (POSIX).  We already
        // recorded a free for it above, so re-record it as a zero-byte alloc
        // to keep it alive in the baseline.  The original size is not
        // available here, so the allocation's byte contribution is lost.
        td.Alloc(oldPtr, 0);
        return newPtr;
    }

    td.Alloc(newPtr, Tf_GetMallocBlockSize(newPtr, nBytes));
    return newPtr;
}

void*
TfMallocTag::_MemalignWrapper(size_t alignment, size_t nBytes, const void*)
{
    void* ptr = _mallocHook.Memalign(alignment, nBytes);

    if (!IsInitialized()) {
        return ptr;
    }

    _ThreadData &td = Tls::Find();
    if (!td.TaggingEnabled() || ARCH_UNLIKELY(!ptr)) {
        return ptr;
    }
    
    _TemporaryDisabler disable(&td);
    td.Alloc(ptr, Tf_GetMallocBlockSize(ptr, nBytes));
    return ptr;
}

void
TfMallocTag::_FreeWrapper(void* ptr, const void*)
{
    if (!ptr) {
        return;
    }

    if (!IsInitialized()) {
        // Deliberately *not* bumping _globalFreeCount here.  The cancellation
        // condition only ever compares two events recorded by the same thread,
        // and no event pair can straddle an uninitialized period:
        // _ClearAll() discards every thread buffer and the whole baseline, so
        // nothing survives the gap to be compared across it.  This is also the
        // tagging-off hot path, which must stay free of atomic RMWs.
        _mallocHook.Free(ptr);
        return;
    }

    _ThreadData &td = Tls::Find();

    // If tagging is explicitly disabled, just do the free and skip everything
    // else.
    if (!td.TaggingEnabled()) {
        // We are not recording this free, but it must still be counted, because
        // other threads *are* recording and rely on _globalFreeCount to detect
        // that someone else may have freed an address they are holding.
        //
        // Without this, the consolidator thread -- which is permanently
        // _TaggingDisabled and frees constantly -- could free address X
        // invisibly, the allocator could hand X back to an app thread, and that
        // thread would end up with two live allocs for X at the same
        // globalFreeCount: it would wrongly cancel one against a later free and
        // trip the IsAlloc()-xor axiom in IsNewer().
        //
        // Bumping the global counter moves every thread's recorded count out of
        // date across this point, so cancellation fails conservatively.  That
        // is exactly the safe direction.
        ++_globalFreeCount[_FreeCountIdx(ptr)].value;
        _mallocHook.Free(ptr);
        return;
    }

    // Record the free before releasing the memory; see the note on recording
    // order at the top of this section.  These two statements must stay in this
    // order.
    _TemporaryDisabler disable(&td);
    td.Free(ptr);
    _mallocHook.Free(ptr);
}

////////////////////////////////////////////////////////////////////////
// Public API

/* static */ bool
TfMallocTag::Initialize(string *errMsgIn)
{
    string localErr;
    string *errMsg = errMsgIn ? errMsgIn : &localErr;
    
    // Spin until we either perform the initialization ourselves or observe that
    // another thread has completed it.
    while (true) {
        _InitState expected = _NotInitialized;
        if (_initState.compare_exchange_weak(expected, _Initializing,
                                             std::memory_order_acquire,
                                             std::memory_order_relaxed)) {
            // We won the CAS -- we are responsible for initialization.
            break;
        }
        if (expected == _Initialized) {
            return true;
        }
        // expected is _Initializing or _ShuttingDown -- spin.
        std::this_thread::yield();
    }

    // If global data doesn't exist yet, this is a first-time initialization.
    // Create it, try to install hooks, and we're done.
    const bool firstTime = !_mallocGlobalData;
    if (firstTime) {
        // Pull the settings before we fully initialize, otherwise we can get
        // recursion if we're the first to pull on the env setting registry
        // during a malloc.
        TfGetEnvSetting(PXR_TF_MALLOC_TAG_CONSOLIDATE_PERIOD_MS);
        TfGetEnvSetting(PXR_TF_MALLOC_TAG_EVENT_BUFFERS_MB);
        TfGetEnvSetting(PXR_TF_MALLOC_TAG_PATH_NODE_TABLE_MB);

        // Before anything can resolve a tag.  Idempotent, but only reachable
        // here anyway: nodes outlive a session, so a re-Initialize() after
        // Shutdown() must keep the table it already has.
        _pathNodeTable.Initialize();

        _mallocGlobalData = new Tf_MallocGlobalData();
        Tf_MallocCallSite* site = _mallocGlobalData->
            _GetOrCreateCallSite("__root", /*nameIsImmortal=*/true);
        // Create _rootNode from the arena like every other node, so all nodes
        // live in one place.  Deliberately not published into _pathNodeTable:
        // the root has no (parent, site) key to hash, and nothing ever looks it
        // up -- callers reach it directly through _rootNode.  Every enumeration
        // of the table therefore has to account for the root separately.
        _mallocGlobalData->_rootNode = new (_pathNodeArena.Allocate())
            Tf_MallocPathNode(site, /*parent=*/nullptr);
    }

    // Install hooks if not already installed.
    if (!_mallocHook.IsInitialized()) {
        _TemporaryDisabler disable;
        if (!_mallocHook.Initialize(
                _MallocWrapper, _ReallocWrapper, _MemalignWrapper, _FreeWrapper,
                errMsg)) {
            // Hook installation failed -- leave everything initialized but not
            // active so that a subsequent Initialize() can try again.  Note: we
            // leave _mallocGlobalData alive since destroying it safely would
            // require more machinery than it's worth.
            _initState.store(_NotInitialized, std::memory_order_release);
            return false;
        }
    }

    // Re-initialization after a Shutdown().  Hooks are already installed.
    // Stamp all existing threads with the new epoch to open the new session,
    // and clear any residual state.
    if (!firstTime) {
        _mallocGlobalData->_ReinitializeThreads();
    }

    _initState.store(_Initialized, std::memory_order_release);

    // Start the periodic-wake consolidator now, rather than waiting for the
    // first spill or demand so we don't miss allocation peaks before the first
    // buffer spill or demand consolidation.
    _mallocGlobalData->_EnsureConsolidatorRunning();
    return true;
}

/* static */ void
TfMallocTag::Shutdown()
{
    // Spin until either we transition from _Initialized to _ShuttingDown
    // (meaning we get to do the shutting down) or we see the _NotInitialized
    // state, meaning another thread has already shut us down.
    _InitState expected = _Initialized;
    while (true) {
        if (_initState.compare_exchange_weak(
                expected, _ShuttingDown,
                std::memory_order_acquire,
                std::memory_order_relaxed)) {
            break;
        }
        if (expected == _NotInitialized) {
            return;
        }
        // Continue until either we get to do the shutdown, or saw that someone
        // else completed it.
        expected = _Initialized;
        std::this_thread::yield();
    }

    _TemporaryDisabler disable;

    // Note that _initState is _ShuttingDown at this point, so IsInitialized()
    // is already false and threads have stopped recording new events.
    //
    // _ClearAll() opens a new epoch and stamps every existing thread with it
    // while holding the thread list lock, so any thread that slipped past the
    // initialized gate and spills a buffer from the old epoch -- or an
    // in-flight consolidation that drained one -- sees the mismatch and
    // self-discards its stale data.
    _mallocGlobalData->_ClearAll(/*restartConsolidator=*/false);

    _initState.store(_NotInitialized, std::memory_order_release);
}


void
TfMallocTag::Clear()
{
    if (IsInitialized()) {
        _TemporaryDisabler disable;
        _mallocGlobalData->_ClearAll(/*restartConsolidator=*/true);
    }
}

std::string
TfMallocTag::GetPerfStats(bool showPerThread)
{
    if (IsInitialized()) {
        return _mallocGlobalData->_GetPerfStats(showPerThread);
    }
    else {
        return "TfMallocTag not initialized\n";
    }
}    

bool
TfMallocTag::GetCallTree(CallTree* tree, bool skipRepeated)
{
    tree->callSites.clear();
    tree->root.nBytes = tree->root.nBytesDirect = 0;
    tree->root.nAllocations = 0;
    tree->root.siteName.clear();
    tree->root.children.clear();

    if (IsInitialized()) {
        _mallocGlobalData->_BuildCallTree(tree, skipRepeated);
        return true;
    }
    return false;
}

size_t
TfMallocTag::GetTotalBytes()
{
    // Note: triggers a full consolidation; not a lightweight call.
    return IsInitialized() ? _mallocGlobalData->_GetTotalBytes() : 0;
}

size_t
TfMallocTag::GetMaxTotalBytes()
{
    // Note: triggers a full consolidation; not a lightweight call.
    return IsInitialized() ? _mallocGlobalData->_GetMaxTotalBytes() : 0;
}

void
TfMallocTag::SetDebugMatchList(const std::string& matchList)
{
    if (TfMallocTag::IsInitialized()) {
        TfBigRWMutex::ScopedLock lock(_mallocGlobalData->_taggingStateMutex);
        _mallocGlobalData->_SetDebugNames(matchList);
    }
}

void
TfMallocTag::SetCapturedMallocStacksMatchList(const std::string& matchList)
{
    if (TfMallocTag::IsInitialized()) {
        TfBigRWMutex::ScopedLock lock(_mallocGlobalData->_taggingStateMutex);
        _mallocGlobalData->_SetTraceNames(matchList);
    }
}

vector<vector<uintptr_t>>
TfMallocTag::GetCapturedMallocStacks()
{
    vector<vector<uintptr_t>> result;

    if (!TfMallocTag::IsInitialized()) {
        return result;
    }

    _mallocGlobalData->_DemandConsolidate();

    // Push some malloc tags so what we do here doesn't pollute the results.
    TfAutoMallocTag tag("Tf", "TfMallocTag::GetCapturedMallocStacks");

    // Return one entry per live traced allocation -- the caller expects repeats
    // since it uses the multiset of stacks to display allocation counts.
    TfSpinMutex::ScopedLock lock(_mallocGlobalData->_baselineMutex);

    for (auto const &p: _mallocGlobalData->_baselineEvents) {
        // Skip free tombstones and untraced allocations.  A tombstone never
        // carries the trace bit, so the IsFree() test is redundant today; it is
        // here so that every baseline walk reads the same way.
        if (p.second.IsFree() || !p.second.HasTrace()) {
            continue;
        }
        auto traceIt = _mallocGlobalData->_baselineTraceMap.find(p.first);
        TF_DEV_AXIOM(traceIt != _mallocGlobalData->_baselineTraceMap.end());
        result.push_back(traceIt->second->frames);
    }
    return result;
}

TfMallocTag::_ThreadData *
TfMallocTag::_Begin(const char* name, _ThreadData *threadData)
{
    if (!name || !name[0]) {
        return nullptr;
    }

    _ThreadData &tls = threadData ? *threadData : TfMallocTag::Tls::Find();
    _TemporaryDisabler disable(&tls);

    tls.PushNewChild(name);

    return &tls;
}

TfMallocTag::_ThreadData *
TfMallocTag::_Begin(_ImmortalName name, _ThreadData *threadData)
{
    if (!name.str || !name.str[0]) {
        return nullptr;
    }

    _ThreadData &tls = threadData ? *threadData : TfMallocTag::Tls::Find();
    _TemporaryDisabler disable(&tls);

    tls.PushNewChild(name.str, /*nameIsImmortal=*/true);

    return &tls;
}


// The Auto/StackOverride exit path.  `tls` is always the thread data the
// matching pushes went to, and `nTags` is always the number of pushes that
// actually happened, so the pops here are balanced by construction and use the
// unchecked _ThreadData::Pop().  See the comment on Pop() for why that matters.
void
TfMallocTag::_End(int nTags, TfMallocTag::_ThreadData *tls)
{
    TF_DEV_AXIOM(tls);
    while (nTags--) {
        tls->Pop();
    }
}

// The manual TfMallocTag::Pop() path, which cannot assume a matching push.
void
TfMallocTag::_PopChecked()
{
    TfMallocTag::Tls::Find().PopChecked();
}

TfMallocTag::StackState
TfMallocTag::_GetCurrentStackState()
{
    _ThreadData &tls = TfMallocTag::Tls::Find();
    _TemporaryDisabler disable(&tls);
    // Call EnsureCurrentPathNode first then read the stack top.
    tls.EnsureCurrentPathNode();
    return StackState { tls.GetStackTopNode() };
}

TfMallocTag::_ThreadData *
TfMallocTag::StackOverride::_Push() const
{
    if (!TF_VERIFY(_state._top)) {
        return nullptr;
    }
    _ThreadData &tls = TfMallocTag::Tls::Find();
    _TemporaryDisabler disable(&tls);
    
    // We can just push the override node top onto the stack.  The node we're
    // pushing is likely to have a different parent than the current stack top,
    // but that's okay.  Nothing looks beyond the stack top, so operations like
    // pushing & popping new tags will create new path nodes appropriately.
    // When the stack override pops, the stack state returns to what it was
    // prior to the override.  This makes tag-stack overrides ultra-lightweight.
    tls.Push(_state._top);
    return &tls;
}

void
TfMallocTag::StackOverride::_Pop() const
{
    if (!TF_VERIFY(_state._top && _tls)) {
        return;
    }
    TF_DEV_AXIOM(_tls->GetStackTopNode() == _state._top);
    _tls->Pop();
}


void
TfMallocTag::PauseControl::_Pause(_Scope desired)
{
    TF_DEV_AXIOM(_scope == _NotPaused);
    
    if (!TfMallocTag::IsInitialized()) {
        return;
    }
    // desired indicates the state we aspire to reach.  We're guaranteed not to
    // be paused currently.
    _td = &Tls::Find();
    if (!TF_VERIFY(_td)) {
        return;
    }
    if (desired == _ThisThread) {
        _td->PauseLocal();
    }
    else if (TF_VERIFY(desired == _AllThreads)) {
        _td->PauseGlobal();
    }
    _scope = desired;
}

void
TfMallocTag::PauseControl::_Unpause()
{
    TF_DEV_AXIOM(_scope != _NotPaused);
    if (!TF_VERIFY(_td)) {
        _scope = _NotPaused;
        return;
    }
    // Verify we're on the same thread that paused.
    TF_VERIFY(_td == &Tls::Find());
    if (_scope == _ThisThread) {
        _td->UnpauseLocal();
    }
    else {
        TF_DEV_AXIOM(_scope == _AllThreads);
        _td->UnpauseGlobal();
    }
    _scope = _NotPaused;
    _td = nullptr;
}

////////////////////////////////////////////////////////////////////////
// Report generation

namespace {
// Hash functor for a malloc stack.
//
struct _HashMallocStack
{
    size_t operator()(const vector<uintptr_t> &stack) const {
        return ArchHash(
            (const char *)&stack[0], sizeof(uintptr_t) * stack.size());
    }
};

// The data associated with a malloc stack (a pointer to the malloc stack
// itself, and the allocation size and number of allocations).
//
struct _MallocStackData
{
    vector<uintptr_t> const *stack;
    size_t size;
    size_t numAllocations;
};
}

// Builds a vector of unique captured malloc stacks and stores the result in
// tree->capturedCallStacks.  The malloc stacks are sorted with the stacks that
// allocated the most memory at the front of the vector.
//
void
Tf_MallocGlobalData::_BuildUniqueMallocStacks(TfMallocTag::CallTree *tree)
{
    if (_baselineTraceMap.empty()) {
        return;
    }

    // Accumulate size and numAllocations per unique stack trace.  We key by
    // _StackTrace const* since the global _StackTraceTable deduplicates by
    // content, so pointer equality implies stack equality.
    std::unordered_map<_StackTrace const*, _MallocStackData> map;

    for (auto const &p: _baselineEvents) {
        // Skip free tombstones and untraced allocations -- see the same guard in
        // TfMallocTag::GetCapturedMallocStacks().
        if (p.second.IsFree() || !p.second.HasTrace()) {
            continue;
        }
        auto traceIt = _baselineTraceMap.find(p.first);
        TF_DEV_AXIOM(traceIt != _baselineTraceMap.end());
        _StackTrace const *trace = traceIt->second;

        _MallocStackData &data = map[trace];
        data.stack = &trace->frames;
        data.size += p.second.size;
        data.numAllocations += 1;
    }
    
    // Sort by allocation size.
    std::vector<_MallocStackData const*> sorted;
    sorted.reserve(map.size());
    for (auto const &kv : map) {
        sorted.push_back(&kv.second);
    }
    std::sort(sorted.begin(), sorted.end(),
              [](auto const *l, auto const *r) {
                  return l->size < r->size;
              });

    tree->capturedCallStacks.reserve(sorted.size());
    for (auto it = sorted.rbegin(); it != sorted.rend(); ++it) {
        _MallocStackData const &data = **it;
        tree->capturedCallStacks.push_back(TfMallocTag::CallStackInfo());
        TfMallocTag::CallStackInfo &info =
            tree->capturedCallStacks.back();
        info.stack = *data.stack;
        info.size = data.size;
        info.numAllocations = data.numAllocations;
    }
}


// Returns the given number as a string with commas used as thousands
// separators.
//
static string
_GetAsCommaSeparatedString(size_t number)
{
    string result;

    string str = TfStringPrintf("%zu", number);
    size_t n = str.size();

    for (char c: str) {
        if (n < str.size() && n%3 == 0) {
            result.push_back(',');
        }
        result.push_back(c);
        n--;
    }
    return result;
}

static void
_PrintHeader(string *rpt)
{
    *rpt += "\n" + string(80, '-') + "\n";
    *rpt += TfStringPrintf("\nMalloc Tag Report\n\n\n");
    *rpt += TfStringPrintf("Total bytes = %s\n\n\n",
        _GetAsCommaSeparatedString(TfMallocTag::GetTotalBytes()).c_str());
}

static size_t
_PrintMallocNode(
    string* rpt,
    const TfMallocTag::CallTree::PathNode &node,
    size_t rootTotal,
    size_t parentTotal,
    size_t level,
    size_t &printedNodes,
    size_t maxPrintedNodes)
{
    if (!level) {
        // XXX:cleanup  We should pass in maxNameWidth and generate format
        //              strings like in _PrintMallocCallSites().
        *rpt += TfStringPrintf("%-72s %15s%15s %5s %5s %5s\n", "TAGNAME",
                               "BytesIncl", "BytesExcl", "%Prnt", "% Exc",
                               "%Totl");
        *rpt += TfStringPrintf("%-72s %12s%12s %5s %5s %5s\n\n",
                               string(72, '-').c_str(),
                               " --------------", " --------------",
                               "-----", "-----", "-----");

        rootTotal = node.nBytes;
    }

    size_t maxNameWidth = 72;
    size_t indent = level;

    if (printedNodes >= maxPrintedNodes) {
        return 0;
    }
    printedNodes++;

    string name = string(indent, ' ') +
        node.siteName.substr(0, maxNameWidth-indent);
    int postLen = static_cast<int>(maxNameWidth - name.length());
    if (postLen > 0) {
        name += string(postLen, ' ');
    }

    *rpt += TfStringPrintf(
        "%s %15s%15s ",
        name.c_str(),
        _GetAsCommaSeparatedString(node.nBytes).c_str(),
        _GetAsCommaSeparatedString(node.nBytesDirect).c_str());

    string curPercent;
    string curPercentDirect;
    string percentDirectOfRoot;

    if (parentTotal) {

        float percent = node.nBytes/(float)parentTotal*100;
        if (percent > 0.5) {
            curPercent = TfStringPrintf(" %.0f%%", percent);
        }
        percent = node.nBytesDirect/(float)node.nBytes*100;
        if (percent > 0.5) {
            curPercentDirect = TfStringPrintf(" %.0f%%", percent);
        }

        percent = node.nBytesDirect/(float)rootTotal*100;
        if (percent > 0.5) {
            percentDirectOfRoot = TfStringPrintf(" %.0f%%", percent);
        }
    }

    if (!level) {
        // For Root, take the bytesDirect as the rootPercentage

        float percent = 100*node.nBytesDirect/(float)rootTotal;
        if (percent > 0.5) {
            percentDirectOfRoot = TfStringPrintf(" %.0f%%", percent);
        }
    }
    *rpt += TfStringPrintf("%5s %5s %5s\n", curPercent.c_str(),
                           curPercentDirect.c_str(),
                           percentDirectOfRoot.c_str());

    vector<TfMallocTag::CallTree::PathNode>::const_iterator it;
    for (it = node.children.begin(); it != node.children.end(); ++it) {
        _PrintMallocNode(rpt, *it, rootTotal, node.nBytes, level+1,
                         printedNodes,
                         maxPrintedNodes);
    }

    return rootTotal;
}

static void
_PrintMallocCallSites(
   string* rpt,
   const vector<TfMallocTag::CallTree::CallSite>& callSites,
   size_t rootTotal)
{
    *rpt += TfStringPrintf("\n\nCall Sites\n\n");

    // Use a multimap to sort by allocation size.
    std::multimap<size_t, const string *> map;
    for (TfMallocTag::CallTree::CallSite const &cs: callSites) {
        map.insert(make_pair(cs.nBytes, &cs.name));
    }

    // XXX:cleanup We should pass in maxNameWidth.
    const size_t maxNameWidth = 72;
    const size_t maxBytesWidth = 15;
    const size_t maxPercentageWidth = 15;

    string fmt = TfStringPrintf(
        "%%-%lds %%%lds %%%lds\n",
        maxNameWidth, maxBytesWidth, maxPercentageWidth);

    *rpt += TfStringPrintf(fmt.c_str(), "NAME", "BYTES", "%ROOT");
    *rpt += string(maxNameWidth, '-') + ' ' + 
            string(maxBytesWidth, '-') + ' ' + 
            string(maxPercentageWidth, '-') + "\n\n";

    TF_REVERSE_FOR_ALL(it, map) {
        size_t nBytes = it->first;
        const string &name = *it->second;

        string curPercent;
        if (rootTotal) {
            double percent = 100.0*nBytes/rootTotal;
            // Don't print anything less than 0.1%.
            if (percent < 0.1) {
                break;
            }
            curPercent = TfStringPrintf("%.1f%%", percent);
        }

        *rpt += TfStringPrintf(
            fmt.c_str(),
            name.substr(0, maxNameWidth).c_str(),
            _GetAsCommaSeparatedString(nBytes).c_str(),
            curPercent.c_str());
    }
}

// Comparison for sorting CallTree::PathNodes.
//
static bool
_MallocPathNodeLessThan(
    const TfMallocTag::CallTree::PathNode *lhs,
    const TfMallocTag::CallTree::PathNode *rhs)
{
    return lhs->siteName < rhs->siteName;
}

static void
_ReportMallocNode(
    std::ostream &out,
    const TfMallocTag::CallTree::PathNode &node,
    size_t level,
    const std::string *rootName = nullptr)
{
    // Prune empty branches.  nBytes is inclusive of descendants and counts only
    // live allocations, so a zero here means the whole subtree is empty.
    if (node.nBytes == 0) {
        return;
    }

    string indent(2*level, ' ');

    // Insert '|' characters every 4 spaces.
    for (size_t i=0; i<(level + 1)/2; i++) {
        indent[4*i] = '|';
    }

    out << TfStringPrintf(
        "%13s B %13s B %7ld samples    ",
        _GetAsCommaSeparatedString(node.nBytes).c_str(),
        _GetAsCommaSeparatedString(node.nBytesDirect).c_str(),
        node.nAllocations);

    out << indent
        << (rootName && !rootName->empty() ? *rootName : node.siteName)
        << std::endl;

    // Sort the children by name.  The reason for doing this is that it is the
    // easiest way to provide stable results for diffing.  You could argue that
    // letting the diff program do the sorting is more correct (i.e. that
    // sorting is a view into the unaltered source data).
    std::vector<const TfMallocTag::CallTree::PathNode *> sortedChildren;
    sortedChildren.reserve(node.children.size());
    for (TfMallocTag::CallTree::PathNode const &child: node.children) {
        sortedChildren.push_back(&child);
    }

    std::sort(
        sortedChildren.begin(), sortedChildren.end(), _MallocPathNodeLessThan);

    for (TfMallocTag::CallTree::PathNode const *childPtr: sortedChildren) {
        _ReportMallocNode(out, *childPtr, level+1);
    }
}

static void
_ReportCapturedMallocStacks(
    std::ostream &out,
    const std::vector<TfMallocTag::CallStackInfo> &stackInfos)
{
    size_t numReportedStacks =
        TfMin(stackInfos.size(), _MaxReportedMallocStacks);

    size_t totalSize = 0;
    size_t totalNumAllocations = 0;
    size_t reportSize = 0;
    size_t reportNumAllocations = 0;

    for(size_t n=0; n<stackInfos.size(); n++) {
        const TfMallocTag::CallStackInfo &stackInfo = stackInfos[n];
        totalSize += stackInfo.size;
        totalNumAllocations += stackInfo.numAllocations;
        if (n < numReportedStacks) {
            reportSize += stackInfo.size;
            reportNumAllocations += stackInfo.numAllocations;
        }
    }

    out << "\n\n\n"
        << "Captured Malloc Stacks\n"
        << "\n"
        << "Number of unique captured malloc stacks:          "
            << _GetAsCommaSeparatedString(stackInfos.size()) << "\n"
        << "Total allocated memory by captured mallocs:       "
            << _GetAsCommaSeparatedString(totalSize) << "\n"
        << "Total number of allocations by captured mallocs:  "
            << _GetAsCommaSeparatedString(totalNumAllocations) << "\n"
        << "\n"
        << "Number of captured malloc stacks in report:       "
            << _GetAsCommaSeparatedString(numReportedStacks) << "\n"
        << "Allocated memory by mallocs in report:            "
            << _GetAsCommaSeparatedString(reportSize) << "\n"
        << "Number of allocations by mallocs in report:       "
            << _GetAsCommaSeparatedString(reportNumAllocations) << "\n"
        << "Percentage of allocated memory covered by report: "
            << (totalSize
                ? TfStringPrintf("%.1f%%", 100.0*reportSize/totalSize)
                : std::string("n/a")) << "\n\n";

    for(size_t n=0; n<numReportedStacks; n++) {
        const TfMallocTag::CallStackInfo &stackInfo = stackInfos[n];

        out << string(100, '-') << "\n"
            << "Captured malloc stack #" << n << "\n"
            << "Size:            " <<
                _GetAsCommaSeparatedString(stackInfo.size) << "\n"
            << "Num allocations: " <<
                _GetAsCommaSeparatedString(stackInfo.numAllocations) << "\n";

        ArchPrintStackFrames(out, stackInfo.stack);
    }
}

string
TfMallocTag::CallTree::GetPrettyPrintString(PrintSetting setting,
                                            size_t maxPrintedNodes) const
{
    string rpt;

    _PrintHeader(&rpt);

    if (setting == TREE || setting == BOTH) {
        size_t printedNodes = 0;
        size_t reportedMem = 
            _PrintMallocNode(&rpt, this->root, 0, 0, 0, printedNodes,
                             maxPrintedNodes);
        if (printedNodes >= maxPrintedNodes
            && reportedMem != GetTotalBytes()) {
            rpt += TfStringPrintf("\nWARNING: limit of %zu nodes visited, but "
                                  "only %zu bytes of %zu accounted for.  "
                                  "Running with a larger maxPrintedNodes will "
                                  "produce more accurate results.\n",
                                  maxPrintedNodes, 
                                  reportedMem, 
                                  GetTotalBytes());

        }
    }

    if (setting == CALLSITES || setting == BOTH) {
        _PrintMallocCallSites(&rpt, this->callSites, this->root.nBytes);
    }

    return rpt;
}

void
TfMallocTag::CallTree::Report(
    std::ostream &out) const
{
    const std::string emptyRootName;
    Report(out, emptyRootName);
}

static const std::string &
_GetTreeHeader()
{
    static const std::string treeHeader("Tree view  ==============");
    return treeHeader;
}

void
TfMallocTag::CallTree::Report(
    std::ostream &out,
    const std::string &rootName) const
{
    out << "\n" << _GetTreeHeader() << "\n";
    out << "      inclusive       exclusive\n";

    _ReportMallocNode(out, this->root, 0, &rootName);

    // Also add the dominant call sites to the report.
    out << GetPrettyPrintString(CALLSITES);

    // And the captured malloc stacks if there are any.
    if (!this->capturedCallStacks.empty()) {
        _ReportCapturedMallocStacks(out, this->capturedCallStacks);
    }
}

bool
TfMallocTag::CallTree::LoadReport(
    std::istream &in)
{
    // The matches here are:
    // 1. Exclusive memory (in bytes)
    // 2. Inclusive memory (in bytes)
    // 3. The number of allocations recorded
    // 4. The indentation string, which we match exactly (excluding leading
    //    whitespace) so that we can use the length to compute the depth, below.
    // 5. The name of the callsite.
    static const std::regex re(
        R"( *([\d].*) B *([\d].*) B *([\d].*) samples    ([ |]*)(.*))");

    // State of the parser. 
    enum class State {
        // Tree header not yet found
        FindingTree,
        // Found root, reading scopes
        ReadingTree
    } state = State::FindingTree;

    // Initialize the stack with a synthetic root node, so that we can support
    // loading multiple report trees. (This root node will be elided later if we
    // end up loading a single tree.)
    root = {0, 0, 0, "root", {}};
    std::stack<PathNode *> nodes;
    nodes.push(&root);

    // Parse the file contents
    for (std::string line; std::getline(in, line);) {
        // When finding the tree, only parse for the tree header.
        if (state == State::FindingTree) {
            if (line == _GetTreeHeader()) {
                state = State::ReadingTree;
            }
            continue;
        }
        if (!TF_VERIFY(state == State::ReadingTree)) {
            break;
        }

        // When we see an empty line, that means we've gotten a full tree.  Clear
        // the stack and switch back to tree finding.
        if (TfStringTrim(line).empty()) {
            state = State::FindingTree;
            nodes = {};
            nodes.push(&root);
            continue;
        }

        std::cmatch match;
        if (!std::regex_match(line.c_str(), match, re)) {
            continue;
        }

        const size_t nBytes =
            TfStringToULong(TfStringReplace(match[1].str(), ",", ""));
        const size_t nBytesDirect =
            TfStringToULong(TfStringReplace(match[2].str(), ",", ""));
        const size_t nAllocations = TfStringToULong(match[3].str());
        const size_t depth = match[4].length() / 2;
        const std::string& siteName = match[5].str();

        // Pop nodes off the stack until the top is the parent of the node we
        // just parsed. (We add one here to account for the synthetic root
        // node.)
        while (nodes.size() > depth + 1) {
            nodes.pop();
        }

        PathNode *const parent = nodes.top();

        // Add the current node as a child.
        parent->children.push_back(
            {nBytes, nBytesDirect, nAllocations, siteName, {}});

        // Push the child onto the stack.
        PathNode *const child = &(parent->children.back());
        nodes.push(child);
    }

    // If we only ended up with one tree, elide the root node, since we only
    // created it to support reading multiple trees. Otherwise, sum up the child
    // allocations, mainly so we have a non-zero value at the root, so that we
    // don't produce an empty report by pruning at the root.
    if (root.children.size() == 1) {
        // Copy the child out before overwriting the root: `root =
        // root.children[0]` assigns from a member of the object being assigned,
        // so root.children is destroyed while it is still the source.  Linux
        // happened to survive it; Windows did not.
        const PathNode newRoot = root.children[0];
        root = newRoot;
    } else {
        for (const PathNode& child : root.children) {
            root.nBytes += child.nBytes;
        }
    }

    return true;
}

TfMallocTag::CallTree::PathNode
Tf_MallocPathNode::_BuildTree(Tf_PathNodeChildrenTable const &pathNodeChildren,
                              Tf_PathNodeCountsTable const &nodeCounts,
                              bool skipRepeated) const
{
    // We're doing a non-recursive post-order tree traversal of `this`, building
    // up a `TfMallocTag::CallTree` to eventually return.
    struct _StackEntry {
        Tf_MallocPathNode const *pathNode = nullptr;
        TfMallocTag::CallTree::PathNode callTreeNode;
        uint32_t parentIndex = 0;
        bool postOrderVisit = false;
        // Whether pathNode's call site also appears among its ancestors.
        // Computed when the node is entered, consumed when it is popped.
        bool isRepeated = false;
    };
    // We use a deque here for the convenience that push/pop operations do not
    // invalidate references.
    std::deque<_StackEntry> stack;

    // Occurrence count per call site along the current root-to-node path, which
    // is all `skipRepeated` needs: a node is "repeated" exactly when its own
    // site is already on the path above it, i.e. when its count exceeds one.
    //
    // Correctness rests on a property of this traversal, so check it if the
    // loop below is ever restructured: a node is entered (the postOrderVisit ==
    // false branch) only once it is stack.back(), and at that moment every
    // entry below it is either an ancestor or an ancestor's not-yet-entered
    // sibling.  Since counting happens on entry rather than on push, those
    // pending siblings contribute nothing, so increment-on-enter plus
    // decrement-on-pop tracks the current path exactly.
    //
    // Zero entries are left in place rather than erased.  That bounds the map
    // by the number of distinct sites in the tree instead of by depth, and
    // avoids churning an allocation each time a site leaves and rejoins a path.
    std::unordered_map<Tf_MallocCallSite const *, uint32_t, _HashAddrObj>
        pathSiteCounts;

    // Push a new entry for `pathNode` on the stack, whose parent is at
    // `parentIndex` in the stack.  The new entry gets a `callTreeNode`
    // initialized with `pathNode`'s initial statistics.
    auto push = [&stack, &nodeCounts](Tf_MallocPathNode const *pathNode,
                                      uint32_t parentIndex) {
        TfMallocTag::CallTree::PathNode callTreeNode;
        // A node absent from nodeCounts has no live allocations billed directly
        // to it; it is present in the tree only as an ancestor of nodes that
        // do.
        auto countsIter = nodeCounts.find(pathNode);
        if (countsIter != nodeCounts.end()) {
            callTreeNode.nBytes = callTreeNode.nBytesDirect =
                countsIter->second.nBytes;
            callTreeNode.nAllocations = countsIter->second.nAllocations;
        }
        else {
            callTreeNode.nBytes = callTreeNode.nBytesDirect = 0;
            callTreeNode.nAllocations = 0;
        }
        callTreeNode.siteName = pathNode->_callSite->_name;
        stack.push_back(_StackEntry {
                pathNode, std::move(callTreeNode),
                parentIndex, /*postOrderVisit=*/false
            });
    };

    // Push the root, then process the stack.
    push(this, 0);
    while (stack.size() > 1 || !stack.back().postOrderVisit) {
        _StackEntry &cur = stack.back();
        if (cur.postOrderVisit) {
            // Leaving cur, so it is no longer on the path.
            if (skipRepeated) {
                --pathSiteCounts[cur.pathNode->_callSite];
            }
            // Accumulate cur into its parent and pop.
            _StackEntry &parent = stack[cur.parentIndex];
            // In all cases, add cur's total bytes to its parent.
            parent.callTreeNode.nBytes += cur.callTreeNode.nBytes;
            if (skipRepeated && cur.isRepeated) {
                // If cur is a repeated node, add cur's direct contribution to
                // its parent.
                parent.callTreeNode.nBytesDirect +=
                    cur.callTreeNode.nBytesDirect;
                // And move any children to the parent.
                parent.callTreeNode.children.insert(
                    parent.callTreeNode.children.end(),
                    std::make_move_iterator(cur.callTreeNode.children.begin()),
                    std::make_move_iterator(cur.callTreeNode.children.end())
                    );
            }
            else {
                // Add cur as a child of its parent.
                parent.callTreeNode
                    .children.push_back(std::move(cur.callTreeNode));
            }
            stack.pop_back();
        }
        else {
            // Entering cur: everything entered and not yet popped is exactly
            // its ancestry, so this both records cur on the path and answers
            // whether its site was already there.
            if (skipRepeated) {
                cur.isRepeated =
                    ++pathSiteCounts[cur.pathNode->_callSite] > 1;
            }
            // Push any children and mark cur for postOrderVisit.
            if (pathNodeChildren.count(cur.pathNode)) {
                auto const &children =
                    pathNodeChildren.find(cur.pathNode).value();
                const uint32_t curIndex = static_cast<uint32_t>(stack.size()-1);
                // Push children in reverse order so that they appear in the
                // resulting call tree in forward order.
                for (auto childIter = children.rbegin(), end = children.rend();
                     childIter != end; ++childIter) {
                    push(*childIter, curIndex);
                }
                cur.callTreeNode.children.reserve(children.size());
            }
            cur.postOrderVisit = true;
        }
    }

    // Now we should have just the root left with the completed tree.
    TfMallocTag::CallTree::PathNode ret;
    if (TF_VERIFY(stack.size() == 1 && stack.back().postOrderVisit)) {
        ret = std::move(stack.back().callTreeNode);
    }
    return ret;
}

PXR_NAMESPACE_CLOSE_SCOPE
