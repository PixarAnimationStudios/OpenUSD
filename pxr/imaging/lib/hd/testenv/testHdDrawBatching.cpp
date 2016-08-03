#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/immediateDrawBatch.h"
#include "pxr/imaging/hd/defaultLightingShader.h"
#include "pxr/imaging/hd/drawItemInstance.h"
#include "pxr/imaging/hd/geometricShader.h"
#include "pxr/imaging/hd/glslfxShader.h"
#include "pxr/imaging/hd/indirectDrawBatch.h"
#include "pxr/imaging/hd/meshShaderKey.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/package.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/pointsShaderKey.h"
#include "pxr/imaging/hd/renderPassShader.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/rprimSharedData.h"
#include "pxr/imaging/hd/shaderKey.h"
#include "pxr/imaging/hd/surfaceShader.h"
#include "pxr/imaging/hd/unitTestHelper.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/glslfx.h"
#include "pxr/imaging/glf/testGLContext.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/gf/vec3f.h"

#include <iostream>

static HdSurfaceShaderSharedPtr _GetFallbackShader()
{
    static HdSurfaceShaderSharedPtr _surfaceFallback;

    if (not _surfaceFallback) {
        GlfGLSLFXSharedPtr glslfx = GlfGLSLFXSharedPtr(new GlfGLSLFX(
                    HdPackageFallbackSurfaceShader()));
        _surfaceFallback = HdSurfaceShaderSharedPtr(new HdGLSLFXShader(glslfx));
        _surfaceFallback->Sync();
    }
    return _surfaceFallback;
}


template <typename T>
static VtValue
_BuildArrayValue(T values[], int numValues)
{
    VtArray<T> result(numValues);
    std::copy(values, values+numValues, result.begin());
    return VtValue(result);
}

HdDrawItem
_RegisterDrawItem(
    GLenum primitiveMode,
    HdRprimSharedData *sharedData,
    VtValue const & indicesValue,
    VtValue const & pointsValue,
    VtValue const & normalsValue = VtValue(),
    VtValue const & colorsValue = VtValue())
{
    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();

    HdBufferSourceVector sources;
    HdBufferSpecVector bufferSpecs;

    sharedData->surfaceShader = _GetFallbackShader();

    //
    // topology
    //
    HdBufferSourceSharedPtr indicesSource(
        new HdVtBufferSource(HdTokens->indices, indicesValue));
    sources.push_back(indicesSource);
    indicesSource->AddBufferSpecs(&bufferSpecs);

    HdBufferArrayRangeSharedPtr topologyRange =
        registry->AllocateNonUniformBufferArrayRange(
            HdTokens->topology, bufferSpecs);

    registry->AddSources(topologyRange, sources);
    sources.clear();
    bufferSpecs.clear();

    //
    // constant primvar
    //
    GfMatrix4d matrix(1);
    sources.push_back(
        HdBufferSourceSharedPtr(new
            HdVtBufferSource(HdTokens->transform, matrix)));
    sources.push_back(
        HdBufferSourceSharedPtr(new
            HdVtBufferSource(HdTokens->transformInverse, matrix)));
    sources.push_back(
        HdBufferSourceSharedPtr(new
            HdVtBufferSource(HdTokens->bboxLocalMin, VtValue(GfVec4f(-1)))));
    sources.push_back(
        HdBufferSourceSharedPtr(new
            HdVtBufferSource(HdTokens->bboxLocalMax, VtValue(GfVec4f(1)))));
    sources.push_back(
        HdBufferSourceSharedPtr(new
            HdVtBufferSource(HdTokens->primID, VtValue(GfVec4f(1)))));
    if (colorsValue.IsEmpty()) {
        sources.push_back(
            HdBufferSourceSharedPtr(new
                HdVtBufferSource(HdTokens->color, VtValue(GfVec4f(1)))));
    }

    GLenum matType = HdVtBufferSource::GetDefaultMatrixType();
    bufferSpecs.push_back(HdBufferSpec(HdTokens->transform, matType, 16));
    bufferSpecs.push_back(HdBufferSpec(HdTokens->transformInverse, matType, 16));
    bufferSpecs.push_back(HdBufferSpec(HdTokens->bboxLocalMin, GL_FLOAT, 4));
    bufferSpecs.push_back(HdBufferSpec(HdTokens->bboxLocalMax, GL_FLOAT, 4));
    bufferSpecs.push_back(HdBufferSpec(HdTokens->primID, GL_FLOAT, 4));
    if (colorsValue.IsEmpty()) {
        bufferSpecs.push_back(HdBufferSpec(HdTokens->color, GL_FLOAT, 4));
    }

    HdBufferArrayRangeSharedPtr constantPrimVarRange =
        registry->AllocateShaderStorageBufferArrayRange(
            HdTokens->primVar, bufferSpecs);

    registry->AddSources(constantPrimVarRange, sources);
    sources.clear();
    bufferSpecs.clear();

    //
    // vertex primvar
    //
    HdBufferSourceSharedPtr pointsSource(
        new HdVtBufferSource(HdTokens->points, pointsValue));
    sources.push_back(pointsSource);
    pointsSource->AddBufferSpecs(&bufferSpecs);

    if (not normalsValue.IsEmpty()) {
        HdBufferSourceSharedPtr normalsSource(
            new HdVtBufferSource(HdTokens->normals, normalsValue));
        sources.push_back(normalsSource);
        normalsSource->AddBufferSpecs(&bufferSpecs);
    }

    if (not colorsValue.IsEmpty()) {
        HdBufferSourceSharedPtr colorsSource(
            new HdVtBufferSource(HdTokens->color, colorsValue));
        sources.push_back(colorsSource);
        colorsSource->AddBufferSpecs(&bufferSpecs);
    }

    HdBufferArrayRangeSharedPtr vertexPrimVarRange =
        registry->AllocateNonUniformBufferArrayRange(
            HdTokens->primVar, bufferSpecs);

    registry->AddSources(vertexPrimVarRange, sources);
    sources.clear();
    bufferSpecs.clear();

    //
    // bounds
    // 
    GfRange3d range;
    TF_FOR_ALL(it, pointsValue.Get<VtVec3fArray>()) {
        range.ExtendBy(*it);
    }
    sharedData->bounds.SetRange(range);

    HdDrawItem drawItem(sharedData);
    Hd_MeshShaderKey shaderKey(primitiveMode,
                               /*lit=*/true,
                               /*smoothNormals=*/true,
                               /*doubleSided=*/false,
                               /*faceVarying=*/false,
                               HdCullStyleNothing,
                               HdMeshGeomStyleSurf);

    // need to register to get batching works
    Hd_GeometricShaderSharedPtr geomShader = Hd_GeometricShader::Create(shaderKey);
    drawItem.SetGeometricShader(geomShader);

    HdDrawingCoord *drawingCoord = drawItem.GetDrawingCoord();
    sharedData->barContainer.Set(drawingCoord->GetConstantPrimVarIndex(), constantPrimVarRange);
    sharedData->barContainer.Set(drawingCoord->GetVertexPrimVarIndex(), vertexPrimVarRange);
    sharedData->barContainer.Set(drawingCoord->GetTopologyIndex(), topologyRange);

    return drawItem;
}

static std::vector<HdDrawItem>
_GetDrawItems(std::vector<HdRprimSharedData> &sharedData)
{
    std::vector<HdDrawItem> result;

    GLenum trisMode = GL_TRIANGLES;
    int trisI[] = {
        0, 1, 2,
    };
    GfVec3f trisP[] = {
        GfVec3f( 1.0f, 1.0f, 0.0f),
        GfVec3f(-1.0f,-1.0f, 0.0f),
        GfVec3f( 1.0f,-1.0f, 0.0f),
    };
    GfVec3f trisN[] = {
        GfVec3f(0.0f, 0.0f, 1.0f),
        GfVec3f(0.0f, 0.0f, 1.0f),
        GfVec3f(0.0f, 0.0f, 1.0f),
    };
    GfVec4f trisC[] = {
        GfVec4f(0.0f, 0.0f, 1.0f, 1.0f),
        GfVec4f(0.0f, 0.0f, 1.0f, 1.0f),
        GfVec4f(0.0f, 0.0f, 1.0f, 1.0f),
    };

    GLenum quadsMode = GL_LINES_ADJACENCY;
    int quadsI[] = {
        0, 1, 2, 3
    };
    GfVec3f quadsP[] = {
        GfVec3f( 1.0f, 1.0f, 0.0f),
        GfVec3f(-1.0f, 1.0f, 0.0f),
        GfVec3f(-1.0f,-1.0f, 0.0f),
        GfVec3f( 1.0f,-1.0f, 0.0f),
    };
    GfVec3f quadsN[] = {
        GfVec3f(0.0f, 0.0f, 1.0f),
        GfVec3f(0.0f, 0.0f, 1.0f),
        GfVec3f(0.0f, 0.0f, 1.0f),
        GfVec3f(0.0f, 0.0f, 1.0f),
    };
    GfVec4f quadsC[] = {
        GfVec4f(0.0f, 0.0f, 1.0f, 1.0f),
        GfVec4f(0.0f, 0.0f, 1.0f, 1.0f),
        GfVec4f(0.0f, 0.0f, 1.0f, 1.0f),
        GfVec4f(0.0f, 0.0f, 1.0f, 1.0f),
    };

    // tris w/o color
    result.push_back(_RegisterDrawItem(
        trisMode, &sharedData[0],
        _BuildArrayValue(trisI, sizeof(trisI)/sizeof(trisI[0])),
        _BuildArrayValue(trisP, sizeof(trisP)/sizeof(trisP[0])),
        _BuildArrayValue(trisN, sizeof(trisN)/sizeof(trisN[0]))));

    result.push_back(_RegisterDrawItem(
        trisMode, &sharedData[1],
        _BuildArrayValue(trisI, sizeof(trisI)/sizeof(trisI[0])),
        _BuildArrayValue(trisP, sizeof(trisP)/sizeof(trisP[0])),
        _BuildArrayValue(trisN, sizeof(trisN)/sizeof(trisN[0]))));

    // quads w/o color
    result.push_back(_RegisterDrawItem(
        quadsMode, &sharedData[2],
        _BuildArrayValue(quadsI, sizeof(quadsI)/sizeof(quadsI[0])),
        _BuildArrayValue(quadsP, sizeof(quadsP)/sizeof(quadsP[0])),
        _BuildArrayValue(quadsN, sizeof(quadsN)/sizeof(quadsN[0]))));

    result.push_back(_RegisterDrawItem(
        quadsMode, &sharedData[3],
        _BuildArrayValue(quadsI, sizeof(quadsI)/sizeof(quadsI[0])),
        _BuildArrayValue(quadsP, sizeof(quadsP)/sizeof(quadsP[0])),
        _BuildArrayValue(quadsN, sizeof(quadsN)/sizeof(quadsN[0]))));

    // quads w/ color
    result.push_back(_RegisterDrawItem(
        quadsMode, &sharedData[4],
        _BuildArrayValue(quadsI, sizeof(quadsI)/sizeof(quadsI[0])),
        _BuildArrayValue(quadsP, sizeof(quadsP)/sizeof(quadsP[0])),
        _BuildArrayValue(quadsN, sizeof(quadsN)/sizeof(quadsN[0])),
        _BuildArrayValue(quadsC, sizeof(quadsC)/sizeof(quadsC[0]))));

    result.push_back(_RegisterDrawItem(
        quadsMode, &sharedData[5],
        _BuildArrayValue(quadsI, sizeof(quadsI)/sizeof(quadsI[0])),
        _BuildArrayValue(quadsP, sizeof(quadsP)/sizeof(quadsP[0])),
        _BuildArrayValue(quadsN, sizeof(quadsN)/sizeof(quadsN[0])),
        _BuildArrayValue(quadsC, sizeof(quadsC)/sizeof(quadsC[0]))));

    // tris w/ color
    result.push_back(_RegisterDrawItem(
        trisMode, &sharedData[6],
        _BuildArrayValue(trisI, sizeof(trisI)/sizeof(trisI[0])),
        _BuildArrayValue(trisP, sizeof(trisP)/sizeof(trisP[0])),
        _BuildArrayValue(trisN, sizeof(trisN)/sizeof(trisN[0])),
        _BuildArrayValue(trisC, sizeof(trisC)/sizeof(trisC[0]))));

    result.push_back(_RegisterDrawItem(
        trisMode, &sharedData[7],
        _BuildArrayValue(trisI, sizeof(trisI)/sizeof(trisI[0])),
        _BuildArrayValue(trisP, sizeof(trisP)/sizeof(trisP[0])),
        _BuildArrayValue(trisN, sizeof(trisN)/sizeof(trisN[0])),
        _BuildArrayValue(trisC, sizeof(trisC)/sizeof(trisC[0]))));

    // tris w/o color
    result.push_back(_RegisterDrawItem(
        trisMode, &sharedData[8],
        _BuildArrayValue(trisI, sizeof(trisI)/sizeof(trisI[0])),
        _BuildArrayValue(trisP, sizeof(trisP)/sizeof(trisP[0])),
        _BuildArrayValue(trisN, sizeof(trisN)/sizeof(trisN[0]))));

    result.push_back(_RegisterDrawItem(
        trisMode, &sharedData[9],
        _BuildArrayValue(trisI, sizeof(trisI)/sizeof(trisI[0])),
        _BuildArrayValue(trisP, sizeof(trisP)/sizeof(trisP[0])),
        _BuildArrayValue(trisN, sizeof(trisN)/sizeof(trisN[0]))));

    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();

    registry->Commit();

    return result;
}

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
    PrintPerfCounter(perfLog, HdPerfTokens->drawCalls);
}

static void
ImmediateDrawBatchTest()
{
    std::cout << "==== ImmediateDrawBatchTest:\n";

    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();

    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    VtDictionary dict = registry->GetResourceAllocation();
    Dump("----- begin -----\n", dict, perfLog);

    std::vector<HdRprimSharedData> sharedData(10, HdDrawingCoord::DefaultNumSlots);

    std::vector<HdDrawItem> drawItems = _GetDrawItems(sharedData);
    std::vector<HdDrawItemInstance> drawItemInstances;
    {
        TF_FOR_ALL(drawItemIt, drawItems) {
            drawItemInstances.push_back(HdDrawItemInstance(
                &(*drawItemIt)));
        }
    }
    std::vector<Hd_DrawBatchSharedPtr> drawBatches;
    {
        Hd_DrawBatchSharedPtr batch;
        TF_FOR_ALL(drawItemInstanceIt, drawItemInstances) {
            HdDrawItemInstance *drawItemInstance = &(*drawItemInstanceIt);

            if (not batch or not batch->Append(drawItemInstance)) {
                batch.reset(new Hd_ImmediateDrawBatch(drawItemInstance));
                drawBatches.push_back(batch);
            }
        }
    }

    std::cout << "num batches: " << drawBatches.size() << "\n";

    dict = registry->GetResourceAllocation();
    Dump("----- batched -----\n", dict, perfLog);

    HdRenderPassStateSharedPtr renderPassState(new HdRenderPassState());

    TF_FOR_ALL(batchIt, drawBatches) {
        (*batchIt)->PrepareDraw(renderPassState);
    }
    TF_FOR_ALL(batchIt, drawBatches) {
        (*batchIt)->ExecuteDraw(renderPassState);
    }
    dict = registry->GetResourceAllocation();
    Dump("----- executed -----\n", dict, perfLog);

    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->drawCalls) == 10);

    // clear all
    drawItems.clear();
    drawBatches.clear();
    sharedData.clear();

    // explicit compaction
    registry->GarbageCollect();

    dict = registry->GetResourceAllocation();
    Dump("----- clear all -----\n", dict, perfLog);

    std::cout << "\n";
}

static void
IndirectDrawBatchTest()
{
    std::cout << "==== IndirectDrawBatchTest:\n";

    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();

    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    VtDictionary dict = registry->GetResourceAllocation();
    Dump("----- begin -----\n", dict, perfLog);

    std::vector<HdRprimSharedData> sharedData(
        10, HdDrawingCoord::DefaultNumSlots);
    std::vector<HdDrawItem> drawItems = _GetDrawItems(sharedData);
    std::vector<HdDrawItemInstance> drawItemInstances;
    {
        TF_FOR_ALL(drawItemIt, drawItems) {
            drawItemInstances.push_back(HdDrawItemInstance(
                &(*drawItemIt)));
        }
    }
    std::vector<Hd_DrawBatchSharedPtr> drawBatches;
    {
        Hd_DrawBatchSharedPtr batch;

        TF_FOR_ALL(drawItemInstanceIt, drawItemInstances) {
            HdDrawItemInstance *drawItemInstance = &(*drawItemInstanceIt);

            if (not batch or not batch->Append(drawItemInstance)) {
                batch.reset(new Hd_IndirectDrawBatch(drawItemInstance));
                drawBatches.push_back(batch);
            }
        }
    }

    std::cout << "num batches: " << drawBatches.size() << "\n";

    dict = registry->GetResourceAllocation();
    Dump("----- batched -----\n", dict, perfLog);

    HdRenderPassStateSharedPtr renderPassState(new HdRenderPassState());

    TF_FOR_ALL(batchIt, drawBatches) {
        (*batchIt)->PrepareDraw(renderPassState);
    }
    TF_FOR_ALL(batchIt, drawBatches) {
        (*batchIt)->ExecuteDraw(renderPassState);
    }
    dict = registry->GetResourceAllocation();
    Dump("----- executed -----\n", dict, perfLog);

    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->drawCalls) == 5);

    // clear all
    drawItems.clear();
    drawBatches.clear();
    sharedData.clear();

    // explicit compaction
    registry->GarbageCollect();

    dict = registry->GetResourceAllocation();
    Dump("----- clear all -----\n", dict, perfLog);

    std::cout << "\n";
}

static void
IndirectDrawBatchMigrationTest()
{
    std::cout << "==== IndirectDrawBatchMigrationTest:\n";

    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();
    registry->GarbageCollect();

    HdPerfLog &perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    VtDictionary dict = registry->GetResourceAllocation();
    Dump("----- begin -----\n", dict, perfLog);

    Hd_TestDriver driver;
    Hd_UnitTestDelegate &delegate = driver.GetDelegate();

    delegate.AddCube(SdfPath("/subdiv1"), GfMatrix4f(1), false, SdfPath(),
                     PxOsdOpenSubdivTokens->catmark);
    delegate.AddCube(SdfPath("/bilinear1"), GfMatrix4f(1), false, SdfPath(),
                     PxOsdOpenSubdivTokens->bilinear);
    delegate.AddCube(SdfPath("/subdiv2"), GfMatrix4f(1), false, SdfPath(),
                     PxOsdOpenSubdivTokens->catmark);
    delegate.AddCube(SdfPath("/bilinear2"), GfMatrix4f(1), false, SdfPath(),
                     PxOsdOpenSubdivTokens->bilinear);

    // create 2 renderpasses (smooth & flat)
    HdRenderPassSharedPtr smoothPass(
        new HdRenderPass(&delegate.GetRenderIndex(),
                         HdRprimCollection(HdTokens->geometry,
                                           HdTokens->smoothHull)));
    HdRenderPassSharedPtr flatPass(
        new HdRenderPass(&delegate.GetRenderIndex(),
                         HdRprimCollection(HdTokens->geometry,
                                           HdTokens->hull)));

    HdRenderPassStateSharedPtr renderPassState(new HdRenderPassState());

    // Set camera (for the itemsDrawn counter)
    GfMatrix4d modelView, projection;
    modelView.SetIdentity();
    projection.SetIdentity();
    GfVec4d viewport(0, 0, 512, 512);
    renderPassState->SetCamera(modelView, projection, viewport);
    renderPassState->SetCamera(modelView, projection, viewport);

    PrintPerfCounter(perfLog, HdPerfTokens->rebuildBatches);
    PrintPerfCounter(perfLog, HdPerfTokens->bufferArrayRangeMerged);

    // Draw flat pass. This produces 1 buffer array containing both catmark
    // and bilinear mesh since we don't need normals.
    driver.Draw(flatPass);

    dict = registry->GetResourceAllocation();
    Dump("----- draw flat -----\n", dict, perfLog);
    PrintPerfCounter(perfLog, HdPerfTokens->drawBatches);
    PrintPerfCounter(perfLog, HdTokens->itemsDrawn);
    PrintPerfCounter(perfLog, HdPerfTokens->collectionsRefreshed);
    PrintPerfCounter(perfLog, HdPerfTokens->rebuildBatches);
    PrintPerfCounter(perfLog, HdPerfTokens->bufferArrayRangeMerged);

    // Draw smooth pass. Then subdiv meshes need to be migrated into new
    // buffer array, while bilinear meshes remain.
    driver.Draw(smoothPass);

    dict = registry->GetResourceAllocation();
    Dump("----- draw smooth -----\n", dict, perfLog);
    PrintPerfCounter(perfLog, HdPerfTokens->drawBatches);
    PrintPerfCounter(perfLog, HdTokens->itemsDrawn);
    PrintPerfCounter(perfLog, HdPerfTokens->collectionsRefreshed);
    PrintPerfCounter(perfLog, HdPerfTokens->rebuildBatches);
    PrintPerfCounter(perfLog, HdPerfTokens->bufferArrayRangeMerged);

    // Draw flat pass again. Batches will be rebuilt.
    driver.Draw(flatPass);

    dict = registry->GetResourceAllocation();
    Dump("----- draw flat -----\n", dict, perfLog);
    PrintPerfCounter(perfLog, HdPerfTokens->drawBatches);
    PrintPerfCounter(perfLog, HdTokens->itemsDrawn);
    PrintPerfCounter(perfLog, HdPerfTokens->collectionsRefreshed);
    PrintPerfCounter(perfLog, HdPerfTokens->rebuildBatches);
    PrintPerfCounter(perfLog, HdPerfTokens->bufferArrayRangeMerged);

    // Draw smooth pass again.
    driver.Draw(smoothPass);

    dict = registry->GetResourceAllocation();
    Dump("----- draw smooth -----\n", dict, perfLog);
    PrintPerfCounter(perfLog, HdPerfTokens->drawBatches);
    PrintPerfCounter(perfLog, HdTokens->itemsDrawn);
    PrintPerfCounter(perfLog, HdPerfTokens->collectionsRefreshed);
    PrintPerfCounter(perfLog, HdPerfTokens->rebuildBatches);
    PrintPerfCounter(perfLog, HdPerfTokens->bufferArrayRangeMerged);
}

static void
EmptyDrawBatchTest()
{
    std::cout << "==== EmptyDrawBatchTest:\n";

    // This test covers bug 120354.
    //
    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();
    registry->GarbageCollect();

    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    VtDictionary dict = registry->GetResourceAllocation();
    Dump("----- begin -----\n", dict, perfLog);

    HdRprimSharedData sharedData(HdDrawingCoord::DefaultNumSlots);
    sharedData.surfaceShader = _GetFallbackShader();

    //
    // vertex primvar (points, widths)
    //
    HdBufferSourceVector sources;
    HdBufferSpecVector bufferSpecs;
    HdBufferSourceSharedPtr pointsSource(
        new HdVtBufferSource(HdTokens->points, VtValue(VtVec3fArray(1))));
    sources.push_back(pointsSource);
    pointsSource->AddBufferSpecs(&bufferSpecs);
    HdBufferSourceSharedPtr widthsSource(
        new HdVtBufferSource(HdTokens->widths, VtValue(VtFloatArray(1))));
    sources.push_back(widthsSource);
    widthsSource->AddBufferSpecs(&bufferSpecs);
    HdBufferArrayRangeSharedPtr vertexPrimVarRange =
        registry->AllocateNonUniformBufferArrayRange(
            HdTokens->primVar, bufferSpecs);

    registry->AddSources(vertexPrimVarRange, sources);
    sources.clear();
    bufferSpecs.clear();

    //
    // instance indices (instance)  EMPTY
    //
    HdBufferSourceSharedPtr instanceIndices(
        new HdVtBufferSource(HdTokens->instanceIndices, VtValue(VtIntArray(0))));
    sources.push_back(instanceIndices);
    instanceIndices->AddBufferSpecs(&bufferSpecs);
    HdBufferSourceSharedPtr culledInstanceIndices(
        new HdVtBufferSource(HdTokens->culledInstanceIndices, VtValue(VtIntArray(0))));
    sources.push_back(culledInstanceIndices);
    culledInstanceIndices->AddBufferSpecs(&bufferSpecs);
    HdBufferArrayRangeSharedPtr instanceIndexRange =
        registry->AllocateNonUniformBufferArrayRange(
            HdTokens->topology, bufferSpecs);

    registry->AddSources(instanceIndexRange, sources);
    sources.clear();
    bufferSpecs.clear();

    //
    // constant primvar
    //
    GfMatrix4d matrix(1);
    sources.push_back(
        HdBufferSourceSharedPtr(new
            HdVtBufferSource(HdTokens->transform, matrix)));
    sources.push_back(
        HdBufferSourceSharedPtr(new
            HdVtBufferSource(HdTokens->transformInverse, matrix)));
    sources.push_back(
        HdBufferSourceSharedPtr(new
            HdVtBufferSource(HdTokens->bboxLocalMin, VtValue(GfVec4f(-1)))));
    sources.push_back(
        HdBufferSourceSharedPtr(new
            HdVtBufferSource(HdTokens->bboxLocalMax, VtValue(GfVec4f(1)))));
    sources.push_back(
        HdBufferSourceSharedPtr(new
            HdVtBufferSource(HdTokens->primID, VtValue(GfVec4f(1)))));

    GLenum matType = HdVtBufferSource::GetDefaultMatrixType();
    bufferSpecs.push_back(HdBufferSpec(HdTokens->transform, matType, 16));
    bufferSpecs.push_back(HdBufferSpec(HdTokens->transformInverse, matType, 16));
    bufferSpecs.push_back(HdBufferSpec(HdTokens->bboxLocalMin, GL_FLOAT, 4));
    bufferSpecs.push_back(HdBufferSpec(HdTokens->bboxLocalMax, GL_FLOAT, 4));
    bufferSpecs.push_back(HdBufferSpec(HdTokens->primID, GL_FLOAT, 4));
    bufferSpecs.push_back(HdBufferSpec(HdTokens->color, GL_FLOAT, 4));

    HdBufferArrayRangeSharedPtr constantPrimVarRange =
        registry->AllocateShaderStorageBufferArrayRange(
            HdTokens->primVar, bufferSpecs);

    registry->AddSources(constantPrimVarRange, sources);
    sources.clear();
    bufferSpecs.clear();

    GfRange3d range(GfVec3d(-1,-1,-1), GfVec3d(1,1,1));
    sharedData.bounds.SetRange(range);

    HdDrawItem drawItem(&sharedData);
    Hd_PointsShaderKey shaderKey;

    // need to register to get batching works
    Hd_GeometricShaderSharedPtr geomShader = Hd_GeometricShader::Create(shaderKey);
    drawItem.SetGeometricShader(geomShader);

    HdDrawingCoord *drawingCoord = drawItem.GetDrawingCoord();
    sharedData.barContainer.Set(drawingCoord->GetConstantPrimVarIndex(), constantPrimVarRange);
    sharedData.barContainer.Set(drawingCoord->GetVertexPrimVarIndex(), vertexPrimVarRange);
    sharedData.barContainer.Set(drawingCoord->GetInstanceIndexIndex(), instanceIndexRange);

    HdDrawItemInstance drawItemInstance(&drawItem);

    Hd_DrawBatchSharedPtr batch(new Hd_IndirectDrawBatch(&drawItemInstance));

    dict = registry->GetResourceAllocation();
    Dump("----- batched -----\n", dict, perfLog);

    registry->Commit();

    HdRenderPassStateSharedPtr renderPassState(new HdRenderPassState());
    batch->PrepareDraw(renderPassState);
    batch->ExecuteDraw(renderPassState);

    // ---------------------------------------------------

    dict = registry->GetResourceAllocation();
    Dump("----- executed -----\n", dict, perfLog);

    registry->GarbageCollect();

    dict = registry->GetResourceAllocation();
    Dump("----- clear all -----\n", dict, perfLog);

    std::cout << "\n";
}

int main()
{
    GlfTestGLContext::RegisterGLContextCallbacks();
    GlfGlewInit();
    GlfSharedGLContextScopeHolder sharedContext;

    TfErrorMark mark;

    ImmediateDrawBatchTest();
    IndirectDrawBatchTest();
    IndirectDrawBatchMigrationTest();
    EmptyDrawBatchTest();

    GLF_POST_PENDING_GL_ERRORS();

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

