//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hd/unitTestDelegate.h"
#include "pxr/imaging/hdSt/renderBuffer.h"
#include "pxr/imaging/hdSt/renderPass.h"
#include "pxr/imaging/hdSt/textureUtils.h"
#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/imaging/hio/image.h"
#include "pxr/imaging/hio/types.h"

#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/scoped.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

// Two cubes are drawn in two passes:
// - Small white cube, with depth disabled. Uses the --writeOp value
//   for the stencil buffer.
// - Large blue cube with depth enabled. Uses the --readCompare and --readRef
//   values for the stencil buffer.

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

struct StencilState
{
    bool stencilEnabled = true;
    int clearStencil = 0;
    HdStencilOp writeOp = HdStencilOpKeep;
    int writeRef = 0;
    HdCompareFunction readCmp = HdCmpFuncAlways;
    int readRef = 0;
};

bool
_ParseStencilOp(std::string const& s, HdStencilOp* op)
{
    if (s == "keep") {
        *op = HdStencilOpKeep;
        return true;
    }
    if (s == "zero") {
        *op = HdStencilOpZero;
        return true;
    }
    if (s == "replace") {
        *op = HdStencilOpReplace;
        return true;
    }
    if (s == "increment") {
        *op = HdStencilOpIncrement;
        return true;
    }
    if (s == "incrementWrap") {
        *op = HdStencilOpIncrementWrap;
        return true;
    }
    if (s == "decrement") {
        *op = HdStencilOpDecrement;
        return true;
    }
    if (s == "decrementWrap") {
        *op = HdStencilOpDecrementWrap;
        return true;
    }
    if (s == "invert") {
        *op = HdStencilOpInvert;
        return true;
    }
    return false;
}

bool
_ParseCompareFn(std::string const& s, HdCompareFunction* fn)
{
    if (s == "never") {
        *fn = HdCmpFuncNever;
        return true;
    }
    if (s == "less") {
        *fn = HdCmpFuncLess;
        return true;
    }
    if (s == "equal") {
        *fn = HdCmpFuncEqual;
        return true;
    }
    if (s == "lEqual") {
        *fn = HdCmpFuncLEqual;
        return true;
    }
    if (s == "greater") {
        *fn = HdCmpFuncGreater;
        return true;
    }
    if (s == "notEqual") {
        *fn = HdCmpFuncNotEqual;
        return true;
    }
    if (s == "gEqual") {
        *fn = HdCmpFuncGEqual;
        return true;
    }
    if (s == "always") {
        *fn = HdCmpFuncAlways;
        return true;
    }
    return false;
}

} // anonymous namespace

class HdSt_StencilTestDriver : public HdSt_TestDriverBase<HdUnitTestDelegate>
{
public:
    HdSt_StencilTestDriver()
    {
        _renderPassStates = {std::dynamic_pointer_cast<HdStRenderPassState>(
                                 _GetRenderDelegate()->CreateRenderPassState()),
            std::dynamic_pointer_cast<HdStRenderPassState>(
                _GetRenderDelegate()->CreateRenderPassState())};

        for (auto& rps : _renderPassStates) {
            rps->SetDepthFunc(HdCmpFuncLess);
        }
        // Writer pass: cull back faces so each pixel only gets one stencil
        // write per draw.
        _renderPassStates[0]->SetCullStyle(HdCullStyleBack);
        // Writer pass should not modify the depth buffer, we only want to test
        // the stencil buffer.
        _renderPassStates[0]->SetEnableDepthMask(false);

        _Init();

        const HdRprimCollection collections[] = {
            HdRprimCollection(HdTokens->geometry,
                HdReprSelector(HdReprTokens->smoothHull), SdfPath("/inner")),
            HdRprimCollection(HdTokens->geometry,
                HdReprSelector(HdReprTokens->smoothHull), SdfPath("/outer")),
        };
        for (auto const& c : collections) {
            _renderPasses.push_back(std::make_shared<HdSt_RenderPass>(
                &GetDelegate().GetRenderIndex(), c));
        }
    }

    // Create depth-stencil Aovs instead of just depth.
    void SetupStencilAovs(int width, int height)
    {
        for (auto const& id : _aovBufferIds) {
            GetDelegate().GetRenderIndex().RemoveBprim(
                HdPrimTypeTokens->renderBuffer, id);
        }
        _aovBufferIds.clear();
        _aovBindings.clear();

        const std::array<TfToken, 2> aovOutputs = {
            HdAovTokens->color, HdAovTokens->depthStencil};
        _aovBindings.resize(aovOutputs.size());
        const GfVec3i dim(width, height, 1);

        for (size_t i = 0; i < aovOutputs.size(); ++i) {
            SdfPath aovId =
                SdfPath("/testDriver")
                    .AppendChild(TfToken("aov_" +
                        TfMakeValidIdentifier(aovOutputs[i].GetString())));
            _aovBufferIds.push_back(aovId);

            HdAovDescriptor desc =
                _GetRenderDelegate()->GetDefaultAovDescriptor(aovOutputs[i]);

            HdRenderBufferDescriptor rbDesc{dim, desc.format, false};
            GetDelegate().AddRenderBuffer(aovId, rbDesc);

            HdRenderPassAovBinding& binding = _aovBindings[i];
            binding.aovName = aovOutputs[i];
            binding.aovSettings = desc.aovSettings;
            binding.renderBufferId = aovId;
            binding.renderBuffer = dynamic_cast<HdRenderBuffer*>(
                GetDelegate().GetRenderIndex().GetBprim(
                    HdPrimTypeTokens->renderBuffer, aovId));
            if (aovOutputs[i] == HdAovTokens->color) {
                binding.clearValue = VtValue(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
            } else {
                binding.clearValue = VtValue(HdDepthStencilType(1.0f, 0));
            }
        }

        for (auto& state : _renderPassStates) {
            state->SetAovBindings(_aovBindings);
        }
    }

    bool WriteStencilToFile(const std::string& filename, int width, int height)
    {
        const SdfPath aovId =
            SdfPath("/testDriver")
                .AppendChild(TfToken("aov_" +
                    TfMakeValidIdentifier(
                        HdAovTokens->depthStencil.GetString())));
        HdRenderBuffer* const renderBuffer = dynamic_cast<HdRenderBuffer*>(
            GetDelegate().GetRenderIndex().GetBprim(
                HdPrimTypeTokens->renderBuffer, aovId));
        if (!renderBuffer) {
            TF_CODING_ERROR(
                "No depthStencil render buffer at %s", aovId.GetText());
            return false;
        }

        HdStRenderBuffer* const stBuffer =
            static_cast<HdStRenderBuffer*>(renderBuffer);
        VtValue aov = stBuffer->GetResource(false);
        if (!aov.IsHolding<HgiTextureHandle>()) {
            TF_CODING_ERROR("No Hgi texture for depthStencil aov");
            return false;
        }
        HgiTextureHandle const texture = aov.UncheckedGet<HgiTextureHandle>();

        size_t bufferSize = 0;
        HdStTextureUtils::AlignedBuffer<uint8_t> buffer =
            HdStTextureUtils::HgiStencilReadback(
                GetHgi(), texture, &bufferSize);
        if (!buffer.get() || bufferSize == 0) {
            TF_RUNTIME_ERROR("Stencil readback failed (size=%zu)", bufferSize);
            return false;
        }

        HioImage::StorageSpec storage;
        storage.width = width;
        storage.height = height;
        storage.format = HioFormatUNorm8;
        storage.flipped = true;
        storage.data = buffer.get();

        HioImageSharedPtr image = HioImage::OpenForWriting(filename);
        if (!image || !image->Write(storage)) {
            TF_RUNTIME_ERROR(
                "Failed to write stencil image to %s", filename.c_str());
            return false;
        }
        return true;
    }

    void Draw(StencilState const& cfg)
    {
        // Update the stencil clear value, and only clear in the first pass.
        auto aovsFirst = _renderPassStates[0]->GetAovBindings();
        for (auto& aov : aovsFirst) {
            if (aov.aovName == HdAovTokens->depthStencil) {
                aov.clearValue = VtValue(HdDepthStencilType(
                    1.0f, static_cast<uint32_t>(cfg.clearStencil)));
            }
        }
        _renderPassStates[0]->SetAovBindings(aovsFirst);

        auto aovsSecond = _renderPassStates[1]->GetAovBindings();
        for (auto& aov : aovsSecond) {
            aov.clearValue = VtValue{};
        }
        _renderPassStates[1]->SetAovBindings(aovsSecond);

        _renderPassStates[0]->SetStencilEnabled(cfg.stencilEnabled);
        _renderPassStates[0]->SetStencil(HdCmpFuncAlways, cfg.writeRef, 0xff,
            HdStencilOpKeep, HdStencilOpKeep, cfg.writeOp);

        _renderPassStates[1]->SetStencilEnabled(cfg.stencilEnabled);
        _renderPassStates[1]->SetStencil(cfg.readCmp, cfg.readRef, 0xff,
            HdStencilOpKeep, HdStencilOpKeep, HdStencilOpKeep);

        HdTaskSharedPtrVector tasks;
        for (size_t i = 0; i < _renderPassStates.size(); ++i) {
            tasks.push_back(std::make_shared<HdSt_DrawTask>(_renderPasses[i],
                _renderPassStates[i],
                TfTokenVector{HdRenderTagTokens->geometry}));
        }
        _GetEngine()->Execute(&GetDelegate().GetRenderIndex(), &tasks);
    }
};

class My_TestGLDrawing : public HdSt_UnitTestGLDrawing
{
public:
    My_TestGLDrawing()
    {
        SetCameraRotate(0.0f, 0.0f);
        SetCameraTranslate(GfVec3f(0, 0, -20.0f));
    }

    void InitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;
    void Present(uint32_t framebuffer) override;

protected:
    void ParseArgs(int argc, char* argv[]) override;
    void _Draw();

private:
    std::unique_ptr<HdSt_StencilTestDriver> _driver;
    StencilState _config;
    std::string _outputFilePath;
    std::string _stencilOutputFilePath;
};

void
My_TestGLDrawing::InitTest()
{
    std::cout << "My_TestGLDrawing::InitTest()" << std::endl;

    _driver = std::make_unique<HdSt_StencilTestDriver>();
    _driver->SetClearColor(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
    _driver->SetClearDepth(1.0f);
    _driver->SetupStencilAovs(GetWidth(), GetHeight());

    HdUnitTestDelegate& delegate = _driver->GetDelegate();

    const GfRotation rot{GfVec3f(1, 1, 0), 30};

    // Inner cube closer
    const GfMatrix4f innerXf = GfMatrix4f(1.0f).SetScale(1.5f) *
        GfMatrix4f(1.0f).SetRotate(rot) *
        GfMatrix4f(1.0f).SetTranslate(GfVec3f(0, -7, 0));
    const SdfPath inner{"/inner/cube"};
    delegate.AddCube(
        inner, innerXf, false, SdfPath(), PxOsdOpenSubdivTokens->none);
    delegate.UpdatePrimvarValue(
        inner, HdTokens->displayColor, VtValue(GfVec3f(1, 1, 1)));

    // Outer cube farther
    const GfMatrix4f outerXf = GfMatrix4f(1.0f).SetScale(4.0f) *
        GfMatrix4f(1.0f).SetRotate(rot) *
        GfMatrix4f(1.0f).SetTranslate(GfVec3f(0, 7, 0));
    const SdfPath outer{"/outer/cube"};
    delegate.AddCube(
        outer, outerXf, false, SdfPath(), PxOsdOpenSubdivTokens->none);
    delegate.UpdatePrimvarValue(
        outer, HdTokens->displayColor, VtValue(GfVec3f(0.2f, 0.4f, 1.0f)));
}

void
My_TestGLDrawing::_Draw()
{
    int width = GetWidth(), height = GetHeight();
    _driver->SetCamera(GetViewMatrix(), GetProjectionMatrix(),
        CameraUtilFraming(GfRect2i(GfVec2i(0, 0), width, height)));
    _driver->UpdateAovDimensions(width, height);
    _driver->Draw(_config);
}

void
My_TestGLDrawing::DrawTest()
{
    _Draw();
}

void
My_TestGLDrawing::OffscreenTest()
{
    _Draw();
    if (!_outputFilePath.empty()) {
        _driver->WriteToFile("color", _outputFilePath);
    }
    if (!_stencilOutputFilePath.empty()) {
        _driver->WriteStencilToFile(
            _stencilOutputFilePath, GetWidth(), GetHeight());
    }
}

void
My_TestGLDrawing::Present(uint32_t framebuffer)
{
    _driver->Present(GetWidth(), GetHeight(), framebuffer);
}

void
My_TestGLDrawing::ParseArgs(int argc, char* argv[])
{
    for (int i = 0; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--noStencil") {
            _config.stencilEnabled = false;
        } else if (arg == "--clearStencil" && i + 1 < argc) {
            _config.clearStencil = atoi(argv[++i]);
        } else if (arg == "--writeOp" && i + 1 < argc) {
            if (!_ParseStencilOp(argv[++i], &_config.writeOp)) {
                std::cerr << "Unknown stencil op: " << argv[i] << std::endl;
                std::exit(EXIT_FAILURE);
            }
        } else if (arg == "--writeRef" && i + 1 < argc) {
            _config.writeRef = atoi(argv[++i]);
        } else if (arg == "--readCompare" && i + 1 < argc) {
            if (!_ParseCompareFn(argv[++i], &_config.readCmp)) {
                std::cerr << "Unknown compare fn: " << argv[i] << std::endl;
                std::exit(EXIT_FAILURE);
            }
        } else if (arg == "--readRef" && i + 1 < argc) {
            _config.readRef = atoi(argv[++i]);
        } else if (arg == "--write" && i + 1 < argc) {
            _outputFilePath = argv[++i];
        } else if (arg == "--writeStencil" && i + 1 < argc) {
            _stencilOutputFilePath = argv[++i];
        }
    }
}

void
BasicTest(int argc, char* argv[])
{
    My_TestGLDrawing driver;
    driver.RunTest(argc, argv);
}

int
main(int argc, char* argv[])
{
    TfErrorMark mark;

    BasicTest(argc, argv);

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    }
    std::cout << "FAILED" << std::endl;
    return EXIT_FAILURE;
}
