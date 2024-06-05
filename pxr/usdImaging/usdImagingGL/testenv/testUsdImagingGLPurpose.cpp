//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/range3d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/getenv.h"

#include "pxr/imaging/garch/glDebugWindow.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/usdImaging/usdImaging/unitTestHelper.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/usdImaging/usdImagingGL/engine.h"
#include "pxr/usdImaging/usdImagingGL/unitTestGLDrawing.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

using UsdImagingGLEngineSharedPtr = std::shared_ptr<class UsdImagingGLEngine>;

int main(int argc, char *argv[])
{
    std::string stageFilePath;
    std::string imageFilePath = "out";

    SdfPath purposePath = SdfPath::AbsoluteRootPath();

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--stage") {
            stageFilePath = argv[++i];
        } else if (arg == "--purpose") {
            purposePath = SdfPath(argv[++i]);
        } else if (arg == "--write") {
            imageFilePath = argv[++i];
        }
    }

    imageFilePath = TfStringReplace(imageFilePath, ".png", "");

    // prepare GL context
    int width = 512, height = 512;
    GarchGLDebugWindow window("UsdImagingGL Test", width, height);
    window.Init();

    // open stage
    UsdStageRefPtr stage = UsdStage::Open(stageFilePath);
    UsdImagingGLEngineSharedPtr engine;
    SdfPathVector excludedPaths;

    engine.reset(
        new UsdImagingGLEngine(stage->GetPseudoRoot().GetPath(), 
                excludedPaths));

    TfTokenVector purposes;
    purposes.push_back(UsdGeomTokens->default_);

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

    GfVec4d viewport(0, 0, width, height);
    engine->SetCameraState(modelViewMatrix, projMatrix);
    engine->SetRenderViewport(viewport);

    UsdImagingGLRenderParams params;
    params.drawMode = UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH;
    params.enableLighting = false;
    params.showGuides = false;
    params.showProxy = false;
    params.showRender = false;
    params.clearColor = GfVec4f{ 0.1f, 0.1f, 0.1f, 1.0f };

    engine->SetRendererAov(HdAovTokens->color);

    engine->Render(stage->GetPseudoRoot(), params);

    UsdImagingGL_UnitTestGLDrawing::WriteAovToFile(
        engine.get(), HdAovTokens->color, imageFilePath + "_0.png");

    // Change purpose.
    UsdGeomImageable prim(stage->GetPrimAtPath(purposePath));
    if (!TF_VERIFY(prim)) {
        return EXIT_FAILURE;
    }
    UsdAttribute purposeAttr = prim.CreatePurposeAttr();
    purposeAttr.Set(UsdGeomTokens->render);

    engine->Render(stage->GetPseudoRoot(), params);

    UsdImagingGL_UnitTestGLDrawing::WriteAovToFile(
        engine.get(), HdAovTokens->color, imageFilePath + "_1.png");

    // Enable "render" purpose.
    params.showRender = true;

    engine->Render(stage->GetPseudoRoot(), params);

    UsdImagingGL_UnitTestGLDrawing::WriteAovToFile(
        engine.get(), HdAovTokens->color, imageFilePath + "_2.png");

    // Revert purpose change & params change.
    params.showRender = false;
    purposeAttr.Set(UsdGeomTokens->default_);

    engine->Render(stage->GetPseudoRoot(), params);

    UsdImagingGL_UnitTestGLDrawing::WriteAovToFile(
        engine.get(), HdAovTokens->color, imageFilePath + "_3.png");

    return EXIT_SUCCESS;
}
