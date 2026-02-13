//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/textureObjectRegistry.h"
#include "pxr/imaging/hdSt/textureObject.h"
#include "pxr/imaging/hdSt/textureIdentifier.h"
#include "pxr/imaging/hdSt/glslProgram.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/stackTrace.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

class My_TestGLDrawing : public HdSt_UnitTestGLDrawing {
public:
    My_TestGLDrawing() = default;
    
    // HdSt_UnitTestGLDrawing overrides
    void InitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;
    
private:
    std::unique_ptr<HdSt_TextureTestDriver> _driver;
    std::unique_ptr<HdSt_TextureObjectRegistry> _registry;
};

void
My_TestGLDrawing::InitTest()
{
    _driver = std::make_unique<HdSt_TextureTestDriver>();
    _registry =
        std::make_unique<HdSt_TextureObjectRegistry>(
            _driver->GetResourceRegistry().get());
}

void
My_TestGLDrawing::DrawTest()
{
    std::cout << "DrawTest not supported" << std::endl;
    exit(1);
}

void
My_TestGLDrawing::OffscreenTest()
{
    const int width = GetWidth();
    const int height = GetHeight();

    // Make output texture
    HgiTextureDesc texDesc;
    texDesc.debugName = "Output My_TestGLDrawing";
    texDesc.usage = HgiTextureUsageBitsColorTarget;
    texDesc.type = HgiTextureType2D;
    texDesc.dimensions = GfVec3i(GetWidth(), GetHeight(), 1);
    texDesc.layerCount = 1;
    texDesc.format = HgiFormatFloat32Vec4;
    texDesc.mipLevels = 1;
    texDesc.pixelsByteSize = 
        HgiGetDataSize(texDesc.format, texDesc.dimensions);

    // Fill output texture with dark gray
    std::vector<float> initialData;
    initialData.resize(width * height * 4);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            initialData[width * 4 * i + 4 * j + 0] = 0.1;
            initialData[width * 4 * i + 4 * j + 1] = 0.1;
            initialData[width * 4 * i + 4 * j + 2] = 0.1;
            initialData[width * 4 * i + 4 * j + 3] = 1.f;
        }
    }
    texDesc.initialData = initialData.data();

    HgiTextureHandle dstTexture = _driver->GetHgi()->CreateTexture(texDesc);

    // Make sampler object to use with the various input textures.
    HgiSamplerDesc samplerDesc;
    samplerDesc.magFilter = HgiSamplerFilterLinear;
    samplerDesc.minFilter = HgiSamplerFilterLinear;
    samplerDesc.mipFilter = HgiMipFilterLinear;
    HgiSamplerHandle sampler = _driver->GetHgi()->CreateSampler(samplerDesc);

    {
        HdStTextureObjectSharedPtr const texture1111 =
            _registry->AllocateTextureObject(
                HdStTextureIdentifier(TfToken("useFallback1111.png"),
                    VtValue(GfVec4f(1.0))),
                HdStTextureType::Uv);

        {
            HdStUvTextureObject * const uvTextureObject =
            dynamic_cast<HdStUvTextureObject*>(texture1111.get());
            if (!uvTextureObject) {
                std::cout << "Invalid UV texture object" << std::endl;
                exit(EXIT_FAILURE);
            }
            _registry->Commit();
            _driver->Draw(dstTexture, uvTextureObject->GetTexture(), sampler);
            _driver->WriteToFile(dstTexture, "fallback1111.png");
        }

        HdStTextureObjectSharedPtr const textureDefault =
            _registry->AllocateTextureObject(
                HdStTextureIdentifier(TfToken("useFallbackDefault.png"),
                    VtValue()),
                HdStTextureType::Uv);

        {
            HdStUvTextureObject * const uvTextureObject =
            dynamic_cast<HdStUvTextureObject*>(textureDefault.get());
            if (!uvTextureObject) {
                std::cout << "Invalid UV texture object" << std::endl;
                exit(EXIT_FAILURE);
            }
            _registry->Commit();
            _driver->Draw(dstTexture, uvTextureObject->GetTexture(), sampler);
            _driver->WriteToFile(dstTexture, "fallbackDefault.png");
        }

        HdStTextureObjectSharedPtr const texture1001 =
            _registry->AllocateTextureObject(
                HdStTextureIdentifier(TfToken("useFallback1001.png"),
                    VtValue(GfVec4f(1.0, 0.0, 0.0, 1.0))),
                HdStTextureType::Uv);

        {
            HdStUvTextureObject * const uvTextureObject =
                dynamic_cast<HdStUvTextureObject*>(texture1001.get());
            if (!uvTextureObject) {
                std::cout << "Invalid UV texture object" << std::endl;
                exit(EXIT_FAILURE);
            }
            _registry->Commit();
            _driver->Draw(dstTexture, uvTextureObject->GetTexture(), sampler);
            _driver->WriteToFile(dstTexture, "fallback1001.png");
        }

        HdStTextureObjectSharedPtr const texture1101 =
            _registry->AllocateTextureObject(
                HdStTextureIdentifier(TfToken("useFallback1101.png"),
                    VtValue(GfVec4f(1.0, 1.0, 0.0, 1.0))),
                HdStTextureType::Uv);

        {
            HdStUvTextureObject * const uvTextureObject =
                dynamic_cast<HdStUvTextureObject*>(texture1101.get());
            if (!uvTextureObject) {
                std::cout << "Invalid UV texture object" << std::endl;
                exit(EXIT_FAILURE);
            }
            _registry->Commit();
            _driver->Draw(dstTexture, uvTextureObject->GetTexture(), sampler);
            _driver->WriteToFile(dstTexture, "fallback1101.png");
        }

        HdStTextureObjectSharedPtr const textureInvalid =
            _registry->AllocateTextureObject(
                HdStTextureIdentifier(TfToken("useFallbackInvalid.png"),
                    VtValue(TfToken("DELIBERATELY INVALID FALLBACK"))),
                HdStTextureType::Uv);

        {
            HdStUvTextureObject * const uvTextureObject =
            dynamic_cast<HdStUvTextureObject*>(textureInvalid.get());
            if (!uvTextureObject) {
                std::cout << "Invalid UV texture object" << std::endl;
                exit(EXIT_FAILURE);
            }
            _registry->Commit();
            _driver->Draw(dstTexture, uvTextureObject->GetTexture(), sampler);
            _driver->WriteToFile(dstTexture, "fallbackInvalid.png");
        }
    }

    _driver->GetHgi()->DestroyTexture(&dstTexture);
    _driver->GetHgi()->DestroySampler(&sampler);
    
    // Clean-up things.
    _registry->GarbageCollect();
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
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
