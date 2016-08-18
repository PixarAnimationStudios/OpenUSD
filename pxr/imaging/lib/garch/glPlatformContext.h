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
#ifndef GARCH_GLPLATFORMCONTEXT_H
#define GARCH_GLPLATFORMCONTEXT_H

/// \file garch/glPlatformContext.h

#include "pxr/base/arch/defines.h"
#include <cstddef>

#if defined ARCH_OS_LINUX

#include "pxr/imaging/garch/glPlatformContextGLX.h"

#elif defined ARCH_OS_DARWIN

#include "pxr/imaging/garch/glPlatformContextDarwin.h"

#elif defined ARCH_OS_WINDOWS

#include "pxr/imaging/garch/glPlatformContextWindows.h"

#else

#error "Unknown platform"

#endif

GARCH_API GarchGLPlatformContextState GarchGetNullGLPlatformContextState();

inline
size_t
hash_value(const GarchGLPlatformContextState& x)
{
    return x.GetHash();
}

#endif  // GARCH_GLPLATFORMCONTEXT_H
