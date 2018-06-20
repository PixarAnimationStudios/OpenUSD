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
#ifndef ARCH_LIBRARY_H
#define ARCH_LIBRARY_H

#include "pxr/pxr.h"
#include "pxr/base/arch/api.h"

#include <string>

#if defined(ARCH_OS_WINDOWS)
#   define ARCH_LIBRARY_LAZY    0
#   define ARCH_LIBRARY_NOW     0
#   define ARCH_LIBRARY_LOCAL   0
#   define ARCH_LIBRARY_GLOBAL  0
#   define ARCH_LIBRARY_SUFFIX  ".dll"
#   define ARCH_STATIC_LIBRARY_SUFFIX ".lib"
#else
#   include <dlfcn.h>
#   define ARCH_LIBRARY_LAZY    RTLD_LAZY
#   define ARCH_LIBRARY_NOW     RTLD_NOW
#   define ARCH_LIBRARY_LOCAL   RTLD_LOCAL
#   define ARCH_LIBRARY_GLOBAL  RTLD_GLOBAL
#   if defined(ARCH_OS_DARWIN)
#       define ARCH_LIBRARY_SUFFIX  ".dylib"
#   else
#       define ARCH_LIBRARY_SUFFIX  ".so"
#   endif
#   define ARCH_STATIC_LIBRARY_SUFFIX ".a"
#endif

PXR_NAMESPACE_OPEN_SCOPE

/// library.h
/// Architecture dependent loading and unloading of dynamic libraries.
/// \ingroup group_arch_SystemFunctions

/// Load an executable object file.
/// \ingroup group_arch_SystemFunctions
///
/// Opens the dynamic library that is specified by filename.
/// Returning the handle to the module if successful; false otherwise.
ARCH_API 
void* ArchLibraryOpen(const std::string &filename, int flag);

/// Obtain a description of the most recent error that occurred from
/// \c ArchLibraryOpen.
///\ingroup group_arch_SystemFunctions
ARCH_API
std::string ArchLibraryError();

/// Closes an object opened with \c ArchLibraryOpen.
/// \ingroup group_arch_SystemFunctions
ARCH_API
int ArchLibraryClose(void* handle);

/// Retrieves the address of an exported symbol from the specified
/// dynamic library.
ARCH_API
void* ArchLibraryFindSymbol(void* handle, char const* name);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // ARCH_LIBRARY_H
