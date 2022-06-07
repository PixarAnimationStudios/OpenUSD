//
// Copyright 2022 Pixar
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

#ifndef EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_ARCH_DEFS_H
#define EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_ARCH_DEFS_H

// PRMan depends on a nonstandard set of architecture defines which, when prman
// is built, are defined in the build script and passed to the compiler as -D
// commandline params. Some of the header files in rman/libpxrcore expose these
// dependencies and so we recreate these nonstandard defines here using the more
// standard architecture defines from pxr/base/arch/defines.h to guide us.

#include "pxr/base/arch/defines.h"

// Platform | Standard Define   | pxr/base Define | Prman Define
//==========+===================+=================+==============
// Linux    | __linux__         | ARCH_OS_LINUX   | LINUX
//----------+-------------------+-----------------+--------------
// OSX      | __APPLE__ &&      | ARCH_OS_OSX     | OSX
//          | !TARGET_OS_IPHONE |
//----------+-------------------+-----------------+--------------
// Windows  | _WIN32 || _WIN64  | ARCH_OS_WINDOWS | WIN32
//----------+-------------------+-----------------+--------------

#if defined(ARCH_OS_LINUX)
    #if !defined(LINUX)
        #define LINUX
    #endif
#elif defined(ARCH_OS_OSX)
    #if !defined(OSX)
        #define OSX
    #endif
#elif defined(ARCH_OS_WINDOWS)
    #if !defined(WIN32)
        #define WIN32
    #endif
#endif

#endif // EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_ARCH_DEFS_H
