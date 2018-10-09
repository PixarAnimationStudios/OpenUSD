//
// Copyright 2016 Pixar
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
#include "pxr/pxr.h"

#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/drawTarget.h"
#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/garch/glDebugWindow.h"

#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/renderSetupTask.h"
#include "pxr/imaging/hdx/unitTestDelegate.h"

#include "pxr/imaging/hdSt/renderDelegate.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

int main(int argc, char *argv[])
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();

    // prepare GL context
    GarchGLDebugWindow window("Hdx Test", 256, 256);
    window.Init();
    GlfGlewInit();
    glViewport(0, 0, 256, 256);
    // wrap into GlfGLContext so that GlfDrawTarget works
    GlfGLContextSharedPtr ctx = GlfGLContext::GetCurrentGLContext();

    HdEngine engine;
    HdStRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> index(HdRenderIndex::New(&renderDelegate));
    TF_VERIFY(index != nullptr);
    std::unique_ptr<Hdx_UnitTestDelegate> delegate(
                                         new Hdx_UnitTestDelegate(index.get()));

    // prep render task
    SdfPath renderSetupTask1("/renderSetupTask1");
    SdfPath renderTask1("/renderTask1");
    delegate->AddRenderSetupTask(renderSetupTask1);
    delegate->AddRenderTask(renderTask1);
    HdTaskSharedPtrVector tasks;
    tasks.push_back(index->GetTask(renderSetupTask1));
    tasks.push_back(index->GetTask(renderTask1));

    // prep scene
    delegate->AddGrid(SdfPath("/grid"), GfMatrix4d(1));

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
    engine.Execute(*index, tasks);
    drawTarget->Unbind();
    drawTarget->WriteToFile("color", "color1.png");

    // update render param
    VtValue vParam = delegate->GetTaskParam(renderSetupTask1, HdTokens->params);
    HdxRenderTaskParams param = vParam.Get<HdxRenderTaskParams>();
    param.overrideColor = GfVec4f(1, 0, 0, 1);
    delegate->SetTaskParam(renderSetupTask1, HdTokens->params, VtValue(param));

    // draw #2
    drawTarget->Bind();
    glClearBufferfv(GL_COLOR, 0, clearColor);
    glClearBufferfv(GL_DEPTH, 0, clearDepth);
    engine.Execute(*index, tasks);
    drawTarget->Unbind();
    drawTarget->WriteToFile("color", "color2.png");

    // update collection
    HdRprimCollectionVector collections;
    collections.push_back(HdRprimCollection(HdTokens->geometry, 
        HdReprSelector(HdReprTokens->wire)));
    collections.push_back(HdRprimCollection(HdTokens->geometry, 
        HdReprSelector(HdReprTokens->wire)));
    delegate->SetTaskParam(renderTask1, HdTokens->collection, VtValue(collections));

    // draw #3
    drawTarget->Bind();
    glClearBufferfv(GL_COLOR, 0, clearColor);
    glClearBufferfv(GL_DEPTH, 0, clearDepth);
    engine.Execute(*index, tasks);
    drawTarget->Unbind();
    drawTarget->WriteToFile("color", "color3.png");

    GLF_POST_PENDING_GL_ERRORS();

    std::cout << "OK" << std::endl;
}
