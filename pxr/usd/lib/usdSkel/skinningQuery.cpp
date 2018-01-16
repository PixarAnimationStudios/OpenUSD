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

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/relationship.h"

#include "pxr/usd/usdGeom/primvar.h"

#include "pxr/usd/usdSkel/utils.h"


PXR_NAMESPACE_OPEN_SCOPE


UsdSkelSkinningQuery::UsdSkelSkinningQuery()
    :  _valid(false), _numInfluencesPerComponent(1),
       _interpolation(UsdGeomTokens->constant)
{}


UsdSkelSkinningQuery::UsdSkelSkinningQuery(
    const UsdPrim& prim,
    const VtTokenArray& skelJointOrder,
    const UsdAttribute& jointIndices,
    const UsdAttribute& jointWeights,
    const UsdAttribute& geomBindTransform,
    const VtTokenArray* jointOrder)
    : _valid(false), _numInfluencesPerComponent(1),
      _interpolation(UsdGeomTokens->constant),
      _jointIndicesPrimvar(jointIndices),
      _jointWeightsPrimvar(jointWeights),
      _geomBindTransformAttr(geomBindTransform)
{
    if(jointOrder) {
        _jointOrder = *jointOrder;

        _mapper = std::make_shared<UsdSkelAnimMapper>(
            skelJointOrder, *jointOrder);
    }

    if(!jointIndices) {
        TF_WARN("'jointIndices' is invalid.");
        return;
    }

    if(!jointWeights) {
        TF_WARN("jointWeights' is invalid.");
        return;
    }

    // Validate joint influences.

    int indicesElementSize = _jointIndicesPrimvar.GetElementSize();
    int weightsElementSize = _jointWeightsPrimvar.GetElementSize();
    if(indicesElementSize != weightsElementSize) {
        TF_WARN("JointIndices element size (%d) != "
                "jointWeights element size (%d).",
                indicesElementSize, weightsElementSize);
        return;
    }

    if(indicesElementSize <= 0) {
        TF_WARN("Invalid element size [%d]: element size must "
                "be greater than zero.", indicesElementSize);
        return;
    }

    TfToken indicesInterpolation = _jointIndicesPrimvar.GetInterpolation();
    TfToken weightsInterpolation = _jointWeightsPrimvar.GetInterpolation();
    if(indicesInterpolation != weightsInterpolation) {
        TF_WARN("JointIndices interpolation (%s) != "
                "jointWeights interpolation (%s).",
                indicesInterpolation.GetText(),
                weightsInterpolation.GetText());
        return;
    }

    if(indicesInterpolation != UsdGeomTokens->constant &&
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
    _valid = true;
}


bool
UsdSkelSkinningQuery::IsRigidlyDeformed() const
{   
    return _interpolation == UsdGeomTokens->constant;
}


bool
UsdSkelSkinningQuery::GetJointOrder(VtTokenArray* jointOrder) const
{
    if(jointOrder) {
        if(_jointOrder) {
            *jointOrder = *_jointOrder;
        }
        return true;
    } else {
        TF_CODING_ERROR("'jointOrder' pointer is null.");
        return false;
    }
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
    if(!times) {
        TF_CODING_ERROR("'times' pointer is null.");
        return false;
    }

    // TODO: Use Usd_MergeTimeSamples if it becomes public.

    std::vector<double> tmpTimes;
    for(const auto& pv : {_jointIndicesPrimvar, _jointWeightsPrimvar}) {
        if(pv.GetTimeSamplesInInterval(interval, &tmpTimes)) {  
            times->insert(times->end(), tmpTimes.begin(), tmpTimes.end());
        }
    }
    if(_geomBindTransformAttr.GetTimeSamplesInInterval(interval, &tmpTimes)) {
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

    if(!TF_VERIFY(IsValid(), "invalid skinning query") ||
       !TF_VERIFY(_jointIndicesPrimvar) || !TF_VERIFY(_jointWeightsPrimvar)) {
        return false;
    }

    if(_jointIndicesPrimvar.ComputeFlattened(indices, time) &&
       _jointWeightsPrimvar.ComputeFlattened(weights, time)) {

        if(indices->size() != weights->size()) {
            TF_WARN("Size of jointIndices [%zu] != size of "
                    "jointWeights [%zu].", indices->size(), weights->size());
            return false;
        }

        if(!TF_VERIFY(_numInfluencesPerComponent > 0)) {
            return false;
        }

        if(indices->size()%_numInfluencesPerComponent != 0) {
            TF_WARN("unexpected size of jointIndices and jointWeights "
                    "arrays [%zu]: size must be a multiple of the number of "
                    "influences per component (%d).",
                    indices->size(), _numInfluencesPerComponent);
            return false;
        }

        if(IsRigidlyDeformed() &&
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

    if(ComputeJointInfluences(indices, weights, time)) {
        if(IsRigidlyDeformed()) {
            if(!UsdSkelExpandConstantInfluencesToVarying(indices, numPoints) ||
               !UsdSkelExpandConstantInfluencesToVarying(weights, numPoints)) {
                return false;
            }
            if(!TF_VERIFY(indices->size() == weights->size()))
                return false;
        } else if(indices->size() != numPoints*_numInfluencesPerComponent) {
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


bool
UsdSkelSkinningQuery::ComputeSkinnedPoints(const VtMatrix4dArray& xforms,
                                           VtVec3fArray* points,
                                           UsdTimeCode time) const
{
    TRACE_FUNCTION();

    if(!points) {
        TF_CODING_ERROR("'points' pointer is null.");
        return false;
    }

    VtIntArray jointIndices;
    VtFloatArray jointWeights;
    if(ComputeVaryingJointInfluences(points->size(), &jointIndices,
                                     &jointWeights, time)) {

        // If the binding site has a custom joint ordering, the query will have
        // a mapper that should be used to reorder transforms
        // (skel order -> binding order)
        VtMatrix4dArray orderedXforms(xforms);
        if(_mapper) {
            if(!_mapper->Remap(xforms, &orderedXforms)) {
                std::cerr << "can't remap?\n";
                return false;
            }
        }

        GfMatrix4d geomBindXform = GetGeomBindTransform(time);
        return UsdSkelSkinPointsLBS(geomBindXform, orderedXforms,
                                    jointIndices, jointWeights,
                                    _numInfluencesPerComponent, points);
    }
    return false;
}


bool
UsdSkelSkinningQuery::ComputeSkinnedTransform(const VtMatrix4dArray& xforms,
                                              GfMatrix4d* xform,
                                              UsdTimeCode time) const
{
    TRACE_FUNCTION();

    if(!xform) {
        TF_CODING_ERROR("'xform' pointer is null.");
        return false;
    }

    if(!IsRigidlyDeformed()) {
        TF_CODING_ERROR("Attempted to skin a transform, but "
                        "joint influences are not constant.");
        return false;
    }

    VtIntArray jointIndices;
    VtFloatArray jointWeights;
    if(ComputeJointInfluences(&jointIndices, &jointWeights, time)) {

        // If the binding site has a custom joint ordering, the query will have
        // a mapper that should be used to reorder transforms
        // (skel order -> binding order)
        VtMatrix4dArray orderedXforms(xforms);
        if(_mapper) {
            if(!_mapper->Remap(xforms, &orderedXforms)) {
                return false;
            }
        }

        GfMatrix4d geomBindXform = GetGeomBindTransform(time);
        return UsdSkelSkinTransformLBS(geomBindXform, orderedXforms,
                                       jointIndices, jointWeights, xform);
    }
    return false;
}


GfMatrix4d
UsdSkelSkinningQuery::GetGeomBindTransform(UsdTimeCode time) const
{
    // Geom bind transform attr is optional.
    GfMatrix4d xform;
    if(!_geomBindTransformAttr ||
       !_geomBindTransformAttr.Get(&xform, time)) {
        xform.SetIdentity();
    }
    return xform;
}


std::string
UsdSkelSkinningQuery::GetDescription() const
{   
    if(IsValid()) {
        return TfStringPrintf("UsdSkelSkinningQuery %p (%s)",
                              this, _interpolation.GetText());
    }
    return "invalid UsdSkelSkinningQuery";
}


PXR_NAMESPACE_CLOSE_SCOPE
