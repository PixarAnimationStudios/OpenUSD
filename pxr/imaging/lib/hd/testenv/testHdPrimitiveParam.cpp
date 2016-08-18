#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/meshTopology.h"

#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/triangulate.h"
#include "pxr/imaging/hd/quadrangulate.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/testGLContext.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
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

bool
_ComparePrimitiveIDMap(std::string const & name,
                       std::string const & orientation,
                       VtIntArray numVerts, VtIntArray verts,
                       VtIntArray expectedMapping,
                       bool quadrangulate)
{
    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();

    HdMeshTopology m(_tokens->bilinear, TfToken(orientation), numVerts, verts);
    SdfPath id(name);

    HdBufferSourceSharedPtr source;
    if (quadrangulate) {
        Hd_QuadInfoBuilderComputationSharedPtr quadInfo =
            m.GetQuadInfoBuilderComputation(/*gpu=*/false, id);
        registry->AddSource(quadInfo);
        source = m.GetQuadIndexBuilderComputation(id);
    } else {
        source = m.GetTriangleIndexBuilderComputation(id);
    }

    HdBufferSpecVector bufferSpecs;
    source->AddBufferSpecs(&bufferSpecs);
    HdBufferArrayRangeSharedPtr range =
        registry->AllocateNonUniformBufferArrayRange(
            HdTokens->topology, bufferSpecs);
    registry->AddSource(range, source);

    // execute
    registry->Commit();

    // index compare
    // retrieve result
    VtValue resultValue = range->ReadData(HdTokens->primitiveParam);

    std::cout << "Index Results\n";
    std::cout << resultValue << "\n";

    VtIntArray faceIndices;

    // resultValue can be VtIntArray (tri) or VtVec2iArray (quad)
    if (resultValue.IsHolding< VtIntArray >()) {
        VtIntArray result = resultValue.Get< VtIntArray >();
        for (int i=0; i<result.size(); ++i) {
            faceIndices.push_back(HdMeshTopology::DecodeFaceIndexFromCoarseFaceParam(result[i]));
        }
    } else if (resultValue.IsHolding< VtVec2iArray >()) {
        VtVec2iArray result = resultValue.Get< VtVec2iArray >();
        for (int i=0; i<result.size(); ++i) {
            faceIndices.push_back(HdMeshTopology::DecodeFaceIndexFromCoarseFaceParam(result[i][0]));
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
_ComparePtexFaceIndex(std::string const & name,
                      std::string const & orientation,
                      VtIntArray numVerts, VtIntArray verts,
                      VtIntArray expectedMapping)
{
    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();

    HdMeshTopology m(_tokens->bilinear, TfToken(orientation), numVerts, verts);

    HdBufferSourceSharedPtr source;

    Hd_QuadInfoBuilderComputationSharedPtr quadInfo =
        m.GetQuadInfoBuilderComputation(/*gpu=*/false, SdfPath(name));
    registry->AddSource(quadInfo);
    source = m.GetQuadIndexBuilderComputation(SdfPath(name));

    HdBufferSpecVector bufferSpecs;
    source->AddBufferSpecs(&bufferSpecs);
    HdBufferArrayRangeSharedPtr range =
        registry->AllocateNonUniformBufferArrayRange(
            HdTokens->topology, bufferSpecs);
    registry->AddSource(range, source);

    // execute
    registry->Commit();

    // index compare
    // retrieve result
    VtValue resultValue = range->ReadData(HdTokens->primitiveParam);

    std::cout << "Index Results\n";
    std::cout << resultValue << "\n";

    VtIntArray ptexIndices;
    if (resultValue.IsHolding< VtVec2iArray >()) {
        VtVec2iArray result = resultValue.Get< VtVec2iArray >();
        for (int i=0; i<result.size(); ++i) {
            ptexIndices.push_back(result[i][1]);
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

#define COMPARE_PRIMITIVE_ID_MAP_TRI(name, orientation, numVerts, verts, expectedMapping) \
    _ComparePrimitiveIDMap(name, orientation,                           \
                           _BuildArray(numVerts, sizeof(numVerts)/sizeof(numVerts[0])), \
                           _BuildArray(verts, sizeof(verts)/sizeof(verts[0])), \
                           _BuildArray(expectedMapping, sizeof(expectedMapping)/sizeof(expectedMapping[0])), \
                           false)

#define COMPARE_PRIMITIVE_ID_MAP_QUAD(name, orientation, numVerts, verts, expectedMapping) \
    _ComparePrimitiveIDMap(name, orientation,                           \
                           _BuildArray(numVerts, sizeof(numVerts)/sizeof(numVerts[0])), \
                           _BuildArray(verts, sizeof(verts)/sizeof(verts[0])), \
                           _BuildArray(expectedMapping, sizeof(expectedMapping)/sizeof(expectedMapping[0])), \
                           true)

#define COMPARE_PTEX_FACE_INDEX(name, orientation, numVerts, verts, expectedMapping) \
    _ComparePtexFaceIndex(name, orientation,                           \
                          _BuildArray(numVerts, sizeof(numVerts)/sizeof(numVerts[0])), \
                          _BuildArray(verts, sizeof(verts)/sizeof(verts[0])), \
                          _BuildArray(expectedMapping, sizeof(expectedMapping)/sizeof(expectedMapping[0])))
bool
PrimitiveIDMapTest()
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

        if (!COMPARE_PRIMITIVE_ID_MAP_TRI("triangle", _tokens->rightHanded,
                                      numVerts, verts, expectedTri)) {
            return false;
        }
        if (!COMPARE_PRIMITIVE_ID_MAP_QUAD("triangle", _tokens->rightHanded,
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

        if (!COMPARE_PRIMITIVE_ID_MAP_TRI("quad", _tokens->rightHanded,
                                      numVerts, verts, expectedTri)) {
            return false;
        }
        if (!COMPARE_PRIMITIVE_ID_MAP_QUAD("quad", _tokens->rightHanded,
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

        if (!COMPARE_PRIMITIVE_ID_MAP_TRI("polygons", _tokens->rightHanded,
                                      numVerts, verts, expectedTri)) {
            return false;
        }
        if (!COMPARE_PRIMITIVE_ID_MAP_QUAD("polygons", _tokens->rightHanded,
                                           numVerts, verts, expectedQuad)) {
            return false;
        }
    }
    return true;
}

bool
PtexFaceIndexTest()
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

        if (!COMPARE_PTEX_FACE_INDEX("polygons", _tokens->rightHanded,
                                     numVerts, verts, expectedQuad)) {
            return false;
        }
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
    success &= PrimitiveIDMapTest();
    success &= PtexFaceIndexTest();

    TF_VERIFY(mark.IsClean());

    if (success and mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

