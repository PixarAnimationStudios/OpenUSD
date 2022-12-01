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

#include "pxr/imaging/glf/simpleLightingContext.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/usdImaging/usdImaging/unitTestHelper.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/usdImaging/usdImagingGL/engine.h"

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
    virtual void InitTest();
    virtual void DrawTest(bool offscreen);
    virtual void ShutdownTest();

    virtual void MousePress(int button, int x, int y, int modKeys);
    virtual void MouseRelease(int button, int x, int y, int modKeys);
    virtual void MouseMove(int x, int y, int modKeys);

private:
    UsdStageRefPtr _stage;
    UsdImagingGLEngineSharedPtr _engine;
    GlfSimpleLightingContextRefPtr _lightingContext;

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

    if(IsEnabledTestLighting()) {
        // set same parameter as 
        // GlfSimpleLightingContext::SetStateFromOpenGL OpenGL defaults
        _lightingContext = GlfSimpleLightingContext::New();
        GlfSimpleLight light;
        light.SetPosition(GfVec4f(0, -.5, .5, 0));
        light.SetDiffuse(GfVec4f(1,1,1,1));
        light.SetAmbient(GfVec4f(0,0,0,1));
        light.SetSpecular(GfVec4f(1,1,1,1));
        GlfSimpleLightVector lights;
        lights.push_back(light);
        _lightingContext->SetLights(lights);

        GlfSimpleMaterial material;
        material.SetAmbient(GfVec4f(0.2, 0.2, 0.2, 1.0));
        material.SetDiffuse(GfVec4f(0.8, 0.8, 0.8, 1.0));
        material.SetSpecular(GfVec4f(0,0,0,1));
        material.SetShininess(0.0001f);
        _lightingContext->SetMaterial(material);
        _lightingContext->SetSceneAmbient(GfVec4f(0.2,0.2,0.2,1.0));
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

    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();

    int width = GetWidth(), height = GetHeight();
    std::string imageFilePath = GetOutputFilePath();

    double aspectRatio = double(width)/height;
    GfFrustum frustum;
    frustum.SetPerspective(60.0, aspectRatio, 1, 100000.0);

    GfMatrix4d viewMatrix;
    viewMatrix.SetIdentity();
    viewMatrix *= GfMatrix4d().SetRotate(GfRotation(GfVec3d(0, 1, 0), _rotate[0]));
    viewMatrix *= GfMatrix4d().SetRotate(GfRotation(GfVec3d(1, 0, 0), _rotate[1]));
    viewMatrix *= GfMatrix4d().SetTranslate(GfVec3d(_translate[0], _translate[1], _translate[2]));

    GfMatrix4d projMatrix = frustum.ComputeProjectionMatrix();

    GfMatrix4d modelViewMatrix = viewMatrix; 
    if (UsdGeomGetStageUpAxis(_stage) == UsdGeomTokens->z) {
        // rotate from z-up to y-up
        modelViewMatrix = 
            GfMatrix4d().SetRotate(GfRotation(GfVec3d(1.0,0.0,0.0), -90.0)) *
            modelViewMatrix;
    }

    GfVec4d viewport(0, 0, width, height);

    _engine->SetCameraState(modelViewMatrix, projMatrix);
    _engine->SetRenderViewport(viewport);

    _engine->SetRendererAov(GetRendererAov());

    // Render #1
    UsdImagingGLRenderParams params;
    params.drawMode = UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH;
    params.enableLighting =  IsEnabledTestLighting();
    params.complexity = _GetComplexity();
    params.cullStyle = UsdImagingGLCullStyle::CULL_STYLE_BACK;
    params.clearColor = GetClearColor();

    _engine->SetLightingState(_lightingContext);
    _engine->Render(_stage->GetPseudoRoot(), params);

    WriteToFile(_engine.get(), HdAovTokens->color, "out1.png");

    // Render #2
    params.drawMode = UsdImagingGLDrawMode::DRAW_SHADED_FLAT;
    _engine->Render(_stage->GetPseudoRoot(), params);

    WriteToFile(_engine.get(), HdAovTokens->color, "out2.png");

    // Render #3
    params.drawMode = UsdImagingGLDrawMode::DRAW_WIREFRAME;
    _engine->Render(_stage->GetPseudoRoot(), params);

    WriteToFile(_engine.get(), HdAovTokens->color, "out3.png");

    // Render #4
    params.complexity = 1.1;
    _engine->Render(_stage->GetPseudoRoot(), params);

    WriteToFile(_engine.get(), HdAovTokens->color, "out4.png");

    // Render #5
    params.drawMode = UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH;
    params.complexity = _GetComplexity();
    params.cullStyle = UsdImagingGLCullStyle::CULL_STYLE_BACK;

    _engine->Render(_stage->GetPseudoRoot(), params);

    WriteToFile(_engine.get(), HdAovTokens->color, "out5.png");
}

void
My_TestGLDrawing::ShutdownTest()
{
    std::cout << "My_TestGLDrawing::ShutdownTest()\n";
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
