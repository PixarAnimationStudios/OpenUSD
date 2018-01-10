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
#ifndef USDSKEL_SKINNINGQUERY_H
#define USDSKEL_SKINNINGQUERY_H

/// \file usdSkel/skinningQuery.h

#include "pxr/pxr.h"
#include "pxr/usd/usdSkel/api.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/relationship.h"

#include "pxr/usd/usdGeom/primvar.h"

#include "pxr/usd/usdSkel/animMapper.h"


PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdSkelSkinningQuery
///
/// Object used for querying resolved bindings for skinning.
class UsdSkelSkinningQuery
{
public:
    USDSKEL_API
    UsdSkelSkinningQuery();

    /// Construct a new skining query for the resolved properties
    /// set through the UsdSkelBindingAPI, as inherited on \p prim.
    /// The resulting query will be marked valid only if the inherited
    /// properties provide proper valid joint influences.
    /// The \p prim is passed only for the sake of validation messages,
    /// as is not otherwise important.
    USDSKEL_API
    UsdSkelSkinningQuery(const UsdPrim& prim,
                         const SdfPathVector& skelJointOrder,
                         const UsdAttribute& jointIndices,
                         const UsdAttribute& jointWeights,
                         const UsdAttribute& geomBindTransform,
                         const std::shared_ptr<SdfPathVector>& jointOrder);

    /// Returns true if this query is valid.
    bool IsValid() const { return _valid; }
    
    /// Boolena conversion operator. Equivalent to IsValid().
    explicit operator bool() const { return IsValid(); }

    int GetNumInfluencesPerComponent() const {
        return _numInfluencesPerComponent;
    }

    const TfToken& GetInterpolation() const { return _interpolation; }

    USDSKEL_API
    bool IsRigidlyDeformed() const;

    const UsdAttribute& GetGeomBindTransformAttr() const {
        return _geomBindTransformAttr;
    }

    const UsdGeomPrimvar& GetJointIndicesPrimvar() const {
        return _jointIndicesPrimvar;
    }

    const UsdGeomPrimvar& GetJointWeightsPrimvar() const {
        return _jointWeightsPrimvar;
    }

    /// Return the mapper for this target, if any.
    /// This corresponds to the mapping of the joint order from
    /// the ordering on the skeleton to the order of a custom
    /// \em skel:joints relationships, set inside the hierarchy.
    const UsdSkelAnimMapperRefPtr& GetMapper() const { return _mapper; }

    const std::shared_ptr<SdfPathVector>& GetJointOrder() const {
        return _jointOrder;
    }

    /// Populate \p times with the union of time samples for all properties
    /// that affect skinning, independent of joint transforms and any
    /// other prim-specific properties (such as points).
    ///
    /// \sa UsdAttribute::GetTimeSamples
    USDSKEL_API
    bool GetTimeSamples(std::vector<double>* times) const;

    /// Populate \p times with the union of time samples within \p interval,
    /// for all properties that affect skinning, independent of joint   
    /// transforms and any other prim-specific properties (such as points).
    ///
    /// \sa UsdAttribute::GetTimeSamplesInInterval
    USDSKEL_API
    bool GetTimeSamplesInInterval(const GfInterval& interval,
                                  std::vector<double>* times) const;

    /// Convenience method for computing joint influences.
    /// In addition to querying influences, this will also perform
    /// validation of the basic form of the weight data -- although
    /// the array contents is not validated.
    USDSKEL_API
    bool ComputeJointInfluences(VtIntArray* indices,
                                VtFloatArray* weights,
                                UsdTimeCode time=UsdTimeCode::Default()) const;

    /// Convenience method for computing joint influence, where constant
    /// influences are expanded to hold values per point.
    /// In addition to querying influences, this will also perform
    /// validation of the basic form of the weight data -- although
    /// the array contents is not validated.
    USDSKEL_API
    bool ComputeVaryingJointInfluences(
             size_t numPoints,
             VtIntArray* indices,
             VtFloatArray* weights,
             UsdTimeCode time=UsdTimeCode::Default()) const;

    /// Compute skinned points using linear blend skinning.
    /// Both \p xforms and \p points are given in _skeleton space_,
    /// using the joint order of the bound skeleton.
    /// Joint influences and the (optional) binding transform are computed
    /// at time \p time (which will typically be unvarying).
    ///
    /// \sa UsdSkelSkeletonQuery::ComputeSkinningTransforms
    USDSKEL_API
    bool ComputeSkinnedPoints(const VtMatrix4dArray& xforms,
                              VtVec3fArray* points,
                              UsdTimeCode time=UsdTimeCode::Default()) const;

    /// Compute a skinning transform using linear blend skinning.
    /// The \p xforms are given in _skeleton space_, using the joint order of
    /// the bound skeleton.
    /// Joint influences and the (optional) binding transform are computed
    /// at time \p time (which will typically be unvarying).
    /// If this skinning query holds non-constant joint influences,
    /// no transform will be computed, and the function will return false.
    ///
    /// \sa UsdSkelSkeletonQuery::ComputeSkinningTransforms
    USDSKEL_API
    bool ComputeSkinnedTransform(const VtMatrix4dArray& xforms,
                                 GfMatrix4d* xform,
                                 UsdTimeCode time=UsdTimeCode::Default()) const;

    USDSKEL_API
    GfMatrix4d
    GetGeomBindTransform(UsdTimeCode time=UsdTimeCode::Default()) const;

    USDSKEL_API
    std::string GetDescription() const;

private:
    bool _valid;
    int _numInfluencesPerComponent;
    TfToken _interpolation;

    UsdGeomPrimvar _jointIndicesPrimvar;
    UsdGeomPrimvar _jointWeightsPrimvar;
    UsdAttribute _geomBindTransformAttr;
    UsdSkelAnimMapperRefPtr _mapper;
    std::shared_ptr<SdfPathVector> _jointOrder;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDSKEL_SKINNINGQUERY_H
