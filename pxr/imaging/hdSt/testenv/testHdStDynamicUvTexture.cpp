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
#include "pxr/imaging/hdSt/textureIdentifier.h"
#include "pxr/imaging/hdSt/textureCpuData.h"
#include "pxr/imaging/hdSt/dynamicUvTextureImplementation.h"
#include "pxr/imaging/hdSt/dynamicUvTextureObject.h"
#include "pxr/imaging/hdSt/subtextureIdentifier.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/stackTrace.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

class My_TestGLDrawing : public HdSt_UnitTestGLDrawing
{
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

// A class to store CPU data between loading the texture data
// and uploading them to the GPU.
//
// This example is generating a red-green gradient with a fixed
// blue value.
//
class My_TextureCpuData : public HdStTextureCpuData
{
public:
    My_TextureCpuData(unsigned char blue) {
        
        // Data are stored and owned by a std::vector in this
        // example.
        _data.resize(256 * 256 * 4);
        for (size_t i = 0; i < 256; i++) {
            for (size_t j = 0; j < 256; j++) {
                _data[256 * 4 * i + 4 * j + 0] = i;
                _data[256 * 4 * i + 4 * j + 1] = j;
                _data[256 * 4 * i + 4 * j + 2] = blue;
                _data[256 * 4 * i + 4 * j + 3] = 255;
            }
        }

        _desc.usage = HgiTextureUsageBitsShaderRead;
        _desc.format = HgiFormatUNorm8Vec4;
        _desc.type = HgiTextureType2D;
        _desc.dimensions = GfVec3i(256, 256, 1);
        _desc.pixelsByteSize = _data.size();

        // Pointer to the above data is in the texture descriptor.
        _desc.initialData = _data.data();
    }

    ~My_TextureCpuData() override = default;

    // Descriptor used to upload data to GPU.
    const HgiTextureDesc &GetTextureDesc() const override { return _desc; }
    bool GetGenerateMipmaps() const override { return false; }
    bool IsValid() const override { return true; }

private:
    // The descriptor containing a pointer to the data.
    HgiTextureDesc _desc;
    // Owns the texture data.
    std::vector<unsigned char> _data;
};

// Our own subtexture identifier that simply contains one value for the
// blue component.
//
// It is supposed to be light-weight containing just enough information
// so that the texture can be loaded in the texture implementation.
// 
class My_SubtextureIdentifier : public HdStDynamicUvSubtextureIdentifier
{
public:
    My_SubtextureIdentifier(unsigned char blue) : _blue(blue) { }
    
    std::unique_ptr<HdStSubtextureIdentifier> Clone() const override {
        return std::make_unique<My_SubtextureIdentifier>(_blue);
    }

    // The data of the subtexture identifier.
    unsigned char GetBlue() const { return _blue; }

    // What implements the loading of the texture identified by this
    // subtexture identifier.
    HdStDynamicUvTextureImplementation *GetTextureImplementation() const override;

protected:
    // Hash.
    ID _Hash() const override { return _blue; }

private:
    unsigned char _blue;
};

// Implements loading a texture identified by our subtexture identifier.

class My_DynamicUvTextureImplementation
    : public HdStDynamicUvTextureImplementation
{
public:
    void Load(HdStDynamicUvTextureObject *texture) override {
        // Get the subtexture identifier.
        const My_SubtextureIdentifier * const subId =
            dynamic_cast<const My_SubtextureIdentifier *>(
                texture->GetTextureIdentifier().GetSubtextureIdentifier());
        if (!TF_VERIFY(subId)) {
            return;
        }

        // Ignore file name of texture identifier, just use data
        // from subidentifier.
        const unsigned char blue = subId->GetBlue();

        // Allocate CPU data.
        texture->SetCpuData(std::make_unique<My_TextureCpuData>(blue));
    }

    void Commit(HdStDynamicUvTextureObject *texture) override {
        // Destroy old GPU texture.
        texture->DestroyTexture();

        // Upload CPU data to GPU.
        if (HdStTextureCpuData * const cpuData = texture->GetCpuData()) {
            if (cpuData->IsValid()) {
                texture->CreateTexture(cpuData->GetTextureDesc());
                if (cpuData->GetGenerateMipmaps()) {
                    texture->GenerateMipmaps();
                }
            }
        }
        
        // Delete CPU data.
        texture->SetCpuData(nullptr);
    }

    bool IsValid(const HdStDynamicUvTextureObject *texture) {
        return bool(texture->GetTexture());
    }
};

// Use implementation as singleton.
HdStDynamicUvTextureImplementation *
My_SubtextureIdentifier::GetTextureImplementation() const
{
    static My_DynamicUvTextureImplementation impl;
    return &impl;
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

    HdStShaderCodeSharedPtr const shader =
        std::make_shared<HdSt_MaterialNetworkShader>();

    // Create texture handle using our own subtexture identifier.
    // Low blue component.
    //
    HdStTextureHandleSharedPtr const textureHandle1 =
        _textureHandleRegistry->AllocateTextureHandle(
            HdStTextureIdentifier(
                TfToken(),
                std::make_unique<My_SubtextureIdentifier>(90)),
            HdStTextureType::Uv,
            HdSamplerParameters(
                HdWrapRepeat,
                HdWrapRepeat,
                HdWrapClamp,
                HdMinFilterNearest,
                HdMagFilterNearest),
            /* memoryRequest = */ 2000,
            shader);

    // High blue component.
    HdStTextureHandleSharedPtr const textureHandle2 =
        _textureHandleRegistry->AllocateTextureHandle(
            HdStTextureIdentifier(
                TfToken(),
                std::make_unique<My_SubtextureIdentifier>(230)),
            HdStTextureType::Uv,
            HdSamplerParameters(
                HdWrapRepeat,
                HdWrapRepeat,
                HdWrapClamp,
                HdMinFilterNearest,
                HdMagFilterNearest),
            /* memoryRequest = */ 2000,
            shader);
    
    // Shader needs to be updated since the texture handle
    // was committed for the first time.
    _CheckEqual(_textureHandleRegistry->Commit(), {shader},
                "Expected shader1 from first commit");

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
        _driver->WriteToFile(dstTexture, "outTextureDarkBlue.png");
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
        _driver->WriteToFile(dstTexture, "outTextureLightBlue.png");
    }

    _driver->GetHgi()->DestroyTexture(&dstTexture);
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
