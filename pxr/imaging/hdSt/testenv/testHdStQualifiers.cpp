//
// Copyright 2024 Pixar
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

#include "pxr/imaging/hdSt/codeGen.h"
#include "pxr/imaging/hdSt/drawItem.h"
#include "pxr/imaging/hdSt/glslProgram.h"
#include "pxr/imaging/hdSt/meshShaderKey.h"
#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/shaderKey.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hdSt/pointsShaderKey.h"
#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/imaging/hio/glslfx.h"

#include "pxr/imaging/hd/rprimSharedData.h"
#include "pxr/imaging/hd/tokens.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((glslfxFilename, "testQualifiers.glslfx"))
    ((qualifiersVertex, "Qualifiers.Vertex"))
    ((qualifiersFragment, "Qualifiers.Fragment"))
    ((defaultToken, "default"))
);

class My_Shader : public HdStShaderCode
{
public:
    My_Shader(const std::string& filePath, const TfToken& technique)
        : HdStShaderCode()
        , _glslfx(std::make_unique<HioGlslfx>(filePath, technique))
    {}

    size_t ComputeHash() const override
    {
        return _glslfx->GetHash();
    }

    std::string GetSource(TfToken const &shaderStageKey) const override
    {
        return _glslfx->GetSource(shaderStageKey);
    }

    void BindResources(int program, HdSt_ResourceBinder const &binder) override
    {}

    void UnbindResources(int program, 
        HdSt_ResourceBinder const &binder) override
    {}

    void AddBindings(HdStBindingRequestVector* customBindings) override
    {}

private:
    const HioGlslfx * _GetGlslfx() const override
    {
        return _glslfx.get();
    }

    std::unique_ptr<HioGlslfx> _glslfx;
};

////////////////////////////////////////////////////////////

// Custom Drawing class.
class My_TestGLDrawing : public HdSt_UnitTestGLDrawing {
public:
    My_TestGLDrawing() : _testResult(false) {}

    // HdSt_UnitTestGLDrawing overrides
    void InitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;
    void Present(uint32_t framebuffer) override;

    bool GetTestResult() const { return _testResult; }

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

void
My_TestGLDrawing::InitTest()
{
    _driver = new HdSt_TestDriver();
    GfVec3f center(0);

    // center camera
    SetCameraTranslate(GetCameraTranslate() - center);

    _driver->SetClearColor(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
    _driver->SetClearDepth(1.0f);
    _driver->SetupAovs(GetWidth(), GetHeight());
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
        } else if (arg == "--flatQF") {
            MemberWithQualifiers member;
            member._memberName = argv[++i];
            member._qualifiers = "flat";
            _testMembers.emplace_back(member);
        } else if (arg == "--nopersQF") {
            MemberWithQualifiers member;
            member._memberName = argv[++i];
            member._qualifiers = "noperspective";
            _testMembers.emplace_back(member);
        } else if (arg == "--centroidQF") {
            MemberWithQualifiers member;
            member._memberName = argv[++i];
            member._qualifiers = "centroid";
            _testMembers.emplace_back(member);
        } else if (arg == "--sampleQF") {
            MemberWithQualifiers member;
            member._memberName = argv[++i];
            member._qualifiers = "sample";
            _testMembers.emplace_back(member);
        }
    }
}

// Get the language specified qualifier string.
std::string
_GetInterpolationString(const TfToken& hgiName, const std::string& qualifiers)
{
    if (hgiName == HgiTokens->OpenGL || hgiName == HgiTokens->Vulkan) {
        return qualifiers;
    } else if (hgiName == HgiTokens->Metal) {
        if (qualifiers == "flat") {
            return qualifiers;
        } else if (qualifiers == "noperspective") {
            return "center_no_perspective";
        } else if (qualifiers == "centroid") {
            return "centroid_perspective";
        } else if (qualifiers == "sample") {
            return "sample_perspective";
        } else {
            return "";
        }
    } else {
        return "";
    }
}

// Verify if qualifiers are correctly generated.
bool
_VerifyQualifiers(std::string const& shaderSource, std::string const& member,
    std::string const& qualifier, bool qualifierBeforeMember,
    int offsetOfQualifier)
{
    // Find the member that should have qualifiers.
    size_t index = shaderSource.find(member, 0);
    if (index < qualifier.length() + 1) {
        return false;
    } else {
        if (qualifierBeforeMember) {
            // Check if the correct qualifier is before the member.
            // In GLSL, the syntax for a qualifier is like
            // "flat float dataFlat,".
            // So the position for the qualifier is the start of the member 
            // type minus the length of the qualifier and offset. Here, 
            // offsetQualifer should be -1 for the space before the member.
            // First find the index of the start of member type.
            index = shaderSource.find_last_not_of(' ', index - 1);
            index = shaderSource.find_last_of(' ', index) + 1;
            if (shaderSource.compare(
                index - qualifier.length() + offsetOfQualifier,
                    qualifier.length(), qualifier) == 0) {
                return true;
            } else {
                return false;
            }
        } else {
             // Check if the correct qualifier is after the member.
            // In MSL, the syntax for a qualifier is like
            // "float dataFlat [[flat]];".
            // So the position for the qualifier is the start of the member 
            // plus the length of the member and offset. Here, 
            // offsetQualifer should be 2 for "[[" after the member.
            if (shaderSource.compare(
                index + member.length() + offsetOfQualifier,
                qualifier.length(), qualifier) == 0) {
                return true;
            } else {
                return false;
            }
        }
    }
}

void
My_TestGLDrawing::DrawTest()
{
    HdRenderIndex& renderIndex = _driver->GetDelegate().GetRenderIndex();
    HdStResourceRegistrySharedPtr const& registry =
        std::static_pointer_cast<HdStResourceRegistry>(
            renderIndex.GetResourceRegistry());

    HdStRenderPassShaderSharedPtr renderPassShader =
        std::make_shared<HdStRenderPassShader>();
    std::shared_ptr<My_Shader> shader = std::make_shared<My_Shader>(
        _tokens->glslfxFilename, _tokens->defaultToken);
    HdStShaderCodeSharedPtrVector shaders = { renderPassShader, shader };
    
    HdSt_PointsShaderKey shaderKey = HdSt_PointsShaderKey();

    // Create the geometric shader.
    HdInstance<HdSt_GeometricShaderSharedPtr> geometricShaderInstance =
        registry->RegisterGeometricShader(shaderKey.ComputeHash());

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
    HdSt_GeometricShaderSharedPtr geometricShader =
        geometricShaderInstance.GetValue();

    // Resolve bindings.
    HdSt_ResourceBinder::MetaData::DrawingCoordBufferBinding
        drawingCoordBufferBinding;
    HdStBindingRequestVector customBindings;
    HdRprimSharedData sharedData(1);
    std::unique_ptr<HdStDrawItem> drawItem =
        std::make_unique<HdStDrawItem>(&sharedData);

    // Let resource binder resolve bindings and populate metadata which is owned 
    // by codegen.
    std::unique_ptr<HdSt_ResourceBinder::MetaData> metaData =
        std::make_unique<HdSt_ResourceBinder::MetaData>();
    HdSt_ResourceBinder resourceBinder;
    resourceBinder.ResolveBindings(
        drawItem.get(),
        shaders,
        metaData.get(),
        drawingCoordBufferBinding,
        false,
        customBindings,
        registry->GetHgi()->GetCapabilities());

    // Initialize codegen.
    HdSt_CodeGen codeGen(geometricShader, shaders,
        HdStMaterialTagTokens->defaultMaterialTag, std::move(metaData));

    // Compile the program.
    HdStGLSLProgramSharedPtr glslProgram = codeGen.Compile(registry.get());

    // If we fail to compile or link the program, return failure.
    if (!glslProgram || !glslProgram->Link()) {
        // Failed to compile and link a valid glsl program.
        return;
    }

    Hgi* hgi = registry->GetHgi();
    bool qualifierBeforeMember = true;
    int offset = 0;
    // In GLSL, the syntax for a qualifier is like "flat float dataFlat,".
    // So qualifier is before the member and the extra offset is 1 for the 
    // white space.
    if (hgi->GetAPIName() == HgiTokens->OpenGL ||
        hgi->GetAPIName() == HgiTokens->Vulkan) {
        offset = -1;
    // In MSL, the syntax for a qualifier is like "float dataFlat [[flat]];". 
    // So qualifier is after the member and the extra offset is 2 for "[[".
    } else if (hgi->GetAPIName() == HgiTokens->Metal) {
        qualifierBeforeMember = false;
        offset = 2;
    }

    // Verify if the qualifiers are correctly added before the member.
    for (auto member : _testMembers) {
        if (!_VerifyQualifiers(codeGen.GetVertexShaderSource(),
            member._memberName, _GetInterpolationString(hgi->GetAPIName(),
            member._qualifiers), qualifierBeforeMember, offset)) {
            return;
        }
        if (!_VerifyQualifiers(codeGen.GetFragmentShaderSource(),
            member._memberName, _GetInterpolationString(hgi->GetAPIName(), 
            member._qualifiers), qualifierBeforeMember, offset)) {
            return;
        }
    }

    _testResult = true;
}

int main(int argc, char* argv[])
{
    My_TestGLDrawing driver;
    driver.RunTest(argc, argv);
    const bool success =  driver.GetTestResult();

    if (success) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
