//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImagingGL/unitTestGLDrawing.h"

#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/range3d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/getenv.h"

#include "pxr/imaging/garch/glDebugWindow.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/usdImaging/usdImaging/unitTestHelper.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/usdImaging/usdImagingGL/engine.h"
#include "pxr/imaging/glf/simpleLightingContext.h"
#include "pxr/base/tf/getenv.h"

#include <iomanip>
#include <iostream>
#include <sstream>

PXR_NAMESPACE_USING_DIRECTIVE

using UsdImagingGLEngineSharedPtr = std::shared_ptr<class UsdImagingGLEngine>;

struct OutHit {
    GfVec3d outHitPoint;
    GfVec3d outHitNormal;
    SdfPath outHitPrimPath;
    SdfPath outHitInstancerPath;
    int outHitInstanceIndex;
};


static bool
_CompareOutHit(OutHit const & lhs, OutHit const & rhs)
{
    static const bool skipInstancerDetails = TfGetenvBool(
        "USD_IMAGING_GL_PICK_TEST_SKIP_INSTANCER_DETAILS",
        false);

    double const epsilon = 1e-6;
    return GfIsClose(lhs.outHitPoint[0], rhs.outHitPoint[0], epsilon) &&
           GfIsClose(lhs.outHitPoint[1], rhs.outHitPoint[1], epsilon) &&
           GfIsClose(lhs.outHitPoint[2], rhs.outHitPoint[2], epsilon) &&
           GfIsClose(lhs.outHitNormal[0], rhs.outHitNormal[0], epsilon) &&
           GfIsClose(lhs.outHitNormal[1], rhs.outHitNormal[1], epsilon) &&
           GfIsClose(lhs.outHitNormal[2], rhs.outHitNormal[2], epsilon) &&
           lhs.outHitPrimPath == rhs.outHitPrimPath &&
           (skipInstancerDetails ||
            (lhs.outHitInstancerPath == rhs.outHitInstancerPath &&
             lhs.outHitInstanceIndex == rhs.outHitInstanceIndex));
}

class My_TestGLDrawing : public UsdImagingGL_UnitTestGLDrawing {
public:
    My_TestGLDrawing() {
        _mousePos[0] = _mousePos[1] = 0;
        _mouseButton[0] = _mouseButton[1] = _mouseButton[2] = false;
        _rotate[0] = _rotate[1] = 0;
        _translate[0] = _translate[1] = _translate[2] = 0;
    }

    // UsdImagingGL_UnitTestGLDrawing overrides
    virtual void InitTest();
    virtual void DrawTest(bool offscreen);
    virtual void ShutdownTest();

    virtual void MousePress(int button, int x, int y, int modKeys);
    virtual void MouseRelease(int button, int x, int y, int modKeys);
    virtual void MouseMove(int x, int y, int modKeys);

    void Draw(bool render=true);
    void Pick(GfVec2i const &startPos, GfVec2i const &endPos);
    void Pick(GfVec2i const &startPos, GfVec2i const &endPos, OutHit* out);

private:
    UsdStageRefPtr _stage;
    UsdImagingGLEngineSharedPtr _engine;

    GfFrustum _frustum;
    GfMatrix4d _viewMatrix;

    float _rotate[2];
    float _translate[3];
    int _mousePos[2];
    bool _mouseButton[3];
};

void
My_TestGLDrawing::InitTest()
{
    std::cout << "My_TestGLDrawing::InitTest()\n";
    _stage = UsdStage::Open(GetStageFilePath());
    SdfPathVector excludedPaths;

    _engine.reset(
        new UsdImagingGLEngine(_stage->GetPseudoRoot().GetPath(), 
                excludedPaths));
    if (!_GetRenderer().IsEmpty()) {
        if (!_engine->SetRendererPlugin(_GetRenderer())) {
            std::cerr << "Couldn't set renderer plugin: " <<
                _GetRenderer().GetText() << std::endl;
            exit(-1);
        } else {
            std::cout << "Renderer plugin: " << _GetRenderer().GetText()
                << std::endl;
        }
    }
    _engine->SetSelectionColor(GfVec4f(1, 1, 0, 1));

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

    std::cout << "Testing just picking/TestIntersection without rendering\n";
    const std::map<TfToken, OutHit>& expectedOutputs = {
        { 
            TfToken(""), 
                { GfVec3d(3.386115312576294, -2.0000052452087402, -0.5881438851356506),
                GfVec3d(0, -0.9980430603027344, 2.2161007702308985e-16),
                SdfPath("/Group/GI1/I1/Mesh1/Plane1"), 
                _stage->GetPrimAtPath(SdfPath("/Group/GI1/I1")).GetPrototype().
                    GetPath().AppendPath(SdfPath("Mesh1")),
                2 }
        },
        {
            TfToken("HdEmbreeRendererPlugin"), 
                { GfVec3d(5.819578170776367, -15.916473388671875, -4.240192413330078),
                GfVec3d(0, 0, 0),
                SdfPath("/Instance/I1/Mesh1/Plane1"), 
                SdfPath::EmptyPath(),
                0 }
        }
    };

    const OutHit& expectedOut = expectedOutputs.at(_GetRenderer());
    Draw(false);
    OutHit testOut;
    Pick(GfVec2i(320, 130), GfVec2i(171, 131), &testOut);
    TF_VERIFY(_CompareOutHit(testOut, expectedOut));

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
My_TestGLDrawing::Draw(bool render)
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
    _engine->SetCameraState(_viewMatrix, projMatrix);
    _engine->SetRenderViewport(viewport);

    _engine->SetRendererAov(GetRendererAov());

    UsdImagingGLRenderParams params;
    params.drawMode = GetDrawMode();
    params.enableLighting =  IsEnabledTestLighting();
    params.complexity = _GetComplexity();
    params.cullStyle = GetCullStyle();
    params.highlight = true;
    params.clearColor = GetClearColor();

    if(IsEnabledTestLighting()) {
        GlfSimpleLightingContextRefPtr lightingContext = GlfSimpleLightingContext::New();
        lightingContext->SetStateFromOpenGL();
        _engine->SetLightingState(lightingContext);
    }

    params.clipPlanes = GetClipPlanes();

    if (render) {
        do {
            _engine->Render(_stage->GetPseudoRoot(), params);
        } while (!_engine->IsConverged());

        std::string imageFilePath = GetOutputFilePath();
        if (!imageFilePath.empty()) {
            static size_t i = 0;

            std::stringstream suffix;
            suffix << "_" << std::setw(3) << std::setfill('0') << i << ".png";
            imageFilePath = TfStringReplace(imageFilePath, ".png", suffix.str());
            std::cout << imageFilePath << "\n";
            WriteToFile(_engine.get(), HdAovTokens->color, imageFilePath);

            i++;
        }
    }
}

void
My_TestGLDrawing::ShutdownTest()
{
    std::cout << "My_TestGLDrawing::ShutdownTest()\n";
    _engine.reset();
}

void
My_TestGLDrawing::MousePress(int button, int x, int y, int modKeys)
{
    _mouseButton[button] = 1;
    _mousePos[0] = x;
    _mousePos[1] = y;
}

void
My_TestGLDrawing::MouseRelease(int button, int x, int y, int modKeys)
{
    _mouseButton[button] = 0;

    if (!(modKeys & GarchGLDebugWindow::Alt)) {
        std::cerr << "Pick " << x << ", " << y << "\n";
        GfVec2i startPos = GfVec2i(_mousePos[0] - 1, _mousePos[1] - 1);
        GfVec2i endPos   = GfVec2i(_mousePos[0] + 1, _mousePos[1] + 1);
        Pick(startPos, endPos);
    }
}

void
My_TestGLDrawing::MouseMove(int x, int y, int modKeys)
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
My_TestGLDrawing::Pick(GfVec2i const &startPos, GfVec2i const &endPos) {
    Pick(startPos, endPos, nullptr);
}

void
My_TestGLDrawing::Pick(GfVec2i const &startPos, GfVec2i const &endPos, 
        OutHit* out)
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

    // XXX: For a timevarying test need to set timecode for frame param
    UsdImagingGLRenderParams params;
    params.enableIdRender = true;

    OutHit outHit;

    SdfPathVector selection;

    if (_engine->TestIntersection(
        _viewMatrix,
        frustum.ComputeProjectionMatrix(),
        _stage->GetPseudoRoot(),
        params,
        &outHit.outHitPoint,
        &outHit.outHitNormal,
        &outHit.outHitPrimPath,
        &outHit.outHitInstancerPath,
        &outHit.outHitInstanceIndex)) {

        std::cout << "Hit "
                  << outHit.outHitPoint << ", "
                  << outHit.outHitNormal << ", "
                  << outHit.outHitPrimPath << ", "
                  << outHit.outHitInstancerPath << ", "
                  << outHit.outHitInstanceIndex << "\n";

        _engine->SetSelectionColor(GfVec4f(1, 1, 0, 1));
        selection.push_back(outHit.outHitPrimPath);
    }

    if (out) {
        *out = outHit;
    } else {
        _engine->SetSelected(selection);
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
    BasicTest(argc, argv);
    std::cout << "OK" << std::endl;
}
