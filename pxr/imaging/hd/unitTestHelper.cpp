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
#include "pxr/imaging/hd/unitTestHelper.h"
#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/unitTestNullRenderPass.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/frustum.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/staticTokens.h"

#include <boost/functional/hash.hpp>

#include <string>
#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (l0dir)
    (l0color)
    (l1dir)
    (l1color)
    (sceneAmbient)
    (vec3)

    // Collection names
    (testCollection)
);

class Hd_DrawTask final : public HdTask
{
public:
    Hd_DrawTask(HdRenderPassSharedPtr const &renderPass,
                HdRenderPassStateSharedPtr const &renderPassState,
                bool withGuides)
    : HdTask(SdfPath::EmptyPath())
    , _renderPass(renderPass)
    , _renderPassState(renderPassState)
    , _renderTags()
    {
        _renderTags.reserve(2);
        _renderTags.push_back(HdRenderTagTokens->geometry);

        if (withGuides) {
            _renderTags.push_back(HdRenderTagTokens->guide);
        }
    }

    void Sync(HdSceneDelegate*, HdTaskContext*, HdDirtyBits*) override
    {
        _renderPass->Sync();
    }

    void Prepare(HdTaskContext* ctx, HdRenderIndex* renderIndex) override
    {
        _renderPassState->Prepare(renderIndex->GetResourceRegistry());
    }

    void Execute(HdTaskContext* ctx) override
    {
        _renderPassState->Bind();
        _renderPass->Execute(_renderPassState, GetRenderTags());
        _renderPassState->Unbind();
    }

    const TfTokenVector &GetRenderTags() const override
    {
        return _renderTags;
    }

private:
    HdRenderPassSharedPtr _renderPass;
    HdRenderPassStateSharedPtr _renderPassState;
    TfTokenVector _renderTags;

    Hd_DrawTask() = delete;
    Hd_DrawTask(const Hd_DrawTask &) = delete;
    Hd_DrawTask &operator =(const Hd_DrawTask &) = delete;
};

template <typename T>
static VtArray<T>
_BuildArray(T values[], int numValues)
{
    VtArray<T> result(numValues);
    std::copy(values, values+numValues, result.begin());
    return result;
}

Hd_TestDriver::Hd_TestDriver()
 : _engine()
 , _renderDelegate()
 , _renderIndex(nullptr)
 , _sceneDelegate(nullptr)
 , _renderPass()
 , _renderPassState(_renderDelegate.CreateRenderPassState())
 , _collection(_tokens->testCollection, HdReprSelector())
{
    HdReprSelector reprSelector = HdReprSelector(HdReprTokens->hull);
    if (TfGetenv("HD_ENABLE_SMOOTH_NORMALS", "CPU") == "CPU" ||
        TfGetenv("HD_ENABLE_SMOOTH_NORMALS", "CPU") == "GPU") {
        reprSelector = HdReprSelector(HdReprTokens->smoothHull);
    }
    _Init(reprSelector);
}

Hd_TestDriver::Hd_TestDriver(HdReprSelector const &reprSelector)
 : _engine()
 , _renderDelegate()
 , _renderIndex(nullptr)
 , _sceneDelegate(nullptr)
 , _renderPass()
 , _renderPassState(_renderDelegate.CreateRenderPassState())
 , _collection(_tokens->testCollection, HdReprSelector())
{
    _Init(reprSelector);
}

Hd_TestDriver::~Hd_TestDriver()
{
    delete _sceneDelegate;
    delete _renderIndex;
}

void
Hd_TestDriver::_Init(HdReprSelector const &reprSelector)
{
    _renderIndex = HdRenderIndex::New(&_renderDelegate);
    TF_VERIFY(_renderIndex != nullptr);

    _sceneDelegate = new HdUnitTestDelegate(_renderIndex,
                                             SdfPath::AbsoluteRootPath());

    _sceneDelegate->AddCamera(_cameraId);
    GfMatrix4d viewMatrix = GfMatrix4d().SetIdentity();
    viewMatrix *= GfMatrix4d().SetTranslate(GfVec3d(0.0, 1000.0, 0.0));
    viewMatrix *= GfMatrix4d().SetRotate(GfRotation(GfVec3d(1.0, 0.0, 0.0), -90.0));

    GfFrustum frustum;
    frustum.SetPerspective(45, true, 1, 1.0, 10000.0);
    GfMatrix4d projMatrix = frustum.ComputeProjectionMatrix();

    SetCamera(viewMatrix, projMatrix, GfVec4d(0, 0, 512, 512));

    // set depthfunc to default
    _renderPassState->SetDepthFunc(HdCmpFuncLess);

    // Update collection with repr and add collection to change tracker.
    _collection.SetReprSelector(reprSelector);
    HdChangeTracker &tracker = _renderIndex->GetChangeTracker();
    tracker.AddCollection(_collection.GetName());
}

void
Hd_TestDriver::Draw(bool withGuides)
{
    Draw(GetRenderPass(), withGuides);
}

void
Hd_TestDriver::Draw(HdRenderPassSharedPtr const &renderPass, bool withGuides)
{
    HdTaskSharedPtrVector tasks = {
        boost::make_shared<Hd_DrawTask>(renderPass, _renderPassState, withGuides)
    };
    _engine.Execute(&_sceneDelegate->GetRenderIndex(), &tasks);
}

void
Hd_TestDriver::SetCamera(GfMatrix4d const &modelViewMatrix,
                         GfMatrix4d const &projectionMatrix,
                         GfVec4d const &viewport)
{
    _sceneDelegate->UpdateCamera(
        _cameraId, HdCameraTokens->worldToViewMatrix, VtValue(modelViewMatrix));
    _sceneDelegate->UpdateCamera(
        _cameraId, HdCameraTokens->projectionMatrix, VtValue(projectionMatrix));
    // Baselines for tests were generated without constraining the view
    // frustum based on the viewport aspect ratio.
    _sceneDelegate->UpdateCamera(
        _cameraId, HdCameraTokens->windowPolicy,
        VtValue(CameraUtilDontConform));
    
    HdSprim const *cam = _renderIndex->GetSprim(HdPrimTypeTokens->camera,
                                                 _cameraId);
    TF_VERIFY(cam);
    _renderPassState->SetCameraAndViewport(
        dynamic_cast<HdCamera const *>(cam), viewport);
}

void
Hd_TestDriver::SetCullStyle(HdCullStyle cullStyle)
{
    _renderPassState->SetCullStyle(cullStyle);
}

HdRenderPassSharedPtr const &
Hd_TestDriver::GetRenderPass()
{
    if (!_renderPass) {
        _renderPass = HdRenderPassSharedPtr(
            new Hd_UnitTestNullRenderPass(&_sceneDelegate->GetRenderIndex(),
                                          _collection));
    }
    return _renderPass;
}

void
Hd_TestDriver::SetRepr(HdReprSelector const &reprSelector)
{
    _collection.SetReprSelector(reprSelector);

    // Mark changes.
    HdChangeTracker &tracker = _renderIndex->GetChangeTracker();
    tracker.MarkCollectionDirty(_collection.GetName());

    // Update render pass with updated collection
    _renderPass->SetRprimCollection(_collection);
}

PXR_NAMESPACE_CLOSE_SCOPE

