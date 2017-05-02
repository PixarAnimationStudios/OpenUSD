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
#ifndef GARCH_GLDEBUGWINDOW_H
#define GARCH_GLDEBUGWINDOW_H

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

#endif  // GARCH_GLDEBUGWINDOW_H
