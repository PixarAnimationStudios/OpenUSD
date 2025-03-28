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
    TfErrorMark mark;

    // prepare GL context
    int width = 512, height = 512;
    GarchGLDebugWindow window("UsdImagingGL Test", width, height);
    window.Init();

    // open stage
    UsdStageRefPtr stage = UsdStage::Open("root.usda");
    TF_AXIOM(stage);

    SdfLayerHandle rootLayer = stage->GetRootLayer();

    UsdImagingGLEngineSharedPtr engine;
    SdfPathVector excludedPaths;

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
    double worldSize = world.GetSize().GetLength();

    GfVec3d translate = 
        {-worldCenter[0], -worldCenter[1], -worldCenter[2] - worldSize};

    double aspectRatio = double(width)/height;
    GfFrustum frustum;
    frustum.SetPerspective(60.0, aspectRatio, 1, 100000.0);
    GfMatrix4d viewMatrix = GfMatrix4d().SetTranslate(translate);
    GfMatrix4d projMatrix = frustum.ComputeProjectionMatrix();
    GfMatrix4d modelViewMatrix = viewMatrix;

    // --------------------------------------------------------------------
    // draw.
    GfVec4d viewport(0, 0, width, height);
    engine->SetCameraState(modelViewMatrix, projMatrix);
    engine->SetRenderViewport(viewport);

    engine->SetRendererAov(HdAovTokens->color);

    UsdImagingGLRenderParams params;
    params.drawMode = UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH;
    params.enableLighting = false;
    params.clearColor = GfVec4f{ 0.1f, 0.1f, 0.1f, 1.0f };

    // baseline image
    std::cout << "Render initial image" << std::endl;
    engine->Render(stage->GetPseudoRoot(), params);
    UsdImagingGL_UnitTestGLDrawing::WriteAovToFile(
        engine.get(), HdAovTokens->color, "initial.png");

    // sublayer operation: mute layer with attribute over
    std::cout << "mute: sublayer_over.usda" << std::endl;
    stage->MuteLayer("./sublayer_over.usda");
    engine->Render(stage->GetPseudoRoot(), params);
    UsdImagingGL_UnitTestGLDrawing::WriteAovToFile(
        engine.get(), HdAovTokens->color, "mute_attr.png");

    // sublayer operation: remove layer with sublayer
    std::cout << "remove sublayer: remove_sublayer_with_sub.usda" << std::endl;
    rootLayer->RemoveSubLayerPath(rootLayer->GetNumSubLayerPaths() - 1);
    engine->Render(stage->GetPseudoRoot(), params);
    UsdImagingGL_UnitTestGLDrawing::WriteAovToFile(
        engine.get(), HdAovTokens->color, "remove_sublayer_with_sub.png");
    

    // sublayer operation: unmute layer with attribute over
    std::cout << "unmute: sublayer_over.usda" << std::endl;
    stage->UnmuteLayer("./sublayer_over.usda");
    engine->Render(stage->GetPseudoRoot(), params);
    UsdImagingGL_UnitTestGLDrawing::WriteAovToFile(
        engine.get(), HdAovTokens->color, "unmute_attr.png");

    // sublayer operation: add sublayer with sublayer
    std::cout << "add sublayer: sublayer_with_sub.usda" << std::endl;
    rootLayer->GetSubLayerPaths().push_back("./sublayer_with_sub.usda");
    engine->Render(stage->GetPseudoRoot(), params);
    UsdImagingGL_UnitTestGLDrawing::WriteAovToFile(
        engine.get(), HdAovTokens->color, "add_sublayer_with_sub.png");
    
    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
