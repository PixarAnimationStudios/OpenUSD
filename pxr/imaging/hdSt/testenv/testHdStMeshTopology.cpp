//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdSt/flatNormals.h"
#include "pxr/imaging/hdSt/meshTopology.h"
#include "pxr/imaging/hdSt/quadrangulate.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/smoothNormals.h"
#include "pxr/imaging/hdSt/triangulate.h"
#include "pxr/imaging/hdSt/vertexAdjacency.h"

#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/driver.h"
#include "pxr/imaging/hd/smoothNormals.h"
#include "pxr/imaging/hd/flatNormals.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vertexAdjacency.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/glf/testGLContext.h"

#include "pxr/imaging/hgi/tokens.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3i.h"
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
struct _EmptyArray
{
};

template <typename T, size_t N>
static VtArray<T>
_BuildArray(T const (&values)[N])
{
    VtArray<T> result(N);
    std::copy(values, values+N, result.begin());
    return result;
}

template <typename T>
static VtArray<T>
_BuildArray(_EmptyArray<T> values)
{
    return VtArray<T>();
}

static bool
_CompareIndices(HdStResourceRegistrySharedPtr const &registry,
                std::string const & name,
                std::string const & orientation,
                VtIntArray numVerts, VtIntArray verts, VtIntArray holes,
                VtVec3iArray expected)
{
    HdMeshTopology m(_tokens->bilinear, TfToken(orientation), numVerts, verts);

    m.SetHoleIndices(holes);

    // Convert topology to render delegate version
    HdSt_MeshTopologySharedPtr const rdTopology = HdSt_MeshTopology::New(m, 0);

    // compute triangle indices
    HdBufferSourceSharedPtr const source =
                  rdTopology->GetTriangleIndexBuilderComputation(SdfPath(name));
    HdBufferSpecVector bufferSpecs;
    source->GetBufferSpecs(&bufferSpecs);
    HdBufferArrayRangeSharedPtr const range =
        registry->AllocateNonUniformBufferArrayRange(
            HdTokens->topology,
            bufferSpecs,
            HdBufferArrayUsageHintBitsIndex);
    registry->AddSource(range, source);

    // execute computation
    registry->Commit();

    const VtVec3iArray result = range->ReadData(HdTokens->indices).Get<VtVec3iArray>();
    if (result != expected) {
        std::cout << name << " test failed:\n";
        std::cout << "  expected: " << expected << "\n";
        std::cout << "  result: " << result << "\n";
        return false;
    }
    return true;
}

static bool
_CompareFaceVarying(HdStResourceRegistrySharedPtr const &registry,
                    std::string const &name,
                    std::string const &orientation,
                    VtIntArray numVerts, VtIntArray verts, VtIntArray holes,
                    VtFloatArray fvarValues,
                    VtFloatArray expected)
{
    HdMeshTopology m(_tokens->bilinear, TfToken(orientation), numVerts, verts);

    m.SetHoleIndices(holes);

    // Convert topology to render delegate version
    HdSt_MeshTopologySharedPtr const rdTopology = HdSt_MeshTopology::New(m, 0);

    // compute triangulated fvar values
    HdBufferSourceSharedPtr const fvarSource =
        std::make_shared<HdVtBufferSource>(HdTokens->primvar, VtValue(fvarValues));
    registry->AddSource(fvarSource);
    HdBufferSourceSharedPtr const source =
                rdTopology->GetTriangulateFaceVaryingComputation(fvarSource,
                                                                 SdfPath(name));
    HdBufferSpecVector bufferSpecs;
    source->GetBufferSpecs(&bufferSpecs);
    HdBufferArrayRangeSharedPtr const range =
        registry->AllocateNonUniformBufferArrayRange(
            HdTokens->primvar,
            bufferSpecs,
            HdBufferArrayUsageHintBitsStorage);
    registry->AddSource(range, source);

    // execute computation
    registry->Commit();

    const VtFloatArray result = range->ReadData(HdTokens->primvar).Get<VtFloatArray>();
    if (result != expected) {
        std::cout << name << " test failed:\n";
        std::cout << "  expected: " << expected << "\n";
        std::cout << "  result: " << result << "\n";
        return false;
    }
    return true;
}

#define COMPARE_INDICES(registry, name, orientation, numVerts, verts, expected) \
    _CompareIndices(registry, name, orientation, \
                   _BuildArray(numVerts), \
                   _BuildArray(verts), \
                    /*holes=*/VtIntArray(),                             \
                   _BuildArray(expected))

#define COMPARE_INDICES_HOLE(registry, name, orientation, numVerts, verts, holes, expected) \
    _CompareIndices(registry, name, orientation,                                  \
                    _BuildArray(numVerts), \
                    _BuildArray(verts),      \
                    _BuildArray(holes),      \
                    _BuildArray(expected))

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
_CompareSmoothNormals(HdStResourceRegistrySharedPtr const &registry,
                      std::string const & name,
                      std::string const & orientation,
                      VtIntArray numVerts, VtIntArray verts,
                      VtArray<Vec3Type> points,
                      VtArray<Vec3Type> expectedNormals)
{
    HdMeshTopology topology(_tokens->bilinear, TfToken(orientation), numVerts, verts);
    HdSt_VertexAdjacencyBuilder adjacencyBuilder;
    HdBufferSourceSharedPtr const pointsSource =
        std::make_shared<HdVtBufferSource>(HdTokens->points, VtValue(points));

    // Adjacency computation
    {
        HdBufferSourceSharedPtr const adjComputation =
            adjacencyBuilder.
                GetSharedVertexAdjacencyBuilderComputation(&topology);
        registry->AddSource(adjComputation);
        registry->Commit();
    }

    const int numPoints = pointsSource->GetNumElements();
    const VtValue resultValue = VtValue(Hd_SmoothNormals::ComputeSmoothNormals(
        adjacencyBuilder.GetVertexAdjacency(), numPoints,
        static_cast<const Vec3Type*>(pointsSource->GetData())));

    if (!resultValue.IsHolding< VtArray<Vec3Type> >()) {
        std::cout << name << " test failed:\n";
        std::cout << "  wrong returned value type:\n";
        return false;
    }
    const VtArray<Vec3Type> result = resultValue.Get< VtArray<Vec3Type> >();
    if (!_CompareArrays(result, expectedNormals)) {
        std::cout << name << " test failed:\n";
        std::cout << "  expected: " << expectedNormals << "\n";
        std::cout << "  result: " << result << "\n";
        return false;
    }
    return true;
}

#define COMPARE_SMOOTH_NORMALS(registry, name, orientation, numVerts, verts, points, expected) \
    _CompareSmoothNormals(registry, name, orientation, \
               _BuildArray(numVerts), \
               _BuildArray(verts), \
               _BuildArray(points), \
               _BuildArray(expected))

template <typename Vec3Type>
bool
_CompareFlatNormals(std::string const & name,
                    std::string const & orientation,
                    VtIntArray numVerts, VtIntArray verts,
                    VtArray<Vec3Type> points,
                    VtArray<Vec3Type> expectedNormals)
{
    HdMeshTopology topology(_tokens->bilinear, TfToken(orientation), numVerts, verts);
    HdBufferSourceSharedPtr const pointsSource =
        std::make_shared<HdVtBufferSource>(HdTokens->points, VtValue(points));

    const VtValue resultValue = VtValue(Hd_FlatNormals::ComputeFlatNormals(
        &topology, static_cast<const Vec3Type*>(pointsSource->GetData())));

    if (!resultValue.IsHolding< VtArray<Vec3Type> >()) {
        std::cout << name << " test failed:\n";
        std::cout << "  wrong returned value type:\n";
        return false;
    }
    const VtArray<Vec3Type> result = resultValue.Get< VtArray<Vec3Type> >();
    if (!_CompareArrays(result, expectedNormals)) {
        std::cout << name << " test failed:\n";
        std::cout << "  expected: " << expectedNormals << "\n";
        std::cout << "  result: " << result << "\n";
        return false;
    }
    return true;
}

#define COMPARE_FLAT_NORMALS(name, orientation, numVerts, verts, points, expected) \
    _CompareFlatNormals(name, orientation, \
               _BuildArray(numVerts), \
               _BuildArray(verts), \
               _BuildArray(points), \
               _BuildArray(expected))

template <typename Vec3Type>
bool
_CompareGpuSmoothNormals(HdStResourceRegistrySharedPtr const &registry,
                         std::string const & name,
                         std::string const & orientation,
                         VtIntArray numVerts, VtIntArray verts,
                         VtArray<Vec3Type> points,
                         VtArray<Vec3Type> expectedNormals)
{
    HdMeshTopology topology(_tokens->bilinear, TfToken(orientation), numVerts, verts);
    HdSt_VertexAdjacencyBuilder adjacencyBuilder;

    HdBufferSourceSharedPtr const pointsSource =
        std::make_shared<HdVtBufferSource>(HdTokens->points, VtValue(points));

    // Adjacency computation
    {
        HdBufferSourceSharedPtr adjComputation =
            adjacencyBuilder.
                GetSharedVertexAdjacencyBuilderComputation(&topology);
        registry->AddSource(adjComputation);

        HdBufferSourceSharedPtr const adjGpuComputation =
            std::make_shared<HdSt_VertexAdjacencyBufferSource>(
                adjacencyBuilder.GetVertexAdjacency(), adjComputation);
        HdBufferSpecVector bufferSpecs;
        adjGpuComputation->GetBufferSpecs(&bufferSpecs);
        HdBufferArrayRangeSharedPtr const adjRange =
            registry->AllocateNonUniformBufferArrayRange(
                HdTokens->topology, bufferSpecs,
                HdBufferArrayUsageHintBitsStorage);
        adjacencyBuilder.SetVertexAdjacencyRange(adjRange);
        registry->AddSource(adjRange, adjGpuComputation);
    }

    // GPU smooth computation
    HdStComputationSharedPtr const normalComputation =
        std::make_shared<HdSt_SmoothNormalsComputationGPU>(
            &adjacencyBuilder,
            HdTokens->points, HdTokens->normals,
            /*srcDataType=*/pointsSource->GetTupleType().type,
            /*packed=*/false);

    // build bufferspec
    HdBufferSpecVector bufferSpecs;
    pointsSource->GetBufferSpecs(&bufferSpecs);
    normalComputation->GetBufferSpecs(&bufferSpecs);

    // allocate GPU buffer range
    HdBufferArrayRangeSharedPtr const range =
        registry->AllocateNonUniformBufferArrayRange(
            HdTokens->primvar,
            bufferSpecs,
            HdBufferArrayUsageHintBitsStorage);

    // commit points
    HdBufferSourceSharedPtrVector sources;
    sources.push_back(pointsSource);
    registry->AddSources(range, std::move(sources));
    registry->AddComputation(range, normalComputation, HdStComputeQueueZero);

    // commit & execute
    registry->Commit();

    // retrieve result
    const VtValue resultValue = range->ReadData(HdTokens->normals);
    if (!resultValue.IsHolding< VtArray<Vec3Type> >()) {
        std::cout << name << " test failed:\n";
        std::cout << "  wrong returned value type:\n";
        return false;
    }
    const VtArray<Vec3Type> result = resultValue.Get< VtArray<Vec3Type> >();
    if (!_CompareArrays(result, expectedNormals)) {
        std::cout << name << " test failed:\n";
        std::cout << "  expected: " << expectedNormals << "\n";
        std::cout << "  result: " << result << "\n";
        return false;
    }
    return true;
}

#define COMPARE_GPU_SMOOTH_NORMALS(registry, name, orientation, numVerts, verts, points, expected) \
    _CompareGpuSmoothNormals(registry, name, orientation, \
               _BuildArray(numVerts), \
               _BuildArray(verts), \
               _BuildArray(points), \
               _BuildArray(expected))

template <typename Vec3Type>
bool
_CompareGpuFlatNormals(HdStResourceRegistrySharedPtr const &registry,
                       std::string const & name,
                       std::string const & orientation,
                       VtIntArray numVerts, VtIntArray verts,
                       VtArray<Vec3Type> points,
                       VtArray<Vec3Type> expectedNormals,
                       bool quad)
{
    HdMeshTopology topology(_tokens->bilinear, TfToken(orientation), numVerts, verts);
    HdSt_MeshTopologySharedPtr const stTopo = HdSt_MeshTopology::New(topology, 0);

    HdBufferSourceSharedPtr pointsSource =
        std::make_shared<HdVtBufferSource>(HdTokens->points, VtValue(points));

    // build the points range
    HdBufferSpecVector vertexSpecs;
    pointsSource->GetBufferSpecs(&vertexSpecs);
    HdBufferArrayUsageHint vertexUsageHint =
        HdBufferArrayUsageHintBitsVertex | HdBufferArrayUsageHintBitsStorage;
    HdBufferArrayRangeSharedPtr const vertexRange =
        registry->AllocateNonUniformBufferArrayRange(HdTokens->primvar,
                                                     vertexSpecs,
                                                     vertexUsageHint);

    // index builder
    HdBufferSourceSharedPtr indexComputation;
    HdBufferSourceSharedPtr quadInfoComputation;
    if (quad) {
        quadInfoComputation = stTopo->GetQuadInfoBuilderComputation(
            false, SdfPath("/Test"), registry.get());
        indexComputation = stTopo->GetQuadIndexBuilderComputation(
            SdfPath("/Test"));
        pointsSource = stTopo->GetQuadrangulateComputation(pointsSource,
            SdfPath("/Test"));
    } else {
        indexComputation = stTopo->GetTriangleIndexBuilderComputation(
            SdfPath("/Test"));
    }

    // build the topology range
    HdBufferSpecVector topoSpecs;
    indexComputation->GetBufferSpecs(&topoSpecs);
    HdBufferArrayUsageHint topoUsageHint =
        HdBufferArrayUsageHintBitsIndex | HdBufferArrayUsageHintBitsStorage;
    HdBufferArrayRangeSharedPtr const topoRange =
        registry->AllocateNonUniformBufferArrayRange(HdTokens->topology,
                                                     topoSpecs,
                                                     topoUsageHint);

    // GPU flat normals computation
    const int numFaces = topology.GetFaceVertexCounts().size();
    HdStComputationSharedPtr const normalComputation =
        std::make_shared<HdSt_FlatNormalsComputationGPU>(
            topoRange, vertexRange, numFaces,
            HdTokens->points, HdTokens->normals,
            /*srcDataType=*/pointsSource->GetTupleType().type,
            /*packed=*/false);

    // build the normals range
    HdBufferSpecVector elementSpecs;
    normalComputation->GetBufferSpecs(&elementSpecs);
    HdBufferArrayRangeSharedPtr const elementRange =
        registry->AllocateNonUniformBufferArrayRange(
            HdTokens->primvar,
            elementSpecs,
            HdBufferArrayUsageHintBitsStorage);

    // Add sources
    if (quadInfoComputation) {
        registry->AddSource(quadInfoComputation);
    }
    registry->AddSource(topoRange, indexComputation);
    registry->AddSource(vertexRange, pointsSource);
    registry->AddComputation(
        elementRange, normalComputation, HdStComputeQueueZero);

    // commit & execute
    registry->Commit();

    // retrieve result
    const VtValue resultValue = elementRange->ReadData(HdTokens->normals);
    if (!resultValue.IsHolding< VtArray<Vec3Type> >()) {
        std::cout << name << " test failed:\n";
        std::cout << "  wrong returned value type:\n";
        return false;
    }
    const VtArray<Vec3Type> result = resultValue.Get< VtArray<Vec3Type> >();
    if (!_CompareArrays(result, expectedNormals)) {
        std::cout << name << " test failed:\n";
        std::cout << "  expected: " << expectedNormals << "\n";
        std::cout << "  result: " << result << "\n";
        return false;
    }
    return true;
}

#define COMPARE_GPU_FLAT_NORMALS_TRI(registry, name, orientation, numVerts, verts, points, expected) \
    _CompareGpuFlatNormals(registry, name, orientation, \
               _BuildArray(numVerts), \
               _BuildArray(verts), \
               _BuildArray(points), \
               _BuildArray(expected), false)

#define COMPARE_GPU_FLAT_NORMALS_QUAD(registry, name, orientation, numVerts, verts, points, expected) \
    _CompareGpuFlatNormals(registry, name, orientation, \
               _BuildArray(numVerts), \
               _BuildArray(verts), \
               _BuildArray(points), \
               _BuildArray(expected), true)

bool
BasicTest(HdStResourceRegistrySharedPtr const &registry)
{
    {
        const _EmptyArray<int> numVerts;
        const _EmptyArray<int> verts;
        const _EmptyArray<GfVec3i> expected;
        if (!COMPARE_INDICES(registry, "empty",
                _tokens->rightHanded, numVerts, verts, expected)) {
            return false;
        }
    }
    {
        // This mesh intentionally has one degenerate face with no verts.
        int numVerts[] = { 0, 3 };
        int verts[] = { 1 , 2 , 3 };
        GfVec3i expected[] = { GfVec3i(1,2,3) };
        if (!COMPARE_INDICES(registry, "identity_no_vert_face",
                _tokens->rightHanded, numVerts, verts, expected)) {
            return false;
        }
    }
    {
        // This mesh intentionally has a degenerate face with a single vertex. 
        // The resulting mesh should contain only a single face.
        int numVerts[] = { 1, 3 };
        int verts[] = { 1, 1, 2 , 3 };
        GfVec3i expected[] = { GfVec3i(1,2,3) };
        if (!COMPARE_INDICES(registry, "identity_one_vert_face",
                _tokens->rightHanded, numVerts, verts, expected)) {
            return false;
        }
    }
    {
        // This mesh intentionally has a degenerate face with a two verts. 
        // The resulting mesh should contain only a single face.
        int numVerts[] = { 2, 3 };
        int verts[] = { 1, 1, 1, 2 , 3 };
        GfVec3i expected[] = { GfVec3i(1,2,3) };
        if (!COMPARE_INDICES(registry, "identity_two_vert_face",
                _tokens->rightHanded, numVerts, verts, expected)) {
            return false;
        }
    }
    {
        // This mesh intentionally has one degenerate face with no verts.
        int numVerts[] = { 0, 4 };
        int verts[] = { 1, 2, 3, 4 };
        GfVec3i expected[] = { GfVec3i(1, 2, 3),
                               GfVec3i(1, 3, 4) };
        if (!COMPARE_INDICES(registry, "quad_no_vet_face",
                _tokens->rightHanded, numVerts, verts, expected)) {
            return false;
        }
    }
    {
        // This mesh intentionally has one degenerate face with two verts.
        int numVerts[] = { 2, 4 };
        int verts[] = { 1, 1, 1, 2, 3, 4 };
        GfVec3i expected[] = { GfVec3i(1, 2, 3),
                               GfVec3i(1, 3, 4) };
        if (!COMPARE_INDICES(registry, "quad_two_vert_face",
                _tokens->rightHanded, numVerts, verts, expected)) {
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
        if (!COMPARE_INDICES(registry, "3 4 3",
                _tokens->rightHanded, numVerts, verts, expected)) {
            return false;
        }
    }
    return true;
}

bool
HoleTest(HdStResourceRegistrySharedPtr const &registry)
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
    if (!COMPARE_INDICES_HOLE(registry, "hole", _tokens->rightHanded, numVerts,
            verts, hole, expected)) {
        return false;
    }
    return true;
}

bool
ComputeNormalsTest(HdStResourceRegistrySharedPtr const &registry)
{
    {
        const _EmptyArray<int> numVerts;
        const _EmptyArray<int> verts;
        const _EmptyArray<GfVec3f> points;
        const _EmptyArray<GfVec3f> expectedNormals;
        if (!COMPARE_SMOOTH_NORMALS(registry, "empty", _tokens->rightHanded,
                numVerts, verts, points, expectedNormals)) {
            return false;
        }
    }
    {
        int numVerts[] = {3};
        const _EmptyArray<int> verts;
        GfVec3f points[] = {
            GfVec3f(-1.0, 0.0, 0.0 ),
            GfVec3f( 0.0, 0.0, 2.0 ),
            GfVec3f( 1.0, 0.0, 0.0 ),
        };
        const _EmptyArray<GfVec3f> expectedNormals;
        if (!COMPARE_SMOOTH_NORMALS(
                registry, "missing_faceVertexIndices",_tokens->rightHanded,
                numVerts, verts, points, expectedNormals)) {
            return false;
        }
    }
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
        GfVec3f expectedSmoothNormals[] = {
            GfVec3f( 0.0, 0.0, 1.0 ),
            GfVec3f( 0.0, 0.0, 1.0 ),
            GfVec3f( 0.0, 0.0, 1.0 ),
        };
        GfVec3f expectedFlatNormals[] = {
            GfVec3f( 0.0, 0.0, 0.0 ),
            GfVec3f( 0.0, 0.0, 0.0 ),
            GfVec3f( 0.0, 0.0, 1.0 ),
        };
        if (!COMPARE_SMOOTH_NORMALS(registry, "triangle_cpu",
                _tokens->rightHanded, numVerts, verts, points,
                expectedSmoothNormals)) {
            return false;
        }
        if (!COMPARE_GPU_SMOOTH_NORMALS(registry, "triangle_gpu",
                _tokens->rightHanded,numVerts, verts, points,
                expectedSmoothNormals)) {
            return false;
        }
        if (!COMPARE_FLAT_NORMALS("triangle_flat_cpu", _tokens->rightHanded,
                numVerts, verts, points, expectedFlatNormals)) {
            return false;
        }
        if (!COMPARE_GPU_FLAT_NORMALS_TRI(registry, "triangle_flat_gpu_tri",
                _tokens->rightHanded, numVerts, verts, points,
                expectedFlatNormals)) {
            return false;
        }
        if (!COMPARE_GPU_FLAT_NORMALS_QUAD(registry, "triangle_flat_gpu_quad",
                _tokens->rightHanded, numVerts, verts, points,
                expectedFlatNormals)) {
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
        GfVec3f expectedSmoothNormals[] = {
            GfVec3f( 0.0f, 0.0f, 1.0f ),
            GfVec3f( 0.0f, 0.0f, 1.0f ),
            GfVec3f( 0.0f, 0.0f, 1.0f ),
            GfVec3f( 0.0f, 0.0f, 1.0f ),
        };
        GfVec3f expectedFlatNormals[] = {
            GfVec3f( 0.0f, 0.0f, 1.0f ),
        };
        if (!COMPARE_SMOOTH_NORMALS(registry, "quad_cpu", _tokens->rightHanded,
                numVerts, verts, points, expectedSmoothNormals)) {
            return false;
        }
        if (!COMPARE_GPU_SMOOTH_NORMALS(registry, "quad_gpu",
                _tokens->rightHanded, numVerts, verts, points,
                expectedSmoothNormals)) {
            return false;
        }
        if (!COMPARE_FLAT_NORMALS("quad_flat_cpu", _tokens->rightHanded,
                numVerts, verts, points, expectedFlatNormals)) {
            return false;
        }
        if (!COMPARE_GPU_FLAT_NORMALS_TRI(registry, "quad_flat_gpu_tri",
                _tokens->rightHanded, numVerts, verts, points,
                expectedFlatNormals)) {
            return false;
        }
        if (!COMPARE_GPU_FLAT_NORMALS_QUAD(registry, "quad_flat_gpu_quad",
                _tokens->rightHanded, numVerts, verts, points,
                expectedFlatNormals)) {
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
        GfVec3f expectedSmoothNormals[] = {
            GfVec3f( 0.57735f, 0.57735f, 0.57735f ),
            GfVec3f(-0.57735f, 0.57735f, 0.57735f ),
            GfVec3f(-0.57735f,-0.57735f, 0.57735f ),
            GfVec3f( 0.57735f,-0.57735f, 0.57735f ),
            GfVec3f(-0.57735f,-0.57735f,-0.57735f ),
            GfVec3f(-0.57735f, 0.57735f,-0.57735f ),
            GfVec3f( 0.57735f, 0.57735f,-0.57735f ),
            GfVec3f( 0.57735f,-0.57735f,-0.57735f ),
        };
        GfVec3f expectedFlatNormals[] = {
            GfVec3f( 0.0, 0.0, 1.0 ),
            GfVec3f( 0.0, 0.0,-1.0 ),
            GfVec3f( 0.0, 1.0, 0.0 ),
            GfVec3f( 0.0,-1.0, 0.0 ),
            GfVec3f( 1.0, 0.0, 0.0 ),
            GfVec3f(-1.0, 0.0, 0.0 ),
        };
        if (!COMPARE_SMOOTH_NORMALS(registry, "cube float ccw_cpu",
                _tokens->rightHanded, numVerts, verts, points,
                expectedSmoothNormals)) {
            return false;
        }
        if (!COMPARE_GPU_SMOOTH_NORMALS(registry, "cube float ccw_gpu",
                _tokens->rightHanded,
                numVerts, verts, points, expectedSmoothNormals)) {
            return false;
        }
        if (!COMPARE_FLAT_NORMALS("cube float ccw_flat_cpu",
                _tokens->rightHanded, numVerts, verts, points,
                expectedFlatNormals)) {
            return false;
        }
        if (!COMPARE_GPU_FLAT_NORMALS_TRI(registry,
                "cube float ccw_flat_gpu_tri", _tokens->rightHanded,
                numVerts, verts, points, expectedFlatNormals)) {
            return false;
        }
        if (!COMPARE_GPU_FLAT_NORMALS_QUAD(registry,
                "cube float ccw_flat_gpu_quad", _tokens->rightHanded,
                numVerts, verts, points, expectedFlatNormals)) {
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
        GfVec3f points[] = {
            GfVec3f( 1.0, 1.0, 1.0 ),
            GfVec3f(-1.0, 1.0, 1.0 ),
            GfVec3f(-1.0,-1.0, 1.0 ),
            GfVec3f( 1.0,-1.0, 1.0 ),
            GfVec3f(-1.0,-1.0,-1.0 ),
            GfVec3f(-1.0, 1.0,-1.0 ),
            GfVec3f( 1.0, 1.0,-1.0 ),
            GfVec3f( 1.0,-1.0,-1.0 ),
        };
        GfVec3f expectedSmoothNormals[] = {
            GfVec3f( 0.57735, 0.57735, 0.57735 ),
            GfVec3f(-0.57735, 0.57735, 0.57735 ),
            GfVec3f(-0.57735,-0.57735, 0.57735 ),
            GfVec3f( 0.57735,-0.57735, 0.57735 ),
            GfVec3f(-0.57735,-0.57735,-0.57735 ),
            GfVec3f(-0.57735, 0.57735,-0.57735 ),
            GfVec3f( 0.57735, 0.57735,-0.57735 ),
            GfVec3f( 0.57735,-0.57735,-0.57735 ),
        };
        GfVec3f expectedFlatNormals[] = {
            GfVec3f( 0.0, 0.0, 1.0 ),
            GfVec3f( 0.0, 0.0,-1.0 ),
            GfVec3f( 0.0, 1.0, 0.0 ),
            GfVec3f( 0.0,-1.0, 0.0 ),
            GfVec3f( 1.0, 0.0, 0.0 ),
            GfVec3f(-1.0, 0.0, 0.0 ),
        };
        if (!COMPARE_SMOOTH_NORMALS(registry, "cube float cw_cpu",
                _tokens->leftHanded, numVerts, verts, points,
                expectedSmoothNormals)) {
            return false;
        }
        if (!COMPARE_GPU_SMOOTH_NORMALS(registry, "cube float cw_gpu",
                _tokens->leftHanded, numVerts, verts, points,
                expectedSmoothNormals)) {
            return false;
        }
        if (!COMPARE_FLAT_NORMALS("cube float cw_flat_cpu", _tokens->leftHanded,
                numVerts, verts, points, expectedFlatNormals)) {
            return false;
        }
        if (!COMPARE_GPU_FLAT_NORMALS_TRI(registry,
                "cube float cw_flat_gpu_tri", _tokens->leftHanded,
                numVerts, verts, points, expectedFlatNormals)) {
            return false;
        }
        if (!COMPARE_GPU_FLAT_NORMALS_QUAD(registry,
                "cube float cw_flat_gpu_quad", _tokens->leftHanded,
                numVerts, verts, points, expectedFlatNormals)) {
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
        GfVec3d expectedSmoothNormals[] = {
            GfVec3d( 0.57735, 0.57735, 0.57735 ),
            GfVec3d(-0.57735, 0.57735, 0.57735 ),
            GfVec3d(-0.57735,-0.57735, 0.57735 ),
            GfVec3d( 0.57735,-0.57735, 0.57735 ),
            GfVec3d(-0.57735,-0.57735,-0.57735 ),
            GfVec3d(-0.57735, 0.57735,-0.57735 ),
            GfVec3d( 0.57735, 0.57735,-0.57735 ),
            GfVec3d( 0.57735,-0.57735,-0.57735 ),
        };
        GfVec3d expectedFlatNormals[] = {
            GfVec3d( 0.0, 0.0, 1.0 ),
            GfVec3d( 0.0, 0.0,-1.0 ),
            GfVec3d( 0.0, 1.0, 0.0 ),
            GfVec3d( 0.0,-1.0, 0.0 ),
            GfVec3d( 1.0, 0.0, 0.0 ),
            GfVec3d(-1.0, 0.0, 0.0 ),
        };
        if (!COMPARE_SMOOTH_NORMALS(registry, "cube double_cpu",
                _tokens->rightHanded, numVerts, verts, points,
                expectedSmoothNormals)) {
            return false;
        }
        if (!COMPARE_GPU_SMOOTH_NORMALS(registry, "cube double_gpu",
                _tokens->rightHanded, numVerts, verts, points,
                expectedSmoothNormals)) {
            return false;
        }
        if (!COMPARE_FLAT_NORMALS("cube double_flat_cpu", _tokens->rightHanded,
                numVerts, verts, points, expectedFlatNormals)) {
            return false;
        }
        if (!COMPARE_GPU_FLAT_NORMALS_TRI(registry, "cube double_flat_gpu_tri",
                _tokens->rightHanded,
                numVerts, verts, points, expectedFlatNormals)) {
            return false;
        }
        if (!COMPARE_GPU_FLAT_NORMALS_QUAD(registry, "cube double_flat_gpu_quad",
                _tokens->rightHanded,
                numVerts, verts, points, expectedFlatNormals)) {
            return false;
        }
    }
    return true;
}

bool
FaceVaryingTest(HdStResourceRegistrySharedPtr const &registry)
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

    if (!_CompareFaceVarying(registry, "FaceVarying", _tokens->rightHanded,
                             _BuildArray(numVerts),
                             _BuildArray(verts),
                             _BuildArray(hole),
                             _BuildArray(fvarValues),
                             _BuildArray(expected))) {
        return false;
    }
    return true;
}

bool
InvalidTopologyTest(HdStResourceRegistrySharedPtr const &registry)
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

    if (!_CompareIndices(registry, "Invalid", _tokens->rightHanded,
                         _BuildArray(numVerts),
                         _BuildArray(verts),
                         _BuildArray(hole),
                         _BuildArray(expected))) {
        return false;
    }
    if (!_CompareFaceVarying(registry, "InvalidFaceVarying", _tokens->rightHanded,
                             _BuildArray(numVerts),
                             _BuildArray(verts),
                             _BuildArray(hole),
                             _BuildArray(fvarValues),
                             _BuildArray(fvarExpected))) {
        return false;
    }


    return true;
}

int main()
{
    GlfTestGLContext::RegisterGLContextCallbacks();
    GlfSharedGLContextScopeHolder sharedContext;

    TfErrorMark mark;

    HgiUniquePtr const hgi = Hgi::CreatePlatformDefaultHgi();
    HdDriver driver{HgiTokens->renderDriver, VtValue(hgi.get())};
    HdStRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> const index(
        HdRenderIndex::New(&renderDelegate, {&driver}));
    HdStResourceRegistrySharedPtr const registry =
        std::static_pointer_cast<HdStResourceRegistry>(
        index->GetResourceRegistry());

    bool success = true;
    success &= BasicTest(registry);
    success &= HoleTest(registry);
    success &= ComputeNormalsTest(registry);
    success &= FaceVaryingTest(registry);
    success &= InvalidTopologyTest(registry);

    TF_VERIFY(mark.IsClean());

    if (success && mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

