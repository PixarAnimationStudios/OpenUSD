//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_ARCH_PREFETCH_H
#define PXR_BASE_ARCH_PREFETCH_H

/// \file arch/prefetch.h
/// \ingroup group_arch_Memory
/// Memory prefetch.
///
/// Use these to hide latency when you know you're likely going to read/write an
/// address soon but have the ability to spend the latency cycles doing
/// independent work.

#include "pxr/pxr.h"

#include "pxr/base/arch/align.h"
#include "pxr/base/arch/defines.h"

#include <cstddef>
#include <cstdint>

#if defined(ARCH_COMPILER_MSVC)
#include <intrin.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

/// \addtogroup group_arch_Memory
///@{

/// The kind of access a prefetch is preparing for.  These follow gcc's
/// __builtin_prefetch's 'rw' parameter.  If you don't know what to use, just
/// use Read.  Most modern CPUs treat them the same regardless.
enum class ArchPrefetchAccess {
    Read  = 0,
    Write = 1
};

/// How much temporal locality you expect on the address after the fetch.  For
/// pure-streaming, single access and done, use `None`.  For a cache where you
/// expect multiple repeat hits, use higher levels like L2 or L1.
enum class ArchPrefetchLocality {
    None = 0,
    L3   = 1,
    L2   = 2,
    L1   = 3
};

// Prefetch the single cache line containing addr.  Implementation helper for
// ArchPrefetch() and ArchPrefetchRange().
template <ArchPrefetchAccess Access, ArchPrefetchLocality Locality>
void Arch_PrefetchOneLine(void const *addr) noexcept
{
#if defined(ARCH_COMPILER_GCC)   ||             \
    defined(ARCH_COMPILER_CLANG) ||             \
    defined(ARCH_COMPILER_ICC)

    // The enum values are crafted to match __builtin_prefetch's expectations.
    // The casts are required: clang demands integer constants here and will not
    // convert the enumerators implicitly.
    __builtin_prefetch(addr, static_cast<int>(Access),
                       static_cast<int>(Locality));

#elif defined(ARCH_COMPILER_MSVC) && defined(ARCH_CPU_ARM)

    __prefetch(addr); // No locality/rw options for arm.

#elif defined(ARCH_COMPILER_MSVC) && defined(ARCH_CPU_INTEL)

    constexpr int hint =
        Locality == ArchPrefetchLocality::None ? _MM_HINT_NTA :
        Locality == ArchPrefetchLocality::L3   ? _MM_HINT_T2  :
        Locality == ArchPrefetchLocality::L2   ? _MM_HINT_T1  : _MM_HINT_T0;
    // No rw option for msvc.
    _mm_prefetch(reinterpret_cast<char const *>(addr), hint);

#else
    // Unsupported, do nothing.
    (void)addr;
#endif
}

// Invoke fn with the address of each cache line that the byte range [addr,
// addr+Size) occupies, in increasing address order.  This is factored out of
// ArchPrefetch() so that tests can observe the line selection, which is
// otherwise unobservable.  It costs nothing: fn inlines away, leaving codegen
// identical to prefetching inline.
template <size_t Size, size_t Align, class Fn>
void
Arch_ForEachPrefetchLine(void const *addr, Fn &&fn) noexcept
{
    constexpr size_t LineSize = ARCH_CACHE_LINE_SIZE;

    static_assert(Align != 0 && (Align & (Align - 1)) == 0,
                  "Align must be a power of two");
    static_assert((LineSize & (LineSize - 1)) == 0,
                  "ARCH_CACHE_LINE_SIZE must be a power of two");

    constexpr size_t MinLines = (Size + LineSize - 1) / LineSize;
    constexpr uintptr_t LineMask = ~static_cast<uintptr_t>(LineSize - 1);

    // The worst-case starting offset within a cache line, given alignment
    // Align, is (ARCH_CACHE_LINE_SIZE - Align) -- the last aligned position
    // before a line boundary.  A straddle into an extra line is only possible
    // if that worst-case layout pushes the final byte past MinLines lines.
    // Note an empty range never occupies a line, straddling or otherwise.
    constexpr bool MightStraddle =
        Size != 0 && (Align < LineSize) &&
        ((LineSize - Align) + Size > MinLines * LineSize);

    char const *ptr = reinterpret_cast<char const *>(
        reinterpret_cast<uintptr_t>(addr) & LineMask);

    // The compiler should unroll this loop of constexpr iterations.
    for (size_t i = 0; i != MinLines; ++i) {
        fn(ptr);
        ptr += LineSize;
    }

    // If addr + Size spills to an additional line, prefetch it.
    if constexpr (MightStraddle) {
        if (reinterpret_cast<char const *>(addr) + Size > ptr) {
            fn(ptr);
        }
    }
}

// As above, for a length that is only known at runtime.  Deriving the line
// count from the first and last byte's lines is uniform -- there is no straddle
// case to special-case, and so no use for an alignment guarantee.
//
// Requires that `[addr, addr+numBytes)` not wrap the address space, which holds
// for any real object.
template <class Fn>
void
Arch_ForEachPrefetchLine(void const *addr, size_t numBytes, Fn &&fn) noexcept
{
    constexpr size_t LineSize = ARCH_CACHE_LINE_SIZE;
    constexpr uintptr_t LineMask = ~static_cast<uintptr_t>(LineSize - 1);

    if (numBytes == 0) {
        return;
    }

    uintptr_t const a = reinterpret_cast<uintptr_t>(addr);
    uintptr_t const begin = a & LineMask;
    uintptr_t const last = (a + numBytes - 1) & LineMask;

    // One line past the last.  A range ending in the topmost cache line wraps
    // this to zero, but `p` wraps to zero on the same step -- both walk aligned
    // addresses from `begin` -- so the `!=` test still terminates.
    //
    // Walking to a sentinel rather than counting iterations matters: given a
    // trip count, clang unrolls this loop eightfold with a remainder loop,
    // which is a lot of code to inline at every call site that prefetches a
    // runtime-length range.
    uintptr_t const end = last + LineSize;

    uintptr_t p = begin;
    do {
        fn(reinterpret_cast<char const *>(p));
        p += LineSize;
    } while (p != end);
}

// As above, for `count` objects of type T.  Overload resolution sends a typed
// pointer here and an untyped one to the overload above, so that a count is
// always in the units the pointer implies.
template <class T, class Fn>
void
Arch_ForEachPrefetchLine(T const *addr, size_t count, Fn &&fn) noexcept
{
    Arch_ForEachPrefetchLine(static_cast<void const *>(addr),
                             count * sizeof(T), fn);
}

/// Prefetch the cache lines that `[addr, addr+Size)` occupy.  The template
/// argument \p Align (which must be a power of two) states the alignment the
/// caller guarantees for \p addr.  This can avoid a runtime check for ranges
/// that could straddle cache lines when geometrically impossible due to
/// alignment.  A zero \p Size prefetches nothing.
///
/// Use ArchPrefetchRange() if the length is not a compile-time constant.
template <size_t Size = 1,
          size_t Align = 1,
          ArchPrefetchAccess Access = ArchPrefetchAccess::Read,
          ArchPrefetchLocality Locality = ArchPrefetchLocality::L2>
void
ArchPrefetch(void const *addr) noexcept
{
    Arch_ForEachPrefetchLine<Size, Align>(addr, [](void const *line) {
        Arch_PrefetchOneLine<Access, Locality>(line);
    });
}

/// Prefetch the cache lines that `[addr, addr+numBytes)` occupy, where \p
/// numBytes is a runtime value.  A zero \p numBytes prefetches nothing.
///
/// There is no alignment parameter: unlike ArchPrefetch(), this cannot fold the
/// line count at compile time, so knowing the alignment of \p addr would not
/// save it any work.  Prefer ArchPrefetch() when the length is a constant -- it
/// unrolls to a straight run of prefetch instructions.
template <ArchPrefetchAccess Access = ArchPrefetchAccess::Read,
          ArchPrefetchLocality Locality = ArchPrefetchLocality::L2>
void
ArchPrefetchRange(void const *addr, size_t numBytes) noexcept
{
    Arch_ForEachPrefetchLine(addr, numBytes, [](void const *line) {
        Arch_PrefetchOneLine<Access, Locality>(line);
    });
}

/// Prefetch the cache lines that the \p count objects of type \p T starting at
/// \p addr occupy, as in `ArchPrefetchRange(vec.data(), vec.size())`.  A zero
/// \p count prefetches nothing.
///
/// Note the unit of the second argument follows the pointer, matching the usual
/// C++ conventions: a `void const *` takes a byte count, as `memcpy()` does,
/// and a typed pointer takes an object count, as `std::copy_n()` does.
///
/// \p count objects must actually fit in memory -- `count * sizeof(T)` is not
/// checked for overflow.
template <ArchPrefetchAccess Access = ArchPrefetchAccess::Read,
          ArchPrefetchLocality Locality = ArchPrefetchLocality::L2,
          class T>
void
ArchPrefetchRange(T const *addr, size_t count) noexcept
{
    Arch_ForEachPrefetchLine(addr, count, [](void const *line) {
        Arch_PrefetchOneLine<Access, Locality>(line);
    });
}

/// Prefetch for reading the object at \p addr with given locality.  The type \p
/// T must be complete.  Prefetches the number of cache lines that \p T occupies
/// at \p addr.
template <ArchPrefetchLocality Locality = ArchPrefetchLocality::L2, class T>
void
ArchPrefetchRead(T const *addr) noexcept
{
    ArchPrefetch<sizeof(T), alignof(T),
                 ArchPrefetchAccess::Read, Locality>(addr);
}

/// Prefetch for writing the object at \p addr with given locality.  The type \p
/// T must be complete.  Prefetches the number of cache lines that \p T occupies
/// at \p addr.
template <ArchPrefetchLocality Locality = ArchPrefetchLocality::L2, class T>
void
ArchPrefetchWrite(T const *addr) noexcept
{
    ArchPrefetch<sizeof(T), alignof(T),
                 ArchPrefetchAccess::Write, Locality>(addr);
}

///@}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_ARCH_PREFETCH_H
