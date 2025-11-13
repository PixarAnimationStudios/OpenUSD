//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/unitTestDelegate.h"
#include "pxr/imaging/hdSt/renderPass.h"
#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3d.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>
#include <memory>

// This test draws two almost identical cubes with identical transforms, in two
// different passes. The only difference is that the first cube is white, while
// the second is black. Since both cubes have exactly the same vertices, and
// HdCmpFuncLess is used for depth, the black cube should be completely behind
// the white cube (not even z-fighting). But if we give it even the smallest
// amount of depth bias forward, it should instead completely replace the white
// cube.

PXR_NAMESPACE_USING_DIRECTIVE

class HdSt_MyTestDriver : public HdSt_TestDriverBase<HdUnitTestDelegate>
{
public:
    HdSt_MyTestDriver()
    {
        _renderPassStates = {std::dynamic_pointer_cast<HdStRenderPassState>(
                                 _GetRenderDelegate()->CreateRenderPassState()),
            std::dynamic_pointer_cast<HdStRenderPassState>(
                _GetRenderDelegate()->CreateRenderPassState())};

        _renderPassStates[0]->SetDepthFunc(HdCmpFuncLess);
        _renderPassStates[0]->SetCullStyle(HdCullStyleNothing);
        _renderPassStates[0]->SetDepthBiasUseDefault(false);

        _renderPassStates[1]->SetDepthFunc(HdCmpFuncLess);
        _renderPassStates[1]->SetCullStyle(HdCullStyleNothing);
        _renderPassStates[1]->SetDepthBiasUseDefault(false);
        _renderPassStates[1]->SetDepthBias(-1, 0);

        _Init();

        const HdRprimCollection collections[] = {
            HdRprimCollection(HdTokens->geometry,
                HdReprSelector(HdReprTokens->smoothHull), SdfPath("/white")),
            HdRprimCollection(HdTokens->geometry,
                HdReprSelector(HdReprTokens->smoothHull), SdfPath("/black")),
        };

        for (size_t i = 0; i < std::size(collections); i++) {
            _renderPasses.push_back(std::make_shared<HdSt_RenderPass>(
                &GetDelegate().GetRenderIndex(), collections[i]));
        }
    }

    void Draw(bool enableDepthBias)
    {
        // The unit test AOV setup sets a clear value, which we need to remove
        // so we can do a second pass on the same contents.
        auto aovs = _renderPassStates[1]->GetAovBindings();
        for (auto& aov : aovs) {
            aov.clearValue = VtValue{};
        }
        _renderPassStates[1]->SetAovBindings(aovs);

        _renderPassStates[1]->SetDepthBiasEnabled(enableDepthBias);

        HdTaskSharedPtrVector tasks;
        for (size_t i = 0; i < _renderPassStates.size(); i++) {
            tasks.push_back(std::make_shared<HdSt_DrawTask>(_renderPasses[i],
                _renderPassStates[i],
                TfTokenVector{HdRenderTagTokens->geometry}));
        }
        _GetEngine()->Execute(&GetDelegate().GetRenderIndex(), &tasks);
    }
};

class My_TestGLDrawing : public HdSt_UnitTestGLDrawing
{
public:
    My_TestGLDrawing()
    {
        SetCameraRotate(60.0f, 0.0f);
        SetCameraTranslate(GfVec3f(0, 0, -20.0f));
    }

    void InitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;
    void Present(uint32_t framebuffer) override;

protected:
    void ParseArgs(int argc, char* argv[]) override;

    void _Draw(bool enableDepthBias = false);

private:
    std::unique_ptr<HdSt_MyTestDriver> _driver;
};

////////////////////////////////////////////////////////////////

void
My_TestGLDrawing::InitTest()
{
    std::cout << "My_TestGLDrawing::InitTest()" << std::endl;

    _driver = std::make_unique<HdSt_MyTestDriver>();
    _driver->SetClearColor(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
    _driver->SetClearDepth(1.0f);
    _driver->SetupAovs(GetWidth(), GetHeight());

    const auto cubeTransform = GfMatrix4f(1.0f).SetScale(4.0f) *
        GfMatrix4f(1.0f).SetRotate(GfRotation(GfVec3f(0, 0, 1), 45));

    HdUnitTestDelegate& delegate = _driver->GetDelegate();
    const SdfPath whiteCube{"/white/cube"};
    delegate.AddCube(whiteCube, cubeTransform, false, SdfPath(),
        PxOsdOpenSubdivTokens->none);
    delegate.UpdatePrimvarValue(
        whiteCube, HdTokens->displayColor, VtValue(GfVec3f(1)));

    const SdfPath blackCube{"/black/cube"};
    delegate.AddCube(blackCube, cubeTransform, false, SdfPath(),
        PxOsdOpenSubdivTokens->none);
    delegate.UpdatePrimvarValue(
        blackCube, HdTokens->displayColor, VtValue(GfVec3f(0)));
}

void
My_TestGLDrawing::_Draw(bool enableDepthBias)
{
    int width = GetWidth(), height = GetHeight();
    GfMatrix4d viewMatrix = GetViewMatrix();
    GfMatrix4d projMatrix = GetProjectionMatrix();

    _driver->SetCamera(viewMatrix, projMatrix,
        CameraUtilFraming(GfRect2i(GfVec2i(0, 0), width, height)));

    _driver->UpdateAovDimensions(width, height);

    _driver->Draw(enableDepthBias);
}

void
My_TestGLDrawing::DrawTest()
{
    _Draw();
}

void
My_TestGLDrawing::OffscreenTest()
{
    _Draw(/*enableDepthBias=*/false);
    _driver->WriteToFile("color", "testHdStDepthBias_disabled.png");

    _Draw(/*enableDepthBias=*/true);
    _driver->WriteToFile("color", "testHdStDepthBias_enabled.png");
}

void
My_TestGLDrawing::Present(uint32_t framebuffer)
{
    _driver->Present(GetWidth(), GetHeight(), framebuffer);
}

void
My_TestGLDrawing::ParseArgs(int argc, char* argv[])
{
}

void
BasicTest(int argc, char* argv[])
{
    My_TestGLDrawing driver;
    driver.RunTest(argc, argv);
}

int
main(int argc, char* argv[])
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
