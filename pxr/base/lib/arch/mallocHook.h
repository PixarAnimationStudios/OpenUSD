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
#ifndef ARCH_MALLOCHOOK_H
#define ARCH_MALLOCHOOK_H

/// \file arch/mallocHook.h
/// \ingroup group_arch_Memory
/// Routines for controlling malloc behavior.

#include <stdlib.h>
#include <string>

/// Return true if ptmalloc is being used as the memory allocator
///
/// ptmalloc3 is an external shared library providing implementations of the
/// standard memory allocation functions (e.g. malloc, free). Consumers with
/// special behavior that depends on this library may use this function to
/// determine if it is the active allocator.
///
/// \ingroup group_arch_Memory
bool ArchIsPtmallocActive();

/// Return true if the C++ STL allocator was requested to be turned off.
///
/// Under gcc, this is done by setting the environment variable
/// GLIBCXX_FORCE_NEW, but it might differ (or not even be possible) for other
/// platforms.
///
/// \ingroup group_arch_Memory
bool ArchIsStlAllocatorOff();

/// \class ArchMallocHook
/// \ingroup group_arch_Memory
///
/// Override default malloc() functionality.
///
/// The \c ArchMallocHook class is used on supported systems to install a
/// call-back function in place of the standard malloc/realloc/free/memalign
/// function calls. Supported systems are currently restricted to 64-bit linux
/// systems.
///
/// The call-back function can access the original allocation function by
/// calling, for example, \c ArchMallocHook::Malloc, or it is free to perform
/// its own allocation.
///
/// The \c ArchMallocHook is a POD (plain old datastructure) which means that
/// to use it properly, it should be declared at global scope, ensuring
/// zero-initialization.
class ArchMallocHook {
public:
    /// Initialize hooks.
    ///
    /// Calling \c Initialize() installs the supplied functions as call-back
    /// in place of the standard system memory allocation routines.  Note
    /// that the callbacks take an extra \c const \c void* parameter; on
    /// supported systems, the called function should simply ignore the extra
    /// parameter.
    ///
    /// If initialization fails, \c false is returned and \p *errMsg is set
    /// accordingly. If \c *this has already been initialized, calling \c
    /// Initialize() a second time will fail.
    bool Initialize(void* (*mallocWrapper)(size_t, const void*),
            void* (*reallocWrapper)(void*, size_t, const void*),
            void* (*memalignWrapper)(size_t, size_t, const void*),
            void  (*freeWrapper)(void*, const void*),
            std::string* errMsg);

    /// Return true if \c *this has been (successfully) initialized.
    ///
    /// In order for this function to work properly, \c this cannot be a local
    /// or dynamically initialized variable; rather, \c this must be a global
    /// variable, to ensure zero-initialization.
    bool IsInitialized();

    /// Call the original system \c malloc() function.
    ///
    /// This function allows user-supplied callbacks to access the original
    /// system-supplied malloc() call.  For speed reasons, no safety checks
    /// are performed; in particular, calling this function without having
    /// successfully initialized \p *this will likely crash your program.
    void* Malloc(size_t nBytes) {
        return (*_underlyingMallocFunc)(nBytes);
    }

    /// Call the original system \c realloc() function.
    ///
    /// This function allows user-supplied callbacks to access the original
    /// system-supplied \c realloc() call.  For speed reasons, no safety checks
    /// are performed; in particular, calling this function without having
    /// successfully initialized \p *this will likely crash your program.
    void* Realloc(void* ptr, size_t nBytes) {
        return (*_underlyingReallocFunc)(ptr, nBytes);
    }

    /// Call the original system \c memalign() function.
    ///
    /// This function allows user-supplied callbacks to access the original
    /// system-supplied \c memalign() call.  For speed reasons, no safety checks
    /// are performed; in particular, calling this function without having
    /// successfully initialized \p *this will likely crash your program.
    void* Memalign(size_t alignment, size_t nBytes) {
        return (*_underlyingMemalignFunc)(alignment, nBytes);
    }

    /// Call the original system \c free() function.
    ///
    /// This function allows user-supplied callbacks to access the original
    /// system-supplied \c free() call.  For speed reasons, no safety checks
    /// are performed; in particular, calling this function without having
    /// successfully initialized \p *this will likely crash your program.
    void Free(void* ptr) {
        (*_underlyingFreeFunc)(ptr);
    }

private:
    // Note: this is a POD (plain 'ol data structure) so we depend on zero
    // initialization here to null these out.  Do not add a constructor or
    // destructor to this class.

    void* (*_underlyingMallocFunc)(size_t);
    void* (*_underlyingReallocFunc)(void*, size_t);
    void* (*_underlyingMemalignFunc)(size_t, size_t);
    void  (*_underlyingFreeFunc)(void*);
};

#endif // ARCH_MALLOCHOOK_H
