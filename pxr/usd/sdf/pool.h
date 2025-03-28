//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_POOL_H
#define PXR_USD_SDF_POOL_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/hints.h"
#include "pxr/base/arch/pragmas.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticData.h"

#include <tbb/concurrent_queue.h>

#include <atomic>
#include <thread>

PXR_NAMESPACE_OPEN_SCOPE

// A helper struct for thread_local that uses nullptr initialization as a
// sentinel to prevent guard variable use from being invoked after first
// initialization.
template <class T>
struct Sdf_FastThreadLocalBase
{
    static T &Get() {
        static thread_local T *theTPtr = nullptr;
        if (ARCH_LIKELY(theTPtr)) {
            return *theTPtr;
        }
        static thread_local T theT;
        T *p = &theT;
        theTPtr = p;
        return *p;
    }
};

// Fixed-size scalable pool allocator with 32-bit "handles" returned.  Reserves
// virtual memory in big regions.  It's optimized for per-thread allocations,
// and intended to be used for SdfPath.  The Tag template argument just serves
// as to distinguish one pool instantiation from another with otherwise
// equivalent template arguments.  The ElemSize argument is the size of an
// allocated element in bytes.  It must be at least 4 bytes.  The RegionBits
// argument determines how many contiguous "chunks" of virtual memory the pool
// will use.  A good number for this is 8, meaning we'll have at most 256
// "regions" or "chunks" of contiguous virtual memory, each 2^24 times ElemSize
// bytes in size.  To allocate from reserved chunks, each thread acquires a span
// to hold ElemsPerSpan elements from the range, then uses that to dole out
// individual allocations.  When freed, allocations are placed on a thread-local
// free list, and eventually shared back for use by other threads when the free
// list gets large.
template <class Tag,
          unsigned ElemSize,
          unsigned RegionBits,
          unsigned ElemsPerSpan=16384>
class Sdf_Pool
{
    static_assert(ElemSize >= sizeof(uint32_t),
                  "ElemSize must be at least sizeof(uint32_t)");

public:
    // Number of pool elements per region.
    static constexpr uint64_t ElemsPerRegion = 1ull << (32-RegionBits);

    // Maximum index of an element in a region.
    static constexpr uint32_t MaxIndex = ElemsPerRegion - 1;

    // Mask to extract region number from a handle value.
    static constexpr uint32_t RegionMask = ((1 << RegionBits)-1);

    friend struct Handle;
    // A Handle refers to an item in the pool.  It just wraps around a uint32_t
    // that represents the item's index and the item's region.
    struct Handle {
        constexpr Handle() noexcept = default;
        constexpr Handle(std::nullptr_t) noexcept : value(0) {}
        Handle(unsigned region, uint32_t index) 
            : value((index << RegionBits) | region) {}
        Handle &operator=(Handle const &) = default;
        Handle &operator=(std::nullptr_t) { return *this = Handle(); }
        inline char *GetPtr() const noexcept {
	    ARCH_PRAGMA_PUSH
            ARCH_PRAGMA_MAYBE_UNINITIALIZED
            return Sdf_Pool::_GetPtr(value & RegionMask, value >> RegionBits);
            ARCH_PRAGMA_POP
        }
        static inline Handle GetHandle(char const *ptr) noexcept {
            return Sdf_Pool::_GetHandle(ptr);
        }
        explicit operator bool() const {
            return value != 0;
        }
        inline bool operator ==(Handle const &r) const noexcept {
            return value == r.value;
        }
        inline bool operator !=(Handle const &r) const noexcept {
            return value != r.value;
        }
        inline bool operator <(Handle const &r) const noexcept {
            return value < r.value;
        }
        inline void swap(Handle &r) noexcept {
            std::swap(value, r.value);
        }
        uint32_t value = 0;
    };

private:
    // We maintain per-thread free lists of pool items.
    struct _FreeList {
        inline void Pop() {
            char *p = head.GetPtr();
            Handle *hp = reinterpret_cast<Handle *>(p);
            head = *hp;
            --size;
        }
        inline void Push(Handle h) {
            ++size;
            char *p = h.GetPtr();
            Handle *hp = reinterpret_cast<Handle *>(p);
            *hp = head;
            head = h;
        }
        Handle head;
        size_t size = 0;
    };
    
    // A pool span represents a "chunk" of new pool space that threads allocate
    // from when their free lists are empty.  When both the free list and the
    // pool span are exhausted, a thread will look for a shared free list, or
    // will obtain a new chunk of pool space to use.
    struct _PoolSpan {
        size_t size() const { return endIndex - beginIndex; }
        inline Handle Alloc() {
            return Handle(region, beginIndex++);
        }
        inline bool empty() const {
            return beginIndex == endIndex;
        }
        unsigned region;
        uint32_t beginIndex;
        uint32_t endIndex;
    };

    struct _PerThreadData {
        // Local free-list of elems returned to the pool.
        _FreeList freeList;
        // Contiguous range of reserved but as-yet-unalloc'd space.
        _PoolSpan span;
    };

    // The region state structure represents the global state of the pool.  This
    // is a pool-global structure that is used to reserve new spans of pool data
    // by threads when needed.  See the Reserve() member, and the _ReserveSpan()
    // function that does most of the state manipulation.
    struct _RegionState {
        static constexpr uint32_t LockedState = ~0;
        
        _RegionState() = default;
        constexpr _RegionState(unsigned region, uint32_t index)
            : _state((index << RegionBits) | region) {}
        
        // Make a new state that reserves up to \p num elements.  There must be
        // space left remaining.
        inline _RegionState Reserve(unsigned num) const;

        static constexpr _RegionState GetInitState() {
            return _RegionState(0, 0);
        }

        static constexpr _RegionState GetLockedState() {
            return _RegionState(LockedState, LockedState);
        }
        
        constexpr bool operator==(_RegionState other) const {
            return _state == other._state;
        }
        
        uint32_t GetIndex() const {
            return _state >> RegionBits;
        }
        
        unsigned GetRegion() const {
            return _state & RegionMask;
        }

        bool IsLocked() const {
            return _state == LockedState;
        }
        
        // low RegionBits bits are region id, rest are index.
        uint32_t _state;
    };

public:
    static inline Handle Allocate();
    static inline void Free(Handle h);

private:

    // Given a region id and index, form the pointer into the pool.
    static inline char *_GetPtr(unsigned region, uint32_t index) {
        // Suppress undefined-var-template warnings from clang; _regionStarts
        // is expected to be instantiated in another translation unit via 
        // the SDF_INSTANTIATE_POOL macro.
        ARCH_PRAGMA_PUSH
        ARCH_PRAGMA_UNDEFINED_VAR_TEMPLATE
        return _regionStarts[region] + (index * ElemSize);
        ARCH_PRAGMA_POP
    }
    
    // Given a pointer into the pool, produce its corresponding Handle.  Don't
    // do this unless you really have to, it has to do a bit of a search.
    static inline Handle _GetHandle(char const *ptr) {
        if (ptr) {
            for (unsigned region = 1; region != NumRegions+1; ++region) {
                // Suppress undefined-var-template warnings from clang; _regionStarts
                // is expected to be instantiated in another translation unit via 
                // the SDF_INSTANTIATE_POOL macro.
                ARCH_PRAGMA_PUSH
                ARCH_PRAGMA_UNDEFINED_VAR_TEMPLATE
                uintptr_t start = (uintptr_t)_regionStarts[region];
                ARCH_PRAGMA_POP

                // We rely on modular arithmetic so that if ptr is less than
                // start, the diff will be larger than ElemsPerRegion*ElemSize.
                uintptr_t diff = (uintptr_t)ptr - start;
                if (diff < (uintptr_t)(ElemsPerRegion*ElemSize)) {
                    return Handle(
                        region, static_cast<uint32_t>(diff / ElemSize));
                }
            }
        }
        return nullptr;
    }

    // Try to take a shared free list.
    static bool _TakeSharedFreeList(_FreeList &out) {
        return _sharedFreeLists->try_pop(out);
    }
    
    // Give a free list to be shared by other threads.
    static void _ShareFreeList(_FreeList &in) {
        _sharedFreeLists->push(in);
        in = {};
    }

    // Reserve a new span of pool space.
    static inline void _ReserveSpan(_PoolSpan &out);

    static constexpr int NumRegions = 1 << RegionBits;

    using _ThreadData = Sdf_FastThreadLocalBase<_PerThreadData>;
    SDF_API static _ThreadData _threadData;
    SDF_API static char *_regionStarts[NumRegions+1];
    SDF_API static std::atomic<_RegionState> _regionState;
    SDF_API static TfStaticData<tbb::concurrent_queue<_FreeList>> _sharedFreeLists;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_POOL_H
