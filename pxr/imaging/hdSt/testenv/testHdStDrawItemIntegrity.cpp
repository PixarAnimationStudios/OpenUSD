//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/unitTestDelegate.h"
#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"
#include "pxr/imaging/hdSt/renderPass.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3d.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>
#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

class HdSt_MyTestDriver : public HdSt_TestDriverBase<HdUnitTestDelegate>
{
public:
    HdSt_MyTestDriver()
    {
        _renderPassStates = {
            std::dynamic_pointer_cast<HdStRenderPassState>(
                _GetRenderDelegate()->CreateRenderPassState()) };
        _renderPassStates[0]->SetDepthFunc(HdCmpFuncLess);
        _renderPassStates[0]->SetCullStyle(HdCullStyleNothing);

        // Init sets up the camera in the render pass state and
        // thus needs to be called after render pass state has been setup.
        _Init();

        // Setup _passes!
        // passes for hull, refined, wire, wireOnSurf
        const HdRprimCollection cols[4] = {
            HdRprimCollection(HdTokens->geometry, 
                              HdReprSelector(HdReprTokens->hull)),
            HdRprimCollection(HdTokens->geometry, 
                              HdReprSelector(HdReprTokens->refined)),
            HdRprimCollection(HdTokens->geometry, 
                              HdReprSelector(HdReprTokens->wire)),
            HdRprimCollection(HdTokens->geometry, 
                              HdReprSelector(HdReprTokens->wireOnSurf)),
        };

        for (size_t i = 0; i < 4; i++) {
            _renderPasses.push_back(
                std::make_shared<HdSt_RenderPass>(
                    &(GetDelegate().GetRenderIndex()), cols[i]));
        }
    }

    void Draw(const std::vector<int> &passIdx)
    {
        HdTaskSharedPtrVector tasks;
        for (int idx : passIdx) {
            tasks.push_back(
                std::make_shared<HdSt_DrawTask>(
                    _renderPasses[idx],
                    _renderPassStates[0],
                    TfTokenVector{ HdRenderTagTokens->geometry }));
        }
        _GetEngine()->Execute(&(GetDelegate().GetRenderIndex()), &tasks);
    }
};

class My_TestGLDrawing : public HdSt_UnitTestGLDrawing
{
public:
    My_TestGLDrawing()
        : _lastRefineLevel(0)
    {
        SetCameraRotate(60.0f, 0.0f);
        SetCameraTranslate(GfVec3f(0, 0, -20.0f-1.7320508f*2.0f));
    }

    // Hd_UnitTestGLDrawing overrides
    void InitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;
    void Present(uint32_t framebuffer) override;

protected:
    virtual void ParseArgs(int argc, char *argv[]);

    void _Draw(const std::vector<int> & passIdx, int refineLevel);

private:
    std::unique_ptr<HdSt_MyTestDriver> _driver;

    int _lastRefineLevel;
};

////////////////////////////////////////////////////////////////

void
My_TestGLDrawing::InitTest()
{
    _driver = std::make_unique<HdSt_MyTestDriver>();

    HdUnitTestDelegate &delegate = _driver->GetDelegate();

    delegate.SetRefineLevel(0);

    delegate.PopulateInvalidPrimsSet();
    const GfVec3f center = delegate.PopulateBasicTestSet();

    // center camera
    SetCameraTranslate(GetCameraTranslate() - center);

    _driver->SetClearColor(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
    _driver->SetClearDepth(1.0f);
    _driver->SetupAovs(GetWidth(), GetHeight());
}

void
My_TestGLDrawing::_Draw(const std::vector<int> &passIdx, const int refineLevel)
{
    // camera
    int width = GetWidth(), height = GetHeight();
    GfMatrix4d viewMatrix = GetViewMatrix();
    GfMatrix4d projMatrix = GetProjectionMatrix();

    _driver->SetCamera(
        viewMatrix,
        projMatrix,
        CameraUtilFraming(GfRect2i(GfVec2i(0,0), width, height)));

    if (refineLevel != _lastRefineLevel) {
        _driver->GetDelegate().SetRefineLevel(refineLevel);
        _lastRefineLevel = refineLevel;
    }
    
    _driver->UpdateAovDimensions(width, height);

    _driver->Draw(passIdx);
}

void
My_TestGLDrawing::DrawTest()
{
    _Draw({0}, 0);
}

void
My_TestGLDrawing::OffscreenTest()
{
    HdUnitTestDelegate & delegate = _driver->GetDelegate();

    // All of these tests make sure that draw items are left in a valid state
    // after any rprim sync.

    // Test for bug 153473: 
    // Each of these draw calls will trigger a reset of the geometric shader.
    // We want to make sure that:
    // - (2) calls InitRepr for our new repr "refined", even though the scene
    //       is invisible.
    // - (3) the geometric shader is reset for "hull" (due to refine level
    //       change), even though "hull" won't get synced (verified via 4).
    // If either of those fails to happen, this test will crash.
    delegate.SetVisibility(false);
    /* (1) */ _Draw({0}, 0);
    /* (2) */ _Draw({1}, 0);
    delegate.SetVisibility(true);
    /* (3) */ _Draw({1}, 1);
    delegate.SetVisibility(false);
    /* (4) */ _Draw({0}, 0);
    delegate.SetVisibility(true);

    // Test for bug 155322:
    // If we draw a frame with both the "wire" and "wireOnSurf" reprs, we want
    // to make sure that prims get NewRepr set, and that both end up with a
    // geometric shader (i.e. NewRepr triggers a global rather than a local
    // rebuild of the geometric shader).
    /* (4) */ _Draw({2,3}, 0);
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

