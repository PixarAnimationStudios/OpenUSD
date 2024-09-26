//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/tokens.h"
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
        _reprName = HdReprTokens->hull;
        _refineLevel = 0;

        SetCameraRotate(60.0f, 0.0f);
        SetCameraTranslate(GfVec3f(0, 0, -15.0f-1.7320508f*2.0f));
    }

    // HdSt_UnitTestGLDrawing overrides
    void InitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;
    void Present(uint32_t framebuffer) override;

protected:
    void ParseArgs(int argc, char *argv[]) override;

private:
    HdSt_TestDriverUniquePtr _driver;

    TfToken _reprName;
    int _refineLevel;
    std::string _outputFilePath;
};

////////////////////////////////////////////////////////////

void
My_TestGLDrawing::InitTest()
{
    _driver = std::make_unique<HdSt_TestDriver>(_reprName);
    HdUnitTestDelegate &delegate = _driver->GetDelegate();
    delegate.SetRefineLevel(_refineLevel);

    GfMatrix4d dmat;

    double xPos = 5;
    double yPos = -0.0;
    double zPos = 6.0;
    double dx = 3.0;
    bool useNormals = false;

    // Segment colors: [blue -> green] [pink -> yellow]

    // First row:
    // Curves with camera-facing normals 
    {
        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve1"), HdTokens->linear, TfToken(),
                    GfMatrix4f(dmat),
                    HdInterpolationVertex, HdInterpolationVertex,
                    useNormals);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve2"), HdTokens->cubic,
                    HdTokens->bezier,
                    GfMatrix4f(dmat),
                    HdInterpolationVertex, HdInterpolationVertex,
                    useNormals);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve3"), HdTokens->cubic,
                    HdTokens->bspline,
                    GfMatrix4f(dmat),
                    HdInterpolationVertex, HdInterpolationConstant,
                    useNormals);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve4"), HdTokens->cubic,
                    HdTokens->catmullRom,
                    GfMatrix4f(dmat),
                    HdInterpolationVertex, HdInterpolationConstant,
                    useNormals);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve5"), HdTokens->cubic,
                    HdTokens->centripetalCatmullRom,
                    GfMatrix4f(dmat),
                    HdInterpolationVertex, HdInterpolationConstant,
                    useNormals);
        xPos += dx;
    }

    xPos = 4;
    yPos = -3.0;
    zPos = 6.0;
    dx = 3.0;
    useNormals = true;

    // Second row:
    // Curves with authored normals
    {
        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve1n"), HdTokens->linear, TfToken(),
                    GfMatrix4f(dmat),
                    HdInterpolationVertex, HdInterpolationVertex,
                    useNormals);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve2n"), HdTokens->cubic,
                    HdTokens->bezier,
                    GfMatrix4f(dmat),
                    HdInterpolationVertex, HdInterpolationVertex,
                    useNormals);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve3n"), HdTokens->cubic,
                    HdTokens->bspline,
                    GfMatrix4f(dmat),
                    HdInterpolationVertex, HdInterpolationConstant,
                    useNormals);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve4n"), HdTokens->cubic,
                    HdTokens->catmullRom,
                    GfMatrix4f(dmat),
                    HdInterpolationVertex, HdInterpolationConstant,
                    useNormals);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve5n"), HdTokens->cubic,
                    HdTokens->centripetalCatmullRom,
                    GfMatrix4f(dmat),
                    HdInterpolationVertex, HdInterpolationConstant,
                    useNormals);
        xPos += dx;
    }

    xPos = 4;
    yPos = -6.0;
    zPos = 6.0;
    dx = 3.0;
    useNormals = true;

    // Third row:
    // Curves with varying data
    {
        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve1m"), HdTokens->linear, TfToken(),
                    GfMatrix4f(dmat),
                    HdInterpolationVertex, HdInterpolationVarying,
                    useNormals);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve2m"), HdTokens->cubic,
                    HdTokens->bezier,
                    GfMatrix4f(dmat),
                    HdInterpolationVertex, HdInterpolationVarying,
                    useNormals);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve3m"), HdTokens->cubic,
                    HdTokens->bspline,
                    GfMatrix4f(dmat),
                    HdInterpolationVertex, HdInterpolationVarying,
                    useNormals);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve4m"), HdTokens->cubic,
                    HdTokens->catmullRom,
                    GfMatrix4f(dmat),
                    HdInterpolationVertex, HdInterpolationVarying,
                    useNormals);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve5m"), HdTokens->cubic,
                    HdTokens->centripetalCatmullRom,
                    GfMatrix4f(dmat),
                    HdInterpolationVertex, HdInterpolationVarying,
                    useNormals);
        xPos += dx;
    }

    xPos = 4;
    yPos = -9.0;
    zPos = 6.0;
    dx = 3.0;
    useNormals = true;

    // Fourth row:
    // Curves with uniform color and various width interp modes
    {
        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve1u"), HdTokens->linear, TfToken(),
                    GfMatrix4f(dmat),
                    HdInterpolationUniform, HdInterpolationConstant,
                    useNormals);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve2u"), HdTokens->cubic,
                    HdTokens->bezier,
                    GfMatrix4f(dmat),
                    HdInterpolationUniform, HdInterpolationVertex,
                    useNormals);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve3u"), HdTokens->cubic,
                    HdTokens->bspline,
                    GfMatrix4f(dmat),
                    HdInterpolationUniform, HdInterpolationVarying,
                    useNormals);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve4u"), HdTokens->cubic,
                    HdTokens->catmullRom,
                    GfMatrix4f(dmat),
                    HdInterpolationUniform, HdInterpolationUniform,
                    useNormals);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve5u"), HdTokens->cubic,
                    HdTokens->centripetalCatmullRom,
                    GfMatrix4f(dmat),
                    HdInterpolationUniform, HdInterpolationUniform,
                    useNormals);
        xPos += dx;
    }

    xPos = 4;
    yPos = -12.0;
    zPos = 6.0;
    dx = 3.0;
    useNormals = false;

    // Fifth row:
    // Pinned bspline and catmullRom curves with vertex color and varying width
    // and camera facing normals.
    {
        // "pinned" wrap mode isn't relevant for linear curve type. Still, draw
        // the linear curve to aid comparison.
        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve1p"), HdTokens->linear, TfToken(),
                    GfMatrix4f(dmat),
                    HdInterpolationVertex, HdInterpolationVertex,
                    useNormals);
        xPos += dx;
        
        // "pinned" wrap mode isn't relevant for bezier basis. Still, draw the
        // bezier curve to validate the result.
        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve2p"), HdTokens->cubic,
                    HdTokens->bezier,
                    GfMatrix4f(dmat),
                    HdInterpolationUniform, HdInterpolationVertex,
                    useNormals);
        delegate.SetCurveWrapMode(SdfPath("/curve2p"), HdTokens->pinned);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve3p"), HdTokens->cubic,
                    HdTokens->bspline,
                    GfMatrix4f(dmat),
                    HdInterpolationVertex, HdInterpolationVarying,
                    useNormals);
        delegate.SetCurveWrapMode(SdfPath("/curve3p"), HdTokens->pinned);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve4p"), HdTokens->cubic,
                    HdTokens->catmullRom,
                    GfMatrix4f(dmat),
                    HdInterpolationVertex, HdInterpolationVarying,
                    useNormals);
        delegate.SetCurveWrapMode(SdfPath("/curve4p"), HdTokens->pinned);
        xPos += dx;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        delegate.AddCurves(SdfPath("/curve5p"), HdTokens->cubic,
                    HdTokens->centripetalCatmullRom,
                    GfMatrix4f(dmat),
                    HdInterpolationVertex, HdInterpolationVarying,
                    useNormals);
        delegate.SetCurveWrapMode(SdfPath("/curve5p"), HdTokens->pinned);
        xPos += dx;
    }

    // center camera
    SetCameraTranslate(GetCameraTranslate() + GfVec3f(-xPos/2, 3.0, -7.0));

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
My_TestGLDrawing::Present(uint32_t framebuffer)
{
    _driver->Present(GetWidth(), GetHeight(), framebuffer);
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
        } else if (std::string(argv[i]) == "--write" && i+1<argc) {
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

