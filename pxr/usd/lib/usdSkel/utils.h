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
#ifndef USDSKEL_UTILS_H
#define USDSKEL_UTILS_H

/// \file usdSkel/utils.h
///
/// Collection of utility methods.
///

#include "pxr/pxr.h"
#include "pxr/usd/usdSkel/api.h"

#include "pxr/base/gf/interval.h"
#include "pxr/base/gf/quatf.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3h.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/types.h"

#include "pxr/usd/sdf/path.h"

#include <cstddef>


PXR_NAMESPACE_OPEN_SCOPE


class GfMatrix3f;
class GfMatrix4d;
class GfRotation;
class UsdPrim;
class UsdPrimRange;
class UsdRelationship;
class UsdSkelRoot;
class UsdSkelTopology;


/// \defgroup UsdSkel_Utils Utilities
/// @{


/// Returns true if \p prim is a valid skel animation source.
USDSKEL_API
bool
UsdSkelIsSkelAnimationPrim(const UsdPrim& prim);


/// Returns true if \p prim is considered to be a skinnable primitive.
/// Whether or not the prim is actually skinned additionally depends on whether
/// or not the prim has a bound skeleton, and prop joint influences.
USDSKEL_API
bool
UsdSkelIsSkinnablePrim(const UsdPrim& prim);


/// \defgroup UsdSkel_JointTransformUtils Joint Transform Utilities
/// Utilities for working with vectorized joint transforms.
/// @{


/// Compute joint transforms in joint-local space.
/// Transforms are computed from \p xforms, holding concatenated
/// joint transforms, and \p inverseXforms, providing the inverse
/// of each of those transforms.
/// If the root transforms include an additional, external transformation
/// -- eg., such as the skel local-to-world transformation -- then the
/// inverse of that transform should be passed as \p rootInverseXform.
/// If no \p rootInverseXform is provided, then \p xform and \p inverseXforms
/// should be based on joint transforms computed in skeleton space.
/// Each transform array must be sized to the number of joints from \p topology.
USDSKEL_API
bool
UsdSkelComputeJointLocalTransforms(const UsdSkelTopology& topology,
                                   const VtMatrix4dArray& xforms,
                                   const VtMatrix4dArray& inverseXforms,
                                   VtMatrix4dArray* jointLocalXforms,
                                   const GfMatrix4d* rootInverseXform=nullptr);

/// \overload
/// Convenience overload that computes inverse transforms internally.
/// If the inverse of \p xforms are needed elsewhere, it is more efficine to
/// use the form of this method that takes inverse transforms as an argument,
/// and compute the inverse transforms separately.
USDSKEL_API
bool
UsdSkelComputeJointLocalTransforms(const UsdSkelTopology& topology,
                                   const VtMatrix4dArray& xforms,
                                   VtMatrix4dArray* jointLocalXforms,
                                   const GfMatrix4d* rootInverseXform=nullptr);


/// \overload
USDSKEL_API
bool
UsdSkelComputeJointLocalTransforms(const UsdSkelTopology& topology,
                                   const GfMatrix4d* xforms,
                                   const GfMatrix4d* inverseXforms,
                                   GfMatrix4d* jointLocalXforms,
                                   const GfMatrix4d* rootInverseXform=nullptr);



/// Compute concatenated joint transforms.
/// This concatenates transforms from \p jointLocalXforms, providing joint
/// transforms in joint-local space.
/// If \p rootXform is not provided, or is the identity, the resulting joint
/// transforms will be given in skeleton space. Any additional transformations
/// may be provided on \p rootXform if an additional level of transformation
/// -- such as the skel local to world transform -- are desired.
/// Each transform array must be sized to the number of joints from \p topology.
USDSKEL_API
bool
UsdSkelConcatJointTransforms(const UsdSkelTopology& topology,
                             const VtMatrix4dArray& jointLocalXforms,
                             VtMatrix4dArray* xforms,
                             const GfMatrix4d* rootXform=nullptr);


/// \overload
USDSKEL_API
bool
UsdSkelConcatJointTransforms(const UsdSkelTopology& topology,
                             const GfMatrix4d* jointLocalXforms,
                             GfMatrix4d* xforms,
                             const GfMatrix4d* rootXform=nullptr);


/// Compute an extent from a set of skel-space joint transform.
/// The \p rootXform may also be set to provide an additional root
/// transformation on top of all joints, which is useful for computing
/// extent relative to a different space.
USDSKEL_API
bool
UsdSkelComputeJointsExtent(const VtMatrix4dArray& joints,
                           VtVec3fArray* extent,
                           float pad=0.0f,
                           const GfMatrix4d* rootXform=nullptr);


/// \overload
USDSKEL_API
bool
UsdSkelComputeJointsExtent(const GfMatrix4d* xforms,
                           size_t numXforms,
                           VtVec3fArray* extent,
                           float pad=0.0f,
                           const GfMatrix4d* rootXform=nullptr);


/// @}


/// \defgroup UsdSkel_TransformCompositionUtils Transform Composition Utils
/// Utiltiies for converting transforms to and from component (translate,
/// rotate, scale) form.
/// @{


/// Decompose a transform into translate/rotate/scale components.
/// The transform order for decomposition is scale, rotate, translate.
USDSKEL_API
bool
UsdSkelDecomposeTransform(const GfMatrix4d& xform,
                          GfVec3f* translate,  
                          GfRotation* rotate,
                          GfVec3h* scale);

/// \overload
USDSKEL_API
bool
UsdSkelDecomposeTransform(const GfMatrix4d& xform,
                          GfVec3f* translate,  
                          GfQuatf* rotate,
                          GfVec3h* scale);


/// Decompose an array of transforms into translate/rotate/scale components.
USDSKEL_API
bool
UsdSkelDecomposeTransforms(const VtMatrix4dArray& xforms,
                           VtVec3fArray* translations,
                           VtQuatfArray* rotations,
                           VtVec3hArray* scales);


/// \overload
USDSKEL_API
bool
UsdSkelDecomposeTransforms(const GfMatrix4d* xforms,
                           GfVec3f* translations,
                           GfQuatf* rotations,
                           GfVec3h* scales,
                           size_t count);


/// Create a transform from translate/rotate/scale components.
/// This performs the inverse of UsdSkelDecomposeTransform.
USDSKEL_API
GfMatrix4d
UsdSkelMakeTransform(const GfVec3f& translate,
                     const GfMatrix3f& rotate,
                     const GfVec3h& scale);

/// \overload
USDSKEL_API
GfMatrix4d
UsdSkelMakeTransform(const GfVec3f& translate,
                     const GfQuatf& rotate,
                     const GfVec3h& scale);

/// Create transforms from arrays of components.
USDSKEL_API
bool
UsdSkelMakeTransforms(const VtVec3fArray& translations,
                      const VtQuatfArray& rotations,
                      const VtVec3hArray& scales,
                      VtMatrix4dArray* xforms);

/// \overload
USDSKEL_API
bool
UsdSkelMakeTransforms(const GfVec3f* translations,
                      const GfQuatf* rotations,
                      const GfVec3h* scales,
                      GfMatrix4d* xforms,
                      size_t count);


/// @}


/// \defgroup UsdSkel_JointInfluenceUtils Joint Influence Utils
/// Collection of methods for working \ref UsdSkel_Terminology_JointInfluences
/// "joint influences", as stored through UsdSkelBindingAPI.`
/// @{


/// Helper method to normalize weight values across each consecutive run of
/// \p numInfluencesPerComponent elements.
USDSKEL_API
bool
UsdSkelNormalizeWeights(VtFloatArray* weights, int numInfluencesPerComponent);


/// Sort joint influences such that highest weight values come first.
USDSKEL_API
bool
UsdSkelSortInfluences(VtIntArray* indices, VtFloatArray* weights,
                      int numInfluencesPerComponent);


/// Convert an array of constant influences (joint weights or indices)
/// to an array of varying influences.
/// The \p size should match the size of required for 'vertex' interpolation
/// on the type geometry primitive. Typically, this is the number of points.
/// This is a convenience function for clients that don't understand
/// constant (rigid) weighting.
USDSKEL_API
bool
UsdSkelExpandConstantInfluencesToVarying(VtIntArray* indices, size_t size);

/// \overload
USDSKEL_API
bool
UsdSkelExpandConstantInfluencesToVarying(VtFloatArray* weights, size_t size);

/// Resize the number of influences per component in a weight or indices array,
/// which initially has \p srcNumInfluencesPerComponent influences to have
/// no more than \p newNumInfluencesPerComponent influences per component.
/// If the size decreases, influences are additionally re-normalized.
/// This is a convenience method for clients that require a fixed number of
/// of influences.
USDSKEL_API
bool
UsdSkelResizeInfluences(VtIntArray* indices,
                        int srcNumInfluencesPerComponent,
                        int newNumInfluencesPerComponent);

/// \overload
USDSKEL_API
bool
UsdSkelResizeInfluences(VtFloatArray* weights,
                        int srcNumInfluencesPerComponent,
                        int newNumInfluencesPerComponent);


/// @}


/// \defgroup UsdSkel_SkinningUtils Skinning Implementations
/// Reference skinning implementations for skinning points and transforms.
/// @{


/// Skin points using linear blend skinning (LBS).
/// The \p jointXforms are \ref UsdSkel_Term_SkinningTransforms
/// "skinning transforms", given in _skeleton space_, while the
/// \p geomBindTransform provides the transform that transforms the initial
/// \p points into the same _skeleton space_ that the skinning transforms
/// were computed in.
USDSKEL_API
bool
UsdSkelSkinPointsLBS(const GfMatrix4d& geomBindTransform,
                     const VtMatrix4dArray& jointXforms,
                     const VtIntArray& jointIndices,
                     const VtFloatArray& jointWeights,
                     int numInfluencesPerPoint,
                     VtVec3fArray* points);


/// \overload
USDSKEL_API
bool
UsdSkelSkinPointsLBS(const GfMatrix4d& geomBindTransform,
                     const GfMatrix4d* jointXforms,
                     size_t numJoints,
                     const int* jointIndices,
                     const float* jointWeights,
                     size_t numInfluences,
                     int numInfluencesPerPoint,
                     GfVec3f* points,
                     size_t numPoints,
                     bool forceSerial=false);


/// Skin a transform using linear blend skinning (LBS).
/// The \p jointXforms are \ref UsdSkel_Term_SkinningTransforms
/// "skinning transforms", given in _skeleton space_, while the
/// \p geomBindTransform provides the transform that initially places
/// a primitive in that same _skeleton space_.
USDSKEL_API
bool
UsdSkelSkinTransformLBS(const GfMatrix4d& geomBindTransform,
                        const VtMatrix4dArray& jointXforms,
                        const VtIntArray& jointIndices,
                        const VtFloatArray& jointWeights,
                        GfMatrix4d* xform);


/// \overload
USDSKEL_API
bool
UsdSkelSkinTransformLBS(const GfMatrix4d& geomBindTransform,
                        const GfMatrix4d* jointXforms,
                        size_t numJoints,
                        const int* jointIndices,
                        const float* jointWeights,
                        size_t numInfluences,
                        GfMatrix4d* xform);


/// Bake the effect of skinning prims directly into points and transforms,
/// over \p interval.
/// This is intended to serve as a complete reference implementation,
/// providing a ground truth for testing and validation purposes.
///
/// \warning Since baking the effect of skinning will undo the IO gains that
/// deferred skeletal posing provides, this method should not be used except
/// for testing. It also has been written with an emphasis on correctness rather
/// than performance, and is not expected to scale. Usage should be limited to
/// testing and towards conversion when transmitting the resulting of skinning
/// to an application that does not have an equivalent skinning representation.
USDSKEL_API
bool
UsdSkelBakeSkinning(const UsdSkelRoot& root,
                    const GfInterval& interval=GfInterval::GetFullInterval());


/// Overload of UsdSkelBakeSkinning, which bakes the effect of skinning prims
/// directly into points and transforms, for all SkelRoot prims in \p range.
USDSKEL_API
bool
UsdSkelBakeSkinning(const UsdPrimRange& range,
                    const GfInterval& interval=GfInterval::GetFullInterval());



/// @}


/// @}


PXR_NAMESPACE_CLOSE_SCOPE


#endif // USDSKEL_UTILS_H
