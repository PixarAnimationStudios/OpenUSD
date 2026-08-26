//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/threads.h"
#include "pxr/base/arch/error.h"

#include <cstdio>
#include <string>
#include <thread>

#if defined(ARCH_OS_LINUX)
#  include <cerrno>
#  include <sys/resource.h>
#endif

PXR_NAMESPACE_USING_DIRECTIVE

// Platforms where a thread name can be set and read back, priority can be
// lowered, and std::thread is available for the per-thread scoping checks. WASM
// has none of the three, so it takes the other path below.
//
// Where a facility is absent we assert the documented failure behavior rather
// than skipping, so that an incomplete implementation cannot pass by accident.
#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN) || \
    defined(ARCH_OS_WINDOWS)
#define TEST_HAS_FULL_THREAD_API
#endif

// WASM can set a name via emscripten_set_thread_name(), but only when built
// with --threadprofiler, and it offers no way to read one back.
#if defined(ARCH_OS_WASM_VM) && \
    defined(PTHREADS_PROFILING) && PTHREADS_PROFILING
#define TEST_HAS_THREAD_NAME_SET_ONLY
#endif

#if defined(TEST_HAS_FULL_THREAD_API)

static void TestNaming()
{
    // Short name within all platform limits.
    ARCH_AXIOM(ArchSetThisThreadName("test"));
    ARCH_AXIOM(ArchGetThisThreadName() == "test");

    // A null name is rejected rather than crashing, and leaves the name alone.
    ARCH_AXIOM(!ArchSetThisThreadName(nullptr));
    ARCH_AXIOM(ArchGetThisThreadName() == "test");

    // An empty name is accepted everywhere, and clears the name.
    ARCH_AXIOM(ArchSetThisThreadName(""));
    ARCH_AXIOM(ArchGetThisThreadName() == "");

    // Long name - exercises the Linux truncation path. The get must
    // return the truncated form, not the original.
    ARCH_AXIOM(ArchSetThisThreadName("this-name-is-longer-than-fifteen-bytes"));
    std::string name = ArchGetThisThreadName();
#if defined(ARCH_OS_LINUX)
    ARCH_AXIOM(name.size() == 15);
    ARCH_AXIOM(name == "this-name-is-lo");
#else
    ARCH_AXIOM(name == "this-name-is-longer-than-fifteen-bytes");
#endif

    // UTF-8 round-trip - a 3-byte sequence (U+2022 BULLET = 0xE2 0x80 0xA2).
    // One bullet is 3 bytes, so five bullets = 15 bytes, exactly at the
    // Linux limit.
    ARCH_AXIOM(ArchSetThisThreadName("\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2"
                                     "\xE2\x80\xA2\xE2\x80\xA2")); // 15 bytes
    ARCH_AXIOM(ArchGetThisThreadName() ==
               "\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2"
               "\xE2\x80\xA2\xE2\x80\xA2");

#if defined(ARCH_OS_LINUX)
    // Exactly 16 characters must be truncated to 15.
    ARCH_AXIOM(ArchSetThisThreadName("1234567890123456"));
    ARCH_AXIOM(ArchGetThisThreadName() == "123456789012345");

    // 6 bullets = 18 bytes - must truncate at a character boundary to 5
    // bullets (15 bytes), not 15 raw bytes that would split the 6th bullet.
    ARCH_AXIOM(ArchSetThisThreadName("\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2"
                                     "\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2"));
    name = ArchGetThisThreadName();
    ARCH_AXIOM(name.size() == 15);
    ARCH_AXIOM(name == "\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2"
                       "\xE2\x80\xA2\xE2\x80\xA2");

    // B + 6 bullets = 19 bytes - must truncate at a character boundary to B + 4
    // bullets (13 bytes).
    ARCH_AXIOM(ArchSetThisThreadName("B\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2"
                                      "\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2"));
    name = ArchGetThisThreadName();
    ARCH_AXIOM(name.size() == 13);
    ARCH_AXIOM(name == "B\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2");
#endif

#if defined(ARCH_OS_DARWIN)
    // MacOS enforces MAXTHREADNAMESIZE (64), so names are truncated to 63
    // bytes. The truncation algorithm itself is covered by the Linux cases
    // above; this just confirms the limit is wired up, which matters because
    // MacOS silently rejects an over-long name outright rather than shortening
    // it -- without truncation the name would not change at all here.
    std::string longName(200, 'x');
    ARCH_AXIOM(ArchSetThisThreadName(longName.c_str()));
    name = ArchGetThisThreadName();
    ARCH_AXIOM(name.size() == 63);
    ARCH_AXIOM(name == longName.substr(0, 63));
#endif
}

// Naming is a per-thread attribute: a name set on a worker must not leak back
// to the thread that spawned it.
static void TestNameIsPerThread()
{
    ARCH_AXIOM(ArchSetThisThreadName("main-thread"));

    std::thread t([]() {
        ARCH_AXIOM(!ArchIsMainThread());
        ARCH_AXIOM(ArchSetThisThreadName("worker"));
        ARCH_AXIOM(ArchGetThisThreadName() == "worker");
    });
    t.join();

    ARCH_AXIOM(ArchIsMainThread());
    ARCH_AXIOM(ArchGetThisThreadName() == "main-thread");
}

// ArchSetThisThreadPriority() only ever lowers priority, so the checks below
// need to start from a priority above the levels they request. A test process
// launched under nice(1), or on a build machine that de-prioritizes jobs, would
// fail them for reasons that have nothing to do with the code, so detect that
// and skip instead.
//
// Only Linux is probed here; that is where "the whole job runs niced" is a
// realistic configuration. The other platforms assume a normal-priority start.
static bool CanTestPriority()
{
#if defined(ARCH_OS_LINUX)
    // getpriority() returns -1 on error, but -1 is also a valid nice value, so
    // check errno explicitly.
    errno = 0;
    int nice = getpriority(PRIO_PROCESS, 0);
    if (nice == -1 && errno != 0) {
        printf("Skipping priority tests: getpriority() failed.\n");
        return false;
    }
    if (nice >= 10) {
        printf("Skipping priority tests: process is already at nice=%d, "
               "which is at or below the levels under test.\n", nice);
        return false;
    }
#endif
    return true;
}

// Priority is likewise a per-thread attribute. This matters most on Linux,
// where the implementation depends on the nice value being per-thread -- a
// Linux/NPTL extension, since POSIX specifies it as per-process. If that ever
// stopped holding, the whole process would get de-prioritized here.
static void TestPriorityIsPerThread()
{
#if defined(ARCH_OS_LINUX)
    errno = 0;
    int mainNice = getpriority(PRIO_PROCESS, 0);
    ARCH_AXIOM(!(mainNice == -1 && errno != 0));
#endif

    std::thread t([]() {
        ARCH_AXIOM(ArchSetThisThreadPriority(ArchThreadPriorityLowest));
    });
    t.join();

#if defined(ARCH_OS_LINUX)
    // The worker dropped to nice=19; this thread must be untouched.
    errno = 0;
    ARCH_AXIOM(getpriority(PRIO_PROCESS, 0) == mainNice);
#endif
}

static void TestPriority()
{
    // Low priority - should succeed on a freshly-launched thread.
    ARCH_AXIOM(ArchSetThisThreadPriority(ArchThreadPriorityLow));

#if defined(ARCH_OS_LINUX)
    // Verify low means nice=10 on Linux.
    ARCH_AXIOM(getpriority(PRIO_PROCESS, 0) == 10);
#endif

    // Already at Low - must return false.
    ARCH_AXIOM(!ArchSetThisThreadPriority(ArchThreadPriorityLow));

    // Lowest from Low - should succeed.
    ARCH_AXIOM(ArchSetThisThreadPriority(ArchThreadPriorityLowest));

#if defined(ARCH_OS_LINUX)
    // Verify lowest means nice=19 on Linux.
    ARCH_AXIOM(getpriority(PRIO_PROCESS, 0) == 19);
#endif

    // Already at Lowest - must return false.
    ARCH_AXIOM(!ArchSetThisThreadPriority(ArchThreadPriorityLowest));

    // Low from Lowest must also return false - Lowest is below Low, not
    // above it, so this would be a raise.
    ARCH_AXIOM(!ArchSetThisThreadPriority(ArchThreadPriorityLow));
}

#else // !TEST_HAS_FULL_THREAD_API

// On a platform that implements none (or only part of the API, currently WASM)
// assert the documented contract for the missing parts. This is deliberately
// not a skip: reporting failure correctly is behavior worth verifying, and a
// platform that later grows a real implementation should fail here and be moved
// to the full path above rather than passing silently.
static void TestIncompletelyImplementedApi()
{
#if defined(TEST_HAS_THREAD_NAME_SET_ONLY)
    ARCH_AXIOM(ArchSetThisThreadName("test"));
#else
    ARCH_AXIOM(!ArchSetThisThreadName("test"));
#endif

    // No get API, so a name must not come back.
    ARCH_AXIOM(ArchGetThisThreadName().empty());

    // A null name is rejected on every platform, before any platform code runs.
    ARCH_AXIOM(!ArchSetThisThreadName(nullptr));

    // No thread priority facility, so both levels must report failure rather
    // than silently doing nothing and claiming success.
    ARCH_AXIOM(!ArchSetThisThreadPriority(ArchThreadPriorityLow));
    ARCH_AXIOM(!ArchSetThisThreadPriority(ArchThreadPriorityLowest));
}

#endif // TEST_HAS_FULL_THREAD_API

int main()
{
    ARCH_AXIOM(ArchIsMainThread());

#if defined(TEST_HAS_FULL_THREAD_API)
    // Naming changes are reversible, so they are safe to run on the main
    // thread.
    TestNaming();
    TestNameIsPerThread();

    // Lowering priority is one-way, so run the priority tests on threads we
    // throw away and leave the main thread at its inherited priority. This also
    // keeps TestPriorityIsPerThread()'s baseline meaningful, and lets any test
    // added later still start from a normal priority.
    if (CanTestPriority()) {
        TestPriorityIsPerThread();
        std::thread(TestPriority).join();
    }
#else
    TestIncompletelyImplementedApi();
#endif

    return 0;
}
