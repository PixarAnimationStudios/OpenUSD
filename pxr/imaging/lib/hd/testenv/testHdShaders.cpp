#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/defaultLightingShader.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/simpleLightingShader.h"
#include "pxr/imaging/hd/surfaceShader.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/unitTestGLDrawing.h"
#include "pxr/imaging/hd/unitTestHelper.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"

#include "pxr/base/tf/errorMark.h"

#include <boost/shared_ptr.hpp>

#include <iostream>

class My_TestGLDrawing : public Hd_UnitTestGLDrawing {
public:
    My_TestGLDrawing() {
        SetCameraRotate(60.0f, 0.0f);
        SetCameraTranslate(GfVec3f(0, 0, -10.0f));

        _reprName = HdTokens->hull;
        _refineLevel = 0;
        _cullStyle = HdCullStyleNothing;
        _currentLight = 0;
        _overrideShader = false;
    }

    // Hd_UnitTestGLDrawing overrides
    virtual void InitTest();
    virtual void DrawTest();
    virtual void OffscreenTest();

    virtual void KeyRelease(int key);

protected:
    virtual void ParseArgs(int argc, char *argv[]);

private:
    void toggleLight();
    void updateShader();
    void addPrim();
    void toggleOverrideShader();

    Hd_TestDriver* _driver;

    std::vector<HdLightingShaderSharedPtr> _lightingShaders;

    TfToken _reprName;
    int _refineLevel;
    HdCullStyle _cullStyle;
    int _currentLight;
    bool _overrideShader;
    std::string _outputFilePrefix;
};

////////////////////////////////////////////////////////////

GLuint vao;

void
My_TestGLDrawing::InitTest()
{
    _driver = new Hd_TestDriver(_reprName);
    Hd_UnitTestDelegate &delegate = _driver->GetDelegate();
    delegate.SetRefineLevel(_refineLevel);

    GfMatrix4d dmat;

    // shaders
    std::string shader1Source(
        "vec4 surfaceShader(vec4 Peye, vec3 Neye, vec4 color, vec4 patchCoord) {\n"
        "    return vec4(SimpleLighting(Peye.xyz, Neye, vec3(1)), 1);\n"
        "}\n"
    );

    std::string shader2Source(
        "vec4 surfaceShader(vec4 Peye, vec3 Neye, vec4 color, vec4 patchCoord) {\n"
        "    return vec4(SimpleLighting(Neye, abs(Peye.xyz), HdGet_fallbackColor()), 1);\n"
        "}\n"
    );
    delegate.AddSurfaceShader(SdfPath("/shader1"), shader1Source, HdShaderParamVector());

    HdShaderParamVector shaderParams;
    shaderParams.push_back(HdShaderParam(TfToken("fallbackColor"), VtValue(GfVec3f(1))));
    delegate.AddSurfaceShader(SdfPath("/shader2"), shader2Source, shaderParams);

    // grids
    {
        dmat.SetTranslate(GfVec3d(-4.5, 0.0, 0.0));
        delegate.BindSurfaceShader(SdfPath("/grid1"), SdfPath("/shader1"));
        delegate.AddGrid(SdfPath("/grid1"), 10, 10, GfMatrix4f(dmat));

        dmat.SetTranslate(GfVec3d(-1.5, 0.0, 0.0));
        delegate.BindSurfaceShader(SdfPath("/grid2"), SdfPath("/shader1"));
        delegate.AddGridWithFaceColor(SdfPath("/grid2"), 10, 10, GfMatrix4f(dmat));

        dmat.SetTranslate(GfVec3d(1.5, 0.0, 0.0));
        delegate.BindSurfaceShader(SdfPath("/grid3"), SdfPath("/shader2"));
        delegate.AddGridWithVertexColor(SdfPath("/grid3"), 10, 10, GfMatrix4f(dmat));

        dmat.SetTranslate(GfVec3d(4.5, 0.0, 0.0));
        delegate.BindSurfaceShader(SdfPath("/grid4"), SdfPath("/shader2"));
        delegate.AddGridWithFaceVaryingColor(SdfPath("/grid4"), 3, 3, GfMatrix4f(dmat));
    }

    // XXX: Setup a VAO, the current drawing engine will not yet do this.
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindVertexArray(0);


    // setup lights
    HdSimpleLightingShaderSharedPtr lightingShader1(new HdSimpleLightingShader());
    HdSimpleLightingShaderSharedPtr lightingShader2(new HdSimpleLightingShader());
    Hd_DefaultLightingShaderSharedPtr defaultShader(new Hd_DefaultLightingShader());

    GlfSimpleLightingContextRefPtr light1 = GlfSimpleLightingContext::New();
    GlfSimpleLightingContextRefPtr light2 = GlfSimpleLightingContext::New();

    GlfSimpleLight l0, l1;

    l0.SetPosition(GfVec4f(1, 0, 1, 0));
    l0.SetDiffuse(GfVec4f(1, 0.2, 0.2, 1.0));
    l0.SetSpecular(GfVec4f(0));
    l0.SetAmbient(GfVec4f(0));
    l1.SetPosition(GfVec4f(0, 1, 1, 0));
    l1.SetDiffuse(GfVec4f(0.2, 1, 0.2, 1.0));
    l1.SetSpecular(GfVec4f(0));
    l1.SetAmbient(GfVec4f(0));
    GlfSimpleLightVector lights;

    lights.push_back(l0);
    light1->SetLights(lights);   // l0
    lights.push_back(l1);
    light2->SetLights(lights);   // l0, l1

    lightingShader1->SetLightingState(light1);
    lightingShader2->SetLightingState(light2);

    _lightingShaders.push_back(lightingShader1);
    _lightingShaders.push_back(lightingShader2);
    _lightingShaders.push_back(defaultShader);
    _driver->GetRenderPassState()->SetLightingShader(
        _lightingShaders[0]);
}

void
My_TestGLDrawing::DrawTest()
{
    GLfloat clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    glClearBufferfv(GL_COLOR, 0, clearColor);

    GLfloat clearDepth[1] = { 1.0f };
    glClearBufferfv(GL_DEPTH, 0, clearDepth);

    int width = GetWidth(), height = GetHeight();
    GfMatrix4d viewMatrix = GetViewMatrix();
    GfMatrix4d projMatrix = GetProjectionMatrix();

    // cull style
    _driver->SetCullStyle(_cullStyle);

    // camera
    _driver->SetCamera(viewMatrix, projMatrix, GfVec4d(0, 0, width, height));

    glViewport(0, 0, width, height);

    glEnable(GL_DEPTH_TEST);

    glBindVertexArray(vao);

    _driver->Draw();

    glBindVertexArray(0);
}

void
My_TestGLDrawing::OffscreenTest()
{
    if (not TF_VERIFY(not _outputFilePrefix.empty())) return;

    DrawTest();
    WriteToFile("color", _outputFilePrefix + "_0.png");

    toggleLight();
    DrawTest();
    WriteToFile("color", _outputFilePrefix + "_1.png");

    toggleLight();
    DrawTest();
    WriteToFile("color", _outputFilePrefix + "_2.png");

    addPrim();
    DrawTest();
    WriteToFile("color", _outputFilePrefix + "_3.png");

    updateShader();
    DrawTest();
    WriteToFile("color", _outputFilePrefix + "_4.png");

    toggleLight();
    DrawTest();
    WriteToFile("color", _outputFilePrefix + "_5.png");

    toggleLight();
    toggleLight();
    toggleOverrideShader();
    DrawTest();
    WriteToFile("color", _outputFilePrefix + "_6.png");

    toggleOverrideShader();
    DrawTest();
    WriteToFile("color", _outputFilePrefix + "_7.png");
}

void
My_TestGLDrawing::KeyRelease(int key)
{
    switch(key) {
    case ' ':
        toggleLight();
        break;
    case 'S':
        updateShader();
        break;
    case 'A':
        addPrim();
        break;
    case 'O':
        toggleOverrideShader();
        break;
    }
}

void
My_TestGLDrawing::toggleLight()
{
    _currentLight = (_currentLight + 1) % int(_lightingShaders.size());
    _driver->GetRenderPassState()->SetLightingShader(
        _lightingShaders[_currentLight]);
}

void
My_TestGLDrawing::updateShader()
{
    Hd_UnitTestDelegate &delegate = _driver->GetDelegate();

    static float m = 1.0f;
    m += 1.0f;
    std::string shader1Source(
        "vec4 surfaceShader(vec4 Peye, vec3 Neye, vec4 color, vec4 patchCoord) {\n"
        "    return vec4(sin(" + std::to_string(m) + "*Peye.xyz), 1);\n"
        "}\n"
    );
    delegate.AddSurfaceShader(SdfPath("/shader1"), shader1Source, HdShaderParamVector());
}

void
My_TestGLDrawing::addPrim()
{
    Hd_UnitTestDelegate &delegate = _driver->GetDelegate();

    GfMatrix4d dmat(1);
    dmat.SetTranslate(GfVec3d(0.0, 3.0, 0.0));

    delegate.BindSurfaceShader(SdfPath("/cube1"), SdfPath("/shader1"));
    delegate.AddCube(SdfPath("/cube1"), GfMatrix4f(dmat));
}

void
My_TestGLDrawing::toggleOverrideShader()
{
    Hd_UnitTestDelegate &delegate = _driver->GetDelegate();

    if (_overrideShader) {
        _driver->GetRenderPassState()->SetOverrideShader(HdShaderSharedPtr());
    } else {
        _driver->GetRenderPassState()->SetOverrideShader(
            delegate.GetRenderIndex().GetShaderFallback());
    }
    _overrideShader = !_overrideShader;
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
ShaderTest(int argc, char *argv[])
{
    My_TestGLDrawing driver;

    driver.RunTest(argc, argv);
}

int main(int argc, char *argv[])
{
    TfErrorMark mark;

    ShaderTest(argc, argv);

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

