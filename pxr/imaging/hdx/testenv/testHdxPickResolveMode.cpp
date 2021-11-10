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

#include "pxr/imaging/garch/glDebugWindow.h"

#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/selection.h"

#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/imaging/hdx/pickTask.h"
#include "pxr/imaging/hdx/selectionTask.h"
#include "pxr/imaging/hdx/selectionTracker.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/unitTestDelegate.h"
#include "pxr/imaging/hdx/unitTestUtils.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>
#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (meshPoints)
    (pickables)
);

class Hdx_TestDriver : public HdSt_TestDriverBase<Hdx_UnitTestDelegate>
{
public:
    Hdx_TestDriver(TfToken const &reprName);

    void DrawWithSelection(GfVec4d const &viewport, 
        HdxSelectionTrackerSharedPtr selTracker);

    HdSelectionSharedPtr Pick(GfVec2i const &startPos, GfVec2i const &endPos,
        int width, int height, GfFrustum const &frustum, 
        GfMatrix4d const &viewMatrix, TfToken const &pickTarget,
        TfToken const &resolveMode, HdxPickHitVector *allHits);

protected:
    void _Init(HdReprSelector const &reprSelector) override;

private:
    HdRprimCollection _pickablesCol;

};

Hdx_TestDriver::Hdx_TestDriver(TfToken const &reprName)
{
    _Init(HdReprSelector(reprName));
}

void
Hdx_TestDriver::_Init(HdReprSelector const &reprSelector)
{   
    _SetupSceneDelegate();
    
    Hdx_UnitTestDelegate &delegate = GetDelegate();

    // prepare render task
    SdfPath renderSetupTask("/renderSetupTask");
    SdfPath renderTask("/renderTask");
    SdfPath selectionTask("/selectionTask");
    SdfPath pickTask("/pickTask");
    delegate.AddRenderSetupTask(renderSetupTask);
    delegate.AddRenderTask(renderTask);
    delegate.AddSelectionTask(selectionTask);
    delegate.AddPickTask(pickTask);

    // Add a meshPoints repr since it isn't populated in 
    // HdRenderIndex::_ConfigureReprs
    HdMesh::ConfigureRepr(_tokens->meshPoints,
                          HdMeshReprDesc(HdMeshGeomStylePoints,
                                         HdCullStyleNothing,
                                         HdMeshReprDescTokens->pointColor,
                                         /*flatShadingEnabled=*/true,
                                         /*blendWireframeColor=*/false));

    // Use wireframe and enable points for edge and point picking.
    const auto sceneReprSel = HdReprSelector(HdReprTokens->wireOnSurf,
                                             HdReprTokens->disabled,
                                             _tokens->meshPoints);

    // render task parameters.
    VtValue vParam = delegate.GetTaskParam(renderSetupTask, HdTokens->params);
    HdxRenderTaskParams param = vParam.Get<HdxRenderTaskParams>();
    param.enableLighting = true; // use default lighting
    delegate.SetTaskParam(renderSetupTask, HdTokens->params, VtValue(param));
    delegate.SetTaskParam(renderTask, HdTokens->collection,
        VtValue(HdRprimCollection(HdTokens->geometry, sceneReprSel)));
        
    HdxSelectionTaskParams selParam;
    selParam.enableSelection = true;
    selParam.selectionColor = GfVec4f(1, 1, 0, 1);
    selParam.locateColor = GfVec4f(1, 0, 1, 1);
    delegate.SetTaskParam(
        selectionTask, HdTokens->params, VtValue(selParam));

    // picking
    _pickablesCol = HdRprimCollection(_tokens->pickables, sceneReprSel);
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
    GfMatrix4d const &viewMatrix, TfToken const &pickTarget,
    TfToken const &resolveMode, HdxPickHitVector *allHits)
{
    HdxPickTaskContextParams p;
    p.resolution = HdxUnitTestUtils::CalculatePickResolution(
        startPos, endPos, GfVec2i(4,4));
    p.pickTarget = pickTarget;
    p.resolveMode = resolveMode;
    p.viewMatrix = viewMatrix;
    p.projectionMatrix = HdxUnitTestUtils::ComputePickingProjectionMatrix(
        startPos, endPos, GfVec2i(width, height), frustum);
    p.collection = _pickablesCol;
    p.outHits = allHits;

    HdTaskSharedPtrVector tasks;
    tasks.push_back(GetDelegate().GetRenderIndex().GetTask(
        SdfPath("/pickTask")));
    VtValue pickParams(p);
    _GetEngine()->SetTaskContextData(HdxPickTokens->pickParams, pickParams);
    _GetEngine()->Execute(&GetDelegate().GetRenderIndex(), &tasks);

    return HdxUnitTestUtils::TranslateHitsToSelection(
        p.pickTarget, HdSelection::HighlightModeSelect, *allHits);
}

// --------------------------------------------------------------------------

class My_TestGLDrawing : public HdSt_UnitTestGLDrawing
{
public:
    My_TestGLDrawing() 
    {
        SetCameraRotate(0, 0);
        SetCameraTranslate(GfVec3f(0));
        _reprName = HdReprTokens->wireOnSurf;
        _refineLevel = 0;
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
    void ParseArgs(int argc, char *argv[]) override;
    void _InitScene();
    HdSelectionSharedPtr _Pick(
        GfVec2i const& startPos, GfVec2i const& endPos,
        TfToken const& pickTarget,
        TfToken const& resolveMode,
        HdxPickHitVector *allHits);

private:
    std::unique_ptr<Hdx_TestDriver> _driver;

    HdxUnitTestUtils::Marquee _marquee;
    HdxSelectionTrackerSharedPtr _selTracker;

    TfToken _reprName;
    int _refineLevel;
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
    _driver = std::make_unique<Hdx_TestDriver>(_reprName);
    
    Hdx_UnitTestDelegate &delegate = _driver->GetDelegate();

    delegate.SetRefineLevel(_refineLevel);
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

    delegate.AddCube(SdfPath("/cube0"), _GetTranslate( 5, 0, 5));
    delegate.AddCube(SdfPath("/cube1"), _GetTranslate(-5, 0, 5));
    delegate.AddCube(SdfPath("/cube2"), _GetTranslate(-5, 0,-5));
    delegate.AddCube(SdfPath("/cube3"), _GetTranslate( 5, 0,-5));

    {
        delegate.AddInstancer(SdfPath("/instancerTop"));
        delegate.AddCube(SdfPath("/protoTop"),
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

        delegate.SetInstancerProperties(SdfPath("/instancerTop"),
                                        prototypeIndex,
                                        scale, rotate, translate);
    }

    {
        delegate.AddInstancer(SdfPath("/instancerBottom"));
        delegate.AddTet(SdfPath("/protoBottom"),
                         GfMatrix4d(1), false, SdfPath("/instancerBottom"));
        delegate.SetRefineLevel(SdfPath("/protoBottom"), 2);

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

        delegate.SetInstancerProperties(SdfPath("/instancerBottom"),
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

    return _driver->Pick(startPos, endPos, GetWidth(), GetHeight(),
        GetFrustum(), GetViewMatrix(), pickTarget, resolveMode, allHits);
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
           135 /*edges*/,
            41 /*points*/};
        
        for (size_t i = 0; i < pickTargets.size(); i++) {
            allHits.clear();
            HdSelectionSharedPtr selection = _Pick(pickStartPos, pickEndPos,
                pickTargets[i],
                HdxPickTokens->resolveUnique,
                &allHits);
            std::cout << "allHits: " << allHits.size()
                      << " expectedHitCount:  " << expectedHitCount[i]
                      << std::endl;
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

