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
#ifndef USDSKEL_ANIMQUERY_H
#define USDSKEL_ANIMQUERY_H

/// \file usdSkel/animQuery.h

#include "pxr/pxr.h"
#include "pxr/usd/usdSkel/api.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/vt/types.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/timeCode.h"


PXR_NAMESPACE_OPEN_SCOPE


class GfMatrix4d;
class UsdAttribute;
class UsdSkelCache;


TF_DECLARE_REF_PTRS(UsdSkel_AnimQueryImpl);


/// \class UsdSkelAnimQuery
///
/// Class providing efficient queries of primitives that provide skel animation.
class UsdSkelAnimQuery
{
public:
    USDSKEL_API
    UsdSkelAnimQuery() {}

    UsdSkelAnimQuery(const UsdSkel_AnimQueryImplRefPtr& impl)
        :  _impl(impl) {}

    /// Return true if this query is valid.
    bool IsValid() const { return (bool)_impl; }

    /// Boolean conversion operator. Equivalent to IsValid().
    explicit operator bool() const { return IsValid(); }

    /// Equality comparison.  Return true if \a lhs and \a rhs represent the
    /// same UsdSkelAnimQuery, false otherwise.
    friend bool operator==(const UsdSkelAnimQuery& lhs,
                           const UsdSkelAnimQuery& rhs) {
        return lhs.GetPrim() == rhs.GetPrim();
    }

    /// Inequality comparison. Return false if \a lhs and \a rhs represent the
    /// same UsdSkelAnimQuery, true otherwise.
    friend bool operator!=(const UsdSkelAnimQuery& lhs,
                           const UsdSkelAnimQuery& rhs) {
        return !(lhs == rhs);
    }

    // hash_value overload for std/boost hash.
    friend size_t hash_value(const UsdSkelAnimQuery& query) {
        return hash_value(query.GetPrim());
    }

    /// Return the primitive this anim query reads from.
    USDSKEL_API
    UsdPrim GetPrim() const;

    /// Compute a root transform of the entire animation at \p time.
    USDSKEL_API
    bool ComputeTransform(GfMatrix4d* xform,
                          UsdTimeCode time=UsdTimeCode::Default()) const;

    /// Compute joint transforms in joint-local space.
    /// Transforms are returned in the order specified by the joint ordering
    /// of the animation primitive itself.
    USDSKEL_API
    bool ComputeJointLocalTransforms(
             VtMatrix4dArray* xforms,
             UsdTimeCode time=UsdTimeCode::Default()) const;

    /// Compute translation,rotation,scale components of the joint transforms
    /// in joint-local space. This is provided to facilitate direct streaming
    /// of animation data in a form that can efficiently be processed for
    /// animation blending.
    USDSKEL_API
    bool ComputeJointLocalTransformComponents(
             VtVec3fArray* translations,    
             VtQuatfArray* rotations,
             VtVec3hArray* scales,
             UsdTimeCode time=UsdTimeCode::Default()) const;

    USDSKEL_API
    bool ComputeBlendShapeWeights(
             VtFloatArray* weights,
             UsdTimeCode time=UsdTimeCode::Default()) const;
    
    /// Get the time samples at which values contributing to joint transforms
    /// are set. This only computes the time samples for sampling transforms in
    /// joint-local space, and does not include time samples affecting the
    /// root transformation.
    ///
    /// \sa UsdAttribute::GetTimeSamples
    USDSKEL_API
    bool GetJointTransformTimeSamples(std::vector<double>* times) const;

    /// Get the time samples at which values contributing to joint transforms
    /// are set, over \p interval. This only computes the time samples for
    /// sampling transforms in joint-local space, and does not include time
    /// samples affecting the root transformation.
    ///
    /// \sa UsdAttribute::GetTimeSamplesInInterval
    USDSKEL_API
    bool GetJointTransformTimeSamplesInInterval(const GfInterval& interval,
                                                std::vector<double>* times) const;

    /// Get the attributes contributing to JointTransform computations
    USDSKEL_API
    bool GetJointTransformAttributes(std::vector<UsdAttribute>* attrs) const;

    /// Return true if it possible, but not certain, that joint transforms  
    /// computed through this animation query change over time, false otherwise.
    ///
    /// \sa UsdAttribute::ValueMightBeTimeVayring
    USDSKEL_API
    bool JointTransformsMightBeTimeVarying() const;

    /// Return true if it possible, but not certain, that the root transform
    /// of the animation query changes over time, false otherwise.
    ///
    /// \sa UsdAttribute::ValueMightBeTimeVayring
    USDSKEL_API
    bool TransformMightBeTimeVarying() const;

    /// Returns an array of tokens describing the ordering of joints in the
    /// animation.
    ///
    /// \sa UsdSkelSkeleton::GetJointOrder
    USDSKEL_API
    VtTokenArray GetJointOrder() const;

    /// Returns an array of tokens describing the ordering of blend shape
    /// channels in the animation.
    USDSKEL_API
    VtTokenArray GetBlendShapeOrder() const;

    USDSKEL_API
    std::string GetDescription() const;

private:
    UsdSkel_AnimQueryImplRefPtr _impl;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDSKEL_ANIMQUERY_H
