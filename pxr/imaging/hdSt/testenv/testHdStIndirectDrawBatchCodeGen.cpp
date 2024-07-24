//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdSt/drawItemInstance.h"
#include "pxr/imaging/hdSt/indirectDrawBatch.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/glslfxShader.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/glf/testGLContext.h"
#include "pxr/imaging/hio/glslfx.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static const std::string EXPECTED_SHADER_COMPILE_ERROR =
                                          "Failed to compile shader for prim .";

static bool
HdIndirectDrawBatchTest()
{
    HdSt_TestDriver driver;
    HdUnitTestDelegate &delegate = driver.GetDelegate();
    HdRenderIndex& index = delegate.GetRenderIndex();
    index.Clear();
    HdStResourceRegistrySharedPtr const registry = 
        std::static_pointer_cast<HdStResourceRegistry>(
            index.GetResourceRegistry());

    const std::string surfaceShaderSource = ""
        "-- glslfx version 0.1                                      \n"
        "-- configuration                                           \n"
        "{                                                          \n"
        " \"techniques\": {                                         \n"
        "  \"default\": {                                           \n"
        "   \"surfaceShader\" : {                                   \n"
        "    \"source\": [ \"NullSurface\" ]                        \n"
        "   }                                                       \n"
        "  }                                                        \n"
        " }                                                         \n"
        "}                                                          \n"
        "                                                           \n"
        "-- glsl NullSurface                                        \n"
        "                                                           \n"
        "vec4 Surface() {                                           \n"
        "   null                                                    \n"
        "}                                                          \n"
        "                                                           \n";
    std::istringstream nullStream(surfaceShaderSource);
    HioGlslfxSharedPtr const glslfx = std::make_shared<HioGlslfx>(nullStream);
    
    std::unique_ptr<HdRprimSharedData> const sharedData =
        std::make_unique<HdRprimSharedData>(1, true);
    sharedData->instancerLevels = 1;


    VtVec3fArray pointsVec;
    pointsVec.push_back(GfVec3f(0, 0, 0));
    pointsVec.push_back(GfVec3f(0, 1, 0));
    pointsVec.push_back(GfVec3f(0, 0, 1));
    const VtValue points = VtValue(pointsVec);
    HdBufferSourceSharedPtrVector vertexSources = {
        std::make_shared<HdVtBufferSource>(HdTokens->points, points) };
    HdBufferSpecVector bufferSpecs;
    HdBufferSpec::GetBufferSpecs(vertexSources, &bufferSpecs);
    HdBufferArrayRangeSharedPtr const vertexBar = registry->
        AllocateNonUniformBufferArrayRange(HdTokens->primvar,
                                           bufferSpecs,
                                           HdBufferArrayUsageHintBitsVertex);
    registry->AddSources(vertexBar, std::move(vertexSources));
    registry->Commit();

    std::unique_ptr<HdStDrawItem> const drawItem =
        std::make_unique<HdStDrawItem>(sharedData.get());
    sharedData->barContainer.Set(
            drawItem->GetDrawingCoord()->GetVertexPrimvarIndex(), vertexBar);

    drawItem->GetDrawingCoord()->SetInstancePrimvarBaseIndex(
        HdDrawingCoord::CustomSlotsBegin);

    const std::string geomShaderSource = ""
        "-- glslfx version 0.1                                      \n"
        "-- configuration                                           \n"
        "{                                                          \n"
        " \"techniques\": {                                         \n"
        "  \"default\": {                                           \n"
        "   \"preamble\" : {                                        \n"
        "    \"source\": [ \"Preamble\" ]                           \n" 
        "   },                                                      \n"
        "   \"vertexShader\": {                                     \n"
        "    \"source\": [ \"Vertex\" ]                             \n"
        "   },                                                      \n"
        "   \"geometryShader\": {                                   \n"    
        "    \"source\": [ ]                                        \n"
        "   },                                                      \n"
        "   \"surfaceShader\" : {                                   \n"
        "    \"source\": [ \"Surface\" ]                            \n"
        "   },                                                      \n"
        "   \"fragmentShader\": {                                   \n"
        "    \"source\": [ \"Fragment\" ]                           \n"
        "   }                                                       \n"
        "  }                                                        \n"
        " }                                                         \n"
        "}                                                          \n"
        "                                                           \n"
        "-- glsl Preamble                                           \n"
        "                                                           \n"
        "-- glsl Vertex                                             \n"
        "                                                           \n"
        "void main() {                                              \n"
        "}                                                          \n"
        "                                                           \n"
        "-- glsl Geometry                                           \n"
        "                                                           \n"
        "-- glsl Surface                                            \n"
        "                                                           \n"
        "-- glsl Fragment                                           \n"
        "                                                           \n"
        "out vec4 outColor;                                         \n"
        "                                                           \n"
        "void main() {                                              \n"
        "   outColor = vec4(1);                                     \n"
        "}                                                          \n"
        "                                                           \n";

    HdSt_GeometricShaderSharedPtr const geomShader =
        std::make_shared<HdSt_GeometricShader>(
            geomShaderSource,
            HdSt_GeometricShader::PrimitiveType::PRIM_POINTS,
            HdCullStyleDontCare,
            /*useHardwareFaceCulling*/false,
            /*hasMirroredTransform*/false,
            /*doubleSided*/false,
            /*useMetalTessellation*/false,
            HdPolygonModeFill,
            /*isFrustumCullingPass*/false,
            HdSt_GeometricShader::FvarPatchType::PATCH_NONE);
    drawItem->SetGeometricShader(geomShader);
    drawItem->SetMaterialNetworkShader(
        std::make_shared<HdStGLSLFXShader>(glslfx) );


    std::unique_ptr<HdStDrawItemInstance> const drawItemInstance =
        std::make_unique<HdStDrawItemInstance>(drawItem.get());
    std::unique_ptr<HdSt_IndirectDrawBatch> const batch =
        std::make_unique<HdSt_IndirectDrawBatch>(drawItemInstance.get());

    HdStRenderPassStateSharedPtr const passState =
        std::make_shared<HdStRenderPassState>();

    batch->PrepareDraw(nullptr, passState, registry);

    // Expect shader compilation error
    {
        TfErrorMark mark;

        batch->ExecuteDraw(nullptr, passState, registry,
            /*firstDrawBatch*/true);

        if (mark.IsClean()) {
            std::cout << "Did not get expected shader compilation error\n";
            return false;
        }

        size_t errorCount = 0;
        TfErrorMark::Iterator begin = mark.GetBegin(&errorCount);

        if (errorCount != 1) {
            std::cout << "Did not get expected number of errors\n";
            return false;
        }

        if (EXPECTED_SHADER_COMPILE_ERROR != begin->GetCommentary()) {
            std::cout << "Did not get expected error message\n";
            return false;
        }

        mark.Clear();
    }

    return true;
}

int main()
{
    GlfTestGLContext::RegisterGLContextCallbacks();
    GlfSharedGLContextScopeHolder sharedContext;

    TfErrorMark mark;
    
    const bool success = HdIndirectDrawBatchTest();

    TF_VERIFY(mark.IsClean());

    if (success && mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

