#include "pxr/imaging/glf/glew.h"

#include "pxr/usdImaging/usdImaging/unitTestGLDrawing.h"

#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/range3d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/getenv.h"

#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/usdImaging/usdImaging/unitTestHelper.h"
#include "pxr/usdImaging/usdImaging/hdEngine.h"
#include "pxr/usdImaging/usdImaging/refEngine.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include <boost/shared_ptr.hpp>

#include <QtGui/QApplication>
#include <iomanip>
#include <iostream>
#include <sstream>

typedef boost::shared_ptr<class UsdImagingEngine> UsdImagingEngineSharedPtr;

class My_TestGLDrawing : public UsdImaging_UnitTestGLDrawing {
public:
    My_TestGLDrawing() {
        _mousePos[0] = _mousePos[1] = 0;
        _mouseButton[0] = _mouseButton[1] = _mouseButton[2] = false;
        _rotate[0] = _rotate[1] = 0;
        _translate[0] = _translate[1] = _translate[2] = 0;
    }

    // UsdImaging_UnitTestGLDrawing overrides
    virtual void InitTest();
    virtual void DrawTest(bool offscreen);
    virtual void ShutdownTest();

    virtual void MousePress(int button, int x, int y);
    virtual void MouseRelease(int button, int x, int y);
    virtual void MouseMove(int x, int y);

    void Draw();
    void Pick(GfVec2i const &startPos, GfVec2i const &endPos);

private:
    UsdStageRefPtr _stage;
    UsdImagingEngineSharedPtr _engine;

    GfFrustum _frustum;
    GfMatrix4d _viewMatrix;

    float _rotate[2];
    float _translate[3];
    int _mousePos[2];
    bool _mouseButton[3];
};

GLuint vao;

void
My_TestGLDrawing::InitTest()
{
    std::cout << "My_TestGLDrawing::InitTest()\n";
    _stage = UsdStage::Open(GetStageFilePath());
    SdfPathVector excludedPaths;

    bool isEnabledHydra = (TfGetenv("HD_ENABLED", "1") == "1");
    if (isEnabledHydra) {
        std::cout << "Using HD Renderer.\n";
        _engine.reset(new UsdImagingHdEngine(
            _stage->GetPseudoRoot().GetPath(), excludedPaths));
    } else{
        std::cout << "Using Reference Renderer.\n"; 
        _engine.reset(new UsdImagingRefEngine(excludedPaths));
    }
    _engine->SetSelectionColor(GfVec4f(1, 1, 0, 1));

    std::cout << glGetString(GL_VENDOR) << "\n";
    std::cout << glGetString(GL_RENDERER) << "\n";
    std::cout << glGetString(GL_VERSION) << "\n";

    if(IsEnabledTestLighting()) {
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        float position[4] = {0,-.5,.5,0};
        glLightfv(GL_LIGHT0, GL_POSITION, position);
    }

    if (_ShouldFrameAll()) {
        TfTokenVector purposes;
        purposes.push_back(UsdGeomTokens->default_);
        purposes.push_back(UsdGeomTokens->proxy);

        // Extent hints are sometimes authored as an optimization to avoid
        // computing bounds, they are particularly useful for some tests where
        // there is no bound on the first frame.
        bool useExtentHints = true;
        UsdGeomBBoxCache bboxCache(UsdTimeCode::Default(), purposes, useExtentHints);

        GfBBox3d bbox = bboxCache.ComputeWorldBound(_stage->GetPseudoRoot());
        GfRange3d world = bbox.ComputeAlignedRange();

        GfVec3d worldCenter = (world.GetMin() + world.GetMax()) / 2.0;
        double worldSize = world.GetSize().GetLength();

        std::cerr << "worldCenter: " << worldCenter << "\n";
        std::cerr << "worldSize: " << worldSize << "\n";
        if (UsdGeomGetStageUpAxis(_stage) == UsdGeomTokens->z) {
            // transpose y and z centering translation
            _translate[0] = -worldCenter[0];
            _translate[1] = -worldCenter[2];
            _translate[2] = -worldCenter[1] - worldSize;
        } else {
            _translate[0] = -worldCenter[0];
            _translate[1] = -worldCenter[1];
            _translate[2] = -worldCenter[2] - worldSize;
        }
    } else {
        _translate[0] = 0.0;
        _translate[1] = -1000.0;
        _translate[2] = -2500.0;
    }
}

void
My_TestGLDrawing::DrawTest(bool offscreen)
{
    std::cout << "My_TestGLDrawing::DrawTest()\n";

    if (offscreen) {
        Draw();
        Pick(GfVec2i(170, 130), GfVec2i(171, 131));
        Draw();
        Pick(GfVec2i(170, 200), GfVec2i(171, 201));
        Draw();
        Pick(GfVec2i(320, 130), GfVec2i(321, 131));
        Draw();
        Pick(GfVec2i(400, 200), GfVec2i(401, 201));
        Draw();
    } else {
        Draw();
    }
}

void
My_TestGLDrawing::Draw()
{
    int width = GetWidth(), height = GetHeight();

    double aspectRatio = double(width)/height;
    _frustum.SetPerspective(60.0, aspectRatio, 1, 100000.0);

    _viewMatrix.SetIdentity();
    _viewMatrix *= GfMatrix4d().SetRotate(GfRotation(GfVec3d(0, 1, 0), _rotate[0]));
    _viewMatrix *= GfMatrix4d().SetRotate(GfRotation(GfVec3d(1, 0, 0), _rotate[1]));
    _viewMatrix *= GfMatrix4d().SetTranslate(GfVec3d(_translate[0], _translate[1], _translate[2]));

    GfMatrix4d projMatrix = _frustum.ComputeProjectionMatrix();

    if (UsdGeomGetStageUpAxis(_stage) == UsdGeomTokens->z) {
        // rotate from z-up to y-up
        _viewMatrix = 
            GfMatrix4d().SetRotate(GfRotation(GfVec3d(1.0,0.0,0.0), -90.0)) *
            _viewMatrix;
    }

    GfVec4d viewport(0, 0, width, height);
    _engine->SetCameraState(_viewMatrix, projMatrix, viewport);

    UsdImagingEngine::RenderParams params;
    params.drawMode = GetDrawMode();
    params.enableLighting =  IsEnabledTestLighting();
    params.complexity = _GetComplexity();
    params.cullStyle = IsEnabledCullBackfaces() ?
        UsdImagingEngine::CULL_STYLE_BACK :
        UsdImagingEngine::CULL_STYLE_NOTHING;

    glViewport(0, 0, width, height);

    GLfloat clearColor[4] = { 1.0f, .5f, 0.1f, 1.0f };
    glClearBufferfv(GL_COLOR, 0, clearColor);

    GLfloat clearDepth[1] = { 1.0f };
    glClearBufferfv(GL_DEPTH, 0, clearDepth);

    glEnable(GL_DEPTH_TEST);


    if(IsEnabledTestLighting()) {
        _engine->SetLightingStateFromOpenGL();
    }

    if (not GetClipPlanes().empty()) {
        params.clipPlanes = GetClipPlanes();
        for (int i=0; i<GetClipPlanes().size(); ++i) {
            glEnable(GL_CLIP_PLANE0 + i);
        }
    }

    _engine->Render(_stage->GetPseudoRoot(), params);

    std::string imageFilePath = GetOutputFilePath();
    if (not imageFilePath.empty()) {
        static size_t i = 0;

        std::stringstream suffix;
        suffix << "_" << std::setw(3) << std::setfill('0') << i << ".png";
        imageFilePath = TfStringReplace(imageFilePath, ".png", suffix.str());
        std::cout << imageFilePath << "\n";
        WriteToFile("color", imageFilePath);

        i++;
    }
}

void
My_TestGLDrawing::ShutdownTest()
{
    std::cout << "My_TestGLDrawing::ShutdownTest()\n";
    _engine->InvalidateBuffers();
    _engine.reset();
}

void
My_TestGLDrawing::MousePress(int button, int x, int y)
{
    _mouseButton[button] = 1;
    _mousePos[0] = x;
    _mousePos[1] = y;
}

void
My_TestGLDrawing::MouseRelease(int button, int x, int y)
{
    _mouseButton[button] = 0;

    if (not (QApplication::keyboardModifiers() & Qt::AltModifier)) {
        std::cerr << "Pick " << x << ", " << y << "\n";
        GfVec2i startPos = GfVec2i(_mousePos[0] - 1, _mousePos[1] - 1);
        GfVec2i endPos   = GfVec2i(_mousePos[0] + 1, _mousePos[1] + 1);
        Pick(startPos, endPos);
    }
}

void
My_TestGLDrawing::MouseMove(int x, int y)
{
    int dx = x - _mousePos[0];
    int dy = y - _mousePos[1];

    if (_mouseButton[0]) {
        _rotate[0] += dx;
        _rotate[1] += dy;
    } else if (_mouseButton[1]) {
        _translate[0] += dx;
        _translate[1] -= dy;
    } else if (_mouseButton[2]) {
        _translate[2] += dx;
    }

    _mousePos[0] = x;
    _mousePos[1] = y;
}

void
My_TestGLDrawing::Pick(GfVec2i const &startPos, GfVec2i const &endPos)
{
    GfFrustum frustum = _frustum;
    float width = GetWidth(), height = GetHeight();

    GfVec2d min(2*startPos[0]/width-1, 1-2*startPos[1]/height);
    GfVec2d max(2*(endPos[0]+1)/width-1, 1-2*(endPos[1]+1)/height);
    // scale window
    GfVec2d origin = frustum.GetWindow().GetMin();
    GfVec2d scale = frustum.GetWindow().GetMax() - frustum.GetWindow().GetMin();
    min = origin + GfCompMult(scale, 0.5 * (GfVec2d(1.0, 1.0) + min));
    max = origin + GfCompMult(scale, 0.5 * (GfVec2d(1.0, 1.0) + max));

    frustum.SetWindow(GfRange2d(min, max));

    UsdImagingEngine::RenderParams params;
    params.enableIdRender = true;

    GfVec3d outHitPoint;
    SdfPath outHitPrimPath;
    SdfPath outHitInstancerPath;
    int outHitInstanceIndex;

    SdfPathVector selection;

    if (_engine->TestIntersection(
        _viewMatrix,
        frustum.ComputeProjectionMatrix(),
        GfMatrix4d(1),
        _stage->GetPseudoRoot(),
        params,
        &outHitPoint,
        &outHitPrimPath,
        &outHitInstancerPath,
        &outHitInstanceIndex)) {

        std::cout << "Hit "
                  << outHitPoint << ", "
                  << outHitPrimPath << ", "
                  << outHitInstancerPath << ", "
                  << outHitInstanceIndex << "\n";

        if (not outHitInstancerPath.IsEmpty()) {
            outHitPrimPath = 
                _engine->GetPrimPathFromInstanceIndex(outHitInstancerPath,
                                                      outHitInstanceIndex);
        }
        _engine->SetSelectionColor(GfVec4f(1, 1, 0, 1));

        selection.push_back(outHitPrimPath);
    }

    _engine->SetSelected(selection);

    _Redraw();
}

void
BasicTest(int argc, char *argv[])
{
    My_TestGLDrawing driver;
    driver.RunTest(argc, argv);
}

int main(int argc, char *argv[])
{
    BasicTest(argc, argv);
    std::cout << "OK" << std::endl;
}
