//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hdSt/mesh.h"

#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/vt/array.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static const VtVec3fArray kBoxPoints = {
    GfVec3f(-2, -2, 2),
    GfVec3f(2, -2, 2),
    GfVec3f(-2, 2, 2),
    GfVec3f(2, 2, 2),
    GfVec3f(-2, 2, -2),
    GfVec3f(2, 2, -2),
    GfVec3f(-2, -2, -2),
    GfVec3f(2, -2, -2),
};
static const VtIntArray kBoxNumVerts = {4, 4, 4, 4, 4, 4};
static const VtIntArray kBoxVerts = 
    {0, 1, 3, 2, 4, 5, 7, 6, 2, 3, 5, 4, 6, 7, 1, 0, 1, 7, 5, 3, 6, 0, 2, 4};

// One outward-facing normal per face
static const VtVec3fArray kFaceNormals = {
    GfVec3f(0, 0, 1),
    GfVec3f(0, 0, -1),
    GfVec3f(0, 1, 0),
    GfVec3f(0, -1, 0),
    GfVec3f(1, 0, 0),
    GfVec3f(-1, 0, 0),
};

// Same normals in face-varying
static const VtVec3fArray kFaceVaryingNormals = {
    GfVec3f(0, 0, 1),
    GfVec3f(0, 0, 1),
    GfVec3f(0, 0, 1),
    GfVec3f(0, 0, 1),
    GfVec3f(0, 0, -1),
    GfVec3f(0, 0, -1),
    GfVec3f(0, 0, -1),
    GfVec3f(0, 0, -1),
    GfVec3f(0, 1, 0),
    GfVec3f(0, 1, 0),
    GfVec3f(0, 1, 0),
    GfVec3f(0, 1, 0),
    GfVec3f(0, -1, 0),
    GfVec3f(0, -1, 0),
    GfVec3f(0, -1, 0),
    GfVec3f(0, -1, 0),
    GfVec3f(1, 0, 0),
    GfVec3f(1, 0, 0),
    GfVec3f(1, 0, 0),
    GfVec3f(1, 0, 0),
    GfVec3f(-1, 0, 0),
    GfVec3f(-1, 0, 0),
    GfVec3f(-1, 0, 0),
    GfVec3f(-1, 0, 0),
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

    void RunPackedNormalsTests(int argc, char* argv[]);

private:
    HdSt_TestDriverUniquePtr _driver;
    HdSt_TestLightingShaderSharedPtr _lightingShader;
    std::string _outputFilePath;

    void AddMesh(SdfPath const& id, bool packed,
        HdInterpolation interp = HdInterpolationUniform);
};

////////////////////////////////////////////////////////////

void
My_TestGLDrawing::InitTest()
{
    std::cout << "My_TestGLDrawing::InitTest()" << std::endl;

    _driver = std::make_unique<HdSt_TestDriver>(HdReprTokens->smoothHull);
    HdUnitTestDelegate& delegate = _driver->GetDelegate();

    _lightingShader.reset(
        new HdSt_TestLightingShader(&delegate.GetRenderIndex()));
    _lightingShader->SetLight(
        0, GfVec3f(0.5f, 1.0f, 0.5f), GfVec3f(0.5f, 0.5f, 0.8f));
    _lightingShader->SetSceneAmbient(GfVec3f(0.1f, 0.1f, 0.1f));
    _lightingShader->Prepare();
    _driver->GetRenderPassState()->SetLightingShader(_lightingShader);

    _driver->SetClearColor(GfVec4f(0.f, 0.f, 0.f, 1.0f));
    _driver->SetClearDepth(1.0f);
    _driver->SetupAovs(GetWidth(), GetHeight());
}

void
My_TestGLDrawing::DrawTest()
{
    int width = GetWidth(), height = GetHeight();
    _driver->SetCullStyle(HdCullStyleNothing);
    _driver->SetCamera(GetViewMatrix(), GetProjectionMatrix(),
        CameraUtilFraming(GfRect2i(GfVec2i(0, 0), width, height)));
    _driver->UpdateAovDimensions(width, height);
    _driver->Draw();
}

void
My_TestGLDrawing::OffscreenTest()
{
    DrawTest();
    if (!_outputFilePath.empty()) {
        _driver->WriteToFile("color", _outputFilePath);
    }
}

void
My_TestGLDrawing::Present(uint32_t framebuffer)
{
    _driver->Present(GetWidth(), GetHeight(), framebuffer);
}

void
My_TestGLDrawing::AddMesh(
    SdfPath const& id, bool packed, HdInterpolation interp)
{
    HdUnitTestDelegate& delegate = _driver->GetDelegate();
    const VtIntArray holes;
    delegate.AddMesh(id, GfMatrix4f(1.f), kBoxPoints, kBoxNumVerts, kBoxVerts,
        holes, PxOsdSubdivTags(), VtValue(GfVec3f(1, 1, 1)),
        HdInterpolationConstant, VtValue(1.f), HdInterpolationConstant,
        /*guide*/ false, SdfPath(), PxOsdOpenSubdivTokens->none);

    const VtVec3fArray& floatNormals = (interp == HdInterpolationFaceVarying) ?
        kFaceVaryingNormals :
        kFaceNormals;
    if (packed) {
        VtArray<HdVec4f_2_10_10_10_REV> normals;
        normals.reserve(floatNormals.size());
        for (const GfVec3f& n : floatNormals) {
            normals.push_back(HdVec4f_2_10_10_10_REV(n));
        }
        delegate.AddPrimvar(id, HdTokens->normals, VtValue(normals), interp,
            HdPrimvarRoleTokens->normal);
    } else {
        delegate.AddPrimvar(id, HdTokens->normals, VtValue(floatNormals),
            interp, HdPrimvarRoleTokens->normal);
    }
}

void
My_TestGLDrawing::RunPackedNormalsTests(int argc, char* argv[])
{
    RunTest(argc, argv);

    HdUnitTestDelegate& delegate = _driver->GetDelegate();

    // --- Test 1: Single packed-normals mesh (uniform) ---
    const SdfPath packedMesh("/mesh_packed");
    AddMesh(packedMesh, /*packed=*/true, HdInterpolationUniform);
    _outputFilePath = "testHdStPackedNormals_packed.png";
    RunOffscreenTest();

    // --- Test 2: Side-by-side comparison, float vs packed (uniform) ---
    delegate.Remove(packedMesh);

    const SdfPath floatMesh("/mesh_float");
    AddMesh(floatMesh, /*packed=*/false, HdInterpolationUniform);
    delegate.UpdateTransform(
        floatMesh, GfMatrix4f().SetTranslate(GfVec3f(-2.5f, 0, 0)));

    const SdfPath packedMesh2("/mesh_packed2");
    AddMesh(packedMesh2, /*packed=*/true, HdInterpolationUniform);
    delegate.UpdateTransform(
        packedMesh2, GfMatrix4f().SetTranslate(GfVec3f(2.5f, 0, 0)));

    _outputFilePath = "testHdStPackedNormals_comparison.png";
    RunOffscreenTest();

    // --- Test 3: Face-varying packed normals ---
    delegate.Remove(floatMesh);
    delegate.Remove(packedMesh2);

    const SdfPath fvarMesh("/mesh_fvar");
    AddMesh(fvarMesh, /*packed=*/true, HdInterpolationFaceVarying);
    _outputFilePath = "testHdStPackedNormals_fvar.png";
    RunOffscreenTest();

    // --- Test 4: Update normals from float to packed (uniform) ---
    delegate.Remove(fvarMesh);

    const SdfPath updateMesh("/mesh_update");
    AddMesh(updateMesh, /*packed=*/false, HdInterpolationUniform);
    _outputFilePath = "testHdStPackedNormals_update_uniform_before.png";
    RunOffscreenTest();

    delegate.RemovePrimvar(updateMesh, HdTokens->normals);
    {
        VtArray<HdVec4f_2_10_10_10_REV> normals;
        normals.reserve(kFaceNormals.size());
        for (const GfVec3f& n : kFaceNormals) {
            normals.push_back(HdVec4f_2_10_10_10_REV(n));
        }
        delegate.AddPrimvar(updateMesh, HdTokens->normals, VtValue(normals),
            HdInterpolationUniform, HdPrimvarRoleTokens->normal);
    }
    _outputFilePath = "testHdStPackedNormals_update_uniform_after.png";
    RunOffscreenTest();

    // --- Test 5: Update normals from float to packed (face-varying) ---
    delegate.Remove(updateMesh);

    const SdfPath updateFvarMesh("/mesh_update_fvar");
    AddMesh(updateFvarMesh, /*packed=*/false, HdInterpolationFaceVarying);
    _outputFilePath = "testHdStPackedNormals_update_fvar_before.png";
    RunOffscreenTest();

    delegate.RemovePrimvar(updateFvarMesh, HdTokens->normals);
    {
        VtArray<HdVec4f_2_10_10_10_REV> normals;
        normals.reserve(kFaceVaryingNormals.size());
        for (const GfVec3f& n : kFaceVaryingNormals) {
            normals.push_back(HdVec4f_2_10_10_10_REV(n));
        }
        delegate.AddPrimvar(updateFvarMesh, HdTokens->normals, VtValue(normals),
            HdInterpolationFaceVarying, HdPrimvarRoleTokens->normal);
    }
    _outputFilePath = "testHdStPackedNormals_update_fvar_after.png";
    RunOffscreenTest();
}

////////////////////////////////////////////////////////////

void
BasicTest(int argc, char* argv[])
{
    My_TestGLDrawing driver;
    driver.RunPackedNormalsTests(argc, argv);
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
