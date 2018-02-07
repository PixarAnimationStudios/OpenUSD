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
);

class Hd_DrawTask final : public HdTask
{
public:
    Hd_DrawTask(HdRenderPassSharedPtr const &renderPass,
                HdRenderPassStateSharedPtr const &renderPassState)
    : HdTask()
    , _renderPass(renderPass)
    , _renderPassState(renderPassState)
    {
    }

protected:
    virtual void _Sync(HdTaskContext* ctx) override
    {
        _renderPass->Sync();
        _renderPassState->Sync(
            _renderPass->GetRenderIndex()->GetResourceRegistry());
    }

    virtual void _Execute(HdTaskContext* ctx) override
    {
        _renderPassState->Bind();
        _renderPass->Execute(_renderPassState);
        _renderPassState->Unbind();
    }

private:
    HdRenderPassSharedPtr _renderPass;
    HdRenderPassStateSharedPtr _renderPassState;
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
 , _reprName()
 , _geomPass()
 , _geomAndGuidePass()
 , _renderPassState(_renderDelegate.CreateRenderPassState())
{
    TfToken reprName = HdTokens->hull;
    if (TfGetenv("HD_ENABLE_SMOOTH_NORMALS", "CPU") == "CPU" ||
        TfGetenv("HD_ENABLE_SMOOTH_NORMALS", "CPU") == "GPU") {
        reprName = HdTokens->smoothHull;
    }
    _Init(reprName);
}

Hd_TestDriver::Hd_TestDriver(TfToken const &reprName)
 : _engine()
 , _renderDelegate()
 , _renderIndex(nullptr)
 , _sceneDelegate(nullptr)
 , _reprName()
 , _geomPass()
 , _geomAndGuidePass()
 , _renderPassState(_renderDelegate.CreateRenderPassState())
{
    _Init(reprName);
}

Hd_TestDriver::~Hd_TestDriver()
{
    delete _sceneDelegate;
    delete _renderIndex;
}

void
Hd_TestDriver::_Init(TfToken const &reprName)
{
    _renderIndex = HdRenderIndex::New(&_renderDelegate);
    TF_VERIFY(_renderIndex != nullptr);

    _sceneDelegate = new HdUnitTestDelegate(_renderIndex,
                                             SdfPath::AbsoluteRootPath());

    _reprName = reprName;

    GfMatrix4d viewMatrix = GfMatrix4d().SetIdentity();
    viewMatrix *= GfMatrix4d().SetTranslate(GfVec3d(0.0, 1000.0, 0.0));
    viewMatrix *= GfMatrix4d().SetRotate(GfRotation(GfVec3d(1.0, 0.0, 0.0), -90.0));

    GfFrustum frustum;
    frustum.SetPerspective(45, true, 1, 1.0, 10000.0);
    GfMatrix4d projMatrix = frustum.ComputeProjectionMatrix();

    SetCamera(viewMatrix, projMatrix, GfVec4d(0, 0, 512, 512));

    // set depthfunc to default
    _renderPassState->SetDepthFunc(HdCmpFuncLess);
}

void
Hd_TestDriver::Draw(bool withGuides)
{
    Draw(GetRenderPass(withGuides));
}

void
Hd_TestDriver::Draw(HdRenderPassSharedPtr const &renderPass)
{
    HdTaskSharedPtrVector tasks = {
        boost::make_shared<Hd_DrawTask>(renderPass, _renderPassState)
    };
    _engine.Execute(_sceneDelegate->GetRenderIndex(), tasks);
}

void
Hd_TestDriver::SetCamera(GfMatrix4d const &modelViewMatrix,
                         GfMatrix4d const &projectionMatrix,
                         GfVec4d const &viewport)
{
    _renderPassState->SetCamera(modelViewMatrix,
                                projectionMatrix,
                                viewport);
}

void
Hd_TestDriver::SetCullStyle(HdCullStyle cullStyle)
{
    _renderPassState->SetCullStyle(cullStyle);
}

HdRenderPassSharedPtr const &
Hd_TestDriver::GetRenderPass(bool withGuides)
{
    if (withGuides) {
        if (!_geomAndGuidePass) {
            TfTokenVector renderTags;
            renderTags.push_back(HdTokens->geometry);
            renderTags.push_back(HdTokens->guide);
            
            HdRprimCollection col = HdRprimCollection(
                                     HdTokens->geometry,
                                     _reprName);
            col.SetRenderTags(renderTags);
            _geomAndGuidePass = HdRenderPassSharedPtr(
                new Hd_UnitTestNullRenderPass(&_sceneDelegate->GetRenderIndex(), col));
        }
        return _geomAndGuidePass;
    } else {
        if (!_geomPass) {
            TfTokenVector renderTags;
            renderTags.push_back(HdTokens->geometry);

            HdRprimCollection col = HdRprimCollection(
                                        HdTokens->geometry,
                                        _reprName);
            col.SetRenderTags(renderTags);
            _geomPass = HdRenderPassSharedPtr(
                new Hd_UnitTestNullRenderPass(&_sceneDelegate->GetRenderIndex(), col));
        }
        return _geomPass;
    }
}

void
Hd_TestDriver::SetRepr(TfToken const &reprName)
{
    _reprName = reprName;

    if (_geomAndGuidePass) {
        TfTokenVector renderTags;
        renderTags.push_back(HdTokens->geometry);
        renderTags.push_back(HdTokens->guide);
        
        HdRprimCollection col = HdRprimCollection(
                                 HdTokens->geometry,
                                 _reprName);
        col.SetRenderTags(renderTags);
        _geomAndGuidePass->SetRprimCollection(col);
    }
    if (_geomPass) {
        TfTokenVector renderTags;
        renderTags.push_back(HdTokens->geometry);
        
        HdRprimCollection col = HdRprimCollection(
                                 HdTokens->geometry,
                                 _reprName);
        col.SetRenderTags(renderTags);
        _geomPass->SetRprimCollection(col);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

