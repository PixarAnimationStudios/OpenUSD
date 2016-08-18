#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/drawTarget.h"
#include "pxr/imaging/glf/testGLContext.h"

#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/renderSetupTask.h"
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

    Hdx_UnitTestDelegate delegate;
    HdEngine engine;

    // prep render task
    SdfPath renderSetupTask1("/renderSetupTask1");
    SdfPath renderTask1("/renderTask1");
    delegate.AddRenderSetupTask(renderSetupTask1);
    delegate.AddRenderTask(renderTask1);
    HdTaskSharedPtrVector tasks;
    tasks.push_back(delegate.GetRenderIndex().GetTask(renderSetupTask1));
    tasks.push_back(delegate.GetRenderIndex().GetTask(renderTask1));

    // prep scene
    delegate.AddGrid(SdfPath("/grid"), GfMatrix4d(1));

    // prep draw target
    GlfDrawTargetRefPtr drawTarget = GlfDrawTarget::New(GfVec2i(512, 512));
    drawTarget->Bind();
    drawTarget->AddAttachment("color", GL_RGBA, GL_FLOAT, GL_RGBA);
    drawTarget->AddAttachment("depth", GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8,
                              GL_DEPTH24_STENCIL8);
    drawTarget->Unbind();

    GLfloat clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    GLfloat clearDepth[1] = { 1.0f };

    // draw #1
    drawTarget->Bind();
    glClearBufferfv(GL_COLOR, 0, clearColor);
    glClearBufferfv(GL_DEPTH, 0, clearDepth);
    engine.Execute(delegate.GetRenderIndex(), tasks);
    drawTarget->Unbind();
    drawTarget->WriteToFile("color", "color1.png");

    // update render param
    VtValue vParam = delegate.GetTaskParam(renderSetupTask1, HdTokens->params);
    HdxRenderTaskParams param = vParam.Get<HdxRenderTaskParams>();
    param.overrideColor = GfVec4f(1, 0, 0, 1);
    delegate.SetTaskParam(renderSetupTask1, HdTokens->params, VtValue(param));

    // draw #2
    drawTarget->Bind();
    glClearBufferfv(GL_COLOR, 0, clearColor);
    glClearBufferfv(GL_DEPTH, 0, clearDepth);
    engine.Execute(delegate.GetRenderIndex(), tasks);
    drawTarget->Unbind();
    drawTarget->WriteToFile("color", "color2.png");

    // update collection
    HdRprimCollectionVector collections;
    collections.push_back(HdRprimCollection(HdTokens->geometry, HdTokens->wire));
    collections.push_back(HdRprimCollection(HdTokens->geometry, HdTokens->wire));
    delegate.SetTaskParam(renderTask1, HdTokens->collection, VtValue(collections));

    // draw #3
    drawTarget->Bind();
    glClearBufferfv(GL_COLOR, 0, clearColor);
    glClearBufferfv(GL_DEPTH, 0, clearDepth);
    engine.Execute(delegate.GetRenderIndex(), tasks);
    drawTarget->Unbind();
    drawTarget->WriteToFile("color", "color3.png");

    GLF_POST_PENDING_GL_ERRORS();

    std::cout << "OK" << std::endl;
}
