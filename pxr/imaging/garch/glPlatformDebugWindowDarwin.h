//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GARCH_GL_PLATFORM_DEBUG_WINDOW_DARWIN_H
#define PXR_IMAGING_GARCH_GL_PLATFORM_DEBUG_WINDOW_DARWIN_H

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

class GarchGLDebugWindow;

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
    GarchGLDebugWindow *_callback;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_GARCH_GL_PLATFORM_DEBUG_WINDOW_DARWIN_H
