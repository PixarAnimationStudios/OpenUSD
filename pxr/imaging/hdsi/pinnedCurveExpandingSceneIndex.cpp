//
// Copyright 2022 Pixar
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
#include "pxr/imaging/hdsi/pinnedCurveExpandingSceneIndex.h"

#include "pxr/imaging/hd/basisCurvesSchema.h"
#include "pxr/imaging/hd/basisCurvesTopologySchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/work/loops.h"

#define USE_PARALLEL_EXPANSION 0

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

// Returns the expanded value that is computed by replicating the first and last
// values `numRepeat` times per curve. This is used for expanding index buffers,
// vertex primvars and possibly varying primvars of pinned curves.
// See relevant comments in _ComputeExpandedVaryingValue below.
template <typename T>
VtValue
_ComputeExpandedValue(
    const VtValue &value,
    const VtIntArray &perCurveCounts,
    const size_t numRepeat,
    const TfToken &name,
    bool expandConditionally = false)
{
    // Build cumulative sum arrays to help index into the authored and expanded
    // values per curve.
    const size_t numCurves = perCurveCounts.size();
    VtIntArray authoredStartIndices(numCurves);
    size_t idx = 0;
    size_t authoredSum = 0;
    for (int const &vc : perCurveCounts) {
        authoredStartIndices[idx++] = authoredSum;
        authoredSum += size_t(vc);
    }

    const VtArray<T> input = value.Get<VtArray<T>>();

    if (input.size() != authoredSum) {
        TF_WARN("Data for %s does not match expected size "
                "(got %zu, expected %zu)", name.GetText(),
                input.size(), authoredSum);
        return value;
    }

    const size_t outputSize = input.size() + 2 * numRepeat * numCurves;
    VtArray<T> output(outputSize);

    const auto workLambda = [&](const size_t beginIdx, const size_t endIdx)
    {
        for (size_t curveIdx = beginIdx; curveIdx < endIdx; ++curveIdx) {
            // input index range [start, end)
            const size_t inputStartIdx = authoredStartIndices[curveIdx];
            const size_t inputEndIdx =
                inputStartIdx + perCurveCounts[curveIdx];

            const size_t outStartIdx = inputStartIdx +
                                       2 * numRepeat * curveIdx;
            typename VtArray<T>::iterator outIt = output.begin() + outStartIdx;

            // Repeat the first value as necessary.
            outIt = std::fill_n(outIt, numRepeat, input[inputStartIdx]);

            // Copy authored data.
            outIt = std::copy(input.cbegin() + inputStartIdx,
                              input.cbegin() + inputEndIdx,
                              outIt);
            
            // Repeat the last value as necessary.
            outIt = std::fill_n(outIt, numRepeat, input[inputEndIdx - 1]);
        }
    };

    // Dirty data in Hydra is sync'd in parallel, so whether we benefit from
    // the additional parallelism below needs to be tested.
    #if USE_PARALLEL_EXPANSION
    // XXX Using a simple untested heuristic to divvy up the work.
    constexpr size_t numCurvesPerThread = 25;
    WorkParallelForN(numCurves, workLambda, numCurvesPerThread);
    #else
    workLambda(0, numCurves);
    #endif

    return VtValue(output);
}

// Type-dispatch boiler plate for the function template above.
VtValue
_DispatchComputeExpandedValue(
    const VtValue &val,
    const VtIntArray &perCurveCounts,
    const size_t numRepeat,
    const TfToken &name)
{
    VtValue vExp;

    if (val.IsHolding<VtVec2fArray>()) {
        vExp = _ComputeExpandedValue<GfVec2f>(
            val, perCurveCounts, numRepeat, name);
    } else if (val.IsHolding<VtVec2dArray>()) {
        vExp = _ComputeExpandedValue<GfVec2d>(
            val, perCurveCounts, numRepeat, name);
    } else if (val.IsHolding<VtVec2iArray>()) {
        vExp = _ComputeExpandedValue<GfVec2i>(
            val, perCurveCounts, numRepeat, name);
    } else if (val.IsHolding<VtVec2hArray>()) {
        vExp = _ComputeExpandedValue<GfVec2h>(
            val, perCurveCounts, numRepeat, name);
    } else if (val.IsHolding<VtVec3fArray>()) {
        vExp = _ComputeExpandedValue<GfVec3f>(
            val, perCurveCounts, numRepeat, name);
    } else if (val.IsHolding<VtVec3dArray>()) {
        vExp = _ComputeExpandedValue<GfVec3d>(
            val, perCurveCounts, numRepeat, name);
    } else if (val.IsHolding<VtVec3iArray>()) {
        vExp = _ComputeExpandedValue<GfVec3i>(
            val, perCurveCounts, numRepeat, name);
    } else if (val.IsHolding<VtVec3hArray>()) {
        vExp = _ComputeExpandedValue<GfVec3h>(
            val, perCurveCounts, numRepeat, name);
    } else if (val.IsHolding<VtVec4fArray>()) {
        vExp = _ComputeExpandedValue<GfVec4f>(
            val, perCurveCounts, numRepeat, name);
    } else if (val.IsHolding<VtVec4dArray>()) {
        vExp = _ComputeExpandedValue<GfVec4d>(
            val, perCurveCounts, numRepeat, name);
    } else if (val.IsHolding<VtVec4iArray>()) {
        vExp = _ComputeExpandedValue<GfVec4i>(
            val, perCurveCounts, numRepeat, name);
    } else if (val.IsHolding<VtVec4hArray>()) {
        vExp = _ComputeExpandedValue<GfVec4h>(
            val, perCurveCounts, numRepeat, name);
    } else if (val.IsHolding<VtMatrix4dArray>()) {
        vExp = _ComputeExpandedValue<GfMatrix4d>(
            val, perCurveCounts, numRepeat, name);
    } else if (val.IsHolding<VtStringArray>()) {
        vExp = _ComputeExpandedValue<std::string>(
            val, perCurveCounts, numRepeat, name);
    } else if (val.IsHolding<VtDoubleArray>()) {
        vExp = _ComputeExpandedValue<double>(
            val, perCurveCounts, numRepeat, name);
    } else if (val.IsHolding<VtIntArray>()) {
        vExp = _ComputeExpandedValue<int>(
            val, perCurveCounts, numRepeat, name);
    } else if (val.IsHolding<VtUIntArray>()) {
        vExp = _ComputeExpandedValue<unsigned int>(
            val, perCurveCounts, numRepeat, name);
    } else if (val.IsHolding<VtFloatArray>()) {
        vExp = _ComputeExpandedValue<float>(
            val, perCurveCounts, numRepeat, name);
    } else if (val.IsHolding<VtHalfArray>()) {
        vExp = _ComputeExpandedValue<GfHalf>(
            val, perCurveCounts, numRepeat, name);
    } else {
        TF_WARN("Unsupported primvar type %s", val.GetTypeName().c_str());
    }

    return vExp;
}

// Returns the expanded value that is computed by replicating the first and last
// values `numRepeat` times per curve when the vertex count is 4 or more, and
// repeating the authored varying values if/as necessary otherwise.
//
template <typename T>
VtValue
_ComputeExpandedVaryingValue(
    const VtValue &value,
    const VtIntArray &curveVaryingCounts,
    const VtIntArray &curveVertexCounts,
    const size_t numRepeat,
    const TfToken &name)
{
    if (!TF_VERIFY(curveVaryingCounts.size() == curveVertexCounts.size())) {
        return value;
    }

    // Build cumulative sum arrays to help index into the authored and expanded
    // varying values per curve.
    const size_t numCurves = curveVaryingCounts.size();
    VtIntArray authoredStartIndices(numCurves);
    VtIntArray expandedStartIndices(numCurves);
    size_t authoredSum = 0;
    size_t expandedSum = 0;

    for (size_t idx = 0; idx < numCurves; idx++) {
        const int numVarying = curveVaryingCounts[idx];
        const int numVertices = curveVertexCounts[idx];

        authoredStartIndices[idx] = authoredSum;
        authoredSum += size_t(numVarying);

        expandedStartIndices[idx] = expandedSum;

        if (numVertices >= 4) {
            expandedSum += size_t(numVarying) + 2 * numRepeat;

        } else if (numVertices >= 2) { // i.e., 2 or 3

            const size_t numExpandedVertices = numVertices + 2 * numRepeat;
            const size_t nSegs = numExpandedVertices - 3;
            const size_t nVarying = nSegs + 1;
            expandedSum += nVarying;
        }
    }

    const VtArray<T> input = value.Get<VtArray<T>>();

    if (input.size() != authoredSum) {
        TF_WARN("Data for %s does not match expected size "
                "(got %zu, expected %zu)", name.GetText(),
                input.size(), authoredSum);
        return value;
    }

    VtArray<T> output(expandedSum);

    const auto workLambda = [&](const size_t beginIdx, const size_t endIdx)
    {
        for (size_t curveIdx = beginIdx; curveIdx < endIdx; ++curveIdx) {
            // input index range [start, end)
            const size_t inputStartIdx = authoredStartIndices[curveIdx];
            const size_t outStartIdx   = expandedStartIndices[curveIdx];

            const size_t nextIdx = curveIdx + 1;

            const size_t inputEndIdx = (nextIdx != endIdx)?
                authoredStartIndices[nextIdx] : authoredSum;
            const size_t outEndIdx   = (nextIdx != endIdx)?
                expandedStartIndices[nextIdx] : expandedSum;
            
            const size_t inputSize  = inputEndIdx - inputStartIdx;
            const size_t outputSize = outEndIdx - outStartIdx;
            const size_t padding    = outputSize - inputSize;

            typename VtArray<T>::iterator outIt = output.begin() + outStartIdx;

            if (padding == 2 * numRepeat) {
                // Repeat the first value as necessary.
                outIt = std::fill_n(outIt, numRepeat, input[inputStartIdx]);

                // Copy authored data.
                outIt = std::copy(input.cbegin() + inputStartIdx,
                                  input.cbegin() + inputEndIdx,
                                  outIt);
                
                // Repeat the last value as necessary.
                outIt = std::fill_n(outIt, numRepeat, input[inputEndIdx - 1]);

            } else {
                // curveVertexCount is 2 or 3 requiring special handling for
                // varying primvar expansion. The possible scenarios are:
                //        |           AUTHORED       ||         EXPANDED
                // repeat | vtx count | varyingCount || vtx count | varyingCount
                // -------|--------------------------||-------------------------
                //    1   |     2     |       2      ||     4     |      2
                //    1   |     3     |       2      ||     5     |      3
                //    2   |     2     |       2      ||     6     |      4
                //    2   |     3     |       2      ||     7     |      5
                
                // Repeat the first value *if* necessary.
                outIt = std::fill_n(outIt,
                                    (padding + 1)/2,
                                    input[inputStartIdx]);

                // Copy authored data.
                outIt = std::copy(input.cbegin() + inputStartIdx,
                                  input.cbegin() + inputEndIdx,
                                  outIt);

                // Repeat the last value *if* necessary.
                outIt = std::fill_n(outIt,
                                    padding - (padding + 1)/2,
                                    input[inputEndIdx - 1]);
            }
        }
    };

    // Dirty data in Hydra is sync'd in parallel, so whether we benefit from
    // the additional parallelism below needs to be tested.
    #if USE_PARALLEL_EXPANSION
    // XXX Using a simple untested heuristic to divvy up the work.
    constexpr size_t numCurvesPerThread = 25;
    WorkParallelForN(numCurves, workLambda, numCurvesPerThread);
    #else
    workLambda(0, numCurves);
    #endif

    return VtValue(output);
}

// Type-dispatch boiler plate.
VtValue
_DispatchComputeExpandedVaryingValue(
    const VtValue &val,
    const VtIntArray &curveVaryingCounts,
    const VtIntArray &curveVertexCounts,
    const size_t numRepeat,
    const TfToken &name)
{
    VtValue vExp;

    if (val.IsHolding<VtVec2fArray>()) {
        vExp = _ComputeExpandedVaryingValue<GfVec2f>(
            val, curveVaryingCounts, curveVertexCounts, numRepeat, name);
    } else if (val.IsHolding<VtVec2dArray>()) {
        vExp = _ComputeExpandedVaryingValue<GfVec2d>(
            val, curveVaryingCounts, curveVertexCounts, numRepeat, name);
    } else if (val.IsHolding<VtVec2iArray>()) {
        vExp = _ComputeExpandedVaryingValue<GfVec2i>(
            val, curveVaryingCounts, curveVertexCounts, numRepeat, name);
    } else if (val.IsHolding<VtVec2hArray>()) {
        vExp = _ComputeExpandedVaryingValue<GfVec2h>(
            val, curveVaryingCounts, curveVertexCounts, numRepeat, name);
    } else if (val.IsHolding<VtVec3fArray>()) {
        vExp = _ComputeExpandedVaryingValue<GfVec3f>(
            val, curveVaryingCounts, curveVertexCounts, numRepeat, name);
    } else if (val.IsHolding<VtVec3dArray>()) {
        vExp = _ComputeExpandedVaryingValue<GfVec3d>(
            val, curveVaryingCounts, curveVertexCounts, numRepeat, name);
    } else if (val.IsHolding<VtVec3iArray>()) {
        vExp = _ComputeExpandedVaryingValue<GfVec3i>(
            val, curveVaryingCounts, curveVertexCounts, numRepeat, name);
    } else if (val.IsHolding<VtVec3hArray>()) {
        vExp = _ComputeExpandedVaryingValue<GfVec3h>(
            val, curveVaryingCounts, curveVertexCounts, numRepeat, name);
    } else if (val.IsHolding<VtVec4fArray>()) {
        vExp = _ComputeExpandedVaryingValue<GfVec4f>(
            val, curveVaryingCounts, curveVertexCounts, numRepeat, name);
    } else if (val.IsHolding<VtVec4dArray>()) {
        vExp = _ComputeExpandedVaryingValue<GfVec4d>(
            val, curveVaryingCounts, curveVertexCounts, numRepeat, name);
    } else if (val.IsHolding<VtVec4iArray>()) {
        vExp = _ComputeExpandedVaryingValue<GfVec4i>(
            val, curveVaryingCounts, curveVertexCounts, numRepeat, name);
    } else if (val.IsHolding<VtVec4hArray>()) {
        vExp = _ComputeExpandedVaryingValue<GfVec4h>(
            val, curveVaryingCounts, curveVertexCounts, numRepeat, name);
    } else if (val.IsHolding<VtMatrix4dArray>()) {
        vExp = _ComputeExpandedVaryingValue<GfMatrix4d>(
            val, curveVaryingCounts, curveVertexCounts, numRepeat, name);
    } else if (val.IsHolding<VtStringArray>()) {
        vExp = _ComputeExpandedVaryingValue<std::string>(
            val, curveVaryingCounts, curveVertexCounts, numRepeat, name);
    } else if (val.IsHolding<VtDoubleArray>()) {
        vExp = _ComputeExpandedVaryingValue<double>(
            val, curveVaryingCounts, curveVertexCounts, numRepeat, name);
    } else if (val.IsHolding<VtIntArray>()) {
        vExp = _ComputeExpandedVaryingValue<int>(
            val, curveVaryingCounts, curveVertexCounts, numRepeat, name);
    } else if (val.IsHolding<VtUIntArray>()) {
        vExp = _ComputeExpandedVaryingValue<unsigned int>(
            val, curveVaryingCounts, curveVertexCounts, numRepeat, name);
    } else if (val.IsHolding<VtFloatArray>()) {
        vExp = _ComputeExpandedVaryingValue<float>(
            val, curveVaryingCounts, curveVertexCounts, numRepeat, name);
    } else if (val.IsHolding<VtHalfArray>()) {
        vExp = _ComputeExpandedVaryingValue<GfHalf>(
            val, curveVaryingCounts, curveVertexCounts, numRepeat, name);
    } else {
        TF_WARN("Unsupported primvar type %s", val.GetTypeName().c_str());
    }

    return vExp;
}


template <typename T>
T
_SafeGetTypedValue(typename HdTypedSampledDataSource<T>::Handle ds)
{
    if (ds) {
        return ds->GetTypedValue(0.0f);
    }
    return T();
}

// A fallback container data source for use when an invalid one is provided
// when constructing the pinned curve data source overrides below.
class _EmptyContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_EmptyContainerDataSource);

    bool
    Has(const TfToken &name) override
    {
        return false;
    }

    TfTokenVector
    GetNames() override
    {
        return {};
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        return nullptr;
    }
};


// Primvar schema data source override that:
//  - expands vertex primvars for non-indexed curves (not to be confused with
//    indexed primvars mentioned below).
//  - expands varying primvars.
//  - expands indices for indexed primvars with vertex & varying interp.
class _PrimvarDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimvarDataSource);

    _PrimvarDataSource(
        const HdContainerDataSourceHandle &input,
        const TfToken &primvarName,
        const VtIntArray &curveVertexCounts,
        const size_t numExtraEnds,
        bool hasCurveIndices)
    : _input(input)
    , _primvarName(primvarName)
    , _curveVertexCounts(curveVertexCounts)
    , _numExtraEnds(numExtraEnds)
    , _hasCurveIndices(hasCurveIndices)
    , _expandVaryingConditionally(false)
    {
        if (ARCH_UNLIKELY(!_input)) {
            TF_CODING_ERROR("Invalid container data source input provided.");
            _input = _EmptyContainerDataSource::New();
        }
        // _curveVaryingCounts is initialized when necessary in Get(..).
    }

    bool
    Has(const TfToken &name) override
    {
        return _input->Has(name);
    }

    TfTokenVector
    GetNames() override
    {
        return _input->GetNames();
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        HdDataSourceBaseHandle result = _input->Get(name);
        if (!result) {
            return nullptr;
        }

        if (name == HdPrimvarSchemaTokens->primvarValue) {
            HdPrimvarSchema pvs(_input);
            const TfToken interp = 
                _SafeGetTypedValue<TfToken>(pvs.GetInterpolation());

            if (interp != HdPrimvarSchemaTokens->vertex &&
                interp != HdPrimvarSchemaTokens->varying) {
                // constant and uniform interp don't need expansion.
                // faceVarying isn't relevant for curves.
                return result;
            }

            if (_hasCurveIndices && interp == HdPrimvarSchemaTokens->vertex) {
                // Don't need to expand the primvar since the expanded curve
                // index buffer takes care of it.
                return result;
            }

            HdSampledDataSourceHandle sds = HdSampledDataSource::Cast(result);
            if (!sds) {
                return result;
            }

            if (interp == HdPrimvarSchemaTokens->varying &&
                _curveVaryingCounts.empty()) {
                _InitCurveVaryingCounts();
            }

            VtValue val = sds->GetValue(0.0f);
            VtValue vExp = _ComputeExpandedValue(val, interp, _primvarName);
            return HdRetainedTypedSampledDataSource<VtValue>::New(vExp);
        }
        
        if (name == HdPrimvarSchemaTokens->indices) {
            // For indexed primvars, only the indices needs to be expanded.
            // The indexedPrimvarValue doesn't.
            VtIntArray indices = _SafeGetTypedValue<VtIntArray>(
                HdIntArrayDataSource::Cast(result));
            
            if (!indices.empty()) {
                HdPrimvarSchema pvs(_input);
                const TfToken interp = _SafeGetTypedValue<TfToken>(
                    pvs.GetInterpolation());

                // Similar to the handling above, only expand indices for
                // vertex and varying primvars.
                if (interp != HdPrimvarSchemaTokens->vertex &&
                    interp != HdPrimvarSchemaTokens->varying) {
                    // constant and uniform interp don't need expansion.
                    // faceVarying isn't relevant for curves.
                    return result;
                }

                if (interp == HdPrimvarSchemaTokens->varying &&
                    _curveVaryingCounts.empty()) {
                    _InitCurveVaryingCounts();
                }

                VtValue vExp = _ComputeExpandedValue(
                    VtValue(indices), interp, HdTokens->indices);
                return HdRetainedTypedSampledDataSource<VtIntArray>::New(
                    vExp.UncheckedGet<VtIntArray>());
            }
        }

        return result;
    }

private:

    // Compute the expected number of authored varying primvars per curve.
    // Note that we still compute it as though it were non-periodic.
    void
    _InitCurveVaryingCounts()
    {
        const size_t numCurves = _curveVertexCounts.size();
        _curveVaryingCounts.resize(numCurves);
        for (size_t cId = 0; cId < numCurves; cId++) {
            const int &vertexCount = _curveVertexCounts[cId];
            int &varyingCount = _curveVaryingCounts[cId];

            if (vertexCount < 2) {
                varyingCount = 0;
                continue;
            }
            if (vertexCount < 4) {
                _expandVaryingConditionally = true;
            }

            // Note: We treat cv = 2 or 3 as a single segment requiring 2
            // authored varying values.
            const size_t nSegs =
                std::max(_curveVertexCounts[cId] - 4, 0) + 1;
            const size_t nVarying = nSegs + 1;
            varyingCount = nVarying;

        }
    }

    VtValue
    _ComputeExpandedValue(
        VtValue val,
        const TfToken &interp,
        const TfToken &name)
    {
        bool varying = (interp == HdPrimvarSchemaTokens->varying);
        const VtIntArray &perCurveCounts = varying?
            _curveVaryingCounts : _curveVertexCounts;

        const bool expandConditionally = varying && _expandVaryingConditionally;

        VtValue vExp;
        if (!expandConditionally) {
            vExp = _DispatchComputeExpandedValue(
                val, perCurveCounts, _numExtraEnds, name);
        } else {
            vExp = _DispatchComputeExpandedVaryingValue(
                val, perCurveCounts, _curveVertexCounts, _numExtraEnds, name);
        }
        
        return vExp;
    }

private:
    HdContainerDataSourceHandle _input;
    const TfToken _primvarName;
    const VtIntArray _curveVertexCounts;
    size_t _numExtraEnds;
    bool _hasCurveIndices;
    VtIntArray _curveVaryingCounts;
    bool _expandVaryingConditionally;
};


// Primvars schema data source override.
//
class _PrimvarsDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimvarsDataSource);

    _PrimvarsDataSource(
        const HdContainerDataSourceHandle &input,
        const VtIntArray &curveVertexCounts,
        const size_t &numExtraEnds,
        bool hasCurveIndices)
    : _input(input)
    , _curveVertexCounts(curveVertexCounts)
    , _numExtraEnds(numExtraEnds)
    , _hasCurveIndices(hasCurveIndices)
    {
        if (ARCH_UNLIKELY(!_input)) {
            TF_CODING_ERROR("Invalid container data source input provided.");
            _input = _EmptyContainerDataSource::New();
        }
    }

    bool
    Has(const TfToken &name) override
    {
        return _input->Has(name);
    }

    TfTokenVector
    GetNames() override
    {
        return _input->GetNames();
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        HdDataSourceBaseHandle result = _input->Get(name);
        if (result) {
            if (HdContainerDataSourceHandle pc =
                    HdContainerDataSource::Cast(result)) {
                return _PrimvarDataSource::New(
                    pc, name, _curveVertexCounts, _numExtraEnds,
                    _hasCurveIndices);
            }
        }

        return result;
    }

private:
    HdContainerDataSourceHandle _input;
    const VtIntArray _curveVertexCounts;
    size_t _numExtraEnds;
    bool _hasCurveIndices;
};


// Basis curves topology schema data source override.
//
class _TopologyDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_TopologyDataSource);

    _TopologyDataSource(
        const HdContainerDataSourceHandle &input,
        const VtIntArray &curveVertexCounts,
        const size_t &numExtraEnds)
    : _input(input)
    , _curveVertexCounts(curveVertexCounts)
    , _numExtraEnds(numExtraEnds)
    {
        if (ARCH_UNLIKELY(!_input)) {
            TF_CODING_ERROR("Invalid container data source input provided.");
            _input = _EmptyContainerDataSource::New();
        }
    }

    bool
    Has(const TfToken &name) override
    {
        return _input->Has(name);
    }

    TfTokenVector
    GetNames() override
    {
        return _input->GetNames();
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        HdBasisCurvesTopologySchema ts =
            HdBasisCurvesTopologySchema(_input);
        HdDataSourceBaseHandle result = _input->Get(name);

        if (name == HdBasisCurvesTopologySchemaTokens->curveVertexCounts) {
            VtIntArray curveVertexCounts = _curveVertexCounts;
            for (int &count : curveVertexCounts) {
                count += 2 * _numExtraEnds; // added to beginning and end...
            }

            return HdRetainedTypedSampledDataSource<VtIntArray>::New(
                curveVertexCounts);
        }

        if (name == HdBasisCurvesTopologySchemaTokens->curveIndices) {
            VtIntArray curveIndices = _SafeGetTypedValue<VtIntArray>(
                ts.GetCurveIndices());
            
            if (!curveIndices.empty()) {
                // Curve indices can be expanded just like we'd expand a
                // vertex primvar by replicating the first and last values as
                // necessary.
                VtValue vExpanded =
                    _ComputeExpandedValue<int>(
                        VtValue(curveIndices),
                        _curveVertexCounts,
                        _numExtraEnds,
                        HdBasisCurvesTopologySchemaTokens->curveIndices);

                return HdRetainedTypedSampledDataSource<VtIntArray>::New(
                    vExpanded.UncheckedGet<VtIntArray>());
            }
        }

        if (name == HdBasisCurvesTopologySchemaTokens->wrap) {
            // Override to nonPeriodic.
            return HdRetainedTypedSampledDataSource<TfToken>::New(
                    HdTokens->nonperiodic);
        }

        return result;
    }


private:
    HdContainerDataSourceHandle _input;
    const VtIntArray _curveVertexCounts;
    size_t _numExtraEnds;
};


// Basis curves schema data source override.
//
class _BasisCurvesDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_BasisCurvesDataSource);

    _BasisCurvesDataSource(
        const HdContainerDataSourceHandle &input,
        const VtIntArray &curveVertexCounts,
        const size_t &numExtraEnds)
    : _input(input)
    , _curveVertexCounts(curveVertexCounts)
    , _numExtraEnds(numExtraEnds)
    {
        if (ARCH_UNLIKELY(!_input)) {
            TF_CODING_ERROR("Invalid container data source input provided.");
            _input = _EmptyContainerDataSource::New();
        }
    }

    bool
    Has(const TfToken &name) override
    {
        return _input->Has(name);
    }

    TfTokenVector
    GetNames() override
    {
        return _input->GetNames();
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        HdDataSourceBaseHandle result = _input->Get(name);
        if (name == HdBasisCurvesSchemaTokens->topology) {
            if (HdContainerDataSourceHandle tc =
                    HdContainerDataSource::Cast(result)) {
                return _TopologyDataSource::New(
                    tc, _curveVertexCounts, _numExtraEnds);
            }
        }

        if (name == HdBasisCurvesSchemaTokens->geomSubsets) {
            // XXX Remap geomsubset indices accounting for the additional curve
            //     points.
        }
        return result;
    }


private:
    HdContainerDataSourceHandle _input;
    const VtIntArray _curveVertexCounts;
    size_t _numExtraEnds;
};


// Prim level data source override.
// The basis curves prim container has the following hierarchy:
// prim
//     basisCurvesSchema
//         topologySchema
//             curveVertexCounts
//             curveIndices
//             wrap
//             ...
//         geomsubsets
//     primvarsSchema
//         primvarSchema[]
//             primvarValue
//             indexedPrimvarValue
//     
class _PrimDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimDataSource);

    _PrimDataSource(
        const HdContainerDataSourceHandle &input)
    : _input(input)
    {
        if (ARCH_UNLIKELY(!_input)) {
            TF_CODING_ERROR("Invalid container data source input provided.");
            _input = _EmptyContainerDataSource::New();
        }
    }

    bool
    Has(const TfToken &name) override
    {
        return _input->Has(name);
    }

    TfTokenVector
    GetNames() override
    {
        return _input->GetNames();
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        HdDataSourceBaseHandle result = _input->Get(name);
        if (result) {
            HdBasisCurvesSchema bcs =
                HdBasisCurvesSchema::GetFromParent(_input);

            if (!bcs) {
                return result;
            }

            HdBasisCurvesTopologySchema ts = bcs.GetTopology();
            if (!ts) {
                return result;
            }

            const TfToken wrap =
                _SafeGetTypedValue<TfToken>(ts.GetWrap());
            const TfToken basis =
                _SafeGetTypedValue<TfToken>(ts.GetBasis());

            if (wrap == HdTokens->pinned &&
                (basis == HdTokens->bSpline ||
                 basis == HdTokens->catmullRom)) {

                // Add 2 additional end points for bSpline and
                // 1 for catmullRom.
                const size_t numExtraEnds =
                    (basis == HdTokens->bSpline)? 2 : 1;
                
                // Need to cache the per-curve vertex counts since the
                // expansion is per-curve.
                const VtIntArray curveVertexCounts =
                    _SafeGetTypedValue<VtIntArray>(
                        ts.GetCurveVertexCounts());

                if (name == HdBasisCurvesSchemaTokens->basisCurves) {
                    if (HdContainerDataSourceHandle bcc =
                            HdContainerDataSource::Cast(result)) {
                        return _BasisCurvesDataSource::New(
                            bcc, curveVertexCounts, numExtraEnds);
                    }
                }

                if (name == HdPrimvarsSchemaTokens->primvars) {
                    // If we have authored curve indices, we can avoid expanding
                    // vertex primvars by expanding the curve indices instead.
                    // Note that varying primvars would still need to be 
                    // expanded due to the additional curve segments.
                    VtIntArray curveIndices =
                        _SafeGetTypedValue<VtIntArray>(ts.GetCurveIndices());
                
                    if (HdContainerDataSourceHandle pc =
                            HdContainerDataSource::Cast(result)) {
                        return _PrimvarsDataSource::New(
                            pc, curveVertexCounts, numExtraEnds,
                            !curveIndices.empty());
                    }
                }
            }
        }
        return result;
    }

private:
    HdContainerDataSourceHandle _input;
};


} // namespace anonymous

////////////////////////////////////////////////////////////////////////////////

/* static */
HdsiPinnedCurveExpandingSceneIndexRefPtr
HdsiPinnedCurveExpandingSceneIndex::New(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
{
    return TfCreateRefPtr(
        new HdsiPinnedCurveExpandingSceneIndex(inputSceneIndex));
}


HdsiPinnedCurveExpandingSceneIndex::HdsiPinnedCurveExpandingSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
: HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
}

HdSceneIndexPrim
HdsiPinnedCurveExpandingSceneIndex::GetPrim(const SdfPath &primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    // Override the prim data source for basis curves.
    if (prim.primType == HdPrimTypeTokens->basisCurves && prim.dataSource) {
        prim.dataSource = _PrimDataSource::New(prim.dataSource);
    }

    return prim;
}

SdfPathVector
HdsiPinnedCurveExpandingSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    if (auto input = _GetInputSceneIndex()) {
        return input->GetChildPrimPaths(primPath);
    }

    return {};
}

void
HdsiPinnedCurveExpandingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    _SendPrimsAdded(entries);
}

void
HdsiPinnedCurveExpandingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    _SendPrimsRemoved(entries);
}

void
HdsiPinnedCurveExpandingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    _SendPrimsDirtied(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
