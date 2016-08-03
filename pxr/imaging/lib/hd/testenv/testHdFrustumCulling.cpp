#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/unitTestGLDrawing.h"
#include "pxr/imaging/hd/unitTestHelper.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/errorMark.h"

#include <boost/shared_ptr.hpp>

#include <iostream>

class My_TestGLDrawing : public Hd_UnitTestGLDrawing {
public:
    My_TestGLDrawing() {
        _instance = false;

        SetCameraRotate(0, 0);
        SetCameraTranslate(GfVec3f(0));
    }

    // Hd_UnitTestGLDrawing overrides
    virtual void InitTest();
    virtual void DrawTest();
    virtual void OffscreenTest();

private:
    virtual void ParseArgs(int argc, char *argv[]);
    int DrawScene();

    Hd_TestDriver* _driver;
    bool _instance;
};

////////////////////////////////////////////////////////////

GLuint vao;

static GfMatrix4f
_GetTranslate(float tx, float ty, float tz)
{
    GfMatrix4f m(1.0f);
    m.SetRow(3, GfVec4f(tx, ty, tz, 1.0));
    return m;
}

void
My_TestGLDrawing::InitTest()
{
    _driver = new Hd_TestDriver();
    Hd_UnitTestDelegate &delegate = _driver->GetDelegate();

    if (_instance) {
        GfMatrix4f transform;
        transform.SetIdentity();
        delegate.SetUseInstancePrimVars(true);

        SdfPath instancerId("/instancer");
        delegate.AddInstancer(instancerId);
        delegate.AddCube(SdfPath("/cube0"), transform, false, instancerId);
        delegate.AddGridWithFaceColor(SdfPath("/grid0"), 4, 4, transform,
                                      /*rightHanded=*/true, /*doubleSided=*/false,
                                      instancerId);
        delegate.AddPoints(SdfPath("/points0"), transform,
                           Hd_UnitTestDelegate::VERTEX,
                           Hd_UnitTestDelegate::CONSTANT,
                           instancerId);
        std::vector<SdfPath> prototypes;
        prototypes.push_back(SdfPath("/cube0"));
        prototypes.push_back(SdfPath("/grid0"));
        prototypes.push_back(SdfPath("/points0"));

        int div = 10;
        VtVec3fArray scale(div*div*div);
        VtVec4fArray rotate(div*div*div);
        VtVec3fArray translate(div*div*div);
        VtIntArray prototypeIndex(div*div*div);
        int n = 0;
        for (int z = -div/2; z < div/2; ++z) {
            for (int y = -div/2; y < div/2; ++y) {
                for (int x = -div/2; x < div/2; ++x) {
                    GfQuaternion q = GfRotation(GfVec3d(x/(float)div,
                                                        y/(float)div,
                                                        0),
                                                360*z/(float)div).GetQuaternion();
                    GfVec4f quat(q.GetReal(),
                                 q.GetImaginary()[0],
                                 q.GetImaginary()[1],
                                 q.GetImaginary()[2]);
                    float s = 1-fabs(z/(float)div);
                    scale[n] = GfVec3f(s);
                    rotate[n] = quat;
                    translate[n] = GfVec3f(x*4, y*4, z*4);
                    prototypeIndex[n] = n % int(prototypes.size());
                    ++n;
                }
            }
        }
        delegate.SetInstancerProperties(instancerId,
                                        prototypeIndex,
                                        scale, rotate, translate);

    } else {
        delegate.AddCube(SdfPath("/cube0"), _GetTranslate( 10, 10, 10));
        delegate.AddCube(SdfPath("/cube1"), _GetTranslate(-10, 10, 10));
        delegate.AddCube(SdfPath("/cube2"), _GetTranslate(-10,-10, 10));
        delegate.AddCube(SdfPath("/cube3"), _GetTranslate( 10,-10, 10));
        delegate.AddCube(SdfPath("/cube4"), _GetTranslate( 10, 10,-10));
        delegate.AddCube(SdfPath("/cube5"), _GetTranslate(-10, 10,-10));
        delegate.AddCube(SdfPath("/cube6"), _GetTranslate(-10,-10,-10));
        delegate.AddCube(SdfPath("/cube7"), _GetTranslate( 10,-10,-10));
    }

    // XXX: Setup a VAO, the current drawing engine will not yet do this.
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindVertexArray(0);
}

void
My_TestGLDrawing::DrawTest()
{
    DrawScene();
}

void
My_TestGLDrawing::OffscreenTest()
{
    float diameter = 1.7320508f*2.0f; // for test compatibility

    if (_instance) {
        SetCameraTranslate(GfVec3f(0.0, 0.0,  -20.0 - diameter));
        TF_VERIFY(DrawScene() == 384);

        SetCameraTranslate(GfVec3f(0.0, 0.0,  -40.0 - diameter));
        TF_VERIFY(DrawScene() == 808);

        SetCameraTranslate(GfVec3f(0.0, 0.0, -100.0 - diameter));
        TF_VERIFY(DrawScene() == 1000);
    } else {
        SetCameraTranslate(GfVec3f(0.0, 0.0,  -20.0 - diameter));
        TF_VERIFY(DrawScene() == 4);
        SetCameraTranslate(GfVec3f(0.0, 0.0,  -40.0 - diameter));
        TF_VERIFY(DrawScene() == 8);
    }
}

int
My_TestGLDrawing::DrawScene()
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

    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.ResetCounters();
    perfLog.Enable();

    glBindVertexArray(vao);

    _driver->Draw();

    glBindVertexArray(0);

    int numItemsDrawn = perfLog.GetCounter(HdTokens->itemsDrawn);

    GfVec3f pos = GetCameraTranslate();
    std::cout << "viewer: " << pos[0] << " " << pos[1] << " " << pos[2] << "\n";
    std::cout << "itemsDrawn: " << numItemsDrawn << "\n";

    return numItemsDrawn;
}

/* virtual */
void
My_TestGLDrawing::ParseArgs(int argc, char *argv[])
{
    for (int i=0; i<argc; ++i) {
        if (std::string(argv[i]) == "--instance") {
            _instance = true;
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

