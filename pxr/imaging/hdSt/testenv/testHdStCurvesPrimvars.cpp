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

#include "pxr/imaging/hdSt/basisCurvesComputations.h"

#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
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

PXR_NAMESPACE_USING_DIRECTIVE

HdStResourceRegistrySharedPtr registry;

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

static bool
_ComparePoints(std::string const & name,
                VtIntArray numVerts, VtIntArray indices, VtVec3fArray points, 
                VtVec3fArray expected)
{
    HdBasisCurvesTopology m(HdTokens->cubic, HdTokens->bezier, 
        HdTokens->nonperiodic, numVerts, indices);

    // Convert topology to render delegate version
    HdSt_BasisCurvesTopologySharedPtr const rdTopology 
        = HdSt_BasisCurvesTopology::New(m);

    HdBufferSourceSharedPtr const source = HdBufferSourceSharedPtr(
        std::make_shared<
            HdSt_BasisCurvesPrimvarInterpolaterComputation<GfVec3f>>(
                rdTopology, points, SdfPath(), HdTokens->points,
                HdInterpolationVertex, GfVec3f(1, 0, 0),
                HdGetValueTupleType(VtValue(points)).type));

    HdBufferSpecVector bufferSpecs;
    source->GetBufferSpecs(&bufferSpecs);
    HdBufferArrayRangeSharedPtr const range =
        registry->AllocateNonUniformBufferArrayRange(
            HdTokens->primvar,
            bufferSpecs,
            HdBufferArrayUsageHintBitsVertex);
    registry->AddSource(range, source);

    // execute computation
    registry->Commit();

    const VtVec3fArray result = range->ReadData(HdTokens->points).
        Get<VtVec3fArray>();
    if (!_CompareArrays(result, expected)) {
        std::cout << name << " test failed:\n";
        std::cout << "  expected: " << expected << "\n";
        std::cout << "  result: " << result << "\n";
        return false;
    }
    return true;
}

static bool
_CompareVertexPrimvar(std::string const & name,
                VtIntArray numVerts, VtIntArray indices, VtFloatArray primvar, 
                VtFloatArray expected)
{
    HdBasisCurvesTopology m(HdTokens->cubic, HdTokens->bezier, 
        HdTokens->nonperiodic, numVerts, indices);

    // Convert topology to render delegate version
    HdSt_BasisCurvesTopologySharedPtr const rdTopology = 
        HdSt_BasisCurvesTopology::New(m);

    HdBufferSourceSharedPtr const source = HdBufferSourceSharedPtr(
        std::make_shared<HdSt_BasisCurvesPrimvarInterpolaterComputation<float>>(
            rdTopology, primvar, SdfPath(), HdTokens->primvar,
            HdInterpolationVertex, 0, 
            HdGetValueTupleType(VtValue(primvar)).type));

    HdBufferSpecVector bufferSpecs;
    source->GetBufferSpecs(&bufferSpecs);
    HdBufferArrayRangeSharedPtr const range =
        registry->AllocateNonUniformBufferArrayRange(
            HdTokens->primvar,
            bufferSpecs,
            HdBufferArrayUsageHintBitsVertex);
    registry->AddSource(range, source);

    // execute computation
    registry->Commit();

    const VtFloatArray result = range->ReadData(HdTokens->primvar).
        Get<VtFloatArray>();
    if (result != expected) {
        std::cout << name << " test failed:\n";
        std::cout << "  expected: " << expected << "\n";
        std::cout << "  result: " << result << "\n";
        return false;
    }
    return true;
}

bool
TopologyWithIndicesTest()
{
    {
        int numVerts[] = { 11 };
        int indices[] = { 0, 0, 0, 1, 2, 3, 4, 5, 6, 6, 6 };
        GfVec3f points[] = { GfVec3f(0, 0, 0),
                             GfVec3f(0, 0, 1),
                             GfVec3f(0, 0, 2),
                             GfVec3f(0, 0, 3),
                             GfVec3f(0, 0, 4),
                             GfVec3f(0, 0, 5),
                             GfVec3f(0, 0, 6) };
        // Since the indices buffer references all points below, the expected
        // result is the same.
        GfVec3f expected[] = {  GfVec3f(0, 0, 0),
                                GfVec3f(0, 0, 1),
                                GfVec3f(0, 0, 2),
                                GfVec3f(0, 0, 3),
                                GfVec3f(0, 0, 4),
                                GfVec3f(0, 0, 5),
                                GfVec3f(0, 0, 6) };

        if (!_ComparePoints("topology_w_indices_points_small",
                         _BuildArray(numVerts, sizeof(numVerts)/sizeof(int)),
                         _BuildArray(indices, sizeof(indices)/sizeof(int)),
                         _BuildArray(points, sizeof(points)/sizeof(GfVec3f)),
                         _BuildArray(expected, sizeof(expected)/sizeof(GfVec3f)))) {
            return false;
        }
    }

    {
        int numVerts[] = { 11 };
        int indices[] = { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
        GfVec3f points[] = { GfVec3f(0, 0, 0),
                             GfVec3f(0, 0, 1),
                             GfVec3f(0, 0, 2),
                             GfVec3f(0, 0, 3),
                             GfVec3f(0, 0, 4),
                             GfVec3f(0, 0, 5),
                             GfVec3f(0, 0, 6),
                             GfVec3f(0, 0, 7),
                             GfVec3f(0, 0, 8),
                             GfVec3f(0, 0, 9),
                             GfVec3f(0, 0, 10) };

        if (!_ComparePoints("topology_w_indices_points_equal",
                         _BuildArray(numVerts, sizeof(numVerts)/sizeof(int)),
                         _BuildArray(indices, sizeof(indices)/sizeof(int)),
                         _BuildArray(points, sizeof(points)/sizeof(GfVec3f)),
                         _BuildArray(points, sizeof(points)/sizeof(GfVec3f)))) {
            return false;
        }
    }

    {
        int numVerts[] = { 6 };
        int indices[] = { 2, 3, 4, 7, 8, 9 };
        GfVec3f points[] = { GfVec3f(0, 0, 0),
                             GfVec3f(0, 0, 1),
                             GfVec3f(0, 0, 2),
                             GfVec3f(0, 0, 3),
                             GfVec3f(0, 0, 4),
                             GfVec3f(0, 0, 5),
                             GfVec3f(0, 0, 6),
                             GfVec3f(0, 0, 7),
                             GfVec3f(0, 0, 8),
                             GfVec3f(0, 0, 9),
                             GfVec3f(0, 0, 10),
                             GfVec3f(0, 0, 11) };
        // The indices buffer references upto index 9, so we truncate unused
        // data.
        GfVec3f expected[] = { GfVec3f(0, 0, 0),
                             GfVec3f(0, 0, 1),
                             GfVec3f(0, 0, 2),
                             GfVec3f(0, 0, 3),
                             GfVec3f(0, 0, 4),
                             GfVec3f(0, 0, 5),
                             GfVec3f(0, 0, 6),
                             GfVec3f(0, 0, 7),
                             GfVec3f(0, 0, 8),
                             GfVec3f(0, 0, 9) };

        if (!_ComparePoints("topology_w_indices_points_big",
                         _BuildArray(numVerts, sizeof(numVerts)/sizeof(int)),
                         _BuildArray(indices, sizeof(indices)/sizeof(int)),
                         _BuildArray(points, sizeof(points)/sizeof(GfVec3f)),
                         _BuildArray(expected, sizeof(expected)/sizeof(GfVec3f)))) {
            return false;
        }
    }
    
    {
        int numVerts[] = { 6 };
        int indices[] = { 2, 3, 4, 7, 8, 9 };
        GfVec3f points[] = { GfVec3f(0, 0, 0),
                             GfVec3f(0, 0, 1),
                             GfVec3f(0, 0, 2),
                             GfVec3f(0, 0, 3),
                             GfVec3f(0, 0, 4),
                             GfVec3f(0, 0, 5)};
        // The indices buffer references upto index 9, while we have only 6
        // points authored (instead of 10).
        // "Fallback" value is used instead for all the points.
        GfVec3f expected[10];
        std::fill_n(expected, 10, GfVec3f(1, 0, 0));

        if (!_ComparePoints("topology_w_indices_points_insufficient",
                         _BuildArray(numVerts, sizeof(numVerts)/sizeof(int)),
                         _BuildArray(indices, sizeof(indices)/sizeof(int)),
                         _BuildArray(points, sizeof(points)/sizeof(GfVec3f)),
                         _BuildArray(expected, sizeof(expected)/sizeof(GfVec3f)))) {
            return false;
        }
    }
    
    {   
        int numVerts[] = { 11 };
        int indices[] = { 0, 0, 0, 1, 2, 3, 4, 5, 6, 6, 6 };
        float primvar[] = { 0, 1, 2, 3, 4, 5, 6};
        float expected[] = { 0, 1, 2, 3, 4, 5, 6};
        if (!_CompareVertexPrimvar("topology_w_indices_primvar_small",
                            _BuildArray(numVerts, sizeof(numVerts)/sizeof(int)),
                            _BuildArray(indices, sizeof(indices)/sizeof(int)),
                            _BuildArray(primvar, sizeof(primvar)/sizeof(float)),
                            _BuildArray(expected, sizeof(expected)/sizeof(float)))) {
            return false;
        }
    }

    {   
        int numVerts[] = { 11 };
        int indices[] = { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
        float primvar[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        if (!_CompareVertexPrimvar("topology_w_indices_primvar_equal",
                            _BuildArray(numVerts, sizeof(numVerts)/sizeof(int)),
                            _BuildArray(indices, sizeof(indices)/sizeof(int)),
                            _BuildArray(primvar, sizeof(primvar)/sizeof(float)),
                            _BuildArray(primvar, sizeof(primvar)/sizeof(float)))) {
            return false;
        }
    }

    {   
        int numVerts[] = { 6 };
        int indices[] = { 2, 4, 8, 8, 9, 0 };
        float primvar[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        float expected[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        if (!_CompareVertexPrimvar("topology_w_indices_primvar_big",
                            _BuildArray(numVerts, sizeof(numVerts)/sizeof(int)),
                            _BuildArray(indices, sizeof(indices)/sizeof(int)),
                            _BuildArray(primvar, sizeof(primvar)/sizeof(float)),
                            _BuildArray(expected, sizeof(expected)/sizeof(float)))) {
            return false;
        }
    }

    {   
        int numVerts[] = { 6 };
        int indices[] = { 2, 4, 8, 8, 0 };
        float primvar[]  = { 0, 1, 2, 3, 4, 5, 6};
        // Indices references upto index 8, while only 7 primvar values are
        // authored (instead of 9). "Fallback" value is used instead for the
        // primvar.
        float expected[9];
        std::fill_n(expected, 9, 0);
        if (!_CompareVertexPrimvar("topology_w_indices_primvar_insufficient",
                            _BuildArray(numVerts, sizeof(numVerts)/sizeof(int)),
                            _BuildArray(indices, sizeof(indices)/sizeof(int)),
                            _BuildArray(primvar, sizeof(primvar)/sizeof(float)),
                            _BuildArray(expected, sizeof(expected)/sizeof(float)))) {
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

    static HgiUniquePtr _hgi = Hgi::CreatePlatformDefaultHgi();
    registry = std::make_shared<HdStResourceRegistry>(_hgi.get());

    bool success = true;
    success &= TopologyWithIndicesTest();

    registry->GarbageCollect();
    registry.reset();
    
    TF_VERIFY(mark.IsClean());

    if (success && mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

