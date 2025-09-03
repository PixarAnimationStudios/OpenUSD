//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdsi/pinnedCurveExpandingSceneIndex.h"

#include "pxr/imaging/hd/basisCurvesSchema.h"
#include "pxr/imaging/hd/basisCurvesTopologySchema.h"
#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/geomSubsetSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/vt/visitValue.h"

#include <vector>

#define USE_PARALLEL_EXPANSION 0

// TODO: This scene index doesn't account for time varying curve topology

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

// Returns the expanded value that is computed by replicating the first and last
// values `numRepeat` times per curve.  This may be applied to:
// - topology index buffers
// - primvar index buffers
// - (non-indexed) vertex and varying primvar values
template <typename T>
VtArray<T>
_ComputeExpandedValue(
    const VtArray<T> &input,
    const VtIntArray &perCurveCounts,
    const size_t numRepeat,
    const TfToken &name)
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

    if (input.size() != authoredSum) {
        TF_WARN("Data for %s does not match expected size "
                "(got %zu, expected %zu)", name.GetText(),
                input.size(), authoredSum);
        return input;
    }

    const size_t outputSize = input.size() + 2 * numRepeat * numCurves;
    VtArray<T> output(outputSize);

    const auto workLambda = [&](const size_t beginIdx, const size_t endIdx)
    {
        for (size_t curveIdx = beginIdx; curveIdx < endIdx; ++curveIdx) {
            // In the unlikely event of degenerate topology, we skip to the next
            // curve in order to avoid illegal indexing of the input that the
            // expansion code would perform if the zero-length curve were the
            // first or last curve.  Practically speaking, this just leaves
            // default-constructed values in the output; it doesn't mess up the
            // alignment of anything.
            if (ARCH_UNLIKELY(perCurveCounts[curveIdx] == 0)) {
                continue;
            }

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

    return output;
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


// Typed sampled data source override that expands the contents of the array
// produced by the input data source.
template <typename T>
class _ExpandedDataSource final : public HdTypedSampledDataSource<VtArray<T>>
{
public:
    using Time = HdSampledDataSource::Time;

    HD_DECLARE_DATASOURCE_ABSTRACT(_ExpandedDataSource<T>);

    /// input: the original data source
    _ExpandedDataSource(
        const HdSampledDataSourceHandle& input,
        const TfToken &primvarName,
        const VtIntArray& perCurveCounts,
        const size_t numExtraEnds)
        : _input(input)
        , _primvarName(primvarName)
        , _perCurveCounts(perCurveCounts)
        , _numExtraEnds(numExtraEnds)
    {
    }

    VtValue GetValue(Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

    bool GetContributingSampleTimesForInterval(
        Time startTime,
        Time endTime,
        std::vector<Time>* outSampleTimes) override
    {
        return _input->GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
    }

    VtArray<T> GetTypedValue(Time shutterOffset) override
    {
        const VtValue& v = _input->GetValue(shutterOffset);
        if (v.IsHolding<VtArray<T>>()) {
            const VtArray<T> array = v.UncheckedGet<VtArray<T>>();
            if (array.empty()) {
                return array;
            }

            return _ComputeExpandedValue<T>(
                array, _perCurveCounts, _numExtraEnds, _primvarName);
        }
        return VtArray<T>();
    }

    static typename _ExpandedDataSource<T>::Handle
    New(const HdSampledDataSourceHandle& input,
        const TfToken& primvarName,
        const VtIntArray& perCurveCounts,
        const size_t numExtraEnds)
    {
        return _ExpandedDataSource<T>::Handle(new _ExpandedDataSource<T>(
            input, primvarName, perCurveCounts, numExtraEnds));
    }

private:
    HdSampledDataSourceHandle _input;
    const TfToken _primvarName;
    const VtIntArray _perCurveCounts;
    size_t _numExtraEnds;
};


// Visitor that returns a contents-expanding data source wrapping the given
// input data source if it produced a VtArray, or returns the input data source
// otherwise.  The latter case is considered an error, as the caller will have
// already handled data sources expected to hold non-array values (e.g.
// 'constant' primvars or empty index buffers).
struct _Visitor
{
    HdSampledDataSourceHandle _input;
    const TfToken _primvarName;
    const VtIntArray& _perCurveCounts;
    size_t _numExtraEnds;

    template <typename T>
    HdDataSourceBaseHandle operator()(const VtArray<T>& array)
    {
        return _ExpandedDataSource<T>::New(
            _input, _primvarName, _perCurveCounts, _numExtraEnds);
    }

    HdDataSourceBaseHandle operator()(const VtValue& value)
    {
        TF_WARN(
            "Unsupported type for expansion %s", value.GetTypeName().c_str());
        return _input;
    }
};


// Primvar schema data source override that expands primvar values or indexed
// primvar indices for vertex- and varying-interpolation primvars.  If the
// curves are using an indexed topology, the expansion of that buffer by the
// _TopologyDataSource is taken into account with what's done to the primvar.
// See the Get() method for details.
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
    {
        if (ARCH_UNLIKELY(!_input)) {
            TF_CODING_ERROR("Invalid container data source input provided.");
            _input = _EmptyContainerDataSource::New();
        }
        if (ARCH_UNLIKELY(_numExtraEnds == 0)) {
            // XXX This is to prevent underflowing a size_t in Get()
            TF_CODING_ERROR("Invalid expansion size provided.");
            _numExtraEnds = 1;
        }
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

        // Non-indexed primvars are accessed in the 'primvarValue' data source,
        // while indexed primvars are in 'indexedPrimvarValue' and 'indices'.  A
        // primvar will never have both 'primvarValue' AND 'indices', but the
        // processing we need to do applies to either the (non-indexed) value,
        // or to the indices (but not values) of an indexed primvar.
        if (name == HdPrimvarSchemaTokens->primvarValue ||
            name == HdPrimvarSchemaTokens->indices) {
            HdPrimvarSchema pvs(_input);
            const TfToken interp = 
                _SafeGetTypedValue<TfToken>(pvs.GetInterpolation());

            // Expansion will only be necessary for some vertex and some varying
            // primvars.  Other interpolations and certain combinations of input
            // conditions will allow us to just return the input data source.
            size_t expansionSize = 0;

            if (interp == HdPrimvarSchemaTokens->vertex) {
                // If the topology has an index buffer for mapping vertices, the
                // expansion of the topology's indices is sufficient and nothing
                // needs to be done to the primvar itself.  Otherwise, we expand
                // the primvar values in the same way as the the vertices were
                // expanded (adding one or two duplicate values to each end of
                // each curve).
                expansionSize = _hasCurveIndices ? 0 : _numExtraEnds;
            }
            else if (interp == HdPrimvarSchemaTokens->varying) {
                // Varying primvars require one value for each curve segment,
                // plus an additional value for the end of the last segment.
                // However, the number of "logical" curve segments defined by a
                // given number of vertices is different for a 'pinned' curve
                // than the number of "literal" curve segments defined by a
                // 'nonperiodic' curve.  As this scene index is taking 'pinned'
                // curve input data and producing 'nonperiodic' curves, we need
                // to account for this difference; and what we do with varying
                // primvars depends on how many segments we're adding (how many
                // vertices we pad each end in the topology).
                //
                // The math on this (for each curve) works out to:
                //
                //   numInputSegs = (numInputVerts - 1)      [for 'pinned']
                //   numInputVals = numInputSegs + 1 == numInputVerts
                //
                //   numOutputSegs = (numOutputVerts - 3)    [for 'nonperiodic']
                //   numOutputVals = numOutputSegs + 1 == (numOutputVerts - 2)
                //
                // If we're adding one value to each end, then:
                //
                //   numOutputVerts = (numInputVerts + 2)
                //
                // ...which results in numOutputVals == numInputVals.
                //
                // But if we're adding two values to each end, then:
                //
                //   numOutputVerts = (numInputVerts + 4)
                //
                // ...which requires numOutputVals == (numInputVals + 2).
                //
                // What this all boils down to is: if _numExtraEnds is 1, we can
                // simply pass on the data source from the input; if it's 2, we
                // expand the data by 1 on each end.  Note we're guaranteed by a
                // check in the c'tor that _numExtraEnds will be >= 1.
                expansionSize = _numExtraEnds - 1;
            }

            if (expansionSize > 0) {
                // Use VtVisitValue to manufacture an appropriately-typed array-
                // expanding data source based on the held type of the primvar.
                // Note that the number of input values for a 'pinned' varying
                // primvar is the same as for a vertex primvar (so the size
                // checking logic in the value-expanding function applies
                // uniformly); but the number of output values will be different
                // for the two.
                if (const HdSampledDataSourceHandle sds =
                    HdSampledDataSource::Cast(result)) {
                    return VtVisitValue(
                        sds->GetValue(0.0f),
                        _Visitor { sds, _primvarName, _curveVertexCounts,
                                   expansionSize });
                }
            }
        }

        // If we got here, either:
        // - something other than the non-indexed value or indexed indices was
        //   requested
        // - this primvar is constant or uniform and doesn't need expansion
        // - this primvar is vertex but doesn't need expansion because the
        //   topology is indexed and that is being expanded
        // - this primvar is varying but doesn't need expansion due to the
        //   number of points being added
        // - a data source cast failed so we can't do anything more
        // In any case, return the unmodified data source to the caller.
        return result;
    }

private:
    HdContainerDataSourceHandle _input;
    const TfToken _primvarName;
    const VtIntArray _curveVertexCounts;
    size_t _numExtraEnds;
    bool _hasCurveIndices;
};


// Primvars schema data source override.
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
                const VtIntArray vExpanded =
                    _ComputeExpandedValue(
                        curveIndices,
                        _curveVertexCounts,
                        _numExtraEnds,
                        HdBasisCurvesTopologySchemaTokens->curveIndices);

                return HdRetainedTypedSampledDataSource<VtIntArray>::New(
                    vExpanded);
            }
        }

        if (name == HdBasisCurvesTopologySchemaTokens->wrap) {
            // Override to nonperiodic.
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
            return HdDataSourceBaseHandle();
        }
        if (name != HdBasisCurvesSchemaTokens->basisCurves &&
            name != HdPrimvarsSchemaTokens->primvars) {
            return result;
        }
        HdBasisCurvesSchema bcs = HdBasisCurvesSchema::GetFromParent(_input);
        if (!bcs) {
            return result;
        }
        HdBasisCurvesTopologySchema ts = bcs.GetTopology();
        if (!ts) {
            return result;
        }

        // TODO: Avoid sampling sampled sources here!
        const TfToken wrap =
            _SafeGetTypedValue<TfToken>(ts.GetWrap());
        const TfToken basis =
            _SafeGetTypedValue<TfToken>(ts.GetBasis());

        if (wrap == HdTokens->pinned &&
            (basis == HdTokens->bspline ||
             basis == HdTokens->catmullRom ||
             basis == HdTokens->centripetalCatmullRom)) {

            // Add 2 additional end points for bspline and
            // 1 for catmullRom|centripetalCatmullRom.
            const size_t numExtraEnds =
                (basis == HdTokens->bspline)? 2 : 1;
            
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
                // Varying primvars may still need to be expanded due to the
                // additional curve segments.
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
        return result;
    }

private:
    HdContainerDataSourceHandle _input;
};

class _SubsetIndicesDataSource : public HdIntArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_SubsetIndicesDataSource);
    
    VtValue
    GetValue(Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }
    
    VtIntArray
    GetTypedValue(Time shutterOffset) override
    {
        if (_typeSource->GetTypedValue(shutterOffset) ==
            HdGeomSubsetSchemaTokens->typePointSet) {
            // TODO: Remap geomsubset indices accounting for the additional
            //       curve points.
        }
        return _dataSource->GetTypedValue(shutterOffset);
    }
    
    bool
    GetContributingSampleTimesForInterval(
        Time startTime, Time endTime,
        std::vector<HdSampledDataSource::Time> *outSampleTimes) override
    {
        std::vector<HdSampledDataSourceHandle> sources { 
            _dataSource, _typeSource };
        const auto& topoSchema = HdBasisCurvesTopologySchema::GetFromParent(
            _parentSource);
        if (topoSchema) {
            sources.push_back(topoSchema.GetWrap());
            sources.push_back(topoSchema.GetBasis());
            sources.push_back(topoSchema.GetCurveVertexCounts());
        }
        return HdGetMergedContributingSampleTimesForInterval(
            sources.size(), sources.data(), startTime, endTime, outSampleTimes);
    }

private:
    _SubsetIndicesDataSource(
        const HdIntArrayDataSourceHandle& dataSource,
        const HdTokenDataSourceHandle& typeSource,
        const HdContainerDataSourceHandle& parentSource)
      : _dataSource(dataSource)
      , _typeSource(typeSource)
      , _parentSource(parentSource)
    {
        TF_VERIFY(dataSource);
        TF_VERIFY(typeSource);
        TF_VERIFY(parentSource);
    }
    
    HdIntArrayDataSourceHandle _dataSource;
    HdTokenDataSourceHandle _typeSource;
    HdContainerDataSourceHandle _parentSource;
};

HD_DECLARE_DATASOURCE_HANDLES(_SubsetIndicesDataSource);

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
    if (!prim.dataSource) {
        return prim;
    }

    // Override the prim data source for basis curves.
    if (prim.primType == HdPrimTypeTokens->basisCurves) {
        prim.dataSource = _PrimDataSource::New(prim.dataSource);
    }
    
    // Override the prim data source for geom subsets if parent is basis curves
    if (prim.primType == HdPrimTypeTokens->geomSubset) {
        const HdSceneIndexPrim parentPrim = _GetInputSceneIndex()->GetPrim(
            primPath.GetParentPath());
        if (parentPrim.primType == HdPrimTypeTokens->basisCurves &&
            parentPrim.dataSource) {
                
            // overlay indices
            // XXX: When basis curves support visible subsets,
            //      add support for subset primvars.
            prim.dataSource = HdOverlayContainerDataSource::New(
                HdRetainedContainerDataSource::New(
                    HdGeomSubsetSchemaTokens->indices,
                    _SubsetIndicesDataSource::New(
                        HdIntArrayDataSource::Cast(prim.dataSource->Get(
                            HdGeomSubsetSchemaTokens->indices)),
                        HdTokenDataSource::Cast(prim.dataSource->Get(
                            HdGeomSubsetSchemaTokens->type)),
                        parentPrim.dataSource)),
                prim.dataSource);
        }
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
