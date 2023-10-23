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
#include "pxr/pxr.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/unitTestDelegate.h"

#include "pxr/imaging/hdSt/renderPass.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3d.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>
#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

struct HdSt_DrawTaskParams
{
    HdReprSelector displayStyle;
    HdRenderPassStateSharedPtr renderPassState;
};

bool operator==(const HdSt_DrawTaskParams& lhs,
                const HdSt_DrawTaskParams& rhs)
{
    return lhs.displayStyle == rhs.displayStyle &&
           lhs.renderPassState == rhs.renderPassState;
}

bool operator!=(const HdSt_DrawTaskParams& lhs,
                const HdSt_DrawTaskParams& rhs)
{
    return !(lhs == rhs);
}

// A drawing task that has multiple render passes corressponding to each
// material tag.
using HdRenderPassSharedPtrVector = std::vector<HdRenderPassSharedPtr>;

class HdSt_MyDrawTask final : public HdTask
{
public:
    HdSt_MyDrawTask(HdSceneDelegate* delegate, SdfPath const &id)
    : HdTask(id)
    , _renderTags({HdRenderTagTokens->geometry}) {}

    // ------- HdTask virtual API --------
    void Sync(HdSceneDelegate *sd,
              HdTaskContext *ctx,
              HdDirtyBits *dirtyBits) override
    {
        const bool dirtyParams = *dirtyBits & HdChangeTracker::DirtyParams;
        if (dirtyParams) {
            _GetTaskParams(sd, &_params);
        }
        if (*dirtyBits & HdChangeTracker::DirtyRenderTags) {
            _renderTags = _GetTaskRenderTags(sd); 
        }

        // Create a render pass for each material tag if we haven't.
        if (_renderPasses.empty()) {
            HdRprimCollection col(HdTokens->geometry, _params.displayStyle);

            for (TfToken const& tag : HdStMaterialTagTokens->allTokens) {
                col.SetMaterialTag(tag);

                _renderPasses.push_back( std::make_shared<HdSt_RenderPass>(
                        &(sd->GetRenderIndex()), col) );
            }
        } else if (dirtyParams) {
            // Update repr on the collections for each render pass.
            for (auto &pass : _renderPasses) {
                HdRprimCollection col = pass->GetRprimCollection();
                if (col.GetReprSelector() != _params.displayStyle) {
                    col.SetReprSelector(_params.displayStyle);
                    pass->SetRprimCollection(col);
                }
            }
        }

        *dirtyBits = HdChangeTracker::Clean;

        // Sync render passes.
        for (auto &pass : _renderPasses) {
            pass->Sync();
        }
    }

    void Prepare(HdTaskContext* ctx, HdRenderIndex* renderIndex) override
    {
        _params.renderPassState->Prepare(renderIndex->GetResourceRegistry());
    }

    void Execute(HdTaskContext* ctx) override
    {
        for (auto &pass : _renderPasses) {
            pass->Execute(_params.renderPassState, GetRenderTags());
        }
    }

    const TfTokenVector& GetRenderTags() const override
    {
        return _renderTags;
    }

private:
    HdSt_DrawTaskParams _params;
    TfTokenVector _renderTags;
    HdRenderPassSharedPtrVector _renderPasses;
};

// -----------------------------------------------------------------------------

// HdSt_UnitTestGLDrawing doesn't provide the abstraction for a multi-viewer
// application.
// HdSt_MyTestDriver attempts to do so, wherein
// - the test driver owns the Storm render delegate and Hydra render index, 
//   populates them with the scene prims and instantiates viewers.
// - each viewer manages its rendering tasks and updates the tasks when viewer
//   state such as display style or render tags changes.
//
class HdSt_MyTestDriver : public HdSt_TestDriverBase<HdUnitTestDelegate>
{
public:
    HdSt_MyTestDriver();

    void Draw(std::vector<size_t> viewerIds);

    const HdStRenderPassStateSharedPtr &GetRenderPassState() const {
        return _renderPassStates[0];
    }

    void SetViewerDisplayStyle(size_t viewerId, HdReprSelector const &rs) {
        _GetViewer(viewerId)->SetDisplayStyle(GetDelegate(), rs);
    }

    void SetViewerShowGuides(size_t viewerId, bool showGuides) {
        _GetViewer(viewerId)->SetShowGuides(GetDelegate(), showGuides);
    }

private:
    class _Viewer
    {
    public:
        _Viewer(std::string const &viewerName,
                HdUnitTestDelegate &sd,
                HdStRenderPassStateSharedPtr const &state)
        : _viewerName(viewerName)
        {
            _CreateRenderTasks(sd, state);
        }

        SdfPathVector GetRenderTaskIds() const {
            return { _drawTaskId };
        }

        void SetDisplayStyle(HdUnitTestDelegate &sd, HdReprSelector const &rs);
        void SetShowGuides(HdUnitTestDelegate &sd, bool showGuides);

    private:
        void _CreateRenderTasks(HdUnitTestDelegate &sd,
                                HdStRenderPassStateSharedPtr const &state);

        std::string _viewerName;
        // For now, each viewer has just a drawing task with multiple
        // render passes.
        SdfPath _drawTaskId;
    };

    _Viewer* _GetViewer(size_t index);

    // App viewer state
    std::vector<_Viewer> _viewers;
};

HdSt_MyTestDriver::HdSt_MyTestDriver()
{
    _renderPassStates = {
        std::dynamic_pointer_cast<HdStRenderPassState>(
            _GetRenderDelegate()->CreateRenderPassState()) };
    _renderPassStates[0]->SetDepthFunc(HdCmpFuncLess);
    _renderPassStates[0]->SetCullStyle(HdCullStyleNothing);

    // Init sets up the camera in the render pass state and
    // thus needs to be called after render pass state has been setup.
    _Init();

    // Viewer setup
    {
        const size_t numViewers = 2;
        size_t id = 0;
        while (id < numViewers) {
            _viewers.push_back(
                _Viewer("Viewer" + std::to_string(id),
                    GetDelegate(), GetRenderPassState()));
            id++;
        }
    }
}

void
HdSt_MyTestDriver::Draw(std::vector<size_t> viewerIds)
{
    for (size_t const &vidx : viewerIds) {
        if (vidx < _viewers.size()) {
            SdfPathVector taskIds = _viewers[vidx].GetRenderTaskIds();
            HdTaskSharedPtrVector tasks;
            for (auto const &id : taskIds) {
                tasks.push_back(GetDelegate().GetRenderIndex().GetTask(id));
            }
            std::cout << "Rendering viewer " << vidx << std::endl;
            _GetEngine()->Execute(&GetDelegate().GetRenderIndex(), &tasks);
            std::cout << "Done!" << std::endl;
        }
    }
}

HdSt_MyTestDriver::_Viewer*
HdSt_MyTestDriver::_GetViewer(size_t viewerIdx)
{
    if (viewerIdx < _viewers.size()) {
        return &_viewers[viewerIdx];
    } else {
        return nullptr;
    }
}

// -----------------------------------------------------------------------------

class My_TestGLDrawing : public HdSt_UnitTestGLDrawing {
public:
    My_TestGLDrawing()
    {
        SetCameraRotate(60.0f, 0.0f);
        SetCameraTranslate(GfVec3f(0, 0, -20.0f-1.7320508f*2.0f));
    }

    ~My_TestGLDrawing() = default;

    // Hd_UnitTestGLDrawing overrides
    virtual void InitTest();
    virtual void DrawTest();
    virtual void OffscreenTest();
    void Present(uint32_t framebuffer) override;

protected:
    virtual void ParseArgs(int argc, char *argv[]);

    void _Draw(std::vector<size_t> vieverIds);

private:
    std::unique_ptr<HdSt_MyTestDriver> _driver;
};

// -----------------------------------------------------------------------------

/* virtual */
void
My_TestGLDrawing::InitTest()
{
    _driver = std::make_unique<HdSt_MyTestDriver>();
    
    HdUnitTestDelegate &delegate = _driver->GetDelegate();

    delegate.SetRefineLevel(0);
    delegate.PopulateInvalidPrimsSet();
    const GfVec3f center = delegate.PopulateBasicTestSet();

    // center camera
    SetCameraTranslate(GetCameraTranslate() - center);

    _driver->SetClearColor(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
    _driver->SetClearDepth(1.0f);
    _driver->SetupAovs(GetWidth(), GetHeight());
}

/* virtual */
void
My_TestGLDrawing::DrawTest()
{
    // For interactive purposes, just use the first viewer.
    _Draw({0});
}

/* virtual */
void
My_TestGLDrawing::OffscreenTest()
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();
    
    auto _PrintPerfCounters = [&perfLog]() {
        const TfTokenVector tokens = {HdStPerfTokens->drawItemsCacheMiss,
                                      HdStPerfTokens->drawItemsCacheStale,
                                      HdStPerfTokens->drawItemsCacheHit};
        for (TfToken const &token : tokens) {
            std::cout << token << " = "
                      << perfLog.GetCounter(token) << std::endl;
        }
    };
    
    size_t const numRenderPasses = HdStMaterialTagTokens->allTokens.size();
    
    // 1. Draw just the first viewer. 
    // This submits the draw task with a render pass for each material tag.
    // Each render pass' cache lookup would be a MISS.
    {
        std::cout << "Case 1 : Drawing first viewer...." << std::endl;
        _Draw({0});
        _PrintPerfCounters();
        TF_VERIFY(perfLog.GetCounter(
            HdStPerfTokens->drawItemsCacheMiss) == numRenderPasses);
        TF_VERIFY(perfLog.GetCounter(
            HdStPerfTokens->drawItemsCacheStale) == 0);
        TF_VERIFY(perfLog.GetCounter(
            HdStPerfTokens->drawItemsCacheHit) == 0);
    }

    // 2. Draw just the second viewer. 
    // While this submits a different draw task, each of its render passes
    // benefits from the draw items cache with all lookups being a HIT.
    {
        perfLog.ResetCounters();
        std::cout << "Case 2 : Drawing second viewer...." << std::endl;
        _Draw({1});
        _PrintPerfCounters();
        TF_VERIFY(perfLog.GetCounter(
            HdStPerfTokens->drawItemsCacheMiss) == 0);
        TF_VERIFY(perfLog.GetCounter(
            HdStPerfTokens->drawItemsCacheStale) == 0);
        TF_VERIFY(perfLog.GetCounter(
            HdStPerfTokens->drawItemsCacheHit) == numRenderPasses);
    }

    // 3. Change display style of the second viewer AND
    //    Draw both viewers.
    // The render passes from the first viewer will have up-to-date draw items
    // (this isn't treated as a HIT).
    // The passes from the second viewer will need to refetch draw items and
    // since the repr wasn't seen earlier, this will be a MISS.
    {
        perfLog.ResetCounters();
        std::cout << "Case 3 : Change display style of second viewer...." << std::endl;
        _driver->SetViewerDisplayStyle(1, HdReprSelector(HdReprTokens->refined));
        _Draw({0,1});
        _PrintPerfCounters();
        TF_VERIFY(perfLog.GetCounter(
            HdStPerfTokens->drawItemsCacheMiss) == numRenderPasses);
        TF_VERIFY(perfLog.GetCounter(
            HdStPerfTokens->drawItemsCacheStale) == 0);
        TF_VERIFY(perfLog.GetCounter(
            HdStPerfTokens->drawItemsCacheHit) == 0);
    }

    // 4. Enable guides for the first viewer AND
    //    Draw both viewers.
    // The passes from the first viewer will need to fetch draw items and since
    // the 'guide' tag wasn't seen earlier, this will be a MISS.
    // The passes from the second viewer remain unchanged (this isn't treated as
    // a HIT).
    {
        perfLog.ResetCounters();
        std::cout << "Case 4 : Change render tags opinion of first viewer...." << std::endl;
        _driver->SetViewerShowGuides(0, true);
        _Draw({0,1});
        _PrintPerfCounters();
        TF_VERIFY(perfLog.GetCounter(
            HdStPerfTokens->drawItemsCacheMiss) == numRenderPasses);
        TF_VERIFY(perfLog.GetCounter(
            HdStPerfTokens->drawItemsCacheStale) == 0);
        TF_VERIFY(perfLog.GetCounter(
            HdStPerfTokens->drawItemsCacheHit) == 0);
    }
}

/* virtual */
void
My_TestGLDrawing::ParseArgs(int argc, char *argv[])
{
}

void
My_TestGLDrawing::_Draw(std::vector<size_t> viewerIds)
{
    // Simulate multi-viewer drawing even though we're drawing to the same
    // FBO. This test doesn't use AOVs per viewer.

    // Update shared framing state (used by all the viewers' tasks..)
    int width = GetWidth(), height = GetHeight();
    GfMatrix4d viewMatrix = GetViewMatrix();
    GfMatrix4d projMatrix = GetProjectionMatrix();
    
    _driver->SetCamera(
        viewMatrix, 
        projMatrix, 
        CameraUtilFraming(
            GfRect2i(GfVec2i(0, 0), width, height)));

    _driver->UpdateAovDimensions(width, height);

    _driver->Draw(viewerIds);
}

void
My_TestGLDrawing::Present(uint32_t framebuffer)
{
    _driver->Present(GetWidth(), GetHeight(), framebuffer);
}

//------------------------------------------------------------------------------

void
HdSt_MyTestDriver::_Viewer::SetDisplayStyle(
    HdUnitTestDelegate &sd,
    HdReprSelector const &rs)
{
    VtValue vParams = sd.Get(_drawTaskId, HdTokens->params);
    HdSt_DrawTaskParams params = vParams.UncheckedGet<HdSt_DrawTaskParams>();

    if (params.displayStyle != rs) {
        params.displayStyle = rs;
        sd.UpdateTask(_drawTaskId, HdTokens->params, VtValue(params));
    }
}

void
HdSt_MyTestDriver::_Viewer::SetShowGuides(
    HdUnitTestDelegate &sd,
    bool showGuides)
{
    TfTokenVector tags = sd.GetTaskRenderTags(_drawTaskId);
    auto const it =
        std::find(tags.begin(), tags.end(), HdRenderTagTokens->guide);
    const bool foundGuideTag = (it != tags.end());

    if (showGuides != foundGuideTag) {
        if (showGuides) {
            tags.push_back(HdRenderTagTokens->guide);
        } else {
            tags.erase(it);
        }
        sd.UpdateTask(_drawTaskId, HdTokens->renderTags, VtValue(tags));
    }
}

void
HdSt_MyTestDriver::_Viewer::_CreateRenderTasks(
    HdUnitTestDelegate &sd,
    HdStRenderPassStateSharedPtr const &state)
{
    std::string prefix(_viewerName + "/Tasks/");
    
    {
        _drawTaskId = SdfPath(prefix + "DrawTask");
        sd.AddTask<HdSt_MyDrawTask>(_drawTaskId);
        HdSt_DrawTaskParams params;
        params.displayStyle = HdReprSelector(HdReprTokens->hull);
        params.renderPassState = state;
        sd.UpdateTask(_drawTaskId, HdTokens->params, VtValue(params));
        sd.UpdateTask(_drawTaskId, HdTokens->renderTags,
                       VtValue(TfTokenVector({HdRenderTagTokens->geometry})));
    }
}

//------------------------------------------------------------------------------

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

