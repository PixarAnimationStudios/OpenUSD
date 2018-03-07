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
#ifndef USDIMAGING_UNIT_TEST_HELPER
#define USDIMAGING_UNIT_TEST_HELPER

/// \file usdImaging/unitTestHelper.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/delegate.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/rprim.h"

#include "pxr/imaging/hdSt/renderDelegate.h"

#include <string>
#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


typedef boost::shared_ptr<HdRenderPass> HdRenderPassSharedPtr;

/// \class UsdImaging_TestDriver
///
/// A unit test driver that exercises the core engine.
///
/// \note This test driver does NOT assume OpenGL is available; in the even
/// that is is not available, all OpenGL calls become no-ops, but all other work
/// is performed as usual.
///
class UsdImaging_TestDriver final {
public:
    UsdImaging_TestDriver(std::string const& usdFilePath);
    UsdImaging_TestDriver(std::string const& usdFilePath,
                          TfToken const &collectioName,
                          TfToken const &reprName,
                          TfTokenVector const &renderTags);
    UsdImaging_TestDriver(UsdStageRefPtr const& usdStage);
    UsdImaging_TestDriver(UsdStageRefPtr const& usdStage,
                          TfToken const &collectioName,
                          TfToken const &reprName,
                          TfTokenVector const &renderTags);

    ~UsdImaging_TestDriver();

    void Draw();
    void SetTime(double time);

    /// Marks an rprim in the RenderIndex as dirty with the given dirty flags.
    void MarkRprimDirty(SdfPath path, HdDirtyBits flag);

    /// Set camera to renderpass
    void SetCamera(GfMatrix4d const &modelViewMatrix,
                   GfMatrix4d const &projectionMatrix,
                   GfVec4d const &viewport);

    /// Set fallback refine level
    void SetRefineLevelFallback(int level);

    /// Returns the renderpass
    HdRenderPassSharedPtr const& GetRenderPass();

    /// Returns the underlying delegate for this driver.
    UsdImagingDelegate& GetDelegate();

    /// Returns the populated UsdStage for this driver.
    UsdStageRefPtr const& GetStage();

private:
    HdEngine _engine;
    HdStRenderDelegate   _renderDelegate;
    HdRenderIndex       *_renderIndex;
    UsdImagingDelegate  *_delegate;
    HdRenderPassSharedPtr _geometryPass;
    HdRenderPassStateSharedPtr _renderPassState;
    UsdStageRefPtr _stage;

    void _Init(UsdStageRefPtr const& usdStage,
               TfToken const &collectionName,
               TfToken const &reprName,
               TfTokenVector const &renderTags);
};

/// A simple drawing task that just executes a render pass.
class UsdImaging_DrawTask final : public HdTask
{
public:
    UsdImaging_DrawTask(HdRenderPassSharedPtr const &renderPass,
                        HdRenderPassStateSharedPtr const &renderPassState);

protected:
    virtual void _Sync(HdTaskContext* ctx) override;
    virtual void _Execute(HdTaskContext* ctx) override;

private:
    HdRenderPassSharedPtr _renderPass;
    HdRenderPassStateSharedPtr _renderPassState;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //USDIMAGING_UNIT_TEST_HELPER
