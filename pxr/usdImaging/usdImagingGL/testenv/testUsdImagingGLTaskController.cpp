//
// Copyright 2022 Pixar
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

#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/range3d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"

#include "pxr/imaging/garch/glDebugWindow.h"
#include "pxr/imaging/glf/simpleLightingContext.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/usdImaging/usdImagingGL/engine.h"
#include "pxr/usdImaging/usdImaging/tokens.h"
#include "pxr/usdImaging/usdImaging/unitTestHelper.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

using UsdImagingGLEngineSharedPtr = std::shared_ptr<class UsdImagingGLEngine>;


int main(int argc, char *argv[])
{
    // Parse Arguments
    std::string stageFilePath;
    std::string domeLightTexturePath;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--stage") {
            stageFilePath = argv[++i];
        } else if (arg == "--lightTexture") {
            domeLightTexturePath = argv[++i];
        }
    }
    const int width = 640;
    const int height = 480;

    // prepare GL context
    GarchGLDebugWindow window("UsdImagingGL Test", width, height);
    window.Init();

    // open stage
    UsdStageRefPtr stage = UsdStage::Open(stageFilePath);
    const bool zUpStage = UsdGeomGetStageUpAxis(stage) == UsdGeomTokens->z;


    // Initialize UsdImagingGLEnging
    UsdImagingGLEngineSharedPtr engine;
    SdfPathVector excludedPaths;
    engine.reset(new UsdImagingGLEngine(
        stage->GetPseudoRoot().GetPath(), excludedPaths));

    engine->SetRendererAov(HdAovTokens->color);

    // Extent hints are sometimes authored as an optimization to avoid
    // computing bounds, they are particularly useful for some tests where
    // there is no bound on the first frame.
    bool useExtentHints = true;
    TfTokenVector purposes; 
    purposes.push_back(UsdGeomTokens->default_);
    UsdGeomBBoxCache bboxCache(UsdTimeCode::Default(), purposes, useExtentHints);

    GfBBox3d bbox = bboxCache.ComputeWorldBound(stage->GetPseudoRoot());
    GfRange3d world = bbox.ComputeAlignedRange();

    GfVec3d worldCenter = (world.GetMin() + world.GetMax()) / 2.0;
    double worldSize = world.GetSize().GetLength();

    std::cerr << "worldCenter: " << worldCenter << "\n";
    std::cerr << "worldSize: " << worldSize << "\n";
    GfVec3d translate;
    if (zUpStage) {
        // transpose y and z centering translation
        translate[0] = -worldCenter[0];
        translate[1] = -worldCenter[2];
        translate[2] = -worldCenter[1] - worldSize;
    } else {
        translate[0] = -worldCenter[0];
        translate[1] = -worldCenter[1];
        translate[2] = -worldCenter[2] - worldSize;
    }

    // Camera initialization
    double aspectRatio = double(width)/height;
    GfFrustum frustum;
    frustum.SetPerspective(60.0, aspectRatio, 1, 100000.0);
    GfMatrix4d viewMatrix = GfMatrix4d().SetTranslate(translate);
    GfMatrix4d projMatrix = frustum.ComputeProjectionMatrix();
    GfMatrix4d modelViewMatrix = viewMatrix; 
    if (zUpStage) {
        // rotate from z-up to y-up
        modelViewMatrix = 
            GfMatrix4d().SetRotate(GfRotation(GfVec3d(1.0,0.0,0.0), -90.0)) *
            modelViewMatrix;
    }

    // Ititialize the Lighting Context with a DomeLight
    GlfSimpleLightingContextRefPtr lightingContext(GlfSimpleLightingContext::New());
    GlfSimpleLight light;
    light.SetIsDomeLight(true);
    light.SetDomeLightTextureFile(
        SdfAssetPath(domeLightTexturePath, domeLightTexturePath));
    // DomeLight is yup by default, rotate if the stage is z-up
    if (zUpStage) {
        light.SetTransform(
            GfMatrix4d().SetRotate(GfRotation(GfVec3d(1,0,0), 90.0)));
    }
    light.SetDiffuse(GfVec4f(1,1,1,1));
    light.SetAmbient(GfVec4f(0,0,0,1));
    light.SetSpecular(GfVec4f(1,1,1,1));
    GlfSimpleLightVector lights;
    lights.push_back(light);
    lightingContext->SetLights(lights);

    GlfSimpleMaterial material;
    material.SetAmbient(GfVec4f(0.2, 0.2, 0.2, 1.0));
    material.SetDiffuse(GfVec4f(0.8, 0.8, 0.8, 1.0));
    material.SetSpecular(GfVec4f(0,0,0,1));
    material.SetShininess(0.0001f);
    lightingContext->SetMaterial(material);
    lightingContext->SetSceneAmbient(GfVec4f(0.2,0.2,0.2,1.0));

    // --------------------------------------------------------------------
    // draw.
    GfVec4d viewport(0, 0, width, height);
    engine->SetCameraState(modelViewMatrix, projMatrix);
    engine->SetRenderViewport(viewport);

    // Render #1 - DomeLight created in lightingContext
    UsdImagingGLRenderParams params;
    params.drawMode = UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH;
    params.enableLighting = true;
    params.complexity = 1.3;
    params.clearColor = GfVec4f{ 1.0f, .5f, 0.1f, 1.0f };

    engine->SetLightingState(lightingContext);
    engine->Render(stage->GetPseudoRoot(), params);

    UsdImagingGL_UnitTestGLDrawing::WriteAovToFile(
        engine.get(), HdAovTokens->color, "initialDome.png");

    // Render #2 - Rotated transform on domelight in lightingContext
    GfVec3d upAxis = (zUpStage) ? GfVec3d(0,0,1) : GfVec3d(0,1,0);
    GfMatrix4d rotMatrix = GfMatrix4d().SetRotate(GfRotation(upAxis, 90.0));
    lights.at(0).SetTransform(lights.at(0).GetTransform() * rotMatrix);
    lightingContext->SetLights(lights);
    
    engine->SetLightingState(lightingContext);
    engine->Render(stage->GetPseudoRoot(), params);
    
    UsdImagingGL_UnitTestGLDrawing::WriteAovToFile(
        engine.get(), HdAovTokens->color, "rotatedDome.png");

    return EXIT_SUCCESS;
}
