//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_ARCH_DEFS_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_ARCH_DEFS_H

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

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_ARCH_DEFS_H
