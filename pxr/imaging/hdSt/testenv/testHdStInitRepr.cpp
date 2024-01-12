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
    My_TestGLDrawing();

    // HdSt_UnitTestGLDrawing overrides
    void InitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;
    void Present(uint32_t framebuffer) override;

protected:
    void ParseArgs(int argc, char *argv[]) override;

private:
    HdSt_TestDriver     *_driver;
    HdUnitTestDelegate *_delegate;
    HdRenderIndex       *_renderIndex;
    std::string          _outputFilePrefix;

    HdRenderPassSharedPtr _renderPasses[2];

    void OutputFrame(HdRenderPassSharedPtr &renderPass, int outputNum);
};

////////////////////////////////////////////////////////////

My_TestGLDrawing::My_TestGLDrawing()
 : HdSt_UnitTestGLDrawing()
 , _driver(nullptr)
 , _delegate(nullptr)
 , _renderIndex(nullptr)
 , _outputFilePrefix()
 , _renderPasses()
{
    SetCameraRotate(60.0f, 0.0f);
    SetCameraTranslate(GfVec3f(0, 0, -20.0f-1.7320508f*2.0f));
}


void
My_TestGLDrawing::InitTest()
{
    std::cout << "My_TestGLDrawing::InitTest()\n";

    _driver = new HdSt_TestDriver(HdReprTokens->hull);
    _delegate = &_driver->GetDelegate();

    _renderIndex = &_delegate->GetRenderIndex();
    HdRenderDelegate *renderDelegate = _renderIndex->GetRenderDelegate();

    HdRprimCollection collections[] = {
            HdRprimCollection(HdTokens->geometry, 
                HdReprSelector(HdReprTokens->refined)),
            HdRprimCollection(HdTokens->geometry, 
                HdReprSelector(HdReprTokens->refinedWireOnSurf)),
    };

    _renderPasses[0] = renderDelegate->CreateRenderPass(
                                                  _renderIndex, collections[0]);
    _renderPasses[1] = renderDelegate->CreateRenderPass(
                                                  _renderIndex, collections[1]);

    GfMatrix4d dmat;
    dmat.SetTranslate(GfVec3d(-3.0, 0.0, 0.0));
    _delegate->AddCube(SdfPath("/Cube0"), GfMatrix4f(dmat));

    dmat.SetTranslate(GfVec3d(3.0, 0.0, 0.0));
    _delegate->AddCube(SdfPath("/Cube1"), GfMatrix4f(dmat));

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

    OutputFrame(_renderPasses[0], 0);

    GfMatrix4f unitCube(1.0);
    _delegate->AddCube(SdfPath("/AddedCube"), unitCube);

    OutputFrame(_renderPasses[1], 1);

    OutputFrame(_renderPasses[0], 2);
}

void
My_TestGLDrawing::OffscreenTest()
{
    DrawTest();
}

/* virtual */
void
My_TestGLDrawing::ParseArgs(int argc, char *argv[])
{
    HdSt_UnitTestGLDrawing::ParseArgs(argc, argv);

    for (int i=0; i<argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--outputFilePrefix") {
            _outputFilePrefix = argv[++i];
        }
    }
}

void
My_TestGLDrawing::OutputFrame(HdRenderPassSharedPtr &renderPass, int outputNum)
{
    std::string filename;

    filename = _outputFilePrefix + "_" + std::to_string(outputNum) + ".png";

    _driver->Draw(renderPass, false);

    _driver->WriteToFile("color", filename.c_str());
}

void
My_TestGLDrawing::Present(uint32_t framebuffer)
{
    _driver->Present(GetWidth(), GetHeight(), framebuffer);
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

