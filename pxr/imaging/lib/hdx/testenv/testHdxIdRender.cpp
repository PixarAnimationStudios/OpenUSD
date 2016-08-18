#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/glf/drawTarget.h"

#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/renderSetupTask.h"
#include "pxr/imaging/hdx/unitTestGLDrawing.h"
#include "pxr/imaging/hdx/unitTestDelegate.h"

#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>

class My_TestGLDrawing : public Hdx_UnitTestGLDrawing {
public:
    My_TestGLDrawing() {
        SetCameraRotate(0, 0);
        SetCameraTranslate(GfVec3f(0));
        _reprName = HdTokens->hull;
        _refineLevel = 0;
    }

    struct PickParam {
        GfVec2d location;
        GfVec4d viewport;
    };

    void DrawScene(PickParam const * pickParam = NULL);

    SdfPath PickScene(int pickX, int pickY, int * outInstanceIndex = NULL);

    // Hdx_UnitTestGLDrawing overrides
    virtual void InitTest();
    virtual void UninitTest();
    virtual void DrawTest();
    virtual void OffscreenTest();

    virtual void MousePress(int button, int x, int y);

protected:
    virtual void ParseArgs(int argc, char *argv[]);

private:
    HdEngine _engine;
    Hdx_UnitTestDelegate _delegate;

    TfToken _reprName;
    int _refineLevel;
};

////////////////////////////////////////////////////////////

GLuint vao;

static GfMatrix4d
_GetTranslate(float tx, float ty, float tz)
{
    GfMatrix4d m(1.0f);
    m.SetRow(3, GfVec4f(tx, ty, tz, 1.0));
    return m;
}

void
My_TestGLDrawing::InitTest()
{
    _delegate.SetRefineLevel(_refineLevel);

    // prepare render task
    SdfPath renderSetupTask("/renderSetupTask");
    SdfPath renderTask("/renderTask");
    _delegate.AddRenderSetupTask(renderSetupTask);
    _delegate.AddRenderTask(renderTask);

    // render task parameters.
    HdxRenderTaskParams param
        = _delegate.GetTaskParam(
            renderSetupTask, HdTokens->params).Get<HdxRenderTaskParams>();
    param.enableLighting = true; // use default lighting
    _delegate.SetTaskParam(renderSetupTask, HdTokens->params, VtValue(param));
    _delegate.SetTaskParam(renderTask, HdTokens->collection,
                           VtValue(HdRprimCollection(HdTokens->geometry, _reprName)));

    // prepare scene
    _delegate.AddCube(SdfPath("/cube0"), _GetTranslate( 5, 0, 5));
    _delegate.AddCube(SdfPath("/cube1"), _GetTranslate(-5, 0, 5));
    _delegate.AddCube(SdfPath("/cube2"), _GetTranslate(-5, 0,-5));
    _delegate.AddCube(SdfPath("/cube3"), _GetTranslate( 5, 0,-5));

    {
        _delegate.AddInstancer(SdfPath("/instancerTop"));
        _delegate.AddCube(SdfPath("/protoTop"),
                         GfMatrix4d(1), false, SdfPath("/instancerTop"));

        std::vector<SdfPath> prototypes;
        prototypes.push_back(SdfPath("/protoTop"));

        VtVec3fArray scale(3);
        VtVec4fArray rotate(3);
        VtVec3fArray translate(3);
        VtIntArray prototypeIndex(3);

        scale[0] = GfVec3f(1);
        rotate[0] = GfVec4f(0);
        translate[0] = GfVec3f(3, 0, 2);
        prototypeIndex[0] = 0;

        scale[1] = GfVec3f(1);
        rotate[1] = GfVec4f(0);
        translate[1] = GfVec3f(0, 0, 2);
        prototypeIndex[1] = 0;

        scale[2] = GfVec3f(1);
        rotate[2] = GfVec4f(0);
        translate[2] = GfVec3f(-3, 0, 2);
        prototypeIndex[2] = 0;

        _delegate.SetInstancerProperties(SdfPath("/instancerTop"),
                                        prototypeIndex,
                                        scale, rotate, translate);
    }

    {
        _delegate.AddInstancer(SdfPath("/instancerBottom"));
        _delegate.AddCube(SdfPath("/protoBottom"),
                         GfMatrix4d(1), false, SdfPath("/instancerBottom"));

        std::vector<SdfPath> prototypes;
        prototypes.push_back(SdfPath("/protoBottom"));

        VtVec3fArray scale(3);
        VtVec4fArray rotate(3);
        VtVec3fArray translate(3);
        VtIntArray prototypeIndex(3);

        scale[0] = GfVec3f(1);
        rotate[0] = GfVec4f(0);
        translate[0] = GfVec3f(3, 0, -2);
        prototypeIndex[0] = 0;

        scale[1] = GfVec3f(1);
        rotate[1] = GfVec4f(0);
        translate[1] = GfVec3f(0, 0, -2);
        prototypeIndex[1] = 0;

        scale[2] = GfVec3f(1);
        rotate[2] = GfVec4f(0);
        translate[2] = GfVec3f(-3, 0, -2);
        prototypeIndex[2] = 0;

        _delegate.SetInstancerProperties(SdfPath("/instancerBottom"),
                                        prototypeIndex,
                                        scale, rotate, translate);
    }

    SetCameraTranslate(GfVec3f(0, 0, -20));

    // XXX: Setup a VAO, the current drawing engine will not yet do this.
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindVertexArray(0);
}

void
My_TestGLDrawing::UninitTest()
{
}

void
My_TestGLDrawing::DrawTest()
{
    GLfloat clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    glClearBufferfv(GL_COLOR, 0, clearColor);

    GLfloat clearDepth[1] = { 1.0f };
    glClearBufferfv(GL_DEPTH, 0, clearDepth);

    DrawScene();
}

void
My_TestGLDrawing::OffscreenTest()
{
    SdfPath primId;
    int instanceIndex = -1;

    primId = PickScene(180, 100, &instanceIndex);
    TF_VERIFY(primId == SdfPath("/cube1") and instanceIndex == 0);

    primId = PickScene(250, 190, &instanceIndex);
    TF_VERIFY(primId == SdfPath("/protoTop") and instanceIndex == 2);

    primId = PickScene(320, 290, &instanceIndex);
    TF_VERIFY(primId == SdfPath("/protoBottom") and instanceIndex == 1);
}

void
My_TestGLDrawing::
DrawScene(PickParam const * pickParam)
{
    int width = GetWidth(), height = GetHeight();

    GfMatrix4d viewMatrix = GetViewMatrix();

    GfFrustum frustum = GetFrustum();
    GfVec4d viewport(0, 0, width, height);

    if (pickParam) {
        frustum = frustum.ComputeNarrowedFrustum(
            GfVec2d((2.0 * pickParam->location[0]) / width - 1.0,
                    (2.0 * (height-pickParam->location[1])) / height - 1.0),
            GfVec2d(1.0 / width, 1.0 / height));
        viewport = pickParam->viewport;
    }

    GfMatrix4d projMatrix = frustum.ComputeProjectionMatrix();
    _delegate.SetCamera(viewMatrix, projMatrix);

    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

    HdTaskSharedPtrVector tasks;
    SdfPath renderSetupTask("/renderSetupTask");
    SdfPath renderTask("/renderTask");
    tasks.push_back(_delegate.GetRenderIndex().GetTask(renderSetupTask));
    tasks.push_back(_delegate.GetRenderIndex().GetTask(renderTask));

    HdxRenderTaskParams param
        = _delegate.GetTaskParam(
            renderSetupTask, HdTokens->params).Get<HdxRenderTaskParams>();
    param.enableIdRender = (pickParam != NULL);
    param.viewport = viewport;
    _delegate.SetTaskParam(renderSetupTask, HdTokens->params, VtValue(param));


    glEnable(GL_DEPTH_TEST);

    glBindVertexArray(vao);

    _engine.Execute(_delegate.GetRenderIndex(), tasks);

    glBindVertexArray(0);
}

SdfPath
My_TestGLDrawing::PickScene(int pickX, int pickY, int * outInstanceIndex)
{
    int width = 128;
    int height = 128;

    GlfDrawTargetRefPtr drawTarget = GlfDrawTarget::New(GfVec2i(width, height));
    drawTarget->Bind();
    drawTarget->AddAttachment(
        "primId", GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8);
    drawTarget->AddAttachment(
        "instanceId", GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8);
    drawTarget->AddAttachment(
        "depth", GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_COMPONENT32F);
    drawTarget->Unbind();

    drawTarget->Bind();

    GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, drawBuffers);

    glEnable(GL_DEPTH_TEST);

    GLfloat clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glClearBufferfv(GL_COLOR, 0, clearColor);
    glClearBufferfv(GL_COLOR, 1, clearColor);

    GLfloat clearDepth[1] = { 1.0f };
    glClearBufferfv(GL_DEPTH, 0, clearDepth);

    PickParam pickParam;
    pickParam.location = GfVec2d(pickX, pickY);
    pickParam.viewport = GfVec4d(0, 0, width, height);

    DrawScene(&pickParam);

    drawTarget->Unbind();

    GLubyte primId[width*height*4];
    glBindTexture(GL_TEXTURE_2D,
        drawTarget->GetAttachments().at("primId")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, primId);

    GLubyte instanceId[width*height*4];
    glBindTexture(GL_TEXTURE_2D,
        drawTarget->GetAttachments().at("instanceId")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, instanceId);

    GLfloat depths[width*height];
    glBindTexture(GL_TEXTURE_2D,
        drawTarget->GetAttachments().at("depth")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, depths);

    double zMin = 1.0;
    int zMinIndex = -1;
    for (int y=0, i=0; y<height; y++) {
        for (int x=0; x<width; x++, i++) {
            if (depths[i] < zMin) {
                    zMin = depths[i];
                    zMinIndex = i;
            }
        }
    }

    bool didHit = (zMin < 1.0);

    SdfPath result;
    if (didHit) {
        int idIndex = zMinIndex*4;

        GfVec4i primIdColor(
            primId[idIndex+0],
            primId[idIndex+1],
            primId[idIndex+2],
            primId[idIndex+3]);

        GfVec4i instanceIdColor(
            instanceId[idIndex+0],
            instanceId[idIndex+1],
            instanceId[idIndex+2],
            instanceId[idIndex+3]);

        result = _delegate.GetRenderIndex().GetPrimPathFromPrimIdColor(
                        primIdColor, instanceIdColor, outInstanceIndex);
    }

    return result;
}

void
My_TestGLDrawing::MousePress(int button, int x, int y)
{
    Hdx_UnitTestGLDrawing::MousePress(button, x, y);
    int instanceIndex = 0;
    SdfPath primId = PickScene(x, y, &instanceIndex);

    if (not primId.IsEmpty()) {
        std::cout << "pick(" << x << ", " << y << "): "
                  << "primId == " << primId << " "
                  << "instance == " << instanceIndex << "\n";
    }
}

void
My_TestGLDrawing::ParseArgs(int argc, char *argv[])
{
    for (int i=0; i<argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--repr") {
            _reprName = TfToken(argv[++i]);
        } else if (arg == "--refineLevel") {
            _refineLevel = atoi(argv[++i]);
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

