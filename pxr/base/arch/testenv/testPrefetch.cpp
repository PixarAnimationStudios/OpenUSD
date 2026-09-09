//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/align.h"
#include "pxr/base/arch/error.h"
#include "pxr/base/arch/prefetch.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>

PXR_NAMESPACE_USING_DIRECTIVE

// A prefetch has no observable effect, so we cannot check that ArchPrefetch()
// prefetched anything.  What we can check is which cache lines it selects, by
// running the same line-walking code over a recording callback instead of over
// the prefetch instruction.  The rest of the test just instantiates the public
// surface, so that a combination which fails to compile fails the build.

static constexpr size_t LineSize = ARCH_CACHE_LINE_SIZE;

#define VERIFY(cond, ...)                                               \
    if (!(cond)) {                                                      \
        printf("failed: %s: ", #cond);                                  \
        printf(__VA_ARGS__);                                            \
        printf("\n");                                                   \
        ARCH_ERROR("testPrefetch failed");                              \
    }

// A buffer big enough to place any tested range at any tested offset, aligned
// past the largest alignment we test, so that `buf + off` really does have
// alignment `off` when `off` is a multiple of it.
static constexpr size_t MaxAlign = 4 * LineSize;
static constexpr size_t BufSize = 8 * LineSize;
alignas(MaxAlign) static char buf[BufSize];

// Element types: an awkward size that neither divides a line nor is a power of
// two, one larger than a line, one exactly a line and aligned to it, and one
// that is both larger than a line and under-aligned, so it always straddles.
struct Three { char c[3]; };
struct Big { char c[3 * LineSize / 2]; };
struct alignas(LineSize) OverAligned { char c[LineSize]; };
struct Straddler { double d[LineSize / 4 + 1]; };

// Records the lines Arch_ForEachPrefetchLine() selects.
struct Recorder
{
    void operator()(void const *line) {
        VERIFY(count < MaxLines, "recorded too many lines");
        lines[count++] = static_cast<char const *>(line);
    }
    static constexpr size_t MaxLines = BufSize / LineSize + 2;
    char const *lines[MaxLines] = {};
    size_t count = 0;
};

// Check the line selection for `[buf + off, buf + off + Size)` against an
// independently computed reference: the lines must be exactly the consecutive
// line-aligned addresses covering that byte range, and no others.
template <size_t Size, size_t Align>
static void
TestRange(size_t off)
{
    char const * const addr = buf + off;

    Recorder rec;
    Arch_ForEachPrefetchLine<Size, Align>(addr, rec);

    uintptr_t const a = reinterpret_cast<uintptr_t>(addr);
    char const * const expectFirst =
        reinterpret_cast<char const *>(a & ~uintptr_t(LineSize - 1));
    size_t const expectCount = Size == 0 ? 0 :
        ((a & (LineSize - 1)) + Size + LineSize - 1) / LineSize;

    VERIFY(rec.count == expectCount,
           "Size=%zu Align=%zu off=%zu: got %zu lines, want %zu",
           Size, Align, off, rec.count, expectCount);

    for (size_t i = 0; i != rec.count; ++i) {
        VERIFY(rec.lines[i] == expectFirst + i * LineSize,
               "Size=%zu Align=%zu off=%zu: line %zu is %p, want %p",
               Size, Align, off, i,
               static_cast<void const *>(rec.lines[i]),
               static_cast<void const *>(expectFirst + i * LineSize));
    }
}

// Check ArchPrefetchRange()'s line selection the same way.  Because the length
// is a runtime value this costs no template instantiations, so it can sweep
// every (offset, length) pair in buf rather than a sampling of them.
//
// The cast to `void const *` is important: it selects the byte-counted
// overload.  Passing `buf + off` directly would route to the object-counted one
// instead.
static void
TestRuntimeRange(size_t off, size_t numBytes)
{
    char const * const addr = buf + off;

    Recorder rec;
    Arch_ForEachPrefetchLine(static_cast<void const *>(addr), numBytes, rec);

    uintptr_t const a = reinterpret_cast<uintptr_t>(addr);
    char const * const expectFirst =
        reinterpret_cast<char const *>(a & ~uintptr_t(LineSize - 1));
    size_t const expectCount = numBytes == 0 ? 0 :
        ((a & (LineSize - 1)) + numBytes + LineSize - 1) / LineSize;

    VERIFY(rec.count == expectCount,
           "runtime off=%zu numBytes=%zu: got %zu lines, want %zu",
           off, numBytes, rec.count, expectCount);

    for (size_t i = 0; i != rec.count; ++i) {
        VERIFY(rec.lines[i] == expectFirst + i * LineSize,
               "runtime off=%zu numBytes=%zu: line %zu is %p, want %p",
               off, numBytes, i,
               static_cast<void const *>(rec.lines[i]),
               static_cast<void const *>(expectFirst + i * LineSize));
    }
}

// Check the object-counted overload: `count` objects of T must cover exactly
// the lines that `count * sizeof(T)` bytes do.  This is what catches a missing
// or wrong sizeof() in the unit conversion.
template <class T>
static void
TestTypedRange(size_t off, size_t count)
{
    T const * const tp = reinterpret_cast<T const *>(buf + off);

    Recorder rec;
    Arch_ForEachPrefetchLine(tp, count, rec);

    size_t const numBytes = count * sizeof(T);
    uintptr_t const a = reinterpret_cast<uintptr_t>(tp);
    char const * const expectFirst =
        reinterpret_cast<char const *>(a & ~uintptr_t(LineSize - 1));
    size_t const expectCount = numBytes == 0 ? 0 :
        ((a & (LineSize - 1)) + numBytes + LineSize - 1) / LineSize;

    VERIFY(rec.count == expectCount,
           "typed sizeof(T)=%zu off=%zu count=%zu (%zu bytes): "
           "got %zu lines, want %zu",
           sizeof(T), off, count, numBytes, rec.count, expectCount);

    for (size_t i = 0; i != rec.count; ++i) {
        VERIFY(rec.lines[i] == expectFirst + i * LineSize,
               "typed sizeof(T)=%zu off=%zu count=%zu: line %zu is %p, "
               "want %p", sizeof(T), off, count, i,
               static_cast<void const *>(rec.lines[i]),
               static_cast<void const *>(expectFirst + i * LineSize));
    }
}

// Sweep every legal (offset, count) pair for T within buf.
template <class T>
static void
TestTypedSweep()
{
    static_assert(alignof(T) <= MaxAlign, "buf is not aligned enough for T");
    for (size_t off = 0; off < BufSize; off += alignof(T)) {
        for (size_t n = 0; off + n * sizeof(T) <= BufSize; ++n) {
            TestTypedRange<T>(off, n);
        }
    }
}

// The two variants must agree exactly.  This is a stronger check than either
// against the reference alone, since they derive the line count differently:
// ArchPrefetch() from a compile-time minimum plus a straddle test,
// ArchPrefetchRange() from the first and last byte's lines.
template <size_t Size, size_t Align>
static void
TestVariantsAgree(size_t off)
{
    Recorder ct, rt;
    Arch_ForEachPrefetchLine<Size, Align>(buf + off, ct);
    Arch_ForEachPrefetchLine(static_cast<void const *>(buf + off), Size, rt);

    VERIFY(ct.count == rt.count,
           "Size=%zu Align=%zu off=%zu: compile-time took %zu lines, "
           "runtime took %zu", Size, Align, off, ct.count, rt.count);
    for (size_t i = 0; i != ct.count; ++i) {
        VERIFY(ct.lines[i] == rt.lines[i],
               "Size=%zu Align=%zu off=%zu: line %zu differs", Size, Align,
               off, i);
    }
}

// Walk every offset in buf that honors the Align promise.
template <size_t Size, size_t Align>
static void
TestSizeAlign()
{
    static_assert(Align <= MaxAlign, "buf is not aligned enough for Align");
    static_assert(Size <= BufSize, "buf is not big enough for Size");
    for (size_t off = 0; off + Size <= BufSize; off += Align) {
        TestRange<Size, Align>(off);
        TestVariantsAgree<Size, Align>(off);
    }
}

// Sweep every size from 0 through Size at the given alignment.
//
// Recursive rather than a fold over an index_sequence: a fold needs one
// expression element per size, so at ARCH_CACHE_LINE_SIZE 256 or above this
// would exceed clang's default expression nesting limit and fail to compile.
template <size_t Align, size_t Size>
static void
TestSizesUpTo()
{
    TestSizeAlign<Size, Align>();
    if constexpr (Size != 0) {
        TestSizesUpTo<Align, Size - 1>();
    }
}

int main()
{
    static_assert(LineSize != 0 && (LineSize & (LineSize - 1)) == 0,
                  "ARCH_CACHE_LINE_SIZE must be a power of two");

    // Every size from 0 through two past a line boundary -- that covers the
    // exact-multiple boundaries and both sides of them -- at every power-of-two
    // alignment from a byte up to two lines.
    static constexpr size_t MaxSweep = LineSize + 2;
    TestSizesUpTo<1, MaxSweep>();
    TestSizesUpTo<2, MaxSweep>();
    TestSizesUpTo<4, MaxSweep>();
    TestSizesUpTo<8, MaxSweep>();
    TestSizesUpTo<16, MaxSweep>();
    TestSizesUpTo<LineSize / 2, MaxSweep>();
    TestSizesUpTo<LineSize, MaxSweep>();
    TestSizesUpTo<2 * LineSize, MaxSweep>();

    // Sizes spanning more lines, to exercise deeper multi-line loops and to
    // check that alignment at or above a line suppresses the straddle check
    // without dropping a line.
    TestSizeAlign<2 * LineSize - 1, 1>();
    TestSizeAlign<2 * LineSize, 1>();
    TestSizeAlign<2 * LineSize + 1, 1>();
    TestSizeAlign<2 * LineSize + 1, 8>();
    TestSizeAlign<2 * LineSize + 1, LineSize>();
    TestSizeAlign<5 * LineSize, 1>();
    TestSizeAlign<5 * LineSize + 1, 1>();
    TestSizeAlign<5 * LineSize - 1, 8>();
    TestSizeAlign<8 * LineSize, LineSize>();

    // ArchPrefetchRange(): every offset against every length that fits, which
    // is exhaustive over buf rather than the sampled sweep above.
    for (size_t off = 0; off != BufSize; ++off) {
        for (size_t numBytes = 0; off + numBytes <= BufSize; ++numBytes) {
            TestRuntimeRange(off, numBytes);
        }
    }

    // ArchPrefetchRange() with an object count.  Sizes chosen to include one
    // byte (where an object count and a byte count coincide, so a missing
    // sizeof would hide), a size that is neither a power of two nor a divisor
    // of a line, one that exactly divides a line, one larger than a line, and
    // an over-aligned one.
    TestTypedSweep<char>();
    TestTypedSweep<Three>();
    TestTypedSweep<float>();
    TestTypedSweep<double>();
    TestTypedSweep<Big>();
    TestTypedSweep<OverAligned>();

    // Instantiate the whole public surface: every access/locality pair, plus
    // the defaulted template arguments.  Prefetches have no observable effect,
    // so this is here to fail the build if any combination does not compile.
    ArchPrefetch<>(buf);
    ArchPrefetch<0>(buf);
    ArchPrefetch<1>(buf);
    ArchPrefetch<1, 1>(buf);
    ArchPrefetch<LineSize + 1, 4, ArchPrefetchAccess::Read,
                 ArchPrefetchLocality::None>(buf + 4);
    ArchPrefetch<LineSize + 1, 4, ArchPrefetchAccess::Read,
                 ArchPrefetchLocality::L3>(buf + 4);
    ArchPrefetch<LineSize + 1, 4, ArchPrefetchAccess::Read,
                 ArchPrefetchLocality::L2>(buf + 4);
    ArchPrefetch<LineSize + 1, 4, ArchPrefetchAccess::Read,
                 ArchPrefetchLocality::L1>(buf + 4);
    ArchPrefetch<LineSize + 1, 4, ArchPrefetchAccess::Write,
                 ArchPrefetchLocality::None>(buf + 4);
    ArchPrefetch<LineSize + 1, 4, ArchPrefetchAccess::Write,
                 ArchPrefetchLocality::L3>(buf + 4);
    ArchPrefetch<LineSize + 1, 4, ArchPrefetchAccess::Write,
                 ArchPrefetchLocality::L2>(buf + 4);
    ArchPrefetch<LineSize + 1, 4, ArchPrefetchAccess::Write,
                 ArchPrefetchLocality::L1>(buf + 4);

    // Byte-counted: an untyped pointer.
    ArchPrefetchRange(static_cast<void const *>(buf), 0);
    ArchPrefetchRange(static_cast<void const *>(buf), 1);
    ArchPrefetchRange(static_cast<void const *>(buf + 4), LineSize + 1);
    ArchPrefetchRange(static_cast<void const *>(buf), BufSize);

    // Object-counted: a typed pointer, including the vector-like spelling this
    // overload exists for, and a non-const pointer deducing through T const *.
    Big const *bigs = reinterpret_cast<Big const *>(buf);
    ArchPrefetchRange(bigs, 0);
    ArchPrefetchRange(bigs, 2);
    ArchPrefetchRange<ArchPrefetchAccess::Write>(bigs, 2);
    ArchPrefetchRange<ArchPrefetchAccess::Write,
                      ArchPrefetchLocality::None>(bigs, 2);
    ArchPrefetchRange(buf, BufSize);
    ArchPrefetchRange(reinterpret_cast<double *>(buf),
                      BufSize / sizeof(double));

    ArchPrefetchRange<ArchPrefetchAccess::Read,
                      ArchPrefetchLocality::None>(buf + 4, LineSize + 1);
    ArchPrefetchRange<ArchPrefetchAccess::Read,
                      ArchPrefetchLocality::L3>(buf + 4, LineSize + 1);
    ArchPrefetchRange<ArchPrefetchAccess::Read,
                      ArchPrefetchLocality::L2>(buf + 4, LineSize + 1);
    ArchPrefetchRange<ArchPrefetchAccess::Read,
                      ArchPrefetchLocality::L1>(buf + 4, LineSize + 1);
    ArchPrefetchRange<ArchPrefetchAccess::Write,
                      ArchPrefetchLocality::None>(buf + 4, LineSize + 1);
    ArchPrefetchRange<ArchPrefetchAccess::Write,
                      ArchPrefetchLocality::L3>(buf + 4, LineSize + 1);
    ArchPrefetchRange<ArchPrefetchAccess::Write,
                      ArchPrefetchLocality::L2>(buf + 4, LineSize + 1);
    ArchPrefetchRange<ArchPrefetchAccess::Write,
                      ArchPrefetchLocality::L1>(buf + 4, LineSize + 1);

    Arch_PrefetchOneLine<ArchPrefetchAccess::Read,
                         ArchPrefetchLocality::L1>(buf);
    Arch_PrefetchOneLine<ArchPrefetchAccess::Write,
                         ArchPrefetchLocality::None>(buf);

    // The type-driven wrappers, over a one-byte type, a line-sized line-aligned
    // one, and an under-aligned one bigger than a line.
    char small {};
    OverAligned aligned {};
    Straddler straddler {};

    ArchPrefetchRead(&small);
    ArchPrefetchRead<ArchPrefetchLocality::None>(&small);
    ArchPrefetchRead<ArchPrefetchLocality::L1>(&aligned);
    ArchPrefetchRead<ArchPrefetchLocality::L3>(&straddler);
    ArchPrefetchWrite(&small);
    ArchPrefetchWrite<ArchPrefetchLocality::None>(&aligned);
    ArchPrefetchWrite<ArchPrefetchLocality::L1>(&straddler);

    // A line-sized, line-aligned type must take exactly one line -- the case
    // the Align argument exists to optimize.  An under-aligned type spanning
    // more than a line must take its span plus the straddle.
    Recorder rec;
    Arch_ForEachPrefetchLine<sizeof(OverAligned),
                             alignof(OverAligned)>(&aligned, rec);
    VERIFY(rec.count == 1, "aligned line-sized type took %zu lines", rec.count);

    Recorder str;
    Arch_ForEachPrefetchLine<sizeof(Straddler), alignof(Straddler)>(
        &straddler, str);
    size_t const expect =
        ((reinterpret_cast<uintptr_t>(&straddler) & (LineSize - 1)) +
         sizeof(Straddler) + LineSize - 1) / LineSize;
    VERIFY(str.count == expect, "straddling type took %zu lines, want %zu",
           str.count, expect);

    printf("OK\n");
    return 0;
}
