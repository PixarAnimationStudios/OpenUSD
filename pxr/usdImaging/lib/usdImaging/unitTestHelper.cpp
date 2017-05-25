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
#include "pxr/usdImaging/usdImaging/unitTestHelper.h"

#include "pxr/imaging/hdSt/renderPass.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/tokens.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

// UsdImaging_DrawTask 
UsdImaging_DrawTask::UsdImaging_DrawTask(
        HdRenderPassSharedPtr const &renderPass,
        HdRenderPassStateSharedPtr const &renderPassState)
    : HdTask()
    , _renderPass(renderPass)
    , _renderPassState(renderPassState)
{}

/* virtual */
void
UsdImaging_DrawTask::_Sync(HdTaskContext* ctx)
{
    _renderPass->Sync();
    _renderPassState->Sync();
}

/* virtual */
void
UsdImaging_DrawTask::_Execute(HdTaskContext* ctx)
{
    _renderPassState->Bind();
    _renderPass->Execute(_renderPassState);
    _renderPassState->Unbind();
}

// UsdImaging_TestDriver
void
UsdImaging_TestDriver::_Init(UsdStageRefPtr const& usdStage,
                             TfToken const &collectionName,
                             TfToken const &reprName,
                             TfTokenVector const &renderTags)
{
    _renderIndex = HdRenderIndex::New(&_renderDelegate);
    TF_VERIFY(_renderIndex != nullptr);
    _delegate = new UsdImagingDelegate(_renderIndex, SdfPath::AbsoluteRootPath());

    _stage = usdStage;
    _delegate->Populate(_stage->GetPseudoRoot());
    HdRprimCollection col = HdRprimCollection(collectionName, reprName);
    col.SetRenderTags(renderTags);
    _geometryPass = HdRenderPassSharedPtr(new HdSt_RenderPass(_renderIndex, col));
    _renderPassState = HdRenderPassStateSharedPtr(new HdRenderPassState());
}

UsdImaging_TestDriver::UsdImaging_TestDriver(std::string const& usdFilePath,
                                             TfToken const &collectionName,
                                             TfToken const &reprName,
                                             TfTokenVector const &renderTags)
 : _engine()
 , _renderDelegate()
 , _renderIndex(nullptr)
 , _delegate(nullptr)
 , _geometryPass()
 , _renderPassState()
 , _stage()
{
    _Init(UsdStage::Open(usdFilePath), collectionName, reprName, renderTags);
}

UsdImaging_TestDriver::UsdImaging_TestDriver(std::string const& usdFilePath)
 : _engine()
 , _renderDelegate()
 , _renderIndex(nullptr)
 , _delegate(nullptr)
 , _geometryPass()
 , _renderPassState()
 , _stage()
{
    TfTokenVector renderTags;
    renderTags.push_back(HdTokens->geometry);
    _Init(UsdStage::Open(usdFilePath), HdTokens->geometry, HdTokens->hull, renderTags);
}

UsdImaging_TestDriver::UsdImaging_TestDriver(UsdStageRefPtr const& usdStage,
                                             TfToken const &collectionName,
                                             TfToken const &reprName,
                                             TfTokenVector const &renderTags)
 : _engine()
 , _renderDelegate()
 , _renderIndex(nullptr)
 , _delegate(nullptr)
 , _geometryPass()
 , _renderPassState()
 , _stage()
{
    _Init(usdStage, collectionName, reprName, renderTags);
}

UsdImaging_TestDriver::UsdImaging_TestDriver(UsdStageRefPtr const& usdStage)
 : _engine()
 , _renderDelegate()
 , _renderIndex(nullptr)
 , _delegate(nullptr)
 , _geometryPass()
 , _renderPassState()
 , _stage()
{
    TfTokenVector renderTags;
    renderTags.push_back(HdTokens->geometry);
    _Init(usdStage, HdTokens->geometry, HdTokens->hull, renderTags);
}


UsdImaging_TestDriver::~UsdImaging_TestDriver()
{
    delete _delegate;
    delete _renderIndex;
}

void
UsdImaging_TestDriver::Draw()
{
    HdTaskSharedPtrVector tasks = {
        boost::make_shared<UsdImaging_DrawTask>(_geometryPass, _renderPassState)
    };
    _engine.Execute(_delegate->GetRenderIndex(), tasks);
}

void
UsdImaging_TestDriver::SetTime(double time)
{
    _delegate->SetTime(time);
}

void
UsdImaging_TestDriver::MarkRprimDirty(SdfPath path, HdDirtyBits flag)
{
    _delegate->GetRenderIndex().GetChangeTracker().MarkRprimDirty(path, flag);
}

void
UsdImaging_TestDriver::SetCamera(GfMatrix4d const &modelViewMatrix,
                                 GfMatrix4d const &projectionMatrix,
                                 GfVec4d const & viewport)
{
    _renderPassState->SetCamera(modelViewMatrix, projectionMatrix, viewport);
}

void
UsdImaging_TestDriver::SetRefineLevelFallback(int level)
{
    _delegate->SetRefineLevelFallback(level);
}


HdRenderPassSharedPtr const &
UsdImaging_TestDriver::GetRenderPass()
{
    return _geometryPass;
}

UsdImagingDelegate& 
UsdImaging_TestDriver::GetDelegate()
{
    return *_delegate;
}

/// Returns the populated UsdStage for this driver.
UsdStageRefPtr const&
UsdImaging_TestDriver::GetStage()
{
    return _stage;
}


PXR_NAMESPACE_CLOSE_SCOPE

