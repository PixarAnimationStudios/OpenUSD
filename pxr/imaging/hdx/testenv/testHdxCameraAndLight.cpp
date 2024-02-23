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

#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/testGLContext.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/driver.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hdSt/light.h"
#include "pxr/imaging/hdSt/renderDelegate.h"

#include "pxr/imaging/hdx/renderSetupTask.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/unitTestDelegate.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>
#include <memory>

#define VERIFY_PERF_COUNT(token, count) \
            TF_VERIFY(perfLog.GetCounter(token) == count, \
                    "expected %d found %.0f", \
                    count,\
                    perfLog.GetCounter(token));

PXR_NAMESPACE_USING_DIRECTIVE

static void CameraAndLightTest()
{
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

    HdChangeTracker& tracker = index->GetChangeTracker();
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    HdRprimCollection collection(HdTokens->geometry, 
        HdReprSelector(HdReprTokens->hull));
    HdEngine engine;
    
    // --------------------------------------------------------------------

    // prep tasks
    SdfPath renderSetupTask("/renderSetupTask");
    SdfPath renderTask("/renderTask");
    delegate->AddRenderSetupTask(renderSetupTask);
    delegate->AddRenderTask(renderTask);
    HdTaskSharedPtrVector tasks;
    tasks.push_back(index->GetTask(renderSetupTask));
    tasks.push_back(index->GetTask(renderTask));
    
    // Setup AOVs
    const SdfPath colorAovId = SdfPath("/aov_color");
    const SdfPath depthAovId = SdfPath("/aov_depth");
    HdRenderPassAovBindingVector aovBindings;

    // Color AOV
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

    // Depth AOV
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
    
    // Set render task param
    delegate->SetTaskParam(
        renderTask, HdTokens->collection, VtValue(collection));

    // Set render setup param
    VtValue vParam = delegate->GetTaskParam(renderSetupTask, HdTokens->params);
    HdxRenderTaskParams param = vParam.Get<HdxRenderTaskParams>();
    param.enableLighting = true;
    param.aovBindings = aovBindings;
    delegate->SetTaskParam(renderSetupTask, HdTokens->params, VtValue(param));
    
    // Set up scene
    GfMatrix4d tx(1.0f);
    tx.SetRow(3, GfVec4f(5, 0, 5, 1.0));
    SdfPath cube("/geometry");
    delegate->AddCube(cube, tx);

    SdfPath camera("/camera_test");
    SdfPath light("/light");

    delegate->AddCamera(camera);
    delegate->AddLight(light, GlfSimpleLight());
    delegate->SetLight(light, HdLightTokens->shadowCollection,
                      VtValue(HdRprimCollection(HdTokens->geometry,
                                        HdReprSelector(HdReprTokens->hull))));

    // Draw
    engine.Execute(index.get(), &tasks);

    VERIFY_PERF_COUNT(HdPerfTokens->rebuildBatches, 1);

    // Update camera matrix
    delegate->SetCamera(camera, GfMatrix4d(2), GfMatrix4d(2));
    tracker.MarkSprimDirty(camera, HdCamera::DirtyTransform);
    tracker.MarkSprimDirty(camera, HdCamera::DirtyParams);

    engine.Execute(index.get(), &tasks);

    // batch should not be rebuilt
    VERIFY_PERF_COUNT(HdPerfTokens->rebuildBatches, 1);

    // Update shadow collection
    delegate->SetLight(light, HdLightTokens->shadowCollection,
                      VtValue(HdRprimCollection(HdTokens->geometry,
                        HdReprSelector(HdReprTokens->refined))));
    tracker.MarkSprimDirty(light, HdLight::DirtyCollection);

    engine.Execute(index.get(), &tasks);

    // batch rebuilt
    VERIFY_PERF_COUNT(HdPerfTokens->rebuildBatches, 2);

    // Update shadow collection again with the same data
    delegate->SetLight(light, HdLightTokens->shadowCollection,
                      VtValue(HdRprimCollection(HdTokens->geometry,
                                HdReprSelector(HdReprTokens->refined))));
    tracker.MarkSprimDirty(light, HdLight::DirtyCollection);

    engine.Execute(index.get(), &tasks);

    // batch should not be rebuilt
    VERIFY_PERF_COUNT(HdPerfTokens->rebuildBatches, 2);
}

int main()
{
    TfErrorMark mark;

    GlfTestGLContext::RegisterGLContextCallbacks();
    GlfSharedGLContextScopeHolder sharedContext;

    CameraAndLightTest();

    TF_VERIFY(mark.IsClean());

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

