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

#include "pxr/imaging/hdSt/material.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdSt/fallbackLightingShader.h"
#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"

#include "pxr/base/tf/errorMark.h"

#include "pxr/usd/sdr/registry.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

class My_TestGLDrawing : public HdSt_UnitTestGLDrawing {
public:
    My_TestGLDrawing() {
        SetCameraRotate(60.0f, 0.0f);
        SetCameraTranslate(GfVec3f(0, 0, -10.0f));

        _reprName = HdReprTokens->hull;
        _refineLevel = 0;
        _cullStyle = HdCullStyleNothing;
        _currentLight = 0;
        _useSceneMaterials = true;
    }

    // HdSt_UnitTestGLDrawing overrides
    void InitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;
    void Present(uint32_t framebuffer) override;

    void KeyRelease(int key) override;

protected:
    void ParseArgs(int argc, char *argv[]) override;

private:
    void toggleLight();
    void updateMaterial();
    void addPrim();
    void toggleUseSceneMaterials();
    void rebindMaterial(SdfPath const &rprimId, SdfPath const &materialId);

    HdSt_TestDriver* _driver;

    std::vector<HdStLightingShaderSharedPtr> _lightingShaders;

    TfToken _reprName;
    int _refineLevel;
    HdCullStyle _cullStyle;
    int _currentLight;
    bool _useSceneMaterials;
    std::string _outputFilePrefix;
};

////////////////////////////////////////////////////////////

void
My_TestGLDrawing::InitTest()
{
    _driver = new HdSt_TestDriver(_reprName);
    HdUnitTestDelegate &delegate = _driver->GetDelegate();
    delegate.SetRefineLevel(_refineLevel);

    GfMatrix4d dmat;

    std::string const shaderSurfaceSource(
        "-- glslfx version 0.1 \n"
        "-- configuration \n"
        "{\n"
            "\"techniques\": {\n"
            "    \"default\": {\n"
            "        \"surfaceShader\": {\n"
            "            \"source\": [ \"testHdStShaders.Surface\" ]\n"
            "        }\n"
            "    }\n"
            "}\n\n"
        "}\n"

        "-- glsl testHdStShaders.Surface \n\n"
        "vec4 surfaceShader(vec4 Peye, vec3 Neye, vec4 color, vec4 patchCoord) {\n"
        "    return vec4(FallbackLighting(Peye.xyz, Neye, vec3(1)), 1);\n"
        "}\n"
    );

    std::string const shaderSurfaceFallbackSource(
        "-- glslfx version 0.1 \n"
        "-- configuration \n"
        "{\n"
            "\"parameters\": {\n"
                "\"fallbackColor\": {\n"
                    "\"default\": [0.0, 0.0, 0.0] \n"
                "}\n"
            "},\n"
            "\"techniques\": {\n"
            "    \"default\": {\n"
            "        \"surfaceShader\": {\n"
            "            \"source\": [ \"testHdStShaders.Surface\" ]\n"
            "        }\n"
            "    }\n"
            "}\n\n"
        "}\n"

        "-- glsl testHdStShaders.Surface \n\n"
        "vec4 surfaceShader(vec4 Peye, vec3 Neye, vec4 color, vec4 patchCoord) {\n"
        "    return vec4(FallbackLighting(Neye, abs(Peye.xyz), HdGet_fallbackColor()), 1);\n"
        "}\n"
    );

    std::string const shaderSurfaceDisplacementSource(
        "-- glslfx version 0.1 \n"
        "-- configuration \n"
        "{\n"
            "\"techniques\": {\n"
            "    \"default\": {\n"
            "        \"surfaceShader\": {\n"
            "            \"source\": [ \"testHdStShaders.Surface\" ]\n"
            "        },\n"
            "        \"displacementShader\": {\n"
            "            \"source\": [ \"testHdStShaders.Displacement\" ]\n"
            "        }\n"
            "    }\n"
            "}\n\n"
        "}\n"

        "-- glsl testHdStShaders.Surface \n\n"

        "vec4 surfaceShader(vec4 Peye, vec3 Neye, vec4 color, vec4 patchCoord) {\n"
        "    return vec4(FallbackLighting(Peye.xyz, Neye, vec3(1)), 1);\n"
        "}\n\n"

        "-- glsl testHdStShaders.Displacement \n\n"

        "vec4 displacementShader(int index, vec4 Peye, vec3 Neye, vec4 patchCoord) {\n"
        "    return Peye + vec4(Neye,0.0) * 3.0;\n"
        "}\n"
    );

    std::string const shaderSurfaceImageToWorldSource(
        "-- glslfx version 0.1 \n"
        "-- configuration \n"
        "{\n"
            "\"techniques\": {\n"
            "    \"default\": {\n"
            "        \"surfaceShader\": {\n"
            "            \"source\": [ \"testHdStShaders.Surface\" ]\n"
            "        }\n"
            "    }\n"
            "}\n\n"
        "}\n"

        "-- glsl testHdStShaders.Surface \n\n"
        "vec4 surfaceShader(vec4 Peye, vec3 Neye, vec4 color, vec4 patchCoord) {\n"
        "    vec3 wpos = GetPositionInWorldSpace(gl_FragCoord.xyz);\n"
        "    // scale and translate wpos so that the bounds of the test \n"
        "    // geometry land in 0-1 for test visualization purposes\n"
        "    wpos += vec3(5.5, 1, 1);"
        "    wpos /= vec3(11, 5, 2);\n"
        "    return vec4(wpos, 1);\n"
        "}\n"
    );

    // To get the unique identifier of a custom glslfx we need to consult the
    // SdrRegistry since it will generate a unique hash for them on the fly.
    SdrRegistry &shaderReg = SdrRegistry::GetInstance();

    SdrShaderNodeConstPtr sdrSurfaceNode = 
        shaderReg.GetShaderNodeFromSourceCode(
            shaderSurfaceSource, 
            HioGlslfxTokens->glslfx,
            NdrTokenMap()); // metadata

    SdrShaderNodeConstPtr sdrSurfaceFallbackNode = 
        shaderReg.GetShaderNodeFromSourceCode(
            shaderSurfaceFallbackSource, 
            HioGlslfxTokens->glslfx,
            NdrTokenMap()); // metadata

    SdrShaderNodeConstPtr sdrSurfaceDisplacementNode = 
        shaderReg.GetShaderNodeFromSourceCode(
            shaderSurfaceDisplacementSource, 
            HioGlslfxTokens->glslfx,
            NdrTokenMap()); // metadata

    SdrShaderNodeConstPtr sdrSurfaceImageToWorldNode = 
        shaderReg.GetShaderNodeFromSourceCode(
            shaderSurfaceImageToWorldSource, 
            HioGlslfxTokens->glslfx,
            NdrTokenMap()); // metadata

    TfToken const& terminalType = HdMaterialTerminalTokens->surface;

    // Adding basic material
    HdMaterialNetworkMap material1;
    HdMaterialNetwork& network1 = material1.map[terminalType];
    HdMaterialNode terminal1;
    terminal1.path = SdfPath("/material1/Shader");
    terminal1.identifier = sdrSurfaceNode->GetIdentifier();
    material1.terminals.push_back(terminal1.path);
    network1.nodes.emplace_back(std::move(terminal1)); // must be last in vector
    delegate.AddMaterialResource(
        SdfPath("/material1"),
        VtValue(material1));

    // Adding a material with a parameter
    HdMaterialNetworkMap material2;
    HdMaterialNetwork& network2 = material2.map[terminalType];
    HdMaterialNode terminal2;
    terminal2.path = SdfPath("/material2/Shader");
    terminal2.identifier = sdrSurfaceFallbackNode->GetIdentifier();
    terminal2.parameters[TfToken("fallbackColor")] = VtValue(GfVec3f(1));
    material2.terminals.push_back(terminal2.path);
    network2.nodes.emplace_back(std::move(terminal2)); // must be last in vector
    delegate.AddMaterialResource(
        SdfPath("/material2"),
        VtValue(material2));

    // Adding a material with surface and displacement
    HdMaterialNetworkMap material3;
    HdMaterialNetwork& network3 = material3.map[terminalType];
    HdMaterialNode terminal3;
    terminal3.path = SdfPath("/material3/Shader");
    terminal3.identifier = sdrSurfaceDisplacementNode->GetIdentifier();
    material3.terminals.push_back(terminal3.path);
    network3.nodes.emplace_back(std::move(terminal3)); // must be last in vector
    delegate.AddMaterialResource(
        SdfPath("/material3"),
        VtValue(material3));

    // Adding material exposing world xyz as rgb via imageToWorldMatrix
    HdMaterialNetworkMap material4;
    HdMaterialNetwork& network4 = material4.map[terminalType];
    HdMaterialNode terminal4;
    terminal4.path = SdfPath("/material4/Shader");
    terminal4.identifier = sdrSurfaceImageToWorldNode->GetIdentifier();
    material4.terminals.push_back(terminal4.path);
    network4.nodes.emplace_back(std::move(terminal4)); // must be last in vector
    delegate.AddMaterialResource(
        SdfPath("/material4"),
        VtValue(material4));


    // grids
    {
        dmat.SetTranslate(GfVec3d(-4.5, 0.0, 0.0));
        delegate.BindMaterial(SdfPath("/grid1"), SdfPath("/material1"));
        delegate.AddGrid(SdfPath("/grid1"), 10, 10, GfMatrix4f(dmat));

        dmat.SetTranslate(GfVec3d(-1.5, 0.0, 0.0));
        delegate.BindMaterial(SdfPath("/grid2"), SdfPath("/material1"));
        delegate.AddGridWithFaceColor(SdfPath("/grid2"), 10, 10, GfMatrix4f(dmat));

        dmat.SetTranslate(GfVec3d(1.5, 0.0, 0.0));
        delegate.BindMaterial(SdfPath("/grid3"), SdfPath("/material2"));
        delegate.AddGridWithVertexColor(SdfPath("/grid3"), 10, 10, GfMatrix4f(dmat));

        dmat.SetTranslate(GfVec3d(4.5, 0.0, 0.0));
        delegate.BindMaterial(SdfPath("/grid4"), SdfPath("/material2"));
        delegate.AddGridWithFaceVaryingColor(SdfPath("/grid4"), 3, 3, GfMatrix4f(dmat));
    }

    // setup lights
    HdSt_TestLightingShaderSharedPtr lightingShader1(
                new HdSt_TestLightingShader(&delegate.GetRenderIndex()));
    HdSt_TestLightingShaderSharedPtr lightingShader2(
                new HdSt_TestLightingShader(&delegate.GetRenderIndex()));
    HdSt_FallbackLightingShaderSharedPtr fallbackShader(
                new HdSt_FallbackLightingShader());

    lightingShader1->SetLight(0, GfVec3f(1, 0, 1), GfVec3f(1, 0.2, 0.2));
    lightingShader1->SetSceneAmbient(GfVec3f(0.01, 0.01, 0.01));
    lightingShader1->Prepare();

    lightingShader2->SetLight(0, GfVec3f(1, 0, 1), GfVec3f(1, 0.2, 0.2));
    lightingShader2->SetLight(1, GfVec3f(0, 1, 1), GfVec3f(0.2, 1, 0.2));
    lightingShader2->SetSceneAmbient(GfVec3f(0.01, 0.01, 0.01));
    lightingShader2->Prepare();

    _lightingShaders.push_back(lightingShader1);
    _lightingShaders.push_back(lightingShader2);
    _lightingShaders.push_back(fallbackShader);
    _driver->GetRenderPassState()->SetLightingShader(
        _lightingShaders[_currentLight]);
    
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
    if (!TF_VERIFY(!_outputFilePrefix.empty())) return;

    DrawTest();
    _driver->WriteToFile("color", _outputFilePrefix + "_0.png");

    toggleLight();
    DrawTest();
    _driver->WriteToFile("color", _outputFilePrefix + "_1.png");

    toggleLight();
    DrawTest();
    _driver->WriteToFile("color", _outputFilePrefix + "_2.png");

    addPrim();
    DrawTest();
    _driver->WriteToFile("color", _outputFilePrefix + "_3.png");

    updateMaterial();
    DrawTest();
    _driver->WriteToFile("color", _outputFilePrefix + "_4.png");

    toggleLight();
    DrawTest();
    _driver->WriteToFile("color", _outputFilePrefix + "_5.png");

    toggleLight();
    toggleLight();
    toggleUseSceneMaterials();
    DrawTest();
    _driver->WriteToFile("color", _outputFilePrefix + "_6.png");

    toggleUseSceneMaterials();
    DrawTest();
    _driver->WriteToFile("color", _outputFilePrefix + "_7.png");

    rebindMaterial(SdfPath("/cube1"), SdfPath("/material3"));
    DrawTest();
    _driver->WriteToFile("color", _outputFilePrefix + "_8.png");

    rebindMaterial(SdfPath("/grid1"), SdfPath("/material4"));
    rebindMaterial(SdfPath("/grid2"), SdfPath("/material4"));
    rebindMaterial(SdfPath("/grid3"), SdfPath("/material4"));
    rebindMaterial(SdfPath("/grid4"), SdfPath("/material4"));
    rebindMaterial(SdfPath("/cube1"), SdfPath("/material4"));
    DrawTest();
    _driver->WriteToFile("color", _outputFilePrefix + "_9.png");

    rebindMaterial(SdfPath("/grid1"), SdfPath("/material1"));
    rebindMaterial(SdfPath("/grid2"), SdfPath("/material1"));
    rebindMaterial(SdfPath("/grid3"), SdfPath("/material2"));
    rebindMaterial(SdfPath("/grid4"), SdfPath("/material2"));
    rebindMaterial(SdfPath("/cube1"), SdfPath("/material1"));
    DrawTest();
    _driver->WriteToFile("color", _outputFilePrefix + "_10.png");
}

void
My_TestGLDrawing::KeyRelease(int key)
{
    switch(key) {
    case ' ':
        toggleLight();
        break;
    case 'S':
        updateMaterial();
        break;
    case 'A':
        addPrim();
        break;
    case 'O':
        toggleUseSceneMaterials();
        break;
    case 'D':
        rebindMaterial(SdfPath("/cube1"), SdfPath("/material3"));
        break;
    case 'F':
        rebindMaterial(SdfPath("/cube1"), SdfPath("/material1"));
        break;        
    }
}

void
My_TestGLDrawing::Present(uint32_t framebuffer)
{
    _driver->Present(GetWidth(), GetHeight(), framebuffer);
}

void
My_TestGLDrawing::toggleLight()
{
    _currentLight = (_currentLight + 1) % int(_lightingShaders.size());
    _driver->GetRenderPassState()->SetLightingShader(
        _lightingShaders[_currentLight]);
}

void
My_TestGLDrawing::updateMaterial()
{
    HdUnitTestDelegate &delegate = _driver->GetDelegate();

    static float m = 1.0f;
    m += 1.0f;

    std::string const shader1Source(
        "-- glslfx version 0.1 \n"
        "-- configuration \n"
        "{\n"
            "\"techniques\": {\n"
            "    \"default\": {\n"
            "        \"surfaceShader\": {\n"
            "            \"source\": [ \"testHdStShaders.Surface\" ]\n"
            "        }\n"
            "    }\n"
            "}\n\n"
        "}\n"

        "-- glsl testHdStShaders.Surface \n\n"
        "vec4 surfaceShader(vec4 Peye, vec3 Neye, vec4 color, vec4 patchCoord) {\n"
        "    return vec4(sin(" + std::to_string(m) + "*Peye.xyz), 1);\n"
        "}\n"
    );

    SdrRegistry &shaderReg = SdrRegistry::GetInstance();
    SdrShaderNodeConstPtr sdrSurfaceNode = 
        shaderReg.GetShaderNodeFromSourceCode(
            shader1Source, 
            HioGlslfxTokens->glslfx,
            NdrTokenMap()); // metadata

    TfToken const& terminalType = HdMaterialTerminalTokens->surface;

    // Adding basic material
    HdMaterialNetworkMap material1;
    HdMaterialNetwork& network1 = material1.map[terminalType];
    HdMaterialNode terminal1;
    terminal1.path = SdfPath("/material1/Shader");
    terminal1.identifier = sdrSurfaceNode->GetIdentifier();
    material1.terminals.push_back(terminal1.path);
    network1.nodes.emplace_back(std::move(terminal1)); // must be last in vector
    delegate.UpdateMaterialResource(
        SdfPath("/material1"),
        VtValue(material1));
}

void
My_TestGLDrawing::rebindMaterial(SdfPath const &rprimId,
                                 SdfPath const &materialId)
{
    HdUnitTestDelegate &delegate = _driver->GetDelegate();
    delegate.RebindMaterial(rprimId, materialId);
}

void
My_TestGLDrawing::addPrim()
{
    HdUnitTestDelegate &delegate = _driver->GetDelegate();

    GfMatrix4d dmat(1);
    dmat.SetTranslate(GfVec3d(0.0, 3.0, 0.0));

    delegate.BindMaterial(SdfPath("/cube1"), SdfPath("/material1"));
    delegate.AddCube(SdfPath("/cube1"), GfMatrix4f(dmat));
}

void
My_TestGLDrawing::toggleUseSceneMaterials()
{
    _useSceneMaterials = !_useSceneMaterials;
    _driver->GetRenderPassState()->SetUseSceneMaterials(_useSceneMaterials);
}

/* virtual */
void
My_TestGLDrawing::ParseArgs(int argc, char *argv[])
{
    for (int i=0; i<argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--outputFilePrefix") {
            _outputFilePrefix = argv[++i];
        }
    }
}

void
MaterialTest(int argc, char *argv[])
{
    My_TestGLDrawing driver;

    driver.RunTest(argc, argv);
}

int main(int argc, char *argv[])
{
    TfErrorMark mark;

    MaterialTest(argc, argv);

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

