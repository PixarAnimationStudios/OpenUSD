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

#include <thread>

PXR_NAMESPACE_OPEN_SCOPE

/// Return true if the calling thread is the main thread, false otherwise.
/// \ingroup group_arch_Multithreading
ARCH_API bool ArchIsMainThread();

/// Return the std::thread_id for the thread arch considers to be the "main"
/// thread.
ARCH_API std::thread::id ArchGetMainThreadId();

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
