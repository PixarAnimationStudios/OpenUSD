//
// Copyright 2021 Pixar
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

#include "pxr/imaging/garch/glApi.h"

#include "pxr/usdImaging/usdImaging/delegate.h"

#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdSt/renderPass.h"
#include "pxr/imaging/hdSt/renderPassState.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/imaging/hd/driver.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/garch/glDebugWindow.h"
#include "pxr/imaging/glf/contextCaps.h"
#include "pxr/imaging/glf/drawTarget.h"
#include "pxr/imaging/glf/glContext.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/frustum.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

/// A simple drawing task that just executes a render pass.
class UsdImagingGL_DrawTask final : public HdTask
{
public:
    UsdImagingGL_DrawTask(HdRenderPassSharedPtr const &renderPass,
                        HdRenderPassStateSharedPtr const &renderPassState)
        : HdTask(SdfPath::EmptyPath())
        , _renderPass(renderPass)
        , _renderPassState(renderPassState)
    {
    }

    virtual void Sync(HdSceneDelegate* delegate,
                      HdTaskContext* ctx,
                      HdDirtyBits* dirtyBits) override {
        _renderPass->Sync();

        *dirtyBits = HdChangeTracker::Clean;
    }

    virtual void Prepare(HdTaskContext* ctx,
                         HdRenderIndex* renderIndex) override {
        _renderPassState->Prepare(renderIndex->GetResourceRegistry());
    }

    virtual void Execute(HdTaskContext* ctx) override {
        _renderPass->Execute(_renderPassState, GetRenderTags());
    }

private:
    HdRenderPassSharedPtr _renderPass;
    HdRenderPassStateSharedPtr _renderPassState;
};

class Offscreen {
public:
    Offscreen(std::string const &outPrefix) {
        _count = 0;
        _outPrefix = outPrefix;
        _drawTarget = GlfDrawTarget::New(GfVec2i(512, 512));
        _drawTarget->Bind();
        _drawTarget->AddAttachment("color", GL_RGBA, GL_FLOAT, GL_RGBA);
        _drawTarget->AddAttachment("depth", GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8,
                                   GL_DEPTH24_STENCIL8);
        _drawTarget->Unbind();
    }

    void Begin() {
        GLfloat clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
        GLfloat clearDepth[1] = { 1.0f };

        _drawTarget->Bind();
        glClearBufferfv(GL_COLOR, 0, clearColor);
        glClearBufferfv(GL_DEPTH, 0, clearDepth);
    }

    void End() {
        _drawTarget->Unbind();

        if (_outPrefix.size() > 0) {
            std::string filename = TfStringPrintf(
                "%s_%d.png", _outPrefix.c_str(), _count);
            _drawTarget->WriteToFile("color", filename);
            std::cerr << "**Write to " << filename << "\n";
        }
        ++_count;
    }

private:
    int _count;
    std::string _outPrefix;
    GlfDrawTargetRefPtr _drawTarget;

};

int main(int argc, char *argv[])
{
    GarchGLDebugWindow window("UsdImaging Test", 512, 512);
    window.Init();
    GarchGLApiLoad();

    // wrap into GlfGLContext so that GlfDrawTarget works
    GlfGLContextSharedPtr ctx = GlfGLContext::GetCurrentGLContext();
    GlfContextCaps::InitInstance();


    std::string outPrefix;
    std::string filePath;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--output") {
            outPrefix = argv[++i];
        } else {
            filePath = argv[i];
        }
    }

    if (filePath.empty()) {
        std::cout << "Usage: " << argv[0] << " [--output <filename>] stage.usd\n";
        return EXIT_FAILURE;
    }

    UsdStageRefPtr stage = UsdStage::Open(filePath);

    // Hgi and HdDriver should be constructed before HdEngine to ensure they
    // are destructed last. Hgi may be used during engine/delegate destruction.
    HgiUniquePtr hgi = Hgi::CreatePlatformDefaultHgi();
    HdDriver driver{HgiTokens->renderDriver, VtValue(hgi.get())};

    HdEngine engine;
    HdStRenderDelegate renderDelegate;

    std::unique_ptr<HdRenderIndex> renderIndex(
        HdRenderIndex::New(&renderDelegate, {&driver}));
    TF_VERIFY(renderIndex);
    std::unique_ptr<UsdImagingDelegate> delegate(
                          new UsdImagingDelegate(renderIndex.get(),
                                                  SdfPath::AbsoluteRootPath()));
    delegate->Populate(stage->GetPseudoRoot());
    delegate->SetTime(1.0);

    // prep draw target
    Offscreen offscreen(outPrefix);

    HdRenderPassSharedPtr renderPass(
        new HdSt_RenderPass(
            &delegate->GetRenderIndex(),
            HdRprimCollection(HdTokens->geometry, 
                HdReprSelector(HdReprTokens->smoothHull))));
    HdStRenderPassStateSharedPtr state(new HdStRenderPassState());

    HdTaskSharedPtr drawTask = std::make_shared<UsdImagingGL_DrawTask>(
        renderPass, state);
    HdTaskSharedPtrVector tasks = { drawTask };

    GfMatrix4d viewMatrix;
    viewMatrix.SetLookAt(GfVec3d(10, 20, 20),
                         GfVec3d(10, 0, 0),
                         GfVec3d(0, 1, 0));
    GfFrustum frustum;
    frustum.SetPerspective(60.0, true, 1.0, 0.1, 100.0);
    state->SetCameraFramingState(viewMatrix,
                                 frustum.ComputeProjectionMatrix(),
                                 GfVec4d(0, 0, 512, 512),
                                 HdRenderPassState::ClipPlanesVector());

    // initial draw
    glViewport(0, 0, 512, 512);
    glEnable(GL_DEPTH_TEST);

    offscreen.Begin();
    engine.Execute(&delegate->GetRenderIndex(), &tasks);
    offscreen.End();

    /*  in test.usda

       /Cube
       /Foo/X1/C1     (instance)
       /Foo/X2/C2     (instance)
       /Foo/X3/C3     (instance)
       /Foo/Bar/C
       /Foo/Bar/X4/C4 (instance)
       /Bar/C
       /Bar/X5/C5     (instance)
     */

    // Set root transform
    delegate->SetRootTransform(GfMatrix4d(1,0,0,0,0,1,0,0,0,0,1,0,1,0,0,1));

    offscreen.Begin();
    engine.Execute(&delegate->GetRenderIndex(), &tasks);
    offscreen.End();

    // Reset root transform
    delegate->SetRootTransform(GfMatrix4d(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1));

    offscreen.Begin();
    engine.Execute(&delegate->GetRenderIndex(), &tasks);
    offscreen.End();

    // Set rigid xform override
    UsdImagingDelegate::RigidXformOverridesMap overrides;
    overrides[SdfPath("/Foo/X2")] = GfMatrix4d(1,0,0,0,0,1,0,0,0,0,1,0, 1,0, 0,1);
    overrides[SdfPath("/Bar")]    = GfMatrix4d(1,0,0,0,0,1,0,0,0,0,1,0, 0,5,-5,1);

    delegate->SetRigidXformOverrides(overrides);

    offscreen.Begin();
    engine.Execute(&delegate->GetRenderIndex(), &tasks);
    offscreen.End();

    // Set root transform again (+rigid xform)
    delegate->SetRootTransform(GfMatrix4d(1,0,0,0,0,1,0,0,0,0,1,0,2,0,0,1));

    offscreen.Begin();
    engine.Execute(&delegate->GetRenderIndex(), &tasks);
    offscreen.End();

    // Invis cube
    SdfPathVector invisedPaths;
    invisedPaths.push_back(SdfPath("/Cube"));
    delegate->SetInvisedPrimPaths(invisedPaths);

    offscreen.Begin();
    engine.Execute(&delegate->GetRenderIndex(), &tasks);
    offscreen.End();

    // Invis instances
    invisedPaths.push_back(SdfPath("/Foo/X2"));
    invisedPaths.push_back(SdfPath("/Foo/Bar/X4/C4"));
    delegate->SetInvisedPrimPaths(invisedPaths);

    offscreen.Begin();
    engine.Execute(&delegate->GetRenderIndex(), &tasks);
    offscreen.End();

    // un-invis
    delegate->SetInvisedPrimPaths(SdfPathVector());
    offscreen.Begin();
    engine.Execute(&delegate->GetRenderIndex(), &tasks);
    offscreen.End();

    // Set rigid xform override, overlapped
    overrides.clear();

    overrides[SdfPath("/Foo")]
        = GfMatrix4d(1,0,0,0,0,1,0,0,0,0,1,0,1,0,0,1);
    overrides[SdfPath("/Foo/Bar")]
        = GfMatrix4d(1,0,0,0,0,1,0,0,0,0,1,0,0,1,0,1);
    overrides[SdfPath("/Foo/Bar/X4")]
        = GfMatrix4d(1,0,0,0,0,1,0,0,0,0,1,0,0,0,6,1);

    delegate->SetRigidXformOverrides(overrides);

    offscreen.Begin();
    engine.Execute(&delegate->GetRenderIndex(), &tasks);
    offscreen.End();

    std::cout << "OK" << std::endl;
    return EXIT_SUCCESS;
}

