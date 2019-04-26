//
// Copyright 2019 Pixar
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
#include "pxr/usd/usdSkel/blendShapeQuery.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/reduce.h"

#include "pxr/usd/usdSkel/bindingAPI.h"
#include "pxr/usd/usdSkel/utils.h"

#include <algorithm>


PXR_NAMESPACE_OPEN_SCOPE


namespace {

const float EPS = 1e-6;

} // namespace


UsdSkelBlendShapeQuery::UsdSkelBlendShapeQuery(
    const UsdSkelBindingAPI& binding)
{
    UsdRelationship blendShapeTargetsRel = binding.GetBlendShapeTargetsRel();
    if (!blendShapeTargetsRel) {
        return;
    }

    SdfPathVector targets;
    blendShapeTargetsRel.GetTargets(&targets);

    _blendShapes.resize(targets.size());

    // Fill _shapes with the cumulative set of primary, null and inbetween shapes.
    for (size_t i = 0; i < targets.size(); ++i) {

        const SdfPath& target = targets[i];

        UsdSkelBlendShape targetShape =
            UsdSkelBlendShape::Get(binding.GetPrim().GetStage(), target);
        if (!targetShape) {
            TF_WARN("Target <%s> is not a valid BlendShape", target.GetText());
            continue;
        }

        _BlendShape& blendShape = _blendShapes[i];;
        blendShape.shape = targetShape;
        blendShape.firstSubShape = _subShapes.size();

        if (!targetShape.GetPrim().IsActive()) {
            // Target is inactive. Still need an entry for the
            // prim, but it will have no shapes.
            continue;
        }

        // Add subshapes for the primary and null shapes.
        // ComputeSubShapes() depends on this ordering being consistent
        // (i.e., primary shape comes last).
        _subShapes.emplace_back(i, -1, 1.0f);
        _subShapes.emplace_back(i, -1, 0.0f);
        
        // Add all inbetweens.
        for (const auto& inbetween : targetShape.GetInbetweens()) {
            // Skip inactive inbetweens.
            float weight = 0;
            if (!inbetween.GetWeight(&weight)) {
                continue;
            }
            
            if (GfIsClose(weight, 0.0, EPS) || GfIsClose(weight, 1.0, EPS)) {
                TF_WARN("%s -- skipping inbetween with invalid weight (%f)",
                        inbetween.GetAttr().GetPath().GetText(), weight);
                continue;
            }

            const int inbetweenIndex = static_cast<int>(_inbetweens.size());

            _subShapes.emplace_back(i, inbetweenIndex, weight);
            _inbetweens.push_back(inbetween);
        }
        blendShape.numSubShapes = _subShapes.size() - blendShape.firstSubShape;

        // Sort all subshapes of this shape according to weight.
        _SubShape* start = _subShapes.data() + blendShape.firstSubShape;
        _SubShape* end = start + blendShape.numSubShapes;
        std::sort(start, end, _SubShapeCompareByWeight());
    }
    _prim = binding.GetPrim();
}


UsdSkelBlendShape
UsdSkelBlendShapeQuery::GetBlendShape(size_t blendShapeIndex) const
{
    return blendShapeIndex < _blendShapes.size() ?
        _blendShapes[blendShapeIndex].shape : UsdSkelBlendShape();
}


UsdSkelInbetweenShape
UsdSkelBlendShapeQuery::GetInbetween(size_t subShapeIndex) const
{
    if (subShapeIndex < _subShapes.size()) {
        const auto& shape = _subShapes[subShapeIndex];
        if (shape.IsInbetween()) {
            if (TF_VERIFY(static_cast<size_t>(shape.GetInbetweenIndex())
                          < _inbetweens.size())) {
                return _inbetweens[shape.GetInbetweenIndex()];
            }
        }
    }
    return UsdSkelInbetweenShape();
}


std::vector<VtUIntArray>
UsdSkelBlendShapeQuery::ComputeBlendShapePointIndices() const
{
    std::vector<VtUIntArray> indices(_blendShapes.size());

    WorkParallelForN(
        _blendShapes.size(),
        [&](size_t start, size_t end)
        {
            for (size_t i = start; i < end; ++i) {
                const auto& blendShape = _blendShapes[i];
                // XXX: Some null blend shapes may be stored on _blendShapes
                // to preserve the 'blendShapeTargets' order.
                if (blendShape.shape) {
                    blendShape.shape.GetPointIndicesAttr().Get(&indices[i]);
                }
            }
        });
    return indices;
}


std::vector<VtVec3fArray>
UsdSkelBlendShapeQuery::ComputeSubShapePointOffsets() const
{
    std::vector<VtVec3fArray> offsets(_subShapes.size());
    
    WorkParallelForN(
        _subShapes.size(),
        [&](size_t start, size_t end)
        {
            for (size_t i = start; i < end; ++i) {
                const _SubShape& shape = _subShapes[i];

                if (shape.IsInbetween()) {
                    if (TF_VERIFY(static_cast<size_t>(shape.GetInbetweenIndex())
                                  < _inbetweens.size())) {
                        const auto& inbetween =
                            _inbetweens[shape.GetInbetweenIndex()];
                        inbetween.GetOffsets(&offsets[i]);
                    }
                } else if (!shape.IsNullShape()) {
                    if (TF_VERIFY(shape.GetBlendShapeIndex() <
                                  _blendShapes.size())) {
                        const auto& blendShape =
                            _blendShapes[shape.GetBlendShapeIndex()];
                        if (blendShape.shape) {
                            blendShape.shape.GetOffsetsAttr().Get(&offsets[i]);
                        }
                    }
                }
            }
        });
    return offsets;
}


bool
UsdSkelBlendShapeQuery::ComputeSubShapeWeights(
    const TfSpan<const float>& weights,
    VtFloatArray* subShapeWeights,
    VtUIntArray* blendShapeIndices,
    VtUIntArray* subShapeIndices) const
{
    TRACE_FUNCTION();

    if (!subShapeWeights) {
        TF_CODING_ERROR("'subShapeWeights' is null");
        return false;
    }
    if (!blendShapeIndices) {
        TF_CODING_ERROR("'blendShapeIndices' is null");
        return false;
    }
    if (!subShapeIndices) {
        TF_CODING_ERROR("'subShapeIndices' is null");
        return false;
    }
    
    if (static_cast<size_t>(weights.size()) != _blendShapes.size()) {
        TF_WARN("Size of weights [%td] != number of blend shapes [%zu]",
                weights.size(), _blendShapes.size());
        return false;
    }

    subShapeWeights->reserve(weights.size()*2);
    blendShapeIndices->reserve(weights.size()*2);
    subShapeIndices->reserve(weights.size()*2);

    for (ptrdiff_t i = 0; i < weights.size(); ++i) {

        const _BlendShape& blendShape = _blendShapes[i];

        // Take the fast route if there are few subshapes.
        if (blendShape.numSubShapes < 3) {

            TF_DEV_AXIOM(blendShape.numSubShapes == 2);

            // The second subshape should be the primary shape.
            const auto& subShape = _subShapes[blendShape.firstSubShape + 1];
            TF_DEV_AXIOM(subShape.GetWeight() == 1.0f);

            subShapeWeights->push_back(weights[i]);
            blendShapeIndices->push_back(i);
            subShapeIndices->push_back(blendShape.firstSubShape + 1);
            continue;
        }


        const float w = weights[i];

        const _SubShape* start = &_subShapes[blendShape.firstSubShape];
        const _SubShape* end = start + blendShape.numSubShapes;

        // Find the two nearest bounding subShapes.
        const _SubShape* it =
            std::upper_bound(start, end, w, _SubShapeCompareByWeight());

        const _SubShape* lower = nullptr;
        const _SubShape* upper = nullptr;

        if (it != end) {
            if (it > start) {
                lower = it - 1;
                upper = it;
            } else {
                lower = start;
                upper = start + 1;
            }
        } else {
            lower = end - 2;
            upper = end - 1;
        }

        const float weightDelta = upper->GetWeight() - lower->GetWeight();

        TF_DEV_AXIOM(weightDelta >= 0);

        if (weightDelta > EPS) {
            // Compute normalized pos between shapes.
            const float alpha = (w - lower->GetWeight())/weightDelta;
            
            if (!lower->IsNullShape() && !GfIsClose(alpha, 1.0, EPS)) {
                const size_t subShapeIndex =
                    std::distance(_subShapes.data(), lower);

                subShapeWeights->push_back(1.0 - alpha);
                blendShapeIndices->push_back(i);
                subShapeIndices->push_back(subShapeIndex);
            }
            if (!upper->IsNullShape() && !GfIsClose(alpha, 0.0, EPS)) {
                const size_t subShapeIndex = 
                    std::distance(_subShapes.data(), upper);

                subShapeWeights->push_back(alpha);
                blendShapeIndices->push_back(i);
                subShapeIndices->push_back(subShapeIndex);
            }
        }
    }
    return true;
}


bool
UsdSkelBlendShapeQuery::ComputeFlattenedSubShapeWeights(
    const TfSpan<const float>& weights,
    VtFloatArray* subShapeWeights) const
{
    if (!subShapeWeights) {
        TF_CODING_ERROR("'subShapeWeights' is null");
        return false;
    }

    VtFloatArray sparseSubShapeWeights;
    VtUIntArray sparseBlendShapeIndices;
    VtUIntArray sparseSubShapeIndices;

    if (ComputeSubShapeWeights(weights, &sparseSubShapeWeights,
                               &sparseBlendShapeIndices,
                               &sparseSubShapeIndices)) {
        subShapeWeights->assign(_subShapes.size(), 0.0f);
        auto dst = TfMakeSpan(*subShapeWeights);
        for (size_t i = 0; i < sparseSubShapeWeights.size(); ++i) {
            dst[sparseSubShapeIndices[i]] = sparseSubShapeWeights[i];
        }
        return true;
    }
    return false;
}


bool
UsdSkelBlendShapeQuery::ComputeDeformedPoints(
    const TfSpan<const float> subShapeWeights,
    const TfSpan<const unsigned> blendShapeIndices,
    const TfSpan<const unsigned> subShapeIndices,
    const std::vector<VtUIntArray>& blendShapePointIndices,
    const std::vector<VtVec3fArray>& subShapePointOffsets,
    TfSpan<GfVec3f> points) const
{
    if (blendShapeIndices.size() != subShapeWeights.size()) {
        TF_WARN("blendShapeIndices size [%td] != subShapeWeights size [%td]",
                blendShapeIndices.size(), subShapeWeights.size());
        return false;
    }
    if (subShapeIndices.size() != subShapeWeights.size()) {
        TF_WARN("subShapeIndices size [%td] != subShapeWeights size [%td]",
                subShapeIndices.size(), subShapeWeights.size());
        return false;
    }

    for (ptrdiff_t i = 0; i < subShapeWeights.size(); ++i) {
        const unsigned blendShapeIndex = blendShapeIndices[i];
        if (blendShapeIndex < blendShapePointIndices.size()) {
            const unsigned subShapeIndex = subShapeIndices[i];

            if (subShapeIndex < subShapePointOffsets.size()) {
                if (!UsdSkelApplyBlendShape(
                        subShapeWeights[i],
                        subShapePointOffsets[subShapeIndex],
                        blendShapePointIndices[blendShapeIndex],
                        points)) {
                    return false;
                }
            } else {
                TF_WARN("%td'th subShapeIndices entry [%d] >= "
                        "subShapePointOffsets size [%zu].",
                        i, subShapeIndex, subShapePointOffsets.size());
                return false;
            }
        } else {
            TF_WARN("%td'th blendShapeIndices entry [%d] >= "
                    "blendShapePointIndices size [%zu]",
                    i, blendShapeIndex, blendShapePointIndices.size());
            return false;
        }
    }
    return true;
}


namespace {


/// Compute a span of (start,end) ranges for a set of contiguous
/// elements. The \p counts gives the number of values per element.
/// Returns the total number of elements.
unsigned
_ComputeRangesFromCounts(const TfSpan<const unsigned>& counts,
                         TfSpan<GfVec2i> ranges)
{
    TF_AXIOM(counts.size() == ranges.size());

    unsigned start = 0;
    for (ptrdiff_t i = 0; i < counts.size(); ++i) { 
        const unsigned count = counts[i];
        ranges[i] = GfVec2i(start, start+count);
        start += count;
    }
    return start;
}


/// Compute an upper bound on the number of points needed for a set of shapes.
/// Note that his may not be the actual point count; it is only a point count
/// sufficient to satisfy the given shapes.
size_t
_ComputeApproximateNumPointsForShapes(
    const std::vector<VtUIntArray>& indicesPerBlendShape,
    const std::vector<VtVec3fArray>& offsetsPerSubShape)
{
    // Get the max index across all of the shapes.
    unsigned maxIndex =
        WorkParallelReduceN(
            0, indicesPerBlendShape.size(),
            [&indicesPerBlendShape](size_t start, size_t end, unsigned init) {
                for (auto i = start; i < end; ++i) {
                    for (auto index : indicesPerBlendShape[i]) {
                        init = std::max(init, index);
                    }
                }
                return init;
            },
            [](unsigned lhs, unsigned rhs) {
                return std::max(lhs, rhs);
            });

    // Also take the sizes of sub-shapes into account, for non-indexed shapes.
    for (const auto& offsets : offsetsPerSubShape) {
        maxIndex = std::max(maxIndex, static_cast<unsigned>(offsets.size()));
    }
    return maxIndex + 1;
}


} // namespace


bool
UsdSkelBlendShapeQuery::ComputePackedShapeTable(
    VtVec4fArray* offsets,
    VtVec2iArray* ranges) const
{
    if (!offsets) {
        TF_CODING_ERROR("'offsets' is null");
        return false;
    }
    if (!ranges) {
        TF_CODING_ERROR("'ranges' is null");
        return false;
    }

    const std::vector<VtUIntArray> indicesPerBlendShape =    
        ComputeBlendShapePointIndices();

    const std::vector<VtVec3fArray> offsetsPerSubShape =
        ComputeSubShapePointOffsets();

    const size_t numPoints =
        _ComputeApproximateNumPointsForShapes(indicesPerBlendShape,
                                              offsetsPerSubShape);

    if (numPoints == 0) {
        ranges->clear();
        offsets->clear();
        return true;
    }

    // Count the number of non-null subshapes associated with each blendshape.
    std::vector<unsigned> numSubShapesPerBlendShape(_blendShapes.size(), 0);
    for (size_t i = 0; i < _subShapes.size(); ++i) {
        const auto& subShape = _subShapes[i];
        if (!subShape.IsNullShape()) {
            TF_AXIOM(subShape.GetBlendShapeIndex() < _subShapes.size());
            ++numSubShapesPerBlendShape[subShape.GetBlendShapeIndex()];
        }
    }

    // Compute the number of offsets that map to each point.
    std::vector<unsigned> numOffsetsPerPoint(numPoints, 0);
    for (size_t i = 0; i < _blendShapes.size(); ++i) {
        const unsigned numSubShapes = numSubShapesPerBlendShape[i];
        TF_AXIOM(i < indicesPerBlendShape.size());
        const auto& indices = indicesPerBlendShape[i];
        if (indices.empty()) {
            // Blend shape is non-sparse. Increment count for all points.
            for (unsigned& numOffsets : numOffsetsPerPoint) {
                numOffsets += numSubShapes;
            }
        } else {
            // Blend shape is sparse. Only increment indexed points.
            for (const unsigned index : indices) {
                TF_AXIOM(index < numOffsetsPerPoint.size());
                numOffsetsPerPoint[index] += numSubShapes;
            }
        }
    }

    // Use the per-point offset count to compute the ranges.
    ranges->resize(numPoints);
    const unsigned numOffsets =
        _ComputeRangesFromCounts(numOffsetsPerPoint, *ranges);

    // Create a vector storing the start index of the offsets for every point.
    /// This will be incremented per-point while filling offset.
    std::vector<unsigned> nextOffsetIndexPerPoint(numPoints);   
    for (size_t i = 0; i < numPoints; ++i) {
        nextOffsetIndexPerPoint[i] = (*ranges)[i][0];
    }

    // Fill in the packed offset table
    offsets->assign(numOffsets, GfVec4f(0,0,0,0));

    auto dst = TfMakeSpan(*offsets);

    for (size_t i = 0; i < _subShapes.size(); ++i) {
        const auto& subShape = _subShapes[i];
        if (subShape.IsNullShape()) {
            continue;
        }

        TF_AXIOM(i < offsetsPerSubShape.size());
        TF_AXIOM(subShape.GetBlendShapeIndex() < indicesPerBlendShape.size());
        
        const auto& offsets = offsetsPerSubShape[i];
        const auto& indices =
            indicesPerBlendShape[subShape.GetBlendShapeIndex()];

        const float subShapeIndexAsFloat = static_cast<float>(i);

        if (indices.empty()) {
            // Blend shape is non-sparse. Fill for all offsets.
            for (size_t pi = 0; pi < offsets.size(); ++pi) {
                const GfVec3f& offset = offsets[pi];

                TF_AXIOM(pi < nextOffsetIndexPerPoint.size());
                
                const unsigned offsetIndex = nextOffsetIndexPerPoint[pi];
                dst[offsetIndex] = GfVec4f(offset[0], offset[1], offset[2],
                                           subShapeIndexAsFloat);

                ++nextOffsetIndexPerPoint[pi];
            }
        } else {
            // Must take indices into account.
            for (size_t j = 0; j < indices.size(); ++j) {
                const unsigned pointIndex = indices[j];

                const GfVec3f& offset = offsets[j];
                
                const unsigned offsetIndex =
                    nextOffsetIndexPerPoint[pointIndex];
                dst[offsetIndex] = GfVec4f(offset[0], offset[1], offset[2],
                                           subShapeIndexAsFloat);
                
                ++nextOffsetIndexPerPoint[pointIndex];
            }
        }
    }
    return true;
}


std::string
UsdSkelBlendShapeQuery::GetDescription() const
{
    if(IsValid()) {
        return TfStringPrintf("UsdSkelBlendShapeQuery <%s>",
                              _prim.GetPath().GetText());
    }
    return "invalid UsdSkelBlendShapeQuery";
}


PXR_NAMESPACE_CLOSE_SCOPE
