//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImagingGL/unitTestGLDrawing.h"

#include "pxr/imaging/glf/simpleLightingContext.h"
#include "pxr/imaging/hdx/pickTask.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usdImaging/usdImagingGL/engine.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

/// For each frame in the Stage's frame range, uses the pick camera
/// for picking and reports the result to stdout.
///
/// The reported results include the detailed instancing information but
/// not geometric information such as the world hit point.
///
class My_TestGLDrawing : public UsdImagingGL_UnitTestGLDrawing
{
public:
    void InitTest() override;
    void DrawTest(bool offscreen) override;
    void ShutdownTest() override;

    UsdImagingGLRenderParams GetRenderParams(UsdTimeCode frame) const;
    void Draw(UsdTimeCode frame, bool render=true);
    void Pick(UsdTimeCode frame);

private:
    UsdStageRefPtr _stage;
    UsdGeomCamera _pickCamera;
    std::unique_ptr<UsdImagingGLEngine> _engine;
};

void
My_TestGLDrawing::InitTest()
{
    _stage = UsdStage::Open(GetStageFilePath());

    if (!_stage) {
        std::cerr << "Couldn't open stage "
                  << GetStageFilePath() << std::endl;
        exit(-1);
    }

    _pickCamera = UsdGeomCamera(
        _stage->GetPrimAtPath(SdfPath(GetPickCameraPath())));

    if (!_pickCamera) {
        std::cerr << "No pick camera at path "
                  << GetPickCameraPath() << std::endl;
        exit(-1);
    }

    _engine = std::make_unique<UsdImagingGLEngine>(
        UsdImagingGLEngine::Parameters{});

    if (!_GetRenderer().IsEmpty()) {
        if (!_engine->SetRendererPlugin(_GetRenderer())) {
            std::cerr << "Couldn't set renderer plugin: " <<
                _GetRenderer().GetText() << std::endl;
            exit(-1);
        }
    }

    _engine->SetSelectionColor(GfVec4f(1, 1, 0, 1));

    _engine->SetCameraPath(SdfPath(GetCameraPath()));
}

void
My_TestGLDrawing::ShutdownTest()
{
    _engine.reset();
}

void
My_TestGLDrawing::DrawTest(bool offscreen)
{
    /// Draw one image for control.
    Draw(UsdTimeCode());

    std::cout << "Picking results for camera "
              << _pickCamera.GetPath() << ":" << std::endl;

    /// Pick for each frame in the frame range.
    const double startTime = _stage->GetStartTimeCode();
    const double diff = _stage->GetEndTimeCode() - startTime;
    if (diff > 0.0) {
        for (int i = 0; i <= diff; i++) {
            Pick(UsdTimeCode(startTime + i));
        }
    } else {
        Pick(UsdTimeCode());
    }
}

UsdImagingGLRenderParams
My_TestGLDrawing::GetRenderParams(UsdTimeCode frame) const
{
    UsdImagingGLRenderParams params;
    params.drawMode = GetDrawMode();
    params.enableLighting =  IsEnabledTestLighting();
    params.complexity = _GetComplexity();
    params.cullStyle = GetCullStyle();
    params.highlight = true;
    params.clearColor = GetClearColor();
    params.clipPlanes = GetClipPlanes();
    params.frame = frame;
    return params;
}

void
My_TestGLDrawing::Draw(const UsdTimeCode frame, const bool render)
{
    _engine->SetRenderViewport(GfVec4d(0, 0, GetWidth(), GetHeight()));
    _engine->SetRendererAov(GetRendererAov());

    if(IsEnabledTestLighting()) {
        GlfSimpleLightingContextRefPtr lightingContext =
            GlfSimpleLightingContext::New();
        lightingContext->SetStateFromOpenGL();
        _engine->SetLightingState(lightingContext);
    }

    if (!render) {
        return;
    }

    do {
        _engine->Render(_stage->GetPseudoRoot(), GetRenderParams(frame));
    } while (!_engine->IsConverged());

    const std::string imageFilePath = GetOutputFilePath();
    if (imageFilePath.empty()) {
        return;
    }

    static int i = 0;

    WriteToFile(
        _engine.get(),
        HdAovTokens->color,
        TfStringReplace(
            imageFilePath,
            ".png", TfStringPrintf("_%03d.png", i)));

    i++;
}

std::string
_ToString(const UsdImagingGLEngine::IntersectionResult &hit)
{
    std::string result;
    result += TfStringPrintf(
        "        Prim at %s\n",
        hit.hitPrimPath.GetText());

    const size_t n = hit.instancerContext.size();
    for (size_t i = 0; i < n; i++) {
        const auto &[instancerPath, instanceIndex] =
            hit.instancerContext[n - i - 1];
        result += TfStringPrintf(
            "%s            which is point-instanced by %s "
            "(instanceIndex = %d)\n",
            std::string(4 * i, ' ').c_str(), instancerPath.GetText(),
            instanceIndex);
    }
    return result;
}

void
My_TestGLDrawing::Pick(const UsdTimeCode frame)
{
    std::cout << "    For frame " << frame << ":" << std::endl;

    const GfFrustum frustum = _pickCamera.GetCamera(frame).GetFrustum();

    UsdImagingGLEngine::PickParams pickParams;
    pickParams.resolveMode = HdxPickResolveModeTokens->resolveDeep;
    
    UsdImagingGLEngine::IntersectionResultVector hits;

    if (_engine->TestIntersection(
            pickParams,
            frustum.ComputeViewMatrix(),
            frustum.ComputeProjectionMatrix(),
            _stage->GetPseudoRoot(),
            GetRenderParams(frame),
            &hits)) {

        /// Sort hits for stability.

        std::vector<std::string> formattedHits;
        for (const UsdImagingGLEngine::IntersectionResult &hit : hits) {
            formattedHits.push_back(_ToString(hit));
        }
        std::sort(formattedHits.begin(), formattedHits.end());

        for (const std::string &formattedHit : formattedHits) {
            std::cout << formattedHit;
        }
        std::cout << std::flush;
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
    TfErrorMark mark;

    BasicTest(argc, argv);

    if (mark.IsClean()) {
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}
