//
// Copyright 2016 Pixar
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
#include "pxr/pxr.h"

#include "pxr/imaging/garch/glDebugWindow.h"

#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/imaging/hdx/selectionTask.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/unitTestDelegate.h"
#include "pxr/imaging/hdx/unitTestUtils.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>
#include <unordered_set>
#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (pickables)
);

class Hdx_TestDriver : public HdSt_TestDriverBase<Hdx_UnitTestDelegate>
{
public:
    Hdx_TestDriver();

    void DrawWithSelection(GfVec4d const &viewport, 
        HdxSelectionTrackerSharedPtr selTracker);

    HdSelectionSharedPtr Pick(GfVec2i const &startPos, GfVec2i const& endPos,
        int width, int height, GfFrustum const &frustum, 
        GfMatrix4d const &viewMatrix, bool doUnpickablesOcclude);

    void SetUnpickable(SdfPathVector const &excludePaths);

protected:
    void _Init(HdReprSelector const &reprSelector) override;

private:
    HdRprimCollection _pickablesCol;
};

Hdx_TestDriver::Hdx_TestDriver()
{
    _Init(HdReprSelector(HdReprTokens->hull));
}

void
Hdx_TestDriver::_Init(HdReprSelector const &reprSelector)
{
    _SetupSceneDelegate();
    
    Hdx_UnitTestDelegate & delegate = GetDelegate();

    // prepare render task
    SdfPath renderSetupTask("/renderSetupTask");
    SdfPath renderTask("/renderTask");
    SdfPath selectionTask("/selectionTask");
    SdfPath pickTask("/pickTask");
    delegate.AddRenderSetupTask(renderSetupTask);
    delegate.AddRenderTask(renderTask);
    delegate.AddSelectionTask(selectionTask);
    delegate.AddPickTask(pickTask);

    // render task parameters.
    VtValue vParam = delegate.GetTaskParam(renderSetupTask, HdTokens->params);
    HdxRenderTaskParams param = vParam.Get<HdxRenderTaskParams>();
    param.enableLighting = true; // use default lighting
    delegate.SetTaskParam(renderSetupTask, HdTokens->params, VtValue(param));
    delegate.SetTaskParam(renderTask, HdTokens->collection,
        VtValue(HdRprimCollection(HdTokens->geometry, reprSelector)));
        
    HdxSelectionTaskParams selParam;
    selParam.enableSelectionHighlight = true;
    selParam.selectionColor = GfVec4f(1, 1, 0, 1);
    selParam.locateColor = GfVec4f(1, 0, 1, 1);
    delegate.SetTaskParam(selectionTask, HdTokens->params, VtValue(selParam));

    // picking
    _pickablesCol = HdRprimCollection(_tokens->pickables, reprSelector);
    // We have to unfortunately explictly add collections besides 'geometry'
    // See HdRenderIndex constructor.
    delegate.GetRenderIndex().GetChangeTracker().AddCollection(
        _tokens->pickables);
}

void
Hdx_TestDriver::DrawWithSelection(GfVec4d const &viewport, 
    HdxSelectionTrackerSharedPtr selTracker)
{
    SdfPath renderSetupTask("/renderSetupTask");
    SdfPath renderTask("/renderTask");
    SdfPath selectionTask("/selectionTask");

    HdxRenderTaskParams param = GetDelegate().GetTaskParam(
        renderSetupTask, HdTokens->params).Get<HdxRenderTaskParams>();
    param.viewport = viewport;
    param.aovBindings = _aovBindings;
    GetDelegate().SetTaskParam(
        renderSetupTask, HdTokens->params, VtValue(param));

    HdTaskSharedPtrVector tasks;
    tasks.push_back(GetDelegate().GetRenderIndex().GetTask(renderSetupTask));
    tasks.push_back(GetDelegate().GetRenderIndex().GetTask(renderTask));
    tasks.push_back(GetDelegate().GetRenderIndex().GetTask(selectionTask));

    _GetEngine()->SetTaskContextData(
        HdxTokens->selectionState, VtValue(selTracker));
    _GetEngine()->Execute(&GetDelegate().GetRenderIndex(), &tasks);
}

HdSelectionSharedPtr
Hdx_TestDriver::Pick(GfVec2i const &startPos, GfVec2i const &endPos,
    int width, int height, GfFrustum const &frustum, 
    GfMatrix4d const &viewMatrix, bool doUnpickablesOcclude)
{
    HdxPickHitVector allHits;
    HdxPickTaskContextParams p;
    p.resolution = HdxUnitTestUtils::CalculatePickResolution(
        startPos, endPos, GfVec2i(4,4));
    p.resolveMode = HdxPickTokens->resolveUnique;
    p.doUnpickablesOcclude = doUnpickablesOcclude;
    p.viewMatrix = viewMatrix;
    p.projectionMatrix = HdxUnitTestUtils::ComputePickingProjectionMatrix(
        startPos, endPos, GfVec2i(width, height), frustum);
    p.collection = _pickablesCol;
    p.outHits = &allHits;

    HdTaskSharedPtrVector tasks;
    tasks.push_back(GetDelegate().GetRenderIndex().GetTask(
        SdfPath("/pickTask")));
    VtValue pickParams(p);
    _GetEngine()->SetTaskContextData(HdxPickTokens->pickParams, pickParams);
    _GetEngine()->Execute(&GetDelegate().GetRenderIndex(), &tasks);

    return HdxUnitTestUtils::TranslateHitsToSelection(
        p.pickTarget, HdSelection::HighlightModeSelect, allHits);
}

void
Hdx_TestDriver::SetUnpickable(SdfPathVector const &excludePaths) 
{
    _pickablesCol.SetExcludePaths(excludePaths);
}

// --------------------------------------------------------------------------

class My_TestGLDrawing : public HdSt_UnitTestGLDrawing
{
public:
    My_TestGLDrawing()
    {
        SetCameraRotate(0, 0);
        SetCameraTranslate(GfVec3f(0));
    }

    void DrawScene();
    void DrawMarquee();

    // HdSt_UnitTestGLDrawing overrides
    void InitTest() override;
    void UninitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;
    void Present(uint32_t framebuffer) override;
    void MousePress(int button, int x, int y, int modKeys) override;
    void MouseRelease(int button, int x, int y, int modKeys) override;
    void MouseMove(int x, int y, int modKeys) override;

protected:
    void _InitScene();
    HdSelectionSharedPtr _Pick(
        GfVec2i const& startPos, GfVec2i const& endPos,
        bool doUnpickablesOcclude);

private:
    std::unique_ptr<Hdx_TestDriver> _driver;

    HdxUnitTestUtils::Marquee _marquee;
    HdxSelectionTrackerSharedPtr _selTracker;

    GfVec2i _startPos, _endPos;
};

////////////////////////////////////////////////////////////

static GfMatrix4d
_GetTranslate(float tx, float ty, float tz)
{
    GfMatrix4d m(1.0f);
    m.SetRow(3, GfVec4f(tx, ty, tz, 1.0));
    return m;
}

void
My_TestGLDrawing::InitTest()
{
    _driver = std::make_unique<Hdx_TestDriver>();

    _selTracker.reset(new HdxSelectionTracker);

    // prepare scene
    _InitScene();
    SetCameraTranslate(GfVec3f(0, 0, -20));

    _marquee.InitGLResources();

    _driver->SetClearColor(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
    _driver->SetClearDepth(1.0f);
    _driver->SetupAovs(GetWidth(), GetHeight());
}

void
My_TestGLDrawing::UninitTest()
{
    _marquee.DestroyGLResources();
}

void
My_TestGLDrawing::_InitScene()
{
    Hdx_UnitTestDelegate &delegate = _driver->GetDelegate();

    delegate.AddCube(SdfPath("/cube0"), _GetTranslate( 0, 0, 0));
    delegate.AddCube(SdfPath("/cube1"), _GetTranslate( 0, 5, 1));
}

HdSelectionSharedPtr
My_TestGLDrawing::_Pick(GfVec2i const& startPos, GfVec2i const& endPos,
    bool doUnpickablesOcclude)
{
    return _driver->Pick(startPos, endPos, GetWidth(), GetHeight(), 
        GetFrustum(), GetViewMatrix(), doUnpickablesOcclude);
}

void
My_TestGLDrawing::DrawTest()
{
    DrawScene();
    DrawMarquee();
}

void
My_TestGLDrawing::OffscreenTest()
{
    DrawScene();
    _driver->WriteToFile("color", "color1_unselected.png");

    // select cube0
    HdSelectionSharedPtr selection =
        _Pick(GfVec2i(319,221), GfVec2i(320,222), false);
    _selTracker->SetSelection(selection);
    DrawScene();
    _driver->WriteToFile("color", "color2_cube0_pickable.png");
    HdSelection::HighlightMode mode = HdSelection::HighlightModeSelect;

    TF_VERIFY(selection->GetSelectedPrimPaths(mode).size() == 1);
    TF_VERIFY(selection->GetSelectedPrimPaths(mode)[0] == SdfPath("/cube0"));

    // make cube0 unpickable; it should not let us pick cube1 since it occludes
    _driver->SetUnpickable({ SdfPath("/cube0") });

    selection = _Pick(GfVec2i(319,221), GfVec2i(320,222), true);
    _selTracker->SetSelection(selection);
    DrawScene();
    _driver->WriteToFile("color", "color3_cube0_unpickable.png");
    //TF_VERIFY(selection->GetSelectedPrimPaths(mode).size() == 0);
}

void
My_TestGLDrawing::DrawScene()
{
    int width = GetWidth(), height = GetHeight();

    GfMatrix4d viewMatrix = GetViewMatrix();
    GfFrustum frustum = GetFrustum();

    GfVec4d viewport(0, 0, width, height);

    GfMatrix4d projMatrix = frustum.ComputeProjectionMatrix();
    _driver->GetDelegate().SetCamera(viewMatrix, projMatrix);

    _driver->UpdateAovDimensions(width, height);

    _driver->DrawWithSelection(viewport, _selTracker);

}

void
My_TestGLDrawing::DrawMarquee()
{
    _marquee.Draw(GetWidth(), GetHeight(), _startPos, _endPos);
}

void
My_TestGLDrawing::Present(uint32_t framebuffer)
{
    _driver->Present(GetWidth(), GetHeight(), framebuffer);
}

void
My_TestGLDrawing::MousePress(int button, int x, int y, int modKeys)
{
    HdSt_UnitTestGLDrawing::MousePress(button, x, y, modKeys);
    _startPos = _endPos = GetMousePos();
}

void
My_TestGLDrawing::MouseRelease(int button, int x, int y, int modKeys)
{
    HdSt_UnitTestGLDrawing::MouseRelease(button, x, y, modKeys);

    if (!(modKeys & GarchGLDebugWindow::Alt)) {
        HdSelectionSharedPtr selection = _Pick(_startPos, _endPos, false);
        _selTracker->SetSelection(selection);
    }
    _startPos = _endPos = GfVec2i(0);
}

void
My_TestGLDrawing::MouseMove(int x, int y, int modKeys)
{
    HdSt_UnitTestGLDrawing::MouseMove(x, y, modKeys);

    if (!(modKeys & GarchGLDebugWindow::Alt)) {
        _endPos = GetMousePos();
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
