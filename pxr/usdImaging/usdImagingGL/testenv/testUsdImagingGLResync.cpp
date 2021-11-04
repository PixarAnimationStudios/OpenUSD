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
#include "pxr/imaging/garch/glApi.h"

#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/range3d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/getenv.h"

#include "pxr/imaging/glf/contextCaps.h"
#include "pxr/imaging/glf/drawTarget.h"
#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/garch/glDebugWindow.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/usdImaging/usdImaging/unitTestHelper.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/usdImaging/usdImagingGL/engine.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

using UsdImagingGLEngineSharedPtr = std::shared_ptr<class UsdImagingGLEngine>;

int main(int argc, char *argv[])
{
    std::string stageFilePath;
    std::string imageFilePath = "out";

    SdfPath unloadPath = SdfPath::AbsoluteRootPath();

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--stage") {
            stageFilePath = argv[++i];
        } else if (arg == "--unload") {
            unloadPath = SdfPath(argv[++i]);
        } else if (arg == "--write") {
            imageFilePath = argv[++i];
        }
    }

    imageFilePath = TfStringReplace(imageFilePath, ".png", "");

    // prepare GL context
    GarchGLDebugWindow window("UsdImagingGL Test", 512, 512);
    window.Init();
    GarchGLApiLoad();

    // wrap into GlfGLContext so that GlfDrawTarget works
    GlfGLContextSharedPtr ctx = GlfGLContext::GetCurrentGLContext();
    GlfContextCaps::InitInstance();

    // prep draw target
    int width = 512, height = 512;
    GlfDrawTargetRefPtr drawTarget = GlfDrawTarget::New(GfVec2i(width, height));
    drawTarget->Bind();
    drawTarget->AddAttachment("color", GL_RGBA, GL_FLOAT, GL_RGBA);
    drawTarget->AddAttachment("depth", GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8,
                              GL_DEPTH24_STENCIL8);
    drawTarget->Unbind();

    GLfloat clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    GLfloat clearDepth[1] = { 1.0f };

    // open stage
    UsdStageRefPtr stage = UsdStage::Open(stageFilePath);
    UsdImagingGLEngineSharedPtr engine;
    SdfPathVector excludedPaths;

    if (UsdImagingGLEngine::IsHydraEnabled()) {
        std::cout << "Using HD Renderer.\n";
    } else {
        std::cout << "Using Reference Renderer.\n";
    }
    engine.reset(
        new UsdImagingGLEngine(stage->GetPseudoRoot().GetPath(), 
                excludedPaths));

    TfTokenVector purposes;
    purposes.push_back(UsdGeomTokens->default_);
    purposes.push_back(UsdGeomTokens->proxy);

    // Extent hints are sometimes authored as an optimization to avoid
    // computing bounds, they are particularly useful for some tests where
    // there is no bound on the first frame.
    bool useExtentHints = true;
    UsdGeomBBoxCache bboxCache(UsdTimeCode::Default(), purposes, useExtentHints);

    GfBBox3d bbox = bboxCache.ComputeWorldBound(stage->GetPseudoRoot());
    GfRange3d world = bbox.ComputeAlignedRange();

    GfVec3d worldCenter = (world.GetMin() + world.GetMax()) / 2.0;
    GfVec3d translate;
    double worldSize = world.GetSize().GetLength();

    std::cout << "worldCenter: " << worldCenter << "\n";
    std::cout << "worldSize: " << worldSize << "\n";
    if (UsdGeomGetStageUpAxis(stage) == UsdGeomTokens->z) {
        // transpose y and z centering translation
        translate[0] = -worldCenter[0];
        translate[1] = -worldCenter[2];
        translate[2] = -worldCenter[1] - worldSize;
    } else {
        translate[0] = -worldCenter[0];
        translate[1] = -worldCenter[1];
        translate[2] = -worldCenter[2] - worldSize;
    }

    double aspectRatio = double(width)/height;
    GfFrustum frustum;
    frustum.SetPerspective(60.0, aspectRatio, 1, 100000.0);
    GfMatrix4d viewMatrix = GfMatrix4d().SetTranslate(translate);
    GfMatrix4d projMatrix = frustum.ComputeProjectionMatrix();
    GfMatrix4d modelViewMatrix = viewMatrix;
    if (UsdGeomGetStageUpAxis(stage) == UsdGeomTokens->z) {
        // rotate from z-up to y-up
        modelViewMatrix =
            GfMatrix4d().SetRotate(GfRotation(GfVec3d(1.0,0.0,0.0), -90.0)) *
            modelViewMatrix;
    }

    // --------------------------------------------------------------------
    // draw.
    drawTarget->Bind();
    glViewport(0, 0, 512, 512);
    glEnable(GL_DEPTH_TEST);

    GfVec4d viewport(0, 0, width, height);
    engine->SetCameraState(modelViewMatrix, projMatrix);
    engine->SetRenderViewport(viewport);

    UsdImagingGLRenderParams params;
    params.drawMode = UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH;
    params.enableLighting = false;

    glClearBufferfv(GL_COLOR, 0, clearColor);
    glClearBufferfv(GL_DEPTH, 0, clearDepth);
    engine->Render(stage->GetPseudoRoot(), params);

    drawTarget->Unbind();
    drawTarget->WriteToFile("color", imageFilePath + "_0.png");
    drawTarget->Bind();

    // Unload
    stage->Unload(unloadPath);

    glClearBufferfv(GL_COLOR, 0, clearColor);
    glClearBufferfv(GL_DEPTH, 0, clearDepth);
    engine->Render(stage->GetPseudoRoot(), params);

    drawTarget->Unbind();
    drawTarget->WriteToFile("color", imageFilePath + "_1.png");
    drawTarget->Bind();

    // Load again
    stage->Load(unloadPath);

    glClearBufferfv(GL_COLOR, 0, clearColor);
    glClearBufferfv(GL_DEPTH, 0, clearDepth);
    engine->Render(stage->GetPseudoRoot(), params);

    drawTarget->Unbind();
    drawTarget->WriteToFile("color", imageFilePath + "_2.png");
}
