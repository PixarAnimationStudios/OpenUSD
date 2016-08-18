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
#ifndef GLFQ_GLPLATFORM_DEBUG_CONTEXT_H
#define GLFQ_GLPLATFORM_DEBUG_CONTEXT_H

#include "pxr/imaging/glfq/api.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/weakBase.h"

#include <boost/scoped_ptr.hpp>

#if !defined(ARCH_OS_WINDOWS)
class GlfQGLPlatformDebugContextPrivate;
#endif

TF_DECLARE_WEAK_PTRS(GlfQGLPlatformDebugContext);

/// \class GlfQGLPlatformDebugContext
///
/// Platform specific context (e.g. X11/GLX) which supports debug output.
///
class GlfQGLPlatformDebugContext : public TfWeakBase {
public:
    GLFQ_API
    GlfQGLPlatformDebugContext(int majorVersion,
                               int minorVersion,
                               bool coreProfile,
                               bool directRenderering);

    GLFQ_API
    virtual ~GlfQGLPlatformDebugContext();

    GLFQ_API
    static bool IsEnabledDebugOutput();

    GLFQ_API
    static bool IsEnabledCoreProfile();

    GLFQ_API
    void makeCurrent();

    GLFQ_API
    void *chooseMacVisual();

public:
#if !defined(ARCH_OS_WINDOWS)
    boost::scoped_ptr<GlfQGLPlatformDebugContextPrivate> _private;
    bool _coreProfile;
#endif
};

#endif // GLFQ_GLPLATFORM_DEBUG_CONTEXT_H
