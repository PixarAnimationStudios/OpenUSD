//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GARCH_GL_DEBUG_WINDOW_H
#define PXR_IMAGING_GARCH_GL_DEBUG_WINDOW_H

#include "pxr/pxr.h"
#include "pxr/imaging/garch/api.h"
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


class Garch_GLPlatformDebugWindow;

/// \class GarchGLDebugWindow
///
/// Platform specific minimum GL widget for unit tests.
///
class GarchGLDebugWindow {
public:
    GARCH_API
    GarchGLDebugWindow(const char *title, int width, int height);
    GARCH_API
    virtual ~GarchGLDebugWindow();

    GARCH_API
    void Init();
    GARCH_API
    void Run();
    GARCH_API
    void ExitApp();

    int GetWidth() const { return _width; }
    int GetHeight() const { return _height; }

    enum Buttons {
        MyButton1 = 0,
        MyButton2 = 1,
        MyButton3 = 2
    };
    enum ModifierKeys {
        NoModifiers = 0,
        Shift = 1,
        Alt   = 2,
        Ctrl  = 4
    };

    GARCH_API
    virtual void OnInitializeGL();
    GARCH_API
    virtual void OnUninitializeGL();
    GARCH_API
    virtual void OnResize(int w, int h);
    GARCH_API
    virtual void OnIdle();
    GARCH_API
    virtual void OnPaintGL();
    GARCH_API
    virtual void OnKeyRelease(int key);
    GARCH_API
    virtual void OnMousePress(int button, int x, int y, int modKeys);
    GARCH_API
    virtual void OnMouseRelease(int button, int x, int y, int modKeys);
    GARCH_API
    virtual void OnMouseMove(int x, int y, int modKeys);

private:
    Garch_GLPlatformDebugWindow *_private;
    std::string _title;
    int _width, _height;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_GARCH_GL_DEBUG_WINDOW_H
