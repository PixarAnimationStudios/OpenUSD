//
// Copyright 2023 Pixar
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
#include <algorithm>
#include <iostream>
#include <unordered_set>

#include "pxr/imaging/hdst/codeGen.h"
#include "pxr/imaging/hdst/glslProgram.h"
#include "pxr/imaging/hdst/shaderKey.h"
#include "pxr/imaging/hdst/tokens.h"
#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"
#include "pxr/imaging/hgi/hgi.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_ENV_SETTING(HGI_ENABLE_VULKAN, 0,
    "Enable Vulkan as platform default Hgi backend (WIP)");

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((baseGLSLFX, "../testenv/testHdstQualifiers/testQualifiers.glslfx"))

    ((qualifiersVertex, "Qualifiers.Vertex"))
    ((qualifiersFragment, "Qualifiers.Fragment"))
);

// The shaderKey for testQualifiers.glslfx.
struct HdSt_TestQualifiersShaderKey : public HdSt_ShaderKey
{
    HdSt_TestQualifiersShaderKey()
    {
        VS[0] = _tokens->qualifiersVertex;
        VS[1] = TfToken();
        FS[0] = _tokens->qualifiersFragment;
        FS[1] = TfToken();
    }
    ~HdSt_TestQualifiersShaderKey() = default;

    TfToken const& GetGlslfxFilename() const override { return _tokens->baseGLSLFX; }
    TfToken const* GetVS() const override { return VS; }
    TfToken const* GetFS() const override { return FS; }
    HdSt_GeometricShader::PrimitiveType GetPrimitiveType() const override {
        return HdSt_GeometricShader::PrimitiveType::PRIM_MESH_COARSE_TRIANGLES;
    }

private:
    TfToken VS[2];
    TfToken FS[2];
};

// Custom CodeGen which can verify if the qualifiers are correctly generated.
class My_CodeGen : public HdSt_CodeGen
{
public:
    My_CodeGen(HdSt_GeometricShaderPtr const& geometricShader,
        HdStShaderCodeSharedPtrVector const& shaders,
        TfToken const& materialTag) 
        : HdSt_CodeGen(geometricShader, shaders, materialTag)
    {}

    ~My_CodeGen() = default;

    bool VerifyQualifiers(TfToken const& shaderStage, std::string const& member,
        std::string const& qualifier, bool qualifierBeforeMember, int offsetOfQualifier)
    {
        // Get the shader source.
        std::string shaderSource;
        if (shaderStage == HdShaderTokens->vertexShader)
        {
            shaderSource = GetVertexShaderSource();
        }
        else if (shaderStage == HdShaderTokens->fragmentShader)
        {
            shaderSource = GetFragmentShaderSource();
        }
        else if (shaderStage == HdShaderTokens->tessControlShader)
        {
            shaderSource = GetTessControlShaderSource();
        }
        else if (shaderStage == HdShaderTokens->tessEvalShader)
        {
            shaderSource = GetTessEvalShaderSource();
        }
        else if (shaderStage == HdShaderTokens->geometryShader)
        {
            shaderSource = GetGeometryShaderSource();
        }
        else if (shaderStage == HdShaderTokens->computeShader)
        {
            shaderSource = GetComputeShaderSource();
        }
        else if (shaderStage == HdShaderTokens->postTessControlShader)
        {
            shaderSource = GetPostTessControlShaderSource();
        }

        // Find the member that should have qualifiers.
        size_t index = shaderSource.find(member, 0);
        if (index < qualifier.length() + 1)
            return false;
        else
        {
            if (qualifierBeforeMember)
            {
                // Check if the correct qualifier is before the member.
                // In GLSL, the syntax for a qualifier is like "flat float dataFlat,".
                // So the position for the qualifier is the start of the member type 
                // minus the length of the qualifier and offset. Here, offsetQualifer 
                // should be -1 for the space before the member.
                // First find the index of the start of member type.
                index = shaderSource.find_last_not_of(' ', index - 1);
                index = shaderSource.find_last_of(' ', index) + 1;
                if (shaderSource.compare(index - qualifier.length() + offsetOfQualifier,
                    qualifier.length(), qualifier) == 0)
                    return true;
                else
                    return false;
            }
            else
            {
                // Check if the correct qualifier is after the member.
                // In MSL, the syntax for a qualifier is like "float dataFlat [[flat]];".
                // So the position for the qualifier is the start of the member plus the
                // length of the member and offset. Here, offsetQualifer should be 3 for
                // " [[" after the member.
                if (shaderSource.compare(index + member.length() + offsetOfQualifier,
                    qualifier.length(), qualifier) == 0)
                    return true;
                else
                    return false;
            }
        }
    }
};

// Custom Drawing class.
class My_TestGLDrawing : public HdSt_UnitTestGLDrawing {
public:
    My_TestGLDrawing() : _testResult(false) {}

    // HdSt_UnitTestGLDrawing overrides
    void InitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;
    void Present(uint32_t framebuffer) override;

    bool TestResult() const { return _testResult; }

    struct MemberWithQualifiers
    {
        std::string _memberName;
        std::string _qualifiers;
    };

protected:
    void ParseArgs(int argc, char* argv[]) override;

private:
    bool _testResult;
    HdSt_TestDriver* _driver;
    std::string _outputFilePath;
    std::vector<MemberWithQualifiers> _testMembers;
};

////////////////////////////////////////////////////////////

void
My_TestGLDrawing::InitTest()
{
    std::cout << "My_TestGLDrawing::InitTest() " << "\n";

    _driver = new HdSt_TestDriver();
    GfVec3f center(0);

    // center camera
    SetCameraTranslate(GetCameraTranslate() - center);

    _driver->SetClearColor(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
    _driver->SetClearDepth(1.0f);
    _driver->SetupAovs(GetWidth(), GetHeight());
}

// Get the language specified qualifier string.
std::string _GetInterpolationString(const TfToken& API, const std::string& qualifiers)
{
    if (API == HgiTokens->OpenGL)
    {
        return qualifiers;
    }
    else if (API == HgiTokens->Metal)
    {
        if (qualifiers == "flat") {
            return qualifiers;
        }
        else if (qualifiers == "noperspective") {
            return "center_no_perspective";
        }
        else if (qualifiers == "centroid") {
            return "centroid_perspective";
        }
        else if (qualifiers == "sample") {
            return "sample_perspective";
        }
        else {
            return "";
        }
    }
    else {
        return "";
    }
}


void
My_TestGLDrawing::DrawTest()
{
    HdRenderIndex& renderIndex = _driver->GetDelegate().GetRenderIndex();
    HdStResourceRegistrySharedPtr const& registryPtr =
        std::static_pointer_cast<HdStResourceRegistry>(renderIndex.GetResourceRegistry());

    HdSt_TestQualifiersShaderKey shaderKey;
    HdStShaderCodeSharedPtrVector shaders;

    // Create the geometric shader.
    HdInstance<HdSt_GeometricShaderSharedPtr> geometricShaderInstance =
        registryPtr->RegisterGeometricShader(shaderKey.ComputeHash());

    if (geometricShaderInstance.IsFirstInstance()) {
        geometricShaderInstance.SetValue(
            std::make_shared<HdSt_GeometricShader>(
                shaderKey.GetGlslfxString(),
                shaderKey.GetPrimitiveType(),
                shaderKey.GetCullStyle(),
                shaderKey.UseHardwareFaceCulling(),
                shaderKey.HasMirroredTransform(),
                shaderKey.IsDoubleSided(),
                shaderKey.UseMetalTessellation(),
                shaderKey.GetPolygonMode(),
                shaderKey.IsFrustumCullingPass(),
                shaderKey.GetFvarPatchType(),
                /*debugId=*/SdfPath(),
                shaderKey.GetLineWidth()));
    }
    HdSt_GeometricShaderSharedPtr geometricShader = geometricShaderInstance.GetValue();

    // Initialize the codeGen.
    My_CodeGen codeGen(geometricShader, shaders, HdStMaterialTagTokens->defaultMaterialTag);

    // Resolve bindings.
    HdSt_ResourceBinder::MetaData::DrawingCoordBufferBinding _drawingCoordBufferBinding;
    HdStBindingRequestVector customBindings;
    HdRprimSharedData sharedData(1);
    std::unique_ptr<HdStDrawItem> drawItem =
        std::make_unique<HdStDrawItem>(&sharedData);
    HdSt_ResourceBinder resourceBinder;
    // let resourcebinder resolve bindings and populate metadata
    // which is owned by codegen.
    resourceBinder.ResolveBindings(drawItem.get(),
        shaders,
        codeGen.GetMetaData(),
        _drawingCoordBufferBinding,
        false,
        customBindings,
        registryPtr->GetHgi()->GetCapabilities());

    // Compile the program.
    HdStGLSLProgramSharedPtr glslProgram = codeGen.Compile(registryPtr.get());

    // If we fail to compile or link the program, return failure.
    if (!glslProgram || !glslProgram->Link()) {
        // Failed to compile and link a valid glsl program.
        return;
    }

    Hgi* hgi = registryPtr->GetHgi();
    bool qualifierBeforeMember = true;
    int offset = 0;
    // In GLSL, the syntax for a qualifier is like "flat float dataFlat,". So qualifier is before
    // the member and the extra offset is 1 for the white space.
    if (hgi->GetAPIName() == HgiTokens->OpenGL)
        offset = -1;
    // In MSL, the syntax for a qualifier is like "float dataFlat [[flat]];". So qualifier is after
    // the member and the extra offset is 3 for " [[".
    else if (hgi->GetAPIName() == HgiTokens->Metal)
    {
        qualifierBeforeMember = false;
        offset = 3;
    }

    // Verify if the qualifiers are correctly added before the member.
    for (auto member : _testMembers)
    {
        if (!codeGen.VerifyQualifiers(HdShaderTokens->vertexShader, member._memberName, 
            _GetInterpolationString(hgi->GetAPIName(), member._qualifiers),
            qualifierBeforeMember, offset))
            return;
        if (!codeGen.VerifyQualifiers(HdShaderTokens->fragmentShader, member._memberName, 
            _GetInterpolationString(hgi->GetAPIName(), member._qualifiers),
            qualifierBeforeMember, offset))
            return;
    }

    _driver->Draw();
    _testResult = true;
}

void
My_TestGLDrawing::OffscreenTest()
{
    DrawTest();

    if (!_outputFilePath.empty()) {
        _driver->WriteToFile("color", _outputFilePath);
    }
}

void
My_TestGLDrawing::Present(uint32_t framebuffer)
{
    _driver->Present(GetWidth(), GetHeight(), framebuffer);
}

/* virtual */
void
My_TestGLDrawing::ParseArgs(int argc, char* argv[])
{
    for (int i = 0; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--write") {
            _outputFilePath = argv[++i];
        }
        else if (arg == "--flatQF") {
            MemberWithQualifiers member;
            member._memberName = argv[++i];
            member._qualifiers = "flat";
            _testMembers.emplace_back(member);
        }
        else if (arg == "--nopersQF") {
            MemberWithQualifiers member;
            member._memberName = argv[++i];
            member._qualifiers = "noperspective";
            _testMembers.emplace_back(member);
        }
        else if (arg == "--centroidQF") {
            MemberWithQualifiers member;
            member._memberName = argv[++i];
            member._qualifiers = "centroid";
            _testMembers.emplace_back(member);
        }
        else if (arg == "--sampleQF") {
            MemberWithQualifiers member;
            member._memberName = argv[++i];
            member._qualifiers = "sample";
            _testMembers.emplace_back(member);
        }
    }
}

bool BasicTest(int argc, char* argv[])
{
    My_TestGLDrawing driver;
    driver.RunTest(argc, argv);
    return driver.TestResult();
}


int main(int argc, char* argv[])
{
    if (BasicTest(argc, argv))
    {
        std::cout << "OK" << std::endl;
            return EXIT_SUCCESS;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
            return EXIT_FAILURE;
    }
}
