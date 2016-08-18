#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/drawTarget.h"
#include "pxr/imaging/glf/testGLContext.h"
#include "pxr/base/gf/frustum.h"

#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hdx/simpleLightTask.h"
#include "pxr/imaging/hdx/shadowTask.h"
#include "pxr/imaging/hdx/renderSetupTask.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/unitTestDelegate.h"

#include <QApplication>
#include <iostream>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();

    // prepare GL context
    GlfTestGLContext::RegisterGLContextCallbacks();
    GlfGlewInit();
    GlfSharedGLContextScopeHolder sharedContext;

    // prep draw target
    GlfDrawTargetRefPtr drawTarget = GlfDrawTarget::New(GfVec2i(512, 512));
    drawTarget->Bind();
    drawTarget->AddAttachment("color", GL_RGBA, GL_FLOAT, GL_RGBA);
    drawTarget->AddAttachment("depth", GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8,
                              GL_DEPTH24_STENCIL8);
    drawTarget->Unbind();

    GLfloat clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    GLfloat clearDepth[1] = { 1.0f };

    Hdx_UnitTestDelegate delegate;
    HdEngine engine;

    // --------------------------------------------------------------------

    // prep render task and shadow task
    SdfPath simpleLightTask("/simpleLightTask");
    SdfPath shadowTask("/shadowTask");
    SdfPath renderSetupTask("/renderSetupTask");
    SdfPath renderTask("/renderTask");
    delegate.AddSimpleLightTask(simpleLightTask);
    delegate.AddShadowTask(shadowTask);
    delegate.AddRenderSetupTask(renderSetupTask);
    delegate.AddRenderTask(renderTask);
    HdTaskSharedPtrVector tasks;
    tasks.push_back(delegate.GetRenderIndex().GetTask(simpleLightTask));
    tasks.push_back(delegate.GetRenderIndex().GetTask(shadowTask));
    tasks.push_back(delegate.GetRenderIndex().GetTask(renderSetupTask));
    tasks.push_back(delegate.GetRenderIndex().GetTask(renderTask));

    // prep lights
    GlfSimpleLight light1;
    light1.SetDiffuse(GfVec4f(0.5, 0.5, 0.5, 1.0));
    light1.SetPosition(GfVec4f(1,0.5,1,0));
    light1.SetHasShadow(true);
    delegate.AddLight(SdfPath("/light1"), light1);

    // prep scene
    delegate.AddGrid(SdfPath("/grid"),
                     GfMatrix4d(10,0,0,0, 0,10,0,0, 0,0,10,0, 0,0,0,1));
    delegate.AddCube(SdfPath("/cube"),
                     GfMatrix4d( 1,0,0,0, 0,1,0,0,  0,0,1,0, -3,0,5,1));
    delegate.AddTet(SdfPath("/tet"),
                     GfMatrix4d( 1,0,0,0, 0,1,0,0,  0,0,1,0,  3,0,5,1));
    delegate.SetRefineLevel(SdfPath("/cube"), 4);
    delegate.SetRefineLevel(SdfPath("/tet"), 3);

    // camera
    GfFrustum frustum;
    frustum.SetNearFar(GfRange1d(0.1, 1000.0));
    frustum.SetPosition(GfVec3d(0, -5, 10));
    frustum.SetRotation(GfRotation(GfVec3d(1, 0, 0), 45));
    delegate.SetCamera(frustum.ComputeViewMatrix(),
                       frustum.ComputeProjectionMatrix());

    // set renderTask
    delegate.SetTaskParam(
        renderTask, HdTokens->collection,
        VtValue(HdRprimCollection(HdTokens->geometry, HdTokens->refined)));

    // set render setup param
    VtValue vParam = delegate.GetTaskParam(renderSetupTask, HdTokens->params);
    HdxRenderTaskParams param = vParam.Get<HdxRenderTaskParams>();
    param.enableLighting = true;
    delegate.SetTaskParam(renderSetupTask, HdTokens->params, VtValue(param));

    // --------------------------------------------------------------------
    // draw.
    drawTarget->Bind();
    glViewport(0, 0, 512, 512);
    glEnable(GL_DEPTH_TEST);
    glClearBufferfv(GL_COLOR, 0, clearColor);
    glClearBufferfv(GL_DEPTH, 0, clearDepth);

    engine.Execute(delegate.GetRenderIndex(), tasks);

    drawTarget->Unbind();
    drawTarget->WriteToFile("color", "color1.png");

    // --------------------------------------------------------------------
    // add light
    GlfSimpleLight light2;
    light2.SetDiffuse(GfVec4f(0.7, 0.5, 0.3, 1.0));
    light2.SetPosition(GfVec4f(0.3,-0.2,1,0));
    light2.SetHasShadow(true);
    delegate.AddLight(SdfPath("/light2"), light2);

    // --------------------------------------------------------------------
    // draw.
    drawTarget->Bind();
    glViewport(0, 0, 512, 512);
    glEnable(GL_DEPTH_TEST);
    glClearBufferfv(GL_COLOR, 0, clearColor);
    glClearBufferfv(GL_DEPTH, 0, clearDepth);

    engine.Execute(delegate.GetRenderIndex(), tasks);

    drawTarget->Unbind();
    drawTarget->WriteToFile("color", "color2.png");

    GLF_POST_PENDING_GL_ERRORS();

    std::cout << "OK" << std::endl;
}
