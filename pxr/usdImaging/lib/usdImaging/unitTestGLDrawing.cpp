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

#include "pxr/usdImaging/usdImaging/unitTestGLDrawing.h"
#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/drawTarget.h"
#include "pxr/imaging/glfq/glDebugContext.h"

#include "pxr/base/arch/attributes.h"
#include "pxr/base/gf/vec2i.h"

#include "pxr/base/plug/registry.h"
#include "pxr/base/arch/systemInfo.h"

#include <QtGui/QApplication>
#include <QtGui/QMouseEvent>
#include <QtOpenGL/QGLWidget>

#include <stdio.h>
#include <stdarg.h>

static void UsdImaging_UnitTestHelper_InitPlugins()
{
    // Unfortunately, in order to properly find plugins in our test setup, we
    // need to know where the test is running.
    std::string testDir = TfGetPathName(ArchGetExecutablePath());
    std::string pluginDir = TfStringCatPaths(testDir, 
            "UsdImagingPlugins/lib/UsdImagingTest.framework/Resources");
    printf("registering plugins in: %s\n", pluginDir.c_str());

    PlugRegistry::GetInstance().RegisterPlugins(pluginDir);
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
    //fmt.setSampleBuffers(1);
    //fmt.setSamples(4);
    return fmt;
}

class UsdImaging_UnitTestDrawingQGLWidget : public QGLWidget {
public:
    typedef UsdImaging_UnitTestDrawingQGLWidget This;

public:
    UsdImaging_UnitTestDrawingQGLWidget(UsdImaging_UnitTestGLDrawing * unitTest,
                                QWidget * parent = NULL);
    virtual ~UsdImaging_UnitTestDrawingQGLWidget();

    void DrawOffscreen();

    bool WriteToFile(std::string const & attachment, 
                     std::string const & filename);

protected:
    // QGLWidget overrides
    void initializeGL();
    void paintGL();

    // QWidget overrides
    virtual void keyReleaseEvent(QKeyEvent * event);
    virtual void mousePressEvent(QMouseEvent * event);
    virtual void mouseReleaseEvent(QMouseEvent * event);
    virtual void mouseMoveEvent(QMouseEvent * event);

private:
    UsdImaging_UnitTestGLDrawing *_unitTest;
    GlfDrawTargetRefPtr _drawTarget;
};

UsdImaging_UnitTestDrawingQGLWidget::UsdImaging_UnitTestDrawingQGLWidget(
        UsdImaging_UnitTestGLDrawing * unitTest,
        QWidget * parent) :
    QGLWidget(new GlfQGLDebugContext(_GetGLFormat()), parent),
    _unitTest(unitTest)
{
}

UsdImaging_UnitTestDrawingQGLWidget::~UsdImaging_UnitTestDrawingQGLWidget()
{
}

/* virtual */
void
UsdImaging_UnitTestDrawingQGLWidget::initializeGL()
{
    GlfGlewInit();
    GlfRegisterDefaultDebugOutputMessageCallback();

    //
    // Create an offscreen draw target which is the same size as this
    // widget and initialize the unit test with the draw target bound.
    //
    _drawTarget = GlfDrawTarget::New(GfVec2i(width(), height()));
    _drawTarget->Bind();
    _drawTarget->AddAttachment("color", GL_RGBA, GL_FLOAT, GL_RGBA);
    _drawTarget->AddAttachment("depth", GL_DEPTH_COMPONENT, GL_FLOAT,
                                        GL_DEPTH_COMPONENT);

    _unitTest->InitTest();

    _drawTarget->Unbind();
}

/* virtual */
void
UsdImaging_UnitTestDrawingQGLWidget::paintGL()
{
    //
    // Update the draw target's size and execute the unit test with
    // the draw target bound.
    //
    _drawTarget->Bind();
    _drawTarget->SetSize(GfVec2i(width(), height()));

    _unitTest->DrawTest(false);

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
UsdImaging_UnitTestDrawingQGLWidget::DrawOffscreen()
{
    //
    // Ask Qt to initialize and draw
    //
    glInit();

    _drawTarget->Bind();
    _drawTarget->SetSize(GfVec2i(width(), height()));

    _unitTest->DrawTest(true);

    _drawTarget->Unbind();
}

bool
UsdImaging_UnitTestDrawingQGLWidget::WriteToFile(std::string const & attachment,
        std::string const & filename)
{
    // We need to unbind the draw target before writing to file to be sure the
    // attachment is in a good state.
    bool isBound = _drawTarget->IsBound();
    if (isBound)
        _drawTarget->Unbind();

    bool result = _drawTarget->WriteToFile(attachment, filename);

    if (isBound)
        _drawTarget->Bind();
    return result;
}

/* virtual */
void
UsdImaging_UnitTestDrawingQGLWidget::keyReleaseEvent(QKeyEvent * event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
    case Qt::Key_Q:
        QApplication::instance()->exit(0);
	return;
    }
}

/* virtual */
void
UsdImaging_UnitTestDrawingQGLWidget::mousePressEvent(QMouseEvent * event)
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
UsdImaging_UnitTestDrawingQGLWidget::mouseReleaseEvent(QMouseEvent * event)
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
UsdImaging_UnitTestDrawingQGLWidget::mouseMoveEvent(QMouseEvent * event)
{
    _unitTest->MouseMove(event->x(), event->y());
    glDraw();
}

////////////////////////////////////////////////////////////

UsdImaging_UnitTestGLDrawing::UsdImaging_UnitTestGLDrawing()
    : _widget(NULL)
    , _testLighting(false)
    , _testIdRender(false)
    , _complexity(1.0f)
    , _drawMode(UsdImagingEngine::DRAW_SHADED_SMOOTH)
    , _shouldFrameAll(false)
    , _cullBackfaces(false)
{
}

UsdImaging_UnitTestGLDrawing::~UsdImaging_UnitTestGLDrawing()
{
}

int
UsdImaging_UnitTestGLDrawing::GetWidth() const
{
    return _widget->width();
}

int
UsdImaging_UnitTestGLDrawing::GetHeight() const
{
    return _widget->height();
}

bool
UsdImaging_UnitTestGLDrawing::WriteToFile(std::string const & attachment,
        std::string const & filename) const
{
    return _widget->WriteToFile(attachment, filename);
}

void
UsdImaging_UnitTestGLDrawing::_Redraw() const
{
    _widget->update();
}

struct UsdImaging_UnitTestGLDrawing::_Args {
    _Args() : offscreen(false) { }

    std::string unresolvedStageFilePath;
    bool offscreen;
    std::string shading;
    std::vector<double> clipPlaneCoords;
    std::vector<double> complexities;
};

static void Die(const char* fmt, ...) ARCH_PRINTF_FUNCTION(1, 2);
static void Die(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fflush(stderr);
    exit(1);
}

static void
ParseError(const char* pname, const char* fmt, ...) ARCH_PRINTF_FUNCTION(2, 3);
static void
ParseError(const char* pname, const char* fmt, ...)
{
    fprintf(stderr, "%s: ", TfGetBaseName(pname).c_str());
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, ".  Try '%s -' for help.\n", TfGetBaseName(pname).c_str());
    fflush(stderr);
    exit(1);
}

static void Usage(int argc, char *argv[])
{
    static const char usage[] =
"%s [-stage filePath] [-write filePath]\n"
"                           [-offscreen] [-lighting] [-idRender]\n"
"                           [-complexity complexity]\n"
"                           [-shading [flat|smooth|wire|wireOnSurface]]\n"
"                           [-frameAll]\n"
"                           [-clipPlane clipPlane1 ... clipPlane4]\n"
"                           [-complexities complexities1 complexities2 ...]\n"
"                           [-times times1 times2 ...] [-cullBackfaces]\n"
"\n"
"  usdImaging basic drawing test\n"
"\n"
"options:\n"
"  -stage filePath     name of usd stage to open []\n"
"  -write filePath     name of image file to write (suffix determines type) []\n"
"  -offscreen          execute without mapping a window\n"
"  -lighting           use simple lighting override shader\n"
"  -idRender           ID rendering\n"
"  -complexity complexity\n"
"                      Set the fallback complexity [1]\n"
"  -shading [flat|smooth|wire|wireOnSurface]\n"
"                      force specific type of shading\n"
"                      [flat|smooth|wire|wireOnSurface] []\n"
"  -frameAll           set the view to frame all root prims on the stage\n"
"  -clipPlane clipPlane1 ... clipPlane4\n"
"                      set an additional camera clipping plane [()]\n"
"  -complexities complexities1 complexities2 ...\n"
"                      One or more complexities, each complexity will\n"
"                      produce an image [()]\n"
"  -times times1 times2 ...\n"
"                      One or more time samples, each time will produce\n"
"                      an image [()]\n"
"  -cullBackfaces      enable backface culling\n";

    Die(usage, TfGetBaseName(argv[0]).c_str());
}

static void CheckForMissingArguments(int i, int n, int argc, char *argv[])
{
    if (i + n >= argc) {
        if (n == 1) {
            ParseError(argv[0], "missing parameter for '%s'", argv[i]);
        }
        else {
            ParseError(argv[0], "argument '%s' requires %d values", argv[i], n);
        }
    }
}

static double ParseDouble(int& i, int argc, char *argv[], bool* invalid=0)
{
    if (i + 1 == argc) {
        if (invalid) {
            *invalid = true;
            return 0.0;
        }
        ParseError(argv[0], "missing parameter for '%s'", argv[i]);
    }
    char* end;
    double result = strtod(argv[i + 1], &end);
    if (end == argv[i + 1] or *end != '\0') {
        if (invalid) {
            *invalid = true;
            return 0.0;
        }
        ParseError(argv[0], "invalid parameter for '%s': %s",
                   argv[i], argv[i + 1]);
    }
    ++i;
    if (invalid) {
        *invalid = false;
    }
    return result;
}

static void
ParseDoubleVector(
    int& i, int argc, char *argv[],
    std::vector<double>* result)
{
    bool invalid = false;
    while (i != argc) {
        const double value = ParseDouble(i, argc, argv, &invalid);
        if (invalid) {
            break;
        }
        result->push_back(value);
    }
}

void
UsdImaging_UnitTestGLDrawing::_Parse(int argc, char *argv[], _Args* args)
{
    for (int i = 1; i != argc; ++i) {
        if (strcmp(argv[i], "-") == 0) {
            Usage(argc, argv);
        }
        else if (strcmp(argv[i], "-frameAll") == 0) {
            _shouldFrameAll = true;
        }
        else if (strcmp(argv[i], "-cullBackfaces") == 0) {
            _cullBackfaces = true;
        }
        else if (strcmp(argv[i], "-offscreen") == 0) {
            args->offscreen = true;
        }
        else if (strcmp(argv[i], "-lighting") == 0) {
            _testLighting = true;
        }
        else if (strcmp(argv[i], "-idRender") == 0) {
            _testIdRender = true;
        }
        else if (strcmp(argv[i], "-stage") == 0) {
            CheckForMissingArguments(i, 1, argc, argv);
            args->unresolvedStageFilePath = argv[++i];
        }
        else if (strcmp(argv[i], "-write") == 0) {
            CheckForMissingArguments(i, 1, argc, argv);
            _outputFilePath = argv[++i];
        }
        else if (strcmp(argv[i], "-shading") == 0) {
            CheckForMissingArguments(i, 1, argc, argv);
            args->shading = argv[++i];
        }
        else if (strcmp(argv[i], "-complexity") == 0) {
            CheckForMissingArguments(i, 1, argc, argv);
            _complexity = ParseDouble(i, argc, argv);
        }
        else if (strcmp(argv[i], "-clipPlane") == 0) {
            CheckForMissingArguments(i, 4, argc, argv);
            args->clipPlaneCoords.push_back(ParseDouble(i, argc, argv));
            args->clipPlaneCoords.push_back(ParseDouble(i, argc, argv));
            args->clipPlaneCoords.push_back(ParseDouble(i, argc, argv));
            args->clipPlaneCoords.push_back(ParseDouble(i, argc, argv));
        }
        else if (strcmp(argv[i], "-complexities") == 0) {
            ParseDoubleVector(i, argc, argv, &args->complexities);
        }
        else if (strcmp(argv[i], "-times") == 0) {
            ParseDoubleVector(i, argc, argv, &_times);
        }
        else {
            ParseError(argv[0], "unknown argument %s", argv[i]);
        }
    }
}

void
UsdImaging_UnitTestGLDrawing::RunTest(int argc, char *argv[])
{
    QApplication app(argc, argv);

    UsdImaging_UnitTestHelper_InitPlugins();

    _Args args;
    _Parse(argc, argv, &args);

    for (size_t i=0; i<args.clipPlaneCoords.size()/4; ++i) {
        _clipPlanes.push_back(GfVec4d(&args.clipPlaneCoords[i*4]));
    }

    _drawMode = UsdImagingEngine::DRAW_SHADED_SMOOTH;

    // Only wireOnSurface/flat are supported
    if (args.shading.compare("wireOnSurface") == 0) {
        _drawMode = UsdImagingEngine::DRAW_WIREFRAME_ON_SURFACE;
    } else if (args.shading.compare("flat") == 0 ) {
        _drawMode = UsdImagingEngine::DRAW_SHADED_FLAT;
    }

    if (not args.unresolvedStageFilePath.empty()) {
        _stageFilePath = args.unresolvedStageFilePath;
    }

    _widget = new UsdImaging_UnitTestDrawingQGLWidget(this);
    _widget->setWindowTitle("Drawing Test");
    _widget->resize(640, 480);

    if (_times.empty()) {
        _times.push_back(-999);
    }

    if (args.complexities.size() > 0) {
        _widget->hide();
        _widget->makeCurrent();

        std::string imageFilePath = GetOutputFilePath();

        TF_FOR_ALL(compIt, args.complexities) {
            _complexity = *compIt;
            if (not imageFilePath.empty()) {
                std::stringstream suffix;
                suffix << "_" << _complexity << ".png";
                _outputFilePath = TfStringReplace(imageFilePath, ".png", suffix.str());
            }

            _widget->DrawOffscreen();
        }

        // Give tests the opportunity to shutdown any OGL 
        // buffers before they have no context available
        ShutdownTest();
        _widget->doneCurrent();

    } else if (args.offscreen) {
        _widget->hide();
        _widget->makeCurrent();
        _widget->DrawOffscreen();

        // Give tests the opportunity to shutdown any OGL 
        // buffers before they have no context available
        ShutdownTest();
        _widget->doneCurrent();
    } else {
        _widget->show();
        app.exec();

        // Give tests the opportunity to shutdown any OGL 
        // buffers before they have no context available
        ShutdownTest();
    }
}
