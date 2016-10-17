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

#include <boost/scoped_ptr.hpp>

class Garch_GLPlatformDebugWindow;

/// \class GarchGLDebugWindow
///
/// Platform specific minimum GL widget for unit tests.
///
class GarchGLDebugWindow {
public:
    GarchGLDebugWindow(const char *title, int width, int height);
    virtual ~GarchGLDebugWindow();

    void Init();
    void Run();
    void ExitApp();

    int GetWidth() const { return _width; }
    int GetHeight() const { return _height; }

    enum Buttons {
        Button1 = 0,
        Button2 = 1,
        Button3 = 2
    };
    enum ModifierKeys {
        None  = 0,
        Shift = 1,
        Alt   = 2,
        Ctrl  = 4
    };

    virtual void OnInitializeGL();
    virtual void OnUninitializeGL();
    virtual void OnResize(int w, int h);
    virtual void OnIdle();
    virtual void OnPaintGL();
    virtual void OnKeyRelease(int key);
    virtual void OnMousePress(int button, int x, int y, int modKeys);
    virtual void OnMouseRelease(int button, int x, int y, int modKeys);
    virtual void OnMouseMove(int x, int y, int modKeys);

private:
    boost::scoped_ptr<Garch_GLPlatformDebugWindow> _private;
    std::string _title;
    int _width, _height;
};

#endif  // GARCH_GLDEBUGWINDOW_H
