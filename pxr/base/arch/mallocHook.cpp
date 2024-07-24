//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/mallocHook.h"
#include "pxr/base/arch/attributes.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/env.h"

#if !defined(ARCH_OS_WINDOWS)
#   include <dlfcn.h>
#endif
#include <cstring>

#if defined(ARCH_OS_IPHONE)
#elif defined(ARCH_OS_DARWIN)
#   include <sys/malloc.h>
#else
#   include <malloc.h>
#endif /* defined(ARCH_OS_IPHONE) */

PXR_NAMESPACE_OPEN_SCOPE

// Malloc hooks were removed in glibc 2.34.
#if defined(ARCH_OS_LINUX) && \
    defined(__GLIBC__) && __GLIBC__ <= 2 && __GLIBC_MINOR__ < 34
#define MALLOC_HOOKS_AVAILABLE
#endif

using std::string;

/*
 * ArchMallocHook requires allocators to provide some specific functionality
 * in order to work properly. Allocators must provide hooks that are executed
 * when allocation functions are called (see above) and a corresponding set of
 * functions that do not execute hooks at all.
 *
 * As of this writing, we have two allocator libraries that have been custom
 * modified to meet these requirements: ptmalloc3 and jemalloc (with the
 * pxmalloc wrapper). Code below explicitly looks for one of these two
 * libraries.
 *
 * If your program doesn't link against these, you could still do everything
 * we're doing with the regular built-in malloc of glibc, but it would require
 * you to LD_PRELOAD an additional library that knew how to work with standard
 * malloc --- but in that case, you might just as well LD_PRELOAD ptmalloc3
 * itself.
 *
 * So, to boil this down: if you link against or LD_PRELOAD the listed
 * libs, the code in this file will be enabled.  Otherwise, it'll
 * run-time detect that it can't do what it wants.
 *
 * Note that support for non-linux and non-64 bit platforms is not provided.
 */


#ifdef MALLOC_HOOKS_AVAILABLE

/*
 * These are hook variables (they're not functions, so they don't need
 * an extern "C"). Allocator libraries must provide these hooks in order for
 * ArchMallocHook to work.
 */

#if !defined(__MALLOC_HOOK_VOLATILE)
#   define __MALLOC_HOOK_VOLATILE
#endif /* !defined(__MALLOC_HOOK_VOLATILE) */

PXR_NAMESPACE_CLOSE_SCOPE

extern void*
(*__MALLOC_HOOK_VOLATILE __malloc_hook)(
    size_t __size,  const void*);
extern void*
(*__MALLOC_HOOK_VOLATILE __realloc_hook)(
    void* __ptr, size_t __size, const void*);
extern void*
(*__MALLOC_HOOK_VOLATILE __memalign_hook)(
    size_t __alignment, size_t __size, const void*);
extern void
(*__MALLOC_HOOK_VOLATILE __free_hook)(
    void* __ptr,  const void*);

PXR_NAMESPACE_OPEN_SCOPE

template <typename T>
static bool _GetSymbol(T* addr, const char* name, string* errMsg) {
    if (void* symbol = dlsym(RTLD_DEFAULT, name)) {
        *addr = (T) symbol;
        return true;
    }
    else {
        *errMsg = "lookup for symbol '" + string(name) + "' failed";
        return false;
    }
}

static inline bool
_CheckMallocTagImpl(const std::string& impl, const char* libname)
{
    return (impl.empty()       ||
            impl == "auto"     ||
            impl == "agnostic" ||
            std::strncmp(impl.c_str(), libname, strlen(libname)) == 0);
}

// Helper function that returns true if "malloc" is provided by the same
// library as the given function. This is needed to determine which allocator
// is active; being able to find a particular library's malloc function doesn't
// ensure that library is the active allocator.
static bool
_MallocProvidedBySameLibraryAs(const char* functionName,
                               bool skipMallocCheck)
{
#if !defined(ARCH_OS_WINDOWS)
    const void* function = dlsym(RTLD_DEFAULT, functionName);
    if (!function) {
        return false;
    }

    Dl_info functionInfo, mallocInfo;
    if (!dladdr(function, &functionInfo) || 
        !dladdr((void *)malloc, &mallocInfo)) {
        return false;
    }

    return (skipMallocCheck || mallocInfo.dli_fbase == functionInfo.dli_fbase);
#else
    return false;
#endif
}

static bool
ArchIsPxmallocActive()
{
    const std::string impl = ArchGetEnv("TF_MALLOC_TAG_IMPL");
    if (!_CheckMallocTagImpl(impl, "pxmalloc")) {
        return false;
    }
    bool skipMallocCheck = (impl == "pxmalloc force");
    return _MallocProvidedBySameLibraryAs("__pxmalloc_malloc", skipMallocCheck);
}

static bool
ArchIsJemallocActive()
{
    const std::string impl = ArchGetEnv("TF_MALLOC_TAG_IMPL");
    if (!_CheckMallocTagImpl(impl, "jemalloc")) {
        return false;
    }
    bool skipMallocCheck = (impl == "jemalloc force");
    return _MallocProvidedBySameLibraryAs("__jemalloc_malloc", skipMallocCheck);
}

static bool
_MallocHookAvailable()
{
    return (ArchIsPxmallocActive() ||
            ArchIsPtmallocActive() ||
            ArchIsJemallocActive());
}

struct Arch_MallocFunctionNames
{
    const char* mallocFn;
    const char* reallocFn;
    const char* memalignFn;
    const char* freeFn;
};

static Arch_MallocFunctionNames
_GetUnderlyingMallocFunctionNames()
{
    Arch_MallocFunctionNames names;
    if (ArchIsPxmallocActive()) {
        names.mallocFn = "__pxmalloc_malloc";
        names.reallocFn = "__pxmalloc_realloc";
        names.memalignFn = "__pxmalloc_memalign";
        names.freeFn = "__pxmalloc_free";
    }
    else if (ArchIsPtmallocActive()) {
        names.mallocFn = "__ptmalloc3_malloc";
        names.reallocFn = "__ptmalloc3_realloc";
        names.memalignFn = "__ptmalloc3_memalign";
        names.freeFn = "__ptmalloc3_free";
    }
    else if (ArchIsJemallocActive()) {
        names.mallocFn = "__jemalloc_malloc";
        names.reallocFn = "__jemalloc_realloc";
        names.memalignFn = "__jemalloc_memalign";
        names.freeFn = "__jemalloc_free";
    }

    return names;
}

#endif // MALLOC_HOOKS_AVAILABLE

bool
ArchIsPtmallocActive()
{
#ifdef MALLOC_HOOKS_AVAILABLE
    const std::string impl = ArchGetEnv("TF_MALLOC_TAG_IMPL");
    if (!_CheckMallocTagImpl(impl, "ptmalloc")) {
        return false;
    }
    bool skipMallocCheck = (impl == "ptmalloc force");
    return _MallocProvidedBySameLibraryAs("__ptmalloc3_malloc", skipMallocCheck);
#else
    return false;
#endif
}

bool
ArchIsStlAllocatorOff()
{
#if defined(ARCH_COMPILER_GCC) || defined(ARCH_COMPILER_ICC) || \
    defined(ARCH_COMPILER_CLANG)
    // I'm assuming that ICC compiles will use the gcc STL library.

    /*
     * This is a race, but the STL library itself does it this way.
     * The assumption is that even if you race, you get the same
     * value.  There's no assurance that the environment variable has
     * the same setting as when gcc code looked at it, but even if it
     * isn't, it's just a preference, not behavior that has to correct
     * to avoid a crash.
     */
    static bool isOff = ArchHasEnv("GLIBCXX_FORCE_NEW");
    return isOff;
#else
    return false;
#endif
}

bool
ArchMallocHook::IsInitialized()
{
    return _underlyingMallocFunc || _underlyingReallocFunc ||
       _underlyingMemalignFunc || _underlyingFreeFunc;
}

bool
ArchMallocHook::Initialize(
    ARCH_UNUSED_ARG void* (*mallocWrapper)(size_t, const void*),
    ARCH_UNUSED_ARG void* (*reallocWrapper)(void*, size_t, const void*),
    ARCH_UNUSED_ARG void* (*memalignWrapper)(size_t, size_t, const void*),
    ARCH_UNUSED_ARG void  (*freeWrapper)(void*, const void*),
    string* errMsg)
{
#if !defined(ARCH_OS_LINUX)
    *errMsg = "ArchMallocHook only available for Linux/glibc systems";
    return false;
#elif !defined(MALLOC_HOOKS_AVAILABLE)
    *errMsg = "C library does not provide malloc hooks";
    return false;
#else
    
    if (IsInitialized()) {
        *errMsg = "ArchMallocHook already initialized";
        return false;
    }

    if (!_MallocHookAvailable()) {
        *errMsg =
            "ArchMallocHook functionality not available for current allocator";
        return false;
    }

    /*
     * Ensure initialization of the malloc system hook mechanism.  The sequence
     * below works for both built-in malloc (i.e. in glibc) and external
     * ptmalloc3.
     */
    free(realloc(malloc(1), 2));
    free(memalign(sizeof(void*), sizeof(void*)));

    // We check here that either the hooks are unset, or they're set to malloc,
    // free, etc.  We do this because at least one allocator (jemalloc)
    // explicitly sets the hooks to point to its malloc functions to work around
    // bugs related to shared libraries opened with the DEEPBIND flag picking up
    // the system (glibc) malloc symbols instead of the custom allocator's
    // (jemalloc's).  Pixar's pxmalloc wrapper does the same, for the same
    // reason.
    if ((__malloc_hook &&
         __malloc_hook != reinterpret_cast<void *>(malloc)) ||
        (__realloc_hook &&
         __realloc_hook != reinterpret_cast<void *>(realloc)) ||
        (__memalign_hook &&
         __memalign_hook != reinterpret_cast<void *>(memalign)) ||
        (__free_hook &&
         __free_hook != reinterpret_cast<void *>(free))) {
        *errMsg =
            "One or more malloc/realloc/free hook variables are already set.\n"
            "This probably means another entity in the program is trying to\n"
            "do its own profiling, pre-empting yours.";
        return false;
    }

    /*
     * We modify _underlyingMallocFunc etc. by reference, to avoid
     * a complaint about "type-punning" w.r.t. strict-aliasing.
     */
    const Arch_MallocFunctionNames names = _GetUnderlyingMallocFunctionNames();

    if (!_GetSymbol(&_underlyingMallocFunc, names.mallocFn, errMsg) ||
        !_GetSymbol(&_underlyingReallocFunc, names.reallocFn, errMsg) ||
        !_GetSymbol(&_underlyingMemalignFunc, names.memalignFn, errMsg) ||
        !_GetSymbol(&_underlyingFreeFunc, names.freeFn, errMsg)) {
        return false;
    }

    if (mallocWrapper)
        __malloc_hook = mallocWrapper;

    if (reallocWrapper)
        __realloc_hook = reallocWrapper;

    if (memalignWrapper)
        __memalign_hook = memalignWrapper;

    if (freeWrapper)
        __free_hook = freeWrapper;

    return true;
#endif
}

PXR_NAMESPACE_CLOSE_SCOPE
