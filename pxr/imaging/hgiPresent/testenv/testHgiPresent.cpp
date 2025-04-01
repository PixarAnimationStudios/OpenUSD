//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiPresent/present.h"
#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/imaging/glf/simpleLightingContext.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/usdImaging/usdImagingGL/engine.h"

#include "testenv/testWindow.h"

#include <iostream>
#include <sstream>

PXR_NAMESPACE_USING_DIRECTIVE

static void
_WriteCaptureToFile(
    const HioImage::StorageSpec &storage, std::string const &fileName)
{
    HioImageSharedPtr const image = HioImage::OpenForWriting(fileName);
    bool const writeSuccess = image && image->Write(storage);
    if (!writeSuccess) {
        TF_RUNTIME_ERROR("Failed to write image to %s", fileName.c_str());
    }
}

struct _Capture
{
    uint32_t start{};
    uint32_t stop{};
    uint32_t skip{};
};

struct _Args
{
    std::string windowApi;
    std::string filePath;
    _Capture capture;
};

static void
_ParseArgs(int argc, char **argv, _Args &args)
{
    // If unspecified, run forever without capturing.
    args.capture = {0, std::numeric_limits<decltype(_Capture::stop)>::max(), 0};

    const auto checkForMissingArgument = [argc, argv](int i, int n = 1) {
        if (i + n >= argc) {
            std::cout << TfGetBaseName(argv[0]) << ": "
                      << "missing parameter(s) for '" << argv[i] << "'"
                      << std::endl;
            return false;
        }

        return true;
    };

    for (int i = 1; i != argc; ++i) {
        if (strcmp(argv[i], "-") == 0) {
            std::cout
                << TfGetBaseName(argv[0]) << ": "
                << "-window apiName -stage filePath -capture start stop skip\n"
                << "\nhgiPresent tests\n\n"
                << "-window apiName           name of the window API to use\n"
                << "-stage filePath           name of the USD stage to open\n"
                << "-capture start stop skip  capture frame between start and"
                << " stop (exclusive), skipping the given number of frame in"
                << " between captures\n"
                << std::endl;
            exit(0);
        } else if (strcmp(argv[i], "-window") == 0) {
            if (!checkForMissingArgument(i)) {
                exit(1);
            }
            args.windowApi = argv[++i];
        } else if (strcmp(argv[i], "-stage") == 0) {
            if (!checkForMissingArgument(i)) {
                exit(1);
            }
            args.filePath = argv[++i];
        } else if (strcmp(argv[i], "-capture") == 0) {
            if (!checkForMissingArgument(i, 3)) {
                exit(1);
            }
            try {
                args.capture.start = std::stoul(argv[++i]);
                args.capture.stop = std::stoul(argv[++i]);
                args.capture.skip = std::stoul(argv[++i]);
            } catch (const std::exception &) {
                std::cout << TfGetBaseName(argv[0]) << ": "
                          << "not a 32bit unsigned integer '" << argv[i] << "'"
                          << std::endl;
                exit(1);
            }
        } else {
            std::cout << TfGetBaseName(argv[0]) << ": "
                      << "unknown argument '" << argv[i] << "'" << std::endl;
            exit(1);
        }
    }

    if (args.filePath.empty()) {
        std::cout << TfGetBaseName(argv[0]) << ": "
                  << "missing argument '-stage'" << std::endl;
        exit(1);
    }

    if (args.windowApi.empty()) {
#if defined(ARCH_OS_DARWIN)
        args.windowApi = "Metal";
#elif defined(ARCH_OS_LINUX)
        args.windowApi = "X11";
#elif defined(ARCH_OS_WINDOWS)
        args.windowApi = "WIN32";
#endif
    }
}

class My_TestGLDrawing
{
public:
    void RunTest(int argc, char **argv);
    void InitTest(const std::string &windowApi, const std::string &filePath);
    void DrawTest(const _Capture &capture);

private:
    std::unique_ptr<HgiPresentTestWindow> _window;
    UsdStageRefPtr _stage;
    std::unique_ptr<UsdImagingGLEngine> _engine;
    GlfSimpleLightingContextRefPtr _lightingContext;

    GfVec2i _windowSize{640, 480};
    std::array<float, 3> _translate{};
};

void
My_TestGLDrawing::RunTest(int argc, char **argv)
{
    _Args args;
    _ParseArgs(argc, argv, args);

    InitTest(args.windowApi, args.filePath);
    DrawTest(args.capture);
}

void
My_TestGLDrawing::InitTest(
    const std::string &windowApi, const std::string &filePath)
{
    _window = HgiPresentTestWindow::Create(windowApi, _windowSize);
    if (!_window) {
        TF_RUNTIME_ERROR("Failed to create window");
        return;
    }

    _stage = UsdStage::Open(filePath);

    UsdImagingGLEngine::Parameters parameters;
    parameters.rootPath = _stage->GetPseudoRoot().GetPath();

    _engine = std::make_unique<UsdImagingGLEngine>(parameters);

    // Extent hints are sometimes authored as an optimization to avoid
    // computing bounds, they are particularly useful for some tests where
    // there is no bound on the first frame.
    const TfTokenVector purposes{UsdGeomTokens->default_};
    UsdGeomBBoxCache bboxCache(UsdTimeCode::Default(), purposes, true);

    GfBBox3d bbox = bboxCache.ComputeWorldBound(_stage->GetPseudoRoot());
    GfRange3d world = bbox.ComputeAlignedRange();

    GfVec3d worldCenter = (world.GetMin() + world.GetMax()) / 2.0;
    double worldSize = world.GetSize().GetLength();

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

    _lightingContext = GlfSimpleLightingContext::New();
    GlfSimpleLight light;
    light.SetPosition(GfVec4f(0, -.5, .5, 0));
    light.SetDiffuse(GfVec4f(1, 1, 1, 1));
    light.SetAmbient(GfVec4f(0, 0, 0, 1));
    light.SetSpecular(GfVec4f(1, 1, 1, 1));
    GlfSimpleLightVector lights;
    lights.push_back(light);
    _lightingContext->SetLights(lights);

    GlfSimpleMaterial material;
    material.SetAmbient(GfVec4f(0.2, 0.2, 0.2, 1.0));
    material.SetDiffuse(GfVec4f(0.8, 0.8, 0.8, 1.0));
    material.SetSpecular(GfVec4f(0, 0, 0, 1));
    material.SetShininess(0.0001f);
    _lightingContext->SetMaterial(material);
    _lightingContext->SetSceneAmbient(GfVec4f(0.2, 0.2, 0.2, 1.0));
}

void
My_TestGLDrawing::DrawTest(const _Capture &capture)
{
    if (!_window) {
        return;
    }

    TfStopwatch renderTime;

    UsdImagingGLRenderParams params;
    params.drawMode = UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH;
    params.enableLighting = true;
    params.enableSceneMaterials = true;
    params.complexity = 1;
    params.cullStyle =
        UsdImagingGLCullStyle::CULL_STYLE_BACK_UNLESS_DOUBLE_SIDED;
    params.colorCorrectionMode = HdxColorCorrectionTokens->sRGB;

    _engine->SetRendererAov(HdAovTokens->color);
    _engine->SetLightingState(_lightingContext);

    // Even though we're frame rate independent, and it would be nice to render
    // as fast as possible to end the test quickly, not vsync-ing causes
    // reliability issues due to screen tearing and partial frames.
    static constexpr auto vsync = true;
    _engine->EnableWindowPresentation(_window->GetHandle(), vsync);

    params.frame = UsdTimeCode::Default();

    // Add some warmup frames before the test starts, to avoid a rare issue
    // where frame 0 is captured blank.
    static constexpr int32_t firstFrame = -4;
    const auto lastFrame = static_cast<int32_t>(capture.stop);
    for (int32_t frame = firstFrame; frame < lastFrame; frame++) {
        if (!_window->Update()) {
            break;
        }

        // Assume a constant 60fps so the test aren't frame rate dependent.
        const double t = frame / 60.;

        // Create an oscillating motion
        double rotateY{}, rotateX{};
        ArchSinCos(t * 2 * M_PI, &rotateY, &rotateX);
        rotateY *= 15;
        rotateX *= 15;

        const double aspectRatio =
            static_cast<double>(_windowSize[0]) / _windowSize[1];
        GfFrustum frustum;
        frustum.SetPerspective(60.0, aspectRatio, 1, 100000.0);
        const GfMatrix4d projMatrix = frustum.ComputeProjectionMatrix();

        GfVec4d viewport = {0, 0, static_cast<double>(_windowSize[0]),
            static_cast<double>(_windowSize[1])};
        _engine->SetRenderViewport(viewport);

        GfMatrix4d viewMatrix(1.0);
        viewMatrix *=
            GfMatrix4d().SetRotate(GfRotation(GfVec3d(0, 1, 0), rotateY));
        viewMatrix *=
            GfMatrix4d().SetRotate(GfRotation(GfVec3d(1, 0, 0), rotateX));
        viewMatrix *= GfMatrix4d().SetTranslate(
            GfVec3d(_translate[0], _translate[1], _translate[2]));
        GfMatrix4d modelViewMatrix = viewMatrix;
        if (UsdGeomGetStageUpAxis(_stage) == UsdGeomTokens->z) {
            // rotate from z-up to y-up
            modelViewMatrix = GfMatrix4d().SetRotate(
                                  GfRotation(GfVec3d(1.0, 0.0, 0.0), -90.0)) *
                modelViewMatrix;
        }
        _engine->SetCameraState(modelViewMatrix, projMatrix);

        renderTime.Start();
        do {
            _engine->Render(_stage->GetPseudoRoot(), params);
        } while (!_engine->IsConverged());
        renderTime.Stop();

        // Don't capture if skip == 0. Ignore warmup frames.
        if (capture.skip != 0 && frame >= 0) {
            const auto captureFrame = static_cast<uint32_t>(frame);
            if (captureFrame >= capture.start &&
                (captureFrame - capture.start) % capture.skip == 0) {
                HioImage::StorageSpec storage{};
                std::vector<uint8_t> buffer;
                if (_window->CaptureImage(storage, buffer)) {
                    std::stringstream fileName;
                    fileName << "testHgiPresent_frame" <<
                        captureFrame << ".png";
                    _WriteCaptureToFile(storage, fileName.str());
                }
            }
        }
    }
}

int
main(int argc, char **argv)
{
    TfErrorMark mark;

    My_TestGLDrawing driver;
    driver.RunTest(argc, argv);

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
