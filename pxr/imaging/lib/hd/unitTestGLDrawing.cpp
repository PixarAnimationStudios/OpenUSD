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

#include "pxr/imaging/hd/unitTestGLDrawing.h"
#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/drawTarget.h"
#include "pxr/imaging/glfq/glDebugContext.h"

#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec4d.h"

#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtGui/QMouseEvent>
#include <QtOpenGL/QGLWidget>

#include <cstdlib>

////////////////////////////////////////////////////////////

static QGLFormat
_GetGLFormat()
{
    QGLFormat fmt;
    fmt.setDoubleBuffer(true);
    fmt.setDepth(true);
    fmt.setAlpha(true);
    fmt.setStencil(true);
    //fmt.setSampleBuffers(1);
    //fmt.setSamples(4);
    return fmt;
}

class Hd_UnitTestDrawingQGLWidget : public QGLWidget {
public:
    typedef Hd_UnitTestDrawingQGLWidget This;

public:
    Hd_UnitTestDrawingQGLWidget(Hd_UnitTestGLDrawing * unitTest,
                                QWidget * parent = NULL);
    virtual ~Hd_UnitTestDrawingQGLWidget();

    void OffscreenTest();

    bool WriteToFile(std::string const & attachment, 
                     std::string const & filename);

    void StartTimer();

protected:
    // QGLWidget overrides
    void initializeGL();
    void paintGL();

    // QWidget overrides
    virtual void keyReleaseEvent(QKeyEvent * event);
    virtual void mousePressEvent(QMouseEvent * event);
    virtual void mouseReleaseEvent(QMouseEvent * event);
    virtual void mouseMoveEvent(QMouseEvent * event);

    bool eventFilter(QObject *object, QEvent *event);

private:
    Hd_UnitTestGLDrawing *_unitTest;
    GlfDrawTargetRefPtr _drawTarget;
    QTimer *_timer;
};

Hd_UnitTestDrawingQGLWidget::Hd_UnitTestDrawingQGLWidget(
        Hd_UnitTestGLDrawing * unitTest,
        QWidget * parent) :
    QGLWidget(new GlfQGLDebugContext(_GetGLFormat()), parent),
    _unitTest(unitTest)
{
    _timer = new QTimer();
    _timer->installEventFilter(this);
}

Hd_UnitTestDrawingQGLWidget::~Hd_UnitTestDrawingQGLWidget()
{
}

/* virtual */
void
Hd_UnitTestDrawingQGLWidget::initializeGL()
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
    _drawTarget = GlfDrawTarget::New(GfVec2i(width(), height()));
    _drawTarget->Bind();
    _drawTarget->AddAttachment("color", GL_RGBA, GL_FLOAT, GL_RGBA);
    _drawTarget->AddAttachment("depth", GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8,
                               GL_DEPTH24_STENCIL8);
    _unitTest->InitTest();

    _drawTarget->Unbind();
}

/* virtual */
void
Hd_UnitTestDrawingQGLWidget::paintGL()
{
    //
    // Update the draw target's size and execute the unit test with
    // the draw target bound.
    //
    _drawTarget->Bind();
    _drawTarget->SetSize(GfVec2i(width(), height()));

    _unitTest->DrawTest();

    _drawTarget->Unbind();

    //
    // Blit the resulting color buffer to the window (this is a noop
    // if we're drawing offscreen).
    //
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _drawTarget->GetFramebufferId());

    glBlitFramebuffer(0, 0, width(), height(),
                      0, 0, width(), height(),
                      GL_COLOR_BUFFER_BIT,
                      GL_NEAREST);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

void
Hd_UnitTestDrawingQGLWidget::OffscreenTest()
{
    //
    // Ask Qt to initialize and draw
    //
    glInit();
    glDraw();

    _drawTarget->Bind();
    _drawTarget->SetSize(GfVec2i(width(), height()));

    _unitTest->OffscreenTest();

    _drawTarget->Unbind();
}

void
Hd_UnitTestDrawingQGLWidget::StartTimer()
{
    _timer->start(10);
}

bool
Hd_UnitTestDrawingQGLWidget::eventFilter(QObject *object, QEvent *event)
{
    if (object == _timer) {
        _unitTest->Idle();
        update();
    }
    return QGLWidget::eventFilter(object, event);
}

bool
Hd_UnitTestDrawingQGLWidget::WriteToFile(std::string const & attachment,
        std::string const & filename)
{
    _drawTarget->Unbind();
    bool ret = _drawTarget->WriteToFile(attachment, filename);
    _drawTarget->Bind();
    return ret;
}

/* virtual */
void
Hd_UnitTestDrawingQGLWidget::keyReleaseEvent(QKeyEvent * event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
    case Qt::Key_Q:
        QApplication::instance()->exit(0);
        return;
    }
    _unitTest->KeyRelease(event->key());
    glDraw();
}

/* virtual */
void
Hd_UnitTestDrawingQGLWidget::mousePressEvent(QMouseEvent * event)
{
    int button = 0;
    switch(event->button()){
    case Qt::LeftButton: button = 0; break;
    case Qt::MidButton: button = 1; break;
    case Qt::RightButton: button = 2; break;
    default: break;
    }
    _unitTest->MousePress(button, event->x(), event->y());
}

/* virtual */
void
Hd_UnitTestDrawingQGLWidget::mouseReleaseEvent(QMouseEvent * event)
{
    int button = 0;
    switch(event->button()){
    case Qt::LeftButton: button = 0; break;
    case Qt::MidButton: button = 1; break;
    case Qt::RightButton: button = 2; break;
    default: break;
    }
    _unitTest->MouseRelease(button, event->x(), event->y());
}

/* virtual */
void
Hd_UnitTestDrawingQGLWidget::mouseMoveEvent(QMouseEvent * event)
{
    _unitTest->MouseMove(event->x(), event->y());
    glDraw();
}

////////////////////////////////////////////////////////////

Hd_UnitTestGLDrawing::Hd_UnitTestGLDrawing()
    : _widget(NULL)
{
    _rotate[0] = _rotate[1] = 0;
    _translate[0] = _translate[1] = _translate[2] = 0;

    _mousePos[0] = _mousePos[1] = 0;
    _mouseButton[0] = _mouseButton[1] = _mouseButton[2] = false;
}

Hd_UnitTestGLDrawing::~Hd_UnitTestGLDrawing()
{
}

int
Hd_UnitTestGLDrawing::GetWidth() const
{
    return _widget->width();
}

int
Hd_UnitTestGLDrawing::GetHeight() const
{
    return _widget->height();
}

bool
Hd_UnitTestGLDrawing::WriteToFile(std::string const & attachment,
        std::string const & filename) const
{
    return _widget->WriteToFile(attachment, filename);
}

void
Hd_UnitTestGLDrawing::RunTest(int argc, char *argv[])
{
    QApplication app(argc, argv);

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

    _widget = new Hd_UnitTestDrawingQGLWidget(this);
    _widget->setWindowTitle("Hd Test");
    _widget->resize(640, 480);

    if (offscreen) {
        // no GUI mode (automated test)
        _widget->hide();
        _widget->makeCurrent();
        _widget->OffscreenTest();
        _widget->doneCurrent();
    } else {
        // Interactive mode
        _widget->show();
        if (animate) _widget->StartTimer();
        app.exec();
    }
}

/* virtual */
void
Hd_UnitTestGLDrawing::Idle()
{
}

/* virtual */
void
Hd_UnitTestGLDrawing::ParseArgs(int argc, char *argv[])
{
}

/* virtual */
void
Hd_UnitTestGLDrawing::MousePress(int button, int x, int y)
{
    _mouseButton[button] = true;
    _mousePos[0] = x;
    _mousePos[1] = y;
}

/* virtual */
void
Hd_UnitTestGLDrawing::MouseRelease(int button, int x, int y)
{
    _mouseButton[button] = false;
}

/* virtual */
void
Hd_UnitTestGLDrawing::MouseMove(int x, int y)
{
    int dx = x - _mousePos[0];
    int dy = y - _mousePos[1];

    if (_mouseButton[0]) {
        _rotate[1] += dx;
        _rotate[0] += dy;
    } else if (_mouseButton[1]) {
        _translate[0] += 0.1*dx;
        _translate[1] -= 0.1*dy;
    } else if (_mouseButton[2]) {
        _translate[2] += 0.1*dx;
    }

    _mousePos[0] = x;
    _mousePos[1] = y;
}

/* virtual */
void
Hd_UnitTestGLDrawing::KeyRelease(int key)
{
}

GfMatrix4d
Hd_UnitTestGLDrawing::GetViewMatrix() const
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
Hd_UnitTestGLDrawing::GetProjectionMatrix() const
{
    return GetFrustum().ComputeProjectionMatrix();
}

GfFrustum
Hd_UnitTestGLDrawing::GetFrustum() const
{
    int width = GetWidth();
    int height = GetHeight();
    double aspectRatio = double(width)/height;

    GfFrustum frustum;
    frustum.SetPerspective(45.0, aspectRatio, 1, 100000.0);
    return frustum;
}
