#include "pxr/usdImaging/usdImaging/unitTestHelper.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/treeIterator.h"
#include "pxr/usd/usdGeom/mesh.h"

#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/gf/frustum.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>
static void
_SetupDriverCamera(UsdImaging_TestDriver* pDriver)
{
    GfFrustum frustum;
    frustum.SetPerspective(45, true, 1.0, 1.0, 10000.0);
    GfMatrix4d projMatrix = frustum.ComputeProjectionMatrix();
    GfMatrix4d viewMatrix = GfMatrix4d().SetIdentity() * 
                            GfMatrix4d().SetTranslate(GfVec3d(0.0, 1000.0, 0.0)) * 
                            GfMatrix4d().SetRotate(GfRotation(GfVec3d(1, 0, 0), -90));
#if HD_API > 2
    GfVec4d viewport(0, 0, 512, 512);
    pDriver->SetCamera(viewMatrix, projMatrix, viewport);
#else
    pDriver->SetCamera(viewMatrix, projMatrix);
#endif
}

void
VaryingTest()
{
    std::string usdPath = "./testUsdImagingDelegate/varying.usda";

    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();

    // Reset all counters we care about.
    perfLog.ResetCache(HdTokens->extent);
    perfLog.ResetCache(HdTokens->points);
    perfLog.ResetCache(HdTokens->topology);
    perfLog.ResetCache(HdTokens->transform);
    perfLog.SetCounter(UsdImagingTokens->usdVaryingExtent, 0);
    perfLog.SetCounter(UsdImagingTokens->usdVaryingPrimVar, 0);
    perfLog.SetCounter(UsdImagingTokens->usdVaryingTopology, 0);
    perfLog.SetCounter(UsdImagingTokens->usdVaryingVisibility, 0);
    perfLog.SetCounter(UsdImagingTokens->usdVaryingXform, 0);
    
    // Variability is reported here, so perfLog must be enabled above.
    UsdImaging_TestDriver driver(usdPath);
    _SetupDriverCamera(&driver);
    TF_VERIFY(perfLog.GetCounter(UsdImagingTokens->usdVaryingExtent) == 1);
    TF_VERIFY(perfLog.GetCounter(UsdImagingTokens->usdVaryingPrimVar) == 1);
    TF_VERIFY(perfLog.GetCounter(UsdImagingTokens->usdVaryingXform) == 1);
    TF_VERIFY(perfLog.GetCounter(UsdImagingTokens->usdVaryingVisibility) == 1);
    TF_VERIFY(perfLog.GetCounter(UsdImagingTokens->usdVaryingTopology) == 0);

    driver.SetTime(1);
    driver.Draw();

    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->extent) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->points) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->topology) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->transform) == 1);
    TF_VERIFY(perfLog.GetCounter(HdTokens->itemsDrawn) == 1, 
                "drawn: %f\n", perfLog.GetCounter(HdTokens->itemsDrawn));

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
    TF_VERIFY(perfLog.GetCounter(HdTokens->itemsDrawn) == 1, 
                "drawn: %f\n", perfLog.GetCounter(HdTokens->itemsDrawn));

    // Reset and use the vectorized SetTimes API
    perfLog.ResetCache(HdTokens->extent);
    perfLog.ResetCache(HdTokens->points);
    perfLog.ResetCache(HdTokens->topology);
    perfLog.ResetCache(HdTokens->transform);
    perfLog.SetCounter(HdTokens->itemsDrawn, 0);
    
    UsdImaging_TestDriver driver2(usdPath);
    driver2.SetTime(1.0);
    _SetupDriverCamera(&driver2);
    driver2.Draw();

    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->extent) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->points) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->topology) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->transform) == 1);
    TF_VERIFY(perfLog.GetCounter(HdTokens->itemsDrawn) == 1);

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
    TF_VERIFY(perfLog.GetCounter(HdTokens->itemsDrawn) == 1);
}

void
UnvaryingTest()
{
    std::string usdPath = "./testUsdImagingDelegate/unvarying.usda";

    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    
    // Reset all counters we care about.
    perfLog.ResetCache(HdTokens->extent);
    perfLog.ResetCache(HdTokens->points);
    perfLog.ResetCache(HdTokens->topology);
    perfLog.ResetCache(HdTokens->transform);
    perfLog.SetCounter(UsdImagingTokens->usdVaryingExtent, 0);
    perfLog.SetCounter(UsdImagingTokens->usdVaryingPrimVar, 0);
    perfLog.SetCounter(UsdImagingTokens->usdVaryingTopology, 0);
    perfLog.SetCounter(UsdImagingTokens->usdVaryingVisibility, 0);
    perfLog.SetCounter(UsdImagingTokens->usdVaryingXform, 0);

    // Variability is reported here, so perfLog must be enabled above.
    UsdImaging_TestDriver driver(usdPath);
    _SetupDriverCamera(&driver);

    TF_VERIFY(perfLog.GetCounter(UsdImagingTokens->usdVaryingExtent) == 0);
    TF_VERIFY(perfLog.GetCounter(UsdImagingTokens->usdVaryingPrimVar) == 0);
    TF_VERIFY(perfLog.GetCounter(UsdImagingTokens->usdVaryingTopology) == 0);
    TF_VERIFY(perfLog.GetCounter(UsdImagingTokens->usdVaryingVisibility) == 0);
    TF_VERIFY(perfLog.GetCounter(UsdImagingTokens->usdVaryingXform) == 0);

    driver.Draw();

    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->extent) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->points) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->topology) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->transform) == 1);
    TF_VERIFY(perfLog.GetCounter(HdTokens->itemsDrawn) == 1);

    driver.SetTime(2);
    driver.Draw();

    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->extent) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->points) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->topology) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->transform) == 1);
    TF_VERIFY(perfLog.GetCounter(HdTokens->itemsDrawn) == 1);

    // Reset and use the vectorized SetTimes API
    perfLog.ResetCache(HdTokens->extent);
    perfLog.ResetCache(HdTokens->points);
    perfLog.ResetCache(HdTokens->topology);
    perfLog.ResetCache(HdTokens->transform);
    perfLog.SetCounter(HdTokens->itemsDrawn, 0);
    
    UsdImaging_TestDriver driver2(usdPath);
    _SetupDriverCamera(&driver2);

    driver2.Draw();

    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->extent) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->points) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->topology) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->transform) == 1);
    TF_VERIFY(perfLog.GetCounter(HdTokens->itemsDrawn) == 1);

    UsdImagingDelegate::SetTimes(
        std::vector<UsdImagingDelegate*>(1, &driver2.GetDelegate()),
        std::vector<UsdTimeCode>(1, UsdTimeCode(2)));
    driver2.Draw();

    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->extent) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->points) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->topology) == 1);
    TF_VERIFY(perfLog.GetCacheMisses(HdTokens->transform) == 1);
    TF_VERIFY(perfLog.GetCounter(HdTokens->itemsDrawn) == 1);
}

void
VectorizedSetTimesTest()
{
    std::string unvaryingUsdPath = "./testUsdImagingDelegate/unvarying.usda";
    std::string varyingUsdPath = "./testUsdImagingDelegate/varying.usda";

    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();

    // Reset all counters we care about.
    perfLog.ResetCache(HdTokens->extent);
    perfLog.ResetCache(HdTokens->points);
    perfLog.ResetCache(HdTokens->topology);
    perfLog.ResetCache(HdTokens->transform);

    UsdImaging_TestDriver varyingDriver(varyingUsdPath);
    _SetupDriverCamera(&varyingDriver);

    UsdImaging_TestDriver unvaryingDriver(unvaryingUsdPath);
    _SetupDriverCamera(&unvaryingDriver);

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
    std::string usdPath = "./testUsdImagingDelegate/unvarying.usda";
    UsdImaging_TestDriver driver(usdPath);
    UsdImagingDelegate& delegate = driver.GetDelegate();
    UsdStageRefPtr const& stage = driver.GetStage();
    HdChangeTracker& tracker = delegate.GetRenderIndex().GetChangeTracker();

    TF_VERIFY(delegate.GetRefineLevelFallback() == 0);
    for (UsdTreeIterator it = stage->Traverse();it;++it) {
        if (it->IsA<UsdGeomMesh>()) {
            TF_VERIFY(delegate.GetRefineLevel(it->GetPath()) == 0);
            TF_VERIFY(tracker.IsRefineLevelDirty(it->GetPath()));
            tracker.MarkRprimClean(it->GetPath());
            TF_VERIFY(not tracker.IsRefineLevelDirty(it->GetPath()));
        }
    }

    delegate.SetRefineLevelFallback(0);
    for (UsdTreeIterator it = stage->Traverse();it;++it) {
        if (it->IsA<UsdGeomMesh>()) {
            TF_VERIFY(delegate.GetRefineLevel(it->GetPath()) == 0);
            // Should not be dirty because the level didn't actually change
            TF_VERIFY(not tracker.IsRefineLevelDirty(it->GetPath()));

            // Set the value to the existing value
            delegate.SetRefineLevel(it->GetPath(), 0);
            // Should not be dirty because the level didn't actually change
            TF_VERIFY(not tracker.IsRefineLevelDirty(it->GetPath()));
        }
    }

    // All prims have an explicit refine level, so setting the fallback should
    // not affect them.
    delegate.SetRefineLevelFallback(8);
    for (UsdTreeIterator it = stage->Traverse();it;++it) {
        if (it->IsA<UsdGeomMesh>()) {
            // Verify value and dirty
            TF_VERIFY(delegate.GetRefineLevel(it->GetPath()) == 0);
            TF_VERIFY(not tracker.IsRefineLevelDirty(it->GetPath()));
            // Clear, clean & verify
            delegate.ClearRefineLevel(it->GetPath());
            TF_VERIFY(delegate.GetRefineLevel(it->GetPath()) == 8);
            TF_VERIFY(tracker.IsRefineLevelDirty(it->GetPath()));
            tracker.MarkRprimClean(it->GetPath());
            TF_VERIFY(not tracker.IsRefineLevelDirty(it->GetPath()));
        }
    }

    // All explicit values are removed, verify fallback changes.
    delegate.SetRefineLevelFallback(1);
    for (UsdTreeIterator it = stage->Traverse();it;++it) {
        if (it->IsA<UsdGeomMesh>()) {
            // Verify value and dirty
            TF_VERIFY(delegate.GetRefineLevel(it->GetPath()) == 1);
            TF_VERIFY(tracker.IsRefineLevelDirty(it->GetPath()));

            // Clean & verify clean
            tracker.MarkRprimClean(it->GetPath());
            TF_VERIFY(not tracker.IsRefineLevelDirty(it->GetPath()));

            // Set to existing & verify clean 
            delegate.SetRefineLevel(it->GetPath(), 1);
            TF_VERIFY(not tracker.IsRefineLevelDirty(it->GetPath()));

            // Set to new value & verify dirty
            delegate.SetRefineLevel(it->GetPath(), 2);
            TF_VERIFY(delegate.GetRefineLevel(it->GetPath()) == 2);
            TF_VERIFY(tracker.IsRefineLevelDirty(it->GetPath()));

            // Clean & verify
            tracker.MarkRprimClean(it->GetPath());
            TF_VERIFY(not tracker.IsRefineLevelDirty(it->GetPath()));
            
            // Set to existing explicit value & verify clean
            delegate.SetRefineLevel(it->GetPath(), 2);
            TF_VERIFY(not tracker.IsRefineLevelDirty(it->GetPath()));

            // Set the fallback, but because we expressed an opinion for this
            // specific prim above, we don't expect the value to change.
            delegate.SetRefineLevelFallback(3);
            TF_VERIFY(delegate.GetRefineLevel(it->GetPath()) == 2);
            // This prim should also not be dirty.
            TF_VERIFY(not tracker.IsRefineLevelDirty(it->GetPath()));

            // Clear the explicit refine level, expect dirty and fallback.
            delegate.ClearRefineLevel(it->GetPath());
            TF_VERIFY(delegate.GetRefineLevel(it->GetPath()) == 3);
            TF_VERIFY(tracker.IsRefineLevelDirty(it->GetPath()));

            // Clean, no-op clear, expect clean and fallback.
            tracker.MarkRprimClean(it->GetPath());
            delegate.ClearRefineLevel(it->GetPath());
            TF_VERIFY(delegate.GetRefineLevel(it->GetPath()) == 3);
            TF_VERIFY(not tracker.IsRefineLevelDirty(it->GetPath()));
        }
    }
}

void
PrimVarNamesTest1()
{
    SdfPath meshPath("/pCube1");
    std::string usdPath = "./testUsdImagingDelegate/unvarying.usda";
    UsdStageRefPtr stage = UsdStage::Open(usdPath);
    UsdImagingDelegate delegate;

    // Only populate is called here, which we want to ensure is enough to
    // populate primvar names.
    delegate.Populate(stage->GetPseudoRoot());
    delegate.SetTime(1.0);
    delegate.SyncAll(/*includeUnvarying*/true);

    // Verify expected names.
    TfTokenVector names = delegate.GetPrimVarVertexNames(meshPath);
    TF_VERIFY(names.size() == 1);
    TF_VERIFY(names[0] == TfToken("points"));

    names = delegate.GetPrimVarConstantNames(meshPath);
    TF_VERIFY(names.size() == 1);
    TF_VERIFY(names[0] == TfToken("color"));
}

void
PrimVarNamesTest2()
{
    SdfPath meshPath("/pCube1");
    std::string usdPath = "./testUsdImagingDelegate/unvarying.usda";
    UsdStageRefPtr stage = UsdStage::Open(usdPath);
    UsdImagingDelegate delegate;
    
    // Setting the time after calling Populate here triggers two updates to the
    // primvar names, the test here is to ensure we accumulate primvars.
    delegate.Populate(stage->GetPseudoRoot());
    delegate.SetTime(1.0);
    delegate.SyncAll(true);

    // Verify expected names.
    TfTokenVector names = delegate.GetPrimVarVertexNames(meshPath);
    TF_VERIFY(names.size() == 1);
    TF_VERIFY(names[0] == TfToken("points"));

    names = delegate.GetPrimVarConstantNames(meshPath);
    TF_VERIFY(names.size() == 1);
    TF_VERIFY(names[0] == TfToken("color"));
}

void
RemoveTest()
{
    HdRenderIndexSharedPtr renderIndex(new HdRenderIndex);
    HdRenderPass renderPass(&*renderIndex,
                            HdRprimCollection(HdTokens->geometry,
                                              HdTokens->smoothHull));
    std::string usdPath = "./testUsdImagingDelegate/test.usda";

    {
        UsdImagingDelegate delegate(renderIndex, SdfPath("/delegateId"));
        UsdStageRefPtr stage = UsdStage::Open(usdPath);

        delegate.Populate(stage->GetPseudoRoot());
        delegate.SetTime(1.0);
        delegate.SyncAll(true);

        renderPass.Sync();
        renderIndex->SyncAll();

        TF_VERIFY(renderIndex->GetRprim(SdfPath("/delegateId/mesh1")));
        TF_VERIFY(renderIndex->GetShader(SdfPath("/delegateId/Shaders/SurfUvTexture1")));
        TF_VERIFY(renderIndex->GetTexture(SdfPath("/delegateId/Shaders/SurfUvTexture1.diffuseColor:texture")));

        // destroy delegate.
    }

    // should not exist (shader should be the fallback)
    TF_VERIFY(not renderIndex->GetRprim(SdfPath("/delegateId/mesh1")));
    TF_VERIFY(renderIndex->GetShader(SdfPath("/delegateId/Shaders/SurfUvTexture1"))
              == renderIndex->GetShaderFallback());
    TF_VERIFY(not renderIndex->GetTexture(SdfPath("/delegateId/Shaders/SurfUvTexture1.diffuseColor:texture")));

    // should successfully sync after deletion
    renderPass.Sync();
    renderIndex->SyncAll();
}

int main()
{
    TfErrorMark mark;

    VaryingTest();
    UnvaryingTest();
    VectorizedSetTimesTest();
    RefineLevelTest();
    PrimVarNamesTest1();
    PrimVarNamesTest2();
    RemoveTest();

    if (TF_VERIFY(mark.IsClean()))
        std::cout << "OK" << std::endl;
    else
        std::cout << "FAILED" << std::endl;
}

