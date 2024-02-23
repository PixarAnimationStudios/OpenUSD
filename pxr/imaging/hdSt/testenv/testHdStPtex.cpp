//
// Copyright 2021 Pixar
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

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/imaging/hdSt/ptexTextureObject.h"

#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/envSetting.h"

#include "pxr/usd/sdr/registry.h"

#include <iostream>
#include <mutex>

#include <Ptexture.h>
#include <PtexUtils.h>

PXR_NAMESPACE_USING_DIRECTIVE

class My_TestGLDrawing : public HdSt_UnitTestGLDrawing {
public:
    My_TestGLDrawing() {
        // this rotation is to make non-quad faces of the sphere asset
        // visible. we should generalize it (to commandline args) later.
        SetCameraRotate(90.0f, 0.0f);
        SetCameraTranslate(GfVec3f(0));

        _reprName = HdReprTokens->hull;
        _refineLevel = 0;
        _cullStyle = HdCullStyleNothing;
        _forceLoop = false;
    }

    // HdSt_UnitTestGLDrawing overrides
    void InitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;
    void Present(uint32_t framebuffer) override;

protected:
    void ParseArgs(int argc, char *argv[]) override;

private:
    HdSt_TestDriver* _driver;

    TfToken _reprName;
    int _refineLevel;
    HdCullStyle _cullStyle;
    bool _forceLoop;

    std::string _textureFilePath;
    std::string _outputFilePath;
};

////////////////////////////////////////////////////////////

void
My_TestGLDrawing::InitTest()
{
    _driver = new HdSt_TestDriver(_reprName);
    HdUnitTestDelegate &delegate = _driver->GetDelegate();
    delegate.SetRefineLevel(_refineLevel);

    SdfPath materialId("/material");

    std::string ptexfile = _textureFilePath;

    std::string const shaderSource(
        "-- glslfx version 0.1 \n"
        "-- configuration \n"
        "{\n"
            "\"textures\" : { \n"
            "    \"ptexColor\": { \n"
            "        \"description\": \"Color PTexture\" \n"
            "    } \n"
            "}, \n"
            "\"techniques\": {\n"
            "    \"default\": {\n"
            "        \"surfaceShader\": {\n"
            "            \"source\": [ \"testHdStPtex.Surface\" ]\n"
            "        }\n"
            "    }\n"
            "}\n\n"
        "}\n"

        "-- glsl testHdStPtex.Surface \n\n"
        "vec4 surfaceShader(vec4 Peye, vec3 Neye, vec4 color, vec4 patchCoord) {\n"
        "    color.rgb = HdGet_ptexColor().xyz;\n"
        "    return color;\n"
        "}\n"
    );

    SdrRegistry &shaderReg = SdrRegistry::GetInstance();
    SdrShaderNodeConstPtr sdrSurfaceNode = 
        shaderReg.GetShaderNodeFromSourceCode(
            shaderSource, 
            HioGlslfxTokens->glslfx,
            NdrTokenMap()); // metadata

    TfToken const& terminalType = HdMaterialTerminalTokens->surface;

    // Adding basic material
    HdMaterialNetworkMap material1;
    HdMaterialNetwork& network1 = material1.map[terminalType];
    HdMaterialNode terminal1;
    terminal1.path = materialId.AppendPath(SdfPath("/Shader"));
    terminal1.identifier = sdrSurfaceNode->GetIdentifier();
    terminal1.parameters[TfToken("ptexColor")] = VtValue(GfVec3f(1,0,0));

    // Insert ptex node
    HdMaterialNode textureNode;
    textureNode.path = SdfPath("/tex0");
    textureNode.identifier = TfToken("HwPtexTexture_1");
    textureNode.parameters[TfToken("fallback")] = VtValue(GfVec3f(1,0,0));
    textureNode.parameters[TfToken("file")] = VtValue(ptexfile);

    // Insert connection between texture node and terminal
    HdMaterialRelationship rel;
    rel.inputId = textureNode.path;
    rel.inputName = TfToken("color");
    rel.outputId = terminal1.path;
    rel.outputName = TfToken("ptexColor");
    network1.relationships.emplace_back(std::move(rel));

    // Insert texture node
    network1.nodes.emplace_back(std::move(textureNode));

    // Insert terminal
    material1.terminals.push_back(terminal1.path);
    network1.nodes.emplace_back(std::move(terminal1)); // must be last in vector
    delegate.AddMaterialResource(
        materialId,
        VtValue(material1));

    delegate.BindMaterial(SdfPath("/mesh"), SdfPath("/material"));

    // read a mesh from ptex's metadata
    GfRange3f range;
    {
        Ptex::String ptexError;
        PtexTexture *ptx = PtexTexture::open(ptexfile.c_str(),
                                             ptexError, true);
        if (ptx == NULL) {
            printf("Error in reading ptex\n");
            exit(1);
        }
        PtexMetaData* meta = ptx->getMetaData();

        const float * vp;
        const int *vi, *vc;
        int nvp = 0, nvi = 0, nvc = 0;
        meta->getValue("PtexVertPositions", vp, nvp);
        meta->getValue("PtexFaceVertCounts", vc, nvc);
        meta->getValue("PtexFaceVertIndices", vi, nvi);
        if (nvc == 0 || nvp == 0 || nvi == 0) {
            exit(1);
        }

        GfMatrix4f transform(1);
        VtArray<GfVec3f> points(nvp/3);
        VtArray<int> numVerts(nvc);
        VtArray<int> verts(nvi);
        std::copy((GfVec3f*)vp, (GfVec3f*)(vp+nvp), points.begin());
        std::copy(vc, vc+nvc, numVerts.begin());
        std::copy(vi, vi+nvi, verts.begin());

        for (size_t i = 0; i < points.size(); ++i) {
            range.UnionWith(points[i]);
        }

        delegate.AddMesh(
            SdfPath("/mesh"),
            transform,
            points,
            numVerts,
            verts,
            false,
            SdfPath(),
            _forceLoop ? PxOsdOpenSubdivTokens->loop : PxOsdOpenSubdivTokens->catmullClark,
            HdTokens->rightHanded,
            false);

        ptx->release();
    }
    // frame the object
    GfVec3f center = (range.GetMin() + range.GetMax()) * 0.5f;
    center[2] += range.GetSize().GetLength();
    SetCameraTranslate(-center);

    _driver->SetClearColor(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
    _driver->SetClearDepth(1.0f);
    _driver->SetupAovs(GetWidth(), GetHeight());
}

void
My_TestGLDrawing::DrawTest()
{
    int width = GetWidth(), height = GetHeight();
    GfMatrix4d viewMatrix = GetViewMatrix();
    GfMatrix4d projMatrix = GetProjectionMatrix();

    _driver->SetCullStyle(_cullStyle);

    _driver->SetCamera(
        viewMatrix,
        projMatrix,
        CameraUtilFraming(
            GfRect2i(GfVec2i(0, 0), width, height)));

    _driver->UpdateAovDimensions(width, height);

    _driver->Draw();
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
My_TestGLDrawing::ParseArgs(int argc, char *argv[])
{
    for (int i=0; i<argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--repr") {
            _reprName = TfToken(argv[++i]);
        } else if (arg == "--refineLevel") {
            _refineLevel = atoi(argv[++i]);
        } else if (arg == "--loop") {
            _forceLoop = true;
        } else if (std::string(argv[i]) == "--cullStyle") {
            std::string style = argv[++i];
            if (style == "Nothing") {
                _cullStyle = HdCullStyleNothing;
            } else if (style == "Back") {
                _cullStyle = HdCullStyleBack;
            } else if (style == "Front") {
                _cullStyle = HdCullStyleFront;
            } else if (style == "BackUnlessDoubleSided") {
                _cullStyle = HdCullStyleBackUnlessDoubleSided;
            } else if (style == "FrontUnlessDoubleSided") {
                _cullStyle = HdCullStyleFrontUnlessDoubleSided;
            } else {
                std::cerr << "Error: Unknown cullstyle = " << style << "\n";
                exit(EXIT_FAILURE);
            }
        } else if (arg == "--ptex") {
            _textureFilePath = argv[++i];
        } else if (std::string(argv[i]) == "--write") {
            _outputFilePath = argv[++i];
        }
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
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

