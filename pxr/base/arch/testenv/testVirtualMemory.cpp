//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/error.h"
#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/arch/virtualMemory.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#if defined(ARCH_OS_WINDOWS)
#  include <Windows.h>
#else
#  include <csetjmp>
#  include <csignal>
#endif

PXR_NAMESPACE_USING_DIRECTIVE

// Checking that a protection took effect means performing an access that must
// fail, and recovering from it.  Windows structured exception handling does
// this directly.  On POSIX we trap the fault signal and siglongjmp() out of the
// handler.
//
// Every other check in this file only inspects return values, which cannot
// distinguish a working implementation from one that reports success and does
// nothing -- so this mechanism is what gives the test its teeth.
#if defined(ARCH_OS_WINDOWS) || defined(ARCH_OS_LINUX) || \
    defined(ARCH_OS_DARWIN)
#define TEST_HAS_FAULT_TRAP
#endif

#if defined(ARCH_OS_WINDOWS)

// The two probes below hold no local objects, so they do not run afoul of
// MSVC's prohibition on __try in a function that requires object unwinding.
// That is also why the probes are fixed single-byte accesses rather than a
// macro wrapping an arbitrary expression.
static bool FaultsOnRead(char volatile *p)
{
    __try { (void)*p; return false; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return true; }
}

static bool FaultsOnWrite(char volatile *p)
{
    __try { *p = 1; return false; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return true; }
}

#elif defined(TEST_HAS_FAULT_TRAP)

static sigjmp_buf faultJmp;
static struct sigaction oldSegv, oldBus;

static void OnFault(int) { siglongjmp(faultJmp, 1); }

static void InstallFaultHandler()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = OnFault;
    sigemptyset(&sa.sa_mask);
    // MacOS reports some protection violations as SIGBUS rather than SIGSEGV.
    ARCH_AXIOM(sigaction(SIGSEGV, &sa, &oldSegv) == 0);
    ARCH_AXIOM(sigaction(SIGBUS, &sa, &oldBus) == 0);
}

static void RemoveFaultHandler()
{
    ARCH_AXIOM(sigaction(SIGSEGV, &oldSegv, nullptr) == 0);
    ARCH_AXIOM(sigaction(SIGBUS, &oldBus, nullptr) == 0);
}

// The savemask argument to sigsetjmp() must be nonzero.  The fault signal is
// blocked on entry to the handler, and only a siglongjmp() to an environment
// saved with the mask restores it.  With savemask 0 the first fault is caught
// and the second one kills the process.
static bool FaultsOnRead(char volatile *p)
{
    InstallFaultHandler();
    bool faulted = sigsetjmp(faultJmp, 1) != 0;
    if (!faulted) {
        (void)*p;
    }
    RemoveFaultHandler();
    return faulted;
}

static bool FaultsOnWrite(char volatile *p)
{
    InstallFaultHandler();
    bool faulted = sigsetjmp(faultJmp, 1) != 0;
    if (!faulted) {
        *p = 1;
    }
    RemoveFaultHandler();
    return faulted;
}

#endif // TEST_HAS_FAULT_TRAP

// Note that EXPECT_WRITABLE() stores a byte, so use it only where the contents
// do not matter afterward.
#if defined(TEST_HAS_FAULT_TRAP)
#define EXPECT_READABLE(p)     ARCH_AXIOM(!FaultsOnRead(p))
#define EXPECT_NOT_READABLE(p) ARCH_AXIOM(FaultsOnRead(p))
#define EXPECT_WRITABLE(p)     ARCH_AXIOM(!FaultsOnWrite(p))
#define EXPECT_NOT_WRITABLE(p) ARCH_AXIOM(FaultsOnWrite(p))
#else
// The access itself has to be skipped, not just the check: without a trap it
// would take the process down.
#define EXPECT_READABLE(p)     ((void)0)
#define EXPECT_NOT_READABLE(p) ((void)0)
#define EXPECT_WRITABLE(p)     ((void)0)
#define EXPECT_NOT_WRITABLE(p) ((void)0)
#endif

// Enough pages to leave uncommitted gaps between the ranges under test, and to
// exceed Windows' 64 KB allocation granularity so that a reservation is not
// rounded up to something much larger than requested.
static const size_t NUM_PAGES = 32;

static void TestPageSize()
{
    // Both this test and ArchSetMemoryProtection()'s rounding derive a mask
    // from this value, so a non-positive or non-power-of-two page size would
    // silently corrupt every address computation rather than fail visibly.
    const int pageSize = ArchGetPageSize();
    ARCH_AXIOM(pageSize > 0);
    ARCH_AXIOM((pageSize & (pageSize - 1)) == 0);
}

static void TestReserve()
{
    const size_t pageSize = ArchGetPageSize();
    const size_t numBytes = NUM_PAGES * pageSize;

    char *base = static_cast<char *>(ArchReserveVirtualMemory(numBytes));
    ARCH_AXIOM(base);

    // A reservation begins on a page boundary, so rounding the base address
    // down is a no-op.  Windows reserves at the larger allocation granularity,
    // which is a multiple of the page size, so this holds there too.
    ARCH_AXIOM(reinterpret_cast<uintptr_t>(base) % pageSize == 0);

    // Reserved memory is inaccessible until committed -- the whole point of
    // separating the two calls.
    EXPECT_NOT_READABLE(base);
    EXPECT_NOT_WRITABLE(base);
    EXPECT_NOT_READABLE(base + (NUM_PAGES - 1) * pageSize);

    ARCH_AXIOM(ArchFreeVirtualMemory(base, numBytes));
}

static void TestCommit()
{
    const size_t pageSize = ArchGetPageSize();
    const size_t numBytes = NUM_PAGES * pageSize;

    char *base = static_cast<char *>(ArchReserveVirtualMemory(numBytes));
    ARCH_AXIOM(base);

    ARCH_AXIOM(ArchCommitVirtualMemoryRange(base, pageSize));

    // Check accessibility through the trap before touching the page directly,
    // so that a commit which reports success without doing anything fails here
    // rather than taking the process down on the next line.
    EXPECT_READABLE(base);

    // Freshly committed memory reads as zero and is writable throughout.
    for (size_t i = 0; i != pageSize; ++i) {
        ARCH_AXIOM(base[i] == 0);
    }
    memset(base, 0xAB, pageSize);
    for (size_t i = 0; i != pageSize; ++i) {
        ARCH_AXIOM(static_cast<unsigned char>(base[i]) == 0xAB);
    }

    // Committing one page must not make the next one accessible.
    EXPECT_NOT_READABLE(base + pageSize);

    // Recommitting a fully committed range is not an error and preserves its
    // contents.
    ARCH_AXIOM(ArchCommitVirtualMemoryRange(base, pageSize));
    ARCH_AXIOM(static_cast<unsigned char>(base[0]) == 0xAB);

    // Nor is committing a range that is only partly committed.
    ARCH_AXIOM(ArchCommitVirtualMemoryRange(base, 3 * pageSize));
    ARCH_AXIOM(static_cast<unsigned char>(base[0]) == 0xAB);
    base[2 * pageSize] = 1;
    EXPECT_NOT_READABLE(base + 3 * pageSize);

    ARCH_AXIOM(ArchFreeVirtualMemory(base, numBytes));
}

// Commit granularity is a page, and the range covers exactly the pages it
// touches -- no more.  The implementation reaches that by rounding start down
// to a page boundary and adding the same offset to the length, so the two
// adjustments cancel at the far end.
static void TestCommitPageCoverage()
{
    const size_t pageSize = ArchGetPageSize();
    const size_t numBytes = NUM_PAGES * pageSize;

    char *base = static_cast<char *>(ArchReserveVirtualMemory(numBytes));
    ARCH_AXIOM(base);

    // One byte inside page 4 commits page 4, and only page 4.
    ARCH_AXIOM(ArchCommitVirtualMemoryRange(base + 4 * pageSize + 17, 1));
    EXPECT_NOT_READABLE(base + 3 * pageSize);
    EXPECT_WRITABLE(base + 4 * pageSize);
    EXPECT_NOT_READABLE(base + 5 * pageSize);

    // A page-sized request starting mid-page touches two pages, so alignment
    // rather than size is what keeps a range off its neighbors.
    ARCH_AXIOM(ArchCommitVirtualMemoryRange(base + 8 * pageSize + 17,
                                            pageSize));
    EXPECT_NOT_READABLE(base + 7 * pageSize);
    EXPECT_WRITABLE(base + 8 * pageSize);
    EXPECT_WRITABLE(base + 9 * pageSize);
    EXPECT_NOT_READABLE(base + 10 * pageSize);

    // A range ending exactly on a page boundary must not pull in the page after
    // it.  This is the case that separates "every page the range touches" from
    // "round the length up by a whole page", which the two cases above cannot
    // tell apart.
    ARCH_AXIOM(ArchCommitVirtualMemoryRange(base + 12 * pageSize + 17,
                                            pageSize - 17));
    EXPECT_NOT_READABLE(base + 11 * pageSize);
    EXPECT_WRITABLE(base + 12 * pageSize);
    EXPECT_NOT_READABLE(base + 13 * pageSize);

    ARCH_AXIOM(ArchFreeVirtualMemory(base, numBytes));
}

static void TestProtection()
{
    const size_t pageSize = ArchGetPageSize();
    const size_t numBytes = NUM_PAGES * pageSize;

    char *base = static_cast<char *>(ArchReserveVirtualMemory(numBytes));
    ARCH_AXIOM(base);
    ARCH_AXIOM(ArchCommitVirtualMemoryRange(base, pageSize));
    EXPECT_WRITABLE(base);
    base[0] = 42;

    // Read-only: reads still work, writes fault.
    ARCH_AXIOM(ArchSetMemoryProtection(base, pageSize, ArchProtectReadOnly));
    EXPECT_READABLE(base);
    ARCH_AXIOM(base[0] == 42);
    EXPECT_NOT_WRITABLE(base);

    // No access: neither works.
    ARCH_AXIOM(ArchSetMemoryProtection(base, pageSize, ArchProtectNoAccess));
    EXPECT_NOT_READABLE(base);
    EXPECT_NOT_WRITABLE(base);

    // Back to read/write.  Changing protection does not disturb the contents;
    // in particular ArchProtectNoAccess does not release the pages, so 42 is
    // still there.
    ARCH_AXIOM(ArchSetMemoryProtection(base, pageSize, ArchProtectReadWrite));
    ARCH_AXIOM(base[0] == 42);
    base[0] = 43;
    ARCH_AXIOM(base[0] == 43);

    // ArchProtectReadWriteCopy is deliberately not exercised against a
    // reservation.  It maps to PAGE_WRITECOPY on Windows, which applies to a
    // view of a file mapping opened with FILE_MAP_COPY rather than to memory
    // from VirtualAlloc().  See the note in virtualMemory.h.

    ARCH_AXIOM(ArchFreeVirtualMemory(base, numBytes));
}

// ArchSetMemoryProtection() covers pages the same way
// ArchCommitVirtualMemoryRange() does.
static void TestProtectionPageCoverage()
{
    const size_t pageSize = ArchGetPageSize();
    const size_t numBytes = NUM_PAGES * pageSize;

    char *base = static_cast<char *>(ArchReserveVirtualMemory(numBytes));
    ARCH_AXIOM(base);
    ARCH_AXIOM(ArchCommitVirtualMemoryRange(base, 2 * pageSize));
    EXPECT_WRITABLE(base);
    EXPECT_WRITABLE(base + pageSize);
    base[0] = 42;
    base[pageSize] = 42;

    // One byte inside page 0 protects all of page 0, and page 1 is untouched
    // because the range does not reach into it.
    ARCH_AXIOM(ArchSetMemoryProtection(base + 17, 1, ArchProtectReadOnly));
    EXPECT_NOT_WRITABLE(base);
    ARCH_AXIOM(base[0] == 42);
    EXPECT_WRITABLE(base + pageSize);
    base[pageSize] = 43;
    ARCH_AXIOM(base[pageSize] == 43);

    // A two-byte range straddling the boundary touches both pages, so both lose
    // access -- page 1 despite the caller naming only one byte of it.
    ARCH_AXIOM(ArchSetMemoryProtection(base + pageSize - 1, 2,
                                       ArchProtectNoAccess));
    EXPECT_NOT_READABLE(base);
    EXPECT_NOT_READABLE(base + pageSize);

    ARCH_AXIOM(ArchFreeVirtualMemory(base, numBytes));
}

// At the time of writing, ArchFreeVirtualMemory() has no callers in the tree
// yet, so this is its only coverage.
static void TestFree()
{
    const size_t pageSize = ArchGetPageSize();
    const size_t numBytes = NUM_PAGES * pageSize;

    // An untouched reservation.
    void *base = ArchReserveVirtualMemory(numBytes);
    ARCH_AXIOM(base);
    ARCH_AXIOM(ArchFreeVirtualMemory(base, numBytes));

    // A partly committed, partly reprotected reservation: freeing takes the
    // whole thing regardless of the state of the pages within it.
    base = ArchReserveVirtualMemory(numBytes);
    ARCH_AXIOM(base);
    ARCH_AXIOM(ArchCommitVirtualMemoryRange(base, 4 * pageSize));
    ARCH_AXIOM(ArchSetMemoryProtection(base, 2 * pageSize,
                                       ArchProtectReadOnly));
    ARCH_AXIOM(ArchFreeVirtualMemory(base, numBytes));

    // A freed reservation is really gone.  Committing and writing first means
    // the pages were live, so a free that quietly did nothing would leave them
    // readable.  Nothing is allocated between the free and the check, so the
    // address cannot have been handed out again.
    char *live = static_cast<char *>(ArchReserveVirtualMemory(numBytes));
    ARCH_AXIOM(live);
    ARCH_AXIOM(ArchCommitVirtualMemoryRange(live, numBytes));
    EXPECT_WRITABLE(live);
    EXPECT_WRITABLE(live + numBytes - 1);
    memset(live, 0xCD, numBytes);
    ARCH_AXIOM(ArchFreeVirtualMemory(live, numBytes));
    EXPECT_NOT_READABLE(live);

    // Reserve/commit/free cycles are repeatable.
    for (int i = 0; i != 16; ++i) {
        void *p = ArchReserveVirtualMemory(numBytes);
        ARCH_AXIOM(p);
        ARCH_AXIOM(ArchCommitVirtualMemoryRange(p, pageSize));
        ARCH_AXIOM(ArchFreeVirtualMemory(p, numBytes));
    }
}

static void TestReserveFailure()
{
    // An impossible reservation must report failure rather than hand back a
    // pointer that faults on first touch.  This is the only error path the API
    // can be driven down portably.
    //
    // Only the return value is checked: the header documents errno, which holds
    // on POSIX but not on Windows, where VirtualAlloc() reports through
    // GetLastError() and leaves errno alone.
    ARCH_AXIOM(ArchReserveVirtualMemory(SIZE_MAX / 2) == nullptr);

    // A numBytes of zero is not tested anywhere in this file.  It is documented
    // as a precondition violation because the platforms disagree: on POSIX
    // mmap(0) fails while mprotect(len == 0) succeeds, and on Windows
    // VirtualAlloc(0) fails while VirtualFree(start, 0, MEM_RELEASE) succeeds
    // and releases the whole reservation.
}

int main()
{
#if !defined(TEST_HAS_FAULT_TRAP)
    // Not a silent skip: without a fault trap this run cannot tell a working
    // implementation from one that returns true and does nothing, which is a
    // live question on platforms that stub out mprotect().
    printf("NOTE: no fault-trap mechanism on this platform. Protection and "
           "commit enforcement are not verified; only return values are.\n");
#endif

    TestPageSize();
    TestReserve();
    TestCommit();
    TestCommitPageCoverage();
    TestProtection();
    TestProtectionPageCoverage();
    TestFree();
    TestReserveFailure();

    printf("OK\n");

    return 0;
}
