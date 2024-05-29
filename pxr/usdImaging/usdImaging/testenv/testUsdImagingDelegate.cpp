//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImaging/unitTestHelper.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/unitTestNullRenderDelegate.h"
#include "pxr/imaging/hd/unitTestNullRenderPass.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/timeSampleArray.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/mesh.h"

#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/gf/frustum.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

void
VaryingTest()
{
    const std::string usdPath = "varying.usda";

    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();

    // Reset all counters we care about.
    perfLog.ResetCache(HdTokens->extent);
    perfLog.ResetCache(HdTokens->points);
    perfLog.ResetCache(HdTokens->topology);
    perfLog.ResetCache(HdTokens->transform);
    perfLog.SetCounter(UsdImagingTokens->usdVaryingExtent, 0);
    perfLog.SetCounter(UsdImagingTokens->usdVaryingPrimvar, 0);
    perfLog.SetCounter(UsdImagingTokens->usdVaryingTopology, 0);
    perfLog.SetCounter(UsdImagingTokens->usdVaryingVisibility, 0);
    perfLog.SetCounter(UsdImagingTokens->usdVaryingXform, 0);
    
    // Variability is reported here, so perfLog must be enabled above.
    UsdImaging_TestDriver driver(usdPath);
    TF_VERIFY(perfLog.GetCounter(UsdImagingTokens->usdVaryingExtent) == 1);
    TF_VERIFY(perfLog.GetCounter(UsdImagingTokens->usdVaryingPrimvar) == 1);
    TF_VERIFY(perfLog.GetCounter(UsdImagingTokens->usdVaryingXform) == 1);
    TF_VERIFY(perfLog.GetCounter(UsdImagingTokens->usdVaryingVisibility) == 1);
    TF_VERIFY(perfLog.GetCounter(UsdImagingTokens->usdVaryingTopology) == 0);

    driver.SetTime(1);
    driver.Draw();

    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->extent) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->points) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->topology) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->transform) == 1);

    driver.SetTime(2);
    driver.Draw();

    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->extent) == 2,
                "Found %lu cache misses", 
                perfLog.GetCacheMisses(HdTokens->extent));
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->points) == 2, 
                "Found %lu cache misses", 
                perfLog.GetCacheMisses(HdTokens->points));
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->topology) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->transform) == 2);

    // Reset and use the vectorized SetTimes API
    perfLog.ResetCache(HdTokens->extent);
    perfLog.ResetCache(HdTokens->points);
    perfLog.ResetCache(HdTokens->topology);
    perfLog.ResetCache(HdTokens->transform);
    
    UsdImaging_TestDriver driver2(usdPath);
    driver2.SetTime(1.0);
    driver2.Draw();

    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->extent) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->points) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->topology) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->transform) == 1);

    UsdImagingDelegate::SetTimes(
        std::vector<UsdImagingDelegate*>(1, &driver2.GetDelegate()),
        std::vector<UsdTimeCode>(1, UsdTimeCode(2)));
    driver2.Draw();

    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->extent) == 2,
                "Found %lu cache misses", 
                perfLog.GetCacheMisses(HdTokens->extent));
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->points) == 2, 
                "Found %lu cache misses", 
                perfLog.GetCacheMisses(HdTokens->points));
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->topology) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->transform) == 2);
}

void
UnvaryingTest()
{
    const std::string usdPath = "unvarying.usda";

    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    
    // Reset all counters we care about.
    perfLog.ResetCache(HdTokens->extent);
    perfLog.ResetCache(HdTokens->points);
    perfLog.ResetCache(HdTokens->topology);
    perfLog.ResetCache(HdTokens->transform);
    perfLog.SetCounter(UsdImagingTokens->usdVaryingExtent, 0);
    perfLog.SetCounter(UsdImagingTokens->usdVaryingPrimvar, 0);
    perfLog.SetCounter(UsdImagingTokens->usdVaryingTopology, 0);
    perfLog.SetCounter(UsdImagingTokens->usdVaryingVisibility, 0);
    perfLog.SetCounter(UsdImagingTokens->usdVaryingXform, 0);

    // Variability is reported here, so perfLog must be enabled above.
    UsdImaging_TestDriver driver(usdPath);

    TF_VERIFY(perfLog.GetCounter(UsdImagingTokens->usdVaryingExtent) == 0);
    TF_VERIFY(perfLog.GetCounter(UsdImagingTokens->usdVaryingPrimvar) == 0);
    TF_VERIFY(perfLog.GetCounter(UsdImagingTokens->usdVaryingTopology) == 0);
    TF_VERIFY(perfLog.GetCounter(UsdImagingTokens->usdVaryingVisibility) == 0);
    TF_VERIFY(perfLog.GetCounter(UsdImagingTokens->usdVaryingXform) == 0);

    driver.Draw();

    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->extent) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->points) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->topology) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->transform) == 1);

    driver.SetTime(2);
    driver.Draw();

    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->extent) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->points) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->topology) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->transform) == 1);

    // Reset and use the vectorized SetTimes API
    perfLog.ResetCache(HdTokens->extent);
    perfLog.ResetCache(HdTokens->points);
    perfLog.ResetCache(HdTokens->topology);
    perfLog.ResetCache(HdTokens->transform);
    
    UsdImaging_TestDriver driver2(usdPath);

    driver2.Draw();

    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->extent) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->points) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->topology) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->transform) == 1);

    UsdImagingDelegate::SetTimes(
        std::vector<UsdImagingDelegate*>(1, &driver2.GetDelegate()),
        std::vector<UsdTimeCode>(1, UsdTimeCode(2)));
    driver2.Draw();

    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->extent) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->points) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->topology) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->transform) == 1);
}

void
VectorizedSetTimesTest()
{
    const std::string unvaryingUsdPath = "unvarying.usda";
    const std::string varyingUsdPath = "varying.usda";

    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();

    // Reset all counters we care about.
    perfLog.ResetCache(HdTokens->extent);
    perfLog.ResetCache(HdTokens->points);
    perfLog.ResetCache(HdTokens->topology);
    perfLog.ResetCache(HdTokens->transform);

    UsdImaging_TestDriver varyingDriver(varyingUsdPath);

    UsdImaging_TestDriver unvaryingDriver(unvaryingUsdPath);

    varyingDriver.Draw();
    unvaryingDriver.Draw();

    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->extent) == 2);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->points) == 2);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->topology) == 2);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->transform) == 2);

    std::vector<UsdImagingDelegate*> delegates;
    delegates.push_back(&varyingDriver.GetDelegate());
    delegates.push_back(&unvaryingDriver.GetDelegate());

    std::vector<UsdTimeCode> times;
    times.push_back(UsdTimeCode(2));
    times.push_back(UsdTimeCode(2));

    UsdImagingDelegate::SetTimes(delegates, times);
    varyingDriver.Draw();
    unvaryingDriver.Draw();
    
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->extent) == 3);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->points) == 3);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->topology) == 2);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->transform) == 3);
}

void
RefineLevelTest()
{
    const std::string usdPath = "unvarying.usda";
    UsdImaging_TestDriver driver(usdPath);
    UsdImagingDelegate& delegate = driver.GetDelegate();
    UsdStageRefPtr const& stage = driver.GetStage();
    HdChangeTracker& tracker = delegate.GetRenderIndex().GetChangeTracker();

    TF_VERIFY(delegate.GetRefineLevelFallback() == 0);
    for (UsdPrim prim: stage->Traverse()) {
        if (prim.IsA<UsdGeomMesh>()) {
            TF_VERIFY(delegate.GetDisplayStyle(prim.GetPath()).refineLevel == 0);
            TF_VERIFY(tracker.IsDisplayStyleDirty(prim.GetPath()));
            tracker.MarkRprimClean(prim.GetPath());
            TF_VERIFY(!tracker.IsDisplayStyleDirty(prim.GetPath()));
        }
    }

    delegate.SetRefineLevelFallback(0);
    for (UsdPrim prim: stage->Traverse()) {
        if (prim.IsA<UsdGeomMesh>()) {
            TF_VERIFY(delegate.GetDisplayStyle(prim.GetPath()).refineLevel == 0);
            // Should not be dirty because the level didn't actually change
            TF_VERIFY(!tracker.IsDisplayStyleDirty(prim.GetPath()));

            // Set the value to the existing value
            delegate.SetRefineLevel(prim.GetPath(), 0);
            // Should not be dirty because the level didn't actually change
            TF_VERIFY(!tracker.IsDisplayStyleDirty(prim.GetPath()));
        }
    }

    // All prims have an explicit refine level, so setting the fallback should
    // not affect them.
    delegate.SetRefineLevelFallback(8);
    for (UsdPrim prim: stage->Traverse()) {
        if (prim.IsA<UsdGeomMesh>()) {
            // Verify value and dirty
            TF_VERIFY(delegate.GetDisplayStyle(prim.GetPath()).refineLevel == 0);
            TF_VERIFY(!tracker.IsDisplayStyleDirty(prim.GetPath()));
            // Clear, clean & verify
            delegate.ClearRefineLevel(prim.GetPath());
            TF_VERIFY(delegate.GetDisplayStyle(prim.GetPath()).refineLevel == 8);
            TF_VERIFY(tracker.IsDisplayStyleDirty(prim.GetPath()));
            tracker.MarkRprimClean(prim.GetPath());
            TF_VERIFY(!tracker.IsDisplayStyleDirty(prim.GetPath()));
        }
    }

    // All explicit values are removed, verify fallback changes.
    delegate.SetRefineLevelFallback(1);
    for (UsdPrim prim: stage->Traverse()) {
        if (prim.IsA<UsdGeomMesh>()) {
            // Verify value and dirty
            TF_VERIFY(delegate.GetDisplayStyle(prim.GetPath()).refineLevel == 1);
            TF_VERIFY(tracker.IsDisplayStyleDirty(prim.GetPath()));

            // Clean & verify clean
            tracker.MarkRprimClean(prim.GetPath());
            TF_VERIFY(!tracker.IsDisplayStyleDirty(prim.GetPath()));

            // Set to existing & verify clean 
            delegate.SetRefineLevel(prim.GetPath(), 1);
            TF_VERIFY(!tracker.IsDisplayStyleDirty(prim.GetPath()));

            // Set to new value & verify dirty
            delegate.SetRefineLevel(prim.GetPath(), 2);
            TF_VERIFY(delegate.GetDisplayStyle(prim.GetPath()).refineLevel == 2);
            TF_VERIFY(tracker.IsDisplayStyleDirty(prim.GetPath()));

            // Clean & verify
            tracker.MarkRprimClean(prim.GetPath());
            TF_VERIFY(!tracker.IsDisplayStyleDirty(prim.GetPath()));
            
            // Set to existing explicit value & verify clean
            delegate.SetRefineLevel(prim.GetPath(), 2);
            TF_VERIFY(!tracker.IsDisplayStyleDirty(prim.GetPath()));

            // Set the fallback, but because we expressed an opinion for this
            // specific prim above, we don't expect the value to change.
            delegate.SetRefineLevelFallback(3);
            TF_VERIFY(delegate.GetDisplayStyle(prim.GetPath()).refineLevel == 2);
            // This prim should also not be dirty.
            TF_VERIFY(!tracker.IsDisplayStyleDirty(prim.GetPath()));

            // Clear the explicit refine level, expect dirty and fallback.
            delegate.ClearRefineLevel(prim.GetPath());
            TF_VERIFY(delegate.GetDisplayStyle(prim.GetPath()).refineLevel == 3);
            TF_VERIFY(tracker.IsDisplayStyleDirty(prim.GetPath()));

            // Clean, no-op clear, expect clean and fallback.
            tracker.MarkRprimClean(prim.GetPath());
            delegate.ClearRefineLevel(prim.GetPath());
            TF_VERIFY(delegate.GetDisplayStyle(prim.GetPath()).refineLevel == 3);
            TF_VERIFY(!tracker.IsDisplayStyleDirty(prim.GetPath()));
        }
    }
}

void
PrimvarNamesTest1()
{
    SdfPath meshPath("/pCube1");
    const std::string usdPath = "unvarying.usda";
    UsdStageRefPtr stage = UsdStage::Open(usdPath);

    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> renderIndex(
        HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    TF_VERIFY(renderIndex);
    std::unique_ptr<UsdImagingDelegate> delegate(
                          new UsdImagingDelegate(renderIndex.get(),
                                                  SdfPath::AbsoluteRootPath()));

    // Only populate is called here, which we want to ensure is enough to
    // populate primvar names.
    delegate->Populate(stage->GetPseudoRoot());
    delegate->SetTime(1.0);
    delegate->SyncAll(/*includeUnvarying*/true);

    // Verify expected names.
    HdPrimvarDescriptorVector vertexPrimvars =
        delegate->GetPrimvarDescriptors(meshPath, HdInterpolationVertex);

    TF_VERIFY(vertexPrimvars.size() == 3);
    TF_VERIFY(vertexPrimvars[0].name == TfToken("points"));
    TF_VERIFY(vertexPrimvars[1].name == TfToken("velocities"));
    TF_VERIFY(vertexPrimvars[2].name == TfToken("accelerations"));

    VtValue points = delegate->Get(meshPath, TfToken("points"));
    VtValue velocities = delegate->Get(meshPath, TfToken("velocities"));
    VtValue accelerations = delegate->Get(meshPath, TfToken("accelerations"));

    // Verify expected values
    VtArray<GfVec3f> velocitiesComparison(8, GfVec3f(1, 1, 1));
    VtArray<GfVec3f> accelerationsComparison(8, GfVec3f(1, 0, 0));
    TF_VERIFY(velocitiesComparison == velocities);
    TF_VERIFY(accelerationsComparison == accelerations);

    HdPrimvarDescriptorVector constantPrimvars =
        delegate->GetPrimvarDescriptors(meshPath, HdInterpolationConstant);
    TF_VERIFY(constantPrimvars.size() == 0);
}

void
PrimvarNamesTest2()
{
    SdfPath meshPath("/pCube1");
    const std::string usdPath = "unvarying.usda";
    UsdStageRefPtr stage = UsdStage::Open(usdPath);

    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> renderIndex(
        HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    TF_VERIFY(renderIndex);
    std::unique_ptr<UsdImagingDelegate> delegate(
                          new UsdImagingDelegate(renderIndex.get(),
                                                  SdfPath::AbsoluteRootPath()));
    
    // Setting the time after calling Populate here triggers two updates to the
    // primvar names, the test here is to ensure we accumulate primvars.
    delegate->Populate(stage->GetPseudoRoot());
    delegate->SetTime(1.0);
    delegate->SyncAll(true);

    // Verify expected names.
    HdPrimvarDescriptorVector vertexPrimvars =
        delegate->GetPrimvarDescriptors(meshPath, HdInterpolationVertex);
    TF_VERIFY(vertexPrimvars.size() == 3);
    TF_VERIFY(vertexPrimvars[0].name == TfToken("points"));
    TF_VERIFY(vertexPrimvars[1].name == TfToken("velocities"));
    TF_VERIFY(vertexPrimvars[2].name == TfToken("accelerations"));

    HdPrimvarDescriptorVector constantPrimvars =
        delegate->GetPrimvarDescriptors(meshPath, HdInterpolationConstant);
    TF_VERIFY(constantPrimvars.size() == 0);
}

void
PrimvarIndicesTest()
{
    SdfPath meshPath("/pCube1");
    const std::string usdPath = "indexedPrimvars.usda";
    UsdStageRefPtr stage = UsdStage::Open(usdPath);

    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> renderIndex(
        HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    TF_VERIFY(renderIndex);
    std::unique_ptr<UsdImagingDelegate> delegate(
                          new UsdImagingDelegate(renderIndex.get(),
                                                  SdfPath::AbsoluteRootPath()));
    delegate->Populate(stage->GetPseudoRoot());
    delegate->SetTime(1.0);
    delegate->SyncAll(/*includeUnvarying*/true);

    // Verify expected names.
    HdPrimvarDescriptorVector vertexPrimvars =
        delegate->GetPrimvarDescriptors(meshPath, HdInterpolationVertex);

    TF_VERIFY(vertexPrimvars.size() == 5);
    TF_VERIFY(vertexPrimvars[0].name == TfToken("points"));
    TF_VERIFY(vertexPrimvars[1].name == TfToken("velocities"));
    TF_VERIFY(vertexPrimvars[2].name == TfToken("displayColor"));
    TF_VERIFY(vertexPrimvars[3].name == TfToken("displayOpacity"));
    TF_VERIFY(vertexPrimvars[4].name == TfToken("customPv"));

    // Verify primvar indexed status
    TF_VERIFY(!vertexPrimvars[0].indexed);
    TF_VERIFY(!vertexPrimvars[1].indexed);
    TF_VERIFY(vertexPrimvars[2].indexed);
    TF_VERIFY(!vertexPrimvars[3].indexed);
    TF_VERIFY(vertexPrimvars[4].indexed);

    // Verify expected values and indices
    VtIntArray indices(0);
    // Normally you should not call "GetIndexedPrimvar" if the primvar 
    // descriptor indicates the primvar is not indexed
    VtValue displayColor = delegate->GetIndexedPrimvar(
        meshPath, TfToken("displayColor"), &indices);
    VtArray<GfVec3f> displayColorComparison = { GfVec3f(1, 0, 0), GfVec3f(0, 1, 0), GfVec3f(0, 0, 1) };
    VtArray<int> displayColorIndicesComparison = { 0, 1, 2, 0, 1, 2, 0, 0 };
    TF_VERIFY(displayColorComparison == displayColor);
    TF_VERIFY(displayColorIndicesComparison == indices);

    VtValue velocities = delegate->GetIndexedPrimvar(
        meshPath, TfToken("velocities"), &indices);
    VtArray<GfVec3f> velocitiesComparison(8, GfVec3f(1, 1, 1));
    VtArray<int> velocityIndicesComparison(0);
    TF_VERIFY(velocitiesComparison == velocities);
    TF_VERIFY(velocityIndicesComparison == indices);

    VtValue customPv = delegate->GetIndexedPrimvar(
        meshPath, TfToken("customPv"), &indices);
    VtArray<float> customPvComparison = { 0.25, 0.75, 0.5 };
    VtArray<int> customPvIndicesComparison = { 0, 1, 1, 1, 1, 2, 2, 0 };
    TF_VERIFY(customPvComparison == customPv);
    TF_VERIFY(customPvIndicesComparison == indices);

    VtValue displayOpacity = delegate->GetIndexedPrimvar(
        meshPath, TfToken("displayOpacity"), &indices);
    VtArray<float> displayOpacityComparison(8, 0.5);
    VtArray<int> displayOpacityIndicesComparison(0);
    TF_VERIFY(displayOpacityComparison == displayOpacity);
    TF_VERIFY(displayOpacityIndicesComparison == indices);
}

void
SamplePrimvarTest()
{
    const SdfPath meshPath("/pCube2");
    const SdfPath cameraPath("/camera");
    const std::string usdPath = "indexedPrimvars.usda";
    UsdStageRefPtr stage = UsdStage::Open(usdPath);
    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> renderIndex(
        HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    TF_VERIFY(renderIndex);
    std::unique_ptr<UsdImagingDelegate> delegate(
                          new UsdImagingDelegate(renderIndex.get(),
                                                  SdfPath::AbsoluteRootPath()));
    delegate->Populate(stage->GetPseudoRoot());

    delegate->SetTime(1.0);
    delegate->SyncAll(/*includeUnvarying*/true);

    // Verify expected names.
    HdPrimvarDescriptorVector vertexPrimvars =
        delegate->GetPrimvarDescriptors(meshPath, HdInterpolationVertex);

    TF_VERIFY(vertexPrimvars.size() == 4);
    TF_VERIFY(vertexPrimvars[0].name == TfToken("points"));
    TF_VERIFY(vertexPrimvars[1].name == TfToken("customPv1"));
    TF_VERIFY(vertexPrimvars[2].name == TfToken("customPv2"));
    TF_VERIFY(vertexPrimvars[3].name == TfToken("customPv3"));

    // Verify primvar indexed status
    TF_VERIFY(!vertexPrimvars[0].indexed);
    TF_VERIFY(!vertexPrimvars[1].indexed);
    TF_VERIFY(vertexPrimvars[2].indexed);
    TF_VERIFY(vertexPrimvars[3].indexed);

    // SamplePrimvar
    HdIndexedTimeSampleArray<VtValue, 10> pvSamples;

    delegate->HdSceneDelegate::SampleIndexedPrimvar(
        meshPath, TfToken("customPv1"), &pvSamples);
    VtFloatArray expectedValues = { 0, 1, 2, 3, 4, 5, 6, 7 };
    VtIntArray expectedIndices(0);
    TF_VERIFY(pvSamples.count == 1);
    TF_VERIFY(pvSamples.times[0] == 0);
    TF_VERIFY(pvSamples.values[0] == expectedValues);
    TF_VERIFY(pvSamples.indices[0]  == expectedIndices);

    delegate->HdSceneDelegate::SampleIndexedPrimvar(
        meshPath, TfToken("customPv2"), &pvSamples);
    expectedValues = { 0.5, 1, 1.5 };
    expectedIndices = { 0, 1, 1, 1, 1, 1, 1, 2 };
    TF_VERIFY(pvSamples.count == 1);
    TF_VERIFY(pvSamples.times[0] == 0);
    TF_VERIFY(pvSamples.values[0] == expectedValues);
    TF_VERIFY(pvSamples.indices[0] == expectedIndices);

    delegate->HdSceneDelegate::SampleIndexedPrimvar(
        meshPath, TfToken("customPv3"), &pvSamples);
    expectedValues = { 2.5, 3, 3.5 };
    expectedIndices = { 0, 1, 2, 0, 1, 2, 0, 1 };
    TF_VERIFY(pvSamples.count == 1);
    TF_VERIFY(pvSamples.times[0] == 0);
    TF_VERIFY(pvSamples.values[0] == expectedValues);
    TF_VERIFY(pvSamples.indices[0] == expectedIndices);

    delegate->SetTime(3.0);
    delegate->SyncAll(/*includeUnvarying*/true);

    delegate->HdSceneDelegate::SampleIndexedPrimvar(
        meshPath, TfToken("customPv1"), &pvSamples);
    expectedValues = { 0, 0, 0, 0, 0, 0, 0, 0 };
    expectedIndices = {}; 
    TF_VERIFY(pvSamples.count == 1);
    TF_VERIFY(pvSamples.times[0] == 0);
    TF_VERIFY(pvSamples.values[0] == expectedValues);
    TF_VERIFY(pvSamples.indices[0] == expectedIndices);

    delegate->HdSceneDelegate::SampleIndexedPrimvar(
        meshPath, TfToken("customPv2"), &pvSamples);
    expectedValues = { 1, 1.5, 2 };
    expectedIndices = { 0, 1, 1, 1, 1, 1, 1, 2 };
    TF_VERIFY(pvSamples.count == 1);
    TF_VERIFY(pvSamples.times[0] == 0);
    TF_VERIFY(pvSamples.values[0] == expectedValues);
    TF_VERIFY(pvSamples.indices[0] == expectedIndices);

    delegate->HdSceneDelegate::SampleIndexedPrimvar(
        meshPath, TfToken("customPv3"), &pvSamples);
    expectedValues = { 0, 0, 0.5 };
    expectedIndices = { 0, 1, 2, 0, 1, 2, 0, 1 };
    TF_VERIFY(pvSamples.count == 1);
    TF_VERIFY(pvSamples.times[0] == 0);
    TF_VERIFY(pvSamples.values[0] == expectedValues);
    TF_VERIFY(pvSamples.indices[0] == expectedIndices);
}

class TestTask final : public HdTask
{
public:
    TestTask(Hd_UnitTestNullRenderPass &renderPass)
    : HdTask(SdfPath::EmptyPath())
    , _renderPass(renderPass)
    {
    }

    virtual void Sync(HdSceneDelegate* delegate,
                      HdTaskContext* ctx,
                      HdDirtyBits* dirtyBits) override
    {
        _renderPass.Sync();
        *dirtyBits = HdChangeTracker::Clean;
    }

    virtual void Prepare(HdTaskContext* ctx,
                         HdRenderIndex* renderIndex) override
    {
    }

    virtual void Execute(HdTaskContext* ctx) override
    {
    }

private:
    Hd_UnitTestNullRenderPass &_renderPass;
};

void
RemoveTest()
{
    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> renderIndex(
        HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    TF_VERIFY(renderIndex);
    Hd_UnitTestNullRenderPass renderPass(renderIndex.get(),
                               HdRprimCollection(HdTokens->geometry,
                                 HdReprSelector(HdReprTokens->smoothHull)));
    HdTaskSharedPtrVector tasks;
    HdTaskContext         taskContext;
    tasks.push_back(std::make_shared<TestTask>(renderPass));

    const std::string usdPath = "test.usda";

    const SdfPath rPrimPath("/delegateId/Geom/Subdiv");
    const SdfPath sPrimPath("/delegateId/Materials/MyMaterial");

    {
        std::unique_ptr<UsdImagingDelegate> delegate(
             new UsdImagingDelegate(renderIndex.get(), SdfPath("/delegateId")));

        UsdStageRefPtr stage = UsdStage::Open(usdPath);

        delegate->Populate(stage->GetPseudoRoot());
        delegate->SetTime(1.0);
        delegate->SyncAll(true);

        renderIndex->SyncAll(&tasks, &taskContext);

        TF_VERIFY(renderIndex->GetRprim(rPrimPath), 
            "Could not get geometry rprim at path <%s>", rPrimPath.GetText());
        TF_VERIFY(renderIndex->GetSprim(HdPrimTypeTokens->material, sPrimPath),
            "Could not get shader sprim at path <%s>", sPrimPath.GetText());

        // destroy delegate.
        delegate.reset();
    }

    // should not exist
    TF_VERIFY(!renderIndex->GetRprim(rPrimPath));
    TF_VERIFY(!renderIndex->GetSprim(HdPrimTypeTokens->material, sPrimPath));

    // should successfully sync after deletion
    renderIndex->SyncAll(&tasks, &taskContext);
}

// Exercise the Sample...() API entrypoints.
static void
TimeSamplingTest()
{
    // Open test USD in delegate.
    const std::string usdPath = "timeSampling.usda";
    UsdStageRefPtr stage = UsdStage::Open(usdPath);
    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex>
        renderIndex(HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    std::unique_ptr<UsdImagingDelegate>
        delegate(new UsdImagingDelegate(renderIndex.get(),
                                        SdfPath::AbsoluteRootPath()));
    delegate->Populate(stage->GetPseudoRoot());
    //
    // SampleTransform
    //
    const GfMatrix4d expectedXf(GfVec4d(1.0, 1.0, 1.0, 1.0));
    SdfPath spherePath("/Sphere");
    HdTimeSampleArray<GfMatrix4d, 8> xfSamples;
    delegate->SetTime(0.0);
    delegate->SyncAll(/*includeUnvarying*/true);
    delegate->HdSceneDelegate::SampleTransform(spherePath, &xfSamples);
    TF_VERIFY(xfSamples.count == 1);
    TF_VERIFY(xfSamples.times[0] == 0);
    TF_VERIFY(xfSamples.values[0] == expectedXf);
    TF_VERIFY(xfSamples.values[0].ExtractTranslation() == GfVec3d(0));
    // If we retrieve samples before the start of animation, it will
    // return a single sample because the xform is constant.
    delegate->SetTime(-100.0);
    delegate->SyncAll(/*includeUnvarying*/true);
    delegate->HdSceneDelegate::SampleTransform(spherePath, &xfSamples);
    TF_VERIFY(xfSamples.count == 1);
    // Samples are relative to current frame of scene, not absolute time.
    TF_VERIFY(xfSamples.times[0] == 0);
    TF_VERIFY(xfSamples.values[0] == expectedXf);
}

static void
GeomSubsetsTest()
{
    const std::string usdPath = "geomSubsets.usda";
    UsdStageRefPtr stage = UsdStage::Open(usdPath);

    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex>
        renderIndex(HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    TF_VERIFY(renderIndex);
    std::unique_ptr<UsdImagingDelegate>
        delegate(new UsdImagingDelegate(renderIndex.get(),
                                        SdfPath::AbsoluteRootPath()));

    delegate->Populate(stage->GetPseudoRoot());
    delegate->SetTime(0.0);
    delegate->SyncAll(true);

    // Verify geom subsets.
    HdMeshTopology topo = delegate->GetMeshTopology(
        SdfPath("/Sphere/pSphere1"));
    HdGeomSubsets subsets = topo.GetGeomSubsets();

    TF_VERIFY(subsets.size() == 3);
    TF_VERIFY(subsets[0].id == SdfPath("/Sphere/pSphere1/lambert2SG"));
    TF_VERIFY(subsets[0].materialId == SdfPath("/Sphere/Looks/lambert2SG"));
    TF_VERIFY(subsets[0].type == HdGeomSubset::TypeFaceSet);
    TF_VERIFY(subsets[0].indices.size() == 8);
    TF_VERIFY(subsets[1].id == SdfPath("/Sphere/pSphere1/lambert3SG"));
    TF_VERIFY(subsets[1].materialId == SdfPath("/Sphere/Looks/lambert3SG"));
    TF_VERIFY(subsets[1].type == HdGeomSubset::TypeFaceSet);
    TF_VERIFY(subsets[1].indices.size() == 4);
    TF_VERIFY(subsets[2].id == SdfPath("/Sphere/pSphere1/blinn3SG"));
    TF_VERIFY(subsets[2].materialId == SdfPath("/Sphere/Looks/blinn3SG"));
    TF_VERIFY(subsets[2].type == HdGeomSubset::TypeFaceSet);
    TF_VERIFY(subsets[2].indices.size() == 4);
}

static void
GeomSubsetsNestedDelegateTest()
{
    const std::string usdPath = "geomSubsets.usda";
    UsdStageRefPtr stage = UsdStage::Open(usdPath);

    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex>
        renderIndex(HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    TF_VERIFY(renderIndex);
    std::unique_ptr<UsdImagingDelegate>
        delegate(new UsdImagingDelegate(renderIndex.get(),
                                        SdfPath("/NestedDelegate")));

    delegate->Populate(stage->GetPseudoRoot());
    delegate->SetTime(0.0);
    delegate->SyncAll(true);

    // Verify geom subsets.
    HdMeshTopology topo = delegate->GetMeshTopology(
        SdfPath("/Sphere/pSphere1"));
    HdGeomSubsets subsets = topo.GetGeomSubsets();

    TF_VERIFY(subsets.size() == 3);
    TF_VERIFY(subsets[0].id == SdfPath("/NestedDelegate/Sphere/pSphere1/lambert2SG"));
    TF_VERIFY(subsets[0].materialId == SdfPath("/NestedDelegate/Sphere/Looks/lambert2SG"));
    TF_VERIFY(subsets[0].type == HdGeomSubset::TypeFaceSet);
    TF_VERIFY(subsets[0].indices.size() == 8);
    TF_VERIFY(subsets[1].id == SdfPath("/NestedDelegate/Sphere/pSphere1/lambert3SG"));
    TF_VERIFY(subsets[1].materialId == SdfPath("/NestedDelegate/Sphere/Looks/lambert3SG"));
    TF_VERIFY(subsets[1].type == HdGeomSubset::TypeFaceSet);
    TF_VERIFY(subsets[1].indices.size() == 4);
    TF_VERIFY(subsets[2].id == SdfPath("/NestedDelegate/Sphere/pSphere1/blinn3SG"));
    TF_VERIFY(subsets[2].materialId == SdfPath("/NestedDelegate/Sphere/Looks/blinn3SG"));
    TF_VERIFY(subsets[2].type == HdGeomSubset::TypeFaceSet);
    TF_VERIFY(subsets[2].indices.size() == 4);
}

static void
NestedPointInstancersTest()
{
    const std::string usdPath = "nestedPointInstancers.usda";
    UsdStageRefPtr stage = UsdStage::Open(usdPath);

    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex>
        renderIndex(HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    TF_VERIFY(renderIndex);
    std::unique_ptr<UsdImagingDelegate>
        delegate(new UsdImagingDelegate(renderIndex.get(),
                                        SdfPath::AbsoluteRootPath()));

    delegate->Populate(stage->GetPseudoRoot());
    delegate->SetTime(0.0);
    delegate->SyncAll(true);

    TF_VERIFY(renderIndex->HasInstancer(SdfPath("/addpointinstancer1")));
    TF_VERIFY(renderIndex->HasInstancer(SdfPath("/addpointinstancer2")));
    TF_VERIFY(renderIndex->HasInstancer(SdfPath("/addpointinstancer3")));

    // USD-6555 regression test
    VtValue vel = delegate->Get(SdfPath("/addpointinstancer1"), TfToken("velocities"));
    TF_VERIFY(!vel.IsEmpty());
}

int main()
{
    TfErrorMark mark;

    VaryingTest();
    UnvaryingTest();
    VectorizedSetTimesTest();
    RefineLevelTest();
    PrimvarNamesTest1();
    PrimvarNamesTest2();
    PrimvarIndicesTest();
    SamplePrimvarTest();
    TimeSamplingTest();
    GeomSubsetsTest();
    GeomSubsetsNestedDelegateTest();
    NestedPointInstancersTest();

    RemoveTest();

    if (TF_VERIFY(mark.IsClean()))
        std::cout << "OK" << std::endl;
    else
        std::cout << "FAILED" << std::endl;
}

