//
// Copyright 2020 Pixar
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

#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/garch/glDebugWindow.h"
#include "pxr/imaging/glf/drawTarget.h"

#include "pxr/imaging/hd/driver.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/selection.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdSt/unitTestGLDrawing.h"

#include "pxr/imaging/hdx/pickTask.h"
#include "pxr/imaging/hdx/selectionTask.h"
#include "pxr/imaging/hdx/selectionTracker.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/unitTestDelegate.h"
#include "pxr/imaging/hdx/unitTestUtils.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>
#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (meshPoints)
    (pickables)
);

class My_TestGLDrawing : public HdSt_UnitTestGLDrawing {
public:
    My_TestGLDrawing() 
    {
        SetCameraRotate(0, 0);
        SetCameraTranslate(GfVec3f(0));
        _reprName = HdReprTokens->wireOnSurf;
        _refineLevel = 0;
    }
    ~My_TestGLDrawing();

    void DrawScene();
    void DrawMarquee();
    
    // HdSt_UnitTestGLDrawing overrides
    void InitTest() override;
    void UninitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;

    void MousePress(int button, int x, int y, int modKeys) override;
    void MouseRelease(int button, int x, int y, int modKeys) override;
    void MouseMove(int x, int y, int modKeys) override;

protected:
    void ParseArgs(int argc, char *argv[]) override;
    void _InitScene();
    void _Clear();
    HdSelectionSharedPtr _Pick(
        GfVec2i const& startPos, GfVec2i const& endPos,
        TfToken const& pickTarget,
        TfToken const& resolveMode,
        HdxPickHitVector *allHits);

private:
    // Hgi and HdDriver should be constructed before HdEngine to ensure they
    // are destructed last. Hgi may be used during engine/delegate destruction.
    HgiUniquePtr _hgi;
    std::unique_ptr<HdDriver> _driver;
    HdEngine _engine;
    HdStRenderDelegate _renderDelegate;
    HdRenderIndex *_renderIndex;
    std::unique_ptr<Hdx_UnitTestDelegate> _delegate;
    
    HdRprimCollection _pickablesCol;
    HdxUnitTestUtils::Marquee _marquee;
    HdxSelectionTrackerSharedPtr _selTracker;

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
    _hgi = Hgi::CreatePlatformDefaultHgi();
    _driver.reset(new HdDriver{HgiTokens->renderDriver, VtValue(_hgi.get())});

    _renderIndex = HdRenderIndex::New(&_renderDelegate, {_driver.get()});
    TF_VERIFY(_renderIndex != nullptr);
    _delegate.reset(new Hdx_UnitTestDelegate(_renderIndex));
    _delegate->SetRefineLevel(_refineLevel);
    _selTracker.reset(new HdxSelectionTracker);

    // Add a meshPoints repr since it isn't populated in 
    // HdRenderIndex::_ConfigureReprs
    HdMesh::ConfigureRepr(_tokens->meshPoints,
                          HdMeshReprDesc(HdMeshGeomStylePoints,
                                         HdCullStyleNothing,
                                         HdMeshReprDescTokens->pointColor,
                                         /*flatShadingEnabled=*/true,
                                         /*blendWireframeColor=*/false));

    // prepare render task
    SdfPath renderSetupTask("/renderSetupTask");
    SdfPath renderTask("/renderTask");
    SdfPath selectionTask("/selectionTask");
    SdfPath pickTask("/pickTask");
    _delegate->AddRenderSetupTask(renderSetupTask);
    _delegate->AddRenderTask(renderTask);
    _delegate->AddSelectionTask(selectionTask);
    _delegate->AddPickTask(pickTask);

    // render task parameters.
    VtValue vParam = _delegate->GetTaskParam(renderSetupTask, HdTokens->params);
    HdxRenderTaskParams param = vParam.Get<HdxRenderTaskParams>();
    param.enableLighting = true; // use default lighting
    _delegate->SetTaskParam(renderSetupTask, HdTokens->params,
                            VtValue(param));
    // Use wireframe and enable points for edge and point picking.
    const auto sceneReprSel = HdReprSelector(HdReprTokens->wireOnSurf,
                                             HdReprTokens->disabled,
                                             _tokens->meshPoints);
    _delegate->SetTaskParam(renderTask, HdTokens->collection,
                            VtValue(HdRprimCollection(HdTokens->geometry, 
                                                      sceneReprSel)));
    HdxSelectionTaskParams selParam;
    selParam.enableSelection = true;
    selParam.selectionColor = GfVec4f(1, 1, 0, 1);
    selParam.locateColor = GfVec4f(1, 0, 1, 1);
    _delegate->SetTaskParam(selectionTask, HdTokens->params,
                            VtValue(selParam));

    // prepare scene
    _InitScene();
    SetCameraTranslate(GfVec3f(0, 0, -20));

    // picking related init
    _pickablesCol = HdRprimCollection(_tokens->pickables, 
                                      sceneReprSel);
    _marquee.InitGLResources();
    _delegate->GetRenderIndex().GetChangeTracker().AddCollection(
        _tokens->pickables);

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

HdSelectionSharedPtr
My_TestGLDrawing::_Pick(GfVec2i const& startPos, GfVec2i const& endPos,
                        TfToken const& pickTarget,
                        TfToken const& resolveMode,
                        HdxPickHitVector *allHits)
{
    if (!allHits) {
        return HdSelectionSharedPtr();
    }
    HdxPickTaskContextParams p;
    p.resolution = HdxUnitTestUtils::CalculatePickResolution(
        startPos, endPos, GfVec2i(4,4));
    p.pickTarget = pickTarget;
    p.resolveMode = resolveMode;
    p.viewMatrix = GetViewMatrix();
    p.projectionMatrix = HdxUnitTestUtils::ComputePickingProjectionMatrix(
        startPos, endPos, GfVec2i(GetWidth(), GetHeight()), GetFrustum());
    p.collection = _pickablesCol;
    p.outHits = allHits;

    HdTaskSharedPtrVector tasks;
    tasks.push_back(_renderIndex->GetTask(SdfPath("/pickTask")));
    VtValue pickParams(p);
    _engine.SetTaskContextData(HdxPickTokens->pickParams, pickParams);
    _engine.Execute(_renderIndex, &tasks);

    return HdxUnitTestUtils::TranslateHitsToSelection(
        p.pickTarget, HdSelection::HighlightModeSelect, *allHits);
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
 
    HdxPickHitVector allHits;
    const HdSelection::HighlightMode mode = HdSelection::HighlightModeSelect;
    // Use the same "marquee" style area pick with different resolve modes
    // This picks:
    //      instances 0 and 1 of /protoTop and /protoBottom
    //      cube0 and cube3
    GfVec2i pickStartPos(270, 80);
    GfVec2i pickEndPos(500, 400);

    // 1. Nearest to camera
    {
        HdSelectionSharedPtr selection = _Pick(pickStartPos, pickEndPos,
            HdxPickTokens->pickPrimsAndInstances,
            HdxPickTokens->resolveNearestToCamera,
            &allHits);
        TF_VERIFY(allHits.size() == 1);
        TF_VERIFY(selection->GetSelectedPrimPaths(mode).size() == 1);
        TF_VERIFY(selection->GetSelectedPrimPaths(mode)[0] ==
                 SdfPath("/protoTop"));
    }

    // 2. Nearest to center (of pick region)
    {
        allHits.clear();
        HdSelectionSharedPtr selection = _Pick(pickStartPos, pickEndPos,
            HdxPickTokens->pickPrimsAndInstances,
            HdxPickTokens->resolveNearestToCenter,
            &allHits);
        TF_VERIFY(allHits.size() == 1);
        TF_VERIFY(selection->GetSelectedPrimPaths(mode).size() == 1);
        TF_VERIFY(selection->GetSelectedPrimPaths(mode)[0]
                  == SdfPath("/protoBottom"));
    }

    // 3. Unique
    {
        // The pick target influences what a "unique" hit is, so cycle through
        // all the supported pickTarget, and verify that a different number of
        // hits is returned each time.
        TfTokenVector pickTargets = {
            HdxPickTokens->pickPrimsAndInstances,
            HdxPickTokens->pickFaces,
            HdxPickTokens->pickEdges,
            HdxPickTokens->pickPoints
        };
        size_t expectedHitCount[] = {
            6  /*primsAndInstances*/,
            69 /*faces*/,
            75 /*edges*/,
            41 /*points*/};
        
        for (size_t i = 0; i < pickTargets.size(); i++) {
            allHits.clear();
            HdSelectionSharedPtr selection = _Pick(pickStartPos, pickEndPos,
                pickTargets[i],
                HdxPickTokens->resolveUnique,
                &allHits);
            TF_VERIFY(allHits.size() == expectedHitCount[i]);
        }
    }

    // 4. All
    {
        allHits.clear();
        HdSelectionSharedPtr selection = _Pick(pickStartPos, pickEndPos,
            HdxPickTokens->pickPrimsAndInstances,
            HdxPickTokens->resolveAll,
            &allHits);
        TF_VERIFY(allHits.size() == 22515);
    }
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
    tasks.push_back(_renderIndex->GetTask(renderSetupTask));
    tasks.push_back(_renderIndex->GetTask(renderTask));
    tasks.push_back(_renderIndex->GetTask(selectionTask));

    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(vao);

    VtValue selTracker(_selTracker);
    _engine.SetTaskContextData(HdxTokens->selectionState, selTracker);
    _engine.Execute(&_delegate->GetRenderIndex(), &tasks);

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
    HdSt_UnitTestGLDrawing::MousePress(button, x, y, modKeys);
    _startPos = _endPos = GetMousePos();
}

void
My_TestGLDrawing::MouseRelease(int button, int x, int y, int modKeys)
{
    HdSt_UnitTestGLDrawing::MouseRelease(button, x, y, modKeys);

    if (!(modKeys & GarchGLDebugWindow::Alt)) {
        std::cout << "Pick region: (" << _startPos << ") to (" << _endPos
                  << ")" << std::endl;
        HdxPickHitVector allHits;
        HdSelectionSharedPtr selection = _Pick(_startPos, _endPos,
            HdxPickTokens->pickPrimsAndInstances,
            HdxPickTokens->resolveNearestToCenter,
            &allHits);
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

