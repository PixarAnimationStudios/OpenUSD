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
        _reprName = HdTokens->hull;
        _refineLevel = 0;
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
    virtual void ParseArgs(int argc, char *argv[]);
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

    TfToken _reprName;
    int _refineLevel;
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
    _delegate->SetRefineLevel(_refineLevel);

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
                            VtValue(HdRprimCollection(HdTokens->geometry, _reprName)));
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
    _delegate->AddCube(SdfPath("/cube0"), _GetTranslate( 5, 0, 5));
    _delegate->AddCube(SdfPath("/cube1"), _GetTranslate(-5, 0, 5));
    _delegate->AddCube(SdfPath("/cube2"), _GetTranslate(-5, 0,-5));
    _delegate->AddCube(SdfPath("/cube3"), _GetTranslate( 5, 0,-5));

    {
        _delegate->AddInstancer(SdfPath("/instancerTop"));
        _delegate->AddCube(SdfPath("/protoTop"),
                         GfMatrix4d(1), false, SdfPath("/instancerTop"));

        std::vector<SdfPath> prototypes;
        prototypes.push_back(SdfPath("/protoTop"));

        VtVec3fArray scale(3);
        VtVec4fArray rotate(3);
        VtVec3fArray translate(3);
        VtIntArray prototypeIndex(3);

        scale[0] = GfVec3f(1);
        rotate[0] = GfVec4f(0);
        translate[0] = GfVec3f(3, 0, 2);
        prototypeIndex[0] = 0;

        scale[1] = GfVec3f(1);
        rotate[1] = GfVec4f(0);
        translate[1] = GfVec3f(0, 0, 2);
        prototypeIndex[1] = 0;

        scale[2] = GfVec3f(1);
        rotate[2] = GfVec4f(0);
        translate[2] = GfVec3f(-3, 0, 2);
        prototypeIndex[2] = 0;

        _delegate->SetInstancerProperties(SdfPath("/instancerTop"),
                                        prototypeIndex,
                                        scale, rotate, translate);
    }

    {
        _delegate->AddInstancer(SdfPath("/instancerBottom"));
        _delegate->AddTet(SdfPath("/protoBottom"),
                         GfMatrix4d(1), false, SdfPath("/instancerBottom"));
        _delegate->SetRefineLevel(SdfPath("/protoBottom"), 2);

        std::vector<SdfPath> prototypes;
        prototypes.push_back(SdfPath("/protoBottom"));

        VtVec3fArray scale(3);
        VtVec4fArray rotate(3);
        VtVec3fArray translate(3);
        VtIntArray prototypeIndex(3);

        scale[0] = GfVec3f(1);
        rotate[0] = GfVec4f(0);
        translate[0] = GfVec3f(3, 0, -2);
        prototypeIndex[0] = 0;

        scale[1] = GfVec3f(1);
        rotate[1] = GfVec4f(0);
        translate[1] = GfVec3f(0, 0, -2);
        prototypeIndex[1] = 0;

        scale[2] = GfVec3f(1);
        rotate[2] = GfVec4f(0);
        translate[2] = GfVec3f(-3, 0, -2);
        prototypeIndex[2] = 0;

        _delegate->SetInstancerProperties(SdfPath("/instancerBottom"),
                                        prototypeIndex,
                                        scale, rotate, translate);
    }
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
    pParams.highlightMode  = HdSelection::HighlightModeSelect;

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
    GLfloat clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    glClearBufferfv(GL_COLOR, 0, clearColor);

    GLfloat clearDepth[1] = { 1.0f };
    glClearBufferfv(GL_DEPTH, 0, clearDepth);

    DrawScene();
    WriteToFile("color", "color1_unselected.png");

    // --------------------- (active) selection high ---------------------------
    // select cube2
    HdSelection::HighlightMode mode = HdSelection::HighlightModeSelect;
    _picker.SetHighlightMode(mode);
    _picker.Pick(GfVec2i(180, 390), GfVec2i(181, 391));

    DrawScene();
    WriteToFile("color", "color2_select.png");
    HdSelectionSharedPtr selection = _picker.GetSelection();
    TF_VERIFY(selection->GetSelectedPrims(mode).size() == 1);
    TF_VERIFY(selection->GetSelectedPrims(mode)[0] == SdfPath("/cube2"));

    // select cube1, /protoTop:1, /protoTop:2, /protoBottom:1, /protoBottom:2
    _picker.Pick(GfVec2i(105,62), GfVec2i(328,288));
    DrawScene();
    WriteToFile("color", "color3_select.png");
    selection = _picker.GetSelection();

    TF_VERIFY(selection->GetSelectedPrims(mode).size() == 5);
    TF_VERIFY(selection->GetSelectedInstances(mode).size() == 2);
    {
        std::vector<VtIntArray> const& indices
            = *TfMapLookupPtr(selection->GetSelectedInstances(mode),
                             SdfPath("/protoTop"));
        TF_VERIFY(indices.size() == 2);
        TF_VERIFY(indices[0][0] == 1 || indices[0][0] == 2);
        TF_VERIFY(indices[1][0] == 1 || indices[1][0] == 2);
    }
    {
        std::vector<VtIntArray> const& indices
            = *TfMapLookupPtr(selection->GetSelectedInstances(mode),
                              SdfPath("/protoBottom"));
        TF_VERIFY(indices.size() == 2);
        TF_VERIFY(indices[0][0] == 1 || indices[0][0] == 2);
        TF_VERIFY(indices[1][0] == 1 || indices[1][0] == 2);
    }

    // --------------------- locate (rollover) selection -----------------------
    mode = HdSelection::HighlightModeLocate;
    _picker.SetHighlightMode(mode);
    // select cube0
    _picker.Pick(GfVec2i(472, 97), GfVec2i(473, 98));
    DrawScene();
    WriteToFile("color", "color4_locate.png");
    selection = _picker.GetSelection();
    TF_VERIFY(selection->GetSelectedPrims(mode).size() == 1);
    TF_VERIFY(selection->GetSelectedPrims(mode)[0] == SdfPath("/cube0"));

    // select cube3, /protoBottom:0
    _picker.Pick(GfVec2i(408,246), GfVec2i(546,420));
    DrawScene();
    WriteToFile("color", "color5_locate.png");
    selection = _picker.GetSelection();

    TF_VERIFY(selection->GetSelectedPrims(mode).size() == 2);
    TF_VERIFY(selection->GetSelectedInstances(mode).size() == 1);
    {
        std::vector<VtIntArray> const& indices
            = *TfMapLookupPtr(selection->GetSelectedInstances(mode),
                             SdfPath("/protoBottom"));
        TF_VERIFY(indices.size() == 1);
        TF_VERIFY(indices[0][0] == 0);
    }

    // ------------------------- mask  selection -------------------------------
    mode = HdSelection::HighlightModeMask;
    _picker.SetHighlightMode(mode);

    // select cube2
    _picker.Pick(GfVec2i(180, 390), GfVec2i(181, 391));
    DrawScene();
    WriteToFile("color", "color6_mask.png");

     // select cube3, /protoBottom:0
    _picker.Pick(GfVec2i(408,246), GfVec2i(546,420));
    DrawScene();
    WriteToFile("color", "color7_mask.png");
    selection = _picker.GetSelection();

    TF_VERIFY(selection->GetSelectedPrims(mode).size() == 2);
    TF_VERIFY(selection->GetSelectedInstances(mode).size() == 1);
    {
        std::vector<VtIntArray> const& indices
            = *TfMapLookupPtr(selection->GetSelectedInstances(mode),
                             SdfPath("/protoBottom"));
        TF_VERIFY(indices.size() == 1);
        TF_VERIFY(indices[0][0] == 0);
    }

    // deselect    
    _picker.Pick(GfVec2i(0,0), GfVec2i(0,0));
    DrawScene();
    WriteToFile("color", "color8_unselected.png");
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
My_TestGLDrawing::ParseArgs(int argc, char *argv[])
{
    for (int i=0; i<argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--repr") {
            _reprName = TfToken(argv[++i]);
        } else if (arg == "--refineLevel") {
            _refineLevel = atoi(argv[++i]);
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

