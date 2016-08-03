#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/unitTestHelper.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/testGLContext.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/gf/vec3f.h"

#include <iostream>

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
    // Get the keys in sorted order.  This ensures consistent reporting
    // regardless of the sort order of dict.
    std::set<std::string> keys;
    for (auto v: dict) {
        keys.insert(v.first);
    }

    std::cout << message;
    for (auto key: keys) {
        std::cout << key << ", ";
        const VtValue& value = dict[key];
        if (value.IsHolding<size_t>()) {
            std::cout << value.Get<size_t>();
        }
        std::cout << "\n";
    }
    PrintPerfCounter(perfLog, HdPerfTokens->garbageCollected);
    PrintPerfCounter(perfLog, HdPerfTokens->meshTopology);
    PrintPerfCounter(perfLog, HdPerfTokens->basisCurvesTopology);
    PrintPerfCounter(perfLog, HdPerfTokens->instMeshTopology);
    PrintPerfCounter(perfLog, HdPerfTokens->instBasisCurvesTopology);
    PrintPerfCounter(perfLog, HdPerfTokens->instVertexAdjacency);
    PrintPerfCounter(perfLog, HdPerfTokens->instMeshTopologyRange);
    PrintPerfCounter(perfLog, HdPerfTokens->instBasisCurvesTopologyRange);
}

static void
BasicTest()
{
    HdResourceRegistry *resourceRegistry = &HdResourceRegistry::GetInstance();

    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    // begin with 0
    VtDictionary dict = resourceRegistry->GetResourceAllocation();
    Dump("----- begin -----\n", dict, perfLog);

    GfMatrix4f identity;
    identity.SetIdentity();

    Hd_TestDriver driver;
    Hd_UnitTestDelegate &delegate = driver.GetDelegate();
    delegate.AddCube(SdfPath("/cube0"), identity);
    delegate.AddCube(SdfPath("/cube1"), identity);
    delegate.AddGrid(SdfPath("/plane0"), 1, 1, identity);
    delegate.AddGrid(SdfPath("/plane1"), 10, 10, identity);
    delegate.AddGrid(SdfPath("/plane2"), 10, 10, identity);
    delegate.AddGrid(SdfPath("/plane3"), 20, 20, identity);
    delegate.AddCurves(SdfPath("/curves1"), HdTokens->linear, identity);
    delegate.AddCurves(SdfPath("/curves2"), HdTokens->linear, identity);
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
ResizeTest()
{
    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    // layout
    HdBufferSpecVector bufferSpecs;
    bufferSpecs.push_back(HdBufferSpec(HdTokens->points, GL_FLOAT, 3));

    // write
    HdBufferArrayRangeSharedPtr range =
        registry->AllocateNonUniformBufferArrayRange(HdTokens->primVar, bufferSpecs);
    HdBufferSourceVector sources;

    // 3 points
    VtArray<GfVec3f> points(3);
    for (int i = 0; i < 3; ++i) points[i] = GfVec3f(i);
    sources.push_back(HdBufferSourceSharedPtr(
                          new HdVtBufferSource(HdTokens->points, VtValue(points))));

    registry->AddSources(range, sources);
    registry->Commit();

    VtDictionary dict = registry->GetResourceAllocation();
    Dump("----- 3 points -----\n", dict, perfLog);

    // resize
    points = VtArray<GfVec3f>(5);
    for (int i = 0; i < 5; ++i) points[i] = GfVec3f(i);
    sources.clear();
    sources.push_back(HdBufferSourceSharedPtr(
                          new HdVtBufferSource(HdTokens->points, VtValue(points))));

    registry->AddSources(range, sources);
    registry->Commit();

    dict = registry->GetResourceAllocation();
    Dump("----- 5 points -----\n", dict, perfLog);

    // shrink
    points = VtArray<GfVec3f>(4);
    for (int i = 0; i < 4; ++i) points[i] = GfVec3f(i);
    sources.clear();
    sources.push_back(HdBufferSourceSharedPtr(
                          new HdVtBufferSource(HdTokens->points, VtValue(points))));

    registry->AddSources(range, sources);
    registry->Commit();

    dict = registry->GetResourceAllocation();
    Dump("----- 4 points before GC -----\n", dict, perfLog);

    // GC
    registry->GarbageCollect();

    dict = registry->GetResourceAllocation();
    Dump("----- 4 points after GC -----\n", dict, perfLog);
}

static void
MergeTest()
{
    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    // 3 points + normals
    VtArray<GfVec3f> points(3), normals(3);
    for (int i = 0; i < 3; ++i) {
        points[i] = GfVec3f(i);
        normals[i] = GfVec3f(-i);
    }

    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferArrayRangeMerged) == 0);

    // send points -----------------------------------------------------------
    HdBufferSourceVector sources;
    sources.push_back(HdBufferSourceSharedPtr(
                          new HdVtBufferSource(
                              HdTokens->points, VtValue(points))));

    // allocate range
    HdBufferSpecVector bufferSpecs;
    HdBufferSpec::AddBufferSpecs(&bufferSpecs, sources);
    HdBufferArrayRangeSharedPtr range =
        registry->AllocateNonUniformBufferArrayRange(
            HdTokens->primVar, bufferSpecs);

    registry->AddSources(range, sources);
    registry->Commit();

    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferArrayRangeMerged) == 0);
    TF_VERIFY(points == range->ReadData(HdTokens->points));

    // send points + normals -------------------------------------------------
    sources.clear();
    sources.push_back(HdBufferSourceSharedPtr(
                          new HdVtBufferSource(
                              HdTokens->points, VtValue(points))));
    sources.push_back(HdBufferSourceSharedPtr(
                          new HdVtBufferSource(
                              HdTokens->normals, VtValue(normals))));
    bufferSpecs.clear();
    HdBufferSpec::AddBufferSpecs(&bufferSpecs, sources);

    // merge
    range = registry->MergeNonUniformBufferArrayRange(
        HdTokens->primVar, bufferSpecs, range);

    registry->AddSources(range, sources);
    registry->Commit();

    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferArrayRangeMerged) == 1);
    TF_VERIFY(points == range->ReadData(HdTokens->points));
    TF_VERIFY(normals == range->ReadData(HdTokens->normals));

    // send normals ---------------------------------------------------------
    sources.clear();
    sources.push_back(HdBufferSourceSharedPtr(
                          new HdVtBufferSource(
                              HdTokens->normals, VtValue(normals))));
    bufferSpecs.clear();
    HdBufferSpec::AddBufferSpecs(&bufferSpecs, sources);

    // merge
    range = registry->MergeNonUniformBufferArrayRange(
        HdTokens->primVar, bufferSpecs, range);

    registry->AddSources(range, sources);
    registry->Commit();

    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferArrayRangeMerged) == 1);
    TF_VERIFY(points == range->ReadData(HdTokens->points));
    TF_VERIFY(normals == range->ReadData(HdTokens->normals));
}

static void
BarShareTest()
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    HdBufferArrayRangeContainer barContainer(HdDrawingCoord::DefaultNumSlots);
    HdDrawingCoord drawingCoord;

    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferArrayRangeContainerResized) == 0);

    barContainer.Set(drawingCoord.GetConstantPrimVarIndex(), HdBufferArrayRangeSharedPtr());
    barContainer.Set(drawingCoord.GetVertexPrimVarIndex(), HdBufferArrayRangeSharedPtr());
    barContainer.Set(drawingCoord.GetTopologyIndex(), HdBufferArrayRangeSharedPtr());

    // Constant, VertexPrimVar, Topology is allocated by default.
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferArrayRangeContainerResized) == 0);

    // when ElementPrimVar is requested, the container should be resized.
    barContainer.Set(drawingCoord.GetElementPrimVarIndex(), HdBufferArrayRangeSharedPtr());
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferArrayRangeContainerResized) == 1);

    // same for instance index
    barContainer.Set(drawingCoord.GetInstanceIndexIndex(), HdBufferArrayRangeSharedPtr());
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferArrayRangeContainerResized) == 2);

    // IntancePrimvar always comes at the very end. The container will be resized.
    barContainer.Set(drawingCoord.GetInstancePrimVarIndex(/*depth=*/0),
                     HdBufferArrayRangeSharedPtr());
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferArrayRangeContainerResized) == 3);
}

int main()
{
    GlfTestGLContext::RegisterGLContextCallbacks();
    GlfGlewInit();
    GlfSharedGLContextScopeHolder sharedContext;

    TfErrorMark mark;

    BasicTest();

    ResizeTest();

    MergeTest();

    BarShareTest();

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

