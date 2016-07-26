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

#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/tokens.h"

#include <string>

void
UsdImaging_TestDriver::_Init(UsdStageRefPtr const& usdStage, 
                             TfToken const &reprName)
{
    _stage = usdStage;
    _delegate.Populate(_stage->GetPseudoRoot());
    //visible = _delegate.GetCollection("geometry");
    TfToken colName = HdTokens->geometry;
    _geometryPass = HdRenderPassSharedPtr(
        new HdRenderPass(&_delegate.GetRenderIndex(),
                         HdRprimCollection(colName, reprName)));
    _renderPassState = HdRenderPassStateSharedPtr(new HdRenderPassState());
}

UsdImaging_TestDriver::UsdImaging_TestDriver(std::string const& usdFilePath,
                                             TfToken const &reprName)
{
    _Init(UsdStage::Open(usdFilePath), reprName);
}

UsdImaging_TestDriver::UsdImaging_TestDriver(std::string const& usdFilePath)
{
    _Init(UsdStage::Open(usdFilePath), HdTokens->hull);
}

UsdImaging_TestDriver::UsdImaging_TestDriver(UsdStageRefPtr const& usdStage, 
                                             TfToken const &reprName)
{
    _Init(usdStage, reprName);
}

UsdImaging_TestDriver::UsdImaging_TestDriver(UsdStageRefPtr const& usdStage)
{
    _Init(usdStage, HdTokens->hull);
}

void
UsdImaging_TestDriver::Draw()
{
#if defined(HD_API) && HD_API > 21
    _engine.Draw(_delegate.GetRenderIndex(), _geometryPass, _renderPassState);
#else
    _engine.Draw(_delegate.GetRenderIndex(), renderPass);
#endif
}

void
UsdImaging_TestDriver::SetTime(double time)
{
    _delegate.SetTime(time);
}

void
UsdImaging_TestDriver::MarkRprimDirty(SdfPath path, HdChangeTracker::DirtyBits flag)
{
    _delegate.GetRenderIndex().GetChangeTracker().MarkRprimDirty(path, flag);
}

void
UsdImaging_TestDriver::SetCamera(GfMatrix4d const &modelViewMatrix,
                                 GfMatrix4d const &projectionMatrix,
                                 GfVec4d const & viewport)
{
#if defined(HD_API) && HD_API > 21
    _renderPassState->SetCamera(modelViewMatrix, projectionMatrix, viewport);
#else
    _geometryPass->GetRasterState()->SetCamera(
        modelViewMatrix, projectionMatrix, viewport);

#endif
}

void
UsdImaging_TestDriver::SetRefineLevelFallback(int level)
{
    _delegate.SetRefineLevelFallback(level);
}


HdRenderPassSharedPtr const &
UsdImaging_TestDriver::GetRenderPass()
{
    return _geometryPass;
}

UsdImagingDelegate& 
UsdImaging_TestDriver::GetDelegate()
{
    return _delegate;
}

/// Returns the populated UsdStage for this driver.
UsdStageRefPtr const&
UsdImaging_TestDriver::GetStage()
{
    return _stage;
}

