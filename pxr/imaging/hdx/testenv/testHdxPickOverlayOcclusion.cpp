//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/imaging/garch/glDebugWindow.h"

#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/selection.h"

#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/imaging/hdx/pickTask.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/unitTestDelegate.h"
#include "pxr/imaging/hdx/unitTestUtils.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>
#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

// Regression test for PRES-98035: a joint guide (an "overlay" prim) rendered
// behind a point used to destroy that point's pickability, because the pick
// task's overlay pass wrote its ID buffers directly on top of the pickable
// pass's, overwriting a valid point hit with an invalid surface fragment.
//
// This exercises HdxPickTask directly (no scene-level material assignment)
// by using Hdx_UnitTestDelegate::SetDisplayInOverlay to mark a prim as an
// overlay item, and verifies the merge of the overlay pass's ID buffers into
// the pickable pass's preserves both: the bug fix (an overlay item behind a
// point must not block picking that point) and the invariants the fix must
// not disturb (an overlay item in front still always wins, and
// pickPrimsAndInstances/pickFaces results are unaffected either way).

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (meshPoints)
    (pickables)
);

class Hdx_TestDriver : public HdSt_TestDriverBase<Hdx_UnitTestDelegate>
{
public:
    Hdx_TestDriver();

    HdSelectionSharedPtr Pick(
        GfVec2i const &startPos, GfVec2i const &endPos,
        int width, int height, GfFrustum const &frustum,
        GfMatrix4d const &viewMatrix, TfToken const &pickTarget,
        HdxPickHitVector *allHits);

protected:
    void _Init(HdReprSelector const &reprSelector) override;

private:
    HdRprimCollection _pickablesCol;
};

Hdx_TestDriver::Hdx_TestDriver()
{
    _Init(HdReprSelector(HdReprTokens->wireOnSurf,
                         HdReprTokens->disabled,
                         _tokens->meshPoints));
}

void
Hdx_TestDriver::_Init(HdReprSelector const &reprSelector)
{
    _SetupSceneDelegate();

    Hdx_UnitTestDelegate &delegate = GetDelegate();

    SdfPath renderSetupTask("/renderSetupTask");
    SdfPath renderTask("/renderTask");
    SdfPath pickTask("/pickTask");
    delegate.AddRenderSetupTask(renderSetupTask);
    delegate.AddRenderTask(renderTask);
    delegate.AddPickTask(pickTask);

    // Add a meshPoints repr since it isn't populated in
    // HdRenderIndex::_ConfigureReprs
    HdMesh::ConfigureRepr(_tokens->meshPoints,
                          HdMeshReprDesc(HdMeshGeomStylePoints,
                                         HdCullStyleNothing,
                                         HdMeshReprDescTokens->pointColor,
                                         /*flatShadingEnabled=*/true,
                                         /*blendWireframeColor=*/false));

    VtValue vParam = delegate.GetTaskParam(renderSetupTask, HdTokens->params);
    HdxRenderTaskParams param = vParam.Get<HdxRenderTaskParams>();
    param.enableLighting = true;
    delegate.SetTaskParam(renderSetupTask, HdTokens->params, VtValue(param));
    delegate.SetTaskParam(renderTask, HdTokens->collection,
        VtValue(HdRprimCollection(HdTokens->geometry, reprSelector)));

    _pickablesCol = HdRprimCollection(_tokens->pickables, reprSelector);
    // We have to unfortunately explicitly add collections besides 'geometry'.
    // See HdRenderIndex constructor.
    delegate.GetRenderIndex().GetChangeTracker().AddCollection(
        _tokens->pickables);
}

HdSelectionSharedPtr
Hdx_TestDriver::Pick(
    GfVec2i const &startPos, GfVec2i const &endPos,
    int width, int height, GfFrustum const &frustum,
    GfMatrix4d const &viewMatrix, TfToken const &pickTarget,
    HdxPickHitVector *allHits)
{
    HdxPickTaskContextParams p;
    p.resolution = HdxUnitTestUtils::CalculatePickResolution(
        startPos, endPos, GfVec2i(4,4));
    p.pickTarget = pickTarget;
    p.resolveMode = HdxPickTokens->resolveUnique;
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
    }

    // HdSt_UnitTestGLDrawing overrides
    void InitTest() override;
    void UninitTest() override;
    void DrawTest() override {}
    void OffscreenTest() override;

protected:
    void _InitScene();
    HdSelectionSharedPtr _Pick(
        GfVec2i const& startPos, GfVec2i const& endPos,
        TfToken const& pickTarget,
        HdxPickHitVector *allHits);

private:
    std::unique_ptr<Hdx_TestDriver> _driver;
};

void
My_TestGLDrawing::InitTest()
{
    _driver = std::make_unique<Hdx_TestDriver>();

    _InitScene();
    SetCameraTranslate(GfVec3f(0, 0, -20));

    _driver->SetClearColor(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
    _driver->SetClearDepth(1.0f);
    _driver->SetupAovs(GetWidth(), GetHeight());
}

void
My_TestGLDrawing::UninitTest()
{
}

void
My_TestGLDrawing::_InitScene()
{
    Hdx_UnitTestDelegate &delegate = _driver->GetDelegate();

    // /frontMesh is a plain pickable cube, directly facing the camera.
    delegate.AddCube(SdfPath("/frontMesh"), GfMatrix4d(1.0));

    // /overlayCube is the same cube, marked as an overlay prim (e.g. a joint
    // guide). Its z-translation is varied per-assertion below to place it
    // either behind or in front of /frontMesh.
    delegate.AddCube(SdfPath("/overlayCube"), GfMatrix4d(1.0));
    delegate.SetDisplayInOverlay(SdfPath("/overlayCube"), true);
}

HdSelectionSharedPtr
My_TestGLDrawing::_Pick(GfVec2i const& startPos, GfVec2i const& endPos,
                        TfToken const& pickTarget,
                        HdxPickHitVector *allHits)
{
    return _driver->Pick(startPos, endPos, GetWidth(), GetHeight(),
        GetFrustum(), GetViewMatrix(), pickTarget, allHits);
}

void
My_TestGLDrawing::OffscreenTest()
{
    Hdx_UnitTestDelegate &delegate = _driver->GetDelegate();
    const HdSelection::HighlightMode mode = HdSelection::HighlightModeSelect;

    // Pick the whole viewport; both cubes are centered on the camera and
    // overlap fully in screen space.
    GfVec2i pickStartPos(0, 0);
    GfVec2i pickEndPos(GetWidth(), GetHeight());

    // 1. Overlay behind, point target: the fix. Before the fix, the overlay
    //    cube's opaque surface fragments overwrite pointId at every pixel
    //    where /frontMesh's points would otherwise have been hit, and
    //    _IsValidHit rejects those overwritten (surface) fragments under a
    //    point target, so no points are selected at all. After the fix,
    //    /frontMesh's points are retained and selected. Only the 4
    //    camera-facing corners of the cube are selected -- the other 4 are
    //    self-occluded by the cube's own front faces, matching the reference
    //    behavior for cube point-picking in testHdxPickTarget.cpp.
    {
        delegate.UpdateTransform(SdfPath("/overlayCube"),
                                  GfMatrix4f(1.0).SetTranslate(
                                      GfVec3f(0, 5, 0)));

        HdxPickHitVector allHits;
        HdSelectionSharedPtr selection = _Pick(
            pickStartPos, pickEndPos, HdxPickTokens->pickPoints, &allHits);
        HdSelection::PrimSelectionState const* selState =
            selection->GetPrimSelectionState(mode, SdfPath("/frontMesh"));
        TF_VERIFY(selState);
        if (selState) {
            TF_VERIFY(selState->pointIndices.size() == 1);
            if (selState->pointIndices.size() == 1) {
                TF_VERIFY(selState->pointIndices[0].size() == 4);
            }
        }
    }

    // 2. Overlay in front, point target: the buried-joint-guide workflow.
    //    An overlay item that is itself a valid hit must still win
    //    unconditionally, exactly as it did before the fix.
    {
        delegate.UpdateTransform(SdfPath("/overlayCube"),
                                  GfMatrix4f(1.0).SetTranslate(
                                      GfVec3f(0, -5, 0)));

        HdxPickHitVector allHits;
        HdSelectionSharedPtr selection = _Pick(
            pickStartPos, pickEndPos, HdxPickTokens->pickPoints, &allHits);
        HdSelection::PrimSelectionState const* selState =
            selection->GetPrimSelectionState(mode, SdfPath("/overlayCube"));
        TF_VERIFY(selState);
        TF_VERIFY(!selection->GetPrimSelectionState(
            mode, SdfPath("/frontMesh")));
    }

    // Scaled up in x/z (the screen-plane axes) and pushed behind /frontMesh
    // along the depth axis, large enough that its on-screen silhouette fills
    // the entire viewport regardless of the exact camera parameters -- unlike
    // assertion 1/2 above, assertions 3 and 4 need every pixel of
    // /frontMesh's footprint to also be a valid overlayCube hit. The depth
    // axis itself is left unscaled so the whole prim stays in front of the
    // near clip plane.
    // pickPrimsAndInstances/pickFaces don't consider depth ordering (any
    // fragment with a primId is a valid hit), so which prim is behind which
    // is not relevant here; only full coverage matters.
    GfMatrix4f const overlayBehindFullCoverage =
        GfMatrix4f(1.0).SetScale(GfVec3f(50.0f, 1.0f, 50.0f)) *
        GfMatrix4f(1.0).SetTranslate(GfVec3f(0, 5, 0));

    // 3. pickPrimsAndInstances, overlay behind: bit-identity guard. For this
    //    pick target, _IsValidHit accepts any fragment with a primId, so the
    //    overlay pass's fragments were always valid hits and the merge must
    //    select exactly what today's overwrite selects: the overlay prim,
    //    not the front mesh.
    {
        delegate.UpdateTransform(SdfPath("/overlayCube"),
                                  overlayBehindFullCoverage);

        HdxPickHitVector allHits;
        HdSelectionSharedPtr selection = _Pick(
            pickStartPos, pickEndPos,
            HdxPickTokens->pickPrimsAndInstances, &allHits);
        TF_VERIFY(selection->GetPrimSelectionState(
            mode, SdfPath("/overlayCube")) != nullptr);
        TF_VERIFY(selection->GetPrimSelectionState(
            mode, SdfPath("/frontMesh")) == nullptr);
    }

    // 4. pickFaces, overlay behind: same expectation as #3, guarding the
    //    other pick target the merge must not perturb.
    {
        delegate.UpdateTransform(SdfPath("/overlayCube"),
                                  overlayBehindFullCoverage);

        HdxPickHitVector allHits;
        HdSelectionSharedPtr selection = _Pick(
            pickStartPos, pickEndPos, HdxPickTokens->pickFaces, &allHits);
        TF_VERIFY(selection->GetPrimSelectionState(
            mode, SdfPath("/overlayCube")) != nullptr);
        TF_VERIFY(selection->GetPrimSelectionState(
            mode, SdfPath("/frontMesh")) == nullptr);
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
