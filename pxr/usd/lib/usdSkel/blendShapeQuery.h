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
#ifndef USDSKEL_BLENDSHAPEQUERY_H
#define USDSKEL_BLENDSHAPEQUERY_H

/// \file usdSkel/blendShapeQuery.h

#include "pxr/pxr.h"
#include "pxr/usd/usdSkel/api.h"
#include "pxr/usd/usdSkel/blendShape.h"
#include "pxr/usd/usdSkel/inbetweenShape.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/span.h"
#include "pxr/base/vt/array.h"


PXR_NAMESPACE_OPEN_SCOPE


class UsdSkelBindingAPI;


/// \class UsdSkelBlendShapeQuery
///
/// Helper class used to resolve blend shape weights, including
/// inbetweens.
class UsdSkelBlendShapeQuery
{
public:

    UsdSkelBlendShapeQuery() = default;

    USDSKEL_API UsdSkelBlendShapeQuery(const UsdSkelBindingAPI& binding);

    /// Return true if this query is valid.
    bool IsValid() const { return bool(_prim); }

    /// Boolean conversion operator. Equivalent to IsValid().
    explicit operator bool() const { return IsValid(); }

    /// Returns the prim the blend shapes apply to.
    const UsdPrim& GetPrim() const { return _prim; }

    /// Returns the blend shape corresponding to \p blendShapeIndex.
    USDSKEL_API UsdSkelBlendShape GetBlendShape(size_t blendShapeIndex) const;

    /// Returns the inbetween shape corresponding to sub-shape \p i, if any.
    USDSKEL_API UsdSkelInbetweenShape GetInbetween(size_t subShapeIndex) const;

    size_t GetNumBlendShapes() const { return _blendShapes.size(); }
    
    size_t GetNumSubShapes() const { return _subShapes.size(); }

    /// Compute an array holding the point indices of all shapes.
    /// This is indexed by the _blendShapeIndices_ returned by
    /// ComputeSubShapes().
    /// Since the _pointIndices_ property of blend shapes is optional,
    /// some of the arrays may be empty.
    USDSKEL_API std::vector<VtUIntArray>
    ComputeBlendShapePointIndices() const;

    /// Compute an array holding the point offsets of all sub-shapes.
    /// This includes offsets of both primary shapes -- those stored directly
    /// on a BlendShape primitive -- as well as those of inbetween shapes.
    /// This is indexed by the _subShapeIndices_ returned by
    /// ComputeSubShapeWeights().
    USDSKEL_API std::vector<VtVec3fArray>
    ComputeSubShapePointOffsets() const;

    /// Compute the resolved weights for all sub-shapes bound to this prim.
    /// The \p weights values are initial weight values, ordered according
    /// to the _skel:blendShapeTargets_ relationship of the prim this query
    /// is associated with. If there are any inbetween shapes, a new set
    /// of weights is computed, providing weighting of the relevant inbetweens.
    ///
    /// All computed arrays shared the same size. Elements of the same index
    /// identify which sub-shape of which blend shape a given weight value
    /// is mapped to.
    USDSKEL_API bool
    ComputeSubShapeWeights(const TfSpan<const float>& weights,
                           VtFloatArray* subShapeWeights,
                           VtUIntArray* blendShapeIndices,
                           VtUIntArray* subShapeIndices) const;

    /// Deform \p points using the resolved sub-shapes given by
    /// \p subShapeWeights, \p blendShapeIndices and \p subShapeIndices.
    /// The \p blendShapePointIndices and \p blendShapePointOffsets
    /// arrays both provide the pre-computed point offsets and indices
    /// of each sub-shape, as computed by ComputeBlendShapePointIndices()
    /// and ComputeSubShapePointOffsets().
    USDSKEL_API bool
    ComputeDeformedPoints(
        const TfSpan<const float> subShapeWeights,
        const TfSpan<const unsigned> blendShapeIndices,
        const TfSpan<const unsigned> subShapeIndices,
        const std::vector<VtUIntArray>& blendShapePointIndices,
        const std::vector<VtVec3fArray>& subShapePointOffsets,
        TfSpan<GfVec3f> points) const;
    
    USDSKEL_API
    std::string GetDescription() const;

private:

    /// Object identifying a general subshape.
    struct _SubShape {
        _SubShape() = default;

        _SubShape(unsigned blendShapeIndex, int inbetweenIndex, float weight)
            : _blendShapeIndex(blendShapeIndex),
              _inbetweenIndex(inbetweenIndex),
              _weight(weight) {}

        unsigned GetBlendShapeIndex() const { return _blendShapeIndex; }
        
        int GetInbetweenIndex() const { return _inbetweenIndex; }

        bool IsInbetween() const    { return _inbetweenIndex >= 0; }
        bool IsNullShape() const    { return _weight == 0.0f; }
        bool IsPrimaryShape() const { return _weight == 1.0f; }

        float GetWeight() const { return _weight; }

    private:
        unsigned _blendShapeIndex = 0;
        int _inbetweenIndex = 0;
        float _weight = 0;
    };

    struct _SubShapeCompareByWeight {
        bool operator()(const _SubShape& lhs, const _SubShape& rhs) const
             { return lhs.GetWeight() < rhs.GetWeight(); }

        bool operator()(float lhs, const _SubShape& rhs) const
             { return lhs < rhs.GetWeight(); }
    };

    struct _BlendShape {
        UsdSkelBlendShape shape;
        size_t firstSubShape = 0;
        size_t numSubShapes = 0;
    };

    UsdPrim _prim;
    std::vector<_SubShape> _subShapes;
    std::vector<_BlendShape> _blendShapes;
    std::vector<UsdSkelInbetweenShape> _inbetweens;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDSKEL_BLENDSHAPEQUERY_H
