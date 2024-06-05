//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_ARCH_BUILD_MODE_H
#define PXR_BASE_ARCH_BUILD_MODE_H

#include "pxr/pxr.h"
#include "pxr/base/arch/defines.h"

PXR_NAMESPACE_OPEN_SCOPE

struct ArchBuildMode {
// Check if the build system has specified a build mode, falling
// back to commonly-used macros if it has not. (Typically, _DEBUG
// is defined by Visual Studio and DEBUG by Xcode for debug-mode builds)
#if defined(BUILD_OPTLEVEL_DEV) || defined(_DEBUG) || defined(DEBUG)
    enum { DEV_BUILD = 1 };
#else
    enum { DEV_BUILD = 0 };
#endif
};

#define ARCH_DEV_BUILD ArchBuildMode::DEV_BUILD

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_ARCH_BUILD_MODE_H
