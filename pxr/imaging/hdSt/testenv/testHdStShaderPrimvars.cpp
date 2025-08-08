//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/unitTestDelegate.h"
#include "pxr/imaging/hdSt/material.h"
#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"
#include "pxr/usd/sdr/registry.h"

#include <iostream>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

class My_TestGLDrawing : public HdSt_UnitTestGLDrawing {
public:
    My_TestGLDrawing() {
        SetCameraRotate(60.0f, 0.0f);
        SetCameraTranslate(GfVec3f(0, 0, -10.0f));

        _reprName = HdReprTokens->hull;
        _refineLevel = 0;
        _cullStyle = HdCullStyleNothing;
    }

    // HdSt_UnitTestGLDrawing overrides
    void InitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;
    void Present(uint32_t framebuffer) override;

protected:
    void ParseArgs(int argc, char* argv[]) override;

private:
    HdSt_TestDriverUniquePtr _driver;

    HdMaterialNetworkMap _networkMap;
    SdfPath _materialPath;

    TfToken _reprName;
    int _refineLevel;
    HdCullStyle _cullStyle;
    std::string _outputFilePrefix;
};

/* virtual */
void
My_TestGLDrawing::InitTest()
{
    // Initialize driver.
    _driver = std::make_unique<HdSt_TestDriver>(_reprName);
    _driver->SetClearColor(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
    _driver->SetClearDepth(1.0f);
    _driver->SetupAovs(GetWidth(), GetHeight());

    HdUnitTestDelegate& delegate = _driver->GetDelegate();
    delegate.SetRefineLevel(_refineLevel);

    // ------------------------------------------------------------------------
    // Process raw shader with Sdr.
    std::string const surfaceShaderSource(
        "-- glslfx version 0.1 \n"
        "-- configuration \n"
        "{\n"
        "\"techniques\": {\n"
        "    \"default\": {\n"
        "        \"surfaceShader\": {\n"
        "            \"source\": [ \"testHdStShaderPrimvars.Surface\" ]\n"
        "        }\n"
        "    }\n"
        "}\n\n"
        "}\n"

        "-- glsl testHdStShaderPrimvars.Surface \n\n"
        "vec4 surfaceShader(vec4 Peye, vec3 Neye, vec4 color, vec4 patchCoord) {\n"
        "#if defined(HD_HAS_Primvar_0) \n"
        "    return HdGet_Primvar_0();\n"
        "#else\n"
        "    return vec4(0.0);\n"
        "#endif\n"
        "}\n"
    );

    SdrRegistry& shaderReg = SdrRegistry::GetInstance();
    SdrShaderNodeConstPtr sdrSurfaceNode =
        shaderReg.GetShaderNodeFromSourceCode(
            surfaceShaderSource,
            HioGlslfxTokens->glslfx,
            SdrTokenMap()); // metadata

    // ------------------------------------------------------------------------
    // Create a material network.
    _materialPath = SdfPath("/Material0");
    HdMaterialNetwork materialNetwork;

    HdMaterialNode materialNode;
    materialNode.path = _materialPath.AppendChild(pxr::TfToken("SurfaceShader"));
    materialNode.identifier = sdrSurfaceNode->GetIdentifier();;
    materialNetwork.nodes.push_back(materialNode);

    const TfToken primvarToken("Primvar_0");
    materialNetwork.primvars.push_back(primvarToken);

    _networkMap.terminals.push_back(materialNode.path);
    _networkMap.map[HdMaterialTerminalTokens->surface] = materialNetwork;
    delegate.AddMaterialResource(_materialPath, VtValue(_networkMap));

    // ------------------------------------------------------------------------
    // Create rprims with corresponding primvars and add to the scene.
    GfMatrix4d dmat;

    const SdfPath grid0Path("/Grid0");
    dmat.SetTranslate(GfVec3d(-4.5, 0.0, 0.0));
    delegate.AddGrid(grid0Path, 10, 10, GfMatrix4f(dmat));
    delegate.AddPrimvar(grid0Path, primvarToken,
        VtValue(GfVec4f(1.0f, 0.0f, 0.0f, 1.0f)), HdInterpolationConstant,
        HdPrimvarRoleTokens->none);
    delegate.BindMaterial(grid0Path, _materialPath);

    const SdfPath grid1Path("/Grid1");
    dmat.SetTranslate(GfVec3d(-1.5, 0.0, 0.0));
    delegate.AddGrid(grid1Path, 10, 10, GfMatrix4f(dmat));
    delegate.AddPrimvar(grid1Path, primvarToken,
        VtValue(GfVec4f(0.0f, 1.0f, 0.0f, 1.0f)), HdInterpolationConstant,
        HdPrimvarRoleTokens->none);
    delegate.BindMaterial(grid1Path, _materialPath);

    const SdfPath grid2Path("/Grid2");
    dmat.SetTranslate(GfVec3d(1.5, 0.0, 0.0));
    delegate.AddGrid(grid2Path, 10, 10, GfMatrix4f(dmat));
    delegate.AddPrimvar(grid2Path, primvarToken,
        VtValue(GfVec4f(0.0f, 0.0f, 1.0f, 1.0f)), HdInterpolationConstant,
        HdPrimvarRoleTokens->none);
    delegate.BindMaterial(grid2Path, _materialPath);

    const SdfPath grid3Path("/Grid3");
    dmat.SetTranslate(GfVec3d(4.5, 0.0, 0.0));
    delegate.AddGrid(grid3Path, 10, 10, GfMatrix4f(dmat));
    delegate.AddPrimvar(grid3Path, primvarToken,
        VtValue(GfVec4f(1.0f, 1.0f, 0.0f, 1.0f)), HdInterpolationConstant,
        HdPrimvarRoleTokens->none);
    delegate.BindMaterial(grid3Path, _materialPath);
}

/* virtual */
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

    // Check the material network.
    // NOTE: By default, the SceneIndex emulation will be used, in which case both
    // ConvertHdMaterialNetworkToHdMaterialSchema & HdConvertToHdMaterialNetwork2 can
    // be tested.
    HdUnitTestDelegate& delegate = _driver->GetDelegate();
    HdMaterialNetworkMap const& resultNetworkMap =
        delegate.GetMaterialResource(_materialPath).Get<HdMaterialNetworkMap>();
    TF_VERIFY(resultNetworkMap == _networkMap);
}

/* virtual */
void
My_TestGLDrawing::OffscreenTest()
{
    if (!TF_VERIFY(!_outputFilePrefix.empty())) return;

    DrawTest();
    _driver->WriteToFile("color", _outputFilePrefix + ".png");
}

/* virtual */
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
        if (arg == "--outputFilePrefix") {
            _outputFilePrefix = argv[++i];
        }
    }
}

void
ShaderPrimvarsTest(int argc, char* argv[])
{
    My_TestGLDrawing driver;

    driver.RunTest(argc, argv);
}

int main(int argc, char* argv[])
{
    TfErrorMark mark;

    ShaderPrimvarsTest(argc, argv);

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
