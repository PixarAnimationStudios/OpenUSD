//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/imaging/hdSt/materialNetworkShader.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/textureObject.h"
#include "pxr/imaging/hdSt/samplerObject.h"
#include "pxr/imaging/hdSt/textureHandle.h"
#include "pxr/imaging/hdSt/textureHandleRegistry.h"
#include "pxr/imaging/hdSt/textureObjectRegistry.h"
#include "pxr/imaging/hdSt/textureIdentifier.h"
#include "pxr/imaging/hdSt/glslProgram.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/stackTrace.h"

#include <iostream>
#include <fstream>

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

    std::unique_ptr<HdStResourceRegistry> _hdStRegistry;
    std::unique_ptr<HdSt_TextureHandleRegistry> _textureHandleRegistry;
};

void
My_TestGLDrawing::InitTest()
{
    _driver = std::make_unique<HdSt_TextureTestDriver>();
    _hdStRegistry = std::make_unique<HdStResourceRegistry>(_driver->GetHgi());
    _textureHandleRegistry =
        std::make_unique<HdSt_TextureHandleRegistry>(_hdStRegistry.get());
    
}

template<typename T>
void _CheckEqual(const T &a, const T &b, const char * msg)
{
    if (a != b) {
        std::cout << msg << std::endl;
        exit(EXIT_FAILURE);
    }
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
    _driver->GetHgi()->StartFrame();

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
    
    HdStShaderCodeSharedPtr const shader1 =
        std::make_shared<HdSt_MaterialNetworkShader>();
    {
        // Basic test, create a handle and drop it.
        HdStTextureHandleSharedPtr const textureHandle =
            _textureHandleRegistry->AllocateTextureHandle(
                HdStTextureIdentifier(TfToken("texture.png")),
                HdStTextureType::Uv,
                HdSamplerParameters(
                    HdWrapRepeat,
                    HdWrapMirror,
                    HdWrapClamp,
                    HdMinFilterLinearMipmapLinear,
                    HdMagFilterNearest),
                /* memoryRequest = */ 2000,
                shader1);

        // Shader1 needs to be updated since the texture handle
        // was committed for the first time.
        _CheckEqual(_textureHandleRegistry->Commit(), {shader1},
                    "Expected shader1 from first commit");

        HdStUvTextureObject * const uvTextureObject =
            dynamic_cast<HdStUvTextureObject*>(
                textureHandle->GetTextureObject().get());
        if (!uvTextureObject) {
            std::cout << "Invalid UV texture object" << std::endl;
            exit(EXIT_FAILURE);
        }

        HdStUvSamplerObject * const uvSamplerObject =
            dynamic_cast<HdStUvSamplerObject*>(
                textureHandle->GetSamplerObject().get());
        if (!uvSamplerObject) {
            std::cout << "Invalid UV sampler object" << std::endl;
            exit(EXIT_FAILURE);
        }

        _driver->Draw(dstTexture, uvTextureObject->GetTexture(), 
            uvSamplerObject->GetSampler());
        _driver->WriteToFile(dstTexture, "outTextureBasic.png");
    }
    
    // Texture was dropped, check that shader gets notified.
    _CheckEqual(_textureHandleRegistry->Commit(), {shader1},
                "Expected shader1 from commit after texture was dropped");

    // Ensure Hgi's internal garbage collector runs to destroy gpu resources.
    _driver->GetHgi()->EndFrame();

    // Calling commit again should do nothing
    _CheckEqual(_textureHandleRegistry->Commit(), {},
                "Expected no shaders");

    _driver->GetHgi()->StartFrame();

    {
        // Allocated two textures to the same handle.

        // Start with one.
        HdStTextureHandleSharedPtr const textureHandle1 =
            _textureHandleRegistry->AllocateTextureHandle(
                HdStTextureIdentifier(TfToken("texture.png")),
                HdStTextureType::Uv,
                HdSamplerParameters(
                    HdWrapRepeat,
                    HdWrapMirror,
                    HdWrapClamp,
                    HdMinFilterLinearMipmapLinear,
                    HdMagFilterNearest),
                /* memoryRequest = */ 100,
                shader1);

        _CheckEqual(_textureHandleRegistry->Commit(), {shader1},
                    "Expected shader1 from re-commit");

        // Check that target memory was correctly computed.
        _CheckEqual(
            textureHandle1->GetTextureObject()->GetTargetMemory(), size_t(100),
            "Expected target memory 100");

        // Let's draw it.
        {
            HdStUvTextureObject * const uvTextureObject =
                dynamic_cast<HdStUvTextureObject*>(
                    textureHandle1->GetTextureObject().get());
            if (!uvTextureObject) {
                std::cout << "Invalid UV texture object" << std::endl;
                exit(EXIT_FAILURE);
            }

            HdStUvSamplerObject * const uvSamplerObject =
                dynamic_cast<HdStUvSamplerObject*>(
                    textureHandle1->GetSamplerObject().get());
            if (!uvSamplerObject) {
                std::cout << "Invalid UV sampler object" << std::endl;
                exit(EXIT_FAILURE);
            }

            _driver->Draw(dstTexture, uvTextureObject->GetTexture(), 
                uvSamplerObject->GetSampler());
            _driver->WriteToFile(dstTexture, "outTextureSampler1LowRes.png");
        }

        {
            // Now allocate a second shader requesting a second
            // texture handle to the same texture.
            HdStShaderCodeSharedPtr const shader2 =
                std::make_shared<HdSt_MaterialNetworkShader>();

            HdStTextureHandleSharedPtr const textureHandle2 =
                _textureHandleRegistry->AllocateTextureHandle(
                    HdStTextureIdentifier(TfToken("texture.png")),
                    HdStTextureType::Uv,
                    HdSamplerParameters(
                        HdWrapRepeat,
                        HdWrapRepeat,
                        HdWrapClamp,
                        HdMinFilterLinearMipmapLinear,
                        HdMagFilterNearest),
                    /* memoryRequest = */ 10000,
                    shader2);

            // The target memory changed and thus the underlying texture.
            // Both shaders need to be updated.
            _CheckEqual(_textureHandleRegistry->Commit(), {shader1, shader2},
                        "Expected shader1 and shader2 from re-commit");

            // The underlying texture should be de-duplicated
            _CheckEqual(textureHandle1->GetTextureObject().get(),
                        textureHandle2->GetTextureObject().get(),
                        "Texture object not deduplicated.");

            // The target memory should be max of the above two requests.
            _CheckEqual(
                textureHandle1->GetTextureObject()->GetTargetMemory(),
                size_t(10000),
                "Expected target memory 10000");

            // Redraw handle1, we should get a new texture sampler handle
            // to the texture which has now been loaded at a much better
            // resolution.
            {
                HdStUvTextureObject * const uvTextureObject =
                    dynamic_cast<HdStUvTextureObject*>(
                        textureHandle1->GetTextureObject().get());
                if (!uvTextureObject) {
                    std::cout << "Invalid UV texture object" << std::endl;
                    exit(EXIT_FAILURE);
                }

                HdStUvSamplerObject * const uvSamplerObject =
                    dynamic_cast<HdStUvSamplerObject*>(
                        textureHandle1->GetSamplerObject().get());
                if (!uvSamplerObject) {
                    std::cout << "Invalid UV sampler object" << std::endl;
                    exit(EXIT_FAILURE);
                }

                _driver->Draw(dstTexture, uvTextureObject->GetTexture(), 
                    uvSamplerObject->GetSampler());
                _driver->WriteToFile(dstTexture, "outTextureSampler1.png");
            }

            {
                HdStUvTextureObject * const uvTextureObject =
                    dynamic_cast<HdStUvTextureObject*>(
                        textureHandle2->GetTextureObject().get());
                if (!uvTextureObject) {
                    std::cout << "Invalid UV texture object" << std::endl;
                    exit(EXIT_FAILURE);
                }

                HdStUvSamplerObject * const uvSamplerObject =
                    dynamic_cast<HdStUvSamplerObject*>(
                        textureHandle2->GetSamplerObject().get());
                if (!uvSamplerObject) {
                    std::cout << "Invalid UV sampler object" << std::endl;
                    exit(EXIT_FAILURE);
                }

                _driver->Draw(dstTexture, uvTextureObject->GetTexture(), 
                    uvSamplerObject->GetSampler());
                _driver->WriteToFile(dstTexture, "outTextureSampler2.png");
            }
        }

        // Target memory changed, all shaders need to be updated,
        // except that shader2 no longer exists.
        _CheckEqual(_textureHandleRegistry->Commit(), {shader1},
                    "Expected shader1 after shader2 was dropped");

        // Target memory should go back.
        _CheckEqual(
            textureHandle1->GetTextureObject()->GetTargetMemory(), size_t(100),
            "Expected target memory to be back at 100");
    }

    _CheckEqual(_textureHandleRegistry->Commit(), {shader1},
                "Expected shader1 after it dropped texture");

    {
        // Test reloading
        HdStShaderCodeSharedPtr const shader =
            std::make_shared<HdSt_MaterialNetworkShader>();

        HdStTextureHandleSharedPtr const textureHandle =
            _textureHandleRegistry->AllocateTextureHandle(
                HdStTextureIdentifier(TfToken("reloadingTexture.png")),
                HdStTextureType::Uv,
                HdSamplerParameters(
                    HdWrapRepeat,
                    HdWrapMirror,
                    HdWrapClamp,
                    HdMinFilterLinearMipmapLinear,
                    HdMagFilterLinear),
                /* memoryRequest = */ 0,
                shader);

        _CheckEqual(_textureHandleRegistry->Commit(), {shader},
                    "Exepected shader from commit");

        
        {
            HdStUvTextureObject * const uvTextureObject =
                dynamic_cast<HdStUvTextureObject*>(
                    textureHandle->GetTextureObject().get());
            if (!uvTextureObject) {
                std::cout << "Invalid UV texture object" << std::endl;
                exit(EXIT_FAILURE);
            }

            HdStUvSamplerObject * const uvSamplerObject =
                dynamic_cast<HdStUvSamplerObject*>(
                    textureHandle->GetSamplerObject().get());
            if (!uvSamplerObject) {
                std::cout << "Invalid UV sampler object" << std::endl;
                exit(EXIT_FAILURE);
            }

            _driver->Draw(dstTexture, uvTextureObject->GetTexture(), 
                uvSamplerObject->GetSampler());
            _driver->WriteToFile(dstTexture, "outTextureBeforeFileChange.png");
        }
                
        {
            TfDeleteFile("reloadingTexture.png");
            std::ifstream src("reloadingTexture2.png");
            std::ofstream dst("reloadingTexture.png");
            
            dst << src.rdbuf();
        }

        _CheckEqual(_textureHandleRegistry->Commit(), {},
                    "Expected no commits before reloading");

        {
            HdStUvTextureObject * const uvTextureObject =
                dynamic_cast<HdStUvTextureObject*>(
                    textureHandle->GetTextureObject().get());
            if (!uvTextureObject) {
                std::cout << "Invalid UV texture object" << std::endl;
                exit(EXIT_FAILURE);
            }

            HdStUvSamplerObject * const uvSamplerObject =
                dynamic_cast<HdStUvSamplerObject*>(
                    textureHandle->GetSamplerObject().get());
            if (!uvSamplerObject) {
                std::cout << "Invalid UV sampler object" << std::endl;
                exit(EXIT_FAILURE);
            }

            _driver->Draw(dstTexture, uvTextureObject->GetTexture(), 
                uvSamplerObject->GetSampler());
            _driver->WriteToFile(dstTexture, "outTextureAfterFileChange.png");
        }
        
        HdSt_TextureObjectRegistry * const reg = 
            _textureHandleRegistry->GetTextureObjectRegistry();
        reg->MarkTextureFilePathDirty(TfToken("reloadingTexture.png"));

        _CheckEqual(_textureHandleRegistry->Commit(), {shader},
                    "Exepected shader from commit after reloading");

        {
            HdStUvTextureObject * const uvTextureObject =
                dynamic_cast<HdStUvTextureObject*>(
                    textureHandle->GetTextureObject().get());
            if (!uvTextureObject) {
                std::cout << "Invalid UV texture object" << std::endl;
                exit(EXIT_FAILURE);
            }

            HdStUvSamplerObject * const uvSamplerObject =
                dynamic_cast<HdStUvSamplerObject*>(
                    textureHandle->GetSamplerObject().get());
            if (!uvSamplerObject) {
                std::cout << "Invalid UV sampler object" << std::endl;
                exit(EXIT_FAILURE);
            }

            _driver->Draw(dstTexture, uvTextureObject->GetTexture(), 
                uvSamplerObject->GetSampler());
            _driver->WriteToFile(dstTexture, "outTextureAfterReload.png");
        }
    }

    {
        // Test reloading
        HdStShaderCodeSharedPtr const shader =
            std::make_shared<HdSt_MaterialNetworkShader>();

        HdStTextureHandleSharedPtr const textureHandle =
            _textureHandleRegistry->AllocateTextureHandle(
                HdStTextureIdentifier(TfToken("texture.png")),
                HdStTextureType::Uv,
                HdSamplerParameters(
                    HdWrapRepeat,
                    HdWrapMirror,
                    HdWrapClamp,
                    HdMinFilterLinearMipmapLinear,
                    HdMagFilterNearest),
                /* memoryRequest = */ 0,
                shader);

        _textureHandleRegistry->SetMemoryRequestForTextureType(
            HdStTextureType::Uv, 3000);

        _CheckEqual(_textureHandleRegistry->Commit(), {shader},
                    "Exepected shader from commit with low global "
                    "memory request");
        
        {
            HdStUvTextureObject * const uvTextureObject =
                dynamic_cast<HdStUvTextureObject*>(
                    textureHandle->GetTextureObject().get());
            if (!uvTextureObject) {
                std::cout << "Invalid UV texture object" << std::endl;
                exit(EXIT_FAILURE);
            }

            HdStUvSamplerObject * const uvSamplerObject =
                dynamic_cast<HdStUvSamplerObject*>(
                    textureHandle->GetSamplerObject().get());
            if (!uvSamplerObject) {
                std::cout << "Invalid UV sampler object" << std::endl;
                exit(EXIT_FAILURE);
            }

            _driver->Draw(dstTexture, uvTextureObject->GetTexture(), 
                uvSamplerObject->GetSampler());
            _driver->WriteToFile(dstTexture, "outTextureWithLowGlobalMemoryRequest.png");
        }
        
        _textureHandleRegistry->SetMemoryRequestForTextureType(
            HdStTextureType::Uv, 15000);

        _CheckEqual(_textureHandleRegistry->Commit(), {shader},
                    "Exepected shader from commit with high global "
                    "memory request");

        {
            HdStUvTextureObject * const uvTextureObject =
                dynamic_cast<HdStUvTextureObject*>(
                    textureHandle->GetTextureObject().get());
            if (!uvTextureObject) {
                std::cout << "Invalid UV texture object" << std::endl;
                exit(EXIT_FAILURE);
            }

            HdStUvSamplerObject * const uvSamplerObject =
                dynamic_cast<HdStUvSamplerObject*>(
                    textureHandle->GetSamplerObject().get());
            if (!uvSamplerObject) {
                std::cout << "Invalid UV sampler object" << std::endl;
                exit(EXIT_FAILURE);
            }

            _driver->Draw(dstTexture, uvTextureObject->GetTexture(), 
                uvSamplerObject->GetSampler());
            _driver->WriteToFile(dstTexture, "outTextureWithHighGlobalMemoryRequest.png");
        } 
    }
    
    _driver->GetHgi()->DestroyTexture(&dstTexture);

    _driver->GetHgi()->EndFrame();
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
