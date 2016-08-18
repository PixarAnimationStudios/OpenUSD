#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/meshTopology.h"

#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/subdivision.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/pxOsd/subdivTags.h"
#include "pxr/imaging/pxOsd/tokens.h"
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
bool
_DumpRefinedPoints(std::string const & name,
                   TfToken const & scheme,
                   TfToken const & orientation,
                   VtIntArray numVerts, VtIntArray verts,
                   VtArray<Vec3Type> points,
                   PxOsdSubdivTags const & subdivTags,
                   int refineLevel,
                   bool gpu)
{
    std::cout << "Test " << name << "\n";
    std::cout << "Scheme " << scheme << "\n";
    std::cout << "Orientation " << orientation << "\n";
    std::cout << "GPU subdivision = " << gpu << "\n";

    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();

    HdMeshTopology m(scheme, orientation, numVerts, verts, refineLevel);

    m.SetSubdivTags(subdivTags);

    HdBufferArrayRangeSharedPtr indexRange;

    // build topology and allocate index buffer
    HdBufferSpecVector bufferSpecs;
    HdBufferSourceSharedPtr topology =
        m.GetOsdTopologyComputation(SdfPath(name));
    registry->AddSource(topology);
    HdBufferSourceSharedPtr index = m.GetOsdIndexBuilderComputation();

    index->AddBufferSpecs(&bufferSpecs);
    indexRange = registry->AllocateNonUniformBufferArrayRange(
        HdTokens->topology, bufferSpecs);
    registry->AddSource(indexRange, index);

    // execute
    registry->Commit();

    // index compare
    // retrieve result
    VtValue resultValue = indexRange->ReadData(HdTokens->indices);
    if (scheme == PxOsdOpenSubdivTokens->loop) {
        if (not resultValue.IsHolding< VtArray<GfVec3i> >()) {
            std::cout << name << " test failed:\n";
            std::cout << "  wrong returned value type:\n";
            return false;
        }
    } else {
        if (not resultValue.IsHolding< VtArray<GfVec4i> >()) {
            std::cout << name << " test failed:\n";
            std::cout << "  wrong returned value type:\n";
            return false;
        }
    }

    std::cout << "Index Results\n";
    std::cout << resultValue << "\n";

    // refined points
    HdBufferSourceSharedPtr pointsSource(
        new HdVtBufferSource(HdTokens->points, VtValue(points)));

    bufferSpecs.clear();
    pointsSource->AddBufferSpecs(&bufferSpecs);

    HdBufferArrayRangeSharedPtr pointsRange =
        registry->AllocateNonUniformBufferArrayRange(HdTokens->points,
                                                         bufferSpecs);

    if (gpu) {
        // add coarse points
        registry->AddSource(pointsRange, pointsSource);

        // GPU refine computation
        HdComputationSharedPtr comp = m.GetOsdRefineComputationGPU(
            pointsSource->GetName(),
            pointsSource->GetGLComponentDataType(),
            pointsSource->GetNumComponents());

        if (comp)
            registry->AddComputation(pointsRange, comp);
    } else {
        // CPU refine computation
        HdBufferSourceSharedPtr comp =
            m.GetOsdRefineComputation(pointsSource, /*varying=*/false);
        if (comp)
            registry->AddSource(pointsRange, comp);
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

    return true;
}

#define DUMP_REFINED_POINTS(name, scheme, orientation, numVerts, verts, points, subdivTags) \
    _DumpRefinedPoints(name, scheme, orientation,                        \
               _BuildArray(numVerts, sizeof(numVerts)/sizeof(numVerts[0])), \
               _BuildArray(verts, sizeof(verts)/sizeof(verts[0])), \
               _BuildArray(points, sizeof(points)/sizeof(points[0])), \
               subdivTags, \
               /*level=*/1, false)

#define DUMP_GPU_REFINED_POINTS(name, scheme, orientation, numVerts, verts, points, subdivTags) \
    _DumpRefinedPoints(name, scheme, orientation,                        \
               _BuildArray(numVerts, sizeof(numVerts)/sizeof(numVerts[0])), \
               _BuildArray(verts, sizeof(verts)/sizeof(verts[0])), \
               _BuildArray(points, sizeof(points)/sizeof(points[0])), \
               subdivTags, \
               /*level=*/1, true)

bool
SubdivisionTest(TfToken const &scheme)
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();

    perfLog.ResetCounters();
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->subdivisionRefineCPU) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->subdivisionRefineGPU) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 0);

    {
        // 7(0)        9(2)
        // +-----4----+
        //  \    |    /
        //   \ __3__ /
        //   5       6
        //     \   /
        //      \ /
        //       +8(1)        (right handed)
        //

        int numVerts[] = { 3 };
        int verts[] = { 0, 1, 2 };
        GfVec3f points[] = {
            GfVec3f( 0.0, 1.0, 0.0 ),
            GfVec3f(-1.0, 0.0, 0.0 ),
            GfVec3f( 1.0, 0.0, 0.0 ),
        };

        if (!DUMP_REFINED_POINTS("triangle", scheme, _tokens->rightHanded,
                                    numVerts, verts, points, PxOsdSubdivTags())) {
            return false;
        }

        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->subdivisionRefineCPU) == 1);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->subdivisionRefineGPU) == 0);
        // subdivision, quadindex, points, refined points.
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 4);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 0);
        perfLog.ResetCounters();

        if (!DUMP_GPU_REFINED_POINTS("triangle", scheme, _tokens->rightHanded,
                                     numVerts, verts, points, PxOsdSubdivTags())) {
            return false;
        }

        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->subdivisionRefineCPU) == 0);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->subdivisionRefineGPU) == 1);
        // subdivision, quadindex, points
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 3);
        // refined points
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 1);
        perfLog.ResetCounters();

    }
    {
        // 7(0)        9(2)
        // +-----4----+
        //  \    |    /
        //   \ __3__ /
        //   5       6
        //     \   /
        //      \ /
        //       +8(1)        (left handed)
        //

        int numVerts[] = { 3 };
        int verts[] = { 0, 1, 2 };
        GfVec3f points[] = {
            GfVec3f( 0.0, 1.0, 0.0 ),
            GfVec3f(-1.0, 0.0, 0.0 ),
            GfVec3f( 1.0, 0.0, 0.0 ),
        };

        if (!DUMP_REFINED_POINTS("triangle", scheme, _tokens->leftHanded,
                                 numVerts, verts, points, PxOsdSubdivTags())) {
            return false;
        }

        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->subdivisionRefineCPU) == 1);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->subdivisionRefineGPU) == 0);
        // subdivision, quadindex, points, refined points.
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 4);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 0);
        perfLog.ResetCounters();

        if (!DUMP_GPU_REFINED_POINTS("triangle", scheme, _tokens->leftHanded,
                                     numVerts, verts, points, PxOsdSubdivTags())) {
            return false;
        }

        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->subdivisionRefineCPU) == 0);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->subdivisionRefineGPU) == 1);
        // subdivision, quadindex, points
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 3);
        // refined points
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 1);
        perfLog.ResetCounters();

    }
    {
        /*      9(0)-----6------12(3)
                 |       |       |
                 |       |       |
                 |       |       |
                 5-------4-------8
                 |       |       |
                 |       |       |
                 |       |       |
                10(1)----7------11(2)
         */
        int numVerts[] = { 4 };
        int verts[] = { 0, 1, 2, 3 };
        GfVec3f points[] = {
            GfVec3f( 1.0f, 1.0f, 0.0f ),
            GfVec3f(-1.0f, 1.0f, 0.0f ),
            GfVec3f(-1.0f,-1.0f, 0.0f ),
            GfVec3f( 1.0f,-1.0f, 0.0f ),
        };

        if (!DUMP_REFINED_POINTS("quad", scheme, _tokens->rightHanded,
                                 numVerts, verts, points, PxOsdSubdivTags())) {
            return false;
        }
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->subdivisionRefineCPU) == 1);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->subdivisionRefineGPU) == 0);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 4);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 0);
        perfLog.ResetCounters();

        if (!DUMP_GPU_REFINED_POINTS("quad", scheme, _tokens->rightHanded,
                                     numVerts, verts, points, PxOsdSubdivTags())) {
            return false;
        }
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->subdivisionRefineCPU) == 0);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->subdivisionRefineGPU) == 1);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 3);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 1);
        perfLog.ResetCounters();

    }
    {
        /*
              +----+----+-------+
             /|    :    |    :   \
            / |    :    |    :   .\
           /  |    :    |     . .  \
          /   + -- + -- +------+    +
         /.  .|    :    |     . .  /
        /  +  |    :    |    :   ./
       /   :  |    :    |    :   /
      +-------+----+----+-------+

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
        if (!DUMP_REFINED_POINTS("polygons", scheme, _tokens->rightHanded,
                                 numVerts, verts, points, PxOsdSubdivTags())) {
            return false;
        }
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->subdivisionRefineCPU) == 1);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->subdivisionRefineGPU) == 0);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 4);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 0);
        perfLog.ResetCounters();

        if (!DUMP_GPU_REFINED_POINTS("polygons", scheme, _tokens->rightHanded,
                                     numVerts, verts, points, PxOsdSubdivTags())) {
            return false;
        }
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->subdivisionRefineCPU) == 0);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->subdivisionRefineGPU) == 1);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 3);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 1);
        perfLog.ResetCounters();

    }
    return true;
}

bool
LoopSubdivisionTest()
{
    std::cout << "\nLoop Subdivision Test\n";

    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();

    perfLog.ResetCounters();
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->subdivisionRefineCPU) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->subdivisionRefineGPU) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 0);

    {
        // 6(0)        8(2)
        // +-----4-----+
        //  \  /  \   /
        //   \/    \ /
        //    3-----5
        //     \   /
        //      \ /
        //       +7(1)        (right handed, loop subdivision)
        //

        int numVerts[] = { 3 };
        int verts[] = { 0, 1, 2 };
        GfVec3f points[] = {
            GfVec3f( 0.0, 1.0, 0.0 ),
            GfVec3f(-1.0, 0.0, 0.0 ),
            GfVec3f( 1.0, 0.0, 0.0 ),
        };

        if (!DUMP_REFINED_POINTS("triangle", PxOsdOpenSubdivTokens->loop,
                                 _tokens->rightHanded,
                                 numVerts, verts, points, PxOsdSubdivTags())) {
            return false;
        }

        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->subdivisionRefineCPU) == 1);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->subdivisionRefineGPU) == 0);
        // subdivision, quadindex, points, refined points.
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 4);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 0);
        perfLog.ResetCounters();

        if (!DUMP_GPU_REFINED_POINTS("triangle", PxOsdOpenSubdivTokens->loop,
                                     _tokens->rightHanded,
                                     numVerts, verts, points, PxOsdSubdivTags())) {
            return false;
        }

        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->subdivisionRefineCPU) == 0);
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->subdivisionRefineGPU) == 1);
        // subdivision, quadindex, points
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 3);
        // refined points
        TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 1);
        perfLog.ResetCounters();
    }
    return true;
}

bool
PrimitiveIDMappingTest(bool usePtexIndex)
{
    std::cout << "\nPrimitiveIDMap Test\n";

        /*
          To Face Index
              +----+----+-------+
             /|    |    |    |   \
            / |  1 |  1 |  2 | 2 /\
           /  |    |    |     \ /  \
          / 0 +----+----+------+  2 +
         /\  /|    |    |     / \  /
        /  \/ |  1 |  1 |  2 | 2 \/
       / 0 | 0|    |    |    |   /
      +-------+----+----+-------+

          To Ptex Index
              +----+----+-------+
             /|    |    |    |   \
            / |  3 |  3 |  4 | 8 /\
           /  |    |    |     \ /  \
          / 0 +----+----+------+  7 +
         /\  /|    |    |     / \  /
        /  \/ |  3 |  3 |  5 | 6 \/
       / 1 | 2|    |    |    |   /
      +-------+----+----+-------+

         */
    int numVertsSrc[] = { 3, 4, 5 };
    int vertsSrc[] = { 0, 1, 2,
                       0, 2, 3, 4,
                       4, 3, 5, 6, 7 };

    VtIntArray numVerts = _BuildArray(numVertsSrc, 3);
    VtIntArray verts = _BuildArray(vertsSrc, 12);

    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();

    int refineLevel = 1;
    HdMeshTopology m(PxOsdOpenSubdivTokens->catmark, _tokens->rightHanded,
                     numVerts, verts, refineLevel);

    HdBufferArrayRangeSharedPtr indexRange;

    // build topology and allocate index buffer
    HdBufferSourceSharedPtr topology
        = m.GetOsdTopologyComputation(SdfPath("/polygons"));
    registry->AddSource(topology);

    HdBufferSourceSharedPtr index = m.GetOsdIndexBuilderComputation();

    HdBufferSpecVector bufferSpecs;
    index->AddBufferSpecs(&bufferSpecs);

    indexRange = registry->AllocateNonUniformBufferArrayRange(
        HdTokens->topology, bufferSpecs);

    registry->AddSource(indexRange, index);

    // execute
    registry->Commit();

    // retrieve result
    VtValue resultValue = indexRange->ReadData(HdTokens->primitiveParam);

    VtIntArray resultIndices;
    VtIntArray faceIndices;

    if (resultValue.IsHolding< VtVec3iArray >()) {
        VtVec3iArray result = resultValue.Get<VtVec3iArray>();
        for (int i=0; i<result.size(); ++i) {
            resultIndices.push_back(result[i][0]);
            if (usePtexIndex) {
                // stored as Far::PatchParam.field0
                faceIndices.push_back(result[i][1] & 0xfffffff);
            } else {
                faceIndices.push_back(HdMeshTopology::DecodeFaceIndexFromCoarseFaceParam(result[i][0]));
            }
        }
    } else {
        std::cout << " test failed:\n";
        std::cout << "  wrong returned value type:\n";
        return false;
    }

    std::cout << "PrimitiveParam Results\n";
    std::cout << resultIndices << "\n";
    std::cout << "Decoded map\n";
    std::cout << faceIndices;

    return true;
}

bool
SubdivTagTest()
{
    std::cout << "\nSubdiv Tag Test\n";
    /*

     0-----3-------4-----7
     |     ||      |     |
     |     || hole |     |
     |     ||       \    |
     1-----2--------[5]--6
           |        /    |
           |       |     |
           |       |     |
           8-------9----10

       =  : creased edge
       [] : corner vertex

    */

    int numVerts[] = { 4, 4, 4, 4, 4};
    int verts[] = {
        0, 1, 2, 3,
        3, 2, 5, 4,
        4, 5, 6, 7,
        2, 8, 9, 5,
        5, 9, 10, 6,
    };
    GfVec3f points[] = {
        GfVec3f(-1.0f, 0.0f,  1.0f ),
        GfVec3f(-1.0f, 0.0f,  0.0f ),
        GfVec3f(-0.5f, 0.0f,  0.0f ),
        GfVec3f(-0.5f, 0.0f,  1.0f ),
        GfVec3f( 0.0f, 0.0f,  1.0f ),
        GfVec3f( 0.5f, 0.0f,  0.0f ),
        GfVec3f( 1.0f, 0.0f,  0.0f ),
        GfVec3f( 1.0f, 0.0f,  1.0f ),
        GfVec3f(-0.5f, 0.0f, -1.0f ),
        GfVec3f( 0.0f, 0.0f, -1.0f ),
        GfVec3f( 1.0f, 0.0f, -1.0f ),
    };
    int holes[] = { 1 };
    int creaseLengths[] = { 2 };
    int creaseIndices[] = { 2, 3 };
    float creaseSharpnesses[] = { 5.0f };
    int cornerIndices[] = { 5 };
    float cornerSharpnesses[] = { 5.0f };

    PxOsdSubdivTags subdivTags;

    subdivTags.SetHoleIndices(_BuildArray(holes,
        sizeof(holes)/sizeof(holes[0])));

    subdivTags.SetCreaseLengths(_BuildArray(creaseLengths,
        sizeof(creaseLengths)/sizeof(creaseLengths[0])));
    subdivTags.SetCreaseIndices(_BuildArray(creaseIndices,
        sizeof(creaseIndices)/sizeof(creaseIndices[0])));
    subdivTags.SetCreaseWeights(_BuildArray(creaseSharpnesses,
        sizeof(creaseSharpnesses)/sizeof(creaseSharpnesses[0])));

    subdivTags.SetCornerIndices(_BuildArray(cornerIndices,
        sizeof(cornerIndices)/sizeof(cornerIndices[0])));
    subdivTags.SetCornerWeights( _BuildArray(cornerSharpnesses,
        sizeof(cornerSharpnesses)/sizeof(cornerSharpnesses[0])));

    subdivTags.SetVertexInterpolationRule(PxOsdOpenSubdivTokens->edgeOnly);
    subdivTags.SetFaceVaryingInterpolationRule(PxOsdOpenSubdivTokens->edgeOnly);


    if (!DUMP_REFINED_POINTS("subdivTag",
                             PxOsdOpenSubdivTokens->catmark, _tokens->rightHanded,
                             numVerts, verts, points, subdivTags)) {
        return false;
    }
    if (!DUMP_GPU_REFINED_POINTS("subdivTag",
                                 PxOsdOpenSubdivTokens->catmark,_tokens->rightHanded,
                                 numVerts, verts, points, subdivTags)) {
        return false;
    }
    return true;
}

bool
SubdivTagTest2()
{
    std::cout << "\nSubdiv Tag Test 2\n";

    /*
       test per-crease sharpness

     0-----3-------4-----7
     |     ||      |     |
     |     ||      |     |
     |-----||-------\----|
     1-----2---------5---6
           |        /    |
           |       |     |
           |       |     |
           8-------9----10

       =  : creased edge
    */

    int numVerts[] = { 4, 4, 4, 4, 4};
    int verts[] = {
        0, 1, 2, 3,
        3, 2, 5, 4,
        4, 5, 6, 7,
        2, 8, 9, 5,
        5, 9, 10, 6,
    };
    GfVec3f points[] = {
        GfVec3f(-1.0f, 0.0f,  1.0f ),
        GfVec3f(-1.0f, 0.0f,  0.0f ),
        GfVec3f(-0.5f, 0.0f,  0.0f ),
        GfVec3f(-0.5f, 0.0f,  1.0f ),
        GfVec3f( 0.0f, 0.0f,  1.0f ),
        GfVec3f( 0.5f, 0.0f,  0.0f ),
        GfVec3f( 1.0f, 0.0f,  0.0f ),
        GfVec3f( 1.0f, 0.0f,  1.0f ),
        GfVec3f(-0.5f, 0.0f, -1.0f ),
        GfVec3f( 0.0f, 0.0f, -1.0f ),
        GfVec3f( 1.0f, 0.0f, -1.0f ),
    };

    int creaseLengths[] = { 2, 4 };
    int creaseIndices[] = { 2, 3, 1, 2, 5, 6 };
    float creaseSharpnesses[] = { 4.0f, 5.0f };

    PxOsdSubdivTags subdivTags;

    subdivTags.SetCreaseLengths(_BuildArray(creaseLengths,
        sizeof(creaseLengths)/sizeof(creaseLengths[0])));
    subdivTags.SetCreaseIndices(_BuildArray(creaseIndices,
        sizeof(creaseIndices)/sizeof(creaseIndices[0])));
    subdivTags.SetCreaseWeights(_BuildArray(creaseSharpnesses,
        sizeof(creaseSharpnesses)/sizeof(creaseSharpnesses[0])));

    subdivTags.SetVertexInterpolationRule(PxOsdOpenSubdivTokens->edgeOnly);
    subdivTags.SetFaceVaryingInterpolationRule(PxOsdOpenSubdivTokens->edgeOnly);

    if (!DUMP_REFINED_POINTS("subdivTag",
                             PxOsdOpenSubdivTokens->catmark,
                             _tokens->rightHanded,
                             numVerts, verts, points, subdivTags)){
        return false;
    }
    if (!DUMP_GPU_REFINED_POINTS("subdivTag",
                                 PxOsdOpenSubdivTokens->catmark,
                                 _tokens->rightHanded,
                                 numVerts, verts, points, subdivTags)) {
        return false;
    }

    return true;
}

bool
InvalidTopologyTest()
{
    std::cout << "\nInvalid Topology Test\n";

    int numVerts[] = { 4, 0, 1, 2 };
    int verts[] = {
        0, 1, 2, 3,
        4,
        5, 6,
    };
    GfVec3f points[1008] = {
        GfVec3f(-1.0f, 0.0f,  1.0f ),
        GfVec3f(-1.0f, 0.0f,  0.0f ),
        GfVec3f(-0.5f, 0.0f,  0.0f ),
        GfVec3f(-0.5f, 0.0f,  1.0f ),
        GfVec3f( 0.0f, 0.0f,  1.0f ),
        GfVec3f( 0.5f, 0.0f,  0.0f ),
        GfVec3f( 1.0f, 0.0f,  0.0f ),
        GfVec3f( 1.0f, 0.0f, -1.0f ), // unused
        // 1000 of unused follow
    };
    for (int i = 8; i < 1008; ++i) {
        // initialize unused values (for baseline stability)
        points[i] = GfVec3f(i, i, i);
    }

    int creaseLengths[] = { 2, 4 };
    int creaseIndices[] = { 2, 3, 1, 2, 6, 7 };
    float creaseSharpnesses[] = { 4.0f, 5.0f };

    PxOsdSubdivTags subdivTags;

    subdivTags.SetCreaseLengths(_BuildArray(creaseLengths,
        sizeof(creaseLengths)/sizeof(creaseLengths[0])));
    subdivTags.SetCreaseIndices(_BuildArray(creaseIndices,
        sizeof(creaseIndices)/sizeof(creaseIndices[0])));
    subdivTags.SetCreaseWeights(_BuildArray(creaseSharpnesses,
        sizeof(creaseSharpnesses)/sizeof(creaseSharpnesses[0])));

    subdivTags.SetVertexInterpolationRule(PxOsdOpenSubdivTokens->edgeOnly);
    subdivTags.SetFaceVaryingInterpolationRule(PxOsdOpenSubdivTokens->edgeOnly);

    if (!DUMP_REFINED_POINTS("subdivTag",
                             PxOsdOpenSubdivTokens->catmark,
                             _tokens->rightHanded,
                             numVerts, verts, points, subdivTags)){
        return false;
    }
    if (!DUMP_GPU_REFINED_POINTS("subdivTag",
                                 PxOsdOpenSubdivTokens->catmark,
                                 _tokens->rightHanded,
                                 numVerts, verts, points, subdivTags)) {
        return false;
    }

    return true;
}

bool
EmptyTopologyTest()
{
    std::cout << "\nEmpty Topology Test\n";

    int numVerts[] = {};
    int verts[] = {};
    GfVec3f points[] = {};

    if (!DUMP_REFINED_POINTS("subdivTag",
                             PxOsdOpenSubdivTokens->catmark,
                             _tokens->rightHanded,
                             numVerts, verts, points, PxOsdSubdivTags())){
        return false;
    }
    if (!DUMP_GPU_REFINED_POINTS("subdivTag",
                                 PxOsdOpenSubdivTokens->catmark,
                                 _tokens->rightHanded,
                                 numVerts, verts, points, PxOsdSubdivTags())) {
        return false;
    }

    return true;
}

bool
TorusTopologyTest()
{
    std::cout << "\nTorus Topology Test\n";

    int numVerts[] = { 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 };

    int verts[] = {  1,  0,  4,  5,  2,  1,  5,  6,  3,  2,  6,  7,
                     0,  3,  7,  4,  5,  4,  8,  9,  6,  5,  9, 10,
                     7,  6, 10, 11,  4,  7, 11,  8,  9,  8, 12, 13,
                     10, 9, 13, 14, 11, 10, 14, 15,  8, 11, 15, 12,
                     13, 12, 0,  1, 14, 13,  1,  2, 15, 14,  2,  3,
                     12, 15, 3, 0 };

    GfVec3f points[] = {
        GfVec3f(   0,    0, -0.5),
        GfVec3f(-0.5,    0,    0),
        GfVec3f(   0,    0,  0.5),
        GfVec3f( 0.5,    0,    0),
        GfVec3f(   0,  0.5,   -1),
        GfVec3f(  -1,  0.5,    0),
        GfVec3f(   0,  0.5,    1),
        GfVec3f(   1,  0.5,    0),
        GfVec3f(   0,    0, -1.5),
        GfVec3f(-1.5,    0,    0),
        GfVec3f(   0,    0,  1.5),
        GfVec3f( 1.5,    0,    0),
        GfVec3f(   0, -0.5,   -1),
        GfVec3f(  -1, -0.5,    0),
        GfVec3f(   0, -0.5,    1),
        GfVec3f(   1, -0.5,    0)
    };

    PxOsdSubdivTags subdivTags;
    if (!DUMP_REFINED_POINTS("subdivTag",
                             PxOsdOpenSubdivTokens->catmark,
                             _tokens->rightHanded,
                             numVerts, verts, points, PxOsdSubdivTags())){
        return false;
    }
    if (!DUMP_GPU_REFINED_POINTS("subdivTag",
                                 PxOsdOpenSubdivTokens->catmark,
                                 _tokens->rightHanded,
                                 numVerts, verts, points, PxOsdSubdivTags())) {
        return false;
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
    success &= SubdivisionTest(PxOsdOpenSubdivTokens->catmark);
    // skip bilinear test until OpenSubdiv3 is updated to the latest.
    //success &= SubdivisionTest(PxOsdOpenSubdivTokens->bilinear);
    success &= LoopSubdivisionTest();
    success &= PrimitiveIDMappingTest(/*usePtexIndex=*/true);
    success &= PrimitiveIDMappingTest(/*usePtexIndex=*/false);
    success &= SubdivTagTest();
    success &= SubdivTagTest2();
    success &= InvalidTopologyTest();
    success &= EmptyTopologyTest();
    success &= TorusTopologyTest();

    TF_VERIFY(mark.IsClean());

    if (success and mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

