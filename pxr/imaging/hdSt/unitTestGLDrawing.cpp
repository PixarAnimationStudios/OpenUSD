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

#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/garch/glDebugWindow.h"

#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec4d.h"

#include <cstdlib>
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE


class HdSt_UnitTestWindow : public GarchGLDebugWindow
{
public:
    typedef HdSt_UnitTestWindow This;

public:
    HdSt_UnitTestWindow(HdSt_UnitTestGLDrawing * unitTest,
                                int width, int height);
    virtual ~HdSt_UnitTestWindow();

    void OffscreenTest();

    void StartTimer();

    // GarchGLDebugWindow overrides
    virtual void OnInitializeGL();
    virtual void OnUninitializeGL();
    virtual void OnIdle();
    virtual void OnPaintGL();
    virtual void OnKeyRelease(int key);
    virtual void OnMousePress(int button, int x, int y, int modKeys);
    virtual void OnMouseRelease(int button, int x, int y, int modKeys);
    virtual void OnMouseMove(int x, int y, int modKeys);

private:
    HdSt_UnitTestGLDrawing *_unitTest;
    bool _animate;
};

HdSt_UnitTestWindow::HdSt_UnitTestWindow(
    HdSt_UnitTestGLDrawing * unitTest, int w, int h)
    : GarchGLDebugWindow("Hd Test", w, h)
    , _unitTest(unitTest)
    , _animate(false)
{
}

HdSt_UnitTestWindow::~HdSt_UnitTestWindow()
{
}

/* virtual */
void
HdSt_UnitTestWindow::OnInitializeGL()
{
    GarchGLApiLoad();
    GlfRegisterDefaultDebugOutputMessageCallback();

    std::cout << glGetString(GL_VENDOR) << "\n";
    std::cout << glGetString(GL_RENDERER) << "\n";
    std::cout << glGetString(GL_VERSION) << "\n";

    _unitTest->InitTest();
}

/* virtual */
void
HdSt_UnitTestWindow::OnUninitializeGL()
{
    _unitTest->UninitTest();
}

/* virtual */
void
HdSt_UnitTestWindow::OnPaintGL()
{
    // Execute the unit test
    _unitTest->DrawTest();
    _unitTest->Present(/*framebuffer*/0);
}

void
HdSt_UnitTestWindow::OffscreenTest()
{
    _unitTest->OffscreenTest();
}

void
HdSt_UnitTestWindow::StartTimer()
{
    _animate = true;
}

/* virtual */
void
HdSt_UnitTestWindow::OnIdle()
{
    if (_animate) {
        _unitTest->Idle();
    }
}

/* virtual */
void
HdSt_UnitTestWindow::OnKeyRelease(int key)
{
    switch (key) {
    case 'q':
        ExitApp();
        return;
    }
    _unitTest->KeyRelease(key);
}

/* virtual */
void
HdSt_UnitTestWindow::OnMousePress(int button, int x, int y, int modKeys)
{
    _unitTest->MousePress(button, x, y, modKeys);
}

/* virtual */
void
HdSt_UnitTestWindow::OnMouseRelease(int button, int x, int y, int modKeys)
{
    _unitTest->MouseRelease(button, x, y, modKeys);
}

/* virtual */
void
HdSt_UnitTestWindow::OnMouseMove(int x, int y, int modKeys)
{
    _unitTest->MouseMove(x, y, modKeys);
}

////////////////////////////////////////////////////////////

HdSt_UnitTestGLDrawing::HdSt_UnitTestGLDrawing()
    : _widget(NULL)
{
    _rotate[0] = _rotate[1] = 0;
    _translate[0] = _translate[1] = _translate[2] = 0;

    _mousePos[0] = _mousePos[1] = 0;
    _mouseButton[0] = _mouseButton[1] = _mouseButton[2] = false;
}

HdSt_UnitTestGLDrawing::~HdSt_UnitTestGLDrawing()
{
}

int
HdSt_UnitTestGLDrawing::GetWidth() const
{
    return _widget->GetWidth();
}

int
HdSt_UnitTestGLDrawing::GetHeight() const
{
    return _widget->GetHeight();
}

void
HdSt_UnitTestGLDrawing::RunTest(int argc, char *argv[])
{
    bool offscreen = false;
    bool animate = false;
    for (int i=0; i<argc; ++i) {
        if (std::string(argv[i]) == "--offscreen") {
            offscreen = true;
        } else
        if (std::string(argv[i]) == "--animate") {
            animate = true;
        }
    }

    this->ParseArgs(argc, argv);

    _widget = new HdSt_UnitTestWindow(this, 640, 480);
    _widget->Init();

    if (offscreen) {
        // no GUI mode (automated test)
        RunOffscreenTest();
    } else {
        // Interactive mode
        if (animate) _widget->StartTimer();
        _widget->Run();
    }
}

void
HdSt_UnitTestGLDrawing::RunOffscreenTest()
{
    _widget->OffscreenTest();
}

/* virtual */
void
HdSt_UnitTestGLDrawing::Idle()
{
}

/* virtual */
void
HdSt_UnitTestGLDrawing::ParseArgs(int argc, char *argv[])
{
}

/* virtual */
void 
HdSt_UnitTestGLDrawing::UninitTest()
{
}

/* virtual */
void
HdSt_UnitTestGLDrawing::MousePress(int button, int x, int y, int modKeys)
{
    _mouseButton[button] = true;
    _mousePos[0] = x;
    _mousePos[1] = y;
}

/* virtual */
void
HdSt_UnitTestGLDrawing::MouseRelease(int button, int x, int y, int modKeys)
{
    _mouseButton[button] = false;
}

/* virtual */
void
HdSt_UnitTestGLDrawing::MouseMove(int x, int y, int modKeys)
{
    int dx = x - _mousePos[0];
    int dy = y - _mousePos[1];

    if (modKeys & GarchGLDebugWindow::Alt) {
        if (_mouseButton[0]) {
            _rotate[1] += dx;
            _rotate[0] += dy;
        } else if (_mouseButton[1]) {
            _translate[0] += 0.1*dx;
            _translate[1] -= 0.1*dy;
        } else if (_mouseButton[2]) {
            _translate[2] += 0.1*dx;
        }
    }

    _mousePos[0] = x;
    _mousePos[1] = y;
}

/* virtual */
void
HdSt_UnitTestGLDrawing::KeyRelease(int key)
{
}

GfMatrix4d
HdSt_UnitTestGLDrawing::GetViewMatrix() const
{
    GfMatrix4d viewMatrix;
    viewMatrix.SetIdentity();
    // rotate from z-up to y-up
    viewMatrix *= GfMatrix4d().SetRotate(GfRotation(GfVec3d(1.0,0.0,0.0), -90.0));
    viewMatrix *= GfMatrix4d().SetRotate(GfRotation(GfVec3d(0, 1, 0), _rotate[1]));
    viewMatrix *= GfMatrix4d().SetRotate(GfRotation(GfVec3d(1, 0, 0), _rotate[0]));
    viewMatrix *= GfMatrix4d().SetTranslate(GfVec3d(_translate[0], _translate[1], _translate[2]));

    return viewMatrix;
}

GfMatrix4d
HdSt_UnitTestGLDrawing::GetProjectionMatrix() const
{
    return GetFrustum().ComputeProjectionMatrix();
}

GfFrustum
HdSt_UnitTestGLDrawing::GetFrustum() const
{
    int width = GetWidth();
    int height = GetHeight();
    double aspectRatio = double(width)/height;

    GfFrustum frustum;
    frustum.SetPerspective(45.0, aspectRatio, 1, 100000.0);
    return frustum;
}


PXR_NAMESPACE_CLOSE_SCOPE

