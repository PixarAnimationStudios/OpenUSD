//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/base/arch/mallocHook.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/attributes.h"

#include <dlfcn.h>
#include <string.h>

#if defined(ARCH_OS_DARWIN)
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif /* defined(ARCH_OS_DARWIN) */

#if not defined(__MALLOC_HOOK_VOLATILE)
#define __MALLOC_HOOK_VOLATILE
#endif /* not defined(__MALLOC_HOOK_VOLATILE) */

using std::string;

/*
 * These are hook variables (they're not functions, so they don't need
 * an extern "C"). Allocator libraries must provide these hooks in order for
 * ArchMallocHook to work.
 */
extern void* (*__MALLOC_HOOK_VOLATILE __malloc_hook)(size_t __size,  const void*);
extern void* (*__MALLOC_HOOK_VOLATILE __realloc_hook)(void* __ptr, size_t __size, const void*);
extern void* (*__MALLOC_HOOK_VOLATILE __memalign_hook)(size_t __alignment, size_t __size, const void*);
extern void (*__MALLOC_HOOK_VOLATILE __free_hook)(void* __ptr,  const void*);

/*
 * ArchMallocHook requires allocators to provide some specific functionality
 * in order to work properly. Allocators must provide hooks that are executed
 * when allocation functions are called (see above) and a corresponding set of
 * functions that do not execute hooks at all.
 *
 * As of this writing, we have two allocator libraries that have been custom
 * modified to meet these requirements: ptmalloc3 and jemalloc. Code below
 * explicitly looks for one of these two libraries.
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

// Helper function that returns true if "malloc" is provided by the same
// library as the given function. This is needed to determine which allocator
// is active; being able to find a particular library's malloc function doesn't
// ensure that library is the active allocator.
static bool
_MallocProvidedBySameLibraryAs(const char* functionName,
                               bool skipMallocCheck)
{
    const void* function = dlsym(RTLD_DEFAULT, functionName);
    if (!function) {
        return false;
    }

    Dl_info functionInfo, mallocInfo;
    if (!dladdr(function, &functionInfo) or
        !dladdr((void *)malloc, &mallocInfo)) {
        return false;
    }

    return (skipMallocCheck || mallocInfo.dli_fbase == functionInfo.dli_fbase);
}

static inline bool
_CheckMallocTagImpl(const char* impl, const char* libname)
{
    return (impl                                    == 0 ||
            strcmp (impl, "auto")                   == 0 ||
            strcmp (impl, "agnostic")               == 0 ||
            strncmp(impl, libname, strlen(libname)) == 0);
}

bool
ArchIsPxmallocActive()
{
    const char* impl = getenv("TF_MALLOC_TAG_IMPL");
    if (!_CheckMallocTagImpl(impl, "pxmalloc")) {
        return false;
    }
    bool skipMallocCheck = (impl && (strcmp(impl, "pxmalloc force") == 0));
    return _MallocProvidedBySameLibraryAs("__pxmalloc_malloc", skipMallocCheck);
}

bool
ArchIsPtmallocActive()
{
    const char* impl = getenv("TF_MALLOC_TAG_IMPL");
    if (!_CheckMallocTagImpl(impl, "ptmalloc")) {
        return false;
    }
    bool skipMallocCheck = (impl && (strcmp(impl, "ptmalloc force") == 0));
    return _MallocProvidedBySameLibraryAs("__ptmalloc3_malloc", skipMallocCheck);
}

bool
ArchIsJemallocActive()
{
    const char* impl = getenv("TF_MALLOC_TAG_IMPL");
    if (!_CheckMallocTagImpl(impl, "jemalloc")) {
        return false;
    }
    bool skipMallocCheck = (impl && (strcmp(impl, "jemalloc force") == 0));
    return _MallocProvidedBySameLibraryAs("__jemalloc_malloc", skipMallocCheck);
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
    static bool isOff = bool(getenv("GLIBCXX_FORCE_NEW"));
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

#if defined(ARCH_OS_LINUX)
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
#endif

static bool
_MallocHookAvailable()
{
    return (ArchIsPxmallocActive() or
            ArchIsPtmallocActive() or
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

bool
ArchMallocHook::Initialize(
    ARCH_UNUSED_ARG void* (*mallocWrapper)(size_t, const void*),
    ARCH_UNUSED_ARG void* (*reallocWrapper)(void*, size_t, const void*),
    ARCH_UNUSED_ARG void* (*memalignWrapper)(size_t, size_t, const void*),
    ARCH_UNUSED_ARG void  (*freeWrapper)(void*, const void*),
    string* errMsg)
{
#if !defined(ARCH_OS_LINUX)
    *errMsg = "ArchMallocHook functionality not implemented for non-linux systems";
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

    if (__malloc_hook || __realloc_hook || __memalign_hook || __free_hook) {
	*errMsg = "One or more malloc/realloc/free hook variables are already set.\n"
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
