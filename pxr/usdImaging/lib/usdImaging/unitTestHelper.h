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
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/unitTestNullRenderDelegate.h"
#include "pxr/imaging/hd/unitTestNullRenderPass.h"

#include <string>
#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


typedef boost::shared_ptr<HdRenderPass> HdRenderPassSharedPtr;

/// A simple test task that just causes sync processing
class UsdImaging_TestTask final : public HdTask
{
public:
    UsdImaging_TestTask(HdRenderPassSharedPtr const &renderPass)
        : HdTask(SdfPath::EmptyPath())
        , _renderPass(renderPass)
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
    }

    virtual void Execute(HdTaskContext* ctx) override {
    }

private:
    HdRenderPassSharedPtr _renderPass;
};

/// \class UsdImaging_TestDriver
///
/// A unit test driver that exercises the core engine.
///
/// \note This test driver uses a Null render delegate, so
/// no images are produced.  It just tests interaction between Hydra and
/// UsdImaging during Hydra's Sync phase. of Hydra
///
class UsdImaging_TestDriver final {
public:
    UsdImaging_TestDriver(std::string const& usdFilePath)
        : _engine()
        , _renderDelegate()
        , _renderIndex(nullptr)
        , _delegate(nullptr)
        , _geometryPass()
        , _stage()
    {
        HdRprimCollection collection = HdRprimCollection(
                HdTokens->geometry,
                HdReprSelector(HdReprTokens->hull));

        TfTokenVector renderTags;
        renderTags.push_back(HdTokens->geometry);
        collection.SetRenderTags(renderTags);

        _Init(UsdStage::Open(usdFilePath),
              collection,
              SdfPath::AbsoluteRootPath());
    }

    UsdImaging_TestDriver(std::string const& usdFilePath,
                          TfToken const &collectionName,
                          TfToken const &reprName,
                          TfTokenVector const &renderTags)
        : _engine()
        , _renderDelegate()
        , _renderIndex(nullptr)
        , _delegate(nullptr)
        , _geometryPass()
        , _stage()
    {
        HdRprimCollection collection = HdRprimCollection(
                collectionName,
                HdReprSelector(reprName));

        collection.SetRenderTags(renderTags);

        _Init(UsdStage::Open(usdFilePath),
              collection,
              SdfPath::AbsoluteRootPath());
    }

    UsdImaging_TestDriver(UsdStageRefPtr const& usdStage)
        : _engine()
        , _renderDelegate()
        , _renderIndex(nullptr)
        , _delegate(nullptr)
        , _geometryPass()
        , _stage()
    {
        HdRprimCollection collection = HdRprimCollection(
                HdTokens->geometry,
                HdReprSelector(HdReprTokens->hull));

        TfTokenVector renderTags;
        renderTags.push_back(HdTokens->geometry);
        collection.SetRenderTags(renderTags);

        _Init(usdStage, collection, SdfPath::AbsoluteRootPath());
    }

    UsdImaging_TestDriver(UsdStageRefPtr const& usdStage,
                          TfToken const &collectionName,
                          TfToken const &reprName,
                          TfTokenVector const &renderTags)
        : _engine()
        , _renderDelegate()
        , _renderIndex(nullptr)
        , _delegate(nullptr)
        , _geometryPass()
        , _stage()
    {
        HdRprimCollection collection = HdRprimCollection(
                collectionName,
                HdReprSelector(reprName));

        collection.SetRenderTags(renderTags);

        _Init(usdStage, collection, SdfPath::AbsoluteRootPath());
    }

    UsdImaging_TestDriver(UsdStageRefPtr const& usdStage,
                          HdRprimCollection const &collection,
                          SdfPath const &delegateId)
        : _engine()
        , _renderDelegate()
        , _renderIndex(nullptr)
        , _delegate(nullptr)
        , _geometryPass()
        , _stage()
    {
        _Init(usdStage, collection, delegateId);
    }

    ~UsdImaging_TestDriver()
    {
        delete _delegate;
        delete _renderIndex;
    }

    void Draw() {
        HdTaskSharedPtrVector tasks = {
            boost::make_shared<UsdImaging_TestTask>(_geometryPass)
        };
        _engine.Execute(_delegate->GetRenderIndex(), tasks);
    }
    void SetTime(double time) {
        _delegate->SetTime(time);
    }

    /// Marks an rprim in the RenderIndex as dirty with the given dirty flags.
    void MarkRprimDirty(SdfPath path, HdDirtyBits flag) {
        _delegate->GetRenderIndex().GetChangeTracker()
            .MarkRprimDirty(path, flag);
    }

    /// Returns the underlying delegate for this driver.
    UsdImagingDelegate& GetDelegate() {
        return *_delegate;
    }

    /// Returns the populated UsdStage for this driver.
    UsdStageRefPtr const& GetStage() {
        return _stage;
    }

private:
    HdEngine _engine;
    Hd_UnitTestNullRenderDelegate _renderDelegate;
    HdRenderIndex       *_renderIndex;
    UsdImagingDelegate  *_delegate;
    HdRenderPassSharedPtr _geometryPass;
    UsdStageRefPtr _stage;

    void _Init(UsdStageRefPtr const& usdStage,
               HdRprimCollection const &collection,
               SdfPath const &delegateId) {
        _renderIndex = HdRenderIndex::New(&_renderDelegate);
        TF_VERIFY(_renderIndex != nullptr);
        _delegate = new UsdImagingDelegate(_renderIndex, delegateId);

        _stage = usdStage;
        _delegate->Populate(_stage->GetPseudoRoot());

        _geometryPass = HdRenderPassSharedPtr(
                       new Hd_UnitTestNullRenderPass(_renderIndex, collection));
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //USDIMAGING_UNIT_TEST_HELPER
