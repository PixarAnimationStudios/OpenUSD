//
// Copyright 2021 Pixar
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

#include "pxr/usdImaging/usdImagingGL/unitTestGLDrawing.h"

#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/gf/frustum.h"
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

#include <iomanip>
#include <iostream>
#include <sstream>

PXR_NAMESPACE_USING_DIRECTIVE

using UsdImagingGLEngineSharedPtr = std::shared_ptr<class UsdImagingGLEngine>;

class My_TestGLDrawing : public UsdImagingGL_UnitTestGLDrawing {
public:
    My_TestGLDrawing() {
        _mousePos[0] = _mousePos[1] = 0;
        _mouseButton[0] = _mouseButton[1] = _mouseButton[2] = false;
        _rotate[0] = _rotate[1] = 0;
        _translate[0] = _translate[1] = _translate[2] = 0;
    }

    // UsdImagingGL_UnitTestGLDrawing overrides
    void InitTest() override;
    void DrawTest(bool offscreen) override;
    void ShutdownTest() override;

    void MousePress(int button, int x, int y, int modKeys) override;
    void MouseRelease(int button, int x, int y, int modKeys) override;
    void MouseMove(int x, int y, int modKeys) override;

    void Draw();
    void Pick(GfVec2i const &startPos, GfVec2i const &endPos);

private:
    UsdStageRefPtr _stage;
    UsdImagingGLEngineSharedPtr _engine;

    GfFrustum _frustum;
    GfMatrix4d _viewMatrix;

    float _rotate[2];
    float _translate[3];
    int _mousePos[2];
    bool _mouseButton[3];
    SdfPath _sharedId;
};

void
My_TestGLDrawing::InitTest()
{
    std::cout << "My_TestGLDrawing::InitTest()\n";
    _stage = UsdStage::Open(GetStageFilePath());
    SdfPathVector excludedPaths;

    _sharedId = SdfPath("/Shared");
    _engine.reset(new UsdImagingGLEngine(_stage->GetPseudoRoot().GetPath(),
                                   excludedPaths,
                                   SdfPathVector(), // invisedPrimPaths
                                   _sharedId));

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

    Draw();
    _engine->SetSelectionColor(GfVec4f(1, 1, 0, 1));

    if (GetStageFilePath() == "instance.usda") {
        SdfPathVector paths;
        paths.push_back(SdfPath("/Group_Multiple_Instances"));
        paths.push_back(SdfPath("/DormRoomDouble/Geom/cube1"));
        _engine->SetSelected(paths);
        Draw();
        _engine->ClearSelected();
        _engine->AddSelected(SdfPath("/Invis_Instance"), -1);
        Draw();
    } else if (GetStageFilePath() == "pi.usda") {
        // Test highlighting PI instances.
        _engine->AddSelected(SdfPath("/World/Instancer"), -1);
        Draw();
        _engine->ClearSelected();
        _engine->AddSelected(SdfPath("/World/Instancer"), 0);
        Draw();
        _engine->ClearSelected();
        _engine->AddSelected(SdfPath("/World/Instancer"), 1);
        Draw();
        _engine->ClearSelected();
        _engine->AddSelected(SdfPath("/World/Instancer"), 2);
        Draw();
        _engine->ClearSelected();
        _engine->AddSelected(SdfPath("/World/Instancer"), 3);
        Draw();
        _engine->ClearSelected();
    } else if (GetStageFilePath() == "pi_ni.usda") {
        // Test PI highlighting.
        _engine->SetSelected({SdfPath("/Bar/C")});
        Draw();
        _engine->SetSelected({SdfPath("/Bar/C/Protos/Proto2")});
        Draw();
        _engine->SetSelected({SdfPath("/Bar/C/Protos/Proto2/P2")});
        Draw();

        // Test PI/NI highlighting.
        _engine->SetSelected({SdfPath("/Cube")});
        Draw();
        _engine->SetSelected({SdfPath("/Cube/Instancer")});
        Draw();
        _engine->SetSelected({SdfPath("/Cube/Instancer/Protos")});
        Draw();
        _engine->SetSelected({SdfPath("/Cube/Instancer/Protos/Proto1")});
        Draw();
        _engine->SetSelected({SdfPath("/Cube/Instancer/Protos/Proto2/cube")});
        Draw();

        // Test NI/PI/NI highlighting.
        _engine->SetSelected({SdfPath("/Foo/X3/C3")});
        Draw();
        _engine->SetSelected({SdfPath("/Foo/X3/C3/Instancer")});
        Draw();
        _engine->SetSelected({SdfPath("/Foo/X3/C3/Instancer/Protos")});
        Draw();
        _engine->SetSelected({SdfPath("/Foo/X3/C3/Instancer/Protos/Proto1")});
        Draw();
        _engine->SetSelected({SdfPath("/Foo/X3/C3/Instancer/Protos/Proto2/cube")});
        Draw();

        // Test highlighting in prototype.
        SdfPath prototype1 = _stage->GetPrimAtPath(SdfPath("/Foo/X3/C3"))
                            .GetPrototype().GetPath();
        _engine->SetSelected({prototype1});
        Draw();
        _engine->SetSelected({prototype1.AppendPath(SdfPath("Instancer"))});
        Draw();
        _engine->SetSelected({prototype1.AppendPath(SdfPath("Instancer/Protos"))});
        Draw();
        _engine->SetSelected({prototype1.AppendPath(SdfPath("Instancer/Protos/Proto1"))});
        Draw();
        _engine->SetSelected({prototype1.AppendPath(SdfPath("Instancer/Protos/Proto2/cube"))});
        Draw();

        SdfPath prototype2 = _stage->GetPrimAtPath(prototype1.AppendPath(SdfPath("Instancer/Protos/Proto1")))
                            .GetPrototype().GetPath();
        _engine->SetSelected({prototype2.AppendPath(SdfPath("cube"))});
        Draw();
    } // else nothing.
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

    _engine->Render(_stage->GetPseudoRoot(), params);

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

    UsdImagingGLRenderParams params;
    params.enableIdRender = true;

    GfVec3d outHitPoint;
    GfVec3d outHitNormal;
    SdfPath outHitPrimPath;
    SdfPath outHitInstancerPath;
    int outHitInstanceIndex;

    SdfPathVector selection;

    if (_engine->TestIntersection(
        _viewMatrix,
        frustum.ComputeProjectionMatrix(),
        _stage->GetPseudoRoot(),
        params,
        &outHitPoint,
        &outHitNormal,
        &outHitPrimPath,
        &outHitInstancerPath,
        &outHitInstanceIndex)) {

        std::cout << "Hit "
                  << outHitPoint << ", "
                  << outHitNormal << ", "
                  << outHitPrimPath << ", "
                  << outHitInstancerPath << ", "
                  << outHitInstanceIndex << "\n";

        _engine->SetSelectionColor(GfVec4f(1, 1, 0, 1));
        selection.push_back(outHitPrimPath);
    }

    _engine->SetSelected(selection);
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
