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

#include "pxr/imaging/hdSt/quadrangulate.h"
#include "pxr/imaging/hdSt/meshTopology.h"

#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/glf/testGLContext.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/staticTokens.h"

#include <algorithm>
#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((bilinear, "bilinear"))
    ((leftHanded, "leftHanded"))
    ((rightHanded, "rightHanded"))
);

template <typename T>
static VtArray<T>
_BuildArray(T values[], int numValues)
{
    VtArray<T> result(numValues);
    std::copy(values, values+numValues, result.begin());
    return result;
}

template <typename Vec3Type>
static bool
_CompareArrays(VtArray<Vec3Type> const & result,
               VtArray<Vec3Type> const & expected)
{
    if (result.size() != expected.size()) {
        return false;
    }
    for (size_t i=0; i<result.size(); ++i) {
        if (!GfIsClose(result[i][0], expected[i][0], 1e-6) ||
            !GfIsClose(result[i][1], expected[i][1], 1e-6) ||
            !GfIsClose(result[i][2], expected[i][2], 1e-6)) {
            return false;
        }
    }
    return true;
}

template <typename Vec3Type>
bool
_CompareQuadPoints(std::string const & name,
                   std::string const & orientation,
                   VtIntArray numVerts, VtIntArray verts,
                   VtArray<Vec3Type> points,
                   VtIntArray holes,
                   VtIntArray expectedIndices,
                   VtArray<Vec3Type> expectedPoints,
                   bool gpu)
{
    std::cout << "GPU quadrangulate = " << gpu << "\n";

    static HgiUniquePtr _hgi = Hgi::CreatePlatformDefaultHgi();
    static HdStResourceRegistrySharedPtr registry(
        new HdStResourceRegistry(_hgi.get()));

    HdMeshTopology m(_tokens->bilinear, TfToken(orientation), numVerts, verts);

    m.SetHoleIndices(holes);

    // Convert topology to render delegate version
    HdSt_MeshTopologySharedPtr rdTopology = HdSt_MeshTopology::New(m, 0);


    HdBufferArrayRangeSharedPtr indexRange;

    // build quadinfo
    HdSt_QuadInfoBuilderComputationSharedPtr quadInfoBuilder =
        rdTopology->GetQuadInfoBuilderComputation(gpu, SdfPath(name), 
            registry.get());
    registry->AddSource(quadInfoBuilder);

    // allocate index buffer
    HdBufferSpecVector bufferSpecs;
    HdBufferSourceSharedPtr quadIndex =
                      rdTopology->GetQuadIndexBuilderComputation(SdfPath(name));
    quadIndex->GetBufferSpecs(&bufferSpecs);
    indexRange = registry->AllocateNonUniformBufferArrayRange(
        HdTokens->topology, bufferSpecs, HdBufferArrayUsageHint());

    registry->AddSource(indexRange, quadIndex);

    // execute
    registry->Commit();

    // index compare
    // retrieve result
    VtValue resultValue = indexRange->ReadData(HdTokens->indices);
    if (!resultValue.IsHolding< VtIntArray >()) {
        std::cout << name << " test failed:\n";
        std::cout << "  wrong returned value type:\n";
        return false;
    }

    std::cout << "Index Results\n";
    std::cout << resultValue << "\n";

    VtIntArray result = resultValue.Get< VtIntArray >();
    if (result != expectedIndices) {
        std::cout << name << " test failed:\n";
        std::cout << "  expected: " << expectedIndices << "\n";
        std::cout << "  result: " << result << "\n";
        return false;
    }

    // quadrangulate points
    HdBufferSourceSharedPtr pointsSource(
        new HdVtBufferSource(HdTokens->points, VtValue(points)));

    std::cout << "Points\n";
    std::cout << points << "\n";

    bufferSpecs.clear();
    pointsSource->GetBufferSpecs(&bufferSpecs);

    HdBufferArrayRangeSharedPtr pointsRange =
        registry->AllocateNonUniformBufferArrayRange(HdTokens->points,
                                                     bufferSpecs,
                                                     HdBufferArrayUsageHint());

    if (gpu) {
        if (points.size() == expectedPoints.size()) {
            // all quads. GPU table has to be deallocated.
            TF_VERIFY(!rdTopology->GetQuadrangulateTableRange());
        } else {
            TF_VERIFY(rdTopology->GetQuadrangulateTableRange());
        }

        HdStComputationSharedPtr comp =
            rdTopology->GetQuadrangulateComputationGPU(
                pointsSource->GetName(),
                pointsSource->GetTupleType().type,
                SdfPath(name));
        if (comp) {
            registry->AddComputation(pointsRange, comp, HdStComputeQueueZero);
        }
        registry->AddSource(pointsRange, pointsSource);
    } else {
        HdBufferSourceSharedPtr comp =
           rdTopology->GetQuadrangulateComputation(pointsSource, SdfPath(name));
        if (comp) {
            registry->AddSource(pointsRange, comp);
        } else {
            // all-quads
            registry->AddSource(pointsRange, pointsSource);
        }
    }

    registry->Commit();

    // retrieve result
    VtValue ptResultValue = pointsRange->ReadData(HdTokens->points);
    if (!ptResultValue.IsHolding< VtArray<Vec3Type> >()) {
        std::cout << name << " test failed:\n";
        std::cout << "  wrong returned value type:\n";
        return false;
    }

    std::cout << "Results\n";
    std::cout << ptResultValue << "\n";

    VtArray<Vec3Type> ptResult = ptResultValue.Get< VtArray<Vec3Type> >();
    if (!_CompareArrays(ptResult, expectedPoints)) {
        std::cout << name << " test failed:\n";
        std::cout << "  expected: " << expectedPoints << "\n";
        std::cout << "  result: " << ptResult << "\n";
        return false;
    }
    return true;
}

#define COMPARE_QUAD_POINTS(name, orientation, numVerts, verts, points, expectedIndices, expectedPoints) \
    _CompareQuadPoints(name, orientation, \
               _BuildArray(numVerts, sizeof(numVerts)/sizeof(numVerts[0])), \
               _BuildArray(verts, sizeof(verts)/sizeof(verts[0])), \
               _BuildArray(points, sizeof(points)/sizeof(points[0])), \
               /*holes=*/VtIntArray(),                          \
               _BuildArray(expectedIndices, sizeof(expectedIndices)/sizeof(expectedIndices[0])), \
               _BuildArray(expectedPoints, sizeof(expectedPoints)/sizeof(expectedPoints[0])), \
               false)

#define COMPARE_GPU_QUAD_POINTS(name, orientation, numVerts, verts, points, expectedIndices, expectedPoints) \
    _CompareQuadPoints(name, orientation, \
               _BuildArray(numVerts, sizeof(numVerts)/sizeof(numVerts[0])), \
               _BuildArray(verts, sizeof(verts)/sizeof(verts[0])), \
               _BuildArray(points, sizeof(points)/sizeof(points[0])), \
               /*holes=*/VtIntArray(),                          \
               _BuildArray(expectedIndices, sizeof(expectedIndices)/sizeof(expectedIndices[0])), \
               _BuildArray(expectedPoints, sizeof(expectedPoints)/sizeof(expectedPoints[0])), \
               true)

#define COMPARE_QUAD_POINTS_HOLE(name, orientation, numVerts, verts, points, holes, expectedIndices, expectedPoints) \
    _CompareQuadPoints(name, orientation, \
               _BuildArray(numVerts, sizeof(numVerts)/sizeof(numVerts[0])), \
               _BuildArray(verts, sizeof(verts)/sizeof(verts[0])), \
               _BuildArray(points, sizeof(points)/sizeof(points[0])), \
               _BuildArray(holes, sizeof(holes)/sizeof(holes[0])), \
               _BuildArray(expectedIndices, sizeof(expectedIndices)/sizeof(expectedIndices[0])), \
               _BuildArray(expectedPoints, sizeof(expectedPoints)/sizeof(expectedPoints[0])), \
               false)

#define COMPARE_GPU_QUAD_POINTS_HOLE(name, orientation, numVerts, verts, points, holes, expectedIndices, expectedPoints) \
    _CompareQuadPoints(name, orientation, \
               _BuildArray(numVerts, sizeof(numVerts)/sizeof(numVerts[0])), \
               _BuildArray(verts, sizeof(verts)/sizeof(verts[0])), \
               _BuildArray(points, sizeof(points)/sizeof(points[0])), \
               _BuildArray(holes, sizeof(holes)/sizeof(holes[0])), \
               _BuildArray(expectedIndices, sizeof(expectedIndices)/sizeof(expectedIndices[0])), \
               _BuildArray(expectedPoints, sizeof(expectedPoints)/sizeof(expectedPoints[0])), \
               true)

bool
QuadrangulationTest()
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();

    perfLog.ResetCounters();
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateCPU) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateGPU) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulatedVerts) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 0);

    {
        // 0            2
        // +-----5----+
        //  \    |    /
        //   \ __6__ /
        //   3      4
        //     \   /
        //      \ /
        //       + 1         (right handed)
        //

        int numVerts[] = { 3 };
        int verts[] = { 0, 1, 2 };
        GfVec3f points[] = {
            GfVec3f( 0.0, 1.0, 0.0 ),
            GfVec3f(-1.0, 0.0, 0.0 ),
            GfVec3f( 1.0, 0.0, 0.0 ),
        };
        GfVec3f expectedPoints[] = {
            GfVec3f( 0.0, 1.0, 0.0 ),
            GfVec3f(-1.0, 0.0, 0.0 ),
            GfVec3f( 1.0, 0.0, 0.0 ),
            GfVec3f(-0.5, 0.5, 0.0 ),
            GfVec3f( 0.0, 0.0, 0.0 ),
            GfVec3f( 0.5, 0.5, 0.0 ),
            GfVec3f( 0.0, 1.0/3.0, 0.0 ),
        };
        int expectedIndices[] = {
            0, 3, 6, 5,
            1, 4, 6, 3,
            2, 5, 6, 4
        };

        if (!COMPARE_QUAD_POINTS("triangle", _tokens->rightHanded,
                numVerts, verts, points, expectedIndices, expectedPoints)) {
            return false;
        }

        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateCPU) == 1);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateGPU) == 0);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulatedVerts) == 4);
        // quadinfo, quadindex, points, quadrangulated points.
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 4);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 0);
        perfLog.ResetCounters();

        if (!COMPARE_GPU_QUAD_POINTS("triangle", _tokens->rightHanded,
                numVerts, verts, points, expectedIndices, expectedPoints)) {
            return false;
        }

        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateCPU) == 0);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateGPU) == 1);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulatedVerts) == 4);
        // quadinfo, quadindex, points, quad tables.
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 4);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 1);
        perfLog.ResetCounters();

    }
    {
        // 0            2
        // +-----5----+
        //  \    |    /
        //   \ __6__ /
        //   3      4
        //     \   /
        //      \ /
        //       + 1         (left handed)
        //

        int numVerts[] = { 3 };
        int verts[] = { 0, 1, 2 };
        GfVec3f points[] = {
            GfVec3f( 0.0, 1.0, 0.0 ),
            GfVec3f(-1.0, 0.0, 0.0 ),
            GfVec3f( 1.0, 0.0, 0.0 ),
        };
        GfVec3f expectedPoints[] = {
            GfVec3f( 0.0, 1.0, 0.0 ),
            GfVec3f(-1.0, 0.0, 0.0 ),
            GfVec3f( 1.0, 0.0, 0.0 ),
            GfVec3f(-0.5, 0.5, 0.0 ),
            GfVec3f( 0.0, 0.0, 0.0 ),
            GfVec3f( 0.5, 0.5, 0.0 ),
            GfVec3f( 0.0, 1.0/3.0, 0.0 ),
        };
        int expectedIndices[] = {
            0, 5, 6, 3,
            1, 3, 6, 4,
            2, 4, 6, 5
        };

        if (!COMPARE_QUAD_POINTS("triangle", _tokens->leftHanded,
                numVerts, verts, points, expectedIndices, expectedPoints)) {
            return false;
        }

        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateCPU) == 1);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateGPU) == 0);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulatedVerts) == 4);
        // quadinfo, quadindex, points, quadrangulated points.
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 4);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 0);
        perfLog.ResetCounters();

        if (!COMPARE_GPU_QUAD_POINTS("triangle", _tokens->leftHanded,
                numVerts, verts, points, expectedIndices, expectedPoints)) {
            return false;
        }

        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateCPU) == 0);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateGPU) == 1);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulatedVerts) == 4);
        // quadinfo, quadindex, points, quad tables.
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 4);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 1);
        perfLog.ResetCounters();

    }
    {
        int numVerts[] = { 4 };
        int verts[] = { 0, 1, 2, 3 };
        GfVec3f points[] = {
            GfVec3f( 1.0f, 1.0f, 0.0f ),
            GfVec3f(-1.0f, 1.0f, 0.0f ),
            GfVec3f(-1.0f,-1.0f, 0.0f ),
            GfVec3f( 1.0f,-1.0f, 0.0f ),
        };
        GfVec3f expectedPoints[] = {
            GfVec3f( 1.0f, 1.0f, 0.0f ),
            GfVec3f(-1.0f, 1.0f, 0.0f ),
            GfVec3f(-1.0f,-1.0f, 0.0f ),
            GfVec3f( 1.0f,-1.0f, 0.0f ),
        };
        int expectedIndices[] = {
            0, 1, 2, 3
        };

        if (!COMPARE_QUAD_POINTS("quad", _tokens->rightHanded,
                numVerts, verts, points, expectedIndices, expectedPoints)) {
            return false;
        }
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateCPU) == 0);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateGPU) == 0);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulatedVerts) == 0);
        // quadinfo, quadindex, points
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 3);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 0);
        perfLog.ResetCounters();

        if (!COMPARE_GPU_QUAD_POINTS("quad", _tokens->rightHanded,
                numVerts, verts, points, expectedIndices, expectedPoints)) {
            return false;
        }
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateCPU) == 0);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateGPU) == 0);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulatedVerts) == 0);
        // quadinfo, quadindex, points, quad tables
        // (quad table will be empty but still the buffer source has to resolved.)
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 4);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 0);
        perfLog.ResetCounters();

    }
    {
        /*       0--------4---16--7
                /|        |       |
               / |        |       15
              /  |        |       |
             8   10      12   17  6
            / 11 |        |       |
           /     |        |       14
          /      |        |       |
         1---9---2--------3---13--5

         */
        int numVerts[] = { 3, 4, 5 };
        int verts[] = { 0, 1, 2,
                        0, 2, 3, 4,
                        4, 3, 5, 6, 7 };
        GfVec3f points[] = {
            GfVec3f( 1.0f, 1.0f, 0.0f),
            GfVec3f( 0.0f, 0.0f, 0.0f),
            GfVec3f( 1.0f, 0.0f, 0.0f),
            GfVec3f( 2.0f, 0.0f, 0.0f),
            GfVec3f( 2.0f, 1.0f, 0.0f),
            GfVec3f( 3.0f, 0.0f, 0.0f),
            GfVec3f( 3.0f, 0.5f, 0.0f),
            GfVec3f( 3.0f, 1.0f, 0.0f),
        };
        GfVec3f expectedPoints[] = {
            GfVec3f( 1.0f, 1.0f, 0.0f),
            GfVec3f( 0.0f, 0.0f, 0.0f),
            GfVec3f( 1.0f, 0.0f, 0.0f),
            GfVec3f( 2.0f, 0.0f, 0.0f),
            GfVec3f( 2.0f, 1.0f, 0.0f),
            GfVec3f( 3.0f, 0.0f, 0.0f),
            GfVec3f( 3.0f, 0.5f, 0.0f),
            GfVec3f( 3.0f, 1.0f, 0.0f),
            GfVec3f( 0.5f, 0.5f, 0.0f),
            GfVec3f( 0.5f, 0.0f, 0.0f),
            GfVec3f( 1.0f, 0.5f, 0.0f),
            GfVec3f(0.666667f, 0.333333f, 0.0f),
            GfVec3f( 2.0f, 0.5f, 0.0f),
            GfVec3f( 2.5f, 0.0f, 0.0f),
            GfVec3f( 3.0f, 0.25f, 0.0f),
            GfVec3f( 3.0f, 0.75f, 0.0f),
            GfVec3f( 2.5f, 1.0f, 0.0f),
            GfVec3f( 2.6f, 0.5f, 0.0f),
        };
        int expectedIndices[] = {
            0, 8, 11, 10,
            1, 9, 11, 8,
            2, 10, 11, 9,
            0, 2, 3, 4,
            4, 12, 17, 16,
            3, 13, 17, 12,
            5, 14, 17, 13,
            6, 15, 17, 14,
            7, 16, 17, 15
        };
        if (!COMPARE_QUAD_POINTS("quad", _tokens->rightHanded,
                numVerts, verts, points, expectedIndices, expectedPoints)) {
            return false;
        }
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateCPU) == 1);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateGPU) == 0);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulatedVerts) == 10);
        // quadinfo, quadindex, points, quadrangulated points.
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 4);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 0);
        perfLog.ResetCounters();

        if (!COMPARE_GPU_QUAD_POINTS("quad", _tokens->rightHanded,
                numVerts, verts, points, expectedIndices, expectedPoints)) {
            return false;
        }
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateCPU) == 0);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateGPU) == 1);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulatedVerts) == 10);
        // quadinfo, quadindex, points, quad tables.
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 4);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 1);
        perfLog.ResetCounters();

    }
    {
        /*       0--------4---16--7
                /|        |       |
               / |        |       15
              /  |        |       |
             8   10 hole  12   17  6
            / 11 |        |       |
           /     |        |       14
          /      |        |       |
         1---9---2--------3---13--5

         */
        int numVerts[] = { 3, 4, 5 };
        int verts[] = { 0, 1, 2,
                        0, 2, 3, 4,
                        4, 3, 5, 6, 7 };
        int holes[] = { 1 };
        GfVec3f points[] = {
            GfVec3f( 1.0f, 1.0f, 0.0f),
            GfVec3f( 0.0f, 0.0f, 0.0f),
            GfVec3f( 1.0f, 0.0f, 0.0f),
            GfVec3f( 2.0f, 0.0f, 0.0f),
            GfVec3f( 2.0f, 1.0f, 0.0f),
            GfVec3f( 3.0f, 0.0f, 0.0f),
            GfVec3f( 3.0f, 0.5f, 0.0f),
            GfVec3f( 3.0f, 1.0f, 0.0f),
        };
        GfVec3f expectedPoints[] = {
            GfVec3f( 1.0f, 1.0f, 0.0f),
            GfVec3f( 0.0f, 0.0f, 0.0f),
            GfVec3f( 1.0f, 0.0f, 0.0f),
            GfVec3f( 2.0f, 0.0f, 0.0f),
            GfVec3f( 2.0f, 1.0f, 0.0f),
            GfVec3f( 3.0f, 0.0f, 0.0f),
            GfVec3f( 3.0f, 0.5f, 0.0f),
            GfVec3f( 3.0f, 1.0f, 0.0f),
            GfVec3f( 0.5f, 0.5f, 0.0f),
            GfVec3f( 0.5f, 0.0f, 0.0f),
            GfVec3f( 1.0f, 0.5f, 0.0f),
            GfVec3f(0.666667f, 0.333333f, 0.0f),
            GfVec3f( 2.0f, 0.5f, 0.0f),
            GfVec3f( 2.5f, 0.0f, 0.0f),
            GfVec3f( 3.0f, 0.25f, 0.0f),
            GfVec3f( 3.0f, 0.75f, 0.0f),
            GfVec3f( 2.5f, 1.0f, 0.0f),
            GfVec3f( 2.6f, 0.5f, 0.0f),
        };
        int expectedIndices[] = {
            0, 8, 11, 10,
            1, 9, 11, 8,
            2, 10, 11, 9,

            4, 12, 17, 16,
            3, 13, 17, 12,
            5, 14, 17, 13,
            6, 15, 17, 14,
            7, 16, 17, 15
        };

        if (!COMPARE_QUAD_POINTS_HOLE("quad", _tokens->rightHanded,
            numVerts, verts, points, holes, expectedIndices, expectedPoints)) {
            return false;
        }
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateCPU) == 1);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateGPU) == 0);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulatedVerts) == 10);
        // quadinfo, quadindex, points, quadrangulated points.
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 4);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 0);
        perfLog.ResetCounters();

        if (!COMPARE_GPU_QUAD_POINTS_HOLE("quad", _tokens->rightHanded,
            numVerts, verts, points, holes, expectedIndices, expectedPoints)) {
            return false;
        }
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateCPU) == 0);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateGPU) == 1);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulatedVerts) == 10);
        // quadinfo, quadindex, points, quad tables.
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 4);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 1);
        perfLog.ResetCounters();

    }
    return true;
}

bool
QuadrangulationInvalidTopologyTest()
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();

    perfLog.ResetCounters();
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateCPU) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateGPU) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulatedVerts) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 0);

    {
        /*       0--------4---16--7
                /|        |       |
               / |        |       15
              /  |        |       |
             8   10      12   17  6
            / 11 |        |       |
           /     |        |       14
          /      |        |       |
         1---9---2--------3---13--5

         */
        int numVerts[] = { 3, 4, 5 };
        int verts[] = { 0, 1, 2,
                        0, 2, 3, 4,
                        //4, 3, 5, 6, 7 // missing
        };
        GfVec3f points[] = {
            GfVec3f( 1.0f, 1.0f, 0.0f),
            GfVec3f( 0.0f, 0.0f, 0.0f),
            GfVec3f( 1.0f, 0.0f, 0.0f),
            GfVec3f( 2.0f, 0.0f, 0.0f),
            GfVec3f( 2.0f, 1.0f, 0.0f),
            GfVec3f( 3.0f, 0.0f, 0.0f),
            GfVec3f( 3.0f, 0.5f, 0.0f),
            GfVec3f( 3.0f, 1.0f, 0.0f),
        };
        GfVec3f expectedPoints[] = {
            GfVec3f( 1.0f, 1.0f, 0.0f),
            GfVec3f( 0.0f, 0.0f, 0.0f),
            GfVec3f( 1.0f, 0.0f, 0.0f),
            GfVec3f( 2.0f, 0.0f, 0.0f),
            GfVec3f( 2.0f, 1.0f, 0.0f),
            //GfVec3f( 3.0f, 0.0f, 0.0f), // 5, missing
            //GfVec3f( 3.0f, 0.5f, 0.0f), // 6, missing
            //GfVec3f( 3.0f, 1.0f, 0.0f), // 7, missing
            GfVec3f( 0.5f, 0.5f, 0.0f),
            GfVec3f( 0.5f, 0.0f, 0.0f),
            GfVec3f( 1.0f, 0.5f, 0.0f),
            GfVec3f(0.666667f, 0.333333f, 0.0f),
            GfVec3f( 1.0f, 1.0f, 0.0f), //=[0], GfVec3f( 2.0f, 0.5f, 0.0f), missing
            GfVec3f( 1.0f, 1.0f, 0.0f), //=[0], GfVec3f( 2.5f, 0.0f, 0.0f), missing
            GfVec3f( 1.0f, 1.0f, 0.0f), //=[0], GfVec3f( 3.0f, 0.25f, 0.0f), missing
            GfVec3f( 1.0f, 1.0f, 0.0f), //=[0], GfVec3f( 3.0f, 0.75f, 0.0f), missing
            GfVec3f( 1.0f, 1.0f, 0.0f), //=[0], GfVec3f( 2.5f, 1.0f, 0.0f), missing
            GfVec3f( 1.0f, 1.0f, 0.0f), //=[0], GfVec3f( 2.6f, 0.5f, 0.0f), missing
        };
        int expectedIndices[] = {
            0, 5, 8, 7,
            1, 6, 8, 5,
            2, 7, 8, 6,
            0, 2, 3, 4,
            0, 0, 0, 0, // missing
            0, 0, 0, 0, // missing
            0, 0, 0, 0, // missing
            0, 0, 0, 0, // missing
            0, 0, 0, 0  // missing
        };
        if (!COMPARE_QUAD_POINTS("quad", _tokens->rightHanded,
                numVerts, verts, points, expectedIndices, expectedPoints)) {
            return false;
        }
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateCPU) == 1);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateGPU) == 0);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulatedVerts) == 10);
        // quadinfo, quadindex, points, quadrangulated points.
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 4);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 0);
        perfLog.ResetCounters();

        if (!COMPARE_GPU_QUAD_POINTS("quad", _tokens->rightHanded,
                numVerts, verts, points, expectedIndices, expectedPoints)) {
            return false;
        }
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateCPU) == 0);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulateGPU) == 1);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->quadrangulatedVerts) == 10);
        // quadinfo, quadindex, points, quad tables.
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 4);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 1);
        perfLog.ResetCounters();
    }
    return true;
}


int main()
{
    GlfTestGLContext::RegisterGLContextCallbacks();
    GlfSharedGLContextScopeHolder sharedContext;

    TfErrorMark mark;

    bool success = true;
    success &= QuadrangulationTest();
    success &= QuadrangulationInvalidTopologyTest();

    TF_VERIFY(mark.IsClean());

    if (success && mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

