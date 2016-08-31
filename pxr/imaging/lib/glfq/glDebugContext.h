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
#ifndef GLFQ_GLDEBUG_CONTEXT_H
#define GLFQ_GLDEBUG_CONTEXT_H

#include "pxr/imaging/glfq/api.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/weakBase.h"

#include <QtOpenGL/QGLContext>

#include <boost/scoped_ptr.hpp>

class GarchGLPlatformDebugContext;

TF_DECLARE_WEAK_PTRS(GlfQGLDebugContext);

/// \class GlfQGLDebugContext
///
/// Extends QGLContext to support OpenGL Debug Output.
///
/// Unextended Qt does not support the creation of a GL context that
/// enables GL Debug Output. This class extends QGLContext by
/// creating a context which does support GL Debug Output.
///
class GlfQGLDebugContext : public QGLContext, public TfWeakBase {
public:
    typedef QGLContext Parent;

public:
    GLFQ_API
    GlfQGLDebugContext(const QGLFormat & format);
    GLFQ_API
    virtual ~GlfQGLDebugContext();

    // QGLContext overrides
    GLFQ_API
    virtual bool create(const QGLContext * shareContext);
    GLFQ_API
    virtual void makeCurrent();
#if defined(ARCH_OS_DARWIN)
    GLFQ_API
    virtual void* chooseMacVisual(GDHandle handle);
#endif

public:
    boost::scoped_ptr<GarchGLPlatformDebugContext> _platformContext;
};

#endif // GLFQ_GL_DEBUG_CONTEXT_H
