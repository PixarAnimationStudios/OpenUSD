#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/unitTestGLDrawing.h"
#include "pxr/imaging/hd/unitTestHelper.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/transform.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>

class My_TestGLDrawing : public Hd_UnitTestGLDrawing {
public:
    My_TestGLDrawing();

    // Hd_UnitTestGLDrawing overrides
    virtual void InitTest();
    virtual void DrawTest();
    virtual void OffscreenTest();

    virtual void Idle();

protected:
    virtual void ParseArgs(int argc, char *argv[]);

private:
    Hd_TestDriver* _driver;
    bool _useInstancePrimVars;

    TfToken _reprName;
    int _refineLevel;
    int _instancerLevel;
    int _div;
    bool _animateIndices;
    bool _rootTransform;
    std::string _outputFilePath;
};

////////////////////////////////////////////////////////////

GLuint program;
GLuint vao;

My_TestGLDrawing::My_TestGLDrawing()
{
    SetCameraRotate(0, 0);
    SetCameraTranslate(GfVec3f(0, 0, -5));
    _useInstancePrimVars = true;
    _instancerLevel = 1;
    _div = 10;
    _animateIndices = false;
    _rootTransform = false;
    _refineLevel = 0;
    _reprName = HdTokens->hull;
}

void
My_TestGLDrawing::InitTest()
{
    _driver = new Hd_TestDriver(_reprName);
    Hd_UnitTestDelegate &delegate = _driver->GetDelegate();
    delegate.SetRefineLevel(_refineLevel);

    delegate.SetUseInstancePrimVars(_useInstancePrimVars);

    GfMatrix4f transform;
    transform.SetIdentity();

    // create instancer hierarchy
    SdfPath instancerId("/instancer");
    delegate.AddInstancer(instancerId);

    // instancer nesting
    for (int i = 0; i < _instancerLevel-1; ++i) {
        SdfPath parentInstancerId = instancerId;
        instancerId = parentInstancerId.AppendChild(TfToken("instancer"));

        GfTransform rootTransform;
        if (_rootTransform) {
            rootTransform.SetRotation(GfRotation(GfVec3d(0,0,1), 45));
        }
        delegate.AddInstancer(instancerId, parentInstancerId,
                              GfMatrix4f(rootTransform.GetMatrix()));
        VtVec3fArray scale(_div);
        VtVec4fArray rotate(_div);
        VtVec3fArray translate(_div);
        VtIntArray prototypeIndex(_div);
        int nPrototypes = 1;

        for (int j = 0; j < _div; ++j) {
            float p = j/(float)_div;
            GfQuaternion q;// = GfRotation(GfVec3d(1, 0, 0), 0).GetQuaternion();
            GfVec4f quat(q.GetReal(),
                         q.GetImaginary()[0],
                         q.GetImaginary()[1],
                         q.GetImaginary()[2]);
            float s = 2.0f/_div;
            float r = 1.0f;

            scale[j] = GfVec3f(s);
            // flip scale.z to test isFlipped
            if (j % 2 == 0) scale[j][2] = -scale[j][2];
            rotate[j] = GfVec4f(0);//quat;
            translate[j] = GfVec3f(r*cos(p*6.28), 0, r*sin(p*6.28));
            prototypeIndex[j] = j % nPrototypes;
        }

        delegate.SetInstancerProperties(parentInstancerId,
                                        prototypeIndex
                                        , scale, rotate, translate);
    }

    // add prototypes
    delegate.AddGridWithFaceColor(SdfPath("/prototype1"), 4, 4, transform,
                                  /*rightHanded=*/true, /*doubleSided=*/false,
                                  instancerId);
    delegate.AddGridWithVertexColor(SdfPath("/prototype2"), 4, 4, transform,
                                    /*rightHanded=*/true, /*doubleSided=*/false,
                                    instancerId);
    delegate.AddCube(SdfPath("/prototype3"), transform, false, instancerId);
    delegate.AddGrid(SdfPath("/prototype4"), 1, 1, transform,
                     /*rightHanded=*/true, /*doubleSided=*/false, instancerId);
    delegate.AddPoints(SdfPath("/prototype5"), transform,
                       Hd_UnitTestDelegate::VERTEX,
                       Hd_UnitTestDelegate::CONSTANT,
                       instancerId);
    delegate.AddCurves(SdfPath("/prototype6"), HdTokens->bSpline, transform,
                       Hd_UnitTestDelegate::VERTEX, Hd_UnitTestDelegate::VERTEX,
                       /*authoredNormals*/false,
                       instancerId);
    delegate.AddCurves(SdfPath("/prototype7"), HdTokens->catmullRom, transform,
                       Hd_UnitTestDelegate::VERTEX, Hd_UnitTestDelegate::VERTEX,
                       /*authoredNormals*/false,
                       instancerId);
    delegate.AddCurves(SdfPath("/prototype8"), HdTokens->catmullRom, transform,
                       Hd_UnitTestDelegate::VERTEX, Hd_UnitTestDelegate::VERTEX,
                       /*authoredNormals*/false,
                       instancerId);

    int nPrototypes = 8;
    VtVec3fArray scale(_div);
    VtVec4fArray rotate(_div);
    VtVec3fArray translate(_div);
    VtIntArray prototypeIndex(_div);
    for (int i = 0; i < _div; ++i) {
        float p = i/(float)_div;
        GfQuaternion q = GfRotation(GfVec3d(1, 0, 0), 90).GetQuaternion();
        GfVec4f quat(q.GetReal(),
                     q.GetImaginary()[0],
                     q.GetImaginary()[1],
                     q.GetImaginary()[2]);
        float s = 2.0/_div;
        float r = 1.0f;

        scale[i] = GfVec3f(s);
        // flip scale.x to test isFlipped
        if (i % 2 == 0) scale[i][0] = -scale[i][0];
        rotate[i] = quat;
        translate[i] = GfVec3f(r*cos(p*6.28), 0, r*sin(p*6.28));
        prototypeIndex[i] = i % nPrototypes;
    }
    delegate.SetInstancerProperties(instancerId,
                                    prototypeIndex,
                                    scale, rotate, translate);


    // XXX: Setup a VAO, the current drawing engine will not yet do this.
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindVertexArray(0);

    //glEnable(GL_CULL_FACE);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
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

void
My_TestGLDrawing::Idle()
{
    static float time = 0;
    //_driver->UpdateRprims(time);
    Hd_UnitTestDelegate &delegate = _driver->GetDelegate();
    delegate.UpdateInstancerPrimVars(time);

    if (_animateIndices) {
        delegate.UpdateInstancerPrototypes(time);
    }
    time += 1.0f;
}

void
My_TestGLDrawing::ParseArgs(int argc, char *argv[])
{
    // note: _driver has not been constructed yet.
    for (int i=0; i<argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--repr") {
            _reprName = TfToken(argv[++i]);
        } else if (arg == "--refineLevel") {
            _refineLevel = atoi(argv[++i]);
        } else if (arg == "--noprimvars") {
            _useInstancePrimVars = false;
        } else if (arg == "--div") {
            _div = atoi(argv[++i]);
        } else if (arg == "--level") {
            _instancerLevel = atoi(argv[++i]);
        } else if (arg == "--animateIndices") {
            _animateIndices = true;
        } else if (arg == "--rootTransform") {
            _rootTransform = true;
        } else if (arg == "--write") {
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

