//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

// This test harness is mostly a clone of testHdStBasicDrawing.cpp with fewer
// options and a custom test scene.
//

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hdSt/materialParam.h"
#include "pxr/imaging/hdSt/shaderKey.h"
#include "pxr/imaging/hdSt/textureIdentifier.h"
#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"
#include "pxr/imaging/hgi/texture.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

class My_ShaderKey : public HdSt_ShaderKey {
public:
    TfToken const& GetGlslfxFilename() const override
    {
        static TfToken glslfxFilename("testGlslfxFile.glslfx");
        return glslfxFilename;
    }

    std::string GetGlslfxString() const override
    {
        return "glslfxString1"; // Dummy glslfx string for testing
    }
    HdSt_GeometricShader::PrimitiveType GetPrimitiveType() const override
    {
        return HdSt_GeometricShader::PrimitiveType::PRIM_POINTS;
    }
    HdCullStyle GetCullStyle() const override
    {
        return HdCullStyleDontCare;
    }
    bool UseHardwareFaceCulling() const override
    {
        return false;
    }
    bool HasMirroredTransform() const override
    {
        return false;
    }
    bool IsDoubleSided() const override
    {
        return false;
    }
    bool UseMetalTessellation() const override
    {
        return false;
    }
    HdPolygonMode GetPolygonMode() const override
    {
        return HdPolygonModeFill;
    }
    bool IsFrustumCullingPass() const override
    {
        return false;
    }
    HdSt_GeometricShader::FvarPatchType GetFvarPatchType() const override
    {
        return HdSt_GeometricShader::FvarPatchType::PATCH_COARSE_TRIANGLES;
    }
    float GetLineWidth() const override
    {
        return 1.0f;
    }
};

class My_TestGLDrawing : public HdSt_UnitTestGLDrawing {
public:
    My_TestGLDrawing() {
    }

    // HdSt_UnitTestGLDrawing overrides
    void InitTest() override;
    void DrawTest() override
    {
        BasicTest();
    }
    void OffscreenTest() override
    {
        BasicTest();
    }

private:
    void BasicTest();

    HdSt_TestDriverUniquePtr _driver;
};

void
My_TestGLDrawing::InitTest()
{
    _driver = std::make_unique<HdSt_TestDriver>();
}

void
My_TestGLDrawing::BasicTest()
{
    HdSt_GeometricShaderSharedPtr geometricShader1 =
        std::make_shared<HdSt_GeometricShader>(
            "glslfxString1",                                    //glslfxString
            HdSt_GeometricShader::PrimitiveType::PRIM_POINTS,   // primType
            HdCullStyleDontCare,                                // cullStyle
            false,                                              // useHardwareFaceCulling
            false,                                              // hasMirroredTransform
            false,                                              // doubleSided
            false,                                              // useMetalTessellation
            HdPolygonModeFill,                                  // polygonMode
            false,                                              // cullingPass
            HdSt_GeometricShader::FvarPatchType::PATCH_COARSE_TRIANGLES,  // fvarPatchType
            SdfPath(),
            1.0f);

    HdSt_GeometricShaderSharedPtr geometricShader2 =
        std::make_shared<HdSt_GeometricShader>(
            "glslfxString1",                                    //glslfxString
            HdSt_GeometricShader::PrimitiveType::PRIM_POINTS,   // primType
            HdCullStyleDontCare,                                // cullStyle
            true,                                               // useHardwareFaceCulling
            true,                                               // hasMirroredTransform
            true,                                               // doubleSided
            false,                                              // useMetalTessellation
            HdPolygonModeLine,                                  // polygonMode
            false,                                              // cullingPass
            HdSt_GeometricShader::FvarPatchType::PATCH_COARSE_TRIANGLES,  // fvarPatchType
            SdfPath(),
            3.0f);

    // glslfxString, primType, cullStyle, useMetalTessellation, cullingPass and fvarPatchType
    // will impact the hash.
    TF_AXIOM(geometricShader1->ComputeHash() == geometricShader2->ComputeHash());

    HdSt_GeometricShaderSharedPtr geometricShader3 =
        std::make_shared<HdSt_GeometricShader>(
            "glslfxString2",                                    //glslfxString
            HdSt_GeometricShader::PrimitiveType::PRIM_POINTS,   // primType
            HdCullStyleDontCare,                                // cullStyle
            true,                                               // useHardwareFaceCulling
            true,                                               // hasMirroredTransform
            true,                                               // doubleSided
            false,                                              // useMetalTessellation
            HdPolygonModeLine,                                  // polygonMode
            false,                                              // cullingPass
            HdSt_GeometricShader::FvarPatchType::PATCH_COARSE_TRIANGLES,  // fvarPatchType
            SdfPath(),
            3.0f);
    // glslfxString will impact the hash value.
    TF_AXIOM(geometricShader1->ComputeHash() != geometricShader3->ComputeHash());

    HdSt_GeometricShaderSharedPtr geometricShader4 =
        std::make_shared<HdSt_GeometricShader>(
            "glslfxString1",                                    //glslfxString
            HdSt_GeometricShader::PrimitiveType::PRIM_BASIS_CURVES_LINEAR_PATCHES,   // primType
            HdCullStyleDontCare,                                // cullStyle
            true,                                               // useHardwareFaceCulling
            true,                                               // hasMirroredTransform
            true,                                               // doubleSided
            false,                                              // useMetalTessellation
            HdPolygonModeLine,                                  // polygonMode
            false,                                              // cullingPass
            HdSt_GeometricShader::FvarPatchType::PATCH_COARSE_TRIANGLES,  // fvarPatchType
            SdfPath(),
            3.0f);
    // primType will impact the hash value.
    TF_AXIOM(geometricShader2->ComputeHash() != geometricShader4->ComputeHash());

    HdSt_GeometricShaderSharedPtr geometricShader5 =
        std::make_shared<HdSt_GeometricShader>(
            "glslfxString1",                                    //glslfxString
            HdSt_GeometricShader::PrimitiveType::PRIM_POINTS,   // primType
            HdCullStyleBack,                                    // cullStyle
            false,                                              // useHardwareFaceCulling
            false,                                              // hasMirroredTransform
            false,                                              // doubleSided
            false,                                              // useMetalTessellation
            HdPolygonModeFill,                                  // polygonMode
            false,                                              // cullingPass
            HdSt_GeometricShader::FvarPatchType::PATCH_COARSE_TRIANGLES,  // fvarPatchType
            SdfPath(),
            1.0f);
    // cullStyle will impact the hash value.
    TF_AXIOM(geometricShader1->ComputeHash() != geometricShader5->ComputeHash());

    HdSt_GeometricShaderSharedPtr geometricShader6 =
        std::make_shared<HdSt_GeometricShader>(
            "glslfxString1",                                    //glslfxString
            HdSt_GeometricShader::PrimitiveType::PRIM_POINTS,   // primType
            HdCullStyleDontCare,                                // cullStyle
            false,                                              // useHardwareFaceCulling
            false,                                              // hasMirroredTransform
            false,                                              // doubleSided
            true,                                               // useMetalTessellation
            HdPolygonModeFill,                                  // polygonMode
            false,                                              // cullingPass
            HdSt_GeometricShader::FvarPatchType::PATCH_COARSE_TRIANGLES,  // fvarPatchType
            SdfPath(),
            1.0f);
    // useMetalTessellation will impact the hash value.
    TF_AXIOM(geometricShader1->ComputeHash() != geometricShader6->ComputeHash());

    HdSt_GeometricShaderSharedPtr geometricShader7 =
        std::make_shared<HdSt_GeometricShader>(
            "glslfxString1",                                    //glslfxString
            HdSt_GeometricShader::PrimitiveType::PRIM_POINTS,   // primType
            HdCullStyleDontCare,                                // cullStyle
            false,                                              // useHardwareFaceCulling
            false,                                              // hasMirroredTransform
            false,                                              // doubleSided
            false,                                              // useMetalTessellation
            HdPolygonModeFill,                                  // polygonMode
            true,                                               // cullingPass
            HdSt_GeometricShader::FvarPatchType::PATCH_COARSE_TRIANGLES,  // fvarPatchType
            SdfPath(),
            1.0f);
    // cullingPass will impact the hash value.
    TF_AXIOM(geometricShader1->ComputeHash() != geometricShader7->ComputeHash());

    HdSt_GeometricShaderSharedPtr geometricShader8 =
        std::make_shared<HdSt_GeometricShader>(
            "glslfxString1",                                    //glslfxString
            HdSt_GeometricShader::PrimitiveType::PRIM_POINTS,   // primType
            HdCullStyleDontCare,                                // cullStyle
            false,                                              // useHardwareFaceCulling
            false,                                              // hasMirroredTransform
            false,                                              // doubleSided
            false,                                              // useMetalTessellation
            HdPolygonModeFill,                                  // polygonMode
            false,                                              // cullingPass
            HdSt_GeometricShader::FvarPatchType::PATCH_COARSE_QUADS,  // fvarPatchType
            SdfPath(),
            1.0f);
    // fvarPatchType will impact the hash value.
    TF_AXIOM(geometricShader1->ComputeHash() != geometricShader8->ComputeHash());

    HdStResourceRegistrySharedPtr const& resourceRegistry =
        std::static_pointer_cast<HdStResourceRegistry>(
            _driver->GetDelegate().GetRenderIndex().GetResourceRegistry());
    My_ShaderKey key;
    HdSt_GeometricShaderSharedPtr geometricShader9 =
        HdSt_GeometricShader::Create(key, {}, {}, resourceRegistry);
    TF_AXIOM(geometricShader1->ComputeHash() == geometricShader9->ComputeHash());

    HdSt_MaterialParam param1(HdSt_MaterialParam::ParamTypeFallback, TfToken("Param1"), VtValue());
    HdSt_GeometricShaderSharedPtr geometricShader10 =
        HdSt_GeometricShader::Create(key, {}, { param1 }, resourceRegistry);
    TF_AXIOM(geometricShader9 != geometricShader10);

    int width = 10;
    int height = 10;
    HgiTextureDesc texDesc;
    texDesc.debugName = "textureHandle";
    texDesc.usage = HgiTextureUsageBitsColorTarget;
    texDesc.type = HgiTextureType2D;
    texDesc.dimensions = GfVec3i(width, height, 1);
    texDesc.layerCount = 1;
    texDesc.format = HgiFormatFloat32Vec4;
    texDesc.mipLevels = 1;
    texDesc.pixelsByteSize =
        HgiGetDataSize(texDesc.format, texDesc.dimensions);

    // Fill output texture with dark gray
    std::vector<float> initialData;
    initialData.resize(width* height * 4);
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

    HdStTextureHandleSharedPtr textureHandle = resourceRegistry->AllocateTextureHandle(
        HdStTextureIdentifier(TfToken()),
        HdStTextureType::Uv,
        HdSamplerParameters(
            HdWrapRepeat,
            HdWrapRepeat,
            HdWrapClamp,
            HdMinFilterNearest,
            HdMagFilterNearest),
        /* memoryRequest = */ 2000,
        HdStShaderCodePtr());

    HdStShaderCode::NamedTextureHandleVector textures;
    textures.push_back(
        { TfToken(),
          HdStTextureType::Uv,
          { textureHandle }, 100 });

    HdSt_GeometricShaderSharedPtr geometricShader11 =
        HdSt_GeometricShader::Create(key, textures, { param1 }, resourceRegistry);
    TF_AXIOM(geometricShader10 != geometricShader11);
}

int main(int argc, char *argv[])
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

