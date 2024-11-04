//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GLF_TEST_GLXCONTEXT_H
#define PXR_IMAGING_GLF_TEST_GLXCONTEXT_H

/// \file glf/testGLXContext.h

#include "pxr/pxr.h"

#include <GL/glx.h>

PXR_NAMESPACE_OPEN_SCOPE

class Glf_TestGLContextPrivate {
public:
    Glf_TestGLContextPrivate( Glf_TestGLContextPrivate const * other=nullptr );

    void  makeCurrent( ) const;

    bool isValid();

    bool operator==(const Glf_TestGLContextPrivate& rhs) const;

    static const Glf_TestGLContextPrivate * currentContext();

    static bool areSharing( const Glf_TestGLContextPrivate * context1,
        const Glf_TestGLContextPrivate * context2 );

private:
    Display * _dpy = nullptr;

    GLXContext _context = nullptr;

    Glf_TestGLContextPrivate const * _sharedContext;

    static GLXWindow _win;

    static Glf_TestGLContextPrivate const * _currenGLContext;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_GLF_TEST_GLXCONTEXT_H
