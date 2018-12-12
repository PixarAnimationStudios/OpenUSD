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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hdx/unitTestGLDrawing.h"
#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/drawTarget.h"
#include "pxr/imaging/garch/glDebugWindow.h"

#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec4d.h"

#include <cstdlib>
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE


////////////////////////////////////////////////////////////

class Hdx_UnitTestWindow : public GarchGLDebugWindow {
public:
    typedef Hdx_UnitTestWindow This;

public:
    Hdx_UnitTestWindow(Hdx_UnitTestGLDrawing * unitTest,
                       int width, int height);
    virtual ~Hdx_UnitTestWindow();

    void OffscreenTest();

    bool WriteToFile(std::string const & attachment, 
                     std::string const & filename);

    void StartTimer();

    // GarchGLDebugWindow overrides
    virtual void OnInitializeGL();
    virtual void OnUninitializeGL();
    virtual void OnPaintGL();
    virtual void OnKeyRelease(int key);
    virtual void OnMousePress(int button, int x, int y, int modKeys);
    virtual void OnMouseRelease(int button, int x, int y, int modKeys);
    virtual void OnMouseMove(int x, int y, int modKeys);

private:
    Hdx_UnitTestGLDrawing *_unitTest;
    GlfDrawTargetRefPtr _drawTarget;
};

Hdx_UnitTestWindow::Hdx_UnitTestWindow(
        Hdx_UnitTestGLDrawing * unitTest, int w, int h)
    : GarchGLDebugWindow("Hdx Test", w, h)
    , _unitTest(unitTest)
{
}

Hdx_UnitTestWindow::~Hdx_UnitTestWindow()
{
}

/* virtual */
void
Hdx_UnitTestWindow::OnInitializeGL()
{
    GlfGlewInit();
    GlfRegisterDefaultDebugOutputMessageCallback();

    std::cout << glGetString(GL_VENDOR) << "\n";
    std::cout << glGetString(GL_RENDERER) << "\n";
    std::cout << glGetString(GL_VERSION) << "\n";

    //
    // Create an offscreen draw target which is the same size as this
    // widget and initialize the unit test with the draw target bound.
    //
    _drawTarget = GlfDrawTarget::New(GfVec2i(GetWidth(), GetHeight()));
    _drawTarget->Bind();
    _drawTarget->AddAttachment("color", GL_RGBA, GL_FLOAT, GL_RGBA);
    _drawTarget->AddAttachment("depth", GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8,
                               GL_DEPTH24_STENCIL8);
    _unitTest->InitTest();

    _drawTarget->Unbind();
}

/* virtual */
void
Hdx_UnitTestWindow::OnUninitializeGL()
{
    _unitTest->UninitTest();
}

/* virtual */
void
Hdx_UnitTestWindow::OnPaintGL()
{
    //
    // Update the draw target's size and execute the unit test with
    // the draw target bound.
    //
    _drawTarget->Bind();
    _drawTarget->SetSize(GfVec2i(GetWidth(), GetHeight()));

    _unitTest->DrawTest();

    _drawTarget->Unbind();

    //
    // Blit the resulting color buffer to the window (this is a noop
    // if we're drawing offscreen).
    //
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _drawTarget->GetFramebufferId());

    glBlitFramebuffer(0, 0, GetWidth(), GetHeight(),
                      0, 0, GetWidth(), GetHeight(),
                      GL_COLOR_BUFFER_BIT,
                      GL_NEAREST);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

void
Hdx_UnitTestWindow::OffscreenTest()
{
    _drawTarget->Bind();
    _drawTarget->SetSize(GfVec2i(GetWidth(), GetHeight()));

    _unitTest->OffscreenTest();

    _drawTarget->Unbind();
}

bool
Hdx_UnitTestWindow::WriteToFile(std::string const & attachment,
        std::string const & filename)
{
    _drawTarget->Unbind();
    bool ret = _drawTarget->WriteToFile(attachment, filename);
    _drawTarget->Bind();
    return ret;
}

/* virtual */
void
Hdx_UnitTestWindow::OnKeyRelease(int key)
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
Hdx_UnitTestWindow::OnMousePress(int button, int x, int y, int modKeys)
{
    _unitTest->MousePress(button, x, y, modKeys);
}

/* virtual */
void
Hdx_UnitTestWindow::OnMouseRelease(int button, int x, int y, int modKeys)
{
    _unitTest->MouseRelease(button, x, y, modKeys);
}

/* virtual */
void
Hdx_UnitTestWindow::OnMouseMove(int x, int y, int modKeys)
{
    _unitTest->MouseMove(x, y, modKeys);
}

////////////////////////////////////////////////////////////

Hdx_UnitTestGLDrawing::Hdx_UnitTestGLDrawing()
    : _widget(NULL)
{
    _rotate[0] = _rotate[1] = 0;
    _translate[0] = _translate[1] = _translate[2] = 0;

    _mousePos[0] = _mousePos[1] = 0;
    _mouseButton[0] = _mouseButton[1] = _mouseButton[2] = false;
}

Hdx_UnitTestGLDrawing::~Hdx_UnitTestGLDrawing()
{
}

int
Hdx_UnitTestGLDrawing::GetWidth() const
{
    return _widget->GetWidth();
}

int
Hdx_UnitTestGLDrawing::GetHeight() const
{
    return _widget->GetHeight();
}

bool
Hdx_UnitTestGLDrawing::WriteToFile(std::string const & attachment,
        std::string const & filename) const
{
    return _widget->WriteToFile(attachment, filename);
}

void
Hdx_UnitTestGLDrawing::RunTest(int argc, char *argv[])
{
    bool offscreen = false;
    for (int i=0; i<argc; ++i) {
        if (std::string(argv[i]) == "--offscreen") {
            offscreen = true;
        }
    }

    this->ParseArgs(argc, argv);

    _widget = new Hdx_UnitTestWindow(this, 640, 480);
    _widget->Init();

    if (offscreen) {
        // no GUI mode (automated test)
        _widget->OffscreenTest();
    } else {
        // Interactive mode
        _widget->Run();
    }
}

/* virtual */
void
Hdx_UnitTestGLDrawing::Idle()
{
}

/* virtual */
void
Hdx_UnitTestGLDrawing::ParseArgs(int argc, char *argv[])
{
}

/* virtual */
void
Hdx_UnitTestGLDrawing::MousePress(int button, int x, int y, int modKeys)
{
    _mouseButton[button] = true;
    _mousePos[0] = x;
    _mousePos[1] = y;
}

/* virtual */
void
Hdx_UnitTestGLDrawing::MouseRelease(int button, int x, int y, int modKeys)
{
    _mouseButton[button] = false;
}

/* virtual */
void
Hdx_UnitTestGLDrawing::MouseMove(int x, int y, int modKeys)
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
Hdx_UnitTestGLDrawing::KeyRelease(int key)
{
}

GfMatrix4d
Hdx_UnitTestGLDrawing::GetViewMatrix() const
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
Hdx_UnitTestGLDrawing::GetProjectionMatrix() const
{
    return GetFrustum().ComputeProjectionMatrix();
}

GfFrustum
Hdx_UnitTestGLDrawing::GetFrustum() const
{
    int width = GetWidth();
    int height = GetHeight();
    double aspectRatio = double(width)/height;

    GfFrustum frustum;
    frustum.SetPerspective(45.0, aspectRatio, 1, 100000.0);
    return frustum;
}

PXR_NAMESPACE_CLOSE_SCOPE

