//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GLF_GL_RAW_CONTEXT_H
#define PXR_IMAGING_GLF_GL_RAW_CONTEXT_H

/// \file glf/glRawContext.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/garch/glPlatformContext.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


typedef std::shared_ptr<class GlfGLRawContext> GlfGLRawContextSharedPtr;

class GlfGLRawContext : public GlfGLContext {
public:
    /// Returns a new object with the current context.
    GLF_API
    static GlfGLRawContextSharedPtr New();

    /// Returns a new object with the given state.
    GLF_API
    static GlfGLRawContextSharedPtr New(const GarchGLPlatformContextState&);

    GLF_API
    virtual ~GlfGLRawContext();

    /// Returns the held state.
    const GarchGLPlatformContextState& GetState() const { return _state; }

    // GlfGLContext overrides
    GLF_API
    virtual bool IsValid() const;

protected:
    // GlfGLContext overrides
    GLF_API
    virtual void _MakeCurrent();
    GLF_API
    virtual bool _IsSharing(const GlfGLContextSharedPtr& rhs) const;
    GLF_API
    virtual bool _IsEqual(const GlfGLContextSharedPtr& rhs) const;

private:
    GlfGLRawContext(const GarchGLPlatformContextState&);

private:
    GarchGLPlatformContextState _state;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_GLF_GL_RAW_CONTEXT_H
