#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/meshTopology.h"

#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/quadrangulate.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/testGLContext.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/staticTokens.h"

#include <algorithm>
#include <iostream>

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
    for (int i=0; i<result.size(); ++i) {
        if (not GfIsClose(result[i][0], expected[i][0], 1e-6) or
            not GfIsClose(result[i][1], expected[i][1], 1e-6) or
            not GfIsClose(result[i][2], expected[i][2], 1e-6)) {
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
                   VtArray<GfVec4i> expectedIndices,
                   VtArray<Vec3Type> expectedPoints,
                   bool gpu)
{
    std::cout << "GPU quadrangulate = " << gpu << "\n";

    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();

    HdMeshTopology m(_tokens->bilinear, TfToken(orientation), numVerts, verts);

    m.SetHoleIndices(holes);

    HdBufferArrayRangeSharedPtr indexRange;

    // build quadinfo
    Hd_QuadInfoBuilderComputationSharedPtr quadInfoBuilder =
        m.GetQuadInfoBuilderComputation(gpu, SdfPath(name), registry);
    registry->AddSource(quadInfoBuilder);

    // allocate index buffer
    HdBufferSpecVector bufferSpecs;
    HdBufferSourceSharedPtr quadIndex =
        m.GetQuadIndexBuilderComputation(SdfPath(name));
    quadIndex->AddBufferSpecs(&bufferSpecs);
    indexRange = registry->AllocateNonUniformBufferArrayRange(
        HdTokens->topology, bufferSpecs);

    registry->AddSource(indexRange, quadIndex);

    // execute
    registry->Commit();

    // index compare
    // retrieve result
    VtValue resultValue = indexRange->ReadData(HdTokens->indices);
    if (not resultValue.IsHolding< VtArray<GfVec4i> >()) {
        std::cout << name << " test failed:\n";
        std::cout << "  wrong returned value type:\n";
        return false;
    }

    std::cout << "Index Results\n";
    std::cout << resultValue << "\n";

    VtArray<GfVec4i> result = resultValue.Get< VtArray<GfVec4i> >();
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
    pointsSource->AddBufferSpecs(&bufferSpecs);

    HdBufferArrayRangeSharedPtr pointsRange =
        registry->AllocateNonUniformBufferArrayRange(HdTokens->points,
                                                         bufferSpecs);

    if (gpu) {
        if (points.size() == expectedPoints.size()) {
            // all quads. GPU table has to be deallocated.
            TF_VERIFY(not m.GetQuadrangulateTableRange());
        } else {
            TF_VERIFY(m.GetQuadrangulateTableRange());
        }

        HdComputationSharedPtr comp = m.GetQuadrangulateComputationGPU(
            pointsSource->GetName(), pointsSource->GetGLComponentDataType(),
            SdfPath(name));
        if (comp) {
            registry->AddComputation(pointsRange, comp);
        }
        registry->AddSource(pointsRange, pointsSource);
    } else {
        HdBufferSourceSharedPtr comp =
            m.GetQuadrangulateComputation(pointsSource, SdfPath(name));
        if (comp) {
            registry->AddSource(pointsSource);
            registry->AddSource(pointsRange, comp);
        } else {
            // all-quads
            registry->AddSource(pointsRange, pointsSource);
        }
    }

    registry->Commit();

    // retrieve result
    VtValue ptResultValue = pointsRange->ReadData(HdTokens->points);
    if (not ptResultValue.IsHolding< VtArray<Vec3Type> >()) {
        std::cout << name << " test failed:\n";
        std::cout << "  wrong returned value type:\n";
        return false;
    }

    std::cout << "Results\n";
    std::cout << ptResultValue << "\n";

    VtArray<Vec3Type> ptResult = ptResultValue.Get< VtArray<Vec3Type> >();
    if (not _CompareArrays(ptResult, expectedPoints)) {
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
        GfVec4i expectedIndices[] = {
            GfVec4i(0, 3, 6, 5),
            GfVec4i(1, 4, 6, 3),
            GfVec4i(2, 5, 6, 4)
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
        GfVec4i expectedIndices[] = {
            GfVec4i(0, 5, 6, 3),
            GfVec4i(1, 3, 6, 4),
            GfVec4i(2, 4, 6, 5)
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
        GfVec4i expectedIndices[] = {
            GfVec4i(0, 1, 2, 3)
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
        GfVec4i expectedIndices[] = {
            GfVec4i( 0, 8, 11, 10),
            GfVec4i( 1, 9, 11, 8),
            GfVec4i( 2, 10, 11, 9),
            GfVec4i( 0, 2, 3, 4),
            GfVec4i( 4, 12, 17, 16),
            GfVec4i( 3, 13, 17, 12),
            GfVec4i( 5, 14, 17, 13),
            GfVec4i( 6, 15, 17, 14),
            GfVec4i( 7, 16, 17, 15)
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
        GfVec4i expectedIndices[] = {
            GfVec4i(0, 8, 11, 10),
            GfVec4i(1, 9, 11, 8),
            GfVec4i(2, 10, 11, 9),
            GfVec4i(0, 12, 17, 16),
            GfVec4i(2, 13, 17, 12),
            GfVec4i(3, 14, 17, 13),
            GfVec4i(4, 15, 17, 14),
            GfVec4i(4, 16, 17, 15)
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
        GfVec4i expectedIndices[] = {
            GfVec4i( 0, 5, 8, 7),
            GfVec4i( 1, 6, 8, 5),
            GfVec4i( 2, 7, 8, 6),
            GfVec4i( 0, 2, 3, 4),
            GfVec4i( 0, 0, 0, 0), // missing
            GfVec4i( 0, 0, 0, 0), // missing
            GfVec4i( 0, 0, 0, 0), // missing
            GfVec4i( 0, 0, 0, 0), // missing
            GfVec4i( 0, 0, 0, 0) // missing
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
    GlfGlewInit();
    GlfSharedGLContextScopeHolder sharedContext;

    TfErrorMark mark;

    bool success = true;
    success &= QuadrangulationTest();
    success &= QuadrangulationInvalidTopologyTest();

    TF_VERIFY(mark.IsClean());

    if (success and mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

