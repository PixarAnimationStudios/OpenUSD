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

#include "pxr/imaging/hd/meshUtil.h"

#include "pxr/imaging/hdSt/mesh.h"
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
    (meshPoints)
    (pickables)
);

class Hdx_TestDriver : public HdSt_TestDriverBase<Hdx_UnitTestDelegate>
{
public:
    Hdx_TestDriver();

    void DrawWithSelection(GfVec4d const &viewport, 
        HdxSelectionTrackerSharedPtr selTracker);

    HdSelectionSharedPtr Pick(GfVec2i const &startPos, GfVec2i const &endPos,
        TfToken const& pickTarget, int width, int height, 
        GfFrustum const &frustum, GfMatrix4d const &viewMatrix);

    void SetSceneColReprSelector(HdReprSelector const &reprSelector) {
        _collection.SetReprSelector(reprSelector);
        SdfPath renderTask("/renderTask");
        GetDelegate().SetTaskParam(renderTask, HdTokens->collection,
            VtValue(_collection));
    }

    void SetPickablesColReprSelector(HdReprSelector const &reprSelector) {
        _pickablesCol.SetReprSelector(reprSelector);
        // don't need to update the picker's collection param, since it is a 
        // ptr to _pickablesCol.
    }

protected:
    using HdSt_TestDriverBase::_Init;
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
    
    Hdx_UnitTestDelegate &delegate = GetDelegate();

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
    delegate.AddRenderSetupTask(renderSetupTask);
    delegate.AddRenderTask(renderTask);
    delegate.AddSelectionTask(selectionTask);
    delegate.AddPickTask(pickTask);

    // render task parameters.
    VtValue vParam = delegate.GetTaskParam(renderSetupTask, HdTokens->params);
    HdxRenderTaskParams param = vParam.Get<HdxRenderTaskParams>();
    param.enableLighting = true; // use default lighting
    delegate.SetTaskParam(renderSetupTask, HdTokens->params, VtValue(param));

    _collection = HdRprimCollection(HdTokens->geometry, reprSelector);
    delegate.SetTaskParam(renderTask, HdTokens->collection, 
        VtValue(_collection));

    HdxSelectionTaskParams selParam;
    selParam.enableSelectionHighlight = true;
    selParam.selectionColor = GfVec4f(1, 1, 0, 1);
    selParam.locateColor = GfVec4f(1, 0, 1, 1);
    delegate.SetTaskParam(selectionTask, HdTokens->params,
                            VtValue(selParam));

    // picking
    _pickablesCol = HdRprimCollection(_tokens->pickables, 
        HdReprSelector(HdReprTokens->refined));
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
    TfToken const& pickTarget, int width, int height, 
    GfFrustum const &frustum, GfMatrix4d const &viewMatrix)
{
    HdxPickHitVector allHits;
    HdxPickTaskContextParams p;
    p.resolution = HdxUnitTestUtils::CalculatePickResolution(
        startPos, endPos, GfVec2i(4,4));
    p.pickTarget = pickTarget;
    p.resolveMode = HdxPickTokens->resolveUnique;
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
        TfToken const& pickTarget);
    using MeshEdges = std::vector<GfVec2i>;
    MeshEdges _GetMeshEdges(SdfPath const & meshPath,
                            VtIntArray const & edgeIndices) const;

private:
    std::unique_ptr<Hdx_TestDriver> _driver;

    HdxUnitTestUtils::Marquee _marquee;
    HdxSelectionTrackerSharedPtr _selTracker;

    GfVec2i _startPos, _endPos;
};

////////////////////////////////////////////////////////////

// rotate followed by translate
static GfMatrix4d
_GetTransform(GfRotation rot, GfVec3d translate)
{
    GfMatrix4d xform;
    xform.SetRotate(rot);
    xform.SetTranslateOnly(translate);

    return xform;
}

My_TestGLDrawing::MeshEdges
My_TestGLDrawing::_GetMeshEdges(SdfPath const & meshPath,
                                VtIntArray const & edgeIds) const
{
    HdRprim const * rprim = _driver->GetDelegate().GetRenderIndex().GetRprim(
        meshPath);
    HdMesh const * mesh = static_cast<HdMesh const *>(rprim);

    MeshEdges result;

    std::vector<int> edgeIndices(edgeIds.begin(), edgeIds.end());

    HdMeshEdgeIndexTable edgeIndexTable(mesh->GetTopology().get());
    edgeIndexTable.GetVerticesForEdgeIndices(edgeIndices, &result);

    return result;
}

void
My_TestGLDrawing::InitTest()
{
    _driver = std::make_unique<Hdx_TestDriver>();

    _selTracker.reset(new HdxSelectionTracker);

    // prepare scene
    _InitScene();
    SetCameraTranslate(GfVec3f(-2.3, -2.3999, -10));
    SetCameraRotate(-1, 13);
    
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

    GfRotation rot(/*axis*/GfVec3d(1,0,1), /*angle*/30);
    delegate.AddCube(SdfPath("/cube0"), _GetTransform(rot, GfVec3d(0,0,0)));
    delegate.AddCube(SdfPath("/cube1"), _GetTransform(rot, GfVec3d(5,0,0)));
    delegate.AddTet (SdfPath("/tet0"),  _GetTransform(rot, GfVec3d(0,0,5)));
    delegate.AddTet (SdfPath("/tet1"),  _GetTransform(rot, GfVec3d(5,0,5)));
}

HdSelectionSharedPtr
My_TestGLDrawing::_Pick(GfVec2i const& startPos, GfVec2i const& endPos,
    TfToken const& pickTarget)
{
    return _driver->Pick(startPos, endPos, pickTarget, GetWidth(), GetHeight(),
        GetFrustum(), GetViewMatrix());
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

    HdSelection::HighlightMode mode = HdSelection::HighlightModeSelect;
    
    //-------------------- prim & instance picking -----------------------------
    // select tet1
    {
        HdSelectionSharedPtr selection = _Pick(
            GfVec2i(436,127), GfVec2i(452,139),
            HdxPickTokens->pickPrimsAndInstances);
        _selTracker->SetSelection(selection);
        DrawScene();
        _driver->WriteToFile("color", "color10_tet1_pick_prims.png");
        HdSelection::PrimSelectionState const* selState =
            selection->GetPrimSelectionState(mode, SdfPath("/tet1"));
        TF_VERIFY(selState);
        TF_VERIFY(selState->fullySelected);
    }

    //---------------------------- face picking --------------------------------
    // select face 3 of cube0
    {
        HdSelectionSharedPtr selection = _Pick(
            GfVec2i(179,407), GfVec2i(179,407), HdxPickTokens->pickFaces);
        _selTracker->SetSelection(selection);
        DrawScene();
        _driver->WriteToFile("color", "color2_cube0_pick_face.png");
        HdSelection::PrimSelectionState const* selState =
            selection->GetPrimSelectionState(mode, SdfPath("/cube0"));
        TF_VERIFY(selState);
        TF_VERIFY(selState->elementIndices.size() == 1);
        VtIntArray const& facesSelected = selState->elementIndices[0];
        TF_VERIFY(facesSelected.size() == 1 && facesSelected[0] == 3);
    }
    
    // select faces 3 & 5 of tet1.
    // note: this isn't lasso picking. we're simply using a larger viewport.
    {
        HdSelectionSharedPtr selection = _Pick(
            GfVec2i(436,127), GfVec2i(452,139), HdxPickTokens->pickFaces);
        _selTracker->SetSelection(selection);
        DrawScene();
        _driver->WriteToFile("color", "color3_tet1_pick_faces.png");
        HdSelection::PrimSelectionState const* selState =
            selection->GetPrimSelectionState(mode, SdfPath("/tet1"));
        TF_VERIFY(selState);
        TF_VERIFY(selState->elementIndices.size() == 1);
        VtIntArray const& facesSelected = selState->elementIndices[0];
        TF_VERIFY(facesSelected.size() == 2);
    }

    // test wireframe face highlighting
    {
        _driver->SetSceneColReprSelector(HdReprSelector(HdReprTokens->wire));
        // note: don't change the pickable collection's repr; picking anywhere 
        // on the face should select it.
        HdSelectionSharedPtr selection = _Pick(
            GfVec2i(179,307), GfVec2i(179,407), HdxPickTokens->pickFaces);
        _selTracker->SetSelection(selection);
        DrawScene();
        _driver->WriteToFile("color", "color9_cube0_wire_pick_face.png");
        HdSelection::PrimSelectionState const* selState =
            selection->GetPrimSelectionState(mode, SdfPath("/cube0"));
        TF_VERIFY(selState);
        TF_VERIFY(selState->elementIndices.size() == 1);
        VtIntArray const& facesSelected = selState->elementIndices[0];
        TF_VERIFY(facesSelected.size() == 2);
    }
    //---------------------------- edge picking --------------------------------
    // Picking or highlighting edges requires the GS stage, so use a repr that
    // guarantees the GS is bound (wire* does)
    // We change the repr on the scene collection in addition to the picking
    // collection to validate selection highlighting.
    // Worth noting that for picking (i.e, in the id render pass), while
    // HdxIntersector could override the repr, we leave it to the application
    // to do it instead.
    _driver->SetSceneColReprSelector(HdReprSelector(HdReprTokens->wireOnSurf));
    _driver->SetPickablesColReprSelector(HdReprSelector(
        HdReprTokens->wireOnSurf));

    // select edge of tet0
    {
        HdSelectionSharedPtr selection = _Pick(
            GfVec2i(158,122), GfVec2i(158,122), HdxPickTokens->pickEdges);
        _selTracker->SetSelection(selection);
        DrawScene();
        _driver->WriteToFile("color", "color4_tet0_pick_edge.png");
        SdfPath meshPath("/tet0");
        HdSelection::PrimSelectionState const* selState =
            selection->GetPrimSelectionState(mode, meshPath);
        TF_VERIFY(selState);
        TF_VERIFY(selState->edgeIndices.size() == 1);
        VtIntArray const& edgeIndicesSelected = selState->edgeIndices[0];
        MeshEdges edgesSelected = _GetMeshEdges(meshPath, edgeIndicesSelected);
        TF_VERIFY(edgesSelected.size() == 1);
    }

    // select edges of cube1
    // note: this isn't lasso picking. we're simply using a larger viewport.
    {
        HdSelectionSharedPtr selection = _Pick(
            GfVec2i(446,335), GfVec2i(462,427), HdxPickTokens->pickEdges);
        _selTracker->SetSelection(selection);
        DrawScene();
        _driver->WriteToFile("color", "color5_cube1_pick_edges.png");
        SdfPath meshPath("/cube1");
        HdSelection::PrimSelectionState const* selState =
            selection->GetPrimSelectionState(mode, meshPath);
        TF_VERIFY(selState);
        TF_VERIFY(selState->edgeIndices.size() == 1);
        VtIntArray const& edgeIndicesSelected = selState->edgeIndices[0];
        MeshEdges edgesSelected = _GetMeshEdges(meshPath, edgeIndicesSelected);
        TF_VERIFY(edgesSelected.size() == 2);
    }

    //---------------------------- point picking -------------------------------
    // Similar to edges, we currently support picking and selection
    // highlighting points on prims only when points are rendered.
    _driver->SetSceneColReprSelector(HdReprSelector(HdReprTokens->wireOnSurf,
                                                    HdReprTokens->disabled,
                                                    _tokens->meshPoints));
    _driver->SetPickablesColReprSelector(HdReprSelector(
        HdReprTokens->wireOnSurf,
        HdReprTokens->disabled,
        _tokens->meshPoints));

    // select points of cube1
    {
        HdSelectionSharedPtr selection = _Pick(
            GfVec2i(346,215), GfVec2i(492,427), HdxPickTokens->pickPoints);
        _selTracker->SetSelection(selection);
        DrawScene();
        _driver->WriteToFile("color", "color6_cube1_pick_points.png");
        HdSelection::PrimSelectionState const* selState =
            selection->GetPrimSelectionState(mode, SdfPath("/cube1"));
        TF_VERIFY(selState);
        TF_VERIFY(selState->pointIndices.size() == 1);
        VtIntArray const& pointsSelected = selState->pointIndices[0];
        TF_VERIFY(pointsSelected.size() == 4);
    }

    {
        // Simulate "pick through" semantics by using wireframe for the picking
        // collection. The scene collection remains as-is (wireOnSurf).
        _driver->SetPickablesColReprSelector(HdReprSelector(
            HdReprTokens->wire,
            HdReprTokens->disabled,
            _tokens->meshPoints));
        HdSelectionSharedPtr selection = _Pick(
            GfVec2i(346,215), GfVec2i(492,427), HdxPickTokens->pickPoints);
        _selTracker->SetSelection(selection);
        DrawScene();
        _driver->WriteToFile("color", "color7_cube1_pick_points_pick_through.png");
        HdSelection::PrimSelectionState const* selState =
            selection->GetPrimSelectionState(mode, SdfPath("/cube1"));
        TF_VERIFY(selState);
        TF_VERIFY(selState->pointIndices.size() == 1);
        VtIntArray const& pointsSelected = selState->pointIndices[0];
        TF_VERIFY(pointsSelected.size() == 5);
    }

    // manually verify if specifying a color for a set of points works.
    {
        // Render just the points.
        _driver->SetSceneColReprSelector(HdReprSelector(HdReprTokens->disabled,
                                                        HdReprTokens->disabled,
                                                        _tokens->meshPoints));
        // The pick below is only to get a handle to the selection.
        HdSelectionSharedPtr selection = _Pick(
            GfVec2i(0,0), GfVec2i(0,1), HdxPickTokens->pickPoints);
        std::vector<int> indices = {0,2,3,4};
        VtIntArray pointIndices(indices.size());
        pointIndices.assign(indices.begin(), indices.end());
        selection->AddPoints(HdSelection::HighlightModeSelect,
                             SdfPath("/cube0"),
                             pointIndices,
                             GfVec4f(1.0, 0.0, 0.0, 1.0));

        selection->AddPoints(HdSelection::HighlightModeSelect,
                             SdfPath("/tet1"),
                             pointIndices,
                             GfVec4f(1.0, 1.0, 0.0, 1.0));
        _selTracker->SetSelection(selection);
        DrawScene();
        _driver->WriteToFile("color", "color8_points_with_color.png");
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
        // update pick params for any camera changes
        HdSelectionSharedPtr selection = _Pick(_startPos, _endPos,
            HdxPickTokens->pickFaces);
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
