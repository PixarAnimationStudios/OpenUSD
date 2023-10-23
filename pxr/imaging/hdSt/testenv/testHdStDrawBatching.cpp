//
// Copyright 2023 Pixar
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

#include "pxr/imaging/hdSt/drawItemInstance.h"
#include "pxr/imaging/hdSt/glslfxShader.h"
#include "pxr/imaging/hdSt/indirectDrawBatch.h"
#include "pxr/imaging/hdSt/materialNetworkShader.h"
#include "pxr/imaging/hdSt/meshShaderKey.h"
#include "pxr/imaging/hdSt/package.h"
#include "pxr/imaging/hdSt/pointsShaderKey.h"
#include "pxr/imaging/hdSt/renderPass.h"
#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/rprimSharedData.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hio/glslfx.h"
#include "pxr/imaging/glf/testGLContext.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/gf/vec3f.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static HdSt_MaterialNetworkShaderSharedPtr _GetFallbackShader()
{
    static HdSt_MaterialNetworkShaderSharedPtr _fallbackMaterialNetworkShader;

    if (!_fallbackMaterialNetworkShader) {
        HioGlslfxSharedPtr const glslfx =
            std::make_shared<HioGlslfx>(
                HdStPackageFallbackMaterialNetworkShader());
        _fallbackMaterialNetworkShader =
                std::make_shared<HdStGLSLFXShader>(glslfx);
    }
    return _fallbackMaterialNetworkShader;
}

static HdStResourceRegistrySharedPtr _GetResourceRegistry()
{
    static HgiUniquePtr _hgi = Hgi::CreatePlatformDefaultHgi();
    static HdStResourceRegistrySharedPtr _resourceRegistry = 
        std::make_shared<HdStResourceRegistry>(_hgi.get());

    return _resourceRegistry;
}

template <typename T>
static VtValue
_BuildArrayValue(T values[], int numValues)
{
    VtArray<T> result(numValues);
    std::copy(values, values+numValues, result.begin());
    return VtValue(result);
}

HdStDrawItem
_RegisterDrawItem(
    HdSt_GeometricShader::PrimitiveType primType,
    HdRprimSharedData *sharedData,
    VtValue const & indicesValue,
    VtValue const & primitiveParamValue,
    VtValue const & edgeIndicesValue,
    VtValue const & pointsValue,
    VtValue const & normalsValue = VtValue(),
    VtValue const & colorsValue = VtValue())
{
    HdStResourceRegistrySharedPtr const& registry = _GetResourceRegistry();

    HdBufferSourceSharedPtrVector sources;
    HdBufferSpecVector bufferSpecs;

    //
    // topology
    //
    HdBufferSourceSharedPtr const indicesSource =
        std::make_shared<HdVtBufferSource>(
            HdTokens->indices, indicesValue);
    sources = { indicesSource };
    indicesSource->GetBufferSpecs(&bufferSpecs);

    HdBufferSourceSharedPtr const primitiveParamSource =
        std::make_shared<HdVtBufferSource>(
            HdTokens->primitiveParam, primitiveParamValue);
    primitiveParamSource->GetBufferSpecs(&bufferSpecs);

    HdBufferSourceSharedPtr const edgeIndicesSource =
        std::make_shared<HdVtBufferSource>(
            HdTokens->edgeIndices, edgeIndicesValue);
    edgeIndicesSource->GetBufferSpecs(&bufferSpecs);

    HdBufferArrayRangeSharedPtr const topologyRange =
        registry->AllocateNonUniformBufferArrayRange(
            HdTokens->topology, bufferSpecs, HdBufferArrayUsageHint());

    registry->AddSources(topologyRange, std::move(sources));
    sources.clear();
    bufferSpecs.clear();

    //
    // constant primvar
    //
    const GfMatrix4d matrix(1);
    sources = {
        std::make_shared<HdVtBufferSource>(
            HdTokens->transform, matrix),
        std::make_shared<HdVtBufferSource>(
            HdTokens->transformInverse, matrix),
        std::make_shared<HdVtBufferSource>(
            HdTokens->bboxLocalMin, VtValue(GfVec4f(-1))),
        std::make_shared<HdVtBufferSource>(
            HdTokens->bboxLocalMax, VtValue(GfVec4f(1))),
        std::make_shared<HdVtBufferSource>(
            HdTokens->primID, VtValue(GfVec4f(1))) };
    if (colorsValue.IsEmpty()) {
        sources.push_back(
            std::make_shared<HdVtBufferSource>(
                HdTokens->displayColor, VtValue(GfVec3f(1))));
    }

    const HdType matType = HdVtBufferSource::GetDefaultMatrixType();
    bufferSpecs.emplace_back(HdTokens->transform,
                             HdTupleType {matType, 1});
    bufferSpecs.emplace_back(HdTokens->transformInverse,
                             HdTupleType {matType, 1});
    bufferSpecs.emplace_back(HdTokens->bboxLocalMin,
                             HdTupleType {HdTypeFloatVec4, 1});
    bufferSpecs.emplace_back(HdTokens->bboxLocalMax,
                             HdTupleType {HdTypeFloatVec4, 1});
    bufferSpecs.emplace_back(HdTokens->primID,
                             HdTupleType {HdTypeFloatVec4, 1});
    if (colorsValue.IsEmpty()) {
        bufferSpecs.emplace_back(HdTokens->displayColor,
                             HdTupleType {HdTypeFloatVec3, 1});
    }

    HdBufferArrayRangeSharedPtr const constantPrimvarRange =
        registry->AllocateShaderStorageBufferArrayRange(
            HdTokens->primvar, bufferSpecs, HdBufferArrayUsageHint());

    registry->AddSources(constantPrimvarRange, std::move(sources));
    sources.clear();
    bufferSpecs.clear();

    //
    // vertex primvar
    //
    HdBufferSourceSharedPtr const pointsSource =
        std::make_shared<HdVtBufferSource>(HdTokens->points, pointsValue);
    sources = { pointsSource };
    pointsSource->GetBufferSpecs(&bufferSpecs);

    if (!normalsValue.IsEmpty()) {
        HdBufferSourceSharedPtr const normalsSource =
            std::make_shared<HdVtBufferSource>(
                HdStTokens->smoothNormals, normalsValue);
        sources.push_back(normalsSource);
        normalsSource->GetBufferSpecs(&bufferSpecs);
    }

    if (!colorsValue.IsEmpty()) {
        HdBufferSourceSharedPtr const colorsSource =
            std::make_shared<HdVtBufferSource>(
                HdTokens->displayColor, colorsValue);
        sources.push_back(colorsSource);
        colorsSource->GetBufferSpecs(&bufferSpecs);
    }

    HdBufferArrayRangeSharedPtr const vertexPrimvarRange =
        registry->AllocateNonUniformBufferArrayRange(
            HdTokens->primvar, bufferSpecs, HdBufferArrayUsageHint());

    registry->AddSources(vertexPrimvarRange, std::move(sources));
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

    HdStDrawItem drawItem(sharedData);
   
    HdSt_MeshShaderKey shaderKey(primType,
        /*shadingTerminal*/HdMeshReprDescTokens->surfaceShader,
        /*normalsSource=*/HdSt_MeshShaderKey::NormalSourceSmooth,
        /*normalsInterpolation=*/HdInterpolationVertex,
        HdCullStyleNothing,
        HdMeshGeomStyleSurf,
        HdSt_GeometricShader::FvarPatchType::PATCH_NONE,
        /*lineWidth=*/0,
        /*doubleSided=*/false,
        /*hasBuiltinBarycentrics*/false,
        /* hasMetalTessellation */ false,
        /*hasCustomDisplacementTerminal=*/false,
        /*faceVarying=*/false,
        /*hasTopologicalVisibility=*/false,
        /*blendWireframeColor=*/false,
        /*hasMirroredTransform=*/false,
        /*hasInstancer=*/false,
        /*enableScalarOverride=*/ true,
        /*isWidget*/ false,
        /* forceOpaqueEdges */ true);

    // need to register to get batching works
    HdSt_GeometricShaderSharedPtr const geomShader = 
        HdSt_GeometricShader::Create(shaderKey, registry);
    TF_VERIFY(geomShader);
    drawItem.SetGeometricShader(geomShader);
    drawItem.SetMaterialNetworkShader(_GetFallbackShader());

    HdDrawingCoord  const *drawingCoord = drawItem.GetDrawingCoord();
    sharedData->barContainer.Set(drawingCoord->GetConstantPrimvarIndex(), constantPrimvarRange);
    sharedData->barContainer.Set(drawingCoord->GetVertexPrimvarIndex(), vertexPrimvarRange);
    sharedData->barContainer.Set(drawingCoord->GetTopologyIndex(), topologyRange);

    return drawItem;
}

static std::vector<HdStDrawItem>
_GetDrawItems(std::vector<HdRprimSharedData> &sharedData)
{
    std::vector<HdStDrawItem> result;

    HdSt_GeometricShader::PrimitiveType primTypeTris = 
        HdSt_GeometricShader::PrimitiveType::PRIM_MESH_COARSE_TRIANGLES;

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
    int triEdges[] = {
        0
    };

    HdSt_GeometricShader::PrimitiveType primTypeQuads = 
        HdSt_GeometricShader::PrimitiveType::PRIM_MESH_COARSE_QUADS;
        
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
    int quadPP[] = {
        0
    };
    GfVec2i quadEdges[] = {
        GfVec2i(0,1)
    };
    // tris w/o color
    result.push_back(_RegisterDrawItem(
        primTypeTris, &sharedData[0],
        _BuildArrayValue(trisI, sizeof(trisI)/sizeof(trisI[0])),
        _BuildArrayValue(trisI, sizeof(trisI)/sizeof(trisI[0])), /* dummy pparam*/
        _BuildArrayValue(triEdges, sizeof(triEdges)/sizeof(triEdges[0])),
        _BuildArrayValue(trisP, sizeof(trisP)/sizeof(trisP[0])),
        _BuildArrayValue(trisN, sizeof(trisN)/sizeof(trisN[0]))));

    result.push_back(_RegisterDrawItem(
        primTypeTris, &sharedData[1],
        _BuildArrayValue(trisI, sizeof(trisI)/sizeof(trisI[0])),
        _BuildArrayValue(trisI, sizeof(trisI)/sizeof(trisI[0])), /* dummy pparam*/
        _BuildArrayValue(triEdges, sizeof(triEdges)/sizeof(triEdges[0])),
        _BuildArrayValue(trisP, sizeof(trisP)/sizeof(trisP[0])),
        _BuildArrayValue(trisN, sizeof(trisN)/sizeof(trisN[0]))));

    // quads w/o color
    result.push_back(_RegisterDrawItem(
        primTypeQuads, &sharedData[2],
        _BuildArrayValue(quadsI, sizeof(quadsI)/sizeof(quadsI[0])),
        _BuildArrayValue(quadPP, sizeof(quadPP)/sizeof(quadPP[0])), /* dummy pparam*/
        _BuildArrayValue(quadEdges, sizeof(quadEdges)/sizeof(quadEdges[0])),
        _BuildArrayValue(quadsP, sizeof(quadsP)/sizeof(quadsP[0])),
        _BuildArrayValue(quadsN, sizeof(quadsN)/sizeof(quadsN[0]))));

    result.push_back(_RegisterDrawItem(
        primTypeQuads, &sharedData[3],
        _BuildArrayValue(quadsI, sizeof(quadsI)/sizeof(quadsI[0])),
        _BuildArrayValue(quadPP, sizeof(quadPP)/sizeof(quadPP[0])), /* dummy pparam*/
        _BuildArrayValue(quadEdges, sizeof(quadEdges)/sizeof(quadEdges[0])),
        _BuildArrayValue(quadsP, sizeof(quadsP)/sizeof(quadsP[0])),
        _BuildArrayValue(quadsN, sizeof(quadsN)/sizeof(quadsN[0]))));

    // quads w/ color
    result.push_back(_RegisterDrawItem(
        primTypeQuads, &sharedData[4],
        _BuildArrayValue(quadsI, sizeof(quadsI)/sizeof(quadsI[0])),
        _BuildArrayValue(quadPP, sizeof(quadPP)/sizeof(quadPP[0])), /* dummy pparam*/
        _BuildArrayValue(quadEdges, sizeof(quadEdges)/sizeof(quadEdges[0])),
        _BuildArrayValue(quadsP, sizeof(quadsP)/sizeof(quadsP[0])),
        _BuildArrayValue(quadsN, sizeof(quadsN)/sizeof(quadsN[0])),
        _BuildArrayValue(quadsC, sizeof(quadsC)/sizeof(quadsC[0]))));

    result.push_back(_RegisterDrawItem(
        primTypeQuads, &sharedData[5],
        _BuildArrayValue(quadsI, sizeof(quadsI)/sizeof(quadsI[0])),
        _BuildArrayValue(quadPP, sizeof(quadPP)/sizeof(quadPP[0])), /* dummy pparam*/
        _BuildArrayValue(quadEdges, sizeof(quadEdges)/sizeof(quadEdges[0])),
        _BuildArrayValue(quadsP, sizeof(quadsP)/sizeof(quadsP[0])),
        _BuildArrayValue(quadsN, sizeof(quadsN)/sizeof(quadsN[0])),
        _BuildArrayValue(quadsC, sizeof(quadsC)/sizeof(quadsC[0]))));

    // tris w/ color
    result.push_back(_RegisterDrawItem(
        primTypeTris, &sharedData[6],
        _BuildArrayValue(trisI, sizeof(trisI)/sizeof(trisI[0])),
        _BuildArrayValue(trisI, sizeof(trisI)/sizeof(trisI[0])), /* dummy pparam*/
        _BuildArrayValue(triEdges, sizeof(triEdges)/sizeof(triEdges[0])),
        _BuildArrayValue(trisP, sizeof(trisP)/sizeof(trisP[0])),
        _BuildArrayValue(trisN, sizeof(trisN)/sizeof(trisN[0])),
        _BuildArrayValue(trisC, sizeof(trisC)/sizeof(trisC[0]))));

    result.push_back(_RegisterDrawItem(
        primTypeTris, &sharedData[7],
        _BuildArrayValue(trisI, sizeof(trisI)/sizeof(trisI[0])),
        _BuildArrayValue(trisI, sizeof(trisI)/sizeof(trisI[0])), /* dummy pparam*/
        _BuildArrayValue(triEdges, sizeof(triEdges)/sizeof(triEdges[0])),
        _BuildArrayValue(trisP, sizeof(trisP)/sizeof(trisP[0])),
        _BuildArrayValue(trisN, sizeof(trisN)/sizeof(trisN[0])),
        _BuildArrayValue(trisC, sizeof(trisC)/sizeof(trisC[0]))));

    // tris w/o color
    result.push_back(_RegisterDrawItem(
        primTypeTris, &sharedData[8],
        _BuildArrayValue(trisI, sizeof(trisI)/sizeof(trisI[0])),
        _BuildArrayValue(trisI, sizeof(trisI)/sizeof(trisI[0])), /* dummy pparam*/
        _BuildArrayValue(triEdges, sizeof(triEdges)/sizeof(triEdges[0])),
        _BuildArrayValue(trisP, sizeof(trisP)/sizeof(trisP[0])),
        _BuildArrayValue(trisN, sizeof(trisN)/sizeof(trisN[0]))));

    result.push_back(_RegisterDrawItem(
        primTypeTris, &sharedData[9],
        _BuildArrayValue(trisI, sizeof(trisI)/sizeof(trisI[0])),
        _BuildArrayValue(trisI, sizeof(trisI)/sizeof(trisI[0])), /* dummy pparam*/
        _BuildArrayValue(triEdges, sizeof(triEdges)/sizeof(triEdges[0])),
        _BuildArrayValue(trisP, sizeof(trisP)/sizeof(trisP[0])),
        _BuildArrayValue(trisN, sizeof(trisN)/sizeof(trisN[0]))));

    HdStResourceRegistrySharedPtr const& registry = _GetResourceRegistry();
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
IndirectDrawBatchTest()
{
    std::cout << "==== IndirectDrawBatchTest:\n";

    HdStResourceRegistrySharedPtr const& registry = _GetResourceRegistry();

    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    VtDictionary dict = registry->GetResourceAllocation();
    Dump("----- begin -----\n", dict, perfLog);

    std::vector<HdRprimSharedData> sharedData(
        10, HdDrawingCoord::DefaultNumSlots);
    for (auto &sd : sharedData) {
        sd.instancerLevels = 0;
    }

    std::vector<HdStDrawItem> drawItems = _GetDrawItems(sharedData);
    std::vector<HdStDrawItemInstance> drawItemInstances;
    {
        TF_FOR_ALL(drawItemIt, drawItems) {
            drawItemInstances.push_back(HdStDrawItemInstance(
                &(*drawItemIt)));
        }
    }
    std::vector<HdSt_DrawBatchSharedPtr> drawBatches;
    {
        HdSt_DrawBatchSharedPtr batch;

        TF_FOR_ALL(drawItemInstanceIt, drawItemInstances) {
            HdStDrawItemInstance *const drawItemInstance =
                &(*drawItemInstanceIt);

            if (!batch || !batch->Append(drawItemInstance)) {
                batch = std::make_shared<HdSt_IndirectDrawBatch>(
                    drawItemInstance);
                drawBatches.push_back(batch);
            }
        }
    }

    std::cout << "num batches: " << drawBatches.size() << "\n";

    dict = registry->GetResourceAllocation();
    Dump("----- batched -----\n", dict, perfLog);

    HdStRenderPassStateSharedPtr const renderPassState
        = std::make_shared<HdStRenderPassState>();

    TF_FOR_ALL(batchIt, drawBatches) {
        (*batchIt)->PrepareDraw(nullptr, renderPassState, registry);
    }
    TF_FOR_ALL(batchIt, drawBatches) {
        (*batchIt)->ExecuteDraw(nullptr, renderPassState, registry);
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

    HdPerfLog &perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    HdSt_TestDriver driver;
    HdUnitTestDelegate &delegate = driver.GetDelegate();

    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        std::static_pointer_cast<HdStResourceRegistry>(
        delegate.GetRenderIndex().GetResourceRegistry());

    VtDictionary dict = resourceRegistry->GetResourceAllocation();
    Dump("----- begin -----\n", dict, perfLog);

    delegate.AddCube(SdfPath("/subdiv1"), GfMatrix4f(1), false, SdfPath(),
                     PxOsdOpenSubdivTokens->catmullClark);
    delegate.AddCube(SdfPath("/bilinear1"), GfMatrix4f(1), false, SdfPath(),
                     PxOsdOpenSubdivTokens->bilinear);
    delegate.AddCube(SdfPath("/subdiv2"), GfMatrix4f(1), false, SdfPath(),
                     PxOsdOpenSubdivTokens->catmullClark);
    delegate.AddCube(SdfPath("/bilinear2"), GfMatrix4f(1), false, SdfPath(),
                     PxOsdOpenSubdivTokens->bilinear);

    // create 2 renderpasses (smooth & flat)
    HdRenderPassSharedPtr const smoothPass =
        std::make_shared<HdSt_RenderPass>(
            &delegate.GetRenderIndex(),
            HdRprimCollection(HdTokens->geometry,
                              HdReprSelector(HdReprTokens->smoothHull)));
    HdRenderPassSharedPtr const flatPass =
        std::make_shared<HdSt_RenderPass>(
            &delegate.GetRenderIndex(),
            HdRprimCollection(HdTokens->geometry,
                              HdReprSelector(HdReprTokens->hull)));

    HdStRenderPassStateSharedPtr const renderPassState =
        std::make_shared<HdStRenderPassState>();

    // Set camera (for the itemsDrawn counter)
    GfMatrix4d modelView, projection;
    modelView.SetIdentity();
    projection.SetIdentity();
    GfVec4d viewport(0, 0, 512, 512);
    renderPassState->SetCameraFramingState(
        modelView, projection, viewport, HdRenderPassState::ClipPlanesVector());
    renderPassState->SetCameraFramingState(
        modelView, projection, viewport, HdRenderPassState::ClipPlanesVector());

    PrintPerfCounter(perfLog, HdPerfTokens->rebuildBatches);
    PrintPerfCounter(perfLog, HdPerfTokens->bufferArrayRangeMigrated);

    // Draw flat pass. This produces 1 buffer array containing both catmullClark
    // and bilinear mesh since we don't need normals.
    driver.Draw(flatPass, false);

    dict = resourceRegistry->GetResourceAllocation();
    Dump("----- draw flat -----\n", dict, perfLog);
    PrintPerfCounter(perfLog, HdPerfTokens->drawBatches);
    PrintPerfCounter(perfLog, HdTokens->itemsDrawn);
    PrintPerfCounter(perfLog, HdStPerfTokens->drawItemsFetched);
    PrintPerfCounter(perfLog, HdPerfTokens->rebuildBatches);
    PrintPerfCounter(perfLog, HdPerfTokens->bufferArrayRangeMigrated);

    // Draw smooth pass. Then subdiv meshes need to be migrated into new
    // buffer array, while bilinear meshes remain.
    driver.Draw(smoothPass, false);

    dict = resourceRegistry->GetResourceAllocation();
    Dump("----- draw smooth -----\n", dict, perfLog);
    PrintPerfCounter(perfLog, HdPerfTokens->drawBatches);
    PrintPerfCounter(perfLog, HdTokens->itemsDrawn);
    PrintPerfCounter(perfLog, HdStPerfTokens->drawItemsFetched);
    PrintPerfCounter(perfLog, HdPerfTokens->rebuildBatches);
    PrintPerfCounter(perfLog, HdPerfTokens->bufferArrayRangeMigrated);

    // Draw flat pass again. Batches will be rebuilt.
    driver.Draw(flatPass, false);

    dict = resourceRegistry->GetResourceAllocation();
    Dump("----- draw flat -----\n", dict, perfLog);
    PrintPerfCounter(perfLog, HdPerfTokens->drawBatches);
    PrintPerfCounter(perfLog, HdTokens->itemsDrawn);
    PrintPerfCounter(perfLog, HdStPerfTokens->drawItemsFetched);
    PrintPerfCounter(perfLog, HdPerfTokens->rebuildBatches);
    PrintPerfCounter(perfLog, HdPerfTokens->bufferArrayRangeMigrated);

    // Draw smooth pass again.
    driver.Draw(smoothPass, false);

    dict = resourceRegistry->GetResourceAllocation();
    Dump("----- draw smooth -----\n", dict, perfLog);
    PrintPerfCounter(perfLog, HdPerfTokens->drawBatches);
    PrintPerfCounter(perfLog, HdTokens->itemsDrawn);
    PrintPerfCounter(perfLog, HdStPerfTokens->drawItemsFetched);
    PrintPerfCounter(perfLog, HdPerfTokens->rebuildBatches);
    PrintPerfCounter(perfLog, HdPerfTokens->bufferArrayRangeMigrated);
}

static void
EmptyDrawBatchTest()
{
    std::cout << "==== EmptyDrawBatchTest:\n";

    // This test covers bug 120354.
    //
    HdStResourceRegistrySharedPtr const& registry = _GetResourceRegistry();
    registry->GarbageCollect();

    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();
    perfLog.ResetCounters();

    VtDictionary dict = registry->GetResourceAllocation();
    Dump("----- begin -----\n", dict, perfLog);

    HdRprimSharedData sharedData(HdDrawingCoord::DefaultNumSlots);
    sharedData.instancerLevels = 0;

    //
    // vertex primvar (points, widths)
    //
    HdBufferSourceSharedPtrVector sources;
    HdBufferSpecVector bufferSpecs;
    HdBufferSourceSharedPtr const pointsSource =
        std::make_shared<HdVtBufferSource>(
            HdTokens->points, VtValue(VtVec3fArray(1)));
    sources.push_back(pointsSource);
    pointsSource->GetBufferSpecs(&bufferSpecs);
    HdBufferSourceSharedPtr const widthsSource =
        std::make_shared<HdVtBufferSource>(
            HdTokens->widths, VtValue(VtFloatArray(1)));
    sources.push_back(widthsSource);
    widthsSource->GetBufferSpecs(&bufferSpecs);
    HdBufferArrayRangeSharedPtr const vertexPrimvarRange =
        registry->AllocateNonUniformBufferArrayRange(
            HdTokens->primvar, bufferSpecs, HdBufferArrayUsageHint());

    registry->AddSources(vertexPrimvarRange, std::move(sources));
    sources.clear();
    bufferSpecs.clear();

    //
    // instance indices (instance)  EMPTY
    //
    HdBufferSourceSharedPtr const instanceIndices =
        std::make_shared<HdVtBufferSource>(
            HdInstancerTokens->instanceIndices,
            VtValue(VtIntArray(0)));
    sources.push_back(instanceIndices);
    instanceIndices->GetBufferSpecs(&bufferSpecs);
    HdBufferSourceSharedPtr const culledInstanceIndices =
        std::make_shared<HdVtBufferSource>(
            HdInstancerTokens->culledInstanceIndices,
            VtValue(VtIntArray(0)));
    sources.push_back(culledInstanceIndices);
    culledInstanceIndices->GetBufferSpecs(&bufferSpecs);
    HdBufferArrayRangeSharedPtr const instanceIndexRange =
        registry->AllocateNonUniformBufferArrayRange(
            HdTokens->topology, bufferSpecs, HdBufferArrayUsageHint());

    registry->AddSources(instanceIndexRange, std::move(sources));
    sources.clear();
    bufferSpecs.clear();

    //
    // constant primvar
    //
    const GfMatrix4d matrix(1);
    sources = {
        std::make_shared<HdVtBufferSource>(
            HdTokens->transform, matrix),
        std::make_shared<HdVtBufferSource>(
            HdTokens->transformInverse, matrix),
        std::make_shared<HdVtBufferSource>(
            HdTokens->bboxLocalMin, VtValue(GfVec4f(-1))),
        std::make_shared<HdVtBufferSource>(
            HdTokens->bboxLocalMax, VtValue(GfVec4f(1))),
        std::make_shared<HdVtBufferSource>(
            HdTokens->primID, VtValue(GfVec4f(1))) };

    const HdType matType = HdVtBufferSource::GetDefaultMatrixType();
    bufferSpecs.emplace_back(HdTokens->transform,
                             HdTupleType { matType, 1 });
    bufferSpecs.emplace_back(HdTokens->transformInverse,
                             HdTupleType { matType, 1 });
    bufferSpecs.emplace_back(HdTokens->bboxLocalMin,
                             HdTupleType { HdTypeFloatVec4, 1 });
    bufferSpecs.emplace_back(HdTokens->bboxLocalMax,
                             HdTupleType { HdTypeFloatVec4, 1 });
    bufferSpecs.emplace_back(HdTokens->primID,
                             HdTupleType { HdTypeFloatVec4, 1 });
    bufferSpecs.emplace_back(HdTokens->displayColor,
                             HdTupleType { HdTypeFloatVec3, 1 });

    HdBufferArrayRangeSharedPtr const constantPrimvarRange =
        registry->AllocateShaderStorageBufferArrayRange(
            HdTokens->primvar, bufferSpecs, HdBufferArrayUsageHint());

    registry->AddSources(constantPrimvarRange, std::move(sources));
    sources.clear();
    bufferSpecs.clear();

    const GfRange3d range(GfVec3d(-1,-1,-1), GfVec3d(1,1,1));
    sharedData.bounds.SetRange(range);

    HdStDrawItem drawItem(&sharedData);
    HdSt_PointsShaderKey shaderKey;

    // need to register to get batching works
    HdSt_GeometricShaderSharedPtr const geomShader = 
        HdSt_GeometricShader::Create(shaderKey, registry);
    TF_VERIFY(geomShader);
    drawItem.SetGeometricShader(geomShader);
    drawItem.SetMaterialNetworkShader(_GetFallbackShader());

    HdDrawingCoord * const drawingCoord = drawItem.GetDrawingCoord();
    sharedData.barContainer.Set(drawingCoord->GetConstantPrimvarIndex(), constantPrimvarRange);
    sharedData.barContainer.Set(drawingCoord->GetVertexPrimvarIndex(), vertexPrimvarRange);
    sharedData.barContainer.Set(drawingCoord->GetInstanceIndexIndex(), instanceIndexRange);

    HdStDrawItemInstance drawItemInstance(&drawItem);

    HdSt_DrawBatchSharedPtr const batch =
        std::make_shared<HdSt_IndirectDrawBatch>(&drawItemInstance);

    dict = registry->GetResourceAllocation();
    Dump("----- batched -----\n", dict, perfLog);

    registry->Commit();

    HdStRenderPassStateSharedPtr const renderPassState =
        std::make_shared<HdStRenderPassState>();
    batch->PrepareDraw(nullptr, renderPassState, registry);
    batch->ExecuteDraw(nullptr, renderPassState, registry);

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
    GlfSharedGLContextScopeHolder sharedContext;

    TfErrorMark mark;

    IndirectDrawBatchTest();
    IndirectDrawBatchMigrationTest();
    EmptyDrawBatchTest();

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

