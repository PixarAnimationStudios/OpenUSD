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
    for (size_t i = 0; i < _blendShapes.size(); ++i) {
        const auto& blendShape = _blendShapes[i];
        // XXX: Some null blend shapes may be stored on _blendShapes
        // to preserve the 'blendShapeTargets' order.
        if (blendShape.shape) {
            blendShape.shape.GetPointIndicesAttr().Get(&indices[i]);
        }
    }
    return indices;
}


std::vector<VtVec3fArray>
UsdSkelBlendShapeQuery::ComputeSubShapePointOffsets() const
{
    std::vector<VtVec3fArray> offsets(_subShapes.size());
    for (size_t i = 0; i < _subShapes.size(); ++i) {
        const _SubShape& shape = _subShapes[i];

        if (shape.IsInbetween()) {
            if (TF_VERIFY(static_cast<size_t>(shape.GetInbetweenIndex())
                          < _inbetweens.size())) {
                const auto& inbetween = _inbetweens[shape.GetInbetweenIndex()];
                inbetween.GetOffsets(&offsets[i]);
            }
        } else if (!shape.IsNullShape()) {
            if (TF_VERIFY(shape.GetBlendShapeIndex() < _blendShapes.size())) {
                const auto& blendShape =
                    _blendShapes[shape.GetBlendShapeIndex()];
                if (blendShape.shape) {
                    blendShape.shape.GetOffsetsAttr().Get(&offsets[i]);
                }
            }
        }
    }
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
