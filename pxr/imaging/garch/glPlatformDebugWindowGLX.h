//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GARCH_GL_PLATFORM_DEBUG_WINDOW_GLX_H
#define PXR_IMAGING_GARCH_GL_PLATFORM_DEBUG_WINDOW_GLX_H

#include "pxr/pxr.h"
#include "pxr/base/tf/declarePtrs.h"
#include <X11/Xlib.h>
#include <GL/glx.h>

PXR_NAMESPACE_OPEN_SCOPE


class GarchGLDebugWindow;
TF_DECLARE_WEAK_AND_REF_PTRS(GarchGLPlatformDebugContext);

/// \class Garch_GLPlatformDebugWindow
///
class Garch_GLPlatformDebugWindow
{
public:
    Garch_GLPlatformDebugWindow(GarchGLDebugWindow *w);

    void Init(const char *title, int width, int height, int nSamples=1);
    void Run();
    void ExitApp();

private:
    bool _running;
    GarchGLDebugWindow *_callback;
    Display *_display;
    Window _window;
    GLXContext _glContext;
    GarchGLPlatformDebugContextRefPtr _glDebugContext;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_GARCH_GL_PLATFORM_DEBUG_WINDOW_GLX_H
