//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdSkelImaging/blendShapeData.h"

#include "pxr/usdImaging/usdSkelImaging/blendShapeSchema.h"
#include "pxr/usdImaging/usdSkelImaging/bindingSchema.h"
#include "pxr/usdImaging/usdSkelImaging/dataSourceUtils.h"
#include "pxr/usdImaging/usdSkelImaging/inbetweenShapeSchema.h"

#include "pxr/imaging/hd/sceneIndex.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/trace/trace.h"

#include <limits>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// Same as usdSkel/blendShapeQuery.cpp
const float EPS = 1e-6;

using _PointIndexAndOffset = std::pair<unsigned int, GfVec4f>;

// Wrapper for HdSceneIndexBase::GetPrim to have a TRACE_FUNCTION.
//
// We could cache some data in the UsdSkelImaging filtering scene indices
// if we see much time spend here.
//
HdSceneIndexPrim
_GetPrim(HdSceneIndexBaseRefPtr const &sceneIndex,
         const SdfPath &primPath)
{
    TRACE_FUNCTION();

    return sceneIndex->GetPrim(primPath);
}

// Pack offset and index of subshape for one point.
GfVec4f
_ToVec4f(const GfVec3f &a, const int b)
{
    return GfVec4f(a[0], a[1], a[2], b);
}

void
_FillPointIndicesAndOffsetsDense(
    const VtArray<GfVec3f> &offsets,
    const int subShape,
    std::vector<_PointIndexAndOffset> * const pointIndicesAndOffsets)
{
    TRACE_FUNCTION();

    for (size_t i = 0; i < offsets.size(); ++i) {
        pointIndicesAndOffsets->emplace_back(
            i, _ToVec4f(offsets[i], subShape));
    }
}

bool
_FillPointIndicesAndOffsetsSparse(
    const SdfPath &blendShapePrimPath /* only for warnings/error messages */,
    const TfToken &inbetweenName /* only used for warnings/error message */,
    const VtIntArray &indices,
    const VtArray<GfVec3f> &offsets,
    const int subShape,
    std::vector<_PointIndexAndOffset> * const pointIndicesAndOffsets)
{
    TRACE_FUNCTION();

    if (offsets.size() != indices.size()) {
        TF_WARN(
            "Length (%zu) of offsets%s%s on BlendShape prim %s does not "
            "match length (%zu) of indices.\n",
            offsets.size(),
            inbetweenName.IsEmpty() ? "" : " for inbetween ",
            inbetweenName.GetText(),
            blendShapePrimPath.GetText(),
            indices.size());
    }

    const size_t n = std::min(offsets.size(), indices.size());

    bool warningEmitted = false;

    for (size_t i = 0; i < n; ++i) {
        if (indices[i] < 0) {
            if (!warningEmitted) {
                TF_WARN(
                    "The indices on BlendShape prim %s has negative numbers.\n",
                    blendShapePrimPath.GetText());
                warningEmitted = true;
            }
            continue;
        }
        pointIndicesAndOffsets->emplace_back(
            indices[i], _ToVec4f(offsets[i], subShape));
    }

    return true;
}

void
_FillPointIndicesAndOffsets(
    const SdfPath &blendShapePrimPath /* only for warnings/error messages */,
    const TfToken &inbetweenName /* only used for warnings/error message */,
    const VtIntArray &indices,
    const VtArray<GfVec3f> &offsets,
    const int subShape,
    std::vector<_PointIndexAndOffset> * const pointIndicesAndOffsets)
{
    if (indices.empty()) {
        _FillPointIndicesAndOffsetsDense(
            offsets,
            subShape,
            pointIndicesAndOffsets);
    } else {
        _FillPointIndicesAndOffsetsSparse(
            blendShapePrimPath,
            inbetweenName,
            indices,
            offsets,
            subShape,
            pointIndicesAndOffsets);
    }
}

void
_ProcessBlendShapePrim(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const SdfPath &primPath /* only for warnings/error messages */,
    const SdfPath &blendShapePrimPath,
    size_t * numSubShapes,
    UsdSkelImagingWeightsAndSubShapeIndices * const weightsAndSubShapeIndices,
    std::vector<_PointIndexAndOffset> * const pointIndicesAndOffsets)
{
    TRACE_FUNCTION();

    const HdSceneIndexPrim blendShapePrim =
        _GetPrim(sceneIndex, blendShapePrimPath);
    const UsdSkelImagingBlendShapeSchema blendShapeSchema =
        UsdSkelImagingBlendShapeSchema::GetFromParent(
            blendShapePrim.dataSource);
    if (!blendShapeSchema) {
        TF_WARN(
            "No valid blend shape prim at %s which is among the "
            "blendShapeTargets of prim %s.\n",
            blendShapePrimPath.GetText(),
            primPath.GetText());
        return;
    }

    const VtIntArray indices =
        UsdSkelImagingGetTypedValue(
            blendShapeSchema.GetPointIndices());

    struct WeightAndOffsets {
        // See UsdSkelImagingWeightAndSubShapeIndex::weight
        float weight;
        HdVec3fArrayDataSourceHandle offsetsDataSource;
        // Empty if not inbetween
        TfToken inbetweenName;

        bool operator<(const WeightAndOffsets &other) const {
            return weight < other.weight;
        }
    };

    std::vector<WeightAndOffsets> weightsAndOffsets;

    weightsAndOffsets.push_back(
        {0.0f, nullptr, TfToken()});
    weightsAndOffsets.push_back(
        {1.0f, blendShapeSchema.GetOffsets(), TfToken()});

    const UsdSkelImagingInbetweenShapeContainerSchema containerSchema =
        blendShapeSchema.GetInbetweenShapes();
    for (const TfToken &name : containerSchema.GetNames()) {
        const UsdSkelImagingInbetweenShapeSchema inbetweenSchema =
            containerSchema.Get(name);
        HdFloatDataSourceHandle const weightDs = inbetweenSchema.GetWeight();
        if (!weightDs) {
            TF_WARN(
                "Inbetween %s on BlendShape prim %s has no weight.\n",
                name.GetText(), blendShapePrimPath.GetText());
            continue;
        }
        const float weight = weightDs->GetTypedValue(0.0f);
        if (GfIsClose(weight, 0.0, EPS) || GfIsClose(weight, 1.0, EPS)) {
            TF_WARN(
                "BlendShape prim %s has inbetween %s with invalid weight.\n",
                blendShapePrimPath.GetText(), name.GetText());
            continue;
        }

        weightsAndOffsets.push_back(
            { weight, inbetweenSchema.GetOffsets(), name });
    }

    {
        TRACE_SCOPE("Sorting weights");

        std::sort(weightsAndOffsets.begin(), weightsAndOffsets.end());
    }

    {
        TRACE_SCOPE("Filling");

        float prevWeight = -std::numeric_limits<float>::infinity();

        for (const WeightAndOffsets &weightAndOffsets : weightsAndOffsets) {
            const float weight = weightAndOffsets.weight;

            if (weight == 0.0f) {
                weightsAndSubShapeIndices->push_back({weight, -1});
            } else {
                if (GfIsClose(prevWeight, weight, EPS)) {
                    TF_WARN(
                        "BlendShape prim %s has two inbetweens with the "
                        "same weight %f.\n",
                        blendShapePrimPath.GetText(),
                        weight);
                    continue;
                }
                prevWeight = weight;

                int subShape(*numSubShapes);

                weightsAndSubShapeIndices->push_back({weight, subShape});

                _FillPointIndicesAndOffsets(
                    blendShapePrimPath,
                    weightAndOffsets.inbetweenName,
                    indices,
                    UsdSkelImagingGetTypedValue(
                        weightAndOffsets.offsetsDataSource),
                    subShape,
                    pointIndicesAndOffsets);

                (*numSubShapes)++;
            }
        }
    }
}

// Simply take the GfVec4f offset from each _PointIndexAndOffset
void
_ComputeBlendShapeOffsets(
    const std::vector<_PointIndexAndOffset> &pointIndicesAndOffsets,
    VtArray<GfVec4f> * const blendShapeOffsets)
{
    TRACE_FUNCTION();

    const size_t numOffsets = pointIndicesAndOffsets.size();

    blendShapeOffsets->resize(
        numOffsets,
        [numOffsets, &pointIndicesAndOffsets](
            GfVec4f * const begin, GfVec4f * const end) {
            for (size_t i = 0; i < numOffsets; ++i) {
                new(begin + i) GfVec4f(pointIndicesAndOffsets[i].second); }});
}

// Callback for VtArray::resize.
void
_FillBlendShapeOffsetRanges(
    const size_t numOffsets,
    const std::vector<_PointIndexAndOffset> &pointIndicesAndOffsets,
    GfVec2i * const begin, GfVec2i * const end)
{
    // Points to the GfVec2i corresponding to the pointIndex we
    // are currently processing.
    // We haven't seen a point yet and have not initialized any GfVec2i,
    // so point to just before the beginning.
    GfVec2i * current = begin - 1;

    for (size_t i = 0; i < numOffsets; i++) {
        while (current < begin + pointIndicesAndOffsets[i].first) {
            if (current >= begin) {
                (*current)[1] = i;
            }
            current++;
            new(current) GfVec2i(i, i);
        }
    }

    // pointIndicesAndOffsets non-empty, so this is safe to call.
    (*current)[1] = numOffsets;
}

// Compute for each point index the range in pointIndicesAndOffsets with
// that given index - assuming pointIndicesAndOffsets are sorted by point
// index.
//
void
_ComputeBlendShapeOffsetRanges(
    const std::vector<_PointIndexAndOffset> &pointIndicesAndOffsets,
    VtArray<GfVec2i> * const blendShapeOffsetRanges)
{
    TRACE_FUNCTION();

    const size_t numOffsets = pointIndicesAndOffsets.size();
    if (numOffsets == 0) {
        return;
    }

    const size_t numOffsetRanges(pointIndicesAndOffsets.back().first + 1);

    blendShapeOffsetRanges->resize(
        numOffsetRanges,
        [ numOffsets, &pointIndicesAndOffsets ](
            GfVec2i * const begin, GfVec2i * const end) {
                _FillBlendShapeOffsetRanges(
                    numOffsets,
                    pointIndicesAndOffsets,
                    begin, end); });
}

}

UsdSkelImagingBlendShapeData
UsdSkelImagingComputeBlendShapeData(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const SdfPath &primPath)
{
    TRACE_FUNCTION();

    UsdSkelImagingBlendShapeData data;
    data.primPath = primPath;

    const HdSceneIndexPrim prim = _GetPrim(sceneIndex, primPath);

    const UsdSkelImagingBindingSchema bindingSchema =
        UsdSkelImagingBindingSchema::GetFromParent(prim.dataSource);

    const VtArray<TfToken> blendShapeNames =
        UsdSkelImagingGetTypedValue(bindingSchema.GetBlendShapes());

    const VtArray<SdfPath> blendShapePrimPaths =
        UsdSkelImagingGetTypedValue(bindingSchema.GetBlendShapeTargets());

    data.numSubShapes = 0;

    if (blendShapeNames.size() != blendShapePrimPaths.size()) {
        TF_WARN(
            "Length (%zu) of blend shapes and length (%zu) of blend shape "
            "targets on prim %s are not matching.\n",
            blendShapeNames.size(),
            blendShapePrimPaths.size(),
            primPath.GetText());
    }

    // Re-implements UsdSkelBlendShapeQuery::ComputePackedShapeTable.

    std::vector<_PointIndexAndOffset> pointIndicesAndOffsets;
    {
        TRACE_SCOPE("Processing blend shapes");

        const size_t n =
            std::min(blendShapeNames.size(), blendShapePrimPaths.size());

        for (size_t i = 0; i < n; ++i) {
            const TfToken &blendShapeName = blendShapeNames[i];
            const SdfPath &blendShapePrimPath = blendShapePrimPaths[i];

            UsdSkelImagingWeightsAndSubShapeIndices &weightsAndSubShapeIndices =
                data.blendShapeNameToWeightsAndSubShapeIndices[blendShapeName];

            if (!weightsAndSubShapeIndices.empty()) {
                TF_WARN(
                    "Duplicate blend shape %s on prim %s.\n",
                    blendShapeName.GetText(), primPath.GetText());
                continue;
            }

            _ProcessBlendShapePrim(
                sceneIndex,
                primPath,
                blendShapePrimPath,
                &data.numSubShapes,
                &weightsAndSubShapeIndices,
                &pointIndicesAndOffsets);
        }
    }


    {
        TRACE_SCOPE("Sorting");

        // Note that UsdSkelBlendShapeQuery avoids the sorting by computing
        // a std::vector<unsigned> numOffsetsPerPoints first.
        //
        // We might need to do something similar if we see this in traces.

        std::sort(
            pointIndicesAndOffsets.begin(),
            pointIndicesAndOffsets.end(),
            [](const _PointIndexAndOffset &a, const _PointIndexAndOffset &b) {
                return
                    a.first < b.first ||
                    (a.first == b.first && a.second[3] < b.second[3]);});
    }

    _ComputeBlendShapeOffsets(
        pointIndicesAndOffsets,
        &data.blendShapeOffsets);
    _ComputeBlendShapeOffsetRanges(
        pointIndicesAndOffsets,
        &data.blendShapeOffsetRanges);

    return data;
}

// Re-implements UsdSkelBlendShapeQuery::ComputeSubShapeWeights.
VtArray<float>
UsdSkelImagingComputeBlendShapeWeights(
    const UsdSkelImagingBlendShapeData &data,
    const VtArray<TfToken> &blendShapeNames,
    const VtArray<float> &blendShapeWeights)
{
    VtArray<float> result(data.numSubShapes, 0.0f);

    if (blendShapeNames.size() != blendShapeWeights.size()) {
        TF_WARN(
            "Length (%zu) of blendShapes and length (%zu) of blendShapeWeights "
            "do not match on animation for prim %s.\n",
            blendShapeNames.size(),
            blendShapeWeights.size(),
            data.primPath.GetText());
    }

    const size_t n = std::min(blendShapeNames.size(), blendShapeWeights.size());

    for (size_t i = 0; i < n; i++) {
        const TfToken &blendShapeName = blendShapeNames[i];
        const float blendShapeWeight = blendShapeWeights[i];

        const UsdSkelImagingWeightsAndSubShapeIndices * const
            weightsAndIndices = TfMapLookupPtr(
                data.blendShapeNameToWeightsAndSubShapeIndices,
                blendShapeName);
        if (!weightsAndIndices) {
            if (0) {
                TF_WARN(
                    "The animation has a weight for blend shape %s but no such "
                    "blend shape exists for prim %s.\n",
                    blendShapeName.GetText(),
                    data.primPath.GetText());
            }
            continue;
        }

        if (weightsAndIndices->size() < 2) {
            TF_CODING_ERROR(
                "UsdSkelImagingBlendShapeData is supposed to have a weight "
                "for 0.0 and 1.0.\n");
            continue;
        }

        if (weightsAndIndices->size() == 2) {
            // No inbetweens. Simply use the one weight.
            const int subShapeIndex = (*weightsAndIndices)[1].subShapeIndex;
            result[subShapeIndex] = blendShapeWeight;
            continue;
        }

        // Find the pair of adjacent weights such that the given weight
        // is inbetween - in a best effort way: if the given weight is
        // smaller or larger than all weights, pick the pair of the two
        // smallest or largest weights.
        //
        const auto upper =
            std::upper_bound(
                weightsAndIndices->begin() + 1,
                weightsAndIndices->end() - 1,
                blendShapeWeight,
                [](const float a,
                   const UsdSkelImagingWeightAndSubShapeIndex &b) {
                    return a < b.weight; });
        const auto lower = upper - 1;

        const float weightDelta = upper->weight - lower->weight;

        if (!(weightDelta > EPS)) {
            // Note that we should have already enforced this in
            // _ProcessBlendShapePrim.
            TF_CODING_ERROR(
                "UsdSkelImagingBlendShapeData is supposed to have unique "
                "weights.\n");
            continue;
        }

        // Blending factor for interpolation (or extrapolation if given weight
        // is smaller or larger than all weights.
        const float alpha = (blendShapeWeight - lower->weight) / weightDelta;

        {
            const int subShapeIndex = lower->subShapeIndex;
            if (subShapeIndex >= 0 && !GfIsClose(alpha, 1.0, EPS)) {
                result[subShapeIndex] = 1.0 - alpha;
            }
        }

        {
            const int subShapeIndex = upper->subShapeIndex;
            if (subShapeIndex >= 0 && !GfIsClose(alpha, 0.0, EPS)) {
                result[subShapeIndex] = alpha;
            }
        }
    }

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
