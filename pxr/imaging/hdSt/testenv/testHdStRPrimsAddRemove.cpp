//
// Copyright 2023 Pixar
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
        _refineLevel = 0;
        _cullStyle = HdCullStyleNothing;
        _addRemoveBasisCurves = false;

        SetCameraRotate(60.0f, 0.0f);
        SetCameraTranslate(GfVec3f(0, 0, -20.0f - 1.7320508f * 2.0f));
    }

    // HdSt_UnitTestGLDrawing overrides
    void InitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;
    void Present(uint32_t framebuffer) override;

protected:
    void ParseArgs(int argc, char* argv[]) override;

    // Test add/remove basisCurves.
    void AddRemoveBasisCurves(double& xPos, double& yPos);

private:
    HdSt_TestDriver* _driver;

    int _refineLevel;
    HdCullStyle _cullStyle;
    std::string _outputFilePath;
    bool _addRemoveBasisCurves;
};

////////////////////////////////////////////////////////////

void
My_TestGLDrawing::InitTest()
{
    std::cout << "My_TestGLDrawing::InitTest() " << "\n";

    _driver = new HdSt_TestDriver();
    HdUnitTestDelegate& delegate = _driver->GetDelegate();
    delegate.SetRefineLevel(_refineLevel);

    _driver->SetClearColor(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
    _driver->SetClearDepth(1.0f);
    _driver->SetupAovs(GetWidth(), GetHeight());
}

void My_TestGLDrawing::AddRemoveBasisCurves(double &xPos, double& yPos)
{
    GfMatrix4d dmat;
    HdUnitTestDelegate& delegate = _driver->GetDelegate();

    // Add Curves
    dmat.SetTranslate(GfVec3d(xPos, yPos, 0.0));
    delegate.AddCurves(SdfPath("/curve1"), HdTokens->linear, TfToken(), HdTokens->none,
        GfMatrix4f(dmat), HdInterpolationVertex, HdInterpolationVertex);
    yPos += 3.0;

    dmat.SetTranslate(GfVec3d(xPos, yPos, 0.0));
    delegate.AddCurves(SdfPath("/curve2"), HdTokens->cubic, HdTokens->bezier, HdTokens->none,
        GfMatrix4f(dmat), HdInterpolationVertex, HdInterpolationVertex);
    yPos += 3.0;

    dmat.SetTranslate(GfVec3d(xPos, yPos, 0.0));
    delegate.AddCurves(SdfPath("/curve3"), HdTokens->cubic, HdTokens->bSpline, HdTokens->none,
        GfMatrix4f(dmat), HdInterpolationVertex, HdInterpolationVertex);
    yPos += 3.0;

    dmat.SetTranslate(GfVec3d(xPos, yPos, 0.0));
    delegate.AddCurves(SdfPath("/curve4"), HdTokens->cubic, HdTokens->catmullRom, HdTokens->none,
        GfMatrix4f(dmat), HdInterpolationVertex, HdInterpolationVertex);
    yPos += 3.0;
    xPos += 3.0;

    dmat.SetTranslate(GfVec3d(xPos, yPos, 0.0));
    delegate.AddCurves(SdfPath("/curve5"), HdTokens->linear, TfToken(), HdTokens->none,
        GfMatrix4f(dmat), HdInterpolationVertex, HdInterpolationVertex);
    yPos += 3.0;

    dmat.SetTranslate(GfVec3d(xPos, yPos, 0.0));
    delegate.AddCurves(SdfPath("/curve6"), HdTokens->linear, TfToken(), HdTokens->dashDot,
        GfMatrix4f(dmat), HdInterpolationVertex, HdInterpolationVertex);
    yPos += 3.0;

    dmat.SetTranslate(GfVec3d(xPos, yPos, 0.0));
    delegate.AddCurves(SdfPath("/curve7"), HdTokens->linear, TfToken(), HdTokens->screenSpaceDashDot,
        GfMatrix4f(dmat), HdInterpolationVertex, HdInterpolationConstant);
    yPos += 3.0;

    dmat.SetTranslate(GfVec3d(xPos, yPos, 0.0));
    delegate.AddCurves(SdfPath("/curve8"), HdTokens->linear, TfToken(), HdTokens->dashDot,
        GfMatrix4f(dmat), HdInterpolationVertex, HdInterpolationConstant);
    yPos += 3.0;

    dmat.SetTranslate(GfVec3d(xPos, yPos, 0.0));
    delegate.AddCurves(SdfPath("/curve9"), HdTokens->linear, TfToken(), HdTokens->screenSpaceDashDot,
        GfMatrix4f(dmat), HdInterpolationVertex, HdInterpolationConstant);

    // Remove Curves
    delegate.Remove(SdfPath("/curve1"));
    delegate.Remove(SdfPath("/curve2"));
    delegate.Remove(SdfPath("/curve6"));
    delegate.Remove(SdfPath("/curve7"));
    delegate.Remove(SdfPath("/curve8"));
    delegate.Remove(SdfPath("/curve9"));
}

void
My_TestGLDrawing::DrawTest()
{
    double xPos = 0.0;
    double yPos = 0.0;

    if (_addRemoveBasisCurves)
        AddRemoveBasisCurves(xPos, yPos);

    // center camera
    SetCameraTranslate(GetCameraTranslate() - GfVec3f(xPos / 2.0, yPos / 2.0, 0));

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
My_TestGLDrawing::ParseArgs(int argc, char* argv[])
{
    for (int i = 0; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--refineLevel") {
            _refineLevel = atoi(argv[++i]);
        }
        else if (arg == "--cullStyle") {
            std::string style = argv[++i];
            if (style == "Nothing") {
                _cullStyle = HdCullStyleNothing;
            }
            else if (style == "Back") {
                _cullStyle = HdCullStyleBack;
            }
            else if (style == "Front") {
                _cullStyle = HdCullStyleFront;
            }
            else if (style == "BackUnlessDoubleSided") {
                _cullStyle = HdCullStyleBackUnlessDoubleSided;
            }
            else if (style == "FrontUnlessDoubleSided") {
                _cullStyle = HdCullStyleFrontUnlessDoubleSided;
            }
            else {
                std::cerr << "Error: Unknown cullstyle = " << style << "\n";
                exit(EXIT_FAILURE);
            }
        }
        else if (arg == "--addRemoveBasisCurves") {
            _addRemoveBasisCurves = true;
        }
        else if (arg == "--write") {
            _outputFilePath = argv[++i];
        }
    }
}

void
BasicTest(int argc, char* argv[])
{
    My_TestGLDrawing driver;
    driver.RunTest(argc, argv);
}

int main(int argc, char* argv[])
{
    BasicTest(argc, argv);
    std::cout << "OK" << std::endl;
    return EXIT_SUCCESS;
}

