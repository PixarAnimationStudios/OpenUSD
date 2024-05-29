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
#include "pxr/base/gf/transform.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

class My_TestGLDrawing : public HdSt_UnitTestGLDrawing {
public:
    My_TestGLDrawing();

    // HdSt_UnitTestGLDrawing overrides
    void InitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;
    void Present(uint32_t framebuffer) override;

    void Idle();

protected:
    void ParseArgs(int argc, char *argv[]) override;

private:
    HdSt_TestDriver* _driver;
    bool _useInstancePrimvars;

    TfToken _reprName;
    int _refineLevel;
    int _instancerLevel;
    int _div;
    bool _animateIndices;
    bool _rootTransform;
    std::string _outputFilePath;
};

////////////////////////////////////////////////////////////

My_TestGLDrawing::My_TestGLDrawing()
{
    SetCameraRotate(0, 0);
    SetCameraTranslate(GfVec3f(0, 0, -5));
    _useInstancePrimvars = true;
    _instancerLevel = 1;
    _div = 10;
    _animateIndices = false;
    _rootTransform = false;
    _refineLevel = 0;
    _reprName = HdReprTokens->hull;
}

void
My_TestGLDrawing::InitTest()
{
    _driver = new HdSt_TestDriver(_reprName);
    HdUnitTestDelegate &delegate = _driver->GetDelegate();
    delegate.SetRefineLevel(_refineLevel);

    delegate.SetUseInstancePrimvars(_useInstancePrimvars);

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
            float s = 2.0f/_div;
            float r = 1.0f;

            scale[j] = GfVec3f(s);
            // flip scale.z to test isFlipped
            if (j % 2 == 0) scale[j][2] = -scale[j][2];
            rotate[j] = GfVec4f(0);
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
                       HdInterpolationVertex,
                       HdInterpolationConstant,
                       instancerId);
    delegate.AddCurves(SdfPath("/prototype6"), HdTokens->cubic, HdTokens->bspline, transform,
                       HdInterpolationVertex, HdInterpolationVertex,
                       /*authoredNormals*/false,
                       instancerId);
    delegate.AddCurves(SdfPath("/prototype7"), HdTokens->cubic, HdTokens->catmullRom, transform,
                       HdInterpolationVertex, HdInterpolationVertex,
                       /*authoredNormals*/false,
                       instancerId);
    delegate.AddCurves(SdfPath("/prototype8"), HdTokens->cubic, HdTokens->catmullRom, transform,
                       HdInterpolationVertex, HdInterpolationVertex,
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
        GfVec4f quat(q.GetImaginary()[0],
                     q.GetImaginary()[1],
                     q.GetImaginary()[2],
                     q.GetReal());
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

    _driver->SetClearColor(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
    _driver->SetClearDepth(1.0f);
    _driver->SetupAovs(GetWidth(), GetHeight());
}

void
My_TestGLDrawing::DrawTest()
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

    _driver->Draw();
}

void
My_TestGLDrawing::OffscreenTest()
{
    DrawTest();

    if (!_outputFilePath.empty()) {
        _driver->WriteToFile("color", _outputFilePath);
    }
}

void
My_TestGLDrawing::Idle()
{
    static float time = 0;
    //_driver->UpdateRprims(time);
    HdUnitTestDelegate &delegate = _driver->GetDelegate();
    delegate.UpdateInstancerPrimvars(time);

    if (_animateIndices) {
        delegate.UpdateInstancerPrototypes(time);
    }
    time += 1.0f;
}

void
My_TestGLDrawing::Present(uint32_t framebuffer)
{
    _driver->Present(GetWidth(), GetHeight(), framebuffer);
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
            _useInstancePrimvars = false;
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

