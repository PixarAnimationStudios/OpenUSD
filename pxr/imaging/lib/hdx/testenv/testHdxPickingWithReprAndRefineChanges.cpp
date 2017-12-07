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

#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/garch/glDebugWindow.h"
#include "pxr/imaging/glf/drawTarget.h"

#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hdSt/renderDelegate.h"

#include "pxr/imaging/hdx/selectionTask.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/unitTestDelegate.h"
#include "pxr/imaging/hdx/unitTestGLDrawing.h"
#include "pxr/imaging/hdx/unitTestUtils.h"

#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>
#include <unordered_set>
#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (pickables)
);

class My_TestGLDrawing : public Hdx_UnitTestGLDrawing {
public:
    My_TestGLDrawing() {
        SetCameraRotate(0, 0);
        SetCameraTranslate(GfVec3f(0));
    }
    ~My_TestGLDrawing();

    void DrawScene();
    void DrawMarquee();

    // Hdx_UnitTestGLDrawing overrides
    virtual void InitTest();
    virtual void UninitTest();
    virtual void DrawTest();
    virtual void OffscreenTest();

    virtual void MousePress(int button, int x, int y, int modKeys);
    virtual void MouseRelease(int button, int x, int y, int modKeys);
    virtual void MouseMove(int x, int y, int modKeys);

protected:
    void _InitScene();
    void _SetPickParams();
    void _Clear();

private:
    HdEngine _engine;
    HdStRenderDelegate _renderDelegate;
    HdRenderIndex *_renderIndex;
    std::unique_ptr<Hdx_UnitTestDelegate> _delegate;
    
    HdRprimCollection _pickablesCol;
    HdxUnitTestUtils::Picker _picker;
    HdxUnitTestUtils::Marquee _marquee;

    GfVec2i _startPos, _endPos;
};

////////////////////////////////////////////////////////////

GLuint vao;

static GfMatrix4d
_GetTranslate(float tx, float ty, float tz)
{
    GfMatrix4d m(1.0f);
    m.SetRow(3, GfVec4f(tx, ty, tz, 1.0));
    return m;
}

My_TestGLDrawing::~My_TestGLDrawing()
{
    delete _renderIndex;
}

void
My_TestGLDrawing::InitTest()
{
    _renderIndex = HdRenderIndex::New(&_renderDelegate);
    TF_VERIFY(_renderIndex != nullptr);
    _delegate.reset(new Hdx_UnitTestDelegate(_renderIndex));

    // prepare render task
    SdfPath renderSetupTask("/renderSetupTask");
    SdfPath renderTask("/renderTask");
    SdfPath selectionTask("/selectionTask");
    _delegate->AddRenderSetupTask(renderSetupTask);
    _delegate->AddRenderTask(renderTask);
    _delegate->AddSelectionTask(selectionTask);

    // render task parameters.
    VtValue vParam = _delegate->GetTaskParam(renderSetupTask, HdTokens->params);
    HdxRenderTaskParams param = vParam.Get<HdxRenderTaskParams>();
    param.enableLighting = true; // use default lighting
    _delegate->SetTaskParam(renderSetupTask, HdTokens->params,
                            VtValue(param));
    _delegate->SetTaskParam(renderTask, HdTokens->collection,
                            VtValue(HdRprimCollection(HdTokens->geometry, HdTokens->hull)));
    HdxSelectionTaskParams selParam;
    selParam.enableSelection = true;
    selParam.selectionColor = GfVec4f(1, 1, 0, 1);
    selParam.locateColor = GfVec4f(1, 0, 1, 1);
    selParam.maskColor = GfVec4f(0, 1, 1, 1);
    _delegate->SetTaskParam(selectionTask, HdTokens->params,
                            VtValue(selParam));

    // prepare scene
    _InitScene();
    SetCameraTranslate(GfVec3f(0, 0, -20));

    // picking related init
    // The collection used for the ID render defaults to including the root path
    // which essentially means that all scene graph prims are pickable.
    // 
    // Worth noting that the collection's repr is set to refined (and not 
    // hull). When a prim has an authored repr, we'll use that instead, as
    // the collection's forcedRepr defaults to false.
    _pickablesCol = HdRprimCollection(_tokens->pickables, HdTokens->refined);
    _marquee.InitGLResources();
    _picker.InitIntersector(_renderIndex);
    _SetPickParams();
    // We have to unfortunately explictly add collections besides 'geometry'
    // See HdRenderIndex constructor.
    _delegate->GetRenderIndex().GetChangeTracker().AddCollection(_tokens->pickables);

    // XXX: Setup a VAO, the current drawing engine will not yet do this.
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindVertexArray(0);
}

void
My_TestGLDrawing::UninitTest()
{
    _marquee.DestroyGLResources();
}

void
My_TestGLDrawing::_InitScene()
{
    _delegate->AddCube(SdfPath("/cube1"), _GetTranslate(-5, 0, 5));
    _delegate->AddCube(SdfPath("/cube2"), _GetTranslate(-5, 0,-5));
}

void
My_TestGLDrawing::_SetPickParams()
{
    HdxUnitTestUtils::PickParams pParams;

    pParams.pickRadius     = GfVec2i(4,4);
    pParams.screenWidth    = GetWidth();
    pParams.screenHeight   = GetHeight();
    pParams.viewFrustum    = GetFrustum();
    pParams.viewMatrix     = GetViewMatrix();
    pParams.engine         = &_engine;
    pParams.pickablesCol   = &_pickablesCol;
    pParams.highlightMode  = HdxSelectionHighlightModeSelect;

    _picker.SetPickParams(pParams);
}

void
My_TestGLDrawing::_Clear()
{
    GLfloat clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    glClearBufferfv(GL_COLOR, 0, clearColor);

    GLfloat clearDepth[1] = { 1.0f };
    glClearBufferfv(GL_DEPTH, 0, clearDepth);
}

void
My_TestGLDrawing::DrawTest()
{
    _Clear();

    DrawScene();

    DrawMarquee();
}

void
My_TestGLDrawing::OffscreenTest()
{
    DrawScene();
    WriteToFile("color", "color1_unselected.png");

    // This test uses 2 collections:
    // (i)  geometry
    // (ii) pickables
    // Picking in this test uses the 'refined' repr. See the collection
    // created in Pick(..) for additional notes.
    // 
    // We want to ensure that these collections' command buffers are updated
    // correctly in the following scenarios:
    // - changing a prim's refine level when using a different non-authored
    // repr from that in the pickables collection 
    // - changing a prim's repr accounts for refineLevel dirtyness intercepted
    // by the picking task.
    // 
    // This test is run with the scene repr = 'hull'. We want to test several
    // cases:
    // (a) Change refine level on prim A with repr hull ==> Drawn image should
    //  not change, since hull doesn't update topology on refinement. The
    //  picking collection will however reflect this change (making this a
    //  weird scenario)
    // 
    // (b) Change repr on prim B ==> Drawn image should reflect the new repr
    //          
    // (c) Change repr on prim A ==> Drawn image should reflect the refineLevel
    //  update in (a) if its repr supports it (refined, refinedWire, refinedWireOnSurf)
    //  
    // (d) Change refine level on prim B ==> Drawn image should reflect the refineLevel
    //  if its repr supports it (refined, refinedWire, refinedWireOnSurf)

    HdxSelectionHighlightMode mode = HdxSelectionHighlightModeSelect;
    HdxSelectionSharedPtr selection;

    // (a)
    {
        _picker.Pick(GfVec2i(0,0), GfVec2i(0,0)); // deselect

        std::cout << "Changing refine level of cube1" << std::endl;
        _delegate->SetRefineLevel(SdfPath("/cube1"), 2);
        // The repr corresponding to picking (refined) would be the one that
        // handles the DirtyRefineLevel bit, since we don't call DrawScene()
        // before Pick(). We don't explicitly mark the collections dirty in this
        // case, since refine level changes trigger change tracker garbage 
        // collection and the render delegate marks all collections dirty.
        // See HdStRenderDelegate::CommitResources
        // XXX: This is hacky.
        // 
        // Since we're not overriding the scene repr, cube1 will still
        // appear unrefined, since it defaults to the hull repr.
        // However, the picking collection will render the refined version, and
        // we won't be able to select cube1 by picking the unrefined version's
        // left top corner.
        _picker.Pick(GfVec2i(138, 60), GfVec2i(138, 60));
        DrawScene();
        WriteToFile("color", "color2_refine_wont_change_cube1.png");
        selection = _picker.GetSelection();
        TF_VERIFY(selection->GetSelectedPrims(mode).size() == 0);
    }

    // (b)
    {
        _picker.Pick(GfVec2i(0,0), GfVec2i(0,0)); // deselect

        std::cout << "Changing repr for cube2" << std::endl;
        _delegate->SetReprName(SdfPath("/cube2"), HdTokens->refinedWireOnSurf);

        // XXX: Repr initialization doesn't currently trigger the collection's
        // command buffer to be rebuilt, so we have to do that explicitly here.
        // Failing to do so will show us a stale repr draw item (i.e. hull)
        _delegate->GetRenderIndex().GetChangeTracker().MarkAllCollectionsDirty();

        _picker.Pick(GfVec2i(152, 376), GfVec2i(152, 376));
        DrawScene();
        WriteToFile("color", "color3_repr_change_cube2.png");
        selection = _picker.GetSelection();
        TF_VERIFY(selection->GetSelectedPrims(mode).size() == 1);
        TF_VERIFY(selection->GetSelectedPrims(mode)[0] == SdfPath("/cube2"));
    }

    // (c)
    {
        _picker.Pick(GfVec2i(0,0), GfVec2i(0,0)); // deselect

       std::cout << "Changing repr on cube1" << std::endl;

        _delegate->SetReprName(SdfPath("/cube1"), HdTokens->refinedWire);
        // XXX: If we don't mark the collection dirty, it'll be drawn with the
        // same command buffer and thus, cube1 will appear as hull and 
        // not refinedWireOnSurf.
        _delegate->GetRenderIndex().GetChangeTracker().MarkAllCollectionsDirty();

        _picker.Pick(GfVec2i(176, 96), GfVec2i(179, 99));
        DrawScene();
        WriteToFile("color", "color4_repr_and_refine_change_cube1.png");
        selection = _picker.GetSelection();
        TF_VERIFY(selection->GetSelectedPrims(mode).size() == 1);
        TF_VERIFY(selection->GetSelectedPrims(mode)[0] == SdfPath("/cube1"));
    }


    // (d)
    {
        _picker.Pick(GfVec2i(0,0), GfVec2i(0,0)); // deselect

        std::cout << "## Changing refine level of cube2 ##" << std::endl;
        _delegate->SetRefineLevel(SdfPath("/cube2"), 3);

        _picker.Pick(GfVec2i(152, 376), GfVec2i(152, 376));
        DrawScene();
        WriteToFile("color", "color5_refine_change_cube2.png");
        selection = _picker.GetSelection();
        // XXX: A bug in primvar sharing (HD_ENABLE_SHARED_VERTEX_PRIMVAR)
        // makes cube2 disappear because the vertex primvar BAR is not
        // updated correctly. The dumped image and the verify below will
        // fail once it is fixed.
        TF_VERIFY(selection->GetSelectedPrims(mode).size() == 0);
        //TF_VERIFY(selection->GetSelectedPrims(mode)[0] == SdfPath("/cube2"));
    }

     // deselect    
    _picker.Pick(GfVec2i(0,0), GfVec2i(0,0));
    DrawScene();
    WriteToFile("color", "color6_unselected.png");
}

void
My_TestGLDrawing::DrawScene()
{
    _Clear();

    int width = GetWidth(), height = GetHeight();

    GfMatrix4d viewMatrix = GetViewMatrix();
    GfFrustum frustum = GetFrustum();

    GfVec4d viewport(0, 0, width, height);

    GfMatrix4d projMatrix = frustum.ComputeProjectionMatrix();
    _delegate->SetCamera(viewMatrix, projMatrix);

    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

    SdfPath renderSetupTask("/renderSetupTask");
    SdfPath renderTask("/renderTask");
    SdfPath selectionTask("/selectionTask");

    // viewport
    HdxRenderTaskParams param
        = _delegate->GetTaskParam(
            renderSetupTask, HdTokens->params).Get<HdxRenderTaskParams>();
    param.viewport = viewport;
    _delegate->SetTaskParam(renderSetupTask, HdTokens->params, VtValue(param));

    HdTaskSharedPtrVector tasks;
    tasks.push_back(_delegate->GetRenderIndex().GetTask(renderSetupTask));
    tasks.push_back(_delegate->GetRenderIndex().GetTask(renderTask));
    tasks.push_back(_delegate->GetRenderIndex().GetTask(selectionTask));

    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(vao);

    VtValue v(_picker.GetSelectionTracker());
    _engine.SetTaskContextData(HdxTokens->selectionState, v);

    _engine.Execute(_delegate->GetRenderIndex(), tasks);

    glBindVertexArray(0);
}

void
My_TestGLDrawing::DrawMarquee()
{
    _marquee.Draw(GetWidth(), GetHeight(), _startPos, _endPos);
}

void
My_TestGLDrawing::MousePress(int button, int x, int y, int modKeys)
{
    Hdx_UnitTestGLDrawing::MousePress(button, x, y, modKeys);
    _startPos = _endPos = GetMousePos();
}

void
My_TestGLDrawing::MouseRelease(int button, int x, int y, int modKeys)
{
    Hdx_UnitTestGLDrawing::MouseRelease(button, x, y, modKeys);

    if (!(modKeys & GarchGLDebugWindow::Alt)) {
        _picker.Pick(_startPos, _endPos);
    }
    _startPos = _endPos = GfVec2i(0);
}

void
My_TestGLDrawing::MouseMove(int x, int y, int modKeys)
{
    Hdx_UnitTestGLDrawing::MouseMove(x, y, modKeys);

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