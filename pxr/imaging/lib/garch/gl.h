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
#ifndef GARCH_GL_H
#define GARCH_GL_H

#include "pxr/pxr.h"
#include "pxr/base/arch/defines.h"
#if defined(ARCH_OS_DARWIN)
// Apple installs OpenGL headers in a non-standard location.
#include <OpenGL/gl.h>
#elif defined(ARCH_OS_WINDOWS)
// Windows must include Windows.h prior to gl.h
#include <Windows.h>
#include <GL/gl.h>
#else
#include <GL/gl.h>
#endif

#ifdef ARCH_OS_DARWIN

PXR_NAMESPACE_OPEN_SCOPE

typedef GLvoid (*ArchGLCallbackType)(...);

#define GL_RGBA16F 0x881A
#define GL_RGB16F 0x881B
#define GL_RGBA32F 0x8814
#define GL_RGB32F 0x8815

PXR_NAMESPACE_CLOSE_SCOPE

#else // !ARCH_OS_DARWIN

PXR_NAMESPACE_OPEN_SCOPE

typedef GLvoid (*ArchGLCallbackType)();

PXR_NAMESPACE_CLOSE_SCOPE

#endif // ARCH_OS_DARWIN

#endif // GARCH_GL_H
