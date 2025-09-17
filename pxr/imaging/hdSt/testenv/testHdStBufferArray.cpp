//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/imaging/hd/drawingCoord.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/glf/testGLContext.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/gf/vec3f.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

#define VERIFY_NULL(dict, token)                        \
    TF_VERIFY(dict.count(token.GetString()) == 0);

#define VERIFY_SIZE(dict, token, size)                          \
    TF_VERIFY(dict[token.GetString()] == VtValue(size_t(size)));


static void
PrintPerfCounter(HdPerfLog &perfLog, TfToken const &token)
{
    std::cout << token << " = " << perfLog.GetCounter(token) << "\n";
}

static void
Dump(std::string const &message, VtDictionary dict, HdPerfLog &perfLog)
{
    // These vary between platforms and runs, we don't want them in the diff.
    static const std::unordered_set<std::string> skippedKeys = {
        HdTokens->drawingShader,
        HdTokens->computeShader,
        HdPerfTokens->gpuMemoryUsed,
        HdPerfTokens->uboSize,
        HdPerfTokens->ssboSize,
    };

    // Get the keys in sorted order. This ensures consistent reporting
    // regardless of the sort order of dict.
    std::set<std::string> keys;
    for (const auto& [key, _] : dict) {
        if (!skippedKeys.count(key)) {
            keys.insert(key);
        }
    }

    std::cout << message;
    for (const auto& key: keys) {
        std::cout << key << ", ";
        const VtValue& value = dict[key];
        if (value.IsHolding<size_t>()) {
            std::cout << value.Get<size_t>();
        }
        std::cout << "\n";
    }
    PrintPerfCounter(perfLog, HdPerfTokens->garbageCollected);
    PrintPerfCounter(perfLog, HdPerfTokens->instMeshTopology);
    PrintPerfCounter(perfLog, HdPerfTokens->instBasisCurvesTopology);
    PrintPerfCounter(perfLog, HdPerfTokens->instVertexAdjacency);
    PrintPerfCounter(perfLog, HdPerfTokens->instMeshTopologyRange);
    PrintPerfCounter(perfLog, HdPerfTokens->instBasisCurvesTopologyRange);
}

static void
BasicTest(HdSt_TestDriver & driver)
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    HdUnitTestDelegate &delegate = driver.GetDelegate();
    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        std::static_pointer_cast<HdStResourceRegistry>(
        delegate.GetRenderIndex().GetResourceRegistry());

    // begin with 0
    VtDictionary dict = resourceRegistry->GetResourceAllocation();
    Dump("----- begin -----\n", dict, perfLog);

    GfMatrix4f identity;
    identity.SetIdentity();

    delegate.AddCube(SdfPath("/cube0"), identity);
    delegate.AddCube(SdfPath("/cube1"), identity);
    delegate.AddGrid(SdfPath("/plane0"), 1, 1, identity);
    delegate.AddGrid(SdfPath("/plane1"), 10, 10, identity);
    delegate.AddGrid(SdfPath("/plane2"), 10, 10, identity);
    delegate.AddGrid(SdfPath("/plane3"), 20, 20, identity);
    delegate.AddCurves(SdfPath("/curves1"), HdTokens->linear, TfToken(), identity);
    delegate.AddCurves(SdfPath("/curves2"), HdTokens->linear, TfToken(), identity);
    driver.Draw();

    // all allocated
    dict = resourceRegistry->GetResourceAllocation();
    Dump("----- allocated -----\n", dict, perfLog);

    // delete a geom
    delegate.Remove(SdfPath("/cube0"));

    // should be same, because we didn't call garbage collection explicitly.
    dict = resourceRegistry->GetResourceAllocation();
    Dump("----- delete a prim -----\n", dict, perfLog);

    // draw triggers garbage collection
    driver.Draw();

    dict = resourceRegistry->GetResourceAllocation();
    Dump("----- garbage collected -----\n", dict, perfLog);

    // delete more
    delegate.Remove(SdfPath("/cube1"));
    delegate.Remove(SdfPath("/plane1"));
    delegate.Remove(SdfPath("/curves1"));

    driver.Draw();

    dict = resourceRegistry->GetResourceAllocation();
    Dump("----- delete more prims -----\n", dict, perfLog);

    // clear all
    delegate.Clear();

    // explicit compaction
    resourceRegistry->GarbageCollect();

    dict = resourceRegistry->GetResourceAllocation();
    Dump("----- clear all -----\n", dict, perfLog);
}

static void
ResizeTest(HdSt_TestDriver & driver)
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    HdUnitTestDelegate &delegate = driver.GetDelegate();
    HdStResourceRegistrySharedPtr const& registry = 
        std::static_pointer_cast<HdStResourceRegistry>(
        delegate.GetRenderIndex().GetResourceRegistry());

    // layout
    HdBufferSpecVector bufferSpecs;
    bufferSpecs.emplace_back(HdTokens->points,
                             HdTupleType {HdTypeFloatVec3, 1});

    // write
    HdBufferArrayRangeSharedPtr range =
        registry->AllocateNonUniformBufferArrayRange(
            HdTokens->primvar,
            bufferSpecs,
            HdBufferArrayUsageHintBitsVertex);
    // 3 points
    VtArray<GfVec3f> points(3);
    for (int i = 0; i < 3; ++i) points[i] = GfVec3f(i);
    registry->AddSources(range,
                         { std::make_shared<HdVtBufferSource>(
                               HdTokens->points, VtValue(points)) });
    registry->Commit();

    VtDictionary dict = registry->GetResourceAllocation();
    Dump("----- 3 points -----\n", dict, perfLog);

    // resize
    points = VtArray<GfVec3f>(5);
    for (int i = 0; i < 5; ++i) points[i] = GfVec3f(i);
    registry->AddSources(range,
                         { std::make_shared<HdVtBufferSource>(
                               HdTokens->points, VtValue(points)) });
    registry->Commit();

    dict = registry->GetResourceAllocation();
    Dump("----- 5 points -----\n", dict, perfLog);

    // shrink
    points = VtArray<GfVec3f>(4);
    for (int i = 0; i < 4; ++i) points[i] = GfVec3f(i);
    registry->AddSources(range,
                         { std::make_shared<HdVtBufferSource>(
                               HdTokens->points, VtValue(points)) });
    registry->Commit();

    dict = registry->GetResourceAllocation();
    Dump("----- 4 points before GC -----\n", dict, perfLog);

    // GC
    registry->GarbageCollect();

    dict = registry->GetResourceAllocation();
    Dump("----- 4 points after GC -----\n", dict, perfLog);

    // shrink to 0
    points = VtArray<GfVec3f>(0);
    registry->AddSources(range,
                         { std::make_shared<HdVtBufferSource>(
                               HdTokens->points, VtValue(points)) });
    registry->Commit();

    dict = registry->GetResourceAllocation();
    Dump("----- 0 points after GC -----\n", dict, perfLog);
}

static void
MergeTest(HdSt_TestDriver & driver)
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    HdUnitTestDelegate &delegate = driver.GetDelegate();
    HdStResourceRegistrySharedPtr const& registry = 
        std::static_pointer_cast<HdStResourceRegistry>(
        delegate.GetRenderIndex().GetResourceRegistry());

    // 3 points + normals
    VtArray<GfVec3f> points(3), normals(3);
    for (int i = 0; i < 3; ++i) {
        points[i] = GfVec3f(i);
        normals[i] = GfVec3f(-i);
    }

    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferArrayRangeMigrated) == 0);

    // send points -----------------------------------------------------------
    HdBufferSourceSharedPtrVector sources = {
        std::make_shared<HdVtBufferSource>(
            HdTokens->points, VtValue(points)) };

    // allocate range
    HdBufferSpecVector bufferSpecs;
    HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);
    HdBufferArrayRangeSharedPtr range =
        registry->AllocateNonUniformBufferArrayRange(
            HdTokens->primvar, bufferSpecs, HdBufferArrayUsageHintBitsVertex);

    registry->AddSources(range, std::move(sources));
    registry->Commit();

    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferArrayRangeMigrated) == 0);
    TF_VERIFY(points == range->ReadData(HdTokens->points));

    // send points + normals -------------------------------------------------
    sources = {
        std::make_shared<HdVtBufferSource>(
            HdTokens->points, VtValue(points)),
        std::make_shared<HdVtBufferSource>(
            HdTokens->normals, VtValue(normals)) };
    bufferSpecs.clear();
    HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);

    // migrate
    range = registry->UpdateNonUniformBufferArrayRange(
        HdTokens->primvar, range, bufferSpecs,
        /*removedSpecs*/HdBufferSpecVector(), HdBufferArrayUsageHintBitsVertex);

    registry->AddSources(range, std::move(sources));
    registry->Commit();

    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferArrayRangeMigrated) == 1);
    TF_VERIFY(points == range->ReadData(HdTokens->points));
    TF_VERIFY(normals == range->ReadData(HdTokens->normals));

    // send normals ---------------------------------------------------------
    sources = {
        std::make_shared<HdVtBufferSource>(
            HdTokens->normals, VtValue(normals)) };
    bufferSpecs.clear();
    HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);

    // migrate
    range = registry->UpdateNonUniformBufferArrayRange(
        HdTokens->primvar, range, bufferSpecs,
        /*removedSpecs*/HdBufferSpecVector(), HdBufferArrayUsageHintBitsVertex);

    registry->AddSources(range, std::move(sources));
    registry->Commit();

    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferArrayRangeMigrated) == 1);
    TF_VERIFY(points == range->ReadData(HdTokens->points));
    TF_VERIFY(normals == range->ReadData(HdTokens->normals));
}

static void
BarShareTest(HdSt_TestDriver & driver)
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    HdBufferArrayRangeContainer barContainer(HdDrawingCoord::DefaultNumSlots);
    HdDrawingCoord drawingCoord;
    drawingCoord.SetInstancePrimvarBaseIndex(HdDrawingCoord::CustomSlotsBegin);

    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferArrayRangeContainerResized) == 0);

    barContainer.Set(drawingCoord.GetConstantPrimvarIndex(), HdBufferArrayRangeSharedPtr());
    barContainer.Set(drawingCoord.GetVertexPrimvarIndex(), HdBufferArrayRangeSharedPtr());
    barContainer.Set(drawingCoord.GetTopologyIndex(), HdBufferArrayRangeSharedPtr());

    // Constant, VertexPrimvar, Topology is allocated by default.
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferArrayRangeContainerResized) == 0);

    // when ElementPrimvar is requested, the container should be resized.
    barContainer.Set(drawingCoord.GetElementPrimvarIndex(), HdBufferArrayRangeSharedPtr());
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferArrayRangeContainerResized) == 1);

    // same for instance index
    barContainer.Set(drawingCoord.GetInstanceIndexIndex(), HdBufferArrayRangeSharedPtr());
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferArrayRangeContainerResized) == 2);

    // IntancePrimvar always comes at the very end. The container will be resized.
    barContainer.Set(drawingCoord.GetInstancePrimvarIndex(/*depth=*/0),
                     HdBufferArrayRangeSharedPtr());
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferArrayRangeContainerResized) == 3);
}

int main()
{
    GlfTestGLContext::RegisterGLContextCallbacks();
    GlfSharedGLContextScopeHolder sharedContext;

    TfErrorMark mark;

    HdSt_TestDriver driver;
    driver.SetupAovs(256, 256);

    BasicTest(driver);

    ResizeTest(driver);

    MergeTest(driver);

    BarShareTest(driver);

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
