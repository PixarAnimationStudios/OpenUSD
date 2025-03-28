//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3d.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

class My_TestGLDrawing : public HdSt_UnitTestGLDrawing {
public:
    My_TestGLDrawing() {
        _reprName = HdReprTokens->hull;
        _refineLevel = 0;
        _cullStyle = HdCullStyleNothing;
        _testLighting = false;

        SetCameraRotate(60.0f, 0.0f);
        SetCameraTranslate(GfVec3f(0, 0, -20.0f-1.7320508f*2.0f));
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
    HdSt_TestLightingShaderSharedPtr _lightingShader;
    std::vector<GfVec4d> _clipPlanes;

    TfToken _reprName;
    int _refineLevel;
    HdCullStyle _cullStyle;
    bool _testLighting;
    std::string _outputFilePath;
};

////////////////////////////////////////////////////////////

void
My_TestGLDrawing::InitTest()
{
    std::cout << "My_TestGLDrawing::InitTest() " << _reprName << "\n";

    _driver = std::make_unique<HdSt_TestDriver>(_reprName);
    HdUnitTestDelegate &delegate = _driver->GetDelegate();
    delegate.SetRefineLevel(_refineLevel);

    GfVec3f center(0);

    delegate.PopulateInvalidPrimsSet();
    center = delegate.PopulateBasicTestSet();

    // center camera
    SetCameraTranslate(GetCameraTranslate() - center);

    if (_testLighting) {
        _lightingShader.reset(
                new HdSt_TestLightingShader(&delegate.GetRenderIndex()));
        _lightingShader->Prepare();
        _driver->GetRenderPassState()->SetLightingShader(
            _lightingShader);
    }

    _driver->SetCameraClipPlanes(_clipPlanes);

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

    _driver->SetCullStyle(_cullStyle);

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
        std::string arg(argv[i]);
        if (arg == "--repr") {
            _reprName = TfToken(argv[++i]);
        } else if (arg == "--refineLevel") {
            _refineLevel = atoi(argv[++i]);
        } else if (std::string(argv[i]) == "--cullStyle") {
            std::string style = argv[++i];
            if (style == "Nothing") {
                _cullStyle = HdCullStyleNothing;
            } else if (style == "Back") {
                _cullStyle = HdCullStyleBack;
            } else if (style == "Front") {
                _cullStyle = HdCullStyleFront;
            } else if (style == "BackUnlessDoubleSided") {
                _cullStyle = HdCullStyleBackUnlessDoubleSided;
            } else if (style == "FrontUnlessDoubleSided") {
                _cullStyle = HdCullStyleFrontUnlessDoubleSided;
            } else {
                std::cerr << "Error: Unknown cullstyle = " << style << "\n";
                exit(EXIT_FAILURE);
            }
        } else if (arg == "--lighting") {
            _testLighting = true;
        } else if (arg == "--clipPlane" && i+4<argc) {
            GfVec4d clipPlane;
            clipPlane[0] = std::strtod(argv[++i], NULL);
            clipPlane[1] = std::strtod(argv[++i], NULL);
            clipPlane[2] = std::strtod(argv[++i], NULL);
            clipPlane[3] = std::strtod(argv[++i], NULL);
            _clipPlanes.push_back(clipPlane);
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

