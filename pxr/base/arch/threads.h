//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_ARCH_THREADS_H
#define PXR_BASE_ARCH_THREADS_H

/// \file arch/threads.h
/// \ingroup group_arch_Multithreading
/// Architecture-specific thread function calls.

#include "pxr/pxr.h"
#include "pxr/base/arch/api.h"
#include "pxr/base/arch/defines.h"

// Needed for ARCH_SPIN_PAUSE on Windows in builds with precompiled
// headers disabled.
#ifdef ARCH_COMPILER_MSVC
#include <intrin.h>
#endif

#include <string>
#include <thread>

PXR_NAMESPACE_OPEN_SCOPE

/// Return true if the calling thread is the main thread, false otherwise.
/// \ingroup group_arch_Multithreading
ARCH_API bool ArchIsMainThread();

/// Return the std::thread_id for the thread arch considers to be the "main"
/// thread.
ARCH_API std::thread::id ArchGetMainThreadId();

/// Set the name of the calling thread as UTF-8 text.
///
/// The name is typically visible in debuggers and OS tools. Return true if the
/// name was set, possibly after truncation (see below). Return false if \p name
/// is null, if the OS rejected the name, or if the platform provides no thread
/// naming facility.
///
/// \note Only call this on threads that your code created or explicitly owns.
/// Calling this on your caller's thread (especially in library code) or on a
/// shared worker thread is inadvisable. For example, TBB reuses worker threads
/// across tasks, so a name set in one task will persist on that thread after
/// the task completes, misleading debuggers and profilers for all subsequent
/// work scheduled onto it.
///
/// \note Names are truncated to a platform-specific byte limit imposed by the
/// underlying API: 15 bytes on Linux, 63 bytes on MacOS, and 31 bytes on WASM,
/// none counting the null terminator. Linux and MacOS reject an over-long name
/// outright; Emscripten cuts one on a byte boundary, which can split a
/// multi-byte character. In all three cases we truncate first, at a UTF-8
/// character boundary so the result remains valid UTF-8, which means
/// ArchGetThisThreadName() may return a shorter name than the one passed here.
/// No truncation is applied on Windows.
///
/// \note On WASM the name is recorded only in builds that enable Emscripten's
/// thread profiler (--threadprofiler). Without it, the underlying call is a
/// documented no-op, so this returns false in that case.
ARCH_API bool ArchSetThisThreadName(char const *name);

/// Return the name of the calling thread as a UTF-8 string, as previously set
/// either by ArchSetThisThreadName() or by another entity calling the
/// underlying OS thread naming APIs.  Return an empty string if the name cannot
/// be retrieved.
///
/// \note A thread that was never explicitly named does not necessarily have an
/// empty name. On Linux a thread's name defaults to the executable's basename,
/// and a newly created thread inherits its creator's name.
///
/// \note Conversely, some platforms provide no way to read a name back, so this
/// can return an empty string even immediately after ArchSetThisThreadName()
/// succeeded. WASM is an example: Emscripten offers a set but no corresponding
/// get.
ARCH_API std::string ArchGetThisThreadName();

/// Specifies a reduced-priority level for ArchSetThisThreadPriority().
///
/// Enumerators are ordered ascending by priority, so 'a < b' means "a is lower
/// priority than b".
enum ArchThreadPriority {
    /// Idle priority. The thread runs only when no other thread wants the
    /// core. Appropriate for work that must never interfere with foreground
    /// activity.
    ArchThreadPriorityLowest,

    /// Below-normal priority. The thread yields to normal-priority threads but
    /// still competes actively for CPU time.
    ArchThreadPriorityLow,
};

/// Lower the priority of the calling thread to \p priority.
///
/// Return true if the priority was successfully changed, false if the thread is
/// already at or below the requested level, if the request failed, or if the
/// platform has no thread priority facility. This is a one-way operation --
/// there is no facility to raise priority back to the inherited default, and
/// calling this function will never raise priority above the current level.
///
/// \note WASM has no thread priority facility, so this always returns false
/// there. Threads are Web Workers, which expose no priority control.
///
/// \note Only call this on threads that your code created or explicitly
/// owns. See ArchSetThisThreadName() for the same rationale.
ARCH_API bool ArchSetThisThreadPriority(ArchThreadPriority priority);


/// ARCH_SPIN_PAUSE -- 'pause' on x86, 'yield' on arm.
#if defined(ARCH_CPU_INTEL)
#if defined(ARCH_COMPILER_GCC) || defined(ARCH_COMPILER_CLANG)
#define ARCH_SPIN_PAUSE() __builtin_ia32_pause()
#elif defined(ARCH_COMPILER_MSVC)
#define ARCH_SPIN_PAUSE() _mm_pause()
#endif
#elif defined(ARCH_CPU_ARM)
#if defined(ARCH_COMPILER_GCC) || defined(ARCH_COMPILER_CLANG)
#define ARCH_SPIN_PAUSE() asm volatile ("yield" ::: "memory")
#elif defined(ARCH_COMPILER_MSVC)
#define ARCH_SPIN_PAUSE() __yield();
#endif
#else
#define ARCH_SPIN_PAUSE()
#endif

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_ARCH_THREADS_H
