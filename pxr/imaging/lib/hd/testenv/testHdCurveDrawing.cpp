#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/unitTestGLDrawing.h"
#include "pxr/imaging/hd/unitTestHelper.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>

class My_TestGLDrawing : public Hd_UnitTestGLDrawing {
public:
    My_TestGLDrawing() {
        _reprName = HdTokens->hull;
        _refineLevel = 0;

        SetCameraRotate(60.0f, 0.0f);
        SetCameraTranslate(GfVec3f(0, 0, -15.0f-1.7320508f*2.0f));
    }

    // Hd_UnitTestGLDrawing overrides
    virtual void InitTest();
    virtual void DrawTest();
    virtual void OffscreenTest();

protected:
    virtual void ParseArgs(int argc, char *argv[]);

private:
    Hd_TestDriver* _driver;

    TfToken _reprName;
    int _refineLevel;
    std::string _outputFilePath;
};

////////////////////////////////////////////////////////////

GLuint vao;

void
My_TestGLDrawing::InitTest()
{
    _driver = new Hd_TestDriver(_reprName);
    Hd_UnitTestDelegate &delegate = _driver->GetDelegate();
    delegate.SetRefineLevel(_refineLevel);

    GfMatrix4d dmat;

    double xPos = 5;
    double yPos = -0.0;
    double zPos = 6.0;
    double dx = 3.0;
    bool useNormals = false;

    // Segment colors: [blue -> green] [pink -> yellow]

    // Back row:
    // Curves with camera-facing normals 
    {
        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve1"), HdTokens->linear,
                    GfMatrix4f(dmat),
                    Hd_UnitTestDelegate::VERTEX, Hd_UnitTestDelegate::VERTEX,
                    useNormals);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve2"), HdTokens->bezier,
                    GfMatrix4f(dmat),
                    Hd_UnitTestDelegate::VERTEX, Hd_UnitTestDelegate::VERTEX,
                    useNormals);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve3"), HdTokens->bSpline,
                    GfMatrix4f(dmat),
                    Hd_UnitTestDelegate::VERTEX, Hd_UnitTestDelegate::CONSTANT,
                    useNormals);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve4"), HdTokens->catmullRom,
                    GfMatrix4f(dmat),
                    Hd_UnitTestDelegate::VERTEX, Hd_UnitTestDelegate::CONSTANT,
                    useNormals);
        xPos += dx;
    }

    xPos = 4;
    yPos = -3.0;
    zPos = 6.0;
    dx = 3.0;
    useNormals = true;

    // Front row:
    // Curves with authored normals
    {
        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve1n"), HdTokens->linear,
                    GfMatrix4f(dmat),
                    Hd_UnitTestDelegate::VERTEX, Hd_UnitTestDelegate::VERTEX,
                    useNormals);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve2n"), HdTokens->bezier,
                    GfMatrix4f(dmat),
                    Hd_UnitTestDelegate::VERTEX, Hd_UnitTestDelegate::VERTEX,
                    useNormals);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve3n"), HdTokens->bSpline,
                    GfMatrix4f(dmat),
                    Hd_UnitTestDelegate::VERTEX, Hd_UnitTestDelegate::CONSTANT,
                    useNormals);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve4n"), HdTokens->catmullRom,
                    GfMatrix4f(dmat),
                    Hd_UnitTestDelegate::VERTEX, Hd_UnitTestDelegate::CONSTANT,
                    useNormals);
        xPos += dx;
    }

    xPos = 4;
    yPos = -6.0;
    zPos = 6.0;
    dx = 3.0;
    useNormals = true;

    // Last row:
    // Curves with facevarying data
    {
        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve1m"), HdTokens->linear,
                    GfMatrix4f(dmat),
                    Hd_UnitTestDelegate::VERTEX, Hd_UnitTestDelegate::VARYING,
                    useNormals);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve2m"), HdTokens->bezier,
                    GfMatrix4f(dmat),
                    Hd_UnitTestDelegate::VERTEX, Hd_UnitTestDelegate::VARYING,
                    useNormals);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve3m"), HdTokens->bSpline,
                    GfMatrix4f(dmat),
                    Hd_UnitTestDelegate::VERTEX, Hd_UnitTestDelegate::VARYING,
                    useNormals);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve4m"), HdTokens->catmullRom,
                    GfMatrix4f(dmat),
                    Hd_UnitTestDelegate::VERTEX, Hd_UnitTestDelegate::VARYING,
                    useNormals);
        xPos += dx;
    }

    // center camera
    SetCameraTranslate(GetCameraTranslate() + GfVec3f(-xPos/2.0, 0, 0));

    // XXX: Setup a VAO, the current drawing engine will not yet do this.
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindVertexArray(0);
}

void
My_TestGLDrawing::DrawTest()
{
    GLfloat clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    glClearBufferfv(GL_COLOR, 0, clearColor);

    GLfloat clearDepth[1] = { 1.0f };
    glClearBufferfv(GL_DEPTH, 0, clearDepth);

    int width = GetWidth(), height = GetHeight();

    GfMatrix4d viewMatrix = GetViewMatrix();
    GfMatrix4d projMatrix = GetProjectionMatrix();

    // camera
    _driver->SetCamera(viewMatrix, projMatrix, GfVec4d(0, 0, width, height));

    glViewport(0, 0, width, height);

    glEnable(GL_DEPTH_TEST);

    glBindVertexArray(vao);

    _driver->Draw();

    glBindVertexArray(0);
}

void
My_TestGLDrawing::OffscreenTest()
{
    DrawTest();

    if (not _outputFilePath.empty()) {
        WriteToFile("color", _outputFilePath);
    }
}

/* virtual */
void
My_TestGLDrawing::ParseArgs(int argc, char *argv[])
{
    for (int i=0; i<argc; ++i) {
        if (std::string(argv[i]) == "--repr") {
            _reprName = TfToken(argv[++i]);
        } else if (std::string(argv[i]) == "--refineLevel") {
            _refineLevel = atoi(argv[++i]);
        } else if (std::string(argv[i]) == "--write" and i+1<argc) {
            _outputFilePath = argv[++i];
        }
    }
}

void
BasicTest(int argc, char *argv[])
{
    My_TestGLDrawing driver;
    driver.RunTest(argc, argv);
}

int main(int argc, char *argv[])
{
    TfErrorMark mark;

    BasicTest(argc, argv);

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

