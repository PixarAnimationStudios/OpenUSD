#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/meshTopology.h"

#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/smoothNormals.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/vertexAdjacency.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/testGLContext.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3i.h"
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

static bool
_CompareIndices(std::string const & name,
                std::string const & orientation,
                VtIntArray numVerts, VtIntArray verts, VtIntArray holes,
                VtVec3iArray expected)
{
    HdMeshTopology m(_tokens->bilinear, TfToken(orientation), numVerts, verts);

    m.SetHoleIndices(holes);

    // compute triangle indices
    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();
    HdBufferSourceSharedPtr source = m.GetTriangleIndexBuilderComputation(
        SdfPath(name));
    HdBufferSpecVector bufferSpecs;
    source->AddBufferSpecs(&bufferSpecs);
    HdBufferArrayRangeSharedPtr range =
        registry->AllocateNonUniformBufferArrayRange(HdTokens->topology,
                                                     bufferSpecs);
    registry->AddSource(range, source);

    // execute computation
    registry->Commit();

    VtVec3iArray result = range->ReadData(HdTokens->indices).Get<VtVec3iArray>();
    if (result != expected) {
        std::cout << name << " test failed:\n";
        std::cout << "  expected: " << expected << "\n";
        std::cout << "  result: " << result << "\n";
        return false;
    }
    return true;
}

static bool
_CompareFaceVarying(std::string const &name,
                    std::string const &orientation,
                    VtIntArray numVerts, VtIntArray verts, VtIntArray holes,
                    VtFloatArray fvarValues,
                    VtFloatArray expected)
{
    HdMeshTopology m(_tokens->bilinear, TfToken(orientation), numVerts, verts);

    m.SetHoleIndices(holes);

    // compute triangulated fvar values
    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();
    HdBufferSourceSharedPtr fvarSource(
        new HdVtBufferSource(HdTokens->primVar, VtValue(fvarValues)));
    registry->AddSource(fvarSource);
    HdBufferSourceSharedPtr source
        = m.GetTriangulateFaceVaryingComputation(fvarSource, SdfPath(name));
    HdBufferSpecVector bufferSpecs;
    source->AddBufferSpecs(&bufferSpecs);
    HdBufferArrayRangeSharedPtr range =
        registry->AllocateNonUniformBufferArrayRange(HdTokens->primVar,
                                                     bufferSpecs);
    registry->AddSource(range, source);

    // execute computation
    registry->Commit();

    VtFloatArray result = range->ReadData(HdTokens->primVar).Get<VtFloatArray>();
    if (result != expected) {
        std::cout << name << " test failed:\n";
        std::cout << "  expected: " << expected << "\n";
        std::cout << "  result: " << result << "\n";
        return false;
    }
    return true;
}

#define COMPARE_INDICES(name, orientation, numVerts, verts, expected) \
    _CompareIndices(name, orientation, \
                   _BuildArray(numVerts, sizeof(numVerts)/sizeof(int)), \
                   _BuildArray(verts, sizeof(verts)/sizeof(int)), \
                    /*holes=*/VtIntArray(),                             \
                   _BuildArray(expected, sizeof(expected)/sizeof(expected[0])))

#define COMPARE_INDICES_HOLE(name, orientation, numVerts, verts, holes, expected) \
    _CompareIndices(name, orientation,                                  \
                    _BuildArray(numVerts, sizeof(numVerts)/sizeof(int)), \
                    _BuildArray(verts, sizeof(verts)/sizeof(int)),      \
                    _BuildArray(holes, sizeof(holes)/sizeof(int)),      \
                    _BuildArray(expected, sizeof(expected)/sizeof(expected[0])))

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
_CompareSmoothNormals(std::string const & name,
                      std::string const & orientation,
                      VtIntArray numVerts, VtIntArray verts,
                      VtArray<Vec3Type> points,
                      VtArray<Vec3Type> expectedNormals)
{
    HdMeshTopology topology(_tokens->bilinear, TfToken(orientation), numVerts, verts);
    Hd_VertexAdjacency adjacency;
    HdBufferSourceSharedPtr pointsSource(
        new HdVtBufferSource(HdTokens->points, VtValue(points)));

    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();

    // Adjacency computation
    {
        HdBufferSourceSharedPtr adjComputation =
            adjacency.GetAdjacencyBuilderComputation(&topology);
        registry->AddSource(adjComputation);
        registry->Commit();
    }

    int numPoints = pointsSource->GetSize();
    VtValue resultValue = VtValue(adjacency.ComputeSmoothNormals(
        numPoints, static_cast<const Vec3Type*>(pointsSource->GetData())));

    if (not resultValue.IsHolding< VtArray<Vec3Type> >()) {
        std::cout << name << " test failed:\n";
        std::cout << "  wrong returned value type:\n";
        return false;
    }
    VtArray<Vec3Type> result = resultValue.Get< VtArray<Vec3Type> >();
    if (not _CompareArrays(result, expectedNormals)) {
        std::cout << name << " test failed:\n";
        std::cout << "  expected: " << expectedNormals << "\n";
        std::cout << "  result: " << result << "\n";
        return false;
    }
    return true;
}

#define COMPARE_SMOOTH_NORMALS(name, orientation, numVerts, verts, points, expected) \
    _CompareSmoothNormals(name, orientation, \
               _BuildArray(numVerts, sizeof(numVerts)/sizeof(numVerts[0])), \
               _BuildArray(verts, sizeof(verts)/sizeof(verts[0])), \
               _BuildArray(points, sizeof(points)/sizeof(points[0])), \
               _BuildArray(expected, sizeof(expected)/sizeof(expected[0])))

template <typename Vec3Type>
bool
_CompareGpuSmoothNormals(std::string const & name,
                         std::string const & orientation,
                         VtIntArray numVerts, VtIntArray verts,
                         VtArray<Vec3Type> points,
                         VtArray<Vec3Type> expectedNormals)
{
    HdMeshTopology topology(_tokens->bilinear, TfToken(orientation), numVerts, verts);
    Hd_VertexAdjacency adjacency;

    HdBufferSourceSharedPtr pointsSource(
        new HdVtBufferSource(HdTokens->points, VtValue(points)));

    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();

    // Adjacency computation
    {
        HdBufferSourceSharedPtr adjComputation =
            adjacency.GetAdjacencyBuilderComputation(&topology);
        registry->AddSource(adjComputation);

        HdBufferSourceSharedPtr adjGpuComputation =
            adjacency.GetAdjacencyBuilderForGPUComputation();
        HdBufferSpecVector bufferSpecs;
        adjGpuComputation->AddBufferSpecs(&bufferSpecs);
        HdBufferArrayRangeSharedPtr adjRange = registry->AllocateNonUniformBufferArrayRange(
            HdTokens->topology, bufferSpecs);
        adjacency.SetAdjacencyRange(adjRange);
        registry->AddSource(adjRange, adjGpuComputation);
    }

    // GPU smooth computation
    HdComputationSharedPtr normalComputation = adjacency.GetSmoothNormalsComputationGPU(
        HdTokens->points, HdTokens->normals, pointsSource->GetGLComponentDataType());

    // build bufferspec
    HdBufferSpecVector bufferSpecs;
    bufferSpecs.push_back(HdBufferSpec(pointsSource->GetName(),
                                       pointsSource->GetGLComponentDataType(),
                                       pointsSource->GetNumComponents()));
    normalComputation->AddBufferSpecs(&bufferSpecs);

    // allocate GPU buffer range
    HdBufferArrayRangeSharedPtr range =
        registry->AllocateNonUniformBufferArrayRange(HdTokens->primVar,
                                                     bufferSpecs);

    // commit points
    HdBufferSourceVector sources;
    sources.push_back(pointsSource);
    registry->AddSources(range, sources);
    registry->AddComputation(range, normalComputation);

    // commit & execute
    registry->Commit();

    // retrieve result
    VtValue resultValue = range->ReadData(HdTokens->normals);
    if (not resultValue.IsHolding< VtArray<Vec3Type> >()) {
        std::cout << name << " test failed:\n";
        std::cout << "  wrong returned value type:\n";
        return false;
    }
    VtArray<Vec3Type> result = resultValue.Get< VtArray<Vec3Type> >();
    if (not _CompareArrays(result, expectedNormals)) {
        std::cout << name << " test failed:\n";
        std::cout << "  expected: " << expectedNormals << "\n";
        std::cout << "  result: " << result << "\n";
        return false;
    }
    return true;
}

#define COMPARE_GPU_SMOOTH_NORMALS(name, orientation, numVerts, verts, points, expected) \
    _CompareGpuSmoothNormals(name, orientation, \
               _BuildArray(numVerts, sizeof(numVerts)/sizeof(numVerts[0])), \
               _BuildArray(verts, sizeof(verts)/sizeof(verts[0])), \
               _BuildArray(points, sizeof(points)/sizeof(points[0])), \
               _BuildArray(expected, sizeof(expected)/sizeof(expected[0])))



bool
BasicTest()
{
    {
        int numVerts[] = {};
        int verts[] = {};
        GfVec3i expected[] = { };
        if (!COMPARE_INDICES("empty", _tokens->rightHanded, numVerts, verts, expected)) {
            return false;
        }
    }
    {
        // This mesh intentionally has one degenerate face with no verts.
        int numVerts[] = { 0, 3 };
        int verts[] = { 1 , 2 , 3 };
        GfVec3i expected[] = { GfVec3i(1,2,3) };
        if (!COMPARE_INDICES("identity", _tokens->rightHanded, numVerts, verts, expected)) {
            return false;
        }
    }
    {
        // This mesh intentionally has a degenerate face with a single vertex. 
        // The resulting mesh should contain only a single face.
        int numVerts[] = { 1, 3 };
        int verts[] = { 1, 1, 2 , 3 };
        GfVec3i expected[] = { GfVec3i(1,2,3) };
        if (!COMPARE_INDICES("identity", _tokens->rightHanded, numVerts, verts, expected)) {
            return false;
        }
    }
    {
        // This mesh intentionally has a degenerate face with a two verts. 
        // The resulting mesh should contain only a single face.
        int numVerts[] = { 2, 3 };
        int verts[] = { 1, 1, 1, 2 , 3 };
        GfVec3i expected[] = { GfVec3i(1,2,3) };
        if (!COMPARE_INDICES("identity", _tokens->rightHanded, numVerts, verts, expected)) {
            return false;
        }
    }
    {
        // This mesh intentionally has one degenerate face with no verts.
        int numVerts[] = { 0, 4 };
        int verts[] = { 1, 2, 3, 4 };
        GfVec3i expected[] = { GfVec3i(1, 2, 3),
                               GfVec3i(1, 3, 4) };
        if (!COMPARE_INDICES("quad", _tokens->rightHanded, numVerts, verts, expected)) {
            return false;
        }
    }
    {
        // This mesh intentionally has one degenerate face with two verts.
        int numVerts[] = { 2, 4 };
        int verts[] = { 1, 1, 1, 2, 3, 4 };
        GfVec3i expected[] = { GfVec3i(1, 2, 3),
                               GfVec3i(1, 3, 4) };
        if (!COMPARE_INDICES("quad", _tokens->rightHanded, numVerts, verts, expected)) {
            return false;
        }
    }
    {
        int numVerts[] = { 3, 4, 3 };
        int verts[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        GfVec3i expected[] = { GfVec3i(1, 2, 3),
                               GfVec3i(4, 5, 6),
                               GfVec3i(4, 6, 7),
                               GfVec3i(8, 9, 10) };
        if (!COMPARE_INDICES("3 4 3", _tokens->rightHanded, numVerts, verts, expected)) {
            return false;
        }
    }
    return true;
}

bool
HoleTest()
{
    /*
         0-----3-------4-----7
         |     |       |     |
         |     |  hole |     |
         |     |        \    |
         1-----2---------5---6
               |        /    |
               |       |     |
               |       |     |
               8-------9----10
    */
    int numVerts[] = { 4, 4, 4, 4, 4};
    int verts[] = {
        0, 1, 2, 3,
        3, 2, 5, 4,
        4, 5, 6, 7,
        2, 8, 9, 5,
        5, 9, 10, 6,
    };
    int hole[] = { 1 };
    GfVec3i expected[] = { GfVec3i(0, 1, 2),
                           GfVec3i(0, 2, 3),
                           GfVec3i(4, 5, 6),
                           GfVec3i(4, 6, 7),
                           GfVec3i(2, 8, 9),
                           GfVec3i(2, 9, 5),
                           GfVec3i(5, 9, 10),
                           GfVec3i(5, 10, 6) };
    if (!COMPARE_INDICES_HOLE("hole", _tokens->rightHanded, numVerts, verts, hole, expected)) {
        return false;
    }
    return true;
}

bool
ComputeSmoothNormalsTest()
{
    /*
      XXX: this doesn't work, since HdBufferSource fails to determine
      type of array if it's empty.
    {
        int numVerts[] = {};
        int verts[] = {};
        GfVec3f points[] = {};
        GfVec3f expectedNormals[] = {};
        if (!COMPARE_SMOOTH_NORMALS("empty", _tokens->rightHanded,
                numVerts, verts, points, expectedNormals)) {
            return false;
        }
    }
    */
    {
        // This mesh intentionally has two degenerate faces, one with no verts
        // and one with a single vertex. The resulting mesh should contain only
        // a single face.
        int numVerts[] = { 0, 1, 3 };
        int verts[] = { 1, 0, 1, 2 };
        GfVec3f points[] = {
            GfVec3f( 0.0, 1.0, 0.0 ),
            GfVec3f(-1.0, 0.0, 0.0 ),
            GfVec3f( 1.0, 0.0, 0.0 ),
        };
        GfVec3f expectedNormals[] = {
            GfVec3f( 0.0, 0.0, 1.0 ),
            GfVec3f( 0.0, 0.0, 1.0 ),
            GfVec3f( 0.0, 0.0, 1.0 ),
        };
        if (!COMPARE_SMOOTH_NORMALS("triangle", _tokens->rightHanded,
                numVerts, verts, points, expectedNormals)) {
            return false;
        }
        if (!COMPARE_GPU_SMOOTH_NORMALS("triangle", _tokens->rightHanded,
                numVerts, verts, points, expectedNormals)) {
            return false;
        }
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
        GfVec3f expectedNormals[] = {
            GfVec3f( 0.0f, 0.0f, 1.0f ),
            GfVec3f( 0.0f, 0.0f, 1.0f ),
            GfVec3f( 0.0f, 0.0f, 1.0f ),
            GfVec3f( 0.0f, 0.0f, 1.0f ),
        };
        if (!COMPARE_SMOOTH_NORMALS("quad", _tokens->rightHanded,
                numVerts, verts, points, expectedNormals)) {
            return false;
        }
        if (!COMPARE_GPU_SMOOTH_NORMALS("quad", _tokens->rightHanded,
                numVerts, verts, points, expectedNormals)) {
            return false;
        }
    }
    {
        int numVerts[] = { 4, 4, 4, 4, 4, 4 };
        int verts[] = {
            0, 1, 2, 3,
            4, 5, 6, 7,
            0, 6, 5, 1,
            4, 7, 3, 2,
            0, 3, 7, 6,
            4, 2, 1, 5,
        };
        GfVec3f points[] = {
            GfVec3f( 1.0f, 1.0f, 1.0f ),
            GfVec3f(-1.0f, 1.0f, 1.0f ),
            GfVec3f(-1.0f,-1.0f, 1.0f ),
            GfVec3f( 1.0f,-1.0f, 1.0f ),
            GfVec3f(-1.0f,-1.0f,-1.0f ),
            GfVec3f(-1.0f, 1.0f,-1.0f ),
            GfVec3f( 1.0f, 1.0f,-1.0f ),
            GfVec3f( 1.0f,-1.0f,-1.0f ),
        };
        GfVec3f expectedNormals[] = {
            GfVec3f( 0.57735f, 0.57735f, 0.57735f ),
            GfVec3f(-0.57735f, 0.57735f, 0.57735f ),
            GfVec3f(-0.57735f,-0.57735f, 0.57735f ),
            GfVec3f( 0.57735f,-0.57735f, 0.57735f ),
            GfVec3f(-0.57735f,-0.57735f,-0.57735f ),
            GfVec3f(-0.57735f, 0.57735f,-0.57735f ),
            GfVec3f( 0.57735f, 0.57735f,-0.57735f ),
            GfVec3f( 0.57735f,-0.57735f,-0.57735f ),
        };
        if (!COMPARE_SMOOTH_NORMALS("cube float ccw", _tokens->rightHanded,
                numVerts, verts, points, expectedNormals)) {
            return false;
        }
        if (!COMPARE_GPU_SMOOTH_NORMALS("cube float ccw", _tokens->rightHanded,
                numVerts, verts, points, expectedNormals)) {
            return false;
        }
    }
    {
        int numVerts[] = { 4, 4, 4, 4, 4, 4 };
        int verts[] = {
            3, 2, 1, 0,
            7, 6, 5, 4,
            1, 5, 6, 0,
            2, 3, 7, 4,
            6, 7, 3, 0,
            5, 1, 2, 4,
        };
        GfVec3d points[] = {
            GfVec3f( 1.0, 1.0, 1.0 ),
            GfVec3f(-1.0, 1.0, 1.0 ),
            GfVec3f(-1.0,-1.0, 1.0 ),
            GfVec3f( 1.0,-1.0, 1.0 ),
            GfVec3f(-1.0,-1.0,-1.0 ),
            GfVec3f(-1.0, 1.0,-1.0 ),
            GfVec3f( 1.0, 1.0,-1.0 ),
            GfVec3f( 1.0,-1.0,-1.0 ),
        };
        GfVec3d expectedNormals[] = {
            GfVec3f( 0.57735, 0.57735, 0.57735 ),
            GfVec3f(-0.57735, 0.57735, 0.57735 ),
            GfVec3f(-0.57735,-0.57735, 0.57735 ),
            GfVec3f( 0.57735,-0.57735, 0.57735 ),
            GfVec3f(-0.57735,-0.57735,-0.57735 ),
            GfVec3f(-0.57735, 0.57735,-0.57735 ),
            GfVec3f( 0.57735, 0.57735,-0.57735 ),
            GfVec3f( 0.57735,-0.57735,-0.57735 ),
        };
        if (!COMPARE_SMOOTH_NORMALS("cube float cw", _tokens->leftHanded,
                numVerts, verts, points, expectedNormals)) {
            return false;
        }
        if (!COMPARE_GPU_SMOOTH_NORMALS("cube float cw", _tokens->leftHanded,
                numVerts, verts, points, expectedNormals)) {
            return false;
        }
    }
    {
        int numVerts[] = { 4, 4, 4, 4, 4, 4 };
        int verts[] = {
            0, 1, 2, 3,
            4, 5, 6, 7,
            0, 6, 5, 1,
            4, 7, 3, 2,
            0, 3, 7, 6,
            4, 2, 1, 5,
        };
        GfVec3d points[] = {
            GfVec3d( 1.0, 1.0, 1.0 ),
            GfVec3d(-1.0, 1.0, 1.0 ),
            GfVec3d(-1.0,-1.0, 1.0 ),
            GfVec3d( 1.0,-1.0, 1.0 ),
            GfVec3d(-1.0,-1.0,-1.0 ),
            GfVec3d(-1.0, 1.0,-1.0 ),
            GfVec3d( 1.0, 1.0,-1.0 ),
            GfVec3d( 1.0,-1.0,-1.0 ),
        };
        GfVec3d expectedNormals[] = {
            GfVec3d( 0.57735, 0.57735, 0.57735 ),
            GfVec3d(-0.57735, 0.57735, 0.57735 ),
            GfVec3d(-0.57735,-0.57735, 0.57735 ),
            GfVec3d( 0.57735,-0.57735, 0.57735 ),
            GfVec3d(-0.57735,-0.57735,-0.57735 ),
            GfVec3d(-0.57735, 0.57735,-0.57735 ),
            GfVec3d( 0.57735, 0.57735,-0.57735 ),
            GfVec3d( 0.57735,-0.57735,-0.57735 ),
        };
        if (!COMPARE_SMOOTH_NORMALS("cube double", _tokens->rightHanded,
                numVerts, verts, points, expectedNormals)) {
            return false;
        }
        if (!COMPARE_GPU_SMOOTH_NORMALS("cube double", _tokens->rightHanded,
                numVerts, verts, points, expectedNormals)) {
            return false;
        }
    }
    return true;
}

bool
FaceVaryingTest()
{
    /*
         0-----3-------4-----7
         |     |       |     |
         |     |  hole |     |
         |     |        \    |
         1-----2---------5---6
               |        /    |
               |       |     |
               |       |     |
               8-------9----10
    */
    int numVerts[] = { 4, 4, 4, 4, 4};
    int verts[] = {
        0, 1, 2, 3,
        3, 2, 5, 4,
        4, 5, 6, 7,
        2, 8, 9, 5,
        5, 9, 10, 6,
    };
    int hole[] = { 1 };
    float fvarValues[] = { 1, 2, 3, 4,
                           5, 6, 7, 8,
                           9, 10, 11, 12,
                           13, 14, 15, 16,
                           17, 18, 19, 20 };
    float expected[] = { 1, 2, 3, 1, 3, 4,
                         // 5, 6, 7, 5, 7, 8, // hole
                         9, 10, 11, 9, 11, 12,
                         13, 14, 15, 13, 15, 16,
                         17, 18, 19, 17, 19, 20 };

    if (!_CompareFaceVarying("FaceVarying", _tokens->rightHanded,
                             _BuildArray(numVerts, sizeof(numVerts)/sizeof(int)),
                             _BuildArray(verts, sizeof(verts)/sizeof(int)),
                             _BuildArray(hole, sizeof(hole)/sizeof(int)),
                             _BuildArray(fvarValues, sizeof(fvarValues)/sizeof(float)),
                             _BuildArray(expected, sizeof(expected)/sizeof(float)))) {
        return false;
    }
    return true;
}

bool
InvalidTopologyTest()
{
    int numVerts[] = { 4, 4, 4, 4, 4};
    int verts[] = { 0, 1, 2, 3,
                    3, 2, 5, 4, //hole
                    4, 5, 6, 7,
                    // 2, 8, 9, 5, missing
                    // 5, 9, 10, 6, missing
    };
    int hole[] = { 1 };
    GfVec3i expected[] = { GfVec3i(0, 1, 2),
                           GfVec3i(0, 2, 3),
                           //GfVec3i(3, 2, 5), // hole, skipped
                           //GfVec3i(3, 5, 4), // hole, skipped
                           GfVec3i(4, 5, 6),
                           GfVec3i(4, 6, 7),
                           GfVec3i(0, 0, 0),  // missing
                           GfVec3i(0, 0, 0),  // missing
                           GfVec3i(0, 0, 0),  // missing
                           GfVec3i(0, 0, 0) };  // missing
    float fvarValues[] = { 1, 2, 3, 4,
                           5, 6, 7, 8, // hole
                           9, 10, 11, 12,
                           13, 14, 15, 16,
                           //17, 18, 19, 20  // missing fvar
    };
    float fvarExpected[] = { 1, 2, 3, 1, 3, 4,
                             //5, 6, 7, 5, 7, 8,  // hole, skipepd
                             9, 10, 11, 9, 11, 12,
                             13, 14, 15, 13, 15, 16,
                             0, 0, 0, 0, 0, 0,  // missing
    };

    if (!_CompareIndices("Invalid", _tokens->rightHanded,
                         _BuildArray(numVerts, sizeof(numVerts)/sizeof(int)),
                         _BuildArray(verts, sizeof(verts)/sizeof(int)),
                         _BuildArray(hole, sizeof(hole)/sizeof(int)),
                         _BuildArray(expected, sizeof(expected)/sizeof(expected[0])))) {
        return false;
    }
    if (!_CompareFaceVarying("InvalidFaceVarying", _tokens->rightHanded,
                             _BuildArray(numVerts, sizeof(numVerts)/sizeof(int)),
                             _BuildArray(verts, sizeof(verts)/sizeof(int)),
                             _BuildArray(hole, sizeof(hole)/sizeof(int)),
                             _BuildArray(fvarValues, sizeof(fvarValues)/sizeof(float)),
                             _BuildArray(fvarExpected, sizeof(fvarExpected)/sizeof(float)))) {
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
    success &= BasicTest();
    success &= HoleTest();
    success &= ComputeSmoothNormalsTest();
    success &= FaceVaryingTest();
    success &= InvalidTopologyTest();

    TF_VERIFY(mark.IsClean());

    if (success and mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

