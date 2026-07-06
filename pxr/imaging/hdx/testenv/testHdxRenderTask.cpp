//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/garch/glDebugWindow.h"

#include "pxr/imaging/hd/driver.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/renderSetupTask.h"
#include "pxr/imaging/hdx/unitTestDelegate.h"

#include "pxr/imaging/hdSt/renderDelegate.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (testCollection)
);

int main(int argc, char *argv[])
{
    TfErrorMark mark;

    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();

    // prepare GL context
    GarchGLDebugWindow window("Hdx Test", 256, 256);
    window.Init();

    // Hgi and HdDriver should be constructed before HdEngine to ensure they
    // are destructed last. Hgi may be used during engine/delegate destruction.
    HgiUniquePtr hgi = Hgi::CreatePlatformDefaultHgi();
    HdDriver driver{HgiTokens->renderDriver, VtValue(hgi.get())};

    HdStRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> index(
        HdRenderIndex::New(&renderDelegate, {&driver}));
    TF_VERIFY(index != nullptr);
    std::unique_ptr<Hdx_UnitTestDelegate> delegate(
                                         new Hdx_UnitTestDelegate(index.get()));
    HdEngine engine;

    // prep render task
    SdfPath renderSetupTask1("/renderSetupTask1");
    SdfPath renderTask1("/renderTask1");
    delegate->AddRenderSetupTask(renderSetupTask1);
    delegate->AddRenderTask(renderTask1);

    // setup AOVs
    const HdRenderPassAovBindingVector aovBindings =
        delegate->AddAovBindings(GfVec2i(512, 512));
    const SdfPath colorAovId = aovBindings[0].renderBufferId;

    // update viewport param (defaults to (0,0,512,512) otherwise)
    {
        VtValue vParam = delegate->GetTaskParam(renderSetupTask1,
                                                HdTokens->params);
        HdxRenderTaskParams param = vParam.Get<HdxRenderTaskParams>();
        param.viewport = GfVec4d(0, 0, 256, 256);
        param.aovBindings = aovBindings;
        delegate->SetTaskParam(renderSetupTask1, HdTokens->params,
                               VtValue(param));
    }

    HdTaskSharedPtrVector tasks;
    tasks.push_back(index->GetTask(renderSetupTask1));
    tasks.push_back(index->GetTask(renderTask1));

    // prep scene
    delegate->AddGrid(SdfPath("/grid"), GfMatrix4d(1));

    // draw #1
    engine.Execute(index.get(), &tasks);
    delegate->WriteRenderBufferToFile(colorAovId, "color1.png");

    // update render param
    VtValue vParam = delegate->GetTaskParam(renderSetupTask1, HdTokens->params);
    HdxRenderTaskParams param = vParam.Get<HdxRenderTaskParams>();
    param.overrideColor = GfVec4f(1, 0, 0, 1);
    delegate->SetTaskParam(renderSetupTask1, HdTokens->params, VtValue(param));

    // draw #2
    engine.Execute(index.get(), &tasks);
    delegate->WriteRenderBufferToFile(colorAovId, "color2.png");

    // update collection
    index->GetChangeTracker().AddCollection(_tokens->testCollection);
    HdRprimCollection collection(_tokens->testCollection,
        HdReprSelector(HdReprTokens->wire));
    delegate->SetTaskParam(renderTask1, HdTokens->collection, VtValue(collection));

    // draw #3
    engine.Execute(index.get(), &tasks);
    delegate->WriteRenderBufferToFile(colorAovId, "color3.png");

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
