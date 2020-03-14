//
// Copyright 2020 Pixar
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

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/gf/matrix4f.h"

#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/rendererPlugin.h"
#include "pxr/imaging/hd/rendererPluginRegistry.h"
#include "pxr/imaging/hd/unitTestDelegate.h"
#include "pxr/imaging/hdx/renderTask.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

void RunHydra()
{
    // Get the renderer plugin and create a new render delegate and index.
    const TfToken tinyRendererPluginId("HdTinyRendererPlugin");

    HdRendererPlugin *rendererPlugin = HdRendererPluginRegistry::GetInstance()
        .GetRendererPlugin(tinyRendererPluginId);
    TF_VERIFY(rendererPlugin != nullptr);

    HdRenderDelegate *renderDelegate = rendererPlugin->CreateRenderDelegate();
    TF_VERIFY(renderDelegate != nullptr);

    HdRenderIndex *renderIndex = HdRenderIndex::New(renderDelegate, {});
    TF_VERIFY(renderIndex != nullptr);

    // Construct a new scene delegate to populate the render index.
    HdUnitTestDelegate *sceneDelegate = new HdUnitTestDelegate(renderIndex, 
        SdfPath::AbsoluteRootPath());
    TF_VERIFY(sceneDelegate != nullptr);

    // Create a cube.
    sceneDelegate->AddCube(SdfPath("/MyCube1"), GfMatrix4f(1));

    // Let's use the HdxRenderTask as an example, and configure it with
    // basic parameters.
    //
    // Another option here could be to create your own task which would
    // look like this :
    //
    // class MyDrawTask final : public HdTask
    // {
    // public:
    //     MyDrawTask(HdRenderPassSharedPtr const &renderPass,
    //                HdRenderPassStateSharedPtr const &renderPassState,
    //                TfTokenVector const &renderTags)
    //     : HdTask(SdfPath::EmptyPath()) { }
    // 
    //     void Sync(HdSceneDelegate* delegate,
    //         HdTaskContext* ctx,
    //         HdDirtyBits* dirtyBits) override { }
    //
    //     void Prepare(HdTaskContext* ctx,
    //         HdRenderIndex* renderIndex) override { }
    //
    //     void Execute(HdTaskContext* ctx) override { }
    // };
    SdfPath renderTask("/renderTask");
    sceneDelegate->AddTask<HdxRenderTask>(renderTask);
    sceneDelegate->UpdateTask(renderTask, HdTokens->params, 
        VtValue(HdxRenderTaskParams()));
    sceneDelegate->UpdateTask(renderTask, 
        HdTokens->collection,
        VtValue(HdRprimCollection(HdTokens->geometry, 
        HdReprSelector(HdReprTokens->refined))));

    // Ask Hydra to execute our render task.
    HdEngine engine;
    HdTaskSharedPtrVector tasks = { renderIndex->GetTask(renderTask) };
    engine.Execute(renderIndex, &tasks);

    // Destroy the data structures
    delete renderIndex;
    delete renderDelegate;
    delete sceneDelegate;
}

int main(int argc, char *argv[])
{
    TfErrorMark mark;
    RunHydra();

    // If no error messages were logged, return success.
    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
