//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/base/gf/rotation.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

class My_TestGLDrawing : public HdSt_UnitTestGLDrawing {
public:
    My_TestGLDrawing() : _firstRun(true) {
        _reprName = HdReprTokens->hull;
        _refineLevel = 0;

        SetCameraRotate(60.0f, 0.0f);
        SetCameraTranslate(GfVec3f(0, 0, -6));
    }

    // HdSt_UnitTestGLDrawing overrides
    void InitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;
    void Present(uint32_t framebuffer) override;

    void RunMultipleFvarTopologiesTest(int argc, char *argv[]);

protected:
    void ParseArgs(int argc, char *argv[]) override;

private:
    HdSt_TestDriver* _driver;

    TfToken _reprName;
    int _refineLevel;
    std::string _outputFilePath;
    bool _firstRun;
};

////////////////////////////////////////////////////////////

void
My_TestGLDrawing::InitTest()
{
    std::cout << "My_TestGLDrawing::InitTest() " << _reprName << "\n";

    _driver = new HdSt_TestDriver(_reprName);
    HdUnitTestDelegate &delegate = _driver->GetDelegate();
    delegate.SetRefineLevel(_refineLevel);

    // Initial state
    delegate.AddFaceVaryingPolygons(SdfPath("/mesh1"), GfMatrix4f(1));
    _outputFilePath = "testHdStMultipleFvarTopologies_0.png";

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

    // camera
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
My_TestGLDrawing::RunMultipleFvarTopologiesTest(int argc, char *argv[])
{
    // Begin Test
    RunTest(argc, argv);

    HdUnitTestDelegate &delegate = _driver->GetDelegate();
      
    const SdfPath mesh("/mesh1");
    // Change displayColor to have non-trivial indices (and thus different 
    // indices from displayOpacity)
    VtVec3fArray colorArray = {
        GfVec3f(1, 0, 0),
        GfVec3f(1, 1, 0), 
        GfVec3f(0, 1, 1)
    };
    VtIntArray colorIndices = { 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2 };
    delegate.UpdatePrimvarValue(mesh, HdTokens->displayColor,
                                VtValue(colorArray), colorIndices);
    _outputFilePath = "testHdStMultipleFvarTopologies_1.png";
    RunOffscreenTest();

    // Remove displayOpacity primvar
    delegate.RemovePrimvar(mesh, HdTokens->displayOpacity);
    _outputFilePath = "testHdStMultipleFvarTopologies_2.png";
    RunOffscreenTest();

    // Add a new displayOpacity with unique indices
    VtFloatArray opacityArray = { 0.6, 1 };
    VtIntArray opacityIndices = { 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
    delegate.AddPrimvar(mesh, HdTokens->displayOpacity,
                        VtValue(opacityArray), HdInterpolationFaceVarying,
                        HdPrimvarRoleTokens->color, opacityIndices);
    _outputFilePath = "testHdStMultipleFvarTopologies_3.png";
    RunOffscreenTest();

    // Make both primvars use the same indices now
    opacityArray = { 0.6, 1, 0.9 };
    delegate.UpdatePrimvarValue(mesh, HdTokens->displayOpacity,
                                VtValue(opacityArray), colorIndices);
    _outputFilePath = "testHdStMultipleFvarTopologies_4.png";
    RunOffscreenTest();
}

void
BasicTest(int argc, char *argv[])
{
    My_TestGLDrawing driver;
    driver.RunMultipleFvarTopologiesTest(argc, argv);
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



