//
// Copyrighty 2020 Pixar
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
#include "pxr/pxr.h"

#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/textureObjectRegistry.h"
#include "pxr/imaging/hdSt/textureObject.h"
#include "pxr/imaging/hdSt/samplerObjectRegistry.h"
#include "pxr/imaging/hdSt/samplerObject.h"
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

    std::unique_ptr<HdStResourceRegistry> _hdStRegistry;
    std::unique_ptr<HdSt_TextureObjectRegistry> _textureRegistry;
    std::unique_ptr<HdSt_SamplerObjectRegistry> _samplerRegistry;
};

void
My_TestGLDrawing::InitTest()
{
    _driver = std::make_unique<HdSt_TextureTestDriver>();
    _hdStRegistry = std::make_unique<HdStResourceRegistry>(_driver->GetHgi());
    _textureRegistry =
        std::make_unique<HdSt_TextureObjectRegistry>(_hdStRegistry.get());
    _samplerRegistry =
        std::make_unique<HdSt_SamplerObjectRegistry>(_hdStRegistry.get());
    
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

    HdStTextureObjectSharedPtr const texture =
        _textureRegistry->AllocateTextureObject(
            HdStTextureIdentifier(TfToken("texture.png")),
            HdTextureType::Uv);

    // Call Commit on both registries to ensure that the shared HgiBlitCmds
    // is submitted in the resource registry.
    _textureRegistry->Commit();

    {
        HdStSamplerObjectSharedPtr const sampler1 =
            _samplerRegistry->AllocateSampler(
                texture,
                HdSamplerParameters(
                    HdWrapRepeat,
                    HdWrapMirror,
                    HdWrapClamp,
                    HdMinFilterNearest,
                    HdMagFilterNearest));
        
        HdStUvTextureObject * const uvTextureObject =
            dynamic_cast<HdStUvTextureObject*>(texture.get());
        if (!uvTextureObject) {
            std::cout << "Invalid UV texture object" << std::endl;
            exit(EXIT_FAILURE);
        }

        HdStUvSamplerObject * const uvSamplerObject =
            dynamic_cast<HdStUvSamplerObject*>(sampler1.get());
        if (!uvSamplerObject) {
            std::cout << "Invalid UV sampler object" << std::endl;
            exit(EXIT_FAILURE);
        }

        _driver->Draw(dstTexture, uvTextureObject->GetTexture(), 
            uvSamplerObject->GetSampler());
        _driver->WriteToFile(dstTexture, "outSampler1.png");
    }
    _samplerRegistry->MarkGarbageCollectionNeeded();

    // Should garbage collect above sampler handle.
    _samplerRegistry->GarbageCollect();

    // Ensure Hgi's internal garbage collector runs to destroy gpu resources.
    _driver->GetHgi()->EndFrame();

    _driver->GetHgi()->StartFrame();
    HdStSamplerObjectSharedPtr const sampler2 =
        _samplerRegistry->AllocateSampler(
            texture,
            HdSamplerParameters(
                HdWrapRepeat,
                HdWrapClamp,
                HdWrapMirror,
                HdMinFilterLinearMipmapLinear,
                HdMagFilterNearest));
    {
        HdStUvTextureObject * const uvTextureObject =
            dynamic_cast<HdStUvTextureObject*>(texture.get());
        if (!uvTextureObject) {
            std::cout << "Invalid UV texture object" << std::endl;
            exit(EXIT_FAILURE);
        }

        HdStUvSamplerObject * const uvSamplerObject =
            dynamic_cast<HdStUvSamplerObject*>(sampler2.get());
        if (!uvSamplerObject) {
            std::cout << "Invalid UV sampler object" << std::endl;
            exit(EXIT_FAILURE);
        }

        _driver->Draw(dstTexture, uvTextureObject->GetTexture(), 
            uvSamplerObject->GetSampler());
        _driver->WriteToFile(dstTexture, "outSampler2.png");
    }

    // Use a high res texture to see whether mipmaps are generated and used.
    HdStTextureObjectSharedPtr const hiResTexture =
        _textureRegistry->AllocateTextureObject(
            HdStTextureIdentifier(TfToken("hiResTexture.png")),
            HdTextureType::Uv);

    // Call Commit on both registries to ensure that the shared HgiBlitCmds
    // is submitted in the resource registry.
    _textureRegistry->Commit();

    HdStSamplerObjectSharedPtr const hiResSampler =
        _samplerRegistry->AllocateSampler(
            texture,
            HdSamplerParameters(
                HdWrapRepeat,
                HdWrapClamp,
                HdWrapMirror,
                HdMinFilterLinearMipmapLinear,
                HdMagFilterNearest));
    
    {
        HdStUvTextureObject * const uvTextureObject =
            dynamic_cast<HdStUvTextureObject*>(hiResTexture.get());
        if (!uvTextureObject) {
            std::cout << "Invalid UV texture object" << std::endl;
            exit(EXIT_FAILURE);
        }

        HdStUvSamplerObject * const uvSamplerObject =
            dynamic_cast<HdStUvSamplerObject*>(hiResSampler.get());
        if (!uvSamplerObject) {
            std::cout << "Invalid UV sampler object" << std::endl;
            exit(EXIT_FAILURE);
        }

        _driver->Draw(dstTexture, uvTextureObject->GetTexture(), 
            uvSamplerObject->GetSampler());
        _driver->WriteToFile(dstTexture, "outHiResSampler.png");
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
