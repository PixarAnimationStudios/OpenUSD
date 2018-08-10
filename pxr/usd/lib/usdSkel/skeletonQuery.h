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
#ifndef USDSKEL_SKELETONQUERY_H
#define USDSKEL_SKELETONQUERY_H

/// \file usdSkel/skeletonQuery.h

#include "pxr/pxr.h"
#include "pxr/usd/usdSkel/api.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/vt/types.h"

#include "pxr/usd/usd/prim.h"

#include "pxr/usd/usdSkel/animMapper.h"
#include "pxr/usd/usdSkel/animQuery.h"


PXR_NAMESPACE_OPEN_SCOPE

class UsdGeomXformCache;
class UsdSkelSkeleton;
class UsdSkelTopology;

TF_DECLARE_REF_PTRS(UsdSkel_SkelDefinition);


/// \class UsdSkelSkeletonQuery
///
/// Primary interface to reading *bound* skeleton data.
/// This is used to query properties such as resolved transforms and animation
/// bindings, as bound through the UsdSkelBindingAPI.
///
/// A UsdSkelSkeletonQuery can not be constructed directly, and instead must be
/// constructed through a UsdSkelCache instance. This is done as follows:
///
/// \code
/// // Global cache, intended to persist.
/// UsdSkelCache skelCache;
/// // Populate the cache for a skel root.
/// skelCache.Populate(UsdSkelRoot(skelRootPrim));
///
/// if (UsdSkelSkeletonQuery skelQuery = skelCache.GetSkelQuery(skelPrim)) {
///     ...
/// }
/// \endcode
///
class UsdSkelSkeletonQuery
{
public:
    UsdSkelSkeletonQuery() {}

    /// Return true if this query is valid.
    bool IsValid() const { return _definition; }

    /// Boolean conversion operator. Equivalent to IsValid().
    explicit operator bool() const { return IsValid(); }

    /// Equality comparison.  Return true if \a lhs and \a rhs represent the
    /// same UsdSkelSkeletonQuery, false otherwise.
    friend bool operator==(const UsdSkelSkeletonQuery& lhs,
                           const UsdSkelSkeletonQuery& rhs) {
        return lhs._definition == rhs._definition &&
               lhs._animQuery == rhs._animQuery;
    }

    /// Inequality comparison. Return false if \a lhs and \a rhs represent the
    /// same UsdSkelSkeletonQuery, true otherwise.
    friend bool operator!=(const UsdSkelSkeletonQuery& lhs,
                           const UsdSkelSkeletonQuery& rhs) {
        return !(lhs == rhs);
    }

    // hash_value overload for std/boost hash.
    USDSKEL_API
    friend size_t hash_value(const UsdSkelSkeletonQuery& query);

    /// Returns the underlying Skeleton primitive corresponding to the
    /// bound skeleton instance, if any.
    USDSKEL_API
    UsdPrim GetPrim() const;

    /// Returns the bound skeleton instance, if any.
    USDSKEL_API
    const UsdSkelSkeleton& GetSkeleton() const;
    
    /// Returns the animation query that provides animation for the
    /// bound skeleton instance, if any.
    USDSKEL_API
    const UsdSkelAnimQuery& GetAnimQuery() const;
    
    /// Returns the topology of the bound skeleton instance, if any.
    USDSKEL_API
    const UsdSkelTopology& GetTopology() const;

    /// Returns an arrray of joint paths, given as tokens, describing
    /// the order and parent-child relationships of joints in the skeleton.
    ///
    /// \sa UsdSkelSkeleton::GetJointOrder
    USDSKEL_API
    VtTokenArray GetJointOrder() const;

    /// Compute joint transforms in joint-local space, at \p time.
    /// This returns transforms in joint order of the skeleton.
    /// If \p atRest is false and an animation source is bound, local transforms
    /// defined by the animation are mapped into the skeleton's joint order.
    /// Any transforms not defined by the animation source use the transforms
    /// from the rest pose as a fallback value.
    /// If valid transforms cannot be computed for the animation source, the
    /// \p xforms are instead set to the rest transforms.
    USDSKEL_API
    bool ComputeJointLocalTransforms(VtMatrix4dArray* xforms,
                                     UsdTimeCode time,
                                     bool atRest=false) const;

    /// Compute joint transforms in skeleton space, at \p time.
    /// This concatenates joint transforms as computed from
    /// ComputeJointLocalTransforms(). If \p atRest is true, any bound animation
    /// source is ignored, and transforms are computed from the rest pose.
    /// The skeleton-space transforms of the rest pose are cached internally.
    USDSKEL_API
    bool ComputeJointSkelTransforms(VtMatrix4dArray* xforms,
                                    UsdTimeCode time,
                                    bool atRest=false) const;

    /// Compute joint transforms in world space, at whatever time is configured
    /// on \p xfCache.
    /// This is equivalent to computing skel-space joint transforms with
    /// CmoputeJointSkelTransforms(), and then concatenating all transforms
    /// by the local-to-world transform of the Skeleton prim.
    /// If \p atRest is true, any bound animation source is ignored, and
    /// transforms are computed from the rest pose.
    USDSKEL_API
    bool ComputeJointWorldTransforms(VtMatrix4dArray* xforms,
                                     UsdGeomXformCache* xfCache,
                                     bool atRest=false) const;

    /// Compute transforms representing the change in transformation
    /// of a joint from its rest pose, in skeleton space.
    ///
    /// I.e.,
    /// \code
    ///     inverse(bindTransform)*jointTransform
    /// \endcode
    ///
    /// These are the transforms usually required for skinning.
    USDSKEL_API
    bool ComputeSkinningTransforms(VtMatrix4dArray* xforms,
                                   UsdTimeCode time) const;

    /// Returns the world space joint transforms at bind time.
    USDSKEL_API
    bool GetJointWorldBindTransforms(VtMatrix4dArray* xforms) const;

    USDSKEL_API
    std::string GetDescription() const;

private:

    USDSKEL_API
    UsdSkelSkeletonQuery(const UsdSkel_SkelDefinitionRefPtr& definition,
                         const UsdSkelAnimQuery& anim=UsdSkelAnimQuery());

    bool _HasMappableAnim() const;

    bool _ComputeJointLocalTransforms(VtMatrix4dArray* xforms,
                                      UsdTimeCode time,
                                      bool atRest=false) const;

    bool _ComputeJointSkelTransforms(VtMatrix4dArray* xforms,
                                     UsdTimeCode time,
                                     bool atRest=false) const;

    bool _ComputeSkinningTransforms(VtMatrix4dArray* xforms,
                                    UsdTimeCode time) const;

private:
    UsdSkel_SkelDefinitionRefPtr _definition;
    UsdSkelAnimQuery _animQuery;
    UsdSkelAnimMapper _animToSkelMapper;

    friend class UsdSkel_CacheImpl;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDSKEL_SKELETONQUERY_H
