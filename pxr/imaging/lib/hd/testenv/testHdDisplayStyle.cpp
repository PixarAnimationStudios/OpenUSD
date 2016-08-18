#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/basisCurves.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/points.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/unitTestGLDrawing.h"
#include "pxr/imaging/hd/unitTestHelper.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/staticTokens.h"

#include <iostream>

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

class My_TestGLDrawing : public Hd_UnitTestGLDrawing {
public:
    My_TestGLDrawing() {
        SetCameraRotate(60.0f, 0.0f);
        SetCameraTranslate(GfVec3f(0, 0, -20.0f-1.7320508f*2.0f));
        _reprName = HdTokens->hull;
        _refineLevel = 0;
    }

    // Hd_UnitTestGLDrawing overrides
    virtual void InitTest();
    virtual void DrawTest();
    virtual void OffscreenTest();

protected:
    virtual void ParseArgs(int argc, char *argv[]);

private:
    TfToken _reprName;
    int _refineLevel;
    Hd_TestDriver* _driver;
    std::string _outputFilePath;
};

////////////////////////////////////////////////////////////

GLuint vao;

void
My_TestGLDrawing::InitTest()
{
    std::cout << "My_TestGLDrawing::InitTest()\n";

    _driver = new Hd_TestDriver(_reprName);
    Hd_UnitTestDelegate &delegate = _driver->GetDelegate();
    delegate.SetRefineLevel(_refineLevel);

    // configure display styles

    // wireframe
    HdMesh::ConfigureRepr(_tokens->wireframe,
                          HdMeshReprDesc(HdMeshGeomStyleEdgeOnly,
                                         HdCullStyleNothing,
                                         /*lit=*/false,
                                         /*smoothNormals=*/false));
    HdBasisCurves::ConfigureRepr(_tokens->wireframe,
                                 HdBasisCurvesGeomStyleLine);
    HdPoints::ConfigureRepr(_tokens->wireframe,
                            HdPointsGeomStylePoints);

    // wireframe + backface culling
    HdMesh::ConfigureRepr(_tokens->wireframeFront,
                          HdMeshReprDesc(HdMeshGeomStyleEdgeOnly,
                                         HdCullStyleBack,
                                         /*lit=*/false,
                                         /*smoothNormals=*/false));
    HdBasisCurves::ConfigureRepr(_tokens->wireframeFront,
                                 HdBasisCurvesGeomStyleLine);
    HdPoints::ConfigureRepr(_tokens->wireframeFront,
                            HdPointsGeomStylePoints);

    // wireframe + frontface culling
    HdMesh::ConfigureRepr(_tokens->wireframeBack,
                          HdMeshReprDesc(HdMeshGeomStyleEdgeOnly,
                                         HdCullStyleFront,
                                         /*lit=*/false,
                                         /*smoothNormals=*/false));
    HdBasisCurves::ConfigureRepr(_tokens->wireframeFront,
                                 HdBasisCurvesGeomStyleLine);
    HdPoints::ConfigureRepr(_tokens->wireframeFront,
                            HdPointsGeomStylePoints);

    // wireframe on surface, unlit
    HdMesh::ConfigureRepr(_tokens->wireOnSurfUnlit,
                          HdMeshReprDesc(HdMeshGeomStyleEdgeOnSurf,
                                         HdCullStyleDontCare,
                                         /*lit=*/false,
                                         /*smoothNormals=*/false));
    HdBasisCurves::ConfigureRepr(_tokens->wireOnSurfUnlit,
                                 HdBasisCurvesGeomStyleLine);
    HdPoints::ConfigureRepr(_tokens->wireOnSurfUnlit,
                            HdPointsGeomStylePoints);

    // 2-pass FeyRay
    HdMesh::ConfigureRepr(_tokens->feyRay,
                          HdMeshReprDesc(HdMeshGeomStyleSurf,
                                         HdCullStyleFront,
                                         /*lit=*/true,
                                         /*smoothNormals=*/true),
                          HdMeshReprDesc(HdMeshGeomStyleEdgeOnly,
                                         HdCullStyleBack,
                                         /*lit=*/false,
                                         /*smoothNormals=*/false));
    HdBasisCurves::ConfigureRepr(_tokens->feyRay,
                                 HdBasisCurvesGeomStyleLine);
    HdPoints::ConfigureRepr(_tokens->feyRay,
                            HdPointsGeomStylePoints);

    // points
    HdMesh::ConfigureRepr(_tokens->points,
                          HdMeshReprDesc(HdMeshGeomStylePoints,
                                         HdCullStyleNothing,
                                         /*lit=*/false,
                                         /*smoothNormals=*/false));

    HdBasisCurves::ConfigureRepr(_tokens->points,
                                 HdBasisCurvesGeomStyleLine);
    HdPoints::ConfigureRepr(_tokens->points,
                            HdPointsGeomStylePoints);

    // points and surface
    HdMesh::ConfigureRepr(_tokens->pointsAndSurf,
                          HdMeshReprDesc(HdMeshGeomStylePoints,
                                         HdCullStyleNothing,
                                         /*lit=*/false,
                                         /*smoothNormals=*/false),
                          HdMeshReprDesc(HdMeshGeomStyleSurf,
                                         HdCullStyleNothing,
                                         /*lit=*/true,
                                         /*smoothNormals=*/true));

    HdBasisCurves::ConfigureRepr(_tokens->pointsAndSurf,
                                 HdBasisCurvesGeomStyleLine);
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
        delegate.SetReprName(SdfPath("/cube2"), HdTokens->smoothHull);
        pos[0] += 3.0;

        dmat.SetTranslate(pos);
        delegate.AddCube(SdfPath("/cube3"), GfMatrix4f(dmat));
        delegate.SetReprName(SdfPath("/cube3"), _tokens->wireframe);
        pos[0] += 3.0;

        dmat.SetTranslate(pos);
        delegate.AddCube(SdfPath("/cube4"), GfMatrix4f(dmat));
        delegate.SetReprName(SdfPath("/cube4"), _tokens->wireframeFront);
        pos[0] += 3.0;

        dmat.SetTranslate(pos);
        delegate.AddCube(SdfPath("/cube5"), GfMatrix4f(dmat));
        delegate.SetReprName(SdfPath("/cube5"), _tokens->wireframeBack);
        pos[0] += 3.0;

        dmat.SetTranslate(pos);
        delegate.AddCube(SdfPath("/cube6"), GfMatrix4f(dmat));
        delegate.SetReprName(SdfPath("/cube6"), _tokens->wireOnSurfUnlit);
        pos[0] += 3.0;

        // wrap
        pos = GfVec3d(0, -3.0, 0);

        dmat.SetTranslate(pos);
        delegate.AddCube(SdfPath("/cube7"), GfMatrix4f(dmat));
        delegate.SetReprName(SdfPath("/cube7"), _tokens->feyRay);
        pos[0] += 3.0;

        dmat.SetTranslate(pos);
        delegate.AddCube(SdfPath("/cube8"), GfMatrix4f(dmat));
        delegate.SetReprName(SdfPath("/cube8"), _tokens->points);
        pos[0] += 3.0;

        dmat.SetTranslate(pos);
        delegate.AddCube(SdfPath("/cube9"), GfMatrix4f(dmat));
        delegate.SetReprName(SdfPath("/cube9"), _tokens->pointsAndSurf);
        pos[0] += 3.0;
    }
    GfVec3f center(7.5f, 0, 1.5f);

    // center camera
    SetCameraTranslate(GetCameraTranslate() - center);

    // XXX: Setup a VAO, the current drawing engine will not yet do this.
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindVertexArray(0);
}

void
My_TestGLDrawing::DrawTest()
{
    GLfloat clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    glClearBufferfv(GL_COLOR, 0, clearColor);

    GLfloat clearDepth[1] = { 1.0f };
    glClearBufferfv(GL_DEPTH, 0, clearDepth);
    GLfloat clearStencil[1] = { 0.0f };
    glClearBufferfv(GL_STENCIL, 0, clearStencil);

    int width = GetWidth(), height = GetHeight();
    GfMatrix4d viewMatrix = GetViewMatrix();
    GfMatrix4d projMatrix = GetProjectionMatrix();

    // camera
    _driver->SetCamera(viewMatrix, projMatrix, GfVec4d(0, 0, width, height));

    glViewport(0, 0, width, height);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_POINT_SMOOTH);

    glBindVertexArray(vao);

    _driver->Draw();

    glBindVertexArray(0);
}

void
My_TestGLDrawing::OffscreenTest()
{
    DrawTest();

    if (not _outputFilePath.empty()) {
        WriteToFile("color", _outputFilePath);
    }
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

