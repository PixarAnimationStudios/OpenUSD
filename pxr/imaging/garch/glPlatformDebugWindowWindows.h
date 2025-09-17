//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GARCH_GL_PLATFORM_DEBUG_WINDOW_WINDOWS_H
#define PXR_IMAGING_GARCH_GL_PLATFORM_DEBUG_WINDOW_WINDOWS_H

#include "pxr/pxr.h"
#include <Windows.h>

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
    static Garch_GLPlatformDebugWindow *_GetWindowByHandle(HWND);
    static LRESULT WINAPI _MsgProc(HWND hWnd, UINT msg,
                                   WPARAM wParam, LPARAM lParam);

    bool _running;
    GarchGLDebugWindow *_callback;
    HWND  _hWND;
    HDC   _hDC;
    HGLRC _hGLRC;
    static LPCTSTR _className;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_GARCH_GL_PLATFORM_DEBUG_WINDOW_WINDOWS_H
