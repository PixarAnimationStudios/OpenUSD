//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/imaging/hd/material.h"

#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/imaging/hdx/selectionTask.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/unitTestDelegate.h"
#include "pxr/imaging/hdx/unitTestUtils.h"

#include "pxr/imaging/hio/glslfx.h"

#include "pxr/usd/sdr/registry.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>
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

    HdSelectionSharedPtr Pick(GfVec2i const &startPos, GfVec2i const &endPos,
        TfToken const& pickTarget, int width, int height,
        GfFrustum const &frustum, GfMatrix4d const &viewMatrix);

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

    // HdSt_UnitTestGLDrawing overrides
    void InitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;
    void Present(uint32_t framebuffer) override;

protected:
    void _InitScene();
    HdSelectionSharedPtr _Pick(
        GfVec2i const& startPos, GfVec2i const& endPos,
        TfToken const& pickTarget);

private:
    std::unique_ptr<Hdx_TestDriver> _driver;

    HdxSelectionTrackerSharedPtr _selTracker;
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

// Builds an inline glslfx surface shader tagged with the given materialTag,
// registering its technique under a tag-specific name so that shaders with
// different tags don't collide in the Sdr registry.
static SdrShaderNodeConstPtr
_GetSurfaceShaderNode(std::string const &materialTag)
{
    std::string const techniqueName =
        "testHdxPickTransparentOccluder.Surface." + materialTag;
    std::string const source =
        "-- glslfx version 0.1 \n"
        "-- configuration \n"
        "{\n"
            "\"metadata\": {\n"
            "    \"materialTag\": \"" + materialTag + "\"\n"
            "},\n"
            "\"techniques\": {\n"
            "    \"default\": {\n"
            "        \"surfaceShader\": {\n"
            "            \"source\": [ \"" + techniqueName + "\" ]\n"
            "        }\n"
            "    }\n"
            "}\n\n"
        "}\n"

        "-- glsl " + techniqueName + " \n\n"

        "vec4 surfaceShader(vec4 Peye, vec3 Neye, vec4 color, vec4 patchCoord) {\n"
        "    return vec4(FallbackLighting(Peye.xyz, Neye, color.rgb), color.a);\n"
        "}\n";

    SdrRegistry &shaderReg = SdrRegistry::GetInstance();
    return shaderReg.GetShaderNodeFromSourceCode(
        source,
        HioGlslfxTokens->glslfx,
        SdrTokenMap()); // metadata
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

    _driver->SetClearColor(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
    _driver->SetClearDepth(1.0f);
    _driver->SetupAovs(GetWidth(), GetHeight());
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
}

void
My_TestGLDrawing::OffscreenTest()
{
    DrawScene();

    HdSelection::HighlightMode mode = HdSelection::HighlightModeSelect;

    // Picks /cube0 at a pixel where its face 3 is visible, and verifies that
    // face 3 is the only face selected on /cube0.
    auto verifyCube0Face3 = [&mode](HdSelectionSharedPtr const &selection) {
        HdSelection::PrimSelectionState const* selState =
            selection->GetPrimSelectionState(mode, SdfPath("/cube0"));
        TF_VERIFY(selState);
        if (!selState) {
            return;
        }
        TF_VERIFY(selState->elementIndices.size() == 1);
        if (selState->elementIndices.empty()) {
            return;
        }
        VtIntArray const& facesSelected = selState->elementIndices[0];
        TF_VERIFY(facesSelected.size() == 1 && facesSelected[0] == 3);
    };

    // Without any occluder, the pick lands on face 3 of /cube0. The cases
    // below repeat this pick with a shell enclosing /cube0.
    {
        HdSelectionSharedPtr selection = _Pick(
            GfVec2i(179,407), GfVec2i(179,407), HdxPickTokens->pickFaces);
        verifyCube0Face3(selection);
    }

    //------------------- transparent occluder face picking --------------------
    // PRES-102831: a fully transparent prim tagged 'translucent' must not
    // steal a face pick from the (visible) geometry it happens to enclose.
    // A zero-displayOpacity prim with no material would instead get the
    // 'masked' material tag and already be discarded today; binding an
    // explicit translucent-tagged material is what reproduces the bug.
    {
        Hdx_UnitTestDelegate &delegate = _driver->GetDelegate();

        // Adds a material whose surface shader carries the given materialTag.
        auto addMaterial = [&delegate](SdfPath const &materialId,
                                       std::string const &materialTag) {
            // This needs SdrGlslfxParserPlugin to have been loaded.
            SdrShaderNodeConstPtr node = _GetSurfaceShaderNode(materialTag);
            if (!TF_VERIFY(node)) {
                return;
            }

            HdMaterialNetworkMap material;
            HdMaterialNetwork& network =
                material.map[HdMaterialTerminalTokens->surface];
            HdMaterialNode terminal;
            terminal.path = materialId.AppendPath(SdfPath("Shader"));
            terminal.identifier = node->GetIdentifier();
            material.terminals.push_back(terminal.path);
            network.nodes.push_back(std::move(terminal)); // must be last
            delegate.AddMaterialResource(materialId, VtValue(material));
        };

        // Encloses /cube0 with a larger copy at the same transform, so the
        // shell wins the depth test at every pixel covering /cube0. Each
        // case below removes its shell afterwards, so the cases differ only
        // in opacity and material tag.
        GfRotation rot(/*axis*/GfVec3d(1,0,1), /*angle*/30);
        GfMatrix4d shellXform =
            GfMatrix4d(1.0).SetScale(1.5) * _GetTransform(rot, GfVec3d(0,0,0));
        auto addShell = [&delegate, &shellXform](SdfPath const &shellId,
                                                 float opacity,
                                                 SdfPath const &materialId) {
            delegate.AddCube(shellId, shellXform, /*guide=*/false,
                              /*instancerId=*/SdfPath(),
                              PxOsdOpenSubdivTokens->catmullClark,
                              /*color=*/VtValue(GfVec3f(1,1,1)),
                              HdInterpolationConstant,
                              /*opacity=*/VtValue(opacity),
                              HdInterpolationConstant);
            delegate.BindMaterial(shellId, materialId);
        };

        SdfPath translucentMaterialId("/translucentMaterial");
        addMaterial(translucentMaterialId, "translucent");
        SdfPath additiveMaterialId("/additiveMaterial");
        addMaterial(additiveMaterialId, "additive");

        // Same pick as the unoccluded case above: the shell must not be
        // picked, and /cube0 face 3 must still be selected.
        SdfPath shellId("/transparentShell");
        addShell(shellId, 0.0f, translucentMaterialId);
        HdSelectionSharedPtr selection = _Pick(
            GfVec2i(179,407), GfVec2i(179,407), HdxPickTokens->pickFaces);
        TF_VERIFY(!selection->GetPrimSelectionState(mode, shellId));
        verifyCube0Face3(selection);
        delegate.Remove(shellId);

        // Semi-transparent-but-visible translucent geometry, with opacity
        // well above alphaThreshold, must stay pickable.
        SdfPath semiShellId("/semiTransparentShell");
        addShell(semiShellId, 0.5f, translucentMaterialId);
        selection = _Pick(
            GfVec2i(179,407), GfVec2i(179,407), HdxPickTokens->pickFaces);
        TF_VERIFY(selection->GetPrimSelectionState(mode, semiShellId));
        delegate.Remove(semiShellId);

        // The pick alpha test applies only to 'translucent'-tagged geometry.
        // Additive is emissive with alpha == 0 by design yet fully visible,
        // so a zero-opacity additive shell must still be picked. This fails
        // if the gate is ever widened beyond 'translucent'.
        SdfPath additiveShellId("/additiveShell");
        addShell(additiveShellId, 0.0f, additiveMaterialId);
        selection = _Pick(
            GfVec2i(179,407), GfVec2i(179,407), HdxPickTokens->pickFaces);
        TF_VERIFY(selection->GetPrimSelectionState(mode, additiveShellId));
        delegate.Remove(additiveShellId);
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
