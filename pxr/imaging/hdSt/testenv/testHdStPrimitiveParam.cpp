//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdSt/meshTopology.h"
#include "pxr/imaging/hdSt/quadrangulate.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"

#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/driver.h"
#include "pxr/imaging/hd/meshUtil.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/glf/testGLContext.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/staticTokens.h"

#include "pxr/usd/sdf/path.h"

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
    for (int i=0; i<result.size(); ++i) {
        if (!GfIsClose(result[i][0], expected[i][0], 1e-6) ||
            !GfIsClose(result[i][1], expected[i][1], 1e-6) ||
            !GfIsClose(result[i][2], expected[i][2], 1e-6)) {
            return false;
        }
    }
    return true;
}

bool
_ComparePrimitiveIDMap(HdStResourceRegistrySharedPtr const &registry,
                       std::string const & name,
                       std::string const & orientation,
                       VtIntArray numVerts, VtIntArray verts,
                       VtIntArray expectedMapping,
                       bool quadrangulate)
{
    HdMeshTopology topology(_tokens->bilinear,
                            TfToken(orientation),
                            numVerts,
                            verts);
    HdSt_MeshTopologySharedPtr m = HdSt_MeshTopology::New(topology, 0);
    SdfPath id(name);

    HdBufferSourceSharedPtr source;
    if (quadrangulate) {
        HdSt_QuadInfoBuilderComputationSharedPtr quadInfo =
            m->GetQuadInfoBuilderComputation(/*gpu=*/false, id);
        registry->AddSource(quadInfo);
        source = m->GetQuadIndexBuilderComputation(id);
    } else {
        source = m->GetTriangleIndexBuilderComputation(id);
    }

    HdBufferSpecVector bufferSpecs;
    source->GetBufferSpecs(&bufferSpecs);
    HdBufferArrayRangeSharedPtr range =
        registry->AllocateNonUniformBufferArrayRange(
            HdTokens->topology, bufferSpecs, HdBufferArrayUsageHintBitsIndex);
    registry->AddSource(range, source);

    // execute
    registry->Commit();

    // index compare
    // retrieve result
    VtValue resultValue = range->ReadData(HdTokens->primitiveParam);

    std::cout << "Index Results\n";
    std::cout << resultValue << "\n";

    VtIntArray faceIndices;

    // resultValue is VtIntArray (tri or quad)
    if (resultValue.IsHolding< VtIntArray >()) {
        VtIntArray result = resultValue.Get< VtIntArray >();
        for (size_t i=0; i<result.size(); ++i) {
            faceIndices.push_back(HdMeshUtil::DecodeFaceIndexFromCoarseFaceParam(result[i]));
        }
    } else {
        std::cout << name << " test failed:\n";
        std::cout << "  wrong returned value type:\n";
        return false;
    }

    if (faceIndices != expectedMapping) {
        std::cout << name << " test failed:\n";
        std::cout << "  expected: " << expectedMapping << "\n";
        std::cout << "  result: " << faceIndices << "\n";
        return false;
    }

    return true;
}

bool
_ComparePtexFaceIndex(HdStResourceRegistrySharedPtr const &registry,
                      std::string const & name,
                      std::string const & orientation,
                      VtIntArray numVerts, VtIntArray verts,
                      VtIntArray expectedMapping)
{
    HdMeshTopology topology(_tokens->bilinear,
                            TfToken(orientation),
                            numVerts,
                            verts);
    HdSt_MeshTopologySharedPtr m = HdSt_MeshTopology::New(topology, 0);

    HdBufferSourceSharedPtr source;

    HdSt_QuadInfoBuilderComputationSharedPtr quadInfo =
        m->GetQuadInfoBuilderComputation(/*gpu=*/false, SdfPath(name));
    registry->AddSource(quadInfo);
    source = m->GetQuadIndexBuilderComputation(SdfPath(name));

    HdBufferSpecVector bufferSpecs;
    source->GetBufferSpecs(&bufferSpecs);
    HdBufferArrayRangeSharedPtr range =
        registry->AllocateNonUniformBufferArrayRange(
            HdTokens->topology, bufferSpecs, HdBufferArrayUsageHintBitsIndex);
    registry->AddSource(range, source);

    // execute
    registry->Commit();

    // index compare
    // retrieve result
    VtValue resultValue = range->ReadData(HdTokens->primitiveParam);

    std::cout << "Index Results\n";
    std::cout << resultValue << "\n";

    VtIntArray ptexIndices;
    if (resultValue.IsHolding< VtIntArray >()) {
        VtIntArray result = resultValue.Get< VtIntArray >();
        for (size_t i=0; i<result.size(); ++i) {
            ptexIndices.push_back(i);
        }
    } else {
        std::cout << name << " test failed:\n";
        std::cout << "  wrong returned value type:\n";
        return false;
    }

    if (ptexIndices != expectedMapping) {
        std::cout << name << " test failed:\n";
        std::cout << "  expected: " << expectedMapping << "\n";
        std::cout << "  result: " << ptexIndices << "\n";
        return false;
    }

    return true;
}

#define COMPARE_PRIMITIVE_ID_MAP_TRI(registry, name, orientation, numVerts, verts, expectedMapping) \
    _ComparePrimitiveIDMap(registry, name, orientation,                           \
                           _BuildArray(numVerts, sizeof(numVerts)/sizeof(numVerts[0])), \
                           _BuildArray(verts, sizeof(verts)/sizeof(verts[0])), \
                           _BuildArray(expectedMapping, sizeof(expectedMapping)/sizeof(expectedMapping[0])), \
                           false)

#define COMPARE_PRIMITIVE_ID_MAP_QUAD(registry, name, orientation, numVerts, verts, expectedMapping) \
    _ComparePrimitiveIDMap(registry, name, orientation,                           \
                           _BuildArray(numVerts, sizeof(numVerts)/sizeof(numVerts[0])), \
                           _BuildArray(verts, sizeof(verts)/sizeof(verts[0])), \
                           _BuildArray(expectedMapping, sizeof(expectedMapping)/sizeof(expectedMapping[0])), \
                           true)

#define COMPARE_PTEX_FACE_INDEX(registry, name, orientation, numVerts, verts, expectedMapping) \
    _ComparePtexFaceIndex(registry, name, orientation,                           \
                          _BuildArray(numVerts, sizeof(numVerts)/sizeof(numVerts[0])), \
                          _BuildArray(verts, sizeof(verts)/sizeof(verts[0])), \
                          _BuildArray(expectedMapping, sizeof(expectedMapping)/sizeof(expectedMapping[0])))
bool
PrimitiveIDMapTest(HdStResourceRegistrySharedPtr const &registry)
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();

    perfLog.ResetCounters();
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 0);

    {
        // triangle
        //
        // +-----------+    +-----------+
        //  \         /      \  0 | 0  /
        //   \   0   /        \ __+__ /
        //    \     /          \     /
        //     \   /            \ 0 /
        //      \ /              \ /
        //       +                +
        //

        int numVerts[] = { 3 };
        int verts[] = { 0, 1, 2 };
        int expectedTri[] = { 0 };
        int expectedQuad[] = { 0, 0, 0 };

        if (!COMPARE_PRIMITIVE_ID_MAP_TRI(
                registry, "triangle", _tokens->rightHanded,
                numVerts, verts, expectedTri)) {
            return false;
        }
        if (!COMPARE_PRIMITIVE_ID_MAP_QUAD(
                registry, "triangle", _tokens->rightHanded,
                numVerts, verts, expectedQuad)) {
            return false;
        }
    }
    {
        // quad
        //
        // +-----------+   +-----------+
        // |\_         |   |           |
        // |  \_   0   |   |           |
        // |    \_     |   |     0     |
        // |      \_   |   |           |
        // |  0     \_ |   |           |
        // |          \|   |           |
        // +-----------+   +-----------+

        int numVerts[] = { 4 };
        int verts[] = { 0, 1, 2, 3 };
        int expectedTri[] = { 0, 0 };
        int expectedQuad[] = { 0 };

        if (!COMPARE_PRIMITIVE_ID_MAP_TRI(
            registry, "quad", _tokens->rightHanded,
            numVerts, verts, expectedTri)) {
            return false;
        }
        if (!COMPARE_PRIMITIVE_ID_MAP_QUAD(
            registry, "quad", _tokens->rightHanded,
            numVerts, verts, expectedQuad)) {
            return false;
        }
    }
    {
        /*
          Element ID
                 +--------+-------+                 +--------+-------+
                /| \      |\      |\               /|        |    |   \
               / |  \  1  | \  2  | \             / |        |  2 | 2 /\
              /  |   \    |  \    |  \           /  |        |     \ /  \
             /   |    \   |   \   | 2 +         / 0 |    1   |------+  2 +
            / 0  |  1  \  | 2  \  |  /         /\  /|        |     / \  /
           /     |      \ |     \ | /         /  \/ |        |  2 | 2 \/
          /      |       \|      \|/         / 0 | 0|        |    |   /
         +-------+--------+-------+         +-------+--------+-------+

         */
        int numVerts[] = { 3, 4, 5 };
        int verts[] = { 0, 1, 2,
                        0, 2, 3, 4,
                        4, 3, 5, 6, 7 };
        int expectedTri[] = { 0, 1, 1, 2, 2, 2 };
        int expectedQuad[] = { 0, 0, 0, 1, 2, 2, 2, 2, 2 };

        if (!COMPARE_PRIMITIVE_ID_MAP_TRI(
            registry, "polygons", _tokens->rightHanded,
            numVerts, verts, expectedTri)) {
            return false;
        }
        if (!COMPARE_PRIMITIVE_ID_MAP_QUAD(
            registry, "polygons", _tokens->rightHanded,
            numVerts, verts, expectedQuad)) {
            return false;
        }
    }
    return true;
}

bool
PtexFaceIndexTest(HdStResourceRegistrySharedPtr const &registry)
{
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();

    perfLog.ResetCounters();
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->bufferSourcesResolved) == 0);
    TF_VERIFY(perfLog.GetCounter(HdPerfTokens->computationsCommited) == 0);

    {
        /*
          Ptex Face Id
                    +--------+-------+
                   /|        |    |   \
                  / |        |  4 | 8 /\
                 /  |        |     \ /  \
                / 0 |    3   |------+  7 +
               /\  /|        |     / \  /
              /  \/ |        |  5 | 6 \/
             / 1 | 2|        |    |   /
            +-------+--------+-------+

         */
        int numVerts[] = { 3, 4, 5 };
        int verts[] = { 0, 1, 2,
                        0, 2, 3, 4,
                        4, 3, 5, 6, 7 };
        int expectedQuad[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };

        if (!COMPARE_PTEX_FACE_INDEX(registry, "polygons", _tokens->rightHanded,
                                     numVerts, verts, expectedQuad)) {
            return false;
        }
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
    success &= PrimitiveIDMapTest(registry);
    success &= PtexFaceIndexTest(registry);

    TF_VERIFY(mark.IsClean());

    if (success && mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

