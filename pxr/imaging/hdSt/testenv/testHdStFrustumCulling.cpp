//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

class My_TestGLDrawing : public HdSt_UnitTestGLDrawing {
public:
    My_TestGLDrawing() {
        _instance = false;
        _tinyPrim = false;

        SetCameraRotate(0, 0);
        SetCameraTranslate(GfVec3f(0));
    }

    // HdSt_UnitTestGLDrawing overrides
    void InitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;
    void Present(uint32_t framebuffer) override;

private:
    void ParseArgs(int argc, char *argv[]) override;
    int DrawScene();

    HdSt_TestDriverUniquePtr _driver;
    bool _instance;
    bool _tinyPrim;
};

////////////////////////////////////////////////////////////

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
    _driver = std::make_unique<HdSt_TestDriver>();
    HdUnitTestDelegate &delegate = _driver->GetDelegate();

    if (_instance) {
        GfMatrix4f transform;
        transform.SetIdentity();
        delegate.SetUseInstancePrimvars(true);

        SdfPath instancerId("/instancer");
        delegate.AddInstancer(instancerId);
        delegate.AddCube(SdfPath("/cube0"), transform, false, instancerId);
        delegate.AddGridWithFaceColor(SdfPath("/grid0"), 4, 4, transform,
                                      /*rightHanded=*/true, /*doubleSided=*/false,
                                      instancerId);
        delegate.AddPoints(SdfPath("/points0"), transform,
                           HdInterpolationVertex,
                           HdInterpolationConstant,
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

    if (_tinyPrim) {
        SetCameraTranslate(GfVec3f(0.0, 0.0,  -2000.0));
    }

    _driver->SetClearColor(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
    _driver->SetClearDepth(1.0f);
    _driver->SetupAovs(GetWidth(), GetHeight());
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
    } else if (_tinyPrim) {
        SetCameraTranslate(GfVec3f(0.0, 0.0,  -40000.0 - diameter));
        TF_VERIFY(DrawScene() == 0);
        SetCameraTranslate(GfVec3f(0.0, 0.0,  -2150.0 - diameter));
        TF_VERIFY(DrawScene() == 4);
        SetCameraTranslate(GfVec3f(0.0, 0.0,  -2000.0 - diameter));
        TF_VERIFY(DrawScene() == 8);
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
    int width = GetWidth(), height = GetHeight();
    GfMatrix4d viewMatrix = GetViewMatrix();
    GfMatrix4d projMatrix = GetProjectionMatrix();

    _driver->SetCamera(
        viewMatrix,
        projMatrix,
        CameraUtilFraming(
            GfRect2i(GfVec2i(0, 0), width, height)));
    
    _driver->UpdateAovDimensions(width, height);

    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.ResetCounters();
    perfLog.Enable();

    _driver->Draw();

    int numItemsDrawn = perfLog.GetCounter(HdTokens->itemsDrawn);

    GfVec3f pos = GetCameraTranslate();
    std::cout << "viewer: " << pos[0] << " " << pos[1] << " " << pos[2] << "\n";
    std::cout << "itemsDrawn: " << numItemsDrawn << "\n";

    return numItemsDrawn;
}

void
My_TestGLDrawing::Present(uint32_t framebuffer)
{
    _driver->Present(GetWidth(), GetHeight(), framebuffer);
}

/* virtual */
void
My_TestGLDrawing::ParseArgs(int argc, char *argv[])
{
    for (int i=0; i<argc; ++i) {
        if (std::string(argv[i]) == "--instance") {
            _instance = true;
        } else if (std::string(argv[i]) == "--tinyprim") {
            _tinyPrim = true;
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

