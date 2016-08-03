#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/drawTarget.h"
#include "pxr/imaging/glfq/glDebugContext.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"

#include <QtGui/QApplication>
#include <QtGui/QMouseEvent>
#include <QtOpenGL/QGLWidget>

#include <iostream>
#include <sstream>

static void
TestDebugOutput()
{
    std::cerr << "Expected Error Begin\n";
    glEnable(GL_TRUE); // raise error
    glLineWidth(-1.0); // raise error
    std::cerr << "Expected Error End\n";

    // clear errors we just raised
    while (glGetError() != GL_NONE) {}
}

////////////////////////////////////////////////////////////

static QGLFormat
_GetGLFormat()
{
    QGLFormat fmt;
    fmt.setDoubleBuffer(true);
    fmt.setDepth(true);
    fmt.setAlpha(true);
    fmt.setStencil(true);
    return fmt;
}

class TestDebugGLWidget : public QGLWidget {
public:
    TestDebugGLWidget(QWidget * parent = NULL);
    virtual ~TestDebugGLWidget();

    void DrawOffscreen();

    bool WriteToFile(std::string const & attachment, std::string const & filename);

protected:
    // QGLWidget overrides
    virtual void initializeGL();
    virtual void paintGL();

    // QWidget overrides
    virtual void keyReleaseEvent(QKeyEvent * event);

private:
    GlfDrawTargetRefPtr _drawTarget;
};

TestDebugGLWidget::TestDebugGLWidget(QWidget * parent)
    : QGLWidget(new GlfQGLDebugContext(_GetGLFormat()), parent)
{
    // nothing
}

TestDebugGLWidget::~TestDebugGLWidget()
{
    // nothing
}

/* virtual */
void
TestDebugGLWidget::initializeGL()
{
    GlfGlewInit();
    GlfRegisterDefaultDebugOutputMessageCallback();

    std::cout << glGetString(GL_VENDOR) << "\n";
    std::cout << glGetString(GL_RENDERER) << "\n";
    std::cout << glGetString(GL_VERSION) << "\n";

    //
    // Create an offscreen draw target which is the same size as this widget.
    //
    _drawTarget = GlfDrawTarget::New(GfVec2i(width(), height()));
    _drawTarget->Bind();
    _drawTarget->AddAttachment(
        "color", GL_RGBA, GL_FLOAT, GL_RGBA);
    _drawTarget->AddAttachment(
        "depth", GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_COMPONENT);
    _drawTarget->Unbind();
}

/* virtual */
void
TestDebugGLWidget::paintGL()
{
    //
    // Update the draw target's size and execute the unit test with
    // the draw target bound.
    //
    _drawTarget->Bind();
    _drawTarget->SetSize(GfVec2i(width(), height()));

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    TestDebugOutput();

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
TestDebugGLWidget::DrawOffscreen()
{
    //
    // Ask Qt to initialize and draw
    //
    glInit();
    glDraw();
}

bool
TestDebugGLWidget::WriteToFile(std::string const & attachment, std::string const & filename)
{
    return _drawTarget->WriteToFile(attachment, filename);
}

/* virtual */
void
TestDebugGLWidget::keyReleaseEvent(QKeyEvent * event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
    case Qt::Key_Q:
        QApplication::instance()->exit(0);
        exit(0);
    }
}

bool
BasicTest(int argc, char *argv[])
{
    QApplication app(argc, argv);

    bool offscreen = false;
    for (int i=0; i<argc; ++i) {
        if (std::string(argv[i]) == "--offscreen") {
            offscreen = true;
        }
    }

    TestDebugGLWidget *widget = new TestDebugGLWidget();
    widget->setWindowTitle("Test");
    widget->resize(640, 480);

    TfErrorMark errorMark;

    if (offscreen) {
        widget->hide();
        widget->makeCurrent();
        widget->DrawOffscreen();
        widget->doneCurrent();
    } else {
        widget->show();
        app.exec();
    }

    if (errorMark.IsClean()) {
        return false;
    }

    return true;
}

int main(int argc, char *argv[])
{
    if (not BasicTest(argc, argv)) {
        std::cout << "FAILED" << std::endl;
        exit(1);
    }
    std::cout << "OK" << std::endl;
    exit(0);
}

