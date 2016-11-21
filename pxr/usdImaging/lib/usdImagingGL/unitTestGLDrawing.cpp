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

#include "pxr/usdImaging/usdImagingGL/unitTestGLDrawing.h"
#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/drawTarget.h"
#include "pxr/imaging/garch/glDebugWindow.h"

#include "pxr/base/arch/attributes.h"
#include "pxr/base/gf/vec2i.h"

#include "pxr/base/plug/registry.h"
#include "pxr/base/arch/systemInfo.h"

#include <stdio.h>
#include <stdarg.h>

static void UsdImagingGL_UnitTestHelper_InitPlugins()
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

class UsdImagingGL_UnitTestWindow : public GarchGLDebugWindow {
public:
    typedef UsdImagingGL_UnitTestWindow This;

public:
    UsdImagingGL_UnitTestWindow(UsdImagingGL_UnitTestGLDrawing * unitTest,
                                int w, int h);
    virtual ~UsdImagingGL_UnitTestWindow();

    void DrawOffscreen();

    bool WriteToFile(std::string const & attachment, 
                     std::string const & filename);

    // GarchGLDebugWIndow overrides;
    virtual void OnInitializeGL();
    virtual void OnUninitializeGL();
    virtual void OnPaintGL();
    virtual void OnKeyRelease(int key);
    virtual void OnMousePress(int button, int x, int y, int modKeys);
    virtual void OnMouseRelease(int button, int x, int y, int modKeys);
    virtual void OnMouseMove(int x, int y, int modKeys);

private:
    UsdImagingGL_UnitTestGLDrawing *_unitTest;
    GlfDrawTargetRefPtr _drawTarget;
};

UsdImagingGL_UnitTestWindow::UsdImagingGL_UnitTestWindow(
    UsdImagingGL_UnitTestGLDrawing * unitTest, int w, int h)
    : GarchGLDebugWindow("UsdImagingGL Test", w, h)
    , _unitTest(unitTest)
{
}

UsdImagingGL_UnitTestWindow::~UsdImagingGL_UnitTestWindow()
{
}

/* virtual */
void
UsdImagingGL_UnitTestWindow::OnInitializeGL()
{
    GlfGlewInit();
    GlfRegisterDefaultDebugOutputMessageCallback();

    //
    // Create an offscreen draw target which is the same size as this
    // widget and initialize the unit test with the draw target bound.
    //
    _drawTarget = GlfDrawTarget::New(GfVec2i(GetWidth(), GetHeight()));
    _drawTarget->Bind();
    _drawTarget->AddAttachment("color", GL_RGBA, GL_FLOAT, GL_RGBA);
    _drawTarget->AddAttachment("depth", GL_DEPTH_COMPONENT, GL_FLOAT,
                                        GL_DEPTH_COMPONENT);

    _unitTest->InitTest();

    _drawTarget->Unbind();
}

/* virtual */
void
UsdImagingGL_UnitTestWindow::OnUninitializeGL()
{
    _drawTarget = GlfDrawTargetRefPtr();

    _unitTest->ShutdownTest();
}

/* virtual */
void
UsdImagingGL_UnitTestWindow::OnPaintGL()
{
    //
    // Update the draw target's size and execute the unit test with
    // the draw target bound.
    //
    int width = GetWidth();
    int height = GetHeight();
    _drawTarget->Bind();
    _drawTarget->SetSize(GfVec2i(width, height));

    _unitTest->DrawTest(false);

    _drawTarget->Unbind();

    //
    // Blit the resulting color buffer to the window (this is a noop
    // if we're drawing offscreen).
    //
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _drawTarget->GetFramebufferId());

    glBlitFramebuffer(0, 0, width, height,
                      0, 0, width, height,
                      GL_COLOR_BUFFER_BIT,
                      GL_NEAREST);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

void
UsdImagingGL_UnitTestWindow::DrawOffscreen()
{
    _drawTarget->Bind();
    _drawTarget->SetSize(GfVec2i(GetWidth(), GetHeight()));

    _unitTest->DrawTest(true);

    _drawTarget->Unbind();
}

bool
UsdImagingGL_UnitTestWindow::WriteToFile(std::string const & attachment,
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
UsdImagingGL_UnitTestWindow::OnKeyRelease(int key)
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
UsdImagingGL_UnitTestWindow::OnMousePress(int button,
                                          int x, int y, int modKeys)
{
    _unitTest->MousePress(button, x, y, modKeys);
}

/* virtual */
void
UsdImagingGL_UnitTestWindow::OnMouseRelease(int button,
                                            int x, int y, int modKeys)
{
    _unitTest->MouseRelease(button, x, y, modKeys);
}

/* virtual */
void
UsdImagingGL_UnitTestWindow::OnMouseMove(int x, int y, int modKeys)
{
    _unitTest->MouseMove(x, y, modKeys);
}

////////////////////////////////////////////////////////////

UsdImagingGL_UnitTestGLDrawing::UsdImagingGL_UnitTestGLDrawing()
    : _widget(NULL)
    , _testLighting(false)
    , _cameraLight(false)
    , _testIdRender(false)
    , _complexity(1.0f)
    , _drawMode(UsdImagingGLEngine::DRAW_SHADED_SMOOTH)
    , _shouldFrameAll(false)
    , _cullBackfaces(false)
{
}

UsdImagingGL_UnitTestGLDrawing::~UsdImagingGL_UnitTestGLDrawing()
{
}

int
UsdImagingGL_UnitTestGLDrawing::GetWidth() const
{
    return _widget->GetWidth();
}

int
UsdImagingGL_UnitTestGLDrawing::GetHeight() const
{
    return _widget->GetHeight();
}

bool
UsdImagingGL_UnitTestGLDrawing::WriteToFile(std::string const & attachment,
        std::string const & filename) const
{
    return _widget->WriteToFile(attachment, filename);
}

struct UsdImagingGL_UnitTestGLDrawing::_Args {
    _Args() : offscreen(false) {
        clearColor[0] = 1.0f;
        clearColor[1] = 0.5f;
        clearColor[2] = 0.1f;
        clearColor[3] = 1.0f;

        translate[0] = 0.0;
        translate[1] = -1000.0;
        translate[2] = -2500.0;
    }

    std::string unresolvedStageFilePath;
    bool offscreen;
    std::string shading;
    std::vector<double> clipPlaneCoords;
    std::vector<double> complexities;
    float clearColor[4];
    float translate[3];
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
"                           [-clear r g b a] [-translate x y z]\n"
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
"  -cullBackfaces      enable backface culling\n"
"  -clear r g b a      clear color\n"
"  -translate x y z    default camera translation\n";

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
UsdImagingGL_UnitTestGLDrawing::_Parse(int argc, char *argv[], _Args* args)
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
        else if (strcmp(argv[i], "-camlight") == 0) {
            _cameraLight = true;
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
        else if (strcmp(argv[i], "-clear") == 0) {
            CheckForMissingArguments(i, 4, argc, argv);
            args->clearColor[0] = (float)ParseDouble(i, argc, argv);
            args->clearColor[1] = (float)ParseDouble(i, argc, argv);
            args->clearColor[2] = (float)ParseDouble(i, argc, argv);
            args->clearColor[3] = (float)ParseDouble(i, argc, argv);
        }
        else if (strcmp(argv[i], "-translate") == 0) {
            CheckForMissingArguments(i, 3, argc, argv);
            args->translate[0] = (float)ParseDouble(i, argc, argv);
            args->translate[1] = (float)ParseDouble(i, argc, argv);
            args->translate[2] = (float)ParseDouble(i, argc, argv);
        }
        else {
            ParseError(argv[0], "unknown argument %s", argv[i]);
        }
    }
}

/* virtual */
void
UsdImagingGL_UnitTestGLDrawing::MousePress(int button, int x, int y,
                                           int modKeys)
{
}
/* virtual */
void
UsdImagingGL_UnitTestGLDrawing::MouseRelease(int button, int x, int y,
                                             int modKeys)
{
}
/* virtual */
void
UsdImagingGL_UnitTestGLDrawing::MouseMove(int x, int y, int modKeys)
{
}
/* virtual */
void
UsdImagingGL_UnitTestGLDrawing::KeyRelease(int key)
{
}

void
UsdImagingGL_UnitTestGLDrawing::RunTest(int argc, char *argv[])
{
    UsdImagingGL_UnitTestHelper_InitPlugins();

    _Args args;
    _Parse(argc, argv, &args);

    for (size_t i=0; i<args.clipPlaneCoords.size()/4; ++i) {
        _clipPlanes.push_back(GfVec4d(&args.clipPlaneCoords[i*4]));
    }
    _clearColor = GfVec4f(args.clearColor);
    _translate = GfVec3f(args.translate);

    _drawMode = UsdImagingGLEngine::DRAW_SHADED_SMOOTH;

    // Only wireOnSurface/flat are supported
    if (args.shading.compare("wireOnSurface") == 0) {
        _drawMode = UsdImagingGLEngine::DRAW_WIREFRAME_ON_SURFACE;
    } else if (args.shading.compare("flat") == 0 ) {
        _drawMode = UsdImagingGLEngine::DRAW_SHADED_FLAT;
    }

    if (not args.unresolvedStageFilePath.empty()) {
        _stageFilePath = args.unresolvedStageFilePath;
    }

    _widget = new UsdImagingGL_UnitTestWindow(this, 640, 480);
    _widget->Init();

    if (_times.empty()) {
        _times.push_back(-999);
    }

    if (args.complexities.size() > 0) {
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

    } else if (args.offscreen) {
        _widget->DrawOffscreen();
    } else {
        _widget->Run();
    }
}
