//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdSt/unitTestGLDrawing.h"

#include "pxr/imaging/hdSt/basisCurves.h"
#include "pxr/imaging/hdSt/mesh.h"
#include "pxr/imaging/hdSt/points.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/staticTokens.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (feyRay)
    (wireframe)
    (wireframeBack)
    (wireframeFront)
    (wireOnSurfUnlit)
    (points)
    (pointsAndSurf)
    );

class My_TestGLDrawing : public HdSt_UnitTestGLDrawing {
public:
    My_TestGLDrawing() {
        SetCameraRotate(60.0f, 0.0f);
        SetCameraTranslate(GfVec3f(0, 0, -20.0f-1.7320508f*2.0f));
        _reprName = HdReprTokens->hull;
        _refineLevel = 0;
    }

    // HdSt_UnitTestGLDrawing overrides
    void InitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;
    void Present(uint32_t framebuffer) override;

protected:
    virtual void ParseArgs(int argc, char *argv[]);

private:
    TfToken _reprName;
    int _refineLevel;
    HdSt_TestDriver* _driver;
    std::string _outputFilePath;
};

////////////////////////////////////////////////////////////

void
My_TestGLDrawing::InitTest()
{
    std::cout << "My_TestGLDrawing::InitTest()\n";

    _driver = new HdSt_TestDriver(_reprName);
    HdUnitTestDelegate &delegate = _driver->GetDelegate();
    delegate.SetRefineLevel(_refineLevel);

    // configure display styles

    // wireframe
    HdMesh::ConfigureRepr(_tokens->wireframe,
                          HdMeshReprDesc(HdMeshGeomStyleEdgeOnly,
                                         HdCullStyleNothing,
                                         HdMeshReprDescTokens->surfaceShaderUnlit,
                                         /*flatShadingEnabled=*/true));
    HdBasisCurves::ConfigureRepr(_tokens->wireframe,
                                 HdBasisCurvesGeomStyleWire);
    HdPoints::ConfigureRepr(_tokens->wireframe,
                            HdPointsGeomStylePoints);

    // wireframe + backface culling
    HdMesh::ConfigureRepr(_tokens->wireframeFront,
                          HdMeshReprDesc(HdMeshGeomStyleEdgeOnly,
                                         HdCullStyleBack,
                                         HdMeshReprDescTokens->surfaceShaderUnlit,
                                         /*flatShadingEnabled=*/true));
    HdBasisCurves::ConfigureRepr(_tokens->wireframeFront,
                                 HdBasisCurvesGeomStyleWire);
    HdPoints::ConfigureRepr(_tokens->wireframeFront,
                            HdPointsGeomStylePoints);

    // wireframe + frontface culling
    HdMesh::ConfigureRepr(_tokens->wireframeBack,
                          HdMeshReprDesc(HdMeshGeomStyleEdgeOnly,
                                         HdCullStyleFront,
                                         HdMeshReprDescTokens->surfaceShaderUnlit,
                                         /*flatShadingEnabled=*/true));
    HdBasisCurves::ConfigureRepr(_tokens->wireframeFront,
                                 HdBasisCurvesGeomStyleWire);
    HdPoints::ConfigureRepr(_tokens->wireframeFront,
                            HdPointsGeomStylePoints);

    // wireframe on surface, unlit
    HdMesh::ConfigureRepr(_tokens->wireOnSurfUnlit,
                          HdMeshReprDesc(HdMeshGeomStyleEdgeOnSurf,
                                         HdCullStyleDontCare,
                                         HdMeshReprDescTokens->surfaceShaderUnlit,
                                         /*flatShadingEnabled=*/true,
                                         /*blendWireframeColor=*/false,
                                         /*forceOpaqueEdges=*/false));
    HdBasisCurves::ConfigureRepr(_tokens->wireOnSurfUnlit,
                                 HdBasisCurvesGeomStyleWire);
    HdPoints::ConfigureRepr(_tokens->wireOnSurfUnlit,
                            HdPointsGeomStylePoints);

    // 2-pass FeyRay
    HdMesh::ConfigureRepr(_tokens->feyRay,
                          HdMeshReprDesc(HdMeshGeomStyleSurf,
                                         HdCullStyleFront,
                                         HdMeshReprDescTokens->surfaceShader,
                                         /*flatShadingEnabled=*/false),
                          HdMeshReprDesc(HdMeshGeomStyleEdgeOnly,
                                         HdCullStyleBack,
                                         HdMeshReprDescTokens->constantColor,
                                         /*flatShadingEnabled=*/true));
    HdBasisCurves::ConfigureRepr(_tokens->feyRay,
                                 HdBasisCurvesGeomStyleWire);
    HdPoints::ConfigureRepr(_tokens->feyRay,
                            HdPointsGeomStylePoints);

    // points
    HdMesh::ConfigureRepr(_tokens->points,
                          HdMeshReprDesc(HdMeshGeomStylePoints,
                                         HdCullStyleNothing,
                                         HdMeshReprDescTokens->constantColor,
                                         /*flatShadingEnabled=*/true));

    HdBasisCurves::ConfigureRepr(_tokens->points,
                                 HdBasisCurvesGeomStyleWire);
    HdPoints::ConfigureRepr(_tokens->points,
                            HdPointsGeomStylePoints);

    // points and surface
    HdMesh::ConfigureRepr(_tokens->pointsAndSurf,
                          HdMeshReprDesc(HdMeshGeomStylePoints,
                                         HdCullStyleNothing,
                                         HdMeshReprDescTokens->constantColor,
                                         /*flatShadingEnabled=*/true),
                          HdMeshReprDesc(HdMeshGeomStyleSurf,
                                         HdCullStyleNothing,
                                         HdMeshReprDescTokens->surfaceShader,
                                         /*flatShadingEnabled=*/false));

    HdBasisCurves::ConfigureRepr(_tokens->pointsAndSurf,
                                 HdBasisCurvesGeomStyleWire);
    HdPoints::ConfigureRepr(_tokens->pointsAndSurf,
                            HdPointsGeomStylePoints);

    GfVec3d pos(0);
    {
        GfMatrix4d dmat;

        dmat.SetTranslate(pos);
        delegate.AddCube(SdfPath("/cube1"), GfMatrix4f(dmat)); // default repr
        pos[0] += 3.0;

        dmat.SetTranslate(pos);
        delegate.AddCube(SdfPath("/cube2"), GfMatrix4f(dmat));
        delegate.SetReprSelector(SdfPath("/cube2"),
                HdReprSelector(HdReprTokens->smoothHull));
        pos[0] += 3.0;

        dmat.SetTranslate(pos);
        delegate.AddCube(SdfPath("/cube3"), GfMatrix4f(dmat));
        delegate.SetReprSelector(SdfPath("/cube3"),
                HdReprSelector(_tokens->wireframe));
        pos[0] += 3.0;

        dmat.SetTranslate(pos);
        delegate.AddCube(SdfPath("/cube4"), GfMatrix4f(dmat));
        delegate.SetReprSelector(SdfPath("/cube4"),
                HdReprSelector(_tokens->wireframeFront));
        pos[0] += 3.0;

        dmat.SetTranslate(pos);
        delegate.AddCube(SdfPath("/cube5"), GfMatrix4f(dmat));
        delegate.SetReprSelector(SdfPath("/cube5"),
                HdReprSelector(_tokens->wireframeBack));
        pos[0] += 3.0;

        dmat.SetTranslate(pos);
        delegate.AddCube(SdfPath("/cube6"), GfMatrix4f(dmat));
        delegate.SetReprSelector(SdfPath("/cube6"),
                HdReprSelector(_tokens->wireOnSurfUnlit));
        pos[0] += 3.0;

        // wrap
        pos = GfVec3d(0, -3.0, 0);

        dmat.SetTranslate(pos);
        delegate.AddCube(SdfPath("/cube7"), GfMatrix4f(dmat));
        delegate.SetReprSelector(SdfPath("/cube7"),
                HdReprSelector(_tokens->feyRay));
        pos[0] += 3.0;

        dmat.SetTranslate(pos);
        delegate.AddCube(SdfPath("/cube8"), GfMatrix4f(dmat));
        delegate.SetReprSelector(SdfPath("/cube8"),
                HdReprSelector(_tokens->points));
        pos[0] += 3.0;

        dmat.SetTranslate(pos);
        delegate.AddCube(SdfPath("/cube9"), GfMatrix4f(dmat));
        delegate.SetReprSelector(SdfPath("/cube9"),
                HdReprSelector(_tokens->pointsAndSurf));
        pos[0] += 3.0;
    }
    GfVec3f center(7.5f, 0, 1.5f);

    // center camera
    SetCameraTranslate(GetCameraTranslate() - center);

    _driver->SetClearColor(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
    _driver->SetClearDepth(1.0f);
    _driver->SetupAovs(GetWidth(), GetHeight());
}

void
My_TestGLDrawing::DrawTest()
{
    int width = GetWidth(), height = GetHeight();
    GfMatrix4d viewMatrix = GetViewMatrix();
    GfMatrix4d projMatrix = GetProjectionMatrix();

    // camera
    _driver->SetCamera(
        viewMatrix,
        projMatrix,
        CameraUtilFraming(
            GfRect2i(GfVec2i(0, 0), width, height)));

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

/* virtual */
void
My_TestGLDrawing::ParseArgs(int argc, char *argv[])
{
    for (int i=0; i<argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--repr") {
            _reprName = TfToken(argv[++i]);
        } else if (arg == "--refineLevel") {
            _refineLevel = atoi(argv[++i]);
        } else if (arg == "--write") {
            _outputFilePath = argv[++i];
        }
    }
}

void
DisplayStyleTest(int argc, char *argv[])
{
    My_TestGLDrawing driver;
    driver.RunTest(argc, argv);
}

int main(int argc, char *argv[])
{
    TfErrorMark mark;

    DisplayStyleTest(argc, argv);

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

