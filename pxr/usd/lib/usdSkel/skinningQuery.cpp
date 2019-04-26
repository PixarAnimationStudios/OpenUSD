//
// Copyright 2016 Pixar
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
#include "pxr/usd/usdSkel/skinningQuery.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/relationship.h"

#include "pxr/usd/usdGeom/boundable.h"
#include "pxr/usd/usdGeom/primvar.h"

#include "pxr/usd/usdSkel/utils.h"


PXR_NAMESPACE_OPEN_SCOPE


namespace {


enum UsdSkel_SkinningQueryFlags {
    UsdSkel_HasJointInfluences = 1 << 0,
    UsdSkel_HasBlendShapes = 1 << 1
};


} // namespace


UsdSkelSkinningQuery::UsdSkelSkinningQuery()
{}


UsdSkelSkinningQuery::UsdSkelSkinningQuery(
    const UsdPrim& prim,
    const VtTokenArray& skelJointOrder,
    const UsdAttribute& jointIndices,
    const UsdAttribute& jointWeights,
    const UsdAttribute& geomBindTransform,
    const UsdAttribute& joints,
    const UsdAttribute& blendShapes,
    const UsdRelationship& blendShapeTargets)
    : _prim(prim),
      _interpolation(UsdGeomTokens->constant),
      _jointIndicesPrimvar(jointIndices),
      _jointWeightsPrimvar(jointWeights),
      _geomBindTransformAttr(geomBindTransform),
      _blendShapes(blendShapes),
      _blendShapeTargets(blendShapeTargets)
{
    VtTokenArray jointOrder;
    if (joints && joints.Get(&jointOrder)) {
        _jointOrder = jointOrder;
        _mapper =
            std::make_shared<UsdSkelAnimMapper>(skelJointOrder, jointOrder);
    }

    _InitializeJointInfluenceBindings(jointIndices, jointWeights);
    _InitializeBlendShapeBindings(blendShapes, blendShapeTargets);
}


void
UsdSkelSkinningQuery::_InitializeJointInfluenceBindings(
    const UsdAttribute& jointIndices,
    const UsdAttribute& jointWeights)
{
    if (!jointIndices || !jointWeights) {
        // Have incomplete joint influences.
        // Skipping remainder of validation.
        return;
    }

    // Validate joint influences.

    const int indicesElementSize = _jointIndicesPrimvar.GetElementSize();
    const int weightsElementSize = _jointWeightsPrimvar.GetElementSize();
    if (indicesElementSize != weightsElementSize) {
        TF_WARN("jointIndices element size (%d) != "
                "jointWeights element size (%d).",
                indicesElementSize, weightsElementSize);
        return;
    }

    if (indicesElementSize <= 0) {
        TF_WARN("Invalid element size [%d]: element size must "
                "be greater than zero.", indicesElementSize);
        return;
    }

    const TfToken indicesInterpolation = _jointIndicesPrimvar.GetInterpolation();
    const TfToken weightsInterpolation = _jointWeightsPrimvar.GetInterpolation();
    if (indicesInterpolation != weightsInterpolation) {
        TF_WARN("jointIndices interpolation (%s) != "
                "jointWeights interpolation (%s).",
                indicesInterpolation.GetText(),
                weightsInterpolation.GetText());
        return;
    }

    if (indicesInterpolation != UsdGeomTokens->constant &&
        indicesInterpolation != UsdGeomTokens->vertex) {
        TF_WARN("Invalid interpolation (%s) for joint influences: "
                "interpolation must be either 'constant' or 'vertex'.",
                indicesInterpolation.GetText());
        return;
    }

    // Valid joint influences, to the extent that we can validate here.
    // Any further validation of joint influences requires the actual
    // indices/weights to be read in, which we won't do here.

    _numInfluencesPerComponent = indicesElementSize;
    _interpolation = indicesInterpolation;

    _flags |= UsdSkel_HasJointInfluences;
}


void
UsdSkelSkinningQuery::_InitializeBlendShapeBindings(
    const UsdAttribute& blendShapes,
    const UsdRelationship& blendShapeTargets)
{
    if (blendShapes && blendShapeTargets) {
        _flags |= UsdSkel_HasBlendShapes;
    }
}


bool
UsdSkelSkinningQuery::HasBlendShapes() const
{
    return _flags & UsdSkel_HasBlendShapes;
}


bool
UsdSkelSkinningQuery::HasJointInfluences() const
{
    return _flags & UsdSkel_HasJointInfluences;
}


bool
UsdSkelSkinningQuery::IsRigidlyDeformed() const
{
    return _interpolation == UsdGeomTokens->constant;
}


bool
UsdSkelSkinningQuery::GetJointOrder(VtTokenArray* jointOrder) const
{
    if (jointOrder) {
        if (_jointOrder) {
            *jointOrder = *_jointOrder;
            return true;
        }
    } else {
        TF_CODING_ERROR("'jointOrder' pointer is null.");
    }
    return false;
}


bool
UsdSkelSkinningQuery::GetTimeSamples(std::vector<double>* times) const
{
    return GetTimeSamplesInInterval(GfInterval::GetFullInterval(), times);
}


bool
UsdSkelSkinningQuery::GetTimeSamplesInInterval(const GfInterval& interval,
                                               std::vector<double>* times) const
{
    if (!times) {
        TF_CODING_ERROR("'times' pointer is null.");
        return false;
    }

    // TODO: Use Usd_MergeTimeSamples if it becomes public.

    std::vector<double> tmpTimes;
    for (const auto& pv : {_jointIndicesPrimvar, _jointWeightsPrimvar}) {
        if (pv.GetTimeSamplesInInterval(interval, &tmpTimes)) {  
            times->insert(times->end(), tmpTimes.begin(), tmpTimes.end());
        }
    }
    if (_geomBindTransformAttr.GetTimeSamplesInInterval(interval, &tmpTimes)) {
        times->insert(times->end(), tmpTimes.begin(), tmpTimes.end());
    }

    std::sort(times->begin(), times->end());
    times->erase(std::unique(times->begin(), times->end()), times->end());
    return true;
}


bool
UsdSkelSkinningQuery::ComputeJointInfluences(VtIntArray* indices,
                                             VtFloatArray* weights,
                                             UsdTimeCode time) const
{
    TRACE_FUNCTION();

    if (!TF_VERIFY(IsValid(), "invalid skinning query") ||
        !TF_VERIFY(_jointIndicesPrimvar) ||
        !TF_VERIFY(_jointWeightsPrimvar)) {
        return false;
    }

    if (_jointIndicesPrimvar.ComputeFlattened(indices, time) &&
        _jointWeightsPrimvar.ComputeFlattened(weights, time)) {

        if (indices->size() != weights->size()) {
            TF_WARN("Size of jointIndices [%zu] != size of "
                    "jointWeights [%zu].", indices->size(), weights->size());
            return false;
        }

        if (!TF_VERIFY(_numInfluencesPerComponent > 0)) {
            return false;
        }

        if (indices->size()%_numInfluencesPerComponent != 0) {
            TF_WARN("unexpected size of jointIndices and jointWeights "
                    "arrays [%zu]: size must be a multiple of the number of "
                    "influences per component (%d).",
                    indices->size(), _numInfluencesPerComponent);
            return false;
        }

        if (IsRigidlyDeformed() &&
           indices->size() != static_cast<size_t>(_numInfluencesPerComponent)) {

            TF_WARN("Unexpected size of jointIndices and jointWeights "
                    "arrays [%zu]: joint influences are defined with 'constant'"
                    " interpolation, so the array size must be equal to the "
                    "element size (%d).", indices->size(),
                    _numInfluencesPerComponent);
            return false;
        }

        return true;
    }
    return false;
}


bool
UsdSkelSkinningQuery::ComputeVaryingJointInfluences(size_t numPoints,
                                                    VtIntArray* indices,
                                                    VtFloatArray* weights,
                                                    UsdTimeCode time) const
{
    TRACE_FUNCTION();

    if (ComputeJointInfluences(indices, weights, time)) {
        if (IsRigidlyDeformed()) {
            if (!UsdSkelExpandConstantInfluencesToVarying(indices, numPoints) ||
                !UsdSkelExpandConstantInfluencesToVarying(weights, numPoints)) {
                return false;
            }
            if (!TF_VERIFY(indices->size() == weights->size()))
                return false;
        } else if (indices->size() != numPoints*_numInfluencesPerComponent) {
            TF_WARN("Unexpected size of jointIndices and jointWeights "
                    "arrays [%zu]: varying influences should be sized to "
                    "numPoints [%zu] * numInfluencesPerComponent [%d].",
                    indices->size(), numPoints, _numInfluencesPerComponent);
            return false;
        }
        return true;
    }
    return false;
}


template <typename Matrix4>
bool
UsdSkelSkinningQuery::ComputeSkinnedPoints(const VtArray<Matrix4>& xforms,
                                           VtVec3fArray* points,
                                           UsdTimeCode time) const
{
    TRACE_FUNCTION();

    if (!points) {
        TF_CODING_ERROR("'points' pointer is null.");
        return false;
    }

    VtIntArray jointIndices;
    VtFloatArray jointWeights;
    if (ComputeVaryingJointInfluences(points->size(), &jointIndices,
                                      &jointWeights, time)) {

        // If the binding site has a custom joint ordering, the query will have
        // a mapper that should be used to reorder transforms
        // (skel order -> binding order)
        VtArray<Matrix4> orderedXforms(xforms);
        if (_mapper) {
            if (!_mapper->RemapTransforms(xforms, &orderedXforms)) {
                return false;
            }
        }

        const Matrix4 geomBindXform(GetGeomBindTransform(time));
        return UsdSkelSkinPointsLBS(geomBindXform, orderedXforms,
                                    jointIndices, jointWeights,
                                    _numInfluencesPerComponent,
                                    *points);
    }
    return false;
}


template USDSKEL_API bool
UsdSkelSkinningQuery::ComputeSkinnedPoints(const VtArray<GfMatrix4d>&,
                                           VtVec3fArray*, UsdTimeCode) const;

template USDSKEL_API bool
UsdSkelSkinningQuery::ComputeSkinnedPoints(const VtArray<GfMatrix4f>&,
                                           VtVec3fArray*, UsdTimeCode) const;


template <typename Matrix4>
bool
UsdSkelSkinningQuery::ComputeSkinnedTransform(const VtArray<Matrix4>& xforms,
                                              Matrix4* xform,
                                              UsdTimeCode time) const
{
    TRACE_FUNCTION();

    if (!xform) {
        TF_CODING_ERROR("'xform' pointer is null.");
        return false;
    }

    if (!IsRigidlyDeformed()) {
        TF_CODING_ERROR("Attempted to skin a transform, but "
                        "joint influences are not constant.");
        return false;
    }

    VtIntArray jointIndices;
    VtFloatArray jointWeights;
    if (ComputeJointInfluences(&jointIndices, &jointWeights, time)) {

        // If the binding site has a custom joint ordering, the query will have
        // a mapper that should be used to reorder transforms
        // (skel order -> binding order)
        VtArray<Matrix4> orderedXforms(xforms);
        if (_mapper) {
            if (!_mapper->Remap(xforms, &orderedXforms)) {
                return false;
            }
        }

        const Matrix4 geomBindXform(GetGeomBindTransform(time));
        return UsdSkelSkinTransformLBS(geomBindXform, orderedXforms,
                                       jointIndices, jointWeights, xform);
    }
    return false;
}


template USDSKEL_API bool
UsdSkelSkinningQuery::ComputeSkinnedTransform(
    const VtArray<GfMatrix4d>&, GfMatrix4d*, UsdTimeCode time) const;

template USDSKEL_API bool
UsdSkelSkinningQuery::ComputeSkinnedTransform(
    const VtArray<GfMatrix4f>&, GfMatrix4f*, UsdTimeCode time) const;


template <typename Matrix4>
float
UsdSkelSkinningQuery::ComputeExtentsPadding(
    const VtArray<Matrix4>& skelRestXforms,
    const UsdGeomBoundable& boundable) const
{
    // Don't use default time; properties may be keyed (and still unvarying)
    // We do, however, expect the computed quantity to not be time varying.
    const UsdTimeCode time = UsdTimeCode::EarliestTime();

    VtVec3fArray boundableExtent;
    if (boundable && 
       boundable.GetExtentAttr().Get(&boundableExtent, time) &&
       boundableExtent.size() == 2) {
        
        GfRange3f jointsRange;
        if (UsdSkelComputeJointsExtent<Matrix4>(skelRestXforms, &jointsRange)) {

            // Get the aligned range of the gprim in its bind pose.
            const GfRange3d gprimRange = 
                GfBBox3d(GfRange3d(boundableExtent[0], boundableExtent[1]),
                         GetGeomBindTransform(time))
                    .ComputeAlignedRange();

            const GfVec3f minDiff =
                jointsRange.GetMin() - GfVec3f(gprimRange.GetMin());
            const GfVec3f maxDiff =
                GfVec3f(gprimRange.GetMax()) - jointsRange.GetMax();

            float padding = 0.0f;
            for (int i = 0; i < 3; ++i) {
                padding = std::max(padding, minDiff[i]);
                padding = std::max(padding, maxDiff[i]);
            }
            return padding;
        }
    }
    return 0.0f;
}


template USDSKEL_API float
UsdSkelSkinningQuery::ComputeExtentsPadding(
    const VtArray<GfMatrix4d>&, const UsdGeomBoundable&) const;

template USDSKEL_API float
UsdSkelSkinningQuery::ComputeExtentsPadding(
    const VtArray<GfMatrix4f>&, const UsdGeomBoundable&) const;


GfMatrix4d
UsdSkelSkinningQuery::GetGeomBindTransform(UsdTimeCode time) const
{
    // Geom bind transform attr is optional.
    GfMatrix4d xform;
    if (!_geomBindTransformAttr ||
        !_geomBindTransformAttr.Get(&xform, time)) {
        xform.SetIdentity();
    }
    return xform;
}


std::string
UsdSkelSkinningQuery::GetDescription() const
{   
    if (IsValid()) {
        return TfStringPrintf("UsdSkelSkinningQuery <%s>",
                              _prim.GetPath().GetText());
    }
    return "invalid UsdSkelSkinningQuery";
}


PXR_NAMESPACE_CLOSE_SCOPE
