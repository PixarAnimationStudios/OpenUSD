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
#include "pxr/imaging/garch/glDebugWindow.h"
#include "pxr/base/gf/frustum.h"

#include "pxr/imaging/hd/driver.h"
#include "pxr/imaging/hd/engine.h"

#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdSt/light.h"

#include "pxr/imaging/hdx/simpleLightTask.h"
#include "pxr/imaging/hdx/shadowTask.h"
#include "pxr/imaging/hdx/renderSetupTask.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/unitTestDelegate.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

int main(int argc, char *argv[])
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();

    // prepare GL context
    GarchGLDebugWindow window("Hdx Test", 512, 512);
    window.Init();

    // Hgi and HdDriver should be constructed before HdEngine to ensure they
    // are destructed last. Hgi may be used during engine/delegate destruction.
    HgiUniquePtr hgi = Hgi::CreatePlatformDefaultHgi();
    HdDriver driver{HgiTokens->renderDriver, VtValue(hgi.get())};

    HdStRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> index(
        HdRenderIndex::New(&renderDelegate, {&driver}));
    TF_VERIFY(index);
    std::unique_ptr<Hdx_UnitTestDelegate> delegate(
                                         new Hdx_UnitTestDelegate(index.get()));
    HdEngine engine;

    // --------------------------------------------------------------------

    // prep render task and shadow task
    SdfPath simpleLightTask("/simpleLightTask");
    SdfPath shadowTask("/shadowTask");
    SdfPath renderSetupTask("/renderSetupTask");
    SdfPath renderTask("/renderTask");
    delegate->AddSimpleLightTask(simpleLightTask);
    delegate->AddShadowTask(shadowTask);
    delegate->AddRenderSetupTask(renderSetupTask);
    delegate->AddRenderTask(renderTask);
    HdTaskSharedPtrVector tasks;
    tasks.push_back(index->GetTask(simpleLightTask));
    tasks.push_back(index->GetTask(shadowTask));
    tasks.push_back(index->GetTask(renderSetupTask));
    tasks.push_back(index->GetTask(renderTask));

    // setup AOVs
    const SdfPath colorAovId = SdfPath("/aov_color");
    const SdfPath depthAovId = SdfPath("/aov_depth");
    HdRenderPassAovBindingVector aovBindings;

    // color AOV
    {
        HdRenderPassAovBinding colorAovBinding;
        const HdAovDescriptor colorAovDesc = 
            renderDelegate.GetDefaultAovDescriptor(HdAovTokens->color);
        colorAovBinding.aovName = HdAovTokens->color;
        colorAovBinding.clearValue = VtValue(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
        colorAovBinding.renderBufferId = colorAovId;
        colorAovBinding.aovSettings = colorAovDesc.aovSettings;
        aovBindings.push_back(std::move(colorAovBinding));

        HdRenderBufferDescriptor colorRbDesc;
        colorRbDesc.dimensions = GfVec3i(512, 512, 1);
        colorRbDesc.format = colorAovDesc.format;
        colorRbDesc.multiSampled = false;
        delegate->AddRenderBuffer(colorAovId, colorRbDesc);
    }

    // depth AOV
    {
        HdRenderPassAovBinding depthAovBinding;
        const HdAovDescriptor depthAovDesc = 
            renderDelegate.GetDefaultAovDescriptor(HdAovTokens->depth);
        depthAovBinding.aovName = HdAovTokens->depth;
        depthAovBinding.clearValue = VtValue(1.f);
        depthAovBinding.renderBufferId = depthAovId;
        depthAovBinding.aovSettings = depthAovDesc.aovSettings;
        aovBindings.push_back(std::move(depthAovBinding));

        HdRenderBufferDescriptor depthRbDesc;
        depthRbDesc.dimensions = GfVec3i(512, 512, 1);
        depthRbDesc.format = depthAovDesc.format;
        depthRbDesc.multiSampled = false;
        delegate->AddRenderBuffer(depthAovId, depthRbDesc);
    }

    // prep lights
    GlfSimpleLight light1;
    light1.SetDiffuse(GfVec4f(0.5, 0.5, 0.5, 1.0));
    light1.SetPosition(GfVec4f(1,0.5,1,0));
    light1.SetHasShadow(true);
    delegate->AddLight(SdfPath("/light1"), light1);

    // prep scene
    delegate->AddGrid(SdfPath("/grid"),
                     GfMatrix4d(10,0,0,0, 0,10,0,0, 0,0,10,0, 0,0,0,1));
    delegate->AddCube(SdfPath("/cube"),
                     GfMatrix4d( 1,0,0,0, 0,1,0,0,  0,0,1,0, -3,0,5,1));
    delegate->AddTet(SdfPath("/tet"),
                     GfMatrix4d( 1,0,0,0, 0,1,0,0,  0,0,1,0,  3,0,5,1));
    delegate->SetRefineLevel(SdfPath("/cube"), 4);
    delegate->SetRefineLevel(SdfPath("/tet"), 3);

    // camera
    GfFrustum frustum;
    frustum.SetNearFar(GfRange1d(0.1, 1000.0));
    frustum.SetPosition(GfVec3d(0, -5, 10));
    frustum.SetRotation(GfRotation(GfVec3d(1, 0, 0), 45));
    delegate->SetCamera(frustum.ComputeViewMatrix(),
                       frustum.ComputeProjectionMatrix());

    // set renderTask
    delegate->SetTaskParam(
        renderTask, HdTokens->collection,
        VtValue(HdRprimCollection(HdTokens->geometry, 
                HdReprSelector(HdReprTokens->refined))));

    // set render setup param
    VtValue vParam = delegate->GetTaskParam(renderSetupTask, HdTokens->params);
    HdxRenderTaskParams param = vParam.Get<HdxRenderTaskParams>();
    param.enableLighting = true;
    param.aovBindings = aovBindings;
    delegate->SetTaskParam(renderSetupTask, HdTokens->params, VtValue(param));

    // --------------------------------------------------------------------
    // draw.
    engine.Execute(index.get(), &tasks);
    delegate->WriteRenderBufferToFile(colorAovId, "color1.png");
    
    // --------------------------------------------------------------------
    // add light
    GlfSimpleLight light2;
    light2.SetDiffuse(GfVec4f(0.7, 0.5, 0.3, 1.0));
    light2.SetPosition(GfVec4f(0.3,-0.2,1,0));
    light2.SetHasShadow(true);
    delegate->AddLight(SdfPath("/light2"), light2);

    // --------------------------------------------------------------------
    // draw.
    engine.Execute(index.get(), &tasks);
    delegate->WriteRenderBufferToFile(colorAovId, "color2.png");

    // --------------------------------------------------------------------
    // move light
    light2.SetPosition(GfVec4f(-0.3,-0.2,1,0));
    delegate->SetLight(SdfPath("/light2"), HdLightTokens->params,
                      VtValue(light2));

    // --------------------------------------------------------------------
    // draw.
    engine.Execute(index.get(), &tasks);
    delegate->WriteRenderBufferToFile(colorAovId, "color3.png");

    // --------------------------------------------------------------------
    // remove first light
    delegate->RemoveLight(SdfPath("/light1"));

    // --------------------------------------------------------------------
    // draw.
    engine.Execute(index.get(), &tasks);
    delegate->WriteRenderBufferToFile(colorAovId, "color4.png");

    // --------------------------------------------------------------------

    std::cout << "OK" << std::endl;
}
