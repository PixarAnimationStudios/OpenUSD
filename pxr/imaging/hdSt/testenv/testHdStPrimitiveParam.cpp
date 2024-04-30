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

#include "pxr/imaging/hdSt/meshTopology.h"
#include "pxr/imaging/hdSt/quadrangulate.h"

#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/meshUtil.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/glf/testGLContext.h"
#include "pxr/imaging/hgi/hgi.h"

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

HdStResourceRegistrySharedPtr registry;

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
_ComparePrimitiveIDMap(std::string const & name,
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
_ComparePtexFaceIndex(std::string const & name,
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
    GlfSharedGLContextScopeHolder sharedContext;

    TfErrorMark mark;

    std::unique_ptr<Hgi> hgi = Hgi::CreatePlatformDefaultHgi();
    registry = std::make_shared<HdStResourceRegistry>(hgi.get());

    bool success = true;
    success &= PrimitiveIDMapTest();
    success &= PtexFaceIndexTest();

    TF_VERIFY(mark.IsClean());

    registry.reset();

    if (success && mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

