//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdSt/bufferResource.h"
#include "pxr/imaging/hdSt/computation.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hdSt/vboMemoryManager.h"
#include "pxr/imaging/hdSt/vboSimpleMemoryManager.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/capabilities.h"

#include "pxr/imaging/garch/glDebugWindow.h"
#include "pxr/imaging/glf/testGLContext.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3f.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/stl.h"

#include <iostream>
#include <sstream>

PXR_NAMESPACE_USING_DIRECTIVE

static std::ostringstream testLog;

class _ResizeComputation : public HdStComputation
{
public:
    _ResizeComputation(int numElements) : _numElements(numElements) { }

    void Execute(HdBufferArrayRangeSharedPtr const &range,
                 HdResourceRegistry *resourceRegistry) override { }
    void GetBufferSpecs(HdBufferSpecVector *specs) const override { }
    int GetNumOutputElements() const override { return _numElements; }

private:
    int _numElements;
};

static size_t
GetGPUMemoryUsed(HdStResourceRegistry * registry)
{
    VtDictionary allocation = registry->GetResourceAllocation();

    VtValue memUsed;
    TF_VERIFY(TfMapLookup(allocation, HdPerfTokens->gpuMemoryUsed, &memUsed));
    TF_VERIFY(memUsed.IsHolding<size_t>());

    return memUsed.Get<size_t>();
}

static void
BasicTest(HdStResourceRegistry * registry)
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.ResetCounters();

    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 0);
    TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferCpuToGpu) == 0);
    TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferGpuToGpu) == 0);

    // layout
    HdBufferSpecVector bufferSpecs;
    bufferSpecs.emplace_back(HdTokens->points,
                             HdTupleType {HdTypeFloatVec3, 1});
    bufferSpecs.emplace_back(HdTokens->displayColor,
                             HdTupleType {HdTypeFloatVec3, 1});

    {
        // write
        HdBufferArrayRangeSharedPtr const range =
            registry->AllocateNonUniformBufferArrayRange(
                HdTokens->primvar,
                bufferSpecs,
                HdBufferArrayUsageHintBitsVertex);
        
        // points
        VtArray<GfVec3f> points(3);
        points[0] = GfVec3f(0);
        points[1] = GfVec3f(1);
        points[2] = GfVec3f(2);
        
        // colors
        VtArray<GfVec3f> colors(3);
        colors[0] = GfVec3f(1, 1, 1);
        colors[1] = GfVec3f(1, 0, 1);
        colors[2] = GfVec3f(1, 1, 0);
        
        registry->AddSources(range,
                             { std::make_shared<HdVtBufferSource>(
                                   HdTokens->displayColor, VtValue(colors)),
                               std::make_shared<HdVtBufferSource>(
                                   HdTokens->points, VtValue(points)) });
    
        registry->Commit();
        TF_VERIFY(range);

        // read
        TF_VERIFY(points == range->ReadData(HdTokens->points));
        TF_VERIFY(colors == range->ReadData(HdTokens->displayColor));
        
        // check perf counters
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 1);
        TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferCpuToGpu) == 2);
        TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferGpuToGpu) == 0);
        
        points[0] = GfVec3f(10);
        points[1] = GfVec3f(20);
        points[2] = GfVec3f(30);
        
        // write
        registry->AddSources(range,
                             { std::make_shared<HdVtBufferSource>(
                                   HdTokens->points, VtValue(points)) });
        
        registry->Commit();
        TF_VERIFY(range);

        // read
        TF_VERIFY(points == range->ReadData(HdTokens->points));
        TF_VERIFY(colors == range->ReadData(HdTokens->displayColor));
        
        // check perf counters
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 1);
        TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferCpuToGpu) == 3);
        TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferGpuToGpu) == 0);
        
        TF_VERIFY(GetGPUMemoryUsed(registry) > 0);
        
        std::cout << *registry;
    }

    // range shared pointer out of scope
    registry->GarbageCollect();

    TF_VERIFY(GetGPUMemoryUsed(registry) == 0);
}

static void
UniformBasicTest(bool ssbo, HdStResourceRegistry * registry)
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.ResetCounters();

    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 0);
    TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferCpuToGpu) == 0);
    TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferGpuToGpu) == 0);

    HdBufferSpecVector bufferSpecs;
    bufferSpecs.emplace_back(HdTokens->transform,
                             HdTupleType {HdTypeDoubleMat4, 1});
    bufferSpecs.emplace_back(HdTokens->displayColor,
                             HdTupleType {HdTypeFloatVec3, 1});

    HdBufferArrayRangeSharedPtr range;
    if (ssbo) {
        range = registry->AllocateShaderStorageBufferArrayRange(
            HdTokens->primvar,
            bufferSpecs,
            HdBufferArrayUsageHintBitsStorage);
    } else {
        range = registry->AllocateUniformBufferArrayRange(
            HdTokens->primvar,
            bufferSpecs,
            HdBufferArrayUsageHintBitsUniform);
    }

    {
        const GfMatrix4d matrix(1);
        
        // set matrix
        registry->AddSources(range,
                             { std::make_shared<HdVtBufferSource>(
                                     HdTokens->transform, VtValue(matrix)) });
        registry->Commit();
        
        TF_VERIFY(matrix == range->ReadData(HdTokens->transform).Get<VtArray<GfMatrix4d> >()[0]);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 1);
        TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferCpuToGpu) == 1);
        TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferGpuToGpu) == 0);
    }        

    {
        const GfMatrix4d matrix(2);
        // update matrix
        registry->AddSources(range,
                             { std::make_shared<HdVtBufferSource>(
                                     HdTokens->transform, VtValue(matrix)) });
        registry->Commit();

        TF_VERIFY(matrix == range->ReadData(HdTokens->transform).Get<VtArray<GfMatrix4d> >()[0]);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 1);
        TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferCpuToGpu) == 2);
        TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferGpuToGpu) == 0);
    }

    TF_VERIFY(GetGPUMemoryUsed(registry) > 0);

    range.reset();
    registry->GarbageCollect();

    TF_VERIFY(GetGPUMemoryUsed(registry) == 0);
}

struct Prim {
    HdBufferArrayRangeSharedPtr range;
    HdBufferSourceSharedPtrVector sources;
    HdBufferSpecVector bufferSpecs;
    std::map<TfToken, VtValue> primvars;
};

static Prim
_CreatePrim(int numElements, bool colors = true)
{
    Prim prim;

    // add points
    VtArray<GfVec3f> points(numElements);
    for (int i = 0; i < numElements; ++i) {
        points[i] = GfVec3f(i);
    }
    prim.sources.push_back(
        std::make_shared<HdVtBufferSource>(
            HdTokens->points, VtValue(points)));

    prim.primvars[HdTokens->points] = points;
    prim.bufferSpecs.emplace_back(HdTokens->points,
                                  HdTupleType {HdTypeFloatVec3, 1});

    // add colors
    if (colors) {
        VtArray<GfVec3f> colors(numElements);
        for (int i = 0; i < numElements; ++i) {
            colors[i] = GfVec3f(i, i, i);
        }
        prim.sources.push_back(
            std::make_shared<HdVtBufferSource>(
                HdTokens->displayColor, VtValue(colors)));
        prim.primvars[HdTokens->displayColor] = colors;
        prim.bufferSpecs.emplace_back(HdTokens->displayColor,
                                      HdTupleType {HdTypeFloatVec3, 1});
    }

    return prim;
}

static void
AggregationTest(bool aggregation, HdStResourceRegistry *registry)
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.ResetCounters();

    int primCount = 10;
    std::vector<Prim> prims;

    for (int i = 0; i < primCount; ++i) {
        prims.push_back(_CreatePrim((i+1)*10));
    }

    // write
    TF_FOR_ALL (it, prims) {
        if (!it->sources.empty()) {
            it->range = registry->AllocateNonUniformBufferArrayRange(
                HdTokens->primvar, it->bufferSpecs,
                HdBufferArrayUsageHintBitsVertex);
            registry->AddSources(it->range, std::move(it->sources));
        }
        it->sources.clear();
    }
    registry->Commit();

    // read
    TF_FOR_ALL (it, prims) {
        TF_VERIFY(it->primvars[HdTokens->points] ==
                  it->range->ReadData(HdTokens->points));
        TF_VERIFY(it->primvars[HdTokens->displayColor] ==
                  it->range->ReadData(HdTokens->displayColor));
    }

    // check perf counters
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) ==
              (aggregation ? 1 : primCount));
    TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferCpuToGpu) ==
              2*primCount);
    TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferGpuToGpu) == 0);

    perfLog.ResetCounters();

    std::cout << *registry;

    // release partially
    for (int i = primCount-1; i >= 0; --i) {
        if (i%3 != 0) {
            prims.erase(prims.begin() + i);
        }
    }

    registry->Commit();
    std::cout << *registry;

    // read
    TF_FOR_ALL (it, prims) {
        TF_VERIFY(it->primvars[HdTokens->points] ==
                  it->range->ReadData(HdTokens->points));
        TF_VERIFY(it->primvars[HdTokens->displayColor] ==
                  it->range->ReadData(HdTokens->displayColor));
    }

    // allocate new prims
    prims.push_back(_CreatePrim(80));
    prims.push_back(_CreatePrim(90));
    primCount = (int)prims.size();

    // write inefficiently
    TF_FOR_ALL (it, prims) {
        if (!it->sources.empty()) {
            if (!it->range) {
                it->range = registry->AllocateNonUniformBufferArrayRange(
                    HdTokens->primvar,
                    it->bufferSpecs,
                    HdBufferArrayUsageHintBitsVertex);
            }
            registry->AddSources(it->range, std::move(it->sources));
        }
        it->sources.clear();

        // intentionally commit within a loop
        registry->Commit();
    }

    std::cout << *registry;
    // read
    TF_FOR_ALL (it, prims) {
        TF_VERIFY(it->primvars[HdTokens->points] ==
                  it->range->ReadData(HdTokens->points));
        TF_VERIFY(it->primvars[HdTokens->displayColor] ==
                  it->range->ReadData(HdTokens->displayColor));
    }

    // check perf counters
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 2);
    TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferCpuToGpu) == 2*2);
    TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferGpuToGpu) ==
              (aggregation ? 10 : 0));

    perfLog.ResetCounters();

    // allocate new prims with different layout
    prims.push_back(_CreatePrim(11, /*colors=*/true));
    int primIndex1 = prims.size() - 1;
    prims.push_back(_CreatePrim(12, /*colors=*/false));
    int primIndex2 = prims.size() - 1;
    prims.push_back(_CreatePrim(13, /*colors=*/false));
    int primIndex3 = prims.size() - 1;
    Prim &prim1 = prims[primIndex1];
    Prim &prim2 = prims[primIndex2];
    Prim &prim3 = prims[primIndex3];

    // write
    TF_FOR_ALL (it, prims) {
        if (!it->sources.empty()) {
            if (!it->range) {
                it->range = registry->AllocateNonUniformBufferArrayRange(
                    HdTokens->primvar,
                    it->bufferSpecs,
                    HdBufferArrayUsageHintBitsVertex);
            }
            registry->AddSources(it->range, std::move(it->sources));
        }
        TF_VERIFY(it->range);
        it->sources.clear();
    }
    registry->Commit();

    // read
    TF_VERIFY(prim1.primvars[HdTokens->points] ==
              prim1.range->ReadData(HdTokens->points));
    TF_VERIFY(prim1.primvars[HdTokens->displayColor] ==
              prim1.range->ReadData(HdTokens->displayColor));
    TF_VERIFY(prim2.primvars[HdTokens->points] ==
              prim2.range->ReadData(HdTokens->points));
    TF_VERIFY(prim3.primvars[HdTokens->points] ==
              prim3.range->ReadData(HdTokens->points));

    // test IsAggregatedWith
    TF_VERIFY(prim1.range->IsAggregatedWith(prim1.range));
    TF_VERIFY(prim2.range->IsAggregatedWith(prim2.range));
    TF_VERIFY(prim3.range->IsAggregatedWith(prim3.range));

    TF_VERIFY(!prim1.range->IsAggregatedWith(prim2.range));
    TF_VERIFY(!prim1.range->IsAggregatedWith(prim3.range));
    TF_VERIFY(!prim2.range->IsAggregatedWith(prim1.range));

    if (aggregation) {
        TF_VERIFY(prim2.range->IsAggregatedWith(prim3.range));
        TF_VERIFY(prim3.range->IsAggregatedWith(prim2.range));
    }

    TF_VERIFY(GetGPUMemoryUsed(registry) > 0);

    std::cout << *registry;

    prims.clear();
    registry->GarbageCollect();

    TF_VERIFY(GetGPUMemoryUsed(registry) == 0);
}

static void
UniformAggregationTest(bool aggregation, bool ssbo, 
                       HdStResourceRegistry *registry)
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.ResetCounters();

    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 0);
    TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferCpuToGpu) == 0);
    TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferGpuToGpu) == 0);

    HdBufferSpecVector bufferSpecs;
    bufferSpecs.emplace_back(HdTokens->transform,
                             HdTupleType {HdTypeDoubleMat4, 1});
    bufferSpecs.emplace_back(HdTokens->displayColor,
                             HdTupleType {HdTypeFloatVec3, 1});

    HdBufferArrayRangeSharedPtr range1;
    HdBufferArrayRangeSharedPtr range2;
    if (ssbo) {
        range1 = registry->AllocateShaderStorageBufferArrayRange(
                     HdTokens->primvar,
                     bufferSpecs,
                     HdBufferArrayUsageHintBitsStorage);
        range2 = registry->AllocateShaderStorageBufferArrayRange(
                    HdTokens->primvar,
                    bufferSpecs,
                    HdBufferArrayUsageHintBitsStorage);
    } else {
        range1 = registry->AllocateUniformBufferArrayRange(
                     HdTokens->primvar,
                     bufferSpecs,
                     HdBufferArrayUsageHintBitsUniform);
        range2 = registry->AllocateUniformBufferArrayRange(
                     HdTokens->primvar,
                     bufferSpecs,
                     HdBufferArrayUsageHintBitsUniform);
    }
    // set matrix
    const GfMatrix4d matrix1(10), matrix2(20);

    registry->AddSources(range1,
                         { std::make_shared<HdVtBufferSource>(
                               HdTokens->transform, VtValue(matrix1)) });
    registry->AddSources(range2,
                         { std::make_shared<HdVtBufferSource>(
                               HdTokens->transform, VtValue(matrix2)) });
    registry->Commit();

    TF_VERIFY(matrix1 == range1->ReadData(HdTokens->transform).Get<VtArray<GfMatrix4d> >()[0]);
    TF_VERIFY(matrix2 == range2->ReadData(HdTokens->transform).Get<VtArray<GfMatrix4d> >()[0]);

    if (aggregation) {
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 1);
        TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferCpuToGpu) == 2);
        TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferGpuToGpu) == 0);
    } else {
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 2);
        TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferCpuToGpu) == 2);
        TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferGpuToGpu) == 0);
    }

    // shader storage layout check
    // this struct has to be aligned:
    // transform dmat4 : 128 byte
    // color vec3      : 12 byte
    // total           : 140 byte
    //                 : 160 byte, round up to 32 byte align (due to dmat4)
    //                   or, 256 byte (GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT)

    {
        HdStBufferArrayRangeSharedPtr range1GL = 
            std::static_pointer_cast<HdStBufferArrayRange>(range1);

        if (aggregation) {
            if (ssbo) {
                TF_VERIFY(range1GL->GetResource(HdTokens->transform)->GetOffset() == 0);
                TF_VERIFY(range1GL->GetResource(HdTokens->displayColor)->GetOffset() == 128);
                TF_VERIFY(range1GL->GetResource(HdTokens->transform)->GetStride() == 160);
                TF_VERIFY(range1GL->GetResource(HdTokens->displayColor)->GetStride() == 160);
            } else {
                TF_VERIFY(range1GL->GetResource(HdTokens->transform)->GetOffset() == 0);
                TF_VERIFY(range1GL->GetResource(HdTokens->displayColor)->GetOffset() == 128);
                TF_VERIFY(range1GL->GetResource(HdTokens->transform)->GetStride() == 256);
                TF_VERIFY(range1GL->GetResource(HdTokens->displayColor)->GetStride() == 256);
            }
        } else {
            TF_VERIFY(range1GL->GetResource(HdTokens->transform)->GetOffset() == 0);
            TF_VERIFY(range1GL->GetResource(HdTokens->displayColor)->GetOffset() == 0);
            TF_VERIFY(range1GL->GetResource(HdTokens->transform)->GetStride() == 128);
            TF_VERIFY(range1GL->GetResource(HdTokens->displayColor)->GetStride() == 12);
        }
    }

    TF_VERIFY(GetGPUMemoryUsed(registry) > 0);

    range1.reset();
    range2.reset();
    registry->GarbageCollect();

    TF_VERIFY(GetGPUMemoryUsed(registry) == 0);
}

void
ResizeTest(HdStResourceRegistry *registry)
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.ResetCounters();

    // create a range
    HdBufferArrayRangeSharedPtr range1;
    HdBufferSourceSharedPtrVector sources;
    HdBufferSpecVector bufferSpecs;

    // allocate 100 points
    VtArray<GfVec3f> points(100);
    for (size_t i = 0; i < points.size(); ++i) points[i] = GfVec3f(i);
    sources.push_back(
        std::make_shared<HdVtBufferSource>(HdTokens->points, VtValue(points)));

    bufferSpecs.emplace_back(HdTokens->points,
                             HdTupleType {HdTypeFloatVec3, 1});

    // register, commit
    range1 = registry->AllocateNonUniformBufferArrayRange(
        HdTokens->primvar, bufferSpecs, HdBufferArrayUsageHintBitsVertex);
    registry->AddSources(range1, std::move(sources));
    registry->Commit();
    TF_VERIFY(points == range1->ReadData(HdTokens->points));
    sources.clear();

    // vbo should be relocated once at this point
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 1);

    // resize to 50
    points = VtArray<GfVec3f>(50);
    for (size_t i = 0; i < points.size(); ++i) points[i] = GfVec3f(i);
    sources.push_back(
        std::make_shared<HdVtBufferSource>(HdTokens->points, VtValue(points)));

    // register, commit
    registry->AddSources(range1, std::move(sources));
    registry->Commit();
    TF_VERIFY(points == range1->ReadData(HdTokens->points));
    sources.clear();

    // (XXX: N/A) vbo shouldn't be relocated since then because we just reduced the size
    //
    // XXX: because of bug 114080, we relocate vbo when any BARs have been
    //      shrunk, so that the indirect dispatch buffer will be rebuilt with
    //      the correct number of elements.
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 2);

    // let's resize back to 100
    points = VtArray<GfVec3f>(100);
    for (size_t i = 0; i < points.size(); ++i) points[i] = GfVec3f(i);
    sources.push_back(
        std::make_shared<HdVtBufferSource>(HdTokens->points, VtValue(points)));

    // register, commit
    registry->AddSources(range1, std::move(sources));
    registry->Commit();
    TF_VERIFY(points == range1->ReadData(HdTokens->points));
    sources.clear();

    // vbo still shouldn't be relocated, because we had a margin in the range
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 3);

    // let's resize to 150, which is larger than initial
    points = VtArray<GfVec3f>(150);
    for (size_t i = 0; i < points.size(); ++i) points[i] = GfVec3f(i);
    sources.push_back(
        HdBufferSourceSharedPtr(new HdVtBufferSource(HdTokens->points, VtValue(points))));

    // register, commit
    registry->AddSources(range1, std::move(sources));
    registry->Commit();
    TF_VERIFY(points == range1->ReadData(HdTokens->points));
    sources.clear();

    // vbo has been relocated.
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 4);

    TF_VERIFY(GetGPUMemoryUsed(registry) > 0);

    // expand (should preserve data)
    //
    // some of GPU computations may read existing data and populate new data
    // into same buffers. ex. OpenSubdiv
    // +-----------------+----------------------+
    // | coarse vertices |  refined vertices    |
    // +-----------------+----------------------+
    //  ^filled by HdBufferSource
    //                      ^fill by HdStComputation
    //
    // The size of computation result is given by
    // HdStComputation::GetNumOutputElements.
    // It could depend on other computations, and means it might not yet
    // be determined when updating via HdBufferSource. To avoid complicated
    // dependency, vbo memory managers copy their data when the range is
    // growing as well. It works as follows.
    //
    // 1. HdBufferSource (filled by CPU)
    // +-----------------+
    // | coarse vertices |
    // +-----------------+
    //
    // 2. HdStComputation gives the total number of vertices.
    //    Reallocate vbo and copy coarse vertices into new buffer.
    // +-----------------+----------------------+
    // | coarse vertices |                      |
    // +-----------------+----------------------+
    //
    // 3. HdStComputation fills the result
    // +-----------------+----------------------+
    // | coarse vertices |  refined vertices    |
    // +-----------------+----------------------+
    //
    // 4. next time, HdBufferSource fills coarse vertices again
    // +-----------------+----------------------+
    // | coarse vert(new)|  refined vertices    |
    // +-----------------+----------------------+
    //
    // At this point, the range could be compacted to the size of coarse vert.
    // But actually it doesn't happen until GarbageCollect is called.
    // So the GPU computation is able to fill the refined vertices without
    // having more redundant reallocations, as long as the total size doesn't
    // change.
    //

    HdStComputationSharedPtr computation =
        std::make_shared<_ResizeComputation>(200);
    registry->AddComputation(range1, computation, HdStComputeQueueZero);
    registry->Commit();

    VtValue result = range1->ReadData(HdTokens->points);
    TF_VERIFY(result.IsHolding<VtArray<GfVec3f> >());
    VtArray<GfVec3f> resultArray = result.Get<VtArray<GfVec3f> >();

    TF_VERIFY(resultArray.size() == 200);
    TF_VERIFY(points.size() == 150);
    for (size_t i = 0; i < points.size(); ++i) {
        TF_VERIFY(resultArray[i] == points[i]);
    }

    // shrink

    VtArray<GfVec3f> fewerPoints = VtArray<GfVec3f>(10);
    for (size_t i = 0; i < fewerPoints.size(); ++i) fewerPoints[i] = GfVec3f(i);
    sources.push_back(
        std::make_shared<HdVtBufferSource>(
            HdTokens->points, VtValue(fewerPoints)));

    // register, commit
    registry->AddSources(range1, std::move(sources));
    registry->Commit();
    TF_VERIFY(fewerPoints == range1->ReadData(HdTokens->points));
    sources.clear();

    registry->GarbageCollect();
    TF_VERIFY(fewerPoints == range1->ReadData(HdTokens->points));

    // clear
    range1.reset();
    registry->GarbageCollect();

    TF_VERIFY(GetGPUMemoryUsed(registry) == 0);
}

static void
TopologyTest(HdStResourceRegistry *registry)
{
    HdPerfLog &perfLog = HdPerfLog::GetInstance();
    perfLog.ResetCounters();

    // write
    HdBufferSpecVector bufferSpecs;
    bufferSpecs.emplace_back(HdTokens->indices,
                             HdTupleType { HdTypeInt32, 1 } );
    HdBufferArrayUsageHint usageHint =
        HdBufferArrayUsageHintBitsIndex | HdBufferArrayUsageHintBitsStorage;
    HdBufferArrayRangeSharedPtr range =
        registry->AllocateNonUniformBufferArrayRange(HdTokens->topology,
                                                     bufferSpecs,
                                                     usageHint);
    TF_VERIFY(range);

    // add indices
    VtArray<int> indices(6);
    for (size_t i = 0; i < indices.size(); ++i) {
        indices[i] = i;
    }
    
    registry->AddSources(range,
                         { std::make_shared<HdVtBufferSource>(
                               HdTokens->indices, VtValue(indices)) });
    registry->Commit();

    // read
    TF_VERIFY(indices == range->ReadData(HdTokens->indices));

    {
        // make sure not to raise a coding error, 
        // we have only one resource on topology.
        HdStBufferArrayRangeSharedPtr rangeGL = 
            std::static_pointer_cast<HdStBufferArrayRange>(range);
        rangeGL->GetResource();
    }

    TF_VERIFY(GetGPUMemoryUsed(registry) > 0);

    range.reset();
    registry->GarbageCollect();

    TF_VERIFY(GetGPUMemoryUsed(registry) == 0);
}

static void
InstancingUniformTest(bool ssbo, HdStResourceRegistry *registry)
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.ResetCounters();

    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 0);
    TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferCpuToGpu) == 0);
    TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferGpuToGpu) == 0);

    // Test with 2 prims
    const size_t arraySize = 2;

    HdBufferSpecVector bufferSpecs;
    bufferSpecs.emplace_back(HdTokens->transform,
                             HdTupleType {HdTypeDoubleMat4, arraySize});
    bufferSpecs.emplace_back(HdTokens->displayColor,
                             HdTupleType {HdTypeFloatVec3, arraySize});

    HdBufferArrayRangeSharedPtr range;
    if (ssbo) {
        range = registry->AllocateShaderStorageBufferArrayRange(
                    HdTokens->primvar,
                    bufferSpecs,
                    HdBufferArrayUsageHintBitsStorage);
    } else {
        range = registry->AllocateUniformBufferArrayRange(
                    HdTokens->primvar,
                    bufferSpecs,
                    HdBufferArrayUsageHintBitsUniform);
    }
    // set 2 prims
    VtArray<GfMatrix4d> matrices(arraySize);
    VtArray<GfVec3f> colors(arraySize);
    matrices[0] = GfMatrix4d(1);
    matrices[1] = GfMatrix4d(2);
    colors[0] = GfVec3f(1, 0, 0);
    colors[1] = GfVec3f(0, 1, 0);
    registry->AddSources(range,
                         { std::make_shared<HdVtBufferSource>(
                               HdTokens->transform,
                               VtValue(matrices),
                               arraySize),
                           std::make_shared<HdVtBufferSource>(
                               HdTokens->displayColor,
                               VtValue(colors),
                               arraySize) });
    registry->Commit();

    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 1);
    // (transform*2, color*2) = 2
    TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferCpuToGpu) == 2);
    TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferGpuToGpu) == 0);

    TF_VERIFY(matrices == range->ReadData(HdTokens->transform));
    TF_VERIFY(colors == range->ReadData(HdTokens->displayColor));
}

static void
OverAggregationTest(HdStResourceRegistry *registry)
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.ResetCounters();

    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 0);
    TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferCpuToGpu) == 0);
    TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferGpuToGpu) == 0);

    // layout
    HdBufferSpecVector bufferSpecs;
    bufferSpecs.emplace_back(HdTokens->points,
                             HdTupleType {HdTypeFloatVec3, 1});

    // 10M points (~114MB)
    VtArray<GfVec3f> points(10000000);
    for (size_t i = 0; i < points.size(); ++i) {
        points[i] = GfVec3f(i);
    }

    // * 50
    //   8 entries = 915MB
    //   split into 7 buffers.
    int count = 50;
    std::vector<HdBufferArrayRangeSharedPtr> ranges;
    for (int i = 0; i < count/2; ++i) {
        HdBufferSourceSharedPtrVector sources;
        sources.push_back(
            std::make_shared<HdVtBufferSource>(
                HdTokens->points, VtValue(points)));

        // write
        HdBufferArrayRangeSharedPtr range =
            registry->AllocateNonUniformBufferArrayRange(
                HdTokens->primvar, bufferSpecs,
                HdBufferArrayUsageHintBitsVertex);
        TF_VERIFY(range);

        ranges.push_back(range);

        registry->AddSources(range, std::move(sources));
    }

    registry->Commit();

    // Schedule some more resources which will aggregate with the 
    // previously committed resources.
    for (int i = count/2; i < count; ++i) {
        HdBufferSourceSharedPtrVector sources;
        sources.push_back(
            std::make_shared<HdVtBufferSource>(
                HdTokens->points, VtValue(points)));

        // write
        HdBufferArrayRangeSharedPtr range =
            registry->AllocateNonUniformBufferArrayRange(
                HdTokens->primvar, bufferSpecs,
                HdBufferArrayUsageHintBitsVertex);
        TF_VERIFY(range);

        ranges.push_back(range);

        registry->AddSources(range, std::move(sources));
    }

    registry->Commit();

    // read
    for (int i = 0; i < count; ++i) {
        VtValue const& rangeData = ranges[i]->ReadData(HdTokens->points);
        if (points != rangeData) {
            // XXX The below code is added for debugging why this test
            // sometimes fails. We suspect a floating-point compare issue where
            // we may need to have a small epsilon for comparing floats?
            TF_VERIFY(rangeData.IsHolding<VtArray<GfVec3f>>());
            VtArray<GfVec3f> const& vec3fArray = 
                rangeData.UncheckedGet<VtArray<GfVec3f>>();

            std::cerr << "point size: " << points.size() << std::endl;
            std::cerr << "rangeData size: " << vec3fArray.size() << std::endl;

            for (size_t x=0; x<points.size(); x++) {
                if (points[x] != vec3fArray[x]) {
                    std::cerr << "Compare failed index: " << x << std::endl;
                    std::cerr << points[x] << " " << vec3fArray[x] << std::endl;
                }
            }

            TF_VERIFY(false);
        }
    }

    std::cerr << perfLog.GetCounter(HdPerfTokens->vboRelocated) << "\n";
    std::cerr << perfLog.GetCounter(HdStPerfTokens->copyBufferCpuToGpu) << "\n";
    std::cerr << perfLog.GetCounter(HdStPerfTokens->copyBufferGpuToGpu) << "\n";

    // check perf counters
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 9);
    TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferCpuToGpu) == 50);
    TF_VERIFY(perfLog.GetCounter(HdStPerfTokens->copyBufferGpuToGpu) == 1);

    ranges.clear();
    registry->GarbageCollect();
}


static void
HintAggregationTest(HdStResourceRegistry *registry)
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.ResetCounters();

    size_t primCount = 10;
    std::vector<Prim> prims;

    for (size_t i = 0; i < primCount; ++i) {
        prims.push_back(_CreatePrim((i+1)*10));
    }

    // write
    for (size_t i = 0; i < prims.size(); ++i) {
        Prim &prim = prims[i];
        if (!prim.sources.empty()) {

            // Prims 3, 6 and 9 are size varying
            // Prim 5 is immutable
            // Prim 0 is size varying and immutable
            // Prims 1, 2, 4, 7 and 8 have no hint.

            HdBufferArrayUsageHint usageHint = HdBufferArrayUsageHintBitsVertex;
            if ((i % 3) == 0) {
                usageHint |= HdBufferArrayUsageHintBitsSizeVarying;
            }
            if ((i % 5) == 0) {
                usageHint |= HdBufferArrayUsageHintBitsImmutable;
            }
            prim.range = registry->AllocateNonUniformBufferArrayRange(
                HdTokens->primvar, prim.bufferSpecs, usageHint);
            registry->AddSources(prim.range, std::move(prim.sources));
        }
        prim.sources.clear();
    }
    registry->Commit();

    // check perf counters
    // There should be 4 buffers as there are 4 hint classes
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 4);

    perfLog.ResetCounters();

    std::cout << *registry;

    prims.clear();
    registry->GarbageCollect();

    TF_VERIFY(GetGPUMemoryUsed(registry) == 0);
}


int main(int argc, char *argv[])
{
    TfErrorMark mark;

    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();

    // prepare GL context
    GarchGLDebugWindow window("Hd Test", 512, 512);
    window.Init();

    // Initialize the resource registry we will test

    std::unique_ptr<Hgi> hgi = Hgi::CreatePlatformDefaultHgi();

    const int uniformBufferOffsetAlignment = 
        hgi->GetCapabilities()->GetUniformBufferOffsetAlignment();

    // Test verification relies on known GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT
    TF_VERIFY(uniformBufferOffsetAlignment == 256);

    HdStResourceRegistry resourceRegistry(hgi.get());

    std::cout << "*Basic Test" << std::endl;
    BasicTest(&resourceRegistry);

    std::cout << "*Aggregation Test" << std::endl;
    AggregationTest(true, &resourceRegistry);

    std::cout << "*Resize Test" << std::endl;
    ResizeTest(&resourceRegistry);

    std::cout << "*Shader Storage Basic Test" << std::endl;
    UniformBasicTest(/*ssbo=*/true, &resourceRegistry);

    std::cout << "*Shader Storage Aggregation Test" << std::endl;
    UniformAggregationTest(/*aggregation=*/true, /*ssbo=*/true, &resourceRegistry);

    std::cout << "*Uniform Basic Test" << std::endl;
    UniformBasicTest(/*ssbo=*/false, &resourceRegistry);

    std::cout << "*Unifrom Aggregation Test" << std::endl;
    UniformAggregationTest(/*aggregation=*/true, /*ssbo=*/false, &resourceRegistry);

    std::cout << "*Topology Test" << std::endl;
    TopologyTest(&resourceRegistry);

    std::cout << "*Instancing Uniform Test" << std::endl;
    InstancingUniformTest(/*ssbo=*/true, &resourceRegistry);

    std::cout << "*Instancing Uniform Test" << std::endl;
    InstancingUniformTest(/*ssbo=*/false, &resourceRegistry);

    std::cout << "*Over aggregation test" << std::endl;
    OverAggregationTest(&resourceRegistry);

    std::cout << "Hint aggregation test" << std::endl;
    HintAggregationTest(&resourceRegistry);


    // switch to simple memory manager
    resourceRegistry.SetNonUniformAggregationStrategy(
        std::make_unique<HdStVBOSimpleMemoryManager>(&resourceRegistry));
    resourceRegistry.SetNonUniformImmutableAggregationStrategy(
        std::make_unique<HdStVBOSimpleMemoryManager>(&resourceRegistry));
    resourceRegistry.SetUniformAggregationStrategy(
        std::make_unique<HdStVBOSimpleMemoryManager>(&resourceRegistry));
    resourceRegistry.SetShaderStorageAggregationStrategy(
        std::make_unique<HdStVBOSimpleMemoryManager>(&resourceRegistry));

    std::cout << "*Basic Test (simple)" << std::endl;
    BasicTest(&resourceRegistry);

    std::cout << "*Aggregation Test (simple)" << std::endl;
    AggregationTest(false, &resourceRegistry);

    std::cout << "*Resize Test" << std::endl;
    ResizeTest(&resourceRegistry);

    std::cout << "*Shader Storage Basic Test (simple)" << std::endl;
    UniformBasicTest(/*ssbo=*/true, &resourceRegistry);

    std::cout << "*Shader Storage Aggregation Test (simple)" << std::endl;
    UniformAggregationTest(/*aggregation=*/false, /*ssbo=*/true, &resourceRegistry);

    std::cout << "*Uniform Basic Test (simple)" << std::endl;
    UniformBasicTest(/*ssbo=*/false, &resourceRegistry);

    std::cout << "*Uniform Aggregation Test (simple)" << std::endl;
    UniformAggregationTest(/*aggregation=*/false, /*ssbo=*/false, &resourceRegistry);

    std::cout << "*Topology Test (simple)" << std::endl;
    TopologyTest(&resourceRegistry);

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

