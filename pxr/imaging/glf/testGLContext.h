//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GLF_TEST_GLCONTEXT_H
#define PXR_IMAGING_GLF_TEST_GLCONTEXT_H

/// \file glf/testGLContext.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/imaging/glf/glContext.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class Glf_TestGLContextPrivate;

typedef std::shared_ptr<class GlfTestGLContext> GlfTestGLContextSharedPtr;

/// \class GlfTestGLContext
///
/// Testing support class for GlfGLContext.
///
class GlfTestGLContext : public GlfGLContext {
public:
    GLF_API
    static void RegisterGLContextCallbacks();

    // GlfGLContext overrides
    GLF_API
    virtual bool IsValid() const;

    GLF_API
    static GlfTestGLContextSharedPtr Create( GlfTestGLContextSharedPtr const & share );

protected:
    // GlfGLContext overrides
    GLF_API
    virtual void _MakeCurrent();
    GLF_API
    virtual bool _IsSharing(const GlfGLContextSharedPtr& rhs) const;
    GLF_API
    virtual bool _IsEqual(const GlfGLContextSharedPtr& rhs) const;

private:
    GlfTestGLContext(Glf_TestGLContextPrivate const * context);

    friend class GlfTestGLContextRegistrationInterface;

private:
    Glf_TestGLContextPrivate * _context;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_GLF_TEST_GLCONTEXT_H
