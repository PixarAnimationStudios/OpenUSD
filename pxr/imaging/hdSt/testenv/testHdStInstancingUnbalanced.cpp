//
// Copyright 2021 Pixar
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

protected:
    void ParseArgs(int argc, char *argv[]) override;

private:
    HdSt_TestDriver* _driver;

    TfToken _reprName;
    int _refineLevel;
    std::string _outputFilePath;
};

////////////////////////////////////////////////////////////

My_TestGLDrawing::My_TestGLDrawing()
{
    SetCameraRotate(60.0f, 45.0f);
    SetCameraTranslate(GfVec3f(-5, -5, -20));
    _refineLevel = 0;
    _reprName = HdReprTokens->hull;
}

void
My_TestGLDrawing::InitTest()
{
    _driver = new HdSt_TestDriver(_reprName);
    HdUnitTestDelegate &delegate = _driver->GetDelegate();
    delegate.SetRefineLevel(_refineLevel);

    // create instancer hierarchy
    /*
          /i0
             |
             +--- proto1: cube1
             |
             +--- proto2: i1 (instancer)
                    |
                    +--- proto1: grid1
                    |
                    +--- proto2: i2 (instancer)
                           |
                           +--- proto1: grid2

     */

    SdfPath i0("/i0");
    SdfPath i1("/i0/i1");
    SdfPath i2("/i0/i1/i2");
    SdfPath cube("/i0/cube");
    SdfPath grid1("/i0/i1/grid1");
    SdfPath grid2("/i0/i1/i2/grid2");
    delegate.AddInstancer(i0);

    int n = 7;
    VtVec3fArray scale(n);
    VtVec4fArray rotate(n);
    VtVec3fArray translate(n);
    VtIntArray prototypeIndex(n);
    for (int i = 0; i < n; ++i) {
        scale[i] = GfVec3f(1, 1, 1);
        rotate[i] = GfVec4f(0, 0, 0, 0);
        translate[i] = GfVec3f(i*3, 0, 0);
        prototypeIndex[i] = i % 2; // 0, 1, 0, 1
    }
    delegate.SetInstancerProperties(
        i0, prototypeIndex, scale, rotate, translate);

    // prototypes
    delegate.AddCube(cube, GfMatrix4f(1), false, i0);
    delegate.AddInstancer(i1, i0);

    {
        int n = 4;
        VtVec3fArray scale(n);
        VtVec4fArray rotate(n);
        VtVec3fArray translate(n);
        VtIntArray prototypeIndex(n);
        for (int i = 0; i < n; ++i) {
            scale[i] = GfVec3f(1, 1, 1);
            rotate[i] = GfVec4f(0, 0, 0, 0);
            translate[i] = GfVec3f(0, i*3, 0);
            prototypeIndex[i] = i % 2;
        }
        delegate.SetInstancerProperties(
            i1, prototypeIndex, scale, rotate, translate);

        // prototypes
        delegate.AddGridWithFaceColor(
            grid1, 4, 4, GfMatrix4f(1), true, false, i1);
        delegate.AddInstancer(i2, i1);

        {
            int n = 8;
            VtVec3fArray scale(n);
            VtVec4fArray rotate(n);
            VtVec3fArray translate(n);
            VtIntArray prototypeIndex(n);
            for (int i = 0; i < n; ++i) {
                scale[i] = GfVec3f(1, 1, 1);
                rotate[i] = GfVec4f(0, 0, 0, 0);
                translate[i] = GfVec3f(0, 0, -i);
                prototypeIndex[i] = 0;
            }
            delegate.SetInstancerProperties(
                i2, prototypeIndex, scale, rotate, translate);

            // prototypes
            delegate.AddGridWithVertexColor(
                grid2, 4, 4, GfMatrix4f(1), true, false, i2);
        }
    }

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

