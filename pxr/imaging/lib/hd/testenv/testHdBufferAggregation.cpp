#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/computation.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderContextCaps.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/vboMemoryManager.h"
#include "pxr/imaging/hd/vboSimpleMemoryManager.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/testGLContext.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/stl.h"

#include <QApplication>
#include <iostream>
#include <sstream>

static std::ostringstream testLog;

class _ResizeComputation : public HdComputation {
public:
    _ResizeComputation(int numElements) : _numElements(numElements) { }
    virtual void Execute(HdBufferArrayRangeSharedPtr const &range) { }
    virtual void AddBufferSpecs(HdBufferSpecVector *specs) const { }
    virtual int GetNumOutputElements() const { return _numElements; }
private:
    int _numElements;
};

static size_t
GetGPUMemoryUsed()
{
    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();
    VtDictionary allocation = registry->GetResourceAllocation();

    VtValue memUsed;
    TF_VERIFY(TfMapLookup(allocation, HdPerfTokens->gpuMemoryUsed, &memUsed));
    TF_VERIFY(memUsed.IsHolding<size_t>());

    return memUsed.Get<size_t>();
}

static void
BasicTest()
{
    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.ResetCounters();

    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glBufferSubData) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glCopyBufferSubData) == 0);

    HdBufferSourceVector sources;

    // add points
    VtArray<GfVec3f> points(3);
    points[0] = GfVec3f(0);
    points[1] = GfVec3f(1);
    points[2] = GfVec3f(2);
    HdBufferSourceSharedPtr pointSource(
        new HdVtBufferSource(HdTokens->points, VtValue(points)));
    sources.push_back(pointSource);

    // add colors
    VtArray<GfVec4f> colors(3);
    colors[0] = GfVec4f(1, 1, 1, 1);
    colors[1] = GfVec4f(1, 0, 1, 1);
    colors[2] = GfVec4f(1, 1, 0, 1);
    HdBufferSourceSharedPtr colorSource(
        new HdVtBufferSource(HdTokens->color, VtValue(colors)));
    sources.push_back(colorSource);

    // layout
    HdBufferSpecVector bufferSpecs;
    bufferSpecs.push_back(HdBufferSpec(HdTokens->points, GL_FLOAT, 3));
    bufferSpecs.push_back(HdBufferSpec(HdTokens->color, GL_FLOAT, 4));

    // write
    HdBufferArrayRangeSharedPtr range =
        registry->AllocateNonUniformBufferArrayRange(HdTokens->primVar, bufferSpecs);
    registry->AddSources(range, sources);
    registry->Commit();
    TF_VERIFY(range);
    sources.clear();

    // read
    TF_VERIFY(points == range->ReadData(HdTokens->points));
    TF_VERIFY(colors == range->ReadData(HdTokens->color));

    // check perf counters
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 1);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glBufferSubData) == 2);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glCopyBufferSubData) == 0);

    // update points
    points[0] = GfVec3f(10);
    points[1] = GfVec3f(20);
    points[2] = GfVec3f(30);
    pointSource.reset(new HdVtBufferSource(HdTokens->points, VtValue(points)));
    sources.push_back(pointSource);

    // write
    registry->AddSources(range, sources);
    registry->Commit();
    TF_VERIFY(range);
    sources.clear();

    // read
    TF_VERIFY(points == range->ReadData(HdTokens->points));
    TF_VERIFY(colors == range->ReadData(HdTokens->color));

    // check perf counters
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 1);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glBufferSubData) == 3);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glCopyBufferSubData) == 0);

    TF_VERIFY(GetGPUMemoryUsed() > 0);

    std::cout << *registry;

    range.reset();
    registry->GarbageCollect();

    TF_VERIFY(GetGPUMemoryUsed() == 0);
}

static void
UniformBasicTest(bool ssbo)
{
    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.ResetCounters();

    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glBufferSubData) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glCopyBufferSubData) == 0);

    HdBufferSpecVector bufferSpecs;
    bufferSpecs.push_back(HdBufferSpec(HdTokens->transform, GL_DOUBLE, 16));
    bufferSpecs.push_back(HdBufferSpec(HdTokens->color, GL_FLOAT, 4));

    HdBufferArrayRangeSharedPtr range =
        ssbo ? registry->AllocateShaderStorageBufferArrayRange(HdTokens->primVar, bufferSpecs)
        : registry->AllocateUniformBufferArrayRange(HdTokens->primVar, bufferSpecs);
    HdBufferSourceVector sources;

    // set matrix
    GfMatrix4d matrix(1);
    sources.push_back(
        HdBufferSourceSharedPtr(new HdVtBufferSource(HdTokens->transform, VtValue(matrix))));
    registry->AddSources(range, sources);
    registry->Commit();
    sources.clear();

    TF_VERIFY(matrix == range->ReadData(HdTokens->transform).Get<VtArray<GfMatrix4d> >()[0]);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 1);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glBufferSubData) == 1);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glCopyBufferSubData) == 0);

    // update matrix
    matrix = GfMatrix4d(2);

    sources.push_back(
        HdBufferSourceSharedPtr(new HdVtBufferSource(HdTokens->transform, VtValue(matrix))));
    registry->AddSources(range, sources);
    registry->Commit();
    sources.clear();

    TF_VERIFY(matrix == range->ReadData(HdTokens->transform).Get<VtArray<GfMatrix4d> >()[0]);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 1);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glBufferSubData) == 2);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glCopyBufferSubData) == 0);

    TF_VERIFY(GetGPUMemoryUsed() > 0);

    range.reset();
    registry->GarbageCollect();

    TF_VERIFY(GetGPUMemoryUsed() == 0);
}


struct Prim {
    HdBufferArrayRangeSharedPtr range;
    HdBufferSourceVector sources;
    HdBufferSpecVector bufferSpecs;
    std::map<TfToken, VtValue> primVars;
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
    HdBufferSourceSharedPtr pointSource(
        new HdVtBufferSource(HdTokens->points, VtValue(points)));
    prim.sources.push_back(pointSource);
    prim.primVars[HdTokens->points] = points;
    prim.bufferSpecs.push_back(HdBufferSpec(HdTokens->points, GL_FLOAT, 3));

    // add colors
    if (colors) {
        VtArray<GfVec4f> colors(numElements);
        for (int i = 0; i < numElements; ++i) {
            colors[i] = GfVec4f(i, i, i, 1);
        }
        HdBufferSourceSharedPtr colorSource(
            new HdVtBufferSource(HdTokens->color, VtValue(colors)));
        prim.sources.push_back(colorSource);
        prim.primVars[HdTokens->color] = colors;
        prim.bufferSpecs.push_back(HdBufferSpec(HdTokens->color, GL_FLOAT, 4));
    }

    return prim;
}

static void
AggregationTest(bool aggregation)
{
    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.ResetCounters();

    int primCount = 10;
    std::vector<Prim> prims;

    for (int i = 0; i < primCount; ++i) {
        prims.push_back(_CreatePrim((i+1)*10));
    }

    // write
    TF_FOR_ALL (it, prims) {
        if (not it->sources.empty()) {
            it->range = registry->AllocateNonUniformBufferArrayRange(
                HdTokens->primVar, it->bufferSpecs);
            registry->AddSources(it->range, it->sources);
        }
        it->sources.clear();
    }
    registry->Commit();

    // read
    TF_FOR_ALL (it, prims) {
        TF_VERIFY(it->primVars[HdTokens->points] ==
                  it->range->ReadData(HdTokens->points));
        TF_VERIFY(it->primVars[HdTokens->color] ==
                  it->range->ReadData(HdTokens->color));
    }

    // check perf counters
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) ==
              (aggregation ? 1 : primCount));
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glBufferSubData) ==
              2*primCount);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glCopyBufferSubData) == 0);

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
        TF_VERIFY(it->primVars[HdTokens->points] ==
                  it->range->ReadData(HdTokens->points));
        TF_VERIFY(it->primVars[HdTokens->color] ==
                  it->range->ReadData(HdTokens->color));
    }

    // allocate new prims
    prims.push_back(_CreatePrim(80));
    prims.push_back(_CreatePrim(90));
    primCount = (int)prims.size();

    // write inefficiently
    TF_FOR_ALL (it, prims) {
        if (not it->sources.empty()) {
            if (not it->range) {
                it->range = registry->AllocateNonUniformBufferArrayRange(
                    HdTokens->primVar, it->bufferSpecs);
            }
            registry->AddSources(it->range, it->sources);
        }
        it->sources.clear();

        // intentionally commit within a loop
        registry->Commit();
    }

    std::cout << *registry;
    // read
    TF_FOR_ALL (it, prims) {
        TF_VERIFY(it->primVars[HdTokens->points] ==
                  it->range->ReadData(HdTokens->points));
        TF_VERIFY(it->primVars[HdTokens->color] ==
                  it->range->ReadData(HdTokens->color));
    }

    // check perf counters
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 2);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glBufferSubData) == 2*2);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glCopyBufferSubData) ==
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
        if (not it->sources.empty()) {
            if (not it->range) {
                it->range = registry->AllocateNonUniformBufferArrayRange(
                    HdTokens->primVar, it->bufferSpecs);
            }
            registry->AddSources(it->range, it->sources);
        }
        TF_VERIFY(it->range);
        it->sources.clear();
    }
    registry->Commit();

    // read
    TF_VERIFY(prim1.primVars[HdTokens->points] ==
              prim1.range->ReadData(HdTokens->points));
    TF_VERIFY(prim1.primVars[HdTokens->color] ==
              prim1.range->ReadData(HdTokens->color));
    TF_VERIFY(prim2.primVars[HdTokens->points] ==
              prim2.range->ReadData(HdTokens->points));
    TF_VERIFY(prim3.primVars[HdTokens->points] ==
              prim3.range->ReadData(HdTokens->points));

    // test IsAggregatedWith
    TF_VERIFY(prim1.range->IsAggregatedWith(prim1.range));
    TF_VERIFY(prim2.range->IsAggregatedWith(prim2.range));
    TF_VERIFY(prim3.range->IsAggregatedWith(prim3.range));

    TF_VERIFY(not prim1.range->IsAggregatedWith(prim2.range));
    TF_VERIFY(not prim1.range->IsAggregatedWith(prim3.range));
    TF_VERIFY(not prim2.range->IsAggregatedWith(prim1.range));

    if (aggregation) {
        TF_VERIFY(prim2.range->IsAggregatedWith(prim3.range));
        TF_VERIFY(prim3.range->IsAggregatedWith(prim2.range));
    }

    TF_VERIFY(GetGPUMemoryUsed() > 0);

    std::cout << *registry;

    prims.clear();
    registry->GarbageCollect();

    TF_VERIFY(GetGPUMemoryUsed() == 0);
}

static void
UniformAggregationTest(bool aggregation, bool ssbo)
{
    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.ResetCounters();

    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glBufferSubData) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glCopyBufferSubData) == 0);

    HdBufferSpecVector bufferSpecs;
    bufferSpecs.push_back(HdBufferSpec(HdTokens->transform, GL_DOUBLE, 16));
    bufferSpecs.push_back(HdBufferSpec(HdTokens->color, GL_FLOAT, 4));

    HdBufferArrayRangeSharedPtr range1 = ssbo ?
        registry->AllocateShaderStorageBufferArrayRange(HdTokens->primVar, bufferSpecs)
        : registry->AllocateUniformBufferArrayRange(HdTokens->primVar, bufferSpecs);
    HdBufferArrayRangeSharedPtr range2 = ssbo ?
        registry->AllocateShaderStorageBufferArrayRange(HdTokens->primVar, bufferSpecs)
        : registry->AllocateUniformBufferArrayRange(HdTokens->primVar, bufferSpecs);
    HdBufferSourceVector sources1, sources2;

    // set matrix
    GfMatrix4d matrix1(10), matrix2(20);
    sources1.push_back(
        HdBufferSourceSharedPtr(new HdVtBufferSource(HdTokens->transform, VtValue(matrix1))));
    sources2.push_back(
        HdBufferSourceSharedPtr(new HdVtBufferSource(HdTokens->transform, VtValue(matrix2))));

    registry->AddSources(range1, sources1);
    registry->AddSources(range2, sources2);
    registry->Commit();

    TF_VERIFY(matrix1 == range1->ReadData(HdTokens->transform).Get<VtArray<GfMatrix4d> >()[0]);
    TF_VERIFY(matrix2 == range2->ReadData(HdTokens->transform).Get<VtArray<GfMatrix4d> >()[0]);

    if (aggregation) {
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 1);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glBufferSubData) == 2);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glCopyBufferSubData) == 0);
    } else {
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 2);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glBufferSubData) == 2);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glCopyBufferSubData) == 0);
    }

    // shader storage layout check
    // this struct has to be aligned:
    // transform dmat4 : 128 byte
    // color vec4      : 16 byte
    // total           : 144 byte
    //                 : 160 byte, round up to 32 byte align (due to dmat4)
    //                   or, 256 byte (GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT)

    if (aggregation) {
        if (ssbo) {
            TF_VERIFY(range1->GetResource(HdTokens->transform)->GetOffset() == 0);
            TF_VERIFY(range1->GetResource(HdTokens->color)->GetOffset() == 128);
            TF_VERIFY(range1->GetResource(HdTokens->transform)->GetStride() == 160);
            TF_VERIFY(range1->GetResource(HdTokens->color)->GetStride() == 160);
        } else {
            TF_VERIFY(range1->GetResource(HdTokens->transform)->GetOffset() == 0);
            TF_VERIFY(range1->GetResource(HdTokens->color)->GetOffset() == 128);
            TF_VERIFY(range1->GetResource(HdTokens->transform)->GetStride() == 256);
            TF_VERIFY(range1->GetResource(HdTokens->color)->GetStride() == 256);
        }
    } else {
        TF_VERIFY(range1->GetResource(HdTokens->transform)->GetOffset() == 0);
        TF_VERIFY(range1->GetResource(HdTokens->color)->GetOffset() == 0);
        TF_VERIFY(range1->GetResource(HdTokens->transform)->GetStride() == 128);
        TF_VERIFY(range1->GetResource(HdTokens->color)->GetStride() == 16);
    }

    TF_VERIFY(GetGPUMemoryUsed() > 0);

    range1.reset();
    range2.reset();
    registry->GarbageCollect();

    TF_VERIFY(GetGPUMemoryUsed() == 0);
}

void
ResizeTest()
{
    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.ResetCounters();

    // create a range
    HdBufferArrayRangeSharedPtr range1;
    HdBufferSourceVector sources;
    HdBufferSpecVector bufferSpecs;

    // allocate 100 points
    VtArray<GfVec3f> points(100);
    for (int i = 0; i < points.size(); ++i) points[i] = GfVec3f(i);
    sources.push_back(
        HdBufferSourceSharedPtr(new HdVtBufferSource(HdTokens->points, VtValue(points))));

    bufferSpecs.push_back(HdBufferSpec(HdTokens->points, GL_FLOAT, 3));

    // register, commit
    range1 = registry->AllocateNonUniformBufferArrayRange(
        HdTokens->primVar, bufferSpecs);
    registry->AddSources(range1, sources);
    registry->Commit();
    TF_VERIFY(points == range1->ReadData(HdTokens->points));
    sources.clear();

    // vbo should be relocated once at this point
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 1);

    // resize to 50
    points = VtArray<GfVec3f>(50);
    for (int i = 0; i < points.size(); ++i) points[i] = GfVec3f(i);
    sources.push_back(
        HdBufferSourceSharedPtr(new HdVtBufferSource(HdTokens->points, VtValue(points))));

    // register, commit
    registry->AddSources(range1, sources);
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
    for (int i = 0; i < points.size(); ++i) points[i] = GfVec3f(i);
    sources.push_back(
        HdBufferSourceSharedPtr(new HdVtBufferSource(HdTokens->points, VtValue(points))));

    // register, commit
    registry->AddSources(range1, sources);
    registry->Commit();
    TF_VERIFY(points == range1->ReadData(HdTokens->points));
    sources.clear();

    // vbo still shouldn't be relocated, because we had a margin in the range
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 3);

    // let's resize to 150, which is larger than initial
    points = VtArray<GfVec3f>(150);
    for (int i = 0; i < points.size(); ++i) points[i] = GfVec3f(i);
    sources.push_back(
        HdBufferSourceSharedPtr(new HdVtBufferSource(HdTokens->points, VtValue(points))));

    // register, commit
    registry->AddSources(range1, sources);
    registry->Commit();
    TF_VERIFY(points == range1->ReadData(HdTokens->points));
    sources.clear();

    // vbo has been relocated.
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 4);

    TF_VERIFY(GetGPUMemoryUsed() > 0);

    // expand (should preserve data)
    //
    // some of GPU computations may read existing data and populate new data
    // into same buffers. ex. OpenSubdiv
    // +-----------------+----------------------+
    // | coarse vertices |  refined vertices    |
    // +-----------------+----------------------+
    //  ^filled by HdBufferSource
    //                      ^fill by HdComputation
    //
    // The size of computation result is given by HdComputation::GetNumOutputElements.
    // It could depend on other computations, and means it might not yet be determined
    // when updating via HdBufferSource. To avoid complicated dependency, vbo memory
    // managers copy their data when the range is growing as well. It works as follows.
    //
    // 1. HdBufferSource (filled by CPU)
    // +-----------------+
    // | coarse vertices |
    // +-----------------+
    //
    // 2. HdComputation gives the total number of vertices.
    //    Reallocate vbo and copy coarse vertices into new buffer.
    // +-----------------+----------------------+
    // | coarse vertices |                      |
    // +-----------------+----------------------+
    //
    // 3. HdComputation fills the result
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
    // having more redundant reallocations, as long as the total size doesn't change.
    //

    HdComputationSharedPtr computation(new _ResizeComputation(200));
    registry->AddComputation(range1, computation);
    registry->Commit();

    VtValue result = range1->ReadData(HdTokens->points);
    TF_VERIFY(result.IsHolding<VtArray<GfVec3f> >());
    VtArray<GfVec3f> resultArray = result.Get<VtArray<GfVec3f> >();

    TF_VERIFY(resultArray.size() == 200);
    TF_VERIFY(points.size() == 150);
    for (int i = 0; i < points.size(); ++i) {
        TF_VERIFY(resultArray[i] == points[i]);
    }

    // shrink

    VtArray<GfVec3f> fewerPoints = VtArray<GfVec3f>(10);
    for (int i = 0; i < points.size(); ++i) points[i] = GfVec3f(i);
    sources.push_back(
        HdBufferSourceSharedPtr(new HdVtBufferSource(
                                    HdTokens->points, VtValue(fewerPoints))));

    // register, commit
    registry->AddSources(range1, sources);
    registry->Commit();
    TF_VERIFY(fewerPoints == range1->ReadData(HdTokens->points));
    sources.clear();

    registry->GarbageCollect();
    TF_VERIFY(fewerPoints == range1->ReadData(HdTokens->points));

    // clear
    range1.reset();
    registry->GarbageCollect();

    TF_VERIFY(GetGPUMemoryUsed() == 0);
}

static void
TopologyTest()
{
    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();
    HdPerfLog &perfLog = HdPerfLog::GetInstance();
    perfLog.ResetCounters();

    HdBufferSourceVector sources;

    // add indices
    VtArray<int> indices(6);
    for (int i = 0; i < indices.size(); ++i) {
        indices[i] = i;
    }
    HdBufferSourceSharedPtr indexSource(
        new HdVtBufferSource(HdTokens->indices, VtValue(indices)));
    sources.push_back(indexSource);

    // write
    HdBufferSpecVector bufferSpecs;
    bufferSpecs.push_back(HdBufferSpec(HdTokens->indices, GL_INT, 1));
    HdBufferArrayRangeSharedPtr range =
        registry->AllocateNonUniformBufferArrayRange(HdTokens->topology, bufferSpecs);
    TF_VERIFY(range);

    registry->AddSources(range, sources);
    registry->Commit();

    // read
    TF_VERIFY(indices == range->ReadData(HdTokens->indices));

    // make sure not to raise a coding error, we have only one resource on topology.
    range->GetResource();

    TF_VERIFY(GetGPUMemoryUsed() > 0);

    range.reset();
    registry->GarbageCollect();

    TF_VERIFY(GetGPUMemoryUsed() == 0);
}

static void
InstancingUniformTest(bool ssbo)
{
    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.ResetCounters();

    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glBufferSubData) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glCopyBufferSubData) == 0);

    HdBufferSpecVector bufferSpecs;
    bufferSpecs.push_back(HdBufferSpec(HdTokens->transform, GL_DOUBLE, 16, /*arraySize=*/2));
    bufferSpecs.push_back(HdBufferSpec(HdTokens->color, GL_FLOAT, 4, /*arraySize=*/2));

    HdBufferArrayRangeSharedPtr range =
        ssbo ? registry->AllocateShaderStorageBufferArrayRange(HdTokens->primVar, bufferSpecs)
        : registry->AllocateUniformBufferArrayRange(HdTokens->primVar, bufferSpecs);
    HdBufferSourceVector sources;

    // set 2 prims
    VtArray<GfMatrix4d> matrices(2);
    VtArray<GfVec4f> colors(2);
    {
        matrices[0] = GfMatrix4d(1);
        matrices[1] = GfMatrix4d(2);
        colors[0] = GfVec4f(1, 0, 0, 1);
        colors[1] = GfVec4f(0, 1, 0, 1);
    }
    sources.push_back(HdBufferSourceSharedPtr(
                          new HdVtBufferSource(HdTokens->transform,
                                               VtValue(matrices))));
    sources.push_back(HdBufferSourceSharedPtr(
                          new HdVtBufferSource(HdTokens->color,
                                               VtValue(colors))));
    registry->AddSources(range, sources);
    registry->Commit();
    sources.clear();

    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 1);
    // (transform*2, color*2) = 2
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glBufferSubData) == 2);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glCopyBufferSubData) == 0);

    TF_VERIFY(matrices == range->ReadData(HdTokens->transform));
    TF_VERIFY(colors == range->ReadData(HdTokens->color));
}

static void
OverAggregationTest()
{
    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.ResetCounters();

    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glBufferSubData) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glCopyBufferSubData) == 0);

    // layout
    HdBufferSpecVector bufferSpecs;
    bufferSpecs.push_back(HdBufferSpec(HdTokens->points, GL_FLOAT, 3));

    // 10M points (~114MB)
    VtArray<GfVec3f> points(10000000);
    for (int i = 0; i < points.size(); ++i) {
        points[i] = GfVec3f(i);
    }

    // * 50
    //   8 entries = 915MB
    //   split into 7 buffers.
    int count = 50;
    std::vector<HdBufferArrayRangeSharedPtr> ranges;
    for (int i = 0; i < count/2; ++i) {
        HdBufferSourceVector sources;
        HdBufferSourceSharedPtr pointSource(
            new HdVtBufferSource(HdTokens->points, VtValue(points)));
        sources.push_back(pointSource);

        // write
        HdBufferArrayRangeSharedPtr range =
            registry->AllocateNonUniformBufferArrayRange(
                HdTokens->primVar, bufferSpecs);
        TF_VERIFY(range);

        ranges.push_back(range);

        registry->AddSources(range, sources);
    }

    registry->Commit();

    // Schedule some more resources which will aggregate with the 
    // previously committed resources.
    for (int i = count/2; i < count; ++i) {
        HdBufferSourceVector sources;
        HdBufferSourceSharedPtr pointSource(
            new HdVtBufferSource(HdTokens->points, VtValue(points)));
        sources.push_back(pointSource);

        // write
        HdBufferArrayRangeSharedPtr range =
            registry->AllocateNonUniformBufferArrayRange(
                HdTokens->primVar, bufferSpecs);
        TF_VERIFY(range);

        ranges.push_back(range);

        registry->AddSources(range, sources);
    }

    registry->Commit();

    // read
    for (int i = 0; i < count; ++i) {
        TF_VERIFY(points == ranges[i]->ReadData(HdTokens->points));
    }

    GLF_POST_PENDING_GL_ERRORS();

    std::cerr << perfLog.GetCounter(HdPerfTokens->vboRelocated) << "\n";
    std::cerr << perfLog.GetCounter(HdPerfTokens->glBufferSubData) << "\n";
    std::cerr << perfLog.GetCounter(HdPerfTokens->glCopyBufferSubData) << "\n";

    // check perf counters
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->vboRelocated) == 9);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glBufferSubData) == 50);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->glCopyBufferSubData) == 1);

    ranges.clear();
    registry->GarbageCollect();
}

int main(int argc, char *argv[])
{
    TfErrorMark mark;

    QApplication app(argc, argv);
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();

    // prepare GL context
    GlfTestGLContext::RegisterGLContextCallbacks();
    GlfGlewInit();
    GlfSharedGLContextScopeHolder sharedContext;

    // Test varification relies on known GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT
    TF_VERIFY(HdRenderContextCaps::GetInstance().uniformBufferOffsetAlignment == 256);

    std::cout << "*Basic Test" << std::endl;
    BasicTest();

    std::cout << "*Aggregation Test" << std::endl;
    AggregationTest(true);

    std::cout << "*Resize Test" << std::endl;
    ResizeTest();

    std::cout << "*Shader Storage Basic Test" << std::endl;
    UniformBasicTest(/*ssbo=*/true);

    std::cout << "*Shader Storage Aggregation Test" << std::endl;
    UniformAggregationTest(/*aggregation=*/true, /*ssbo=*/true);

    std::cout << "*Uniform Basic Test" << std::endl;
    UniformBasicTest(/*ssbo=*/false);

    std::cout << "*Unifrom Aggregation Test" << std::endl;
    UniformAggregationTest(/*aggregation=*/true, /*ssbo=*/false);

    std::cout << "*Topology Test" << std::endl;
    TopologyTest();

    std::cout << "*Instancing Uniform Test" << std::endl;
    InstancingUniformTest(/*ssbo=*/true);

    std::cout << "*Instancing Uniform Test" << std::endl;
    InstancingUniformTest(/*ssbo=*/false);

    std::cout << "*Over aggregation test" << std::endl;
    OverAggregationTest();

    // switch to simple memory manager
    HdResourceRegistry::GetInstance().SetNonUniformAggregationStrategy(
        &HdVBOSimpleMemoryManager::GetInstance());
    HdResourceRegistry::GetInstance().SetUniformAggregationStrategy(
        &HdVBOSimpleMemoryManager::GetInstance());
    HdResourceRegistry::GetInstance().SetShaderStorageAggregationStrategy(
        &HdVBOSimpleMemoryManager::GetInstance());

    std::cout << "*Basic Test (simple)" << std::endl;
    BasicTest();

    std::cout << "*Aggregation Test (simple)" << std::endl;
    AggregationTest(false);

    std::cout << "*Resize Test" << std::endl;
    ResizeTest();

    std::cout << "*Shader Storage Basic Test (simple)" << std::endl;
    UniformBasicTest(/*ssbo=*/true);

    std::cout << "*Shader Storage Aggregation Test (simple)" << std::endl;
    UniformAggregationTest(/*aggregation=*/false, /*ssbo=*/true);

    std::cout << "*Uniform Basic Test (simple)" << std::endl;
    UniformBasicTest(/*ssbo=*/false);

    std::cout << "*Uniform Aggregation Test (simple)" << std::endl;
    UniformAggregationTest(/*aggregation=*/false, /*ssbo=*/false);

    std::cout << "*Topology Test (simple)" << std::endl;
    TopologyTest();

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

